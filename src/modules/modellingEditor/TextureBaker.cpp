#include "TextureBaker.h"
#include "Geometry3D.h"
#include <QtMath>

namespace ks {
namespace io {

static QImage bakeNormalMap(geometry::Mesh3D* mesh, int width, int height) {
    QImage result(width, height, QImage::Format_ARGB32);
    result.fill(Qt::gray);
    auto verts = mesh->vertices();
    auto norms = mesh->normals();
    auto indices = mesh->indices();
    auto uvs = mesh->uvs();
    if (verts.isEmpty() || indices.isEmpty()) return result;

    auto barycentric = [](const QVector2D& a, const QVector2D& b, const QVector2D& c, const QVector2D& p) -> QVector3D {
        QVector2D v0 = b - a, v1 = c - a, v2 = p - a;
        float d00 = QVector2D::dotProduct(v0, v0);
        float d01 = QVector2D::dotProduct(v0, v1);
        float d11 = QVector2D::dotProduct(v1, v1);
        float d20 = QVector2D::dotProduct(v2, v0);
        float d21 = QVector2D::dotProduct(v2, v1);
        float denom = d00 * d11 - d01 * d01;
        if (qFuzzyIsNull(denom)) return QVector3D(1.0f/3, 1.0f/3, 1.0f/3);
        float u = (d11 * d20 - d01 * d21) / denom;
        float v = (d00 * d21 - d01 * d20) / denom;
        float w = 1.0f - u - v;
        return QVector3D(u, v, w);
    };

    for (int i = 0; i + 2 < indices.size(); i += 3) {
        int i0 = indices[i], i1 = indices[i+1], i2 = indices[i+2];
        if (i0 >= verts.size() || i1 >= verts.size() || i2 >= verts.size()) continue;
        QVector3D n;
        if (i0 < norms.size()) n += norms[i0] * 1.0f;
        if (i1 < norms.size()) n += norms[i1] * 1.0f;
        if (i2 < norms.size()) n += norms[i2] * 1.0f;
        if (n.lengthSquared() < 1e-8f) n = QVector3D(0.0f, 0.0f, 1.0f);
        else n.normalize();
        QVector3D a = std::abs(n.y()) >= std::abs(n.x()) && std::abs(n.y()) >= std::abs(n.z())
            ? QVector3D(0.0f, 0.0f, 1.0f)
            : (std::abs(n.x()) >= std::abs(n.z()) ? QVector3D(0.0f, 1.0f, 0.0f)
                                                  : QVector3D(1.0f, 0.0f, 0.0f));
        const QVector3D uAxis = QVector3D::crossProduct(a, n).normalized();
        const QVector3D vAxis = QVector3D::crossProduct(n, uAxis).normalized();
        auto projectUV = [&](int idx, const QVector3D& p) -> QVector2D {
            if (idx < uvs.size()) return uvs[idx];
            return QVector2D(QVector3D::dotProduct(p, uAxis) + 0.5f,
                             QVector3D::dotProduct(p, vAxis) + 0.5f);
        };
        QVector2D uv0 = projectUV(i0, verts[i0]);
        QVector2D uv1 = projectUV(i1, verts[i1]);
        QVector2D uv2 = projectUV(i2, verts[i2]);

        int minX = qMax(0, (int)(std::min({uv0.x(), uv1.x(), uv2.x()}) * width));
        int maxX = qMin(width - 1, (int)(std::max({uv0.x(), uv1.x(), uv2.x()}) * width));
        int minY = qMax(0, (int)(std::min({uv0.y(), uv1.y(), uv2.y()}) * height));
        int maxY = qMin(height - 1, (int)(std::max({uv0.y(), uv1.y(), uv2.y()}) * height));

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                QVector2D p((float)x / width, (float)y / height);
                QVector3D bc = barycentric(uv0, uv1, uv2, p);
                if (bc.x() < 0 || bc.y() < 0 || bc.z() < 0) continue;

                QVector3D n;
                if (i0 < norms.size()) n += norms[i0] * bc.x();
                if (i1 < norms.size()) n += norms[i1] * bc.y();
                if (i2 < norms.size()) n += norms[i2] * bc.z();
                if (n.lengthSquared() < 1e-8f) n = QVector3D(0.0f, 0.0f, 1.0f);
                else n.normalize();

                QColor c;
                c.setRgbF(n.x() * 0.5f + 0.5f, n.y() * 0.5f + 0.5f, n.z() * 0.5f + 0.5f);
                result.setPixelColor(x, y, c);
            }
        }
    }
    return result;
}

static QImage bakeAOMap(geometry::Mesh3D* mesh, int width, int height) {
    QImage result(width, height, QImage::Format_ARGB32);
    result.fill(Qt::white);
    const QVector<QVector3D> verts = mesh->vertices();
    const QVector<QVector3D> norms = mesh->normals();
    const QVector<quint32> indices = mesh->indices();
    const QVector<QVector2D> uvs = mesh->uvs();
    if (verts.size() < 3 || indices.size() < 3) return result;

    struct BakeTri {
        QVector3D v[3];
        QVector3D n[3];
        QVector2D uv[3];
        QVector3D flatN;
    };
    QVector<BakeTri> tris;
    for (int i = 0; i + 2 < indices.size(); i += 3) {
        const int i0 = indices[i], i1 = indices[i + 1], i2 = indices[i + 2];
        if (i0 >= verts.size() || i1 >= verts.size() || i2 >= verts.size()) continue;
        BakeTri t;
        t.v[0] = verts[i0]; t.v[1] = verts[i1]; t.v[2] = verts[i2];
        for (int k = 0; k < 3; ++k)
            t.n[k] = (i0 + k) < norms.size() ? norms[i0 + k] : QVector3D(0.0f, 0.0f, 1.0f);
        t.flatN = QVector3D::normal(t.v[0], t.v[1], t.v[2]);
        if (t.flatN.lengthSquared() < 1e-8f) t.flatN = QVector3D(0.0f, 0.0f, 1.0f);

        // UV vertices; procedurally project on the dominant axis when absent so
        // the bake still produces a usable map.
        auto uvOf = [&](int idx, const QVector3D& p) -> QVector2D {
            if (idx < uvs.size()) return uvs[idx];
            const QVector3D a = std::abs(t.flatN.y()) >= std::abs(t.flatN.x())
                              && std::abs(t.flatN.y()) >= std::abs(t.flatN.z())
                              ? QVector3D(0.0f, 0.0f, 1.0f)
                              : (std::abs(t.flatN.x()) >= std::abs(t.flatN.z())
                                    ? QVector3D(0.0f, 1.0f, 0.0f)
                                    : QVector3D(1.0f, 0.0f, 0.0f));
            const QVector3D u = QVector3D::crossProduct(a, t.flatN).normalized();
            const QVector3D v = QVector3D::crossProduct(t.flatN, u).normalized();
            return QVector2D(QVector3D::dotProduct(p, u) + 0.5f, QVector3D::dotProduct(p, v) + 0.5f);
        };
        t.uv[0] = uvOf(i0, t.v[0]);
        t.uv[1] = uvOf(i1, t.v[1]);
        t.uv[2] = uvOf(i2, t.v[2]);
        tris.append(t);
    }
    if (tris.isEmpty()) return result;

    // Scene bounds -> AO sampling radius.
    QVector3D bmin(1e9f, 1e9f, 1e9f), bmax(-1e9f, -1e9f, -1e9f);
    for (const QVector3D& v : verts) {
        bmin = QVector3D(std::min(bmin.x(), v.x()), std::min(bmin.y(), v.y()), std::min(bmin.z(), v.z()));
        bmax = QVector3D(std::max(bmax.x(), v.x()), std::max(bmax.y(), v.y()), std::max(bmax.z(), v.z()));
    }
    const float radius = (bmax - bmin).length() * 0.15f;
    const int nsamp = 16;

    auto xorshift = [](unsigned& s) { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; };
    auto rndf = [&](unsigned& s) { return (xorshift(s) & 0x00FFFFFF) / 16777216.0f; };
    auto cosHemi = [&](unsigned& s, const QVector3D& n) -> QVector3D {
        QVector3D u = std::abs(n.y()) < 0.999f
            ? QVector3D::crossProduct(n, QVector3D(0.0f, 1.0f, 0.0f)).normalized()
            : QVector3D(1.0f, 0.0f, 0.0f);
        const QVector3D vv = QVector3D::crossProduct(u, n).normalized();
        const float phi = 2.0f * float(M_PI) * rndf(s);
        const float cost = std::sqrt(rndf(s));
        const float sint = std::sqrt(1.0f - cost * cost);
        return (u * (std::cos(phi) * sint) + vv * (std::sin(phi) * sint) + n * cost).normalized();
    };

    auto rayTri = [](const QVector3D& o, const QVector3D& d, const BakeTri& t, float& tHit) -> bool {
        const QVector3D e1 = t.v[1] - t.v[0], e2 = t.v[2] - t.v[0];
        const QVector3D p = QVector3D::crossProduct(d, e2);
        const float det = QVector3D::dotProduct(e1, p);
        if (det > -1e-8f && det < 1e-8f) return false;
        const float invDet = 1.0f / det;
        const QVector3D s = o - t.v[0];
        const float uwo = QVector3D::dotProduct(s, p) * invDet;
        if (uwo < -1e-6f || uwo > 1.0f + 1e-6f) return false;
        const QVector3D q = QVector3D::crossProduct(s, e1);
        const float vwo = QVector3D::dotProduct(d, q) * invDet;
        if (vwo < -1e-6f || uwo + vwo > 1.0f + 1e-6f) return false;
        tHit = QVector3D::dotProduct(e2, q) * invDet;
        return tHit > 1e-4f;
    };

    // Rasterize every triangle in UV space; for each covered texel cast a cosine
    // hemisphere of occlusion rays from the interpolated world position. On
    // overlapping UV islands the darkest result (nearest geometry) wins.
    for (const BakeTri& t : tris) {
        const float u0 = t.uv[0].x(), u1 = t.uv[1].x(), u2 = t.uv[2].x();
        const float v0 = t.uv[0].y(), v1 = t.uv[1].y(), v2 = t.uv[2].y();
        const int minX = qMax(0, int(std::min(u0, std::min(u1, u2)) * width) - 1);
        const int maxX = qMin(width - 1, int(std::max(u0, std::max(u1, u2)) * width) + 1);
        const int minY = qMax(0, int(std::min(v0, std::min(v1, v2)) * height) - 1);
        const int maxY = qMin(height - 1, int(std::max(v0, std::max(v1, v2)) * height) + 1);

        const float du0 = u1 - u0, dv0 = v1 - v0, du1 = u2 - u0, dv1 = v2 - v0;
        const float denom = du0 * dv1 - du1 * dv0;

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                const float pu = (x + 0.5f) / width - u0;
                const float pv = (y + 0.5f) / height - v0;
                float w1, w2;
                if (std::abs(denom) < 1e-12f) { w1 = 1.0f / 3.0f; w2 = 1.0f / 3.0f; }
                else { w1 = (pu * dv1 - pv * du1) / denom; w2 = (pv * du0 - pu * dv0) / denom; }
                const float w0 = 1.0f - w1 - w2;
                if (w0 < -1e-4f || w1 < -1e-4f || w2 < -1e-4f) continue;

                QVector3D P = t.v[0] * w0 + t.v[1] * w1 + t.v[2] * w2;
                QVector3D N = (t.n[0] * w0 + t.n[1] * w1 + t.n[2] * w2);
                if (N.lengthSquared() < 1e-8f) N = t.flatN;
                N.normalize();

                unsigned seed = unsigned(y * 73856093u ^ x * 19349663u ^ 0x12345678u);
                if (!seed) seed = 0x9e3779b9u;
                int hits = 0;
                for (int r = 0; r < nsamp; ++r) {
                    const QVector3D dir = cosHemi(seed, N);
                    bool occluded = false;
                    for (const BakeTri& o : tris) {
                        float tHit;
                        if (rayTri(P + N * 1e-4f, dir, o, tHit) && tHit <= radius) {
                            occluded = true;
                            break;
                        }
                    }
                    if (occluded) ++hits;
                }
                const float ao = 1.0f - float(hits) / nsamp;
                const int gray = qBound(0, int(ao * 255.0f), 255);
                if (gray < qGray(result.pixel(x, y)))
                    result.setPixelColor(x, y, QColor(gray, gray, gray));
            }
        }
    }
    return result;
}

void TextureBaker::addBakeTarget(BakeType type, const QString& outputPath) {
    m_targets[type] = outputPath;
}

QString TextureBaker::textureTypeName(BakeType type) {
    switch (type) {
        case Diffuse: return "diffuse";
        case Normal: return "normal";
        case Roughness: return "roughness";
        case Metallic: return "metallic";
        case AO: return "ao";
        case Height: return "height";
        case Emission: return "emission";
    }
    return "unknown";
}

void TextureBaker::bake(BakeType type) {
    if (!m_source) return;

    QImage tex;
    switch (type) {
        case Normal:
            tex = bakeNormalMap(m_source, m_width, m_height);
            break;
        case AO:
            tex = bakeAOMap(m_source, m_width, m_height);
            break;
        case Roughness: {
            tex = QImage(m_width, m_height, QImage::Format_ARGB32);
            tex.fill(QColor::fromRgbF(0.5f, 0.5f, 0.5f));
            break;
        }
        case Metallic: {
            tex = QImage(m_width, m_height, QImage::Format_ARGB32);
            tex.fill(Qt::black);
            break;
        }
        case Diffuse: {
            tex = QImage(m_width, m_height, QImage::Format_ARGB32);
            tex.fill(m_baseColor);
            break;
        }
        default:
            tex = QImage(m_width, m_height, QImage::Format_ARGB32);
            tex.fill(Qt::white);
            break;
    }

    m_bakedTextures[type] = tex;

    if (m_targets.contains(type)) {
        tex.save(m_targets[type]);
    }
}

QImage TextureBaker::packRgba(int packChannels) const {
    QImage result(m_width, m_height, QImage::Format_ARGB32_Premultiplied);
    result.fill(Qt::transparent);

    auto sampleUV = [&](int x, int y) -> QVector3D {
        QRgb pix = result.pixel(x, y);
        if (qAlpha(pix) > 0) {
            // Return existing non-transparent channel values normalized
            return QVector3D(qRed(pix) / 255.0f, qGreen(pix) / 255.0f, qBlue(pix) / 255.0f);
        }
        // Sample from individual baked textures
        QVector3D sample(0, 0, 0);
        int count = 0;
        if (packChannels & int(PackChannel::RoughnessCh) && m_bakedTextures.contains(Roughness)) {
            sample += QVector3D(m_bakedTextures.value(Roughness).pixel(x, y) & 0xFF, 0, 0);
            ++count;
        }
        if (packChannels & int(PackChannel::MetallicCh) && m_bakedTextures.contains(Metallic)) {
            sample += QVector3D(0, m_bakedTextures.value(Metallic).pixel(x, y) & 0xFF, 0);
            ++count;
        }
        if (packChannels & int(PackChannel::AOCh) && m_bakedTextures.contains(AO)) {
            sample += QVector3D(0, 0, m_bakedTextures.value(AO).pixel(x, y) & 0xFF);
            ++count;
        }
        if (packChannels & int(PackChannel::HeightCh) && m_bakedTextures.contains(Height)) {
            sample += QVector3D(m_bakedTextures.value(Height).pixel(x, y) & 0xFF, 0, 0);
            ++count;
        }
        if (packChannels & int(PackChannel::EmissionCh) && m_bakedTextures.contains(Emission)) {
            sample += QVector3D(0, 0, m_bakedTextures.value(Emission).pixel(x, y) & 0xFF);
            ++count;
        }
        if (count > 0) sample /= float(count);
        return sample;
    };

    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            QVector3D col = sampleUV(x, y);
            result.setPixelColor(x, y, QColor::fromRgbF(col.x(), col.y(), col.z(), 1.0f));
        }
    }
    return result;
}

} // namespace io
} // namespace ks