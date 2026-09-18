#include "CurveSystem.h"

namespace ks {

// ============================================================================
// Evaluation
// ============================================================================

bool CurveData::computeArc(QVector3D& center, float& radius,
                           QVector3D& n0, QVector3D& n1,
                           float& start, float& sweep) const
{
    if (controlPoints.size() < 3)
        return false;
    const QVector3D& p0 = controlPoints[0];
    const QVector3D& p1 = controlPoints[1];
    const QVector3D& p2 = controlPoints[2];

    // Circumcenter of triangle p0,p1,p2.
    QVector3D a = p1 - p0;
    QVector3D b = p2 - p0;
    QVector3D nrm = QVector3D::crossProduct(a, b);
    if (nrm.lengthSquared() < 1e-10f)
        return false;
    nrm.normalize();
    float denom = 2.0f * QVector3D::dotProduct(QVector3D::crossProduct(a, b), nrm);
    if (qAbs(denom) < 1e-10f)
        return false;
    QVector3D c = (b * a.lengthSquared() - a * b.lengthSquared()) / denom;
    center = p0 + c;
    radius = (p0 - center).length();
    if (radius < 1e-8f)
        return false;

    n0 = (p0 - center).normalized();   // reference direction in the plane
    n1 = nrm;                          // plane normal
    QVector3D n2 = (p2 - center).normalized();
    QVector3D yAxis = QVector3D::crossProduct(n1, n0).normalized();
    start = qAtan2(QVector3D::dotProduct(n0, yAxis), QVector3D::dotProduct(n0, n0));
    start = 0.0f; // n0 is the zero-angle reference
    sweep = qAtan2(QVector3D::dotProduct(n2, yAxis), QVector3D::dotProduct(n2, n0));
    if (sweep < 0.0f) sweep += 2.0f * float(M_PI);
    if (sweep > 2.0f * float(M_PI)) sweep -= 2.0f * float(M_PI);
    return true;
}

void CurveData::evalArc(QVector3D center, float radius, QVector3D n0, QVector3D n1,
                        float start, float sweep, float t, QVector3D& out) const
{
    Q_UNUSED(start);
    // Point = center + radius * (cos(angle)*n0 + sin(angle)*yAxis)
    QVector3D yAxis = QVector3D::crossProduct(n1, n0).normalized();
    float angle = sweep * t;
    out = center + (n0 * cosf(angle) + yAxis * sinf(angle)) * radius;
}

QVector3D CurveData::evalPolyline(float t) const
{
    int n = controlPoints.size();
    if (n == 0) return QVector3D();
    if (n == 1) return controlPoints[0];
    if (closed) {
        float ft = t * n;
        int i = int(floorf(ft)) % n;
        float s = ft - floorf(ft);
        const QVector3D& a = controlPoints[i];
        const QVector3D& b = controlPoints[(i + 1) % n];
        return a + (b - a) * s;
    }
    float ft = t * (n - 1);
    int i = int(floorf(ft));
    if (i >= n - 1) return controlPoints[n - 1];
    float s = ft - i;
    return controlPoints[i] + (controlPoints[i + 1] - controlPoints[i]) * s;
}

QVector3D CurveData::evalBezier(float t) const
{
    QVector<QVector3D> pts = controlPoints;
    if (pts.isEmpty()) return QVector3D();
    while (pts.size() > 1) {
        QVector<QVector3D> next;
        next.reserve(pts.size() - 1);
        for (int i = 0; i < pts.size() - 1; ++i)
            next.append(pts[i] + (pts[i + 1] - pts[i]) * t);
        pts = next;
    }
    return pts[0];
}

// Cox-de Boor for a uniform clamped knot vector.
QVector3D CurveData::evalBspline(float t) const
{
    int n = controlPoints.size();
    int p = qBound(1, degree, n - 1);
    if (n < 2) return controlPoints.isEmpty() ? QVector3D() : controlPoints[0];

    // Build uniform clamped knot vector of size n + p + 1.
    QVector<float> knots(n + p + 1, 0.0f);
    for (int i = 0; i <= p; ++i) knots[i] = 0.0f;
    int interior = n - p;
    for (int i = 0; i < interior; ++i)
        knots[p + 1 + i] = float(i + 1) / float(interior + 1);
    for (int i = p + 1 + interior; i <= n + p; ++i) knots[i] = 1.0f;

    // Clamp parameter.
    if (t <= knots[p]) return controlPoints[0];
    if (t >= knots[n]) return controlPoints[n - 1];

    // Find knot span.
    int span = p;
    while (span < n - 1 && knots[span + 1] <= t) ++span;

    // Cox-de Boor recursion.
    QVector<QVector3D> d(p + 1);
    for (int j = 0; j <= p; ++j)
        d[j] = controlPoints[span - p + j];
    for (int r = 1; r <= p; ++r) {
        for (int j = p; j >= r; --j) {
            int idx = span - p + j;
            float denom = knots[idx + p - r + 1] - knots[idx];
            float alpha = (denom < 1e-9f) ? 0.0f : (t - knots[idx]) / denom;
            d[j] = d[j - 1] + (d[j] - d[j - 1]) * alpha;
        }
    }
    return d[p];
}

QVector3D CurveData::evaluate(float t) const
{
    switch (type) {
    case CurveType::Polyline: return evalPolyline(t);
    case CurveType::Bezier:   return evalBezier(t);
    case CurveType::Bspline:  return evalBspline(t);
    case CurveType::Arc:
    case CurveType::Circle: {
        if (type == CurveType::Circle) {
            // First CV = center, second CV = point on circle.
            if (controlPoints.size() < 2) return QVector3D();
            QVector3D center = controlPoints[0];
            QVector3D n = QVector3D::crossProduct(controlPoints[1] - center, QVector3D(0,0,1));
            if (n.lengthSquared() < 1e-9f) n = QVector3D(0,0,1);
            n.normalize();
            float radius = (controlPoints[1] - center).length();
            float angle = t * 2.0f * float(M_PI);
            QVector3D axis = n;
            QVector3D x = (controlPoints[1] - center).normalized();
            QVector3D y = QVector3D::crossProduct(axis, x).normalized();
            return center + x * (cosf(angle) * radius) + y * (sinf(angle) * radius);
        }
        QVector3D center, n0, n1;
        float radius, start, sweep;
        if (!computeArc(center, radius, n0, n1, start, sweep))
            return evalPolyline(t);
        QVector3D out;
        evalArc(center, radius, n0, n1, start, sweep, t, out);
        return out;
    }
    }
    return QVector3D();
}

QVector<QVector3D> CurveData::tessellate(int segments) const
{
    segments = qBound(2, segments, 4096);
    QVector<QVector3D> result;
    result.reserve(segments + 1);
    for (int i = 0; i <= segments; ++i)
        result.append(evaluate(float(i) / float(segments)));
    if (closed && result.size() > 1 && result.last() == result.first())
        result.removeLast();
    return result;
}

// ============================================================================
// Primitives
// ============================================================================

CurveData CurvePrimitives::line(const QVector3D& a, const QVector3D& b)
{
    CurveData c;
    c.type = CurveType::Polyline;
    c.controlPoints = { a, b };
    return c;
}

CurveData CurvePrimitives::polyline(const QVector<QVector3D>& pts)
{
    CurveData c;
    c.type = CurveType::Polyline;
    c.controlPoints = pts;
    return c;
}

CurveData CurvePrimitives::bezier(const QVector<QVector3D>& cv)
{
    CurveData c;
    c.type = CurveType::Bezier;
    c.controlPoints = cv;
    return c;
}

CurveData CurvePrimitives::bspline(const QVector<QVector3D>& cv, int degree)
{
    CurveData c;
    c.type = CurveType::Bspline;
    c.controlPoints = cv;
    c.degree = qBound(1, degree, qMax(1, cv.size() - 1));
    return c;
}

CurveData CurvePrimitives::arc(const QVector3D& p0, const QVector3D& p1, const QVector3D& p2)
{
    CurveData c;
    c.type = CurveType::Arc;
    c.controlPoints = { p0, p1, p2 };
    return c;
}

CurveData CurvePrimitives::circle(const QVector3D& center, const QVector3D& normal, float radius)
{
    CurveData c;
    c.type = CurveType::Circle;
    c.controlPoints = { center, center + normal.normalized() * radius };
    c.closed = true;
    return c;
}

// ============================================================================
// Surfaces
// ============================================================================

namespace {
// Converts a profile curve into a polyline MeshData (vertices only, no faces),
// which is what MeshOperations::sweep expects as its profile argument.
MeshData curveToProfileMesh(const CurveData& curve, int segments)
{
    MeshData profile;
    QVector<QVector3D> pts = curve.tessellate(segments);
    profile.vertices.reserve(pts.size());
    for (const auto& p : pts) {
        Vertex v;
        v.position = p;
        profile.vertices.append(v);
    }
    return profile;
}
} // anonymous namespace

MeshData CurveSurfaces::loft(const CurveData& a, const CurveData& b, int segments, bool close)
{
    QVector<CurveData> profiles = { a, b };
    return loftMulti(profiles, segments, close);
}

MeshData CurveSurfaces::loftMulti(const QVector<CurveData>& profiles, int segments, bool close)
{
    MeshData result;
    if (profiles.size() < 2) return result;

    QVector<QVector<QVector3D>> rings;
    rings.reserve(profiles.size());
    int ringCount = -1;
    for (const auto& prof : profiles) {
        QVector<QVector3D> ring = prof.tessellate(segments);
        if (ringCount < 0) ringCount = ring.size();
        else if (ring.size() != ringCount) {
            // Resample to match the first ring's count.
            QVector<QVector3D> resampled;
            resampled.reserve(ringCount);
            for (int i = 0; i < ringCount; ++i) {
                float t = float(i) / float(ringCount - 1);
                resampled.append(prof.evaluate(t));
            }
            ring = resampled;
        }
        rings.append(ring);
    }

    // Build vertices: one row per profile.
    for (const auto& ring : rings)
        for (const auto& p : ring) {
            Vertex v;
            v.position = p;
            result.vertices.append(v);
        }

    const int rows = rings.size();
    const int cols = ringCount;
    auto idx = [rows, cols](int r, int c) {
        r = (r + rows) % rows;
        c = (c + cols) % cols;
        return r * cols + c;
    };

    for (int r = 0; r < rows - 1; ++r) {
        for (int c = 0; c < cols; ++c) {
            int c2 = (c + 1) % cols;
            Face f;
            f.indices = { idx(r, c), idx(r + 1, c), idx(r + 1, c2), idx(r, c2) };
            result.faces.append(f);
        }
    }
    if (close) {
        // Cap between last and first row.
        for (int c = 0; c < cols; ++c) {
            int c2 = (c + 1) % cols;
            Face f;
            f.indices = { idx(rows - 1, c), idx(0, c), idx(0, c2), idx(rows - 1, c2) };
            result.faces.append(f);
        }
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

MeshData CurveSurfaces::sweep(const CurveData& profile, const CurveData& path, int segments)
{
    MeshData result;
    if (!profile.isValid() || !path.isValid()) return result;

    QVector<QVector3D> pathPts = path.tessellate(segments);
    if (pathPts.size() < 2) return result;

    // Framing: parallel transport style using the path tangent.
    QVector<QMatrix4x4> frames;
    frames.reserve(pathPts.size());
    QVector3D prevTangent = QVector3D(0, 0, 1);
    for (int i = 0; i < pathPts.size(); ++i) {
        QVector3D tangent;
        if (i == 0)
            tangent = (pathPts[1] - pathPts[0]).normalized();
        else if (i == pathPts.size() - 1)
            tangent = (pathPts[i] - pathPts[i - 1]).normalized();
        else
            tangent = (pathPts[i + 1] - pathPts[i - 1]).normalized();
        if (tangent.lengthSquared() < 1e-9f) tangent = prevTangent;
        tangent.normalize();
        prevTangent = tangent;

        QVector3D up = qFuzzyIsNull(tangent.z()) ? QVector3D(0,0,1) : QVector3D(0,1,0);
        QVector3D side = QVector3D::crossProduct(tangent, up).normalized();
        QVector3D correctedUp = QVector3D::crossProduct(side, tangent).normalized();

        QMatrix4x4 frame;
        frame.setToIdentity();
        frame.setColumn(0, QVector4D(side, 0.0f));
        frame.setColumn(1, QVector4D(correctedUp, 0.0f));
        frame.setColumn(2, QVector4D(tangent, 0.0f));
        frame.setColumn(3, QVector4D(pathPts[i], 1.0f));
        frames.append(frame);
    }

    MeshData profileMesh = curveToProfileMesh(profile, 16);
    return MeshOperations::sweep(profileMesh, frames, false);
}

MeshData CurveSurfaces::revolve(const CurveData& profile, const QVector3D& axis, float angleDeg, int steps)
{
    MeshData result;
    if (!profile.isValid()) return result;

    QVector3D n = axis.normalized();
    QVector3D axisStart(0, 0, 0);
    QVector3D axisEnd = n;

    QVector<QMatrix4x4> transforms;
    transforms.reserve(steps + 1);
    for (int i = 0; i <= steps; ++i) {
        float ang = qDegreesToRadians(angleDeg) * float(i) / float(steps);
        QMatrix4x4 m;
        m.setToIdentity();
        m.rotate(ang, n.x(), n.y(), n.z());
        // Translate so the profile is placed relative to the axis origin.
        transforms.append(m);
    }

    MeshData profileMesh = curveToProfileMesh(profile, 16);
    return MeshOperations::sweep(profileMesh, transforms, true);
}

MeshData CurveSurfaces::rail(const CurveData& rail1, const CurveData& rail2,
                             const CurveData& profile, int segments)
{
    MeshData result;
    if (!rail1.isValid() || !rail2.isValid()) return result;

    QVector<QVector3D> r1 = rail1.tessellate(segments);
    QVector<QVector3D> r2 = rail2.tessellate(segments);

    QVector<QMatrix4x4> frames;
    frames.reserve(r1.size());
    for (int i = 0; i < r1.size(); ++i) {
        QVector3D a = r1[i];
        QVector3D b = r2[i];
        QVector3D side = (b - a).normalized();
        QVector3D tangent = (i < r1.size() - 1 ? (r1[i + 1] - a) : (a - r1[i - 1])).normalized();
        QVector3D up = QVector3D::crossProduct(side, tangent).normalized();
        if (up.lengthSquared() < 1e-9f) up = QVector3D(0, 1, 0);
        up.normalize();
        side = QVector3D::crossProduct(tangent, up).normalized();

        QMatrix4x4 frame;
        frame.setToIdentity();
        frame.setColumn(0, QVector4D(side, 0.0f));
        frame.setColumn(1, QVector4D(up, 0.0f));
        frame.setColumn(2, QVector4D(tangent, 0.0f));
        frame.setColumn(3, QVector4D(a, 1.0f));
        // Scale to fit the rail width.
        frame.scale(1.0f, (b - a).length(), 1.0f);
        frames.append(frame);
    }

    MeshData profileMesh = curveToProfileMesh(profile, 16);
    return MeshOperations::sweep(profileMesh, frames, false);
}

MeshData CurveSurfaces::curveRibbon(const CurveData& curve, float width, int segments)
{
    MeshData result;
    if (!curve.isValid()) return result;

    QVector<QVector3D> pts = curve.tessellate(segments);
    if (pts.size() < 2) return result;

    for (int i = 0; i < pts.size(); ++i) {
        QVector3D tangent;
        if (i == 0)
            tangent = (pts[1] - pts[0]).normalized();
        else if (i == pts.size() - 1)
            tangent = (pts[i] - pts[i - 1]).normalized();
        else
            tangent = (pts[i + 1] - pts[i - 1]).normalized();
        if (tangent.lengthSquared() < 1e-9f) tangent = QVector3D(0, 0, 1);
        tangent.normalize();

        QVector3D up = qFuzzyIsNull(tangent.z()) ? QVector3D(0, 0, 1) : QVector3D(0, 1, 0);
        QVector3D side = QVector3D::crossProduct(tangent, up).normalized() * (width * 0.5f);

        Vertex a, b;
        a.position = pts[i] - side;
        b.position = pts[i] + side;
        result.vertices.append(a);
        result.vertices.append(b);
    }

    for (int i = 0; i < pts.size() - 1; ++i) {
        Face f;
        f.indices = { i * 2, (i + 1) * 2, (i + 1) * 2 + 1, i * 2 + 1 };
        result.faces.append(f);
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

// ============================================================================
// Serialization
// ============================================================================

QVariantMap CurveIO::toVariant(const CurveData& c)
{
    QVariantMap m;
    m["type"] = curveTypeToString(c.type);
    m["degree"] = c.degree;
    m["closed"] = c.closed;
    QVariantList pts;
    for (const auto& p : c.controlPoints) {
        QVariantList v;
        v << p.x() << p.y() << p.z();
        pts.append(v);
    }
    m["points"] = pts;
    return m;
}

CurveData CurveIO::fromVariant(const QVariantMap& m)
{
    CurveData c;
    c.type = curveTypeFromString(m.value("type").toString());
    c.degree = m.value("degree", 3).toInt();
    c.closed = m.value("closed", false).toBool();
    QVariantList pts = m.value("points").toList();
    for (const auto& p : pts) {
        QVariantList v = p.toList();
        if (v.size() >= 3)
            c.controlPoints.append(QVector3D(v[0].toFloat(), v[1].toFloat(), v[2].toFloat()));
    }
    return c;
}

} // namespace ks
