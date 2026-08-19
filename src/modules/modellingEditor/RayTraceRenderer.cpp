#include "RayTraceRenderer.h"

#include <QtGlobal>
#include <cmath>
#include <algorithm>

namespace ks {

namespace {
const QVector3D kSunDir = QVector3D(0.45f, 0.85f, 0.28f).normalized();
const float kAmbient = 0.32f;

inline float clamp01(float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }
inline float QColorLuma(const QColor& c) { return 0.2126f * c.redF() + 0.7152f * c.greenF() + 0.0722f * c.blueF(); }

inline unsigned xorshift(unsigned& s) { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
inline float rnd(unsigned& s) { return (xorshift(s) & 0x00FFFFFF) / 16777216.0f; }

QVector3D randomCosHemisphere(unsigned& s, const QVector3D& n)
{
    QVector3D u = std::abs(n.y()) < 0.999f
        ? QVector3D::crossProduct(n, QVector3D(0.0f, 1.0f, 0.0f)).normalized()
        : QVector3D(1.0f, 0.0f, 0.0f);
    const QVector3D v = QVector3D::crossProduct(u, n).normalized();
    const float phi = 2.0f * float(M_PI) * rnd(s);
    const float cost = std::sqrt(rnd(s));
    const float sint = std::sqrt(1.0f - cost * cost);
    return (u * (std::cos(phi) * sint) + v * (std::sin(phi) * sint) + n * cost).normalized();
}

QVector3D sampleSunDir(unsigned& s)
{
    QVector3D u = std::abs(kSunDir.y()) < 0.999f
        ? QVector3D::crossProduct(kSunDir, QVector3D(0.0f, 1.0f, 0.0f)).normalized()
        : QVector3D(1.0f, 0.0f, 0.0f);
    const QVector3D v = QVector3D::crossProduct(u, kSunDir).normalized();
    const float r = std::sqrt(rnd(s));
    const float phi = 2.0f * float(M_PI) * rnd(s);
    const float spread = 0.04f; // sun angular half-width
    return (kSunDir + (u * std::cos(phi) + v * std::sin(phi)) * r * spread).normalized();
}

// Linear interpolation of a normalized vertical IES curve (index 0..90 == 0..90
// degrees from the light forward axis). Empty curve -> 1.0.
inline float iesAt(const QVector<float>& curve, float angleDeg)
{
    if (curve.isEmpty())
        return 1.0f;
    const float x = qBound(0.0f, angleDeg, 90.0f);
    if (x <= 0.0f) return curve.first();
    if (x >= 90.0f) return curve.last();
    const float f = x * (curve.size() - 1) / 90.0f;
    const int i = std::min(int(f), int(curve.size()) - 2);
    const float t = f - i;
    return curve[i] * (1.0f - t) + curve[i + 1] * t;
}
} // namespace

void RayTraceRenderer::setObjects(const QVector<RTTriangle>& tris)
{
    m_tris = tris;
    m_triIndices.clear();
    m_triIndices.reserve(m_tris.size());
    for (int i = 0; i < m_tris.size(); ++i)
        m_triIndices.append(i);
    m_bvh.clear();
    if (!m_tris.isEmpty())
        buildNode(0, m_triIndices.size());
}

RayTraceRenderer::AABB RayTraceRenderer::boundsOf(const RTTriangle& t)
{
    AABB b;
    b.min = t.v0;
    b.max = t.v0;
    b.min = QVector3D(std::min(b.min.x(), std::min(t.v1.x(), t.v2.x())),
                      std::min(b.min.y(), std::min(t.v1.y(), t.v2.y())),
                      std::min(b.min.z(), std::min(t.v1.z(), t.v2.z())));
    b.max = QVector3D(std::max(b.max.x(), std::max(t.v1.x(), t.v2.x())),
                      std::max(b.max.y(), std::max(t.v1.y(), t.v2.y())),
                      std::max(b.max.z(), std::max(t.v1.z(), t.v2.z())));
    return b;
}

RayTraceRenderer::AABB RayTraceRenderer::unionBox(const AABB& a, const AABB& b)
{
    AABB r;
    r.min = QVector3D(std::min(a.min.x(), b.min.x()), std::min(a.min.y(), b.min.y()), std::min(a.min.z(), b.min.z()));
    r.max = QVector3D(std::max(a.max.x(), b.max.x()), std::max(a.max.y(), b.max.y()), std::max(a.max.z(), b.max.z()));
    return r;
}

int RayTraceRenderer::buildNode(int begin, int end)
{
    const int nodeIdx = m_bvh.size();
    m_bvh.append(BVHNode());

    // Bounding box of the range.
    AABB box;
    bool first = true;
    for (int i = begin; i < end; ++i) {
        AABB b = boundsOf(m_tris[m_triIndices[i]]);
        if (first) { box = b; first = false; }
        else box = unionBox(box, b);
    }
    m_bvh[nodeIdx].box = box;
    m_bvh[nodeIdx].triStart = begin;
    m_bvh[nodeIdx].triCount = end - begin;

    if (end - begin <= 4)
        return nodeIdx;

    // Split along the largest axis at the median centroid.
    QVector3D extent = box.max - box.min;
    int axis = 0;
    if (extent.y() >= extent.x() && extent.y() >= extent.z()) axis = 1;
    else if (extent.z() >= extent.x() && extent.z() >= extent.y()) axis = 2;

    int mid = begin + (end - begin) / 2;
    std::nth_element(m_triIndices.begin() + begin, m_triIndices.begin() + mid, m_triIndices.begin() + end,
                     [&](int a, int b) {
                         const QVector3D& ca = m_tris[a].v0;
                         const QVector3D& cb = m_tris[b].v0;
                         if (axis == 0) return ca.x() < cb.x();
                         if (axis == 1) return ca.y() < cb.y();
                         return ca.z() < cb.z();
                     });

    m_bvh[nodeIdx].left = buildNode(begin, mid);
    m_bvh[nodeIdx].right = buildNode(mid, end);
    m_bvh[nodeIdx].triStart = 0;
    m_bvh[nodeIdx].triCount = 0;
    return nodeIdx;
}

bool RayTraceRenderer::rayBox(const QVector3D& o, const QVector3D& d, const AABB& b, float tMax) const
{
    float tmin = 0.0f, tmax = tMax;
    for (int i = 0; i < 3; ++i) {
        float oi = i == 0 ? o.x() : (i == 1 ? o.y() : o.z());
        float di = i == 0 ? d.x() : (i == 1 ? d.y() : d.z());
        float bi0 = i == 0 ? b.min.x() : (i == 1 ? b.min.y() : b.min.z());
        float bi1 = i == 0 ? b.max.x() : (i == 1 ? b.max.y() : b.max.z());
        if (std::abs(di) < 1e-8f) {
            if (oi < bi0 || oi > bi1) return false;
            continue;
        }
        float inv = 1.0f / di;
        float t0 = (bi0 - oi) * inv;
        float t1 = (bi1 - oi) * inv;
        if (t0 > t1) std::swap(t0, t1);
        tmin = std::max(tmin, t0);
        tmax = std::min(tmax, t1);
        if (tmax < tmin) return false;
    }
    return true;
}

bool RayTraceRenderer::intersectTri(const QVector3D& o, const QVector3D& d, const RTTriangle& t, float& tHit, float& u, float& v)
{
    const QVector3D e1 = t.v1 - t.v0;
    const QVector3D e2 = t.v2 - t.v0;
    const QVector3D p = QVector3D::crossProduct(d, e2);
    const float det = QVector3D::dotProduct(e1, p);
    if (det > -1e-8f && det < 1e-8f) return false;
    const float invDet = 1.0f / det;
    const QVector3D s = o - t.v0;
    u = QVector3D::dotProduct(s, p) * invDet;
    if (u < -1e-6f || u > 1.0f + 1e-6f) return false;
    const QVector3D q = QVector3D::crossProduct(s, e1);
    v = QVector3D::dotProduct(d, q) * invDet;
    if (v < -1e-6f || u + v > 1.0f + 1e-6f) return false;
    tHit = QVector3D::dotProduct(e2, q) * invDet;
    return tHit > 1e-5f;
}

void RayTraceRenderer::intersectNode(const QVector3D& o, const QVector3D& d, int node, float tMax, Hit& out) const
{
    const BVHNode& n = m_bvh[node];
    if (!rayBox(o, d, n.box, tMax)) return;
    if (n.triCount > 0) {
        for (int i = n.triStart; i < n.triStart + n.triCount; ++i) {
            float tHit, u, v;
            if (intersectTri(o, d, m_tris[m_triIndices[i]], tHit, u, v) && tHit < tMax) {
                tMax = tHit;
                out.hit = true;
                out.t = tHit;
                out.tri = m_triIndices[i];
                out.u = u;
                out.v = v;
            }
        }
        return;
    }
    intersectNode(o, d, n.left, tMax, out);
    intersectNode(o, d, n.right, out.hit ? out.t : tMax, out);
}

void RayTraceRenderer::intersect(const QVector3D& o, const QVector3D& d, float tMin, float tMax, Hit& out) const
{
    out = Hit();
    if (m_bvh.isEmpty()) return;
    // tMin unused: sky/ground handled by caller.
    Q_UNUSED(tMin);
    intersectNode(o, d, 0, tMax, out);
}

QRgb RayTraceRenderer::shadeBackground(const QVector3D& d) const
{
    const float h = clamp01(d.y() * 0.5f + 0.5f);
    const float horizon = 0.30f;
    const float top = 0.10f;
    if (d.y() < 0.0f)
        return qRgb(22, 24, 30);
    const float t = clamp01((h - horizon) / (1.0f - horizon - top));
    const int r = int(90 + (28 - 90) * t);
    const int g = int(105 + (40 - 105) * t);
    const int b = int(140 + (58 - 140) * t);
    return qRgb(r, g, b);
}

QRgb RayTraceRenderer::shade(const QVector3D& o, const QVector3D& d, const Hit& h) const
{
    const RTTriangle& t = m_tris[h.tri];
    const QVector3D n = (t.n1 - t.n0) * h.u + (t.n2 - t.n0) * h.v + t.n0;
    const QVector3D N = n.normalized();

    const QVector3D P = o + d * h.t;

    // Scene lights drive the shading; without them fall back to the original
    // hard-coded sun/ambient (keeps the classic preview look and legacy tests).
    if (!m_lights.isEmpty()) {
        const QVector3D lin = shadePoint(P, N, -d, t.metalness, t.roughness, t.color);
        return qRgb(int(clamp01(lin.x()) * 255.0f),
                    int(clamp01(lin.y()) * 255.0f),
                    int(clamp01(lin.z()) * 255.0f));
    }

    QVector3D L = kSunDir;

    // Hard shadow ray (skip self).
    bool inShadow = false;
    if (!m_bvh.isEmpty()) {
        Hit sh;
        intersect(P + N * 1e-3f, L, 0.0f, 1e9f, sh);
        inShadow = sh.hit;
    }

    const float ndl = std::max(0.0f, QVector3D::dotProduct(N, L));
    const QColor& base = t.color;
    float lit = kAmbient + (inShadow ? 0.0f : ndl * (1.0f - kAmbient));

    // Simple Blinn-Phong specular.
    float spec = 0.0f;
    if (!inShadow && ndl > 0.0f) {
        const QVector3D V = -d;
        const QVector3D H = (L - d).normalized();
        const float ndh = std::max(0.0f, QVector3D::dotProduct(N, H));
        const float shininess = 6.0f + (1.0f - t.roughness) * 60.0f;
        spec = std::pow(ndh, shininess) * (0.5f * t.metalness + 0.15f);
    }

    const float r = clamp01(base.redF() * lit + spec) * 255.0f;
    const float g = clamp01(base.greenF() * lit + spec) * 255.0f;
    const float bl = clamp01(base.blueF() * lit + spec) * 255.0f;
    return qRgb(int(r), int(g), int(bl));
}

QVector3D RayTraceRenderer::shadePoint(const QVector3D& P, const QVector3D& N, const QVector3D& V,
                                       float metalness, float roughness, const QColor& base) const
{
    const float baseR = base.redF(), baseG = base.greenF(), baseB = base.blueF();
    QVector3D lit(baseR * kAmbient, baseG * kAmbient, baseB * kAmbient);

    for (const RTLight& l : m_lights) {
        QVector3D L;
        float atten = 1.0f;
        float cone = 1.0f;
        float ies = 1.0f;
        float maxDist = 1e9f;
        float coneAngleDeg = 0.0f;

        if (l.type == 0) {
            L = -l.direction;
            maxDist = sceneRadius() * 10.0f;
        } else {
            const QVector3D toLight = l.position - P;
            const float dist = toLight.length();
            if (dist < 1e-4f) continue;
            L = toLight / dist;
            maxDist = dist - 1e-3f;
            atten = std::pow(std::max(0.0f, 1.0f - dist / std::max(1e-4f, l.range)), 2.0f);
            if (atten <= 0.0f) continue;
            const float cosA = qBound(-1.0f, QVector3D::dotProduct(l.direction, -L), 1.0f);
            coneAngleDeg = std::acos(cosA) * 180.0f / float(M_PI);
            if (l.type == 2 || l.type == 3) {
                const float outerRad = l.spotAngleDeg * float(M_PI) / 180.0f;
                const float innerRad = std::max(0.0f, (l.spotAngleDeg - l.spotPenumbraDeg)) * float(M_PI) / 180.0f;
                const float cosOuter = std::cos(outerRad);
                const float cosInner = std::cos(innerRad);
                cone = clamp01((cosA - cosOuter) / std::max(1e-5f, cosInner - cosOuter));
                if (cone <= 0.0f) continue;
            }
            if (!l.iesCurve.isEmpty())
                ies = iesAt(l.iesCurve, coneAngleDeg);
        }

        const float ndl = std::max(0.0f, QVector3D::dotProduct(N, L));
        if (ndl <= 0.0f) continue;

        bool occluded = false;
        if (!m_bvh.isEmpty()) {
            Hit sh;
            intersect(P + N * 1e-3f, L, 0.0f, maxDist, sh);
            occluded = sh.hit;
        }
        if (occluded) continue;

        const float intensity = ndl * atten * cone * ies * l.intensity;
        const float lr = l.color.redF() * intensity;
        const float lg = l.color.greenF() * intensity;
        const float lb = l.color.blueF() * intensity;

        lit.setX(lit.x() + baseR * lr);
        lit.setY(lit.y() + baseG * lg);
        lit.setZ(lit.z() + baseB * lb);

        // Blinn-Phong specular (metalness / roughness driven).
        const QVector3D H = (L + V).normalized();
        const float ndh = std::max(0.0f, QVector3D::dotProduct(N, H));
        const float shininess = 6.0f + (1.0f - roughness) * 60.0f;
        const float spec = std::pow(ndh, shininess) * (0.5f * metalness + 0.15f);
        lit.setX(lit.x() + lr * spec);
        lit.setY(lit.y() + lg * spec);
        lit.setZ(lit.z() + lb * spec);
    }
    return lit;
}

float RayTraceRenderer::sceneRadius() const
{
    if (m_bvh.isEmpty())
        return 1.0f;
    const AABB box = m_bvh[0].box;
    const float dx = box.max.x() - box.min.x();
    const float dy = box.max.y() - box.min.y();
    const float dz = box.max.z() - box.min.z();
    const float diag = std::sqrt(dx * dx + dy * dy + dz * dz);
    return diag > 1e-4f ? diag : 1.0f;
}

float RayTraceRenderer::sampleAO(const QVector3D& P, const QVector3D& N, unsigned& seed) const
{
    const float radius = sceneRadius() * 0.15f;
    if (radius <= 0.0f)
        return 1.0f;
    if (m_bvh.isEmpty())
        return 1.0f;
    const int nsamp = 16;
    int occluded = 0;
    for (int i = 0; i < nsamp; ++i) {
        const QVector3D dir = randomCosHemisphere(seed, N);
        Hit sh;
        intersect(P + N * 1e-3f, dir, 0.0f, radius, sh);
        if (sh.hit)
            ++occluded;
    }
    return 1.0f - float(occluded) / float(nsamp);
}

QVector3D RayTraceRenderer::evalPass(const QVector3D& o, const QVector3D& d, const Hit& h,
                                     RenderPass pass, float farDist, unsigned& seed) const
{
    if (!h.hit) {
        switch (pass) {
        case RenderPass::Depth:         return QVector3D(0.0f, 0.0f, 0.0f);
        case RenderPass::AmbientOcclusion:
        case RenderPass::Diffuse:       return QVector3D(1.0f, 1.0f, 1.0f);
        case RenderPass::Normal:        return QVector3D(0.55f, 0.55f, 1.0f);
        default:                        break;
        }
    }

    const RTTriangle& t = m_tris[h.tri];
    const QVector3D n = (t.n1 - t.n0) * h.u + (t.n2 - t.n0) * h.v + t.n0;
    const QVector3D N = n.normalized();
    const QVector3D P = o + d * h.t;

    switch (pass) {
    case RenderPass::Depth: {
        const float v = clamp01(h.t / farDist);
        return QVector3D(1.0f - v, 1.0f - v, 1.0f - v);
    }
    case RenderPass::AmbientOcclusion: {
        const float ao = sampleAO(P, N, seed);
        return QVector3D(ao, ao, ao);
    }
    case RenderPass::Diffuse: {
        float r = t.color.redF(), g = t.color.greenF(), b = t.color.blueF();
        // Linear parameters are expected in sRGB texture space; keep raw so
        // the albedo pass matches the viewport swatch rather than tonemapping.
        return QVector3D(clamp01(r), clamp01(g), clamp01(b));
    }
    case RenderPass::Normal: {
        return QVector3D(N.x() * 0.5f + 0.5f, N.y() * 0.5f + 0.5f, N.z() * 0.5f + 0.5f);
    }
    default:
        break;
    }
    return QVector3D(0.0f, 0.0f, 0.0f);
}

QImage RayTraceRenderer::render(const RTCamera& cam, int width, int height, RenderPass pass, int samples) const
{
    QImage img(width, height, QImage::Format_RGB32);
    if (width <= 0 || height <= 0) return img;

    const int spp = qBound(1, samples, 64);
    const float inv = 1.0f / float(spp * spp);

    const QVector3D forward = (cam.target - cam.eye).normalized();
    const QVector3D up = QVector3D(0.0f, 1.0f, 0.0f);
    QVector3D right = QVector3D::crossProduct(forward, up).normalized();
    if (right.lengthSquared() < 1e-6f) right = QVector3D(1.0f, 0.0f, 0.0f);
    const QVector3D trueUp = QVector3D::crossProduct(right, forward).normalized();

    const float fovRad = cam.fovDeg * float(M_PI) / 180.0f;
    const float tanHalf = std::tan(fovRad * 0.5f);
    const float aspect = float(width) / float(height);
    const float farDist = (cam.target - cam.eye).length() * 0.5f + sceneRadius() * 2.0f;

    for (int y = 0; y < height; ++y) {
        QRgb* row = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (int x = 0; x < width; ++x) {
            if (pass == RenderPass::Color) {
                if (spp <= 1) {
                    const float px = (2.0f * (x + 0.5f) / width - 1.0f) * tanHalf * aspect;
                    const float py = (1.0f - 2.0f * (y + 0.5f) / height) * tanHalf;
                    const QVector3D dir = (forward + right * px + trueUp * py).normalized();

                    QRgb color;
                    // Ground plane at y = 0 (grid) first, then meshes.
                    if (dir.y() < -1e-5f) {
                        const float tPlane = -cam.eye.y() / dir.y();
                        if (tPlane > 0.0f) {
                            const QVector3D P = cam.eye + dir * tPlane;
                            // Shadow on ground.
                            bool inShadow = false;
                            if (!m_bvh.isEmpty()) {
                                Hit sh;
                                intersect(P + QVector3D(0.0f, 1e-3f, 0.0f), kSunDir, 0.0f, 1e9f, sh);
                                inShadow = sh.hit;
                            }
                            const int cell = 1;
                            const int gx = int(std::floor(P.x() / cell));
                            const int gz = int(std::floor(P.z() / cell));
                            const bool grid = ((gx + gz) & 1) == 0;
                            float base = inShadow ? 0.16f : (grid ? 0.42f : 0.30f);
                            const float fog = clamp01(1.0f - tPlane / 30.0f);
                            const int gr = int((base * 255.0f) * fog + 22 * (1.0f - fog));
                            const int gg = int((base * 255.0f) * fog + 26 * (1.0f - fog));
                            const int gb = int((base * 255.0f) * fog + 30 * (1.0f - fog));
                            color = qRgb(gr, gg, gb);
                        } else {
                            color = shadeBackground(dir);
                        }
                    } else {
                        color = shadeBackground(dir);
                    }

                    Hit h;
                    intersect(cam.eye, dir, 0.0f, 1e9f, h);
                    if (h.hit) color = shade(cam.eye, dir, h);

                    row[x] = color;
                    continue;
                }

                // Supersampled color preview (matches renderFinal's integrator).
                float accR = 0.0f, accG = 0.0f, accB = 0.0f;
                const unsigned baseSeed = unsigned(y * 7741 + x * 131 + 7);
                for (int sy = 0; sy < spp; ++sy) {
                    for (int sx = 0; sx < spp; ++sx) {
                        unsigned seed = baseSeed ^ unsigned(sy * 397 + sx * 1009 + 1);
                        const float jx = (x + rnd(seed)) / width;
                        const float jy = (y + rnd(seed)) / height;
                        const float px = (2.0f * jx - 1.0f) * tanHalf * aspect;
                        const float py = (1.0f - 2.0f * jy) * tanHalf;
                        const QVector3D dir = (forward + right * px + trueUp * py).normalized();
                        const QVector3D c = traceRay(cam.eye, dir, 0);
                        accR += c.x(); accG += c.y(); accB += c.z();
                    }
                }
                float r = 1.0f - std::exp(-accR * inv);
                float g = 1.0f - std::exp(-accG * inv);
                float b = 1.0f - std::exp(-accB * inv);
                r = std::pow(clamp01(r), 1.0f / 2.2f);
                g = std::pow(clamp01(g), 1.0f / 2.2f);
                b = std::pow(clamp01(b), 1.0f / 2.2f);
                row[x] = qRgb(int(r * 255.0f), int(g * 255.0f), int(b * 255.0f));
                continue;
            }

            // Non-color AOV passes.
            float accR = 0.0f, accG = 0.0f, accB = 0.0f;
            const unsigned baseSeed = unsigned(y * 7741 + x * 131 + 7);
            for (int sy = 0; sy < spp; ++sy) {
                for (int sx = 0; sx < spp; ++sx) {
                    unsigned seed = baseSeed ^ unsigned(sy * 397 + sx * 1009 + 1);
                    for (int i = 0; i < 4; ++i)
                        xorshift(seed); // decorrelate per-pass pixel hashes
                    const float jx = (x + (spp > 1 ? rnd(seed) : 0.5f)) / width;
                    const float jy = (y + (spp > 1 ? rnd(seed) : 0.5f)) / height;
                    const float px = (2.0f * jx - 1.0f) * tanHalf * aspect;
                    const float py = (1.0f - 2.0f * jy) * tanHalf;
                    const QVector3D dir = (forward + right * px + trueUp * py).normalized();
                    Hit h;
                    intersect(cam.eye, dir, 0.0f, 1e9f, h);
                    const QVector3D v = evalPass(cam.eye, dir, h, pass, farDist, seed);
                    accR += v.x(); accG += v.y(); accB += v.z();
                }
            }
            accR *= inv; accG *= inv; accB *= inv;
            row[x] = qRgb(int(clamp01(accR) * 255.0f),
                          int(clamp01(accG) * 255.0f),
                          int(clamp01(accB) * 255.0f));
        }
    }
    return img;
}

QVector3D RayTraceRenderer::backgroundVec(const QVector3D& d) const
{
    const QColor c = shadeBackground(d);
    return QVector3D(c.redF(), c.greenF(), c.blueF());
}

QVector3D RayTraceRenderer::traceRay(const QVector3D& o, const QVector3D& d, int depth) const
{
    if (depth > 3)
        return backgroundVec(d);

    Hit h;
    intersect(o, d, 0.0f, 1e9f, h);
    if (!h.hit)
        return backgroundVec(d);

    const RTTriangle& t = m_tris[h.tri];
    const QVector3D n = (t.n1 - t.n0) * h.u + (t.n2 - t.n0) * h.v + t.n0;
    const QVector3D N = n.normalized();
    const QVector3D P = o + d * h.t;
    const QVector3D base(t.color.redF(), t.color.greenF(), t.color.blueF());

    // Direct light: scene lights, or the classic soft sun when none are set.
    // Seed an RNG from the hit position so the result is deterministic per pixel.
    const QVector3D Pl = P * 7919.0f + o * 104729.0f;
    unsigned seed = unsigned(qAbs(Pl.x() * 1e6f)) ^ (unsigned(qAbs(Pl.y() * 1e6f)) << 8) ^ (unsigned(qAbs(Pl.z() * 1e6f)) << 16);
    if (seed == 0) seed = 0x12345678u;
    QVector3D direct;
    if (!m_lights.isEmpty()) {
        direct = shadePoint(P, N, -d, t.metalness, t.roughness, t.color);
    } else {
        const QVector3D sunL = sampleSunDir(seed);
        const float ndl = std::max(0.0f, QVector3D::dotProduct(N, sunL));
        bool occluded = false;
        if (ndl > 0.0f) {
            Hit sh;
            intersect(P + N * 1e-3f, sunL, 0.0f, 1e9f, sh);
            occluded = sh.hit;
        }
        const float sunColor = 1.5f;
        direct = base * (ndl * (occluded ? 0.0f : sunColor));
    }

    // One diffuse indirect bounce (cosine-weighted hemisphere).
    const QVector3D bounceDir = randomCosHemisphere(seed, N);
    const QVector3D bounce = traceRay(P + N * 1e-3f, bounceDir, depth + 1);
    const QVector3D indirect(base.x() * bounce.x() * 0.6f, base.y() * bounce.y() * 0.6f, base.z() * bounce.z() * 0.6f);

    return direct + indirect;
}

QImage RayTraceRenderer::renderFinal(const RTCamera& cam, int width, int height, int samples) const
{
    QImage img(width, height, QImage::Format_RGB32);
    if (width <= 0 || height <= 0) return img;

    const int spp = qBound(1, samples, 64);

    const QVector3D forward = (cam.target - cam.eye).normalized();
    const QVector3D up = QVector3D(0.0f, 1.0f, 0.0f);
    QVector3D right = QVector3D::crossProduct(forward, up).normalized();
    if (right.lengthSquared() < 1e-6f) right = QVector3D(1.0f, 0.0f, 0.0f);
    const QVector3D trueUp = QVector3D::crossProduct(right, forward).normalized();

    const float fovRad = cam.fovDeg * float(M_PI) / 180.0f;
    const float tanHalf = std::tan(fovRad * 0.5f);
    const float aspect = float(width) / float(height);

    for (int y = 0; y < height; ++y) {
        QRgb* row = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (int x = 0; x < width; ++x) {
            float accR = 0.0f, accG = 0.0f, accB = 0.0f;
            const unsigned baseSeed = unsigned(y * 7741 + x * 131 + 7);
            for (int sy = 0; sy < spp; ++sy) {
                for (int sx = 0; sx < spp; ++sx) {
                    unsigned seed = baseSeed ^ unsigned(sy * 397 + sx * 1009 + 1);
                    const float jx = (x + rnd(seed)) / width;
                    const float jy = (y + rnd(seed)) / height;
                    const float px = (2.0f * jx - 1.0f) * tanHalf * aspect;
                    const float py = (1.0f - 2.0f * jy) * tanHalf;
                    const QVector3D dir = (forward + right * px + trueUp * py).normalized();
                    const QVector3D c = traceRay(cam.eye, dir, 0);
                    accR += c.x(); accG += c.y(); accB += c.z();
                }
            }
            const float inv = 1.0f / float(spp * spp);
            float r = accR * inv, g = accG * inv, b = accB * inv;
            // Filmic-ish tonemap + gamma.
            r = 1.0f - std::exp(-r);
            g = 1.0f - std::exp(-g);
            b = 1.0f - std::exp(-b);
            r = std::pow(clamp01(r), 1.0f / 2.2f);
            g = std::pow(clamp01(g), 1.0f / 2.2f);
            b = std::pow(clamp01(b), 1.0f / 2.2f);
            row[x] = qRgb(int(r * 255.0f), int(g * 255.0f), int(b * 255.0f));
        }
    }
    return img;
}

} // namespace ks
