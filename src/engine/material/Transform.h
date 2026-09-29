#pragma once

// Qt-free 2D affine transform (the QTransform replacement for the material
// module). Column-vector convention: device = M * point, and chained calls
// append, so
//     t.translate(x, y); t.rotate(a); t.scale(s, s);
// maps a point as scale -> rotate -> translate, which is the same behaviour
// as the matching QPainter/QTransform sequences used by the paint code.
// The rotation matrix matches QTransform::rotate (m11=cos, m12=sin,
// m21=-sin, m22=cos), so angles keep their visual direction.

#include "MaterialTypes.h"

namespace ks {

class Transform {
public:
    Transform() = default;

    void translate(double dx, double dy)
    {
        m[2] += dx * m[0] + dy * m[1];
        m[5] += dx * m[3] + dy * m[4];
    }

    void rotate(double degrees)
    {
        const double rad = degrees * (3.14159265358979323846 / 180.0);
        const double c = std::cos(rad);
        const double s = std::sin(rad);
        const double m0 = m[0], m1 = m[1], m3 = m[3], m4 = m[4];
        m[0] = m0 * c + m1 * s;
        m[1] = -m0 * s + m1 * c;
        m[3] = m3 * c + m4 * s;
        m[4] = -m3 * s + m4 * c;
    }

    void scale(double sx, double sy)
    {
        m[0] *= sx;
        m[1] *= sy;
        m[3] *= sx;
        m[4] *= sy;
    }

    bool isIdentity() const
    {
        return m[0] == 1.0 && m[1] == 0.0 && m[2] == 0.0 && m[3] == 0.0 &&
               m[4] == 1.0 && m[5] == 0.0;
    }

    PointF map(const PointF& p) const
    {
        return PointF(m[0] * p.x + m[1] * p.y + m[2], m[3] * p.x + m[4] * p.y + m[5]);
    }

    Point map(const Point& p) const { return map(PointF(p)).toPoint(); }

    Transform inverted() const
    {
        const double det = m[0] * m[4] - m[1] * m[3];
        Transform out;
        if (det == 0.0) return out;
        const double inv = 1.0 / det;
        out.m[0] = m[4] * inv;
        out.m[1] = -m[1] * inv;
        out.m[3] = -m[3] * inv;
        out.m[4] = m[0] * inv;
        out.m[2] = -(m[2] * out.m[0] + m[5] * out.m[1]);
        out.m[5] = -(m[2] * out.m[3] + m[5] * out.m[4]);
        return out;
    }

    double m[6] = {1, 0, 0, 0, 1, 0};
};

} // namespace ks
