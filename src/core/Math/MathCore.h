#pragma once

// Minimal math core used by material editor, etc.

#include <cmath>
#include <QMatrix4x4>

struct Vec2 {
    float x;
    float y;

    Vec2() : x(0.0f), y(0.0f) {}
    Vec2(float x_, float y_) : x(x_), y(y_) {}

    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s)       { x *= s;   y *= s;   return *this; }
    Vec2& operator/=(float s)       { x /= s;   y /= s;   return *this; }

    bool operator==(const Vec2& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Vec2& o) const { return !(*this == o); }
};

inline Vec2 operator+(const Vec2& a, const Vec2& b) { return Vec2(a.x + b.x, a.y + b.y); }
inline Vec2 operator-(const Vec2& a, const Vec2& b) { return Vec2(a.x - b.x, a.y - b.y); }
inline Vec2 operator*(const Vec2& v, float s)       { return Vec2(v.x * s, v.y * s); }
inline Vec2 operator*(float s, const Vec2& v)       { return Vec2(v.x * s, v.y * s); }
inline Vec2 operator/(const Vec2& v, float s)       { return Vec2(v.x / s, v.y / s); }
inline Vec2 operator-(const Vec2& v)                { return Vec2(-v.x, -v.y); }

inline float dot(const Vec2& a, const Vec2& b)  { return a.x*b.x + a.y*b.y; }
inline float length(const Vec2& v)              { return std::sqrt(v.x*v.x + v.y*v.y); }
inline float lengthSq(const Vec2& v)            { return v.x*v.x + v.y*v.y; }

inline Vec2 normalize(const Vec2& v) {
    float len = length(v);
    return (len > 1e-6f) ? v / len : Vec2(0.0f, 0.0f);
}

inline Vec2 lerp(const Vec2& a, const Vec2& b, float t) {
    return a * (1.0f - t) + b * t;
}

// Aliases for code expecting Vector2/Vector3 naming

struct Vec3 {
    float x;
    float y;
    float z;

    Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*=(float s)       { x *= s;   y *= s;   z *= s;   return *this; }
    Vec3& operator/=(float s)       { x /= s;   y /= s;   z /= s;   return *this; }

    bool operator==(const Vec3& o) const { return x == o.x && y == o.y && z == o.z; }
    bool operator!=(const Vec3& o) const { return !(*this == o); }

    Vec3 normalized() const { float len = std::sqrt(x*x + y*y + z*z); return (len > 1e-6f) ? Vec3(x/len, y/len, z/len) : Vec3(0,0,0); }
    static Vec3 cross(const Vec3& a, const Vec3& b) {
        return Vec3(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x);
    }
};

inline Vec3 operator+(const Vec3& a, const Vec3& b) { return Vec3(a.x + b.x, a.y + b.y, a.z + b.z); }
inline Vec3 operator-(const Vec3& a, const Vec3& b) { return Vec3(a.x - b.x, a.y - b.y, a.z - b.z); }
inline Vec3 operator*(const Vec3& a, float s)       { return Vec3(a.x * s, a.y * s, a.z * s); }
inline Vec3 operator*(float s, const Vec3& a)       { return Vec3(a.x * s, a.y * s, a.z * s); }
inline Vec3 operator/(const Vec3& a, float s)       { return Vec3(a.x / s, a.y / s, a.z / s); }
inline Vec3 operator-(const Vec3& v)                { return Vec3(-v.x, -v.y, -v.z); }

inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return Vec3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

inline float dot(const Vec3& a, const Vec3& b)  { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline float length(const Vec3& v)              { return std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z); }
inline float lengthSq(const Vec3& v)            { return v.x*v.x + v.y*v.y + v.z*v.z; }

inline Vec3 normalize(const Vec3& v) {
    float len = length(v);
    return (len > 1e-6f) ? v / len : Vec3(0.0f, 0.0f, 0.0f);
}

inline Vec3 lerp(const Vec3& a, const Vec3& b, float t) {
    return a * (1.0f - t) + b * t;
}

// Aliases for code expecting Vector2/Vector3 naming
using Vector2 = Vec2;
using Vector3 = Vec3;

namespace ks {

struct Matrix4 {
    struct Translation { float x, y, z; } translation_ = {0,0,0};
    Vec3 rotation_ = {0,0,0};
    Vec3 scale_ = {1,1,1};
    float m[4][4];

    // Provide data() for compatibility with code expecting contiguous array
    const float* data() const { return &m[0][0]; }
    float* data() { return &m[0][0]; }

    // Allow implicit conversion to float* for legacy code
    operator const float*() const { return &m[0][0]; }
    operator float*() { return &m[0][0]; }

    Matrix4() { rebuild(); }

    void rebuild() {
        // Rebuild matrix from T * R * S
        float cx = cosf(rotation_.x), sx = sinf(rotation_.x);
        float cy = cosf(rotation_.y), sy = sinf(rotation_.y);
        float cz = cosf(rotation_.z), sz = sinf(rotation_.z);

        // Rotation R = Rz * Ry * Rx (XYZ Euler)
        float r00 = cy * cz;
        float r01 = cz * sx * sy - cx * sz;
        float r02 = cx * cz * sy + sx * sz;
        float r10 = cy * sz;
        float r11 = sx * sy * sz + cx * cz;
        float r12 = cx * sy * sz - cz * sx;
        float r20 = -sy;
        float r21 = cy * sx;
        float r22 = cx * cy;

        // Combine with scale: upper-left 3x3 = R * diag(S)
        m[0][0] = r00 * scale_.x; m[0][1] = r01 * scale_.x; m[0][2] = r02 * scale_.x;
        m[1][0] = r10 * scale_.y; m[1][1] = r11 * scale_.y; m[1][2] = r12 * scale_.y;
        m[2][0] = r20 * scale_.z; m[2][1] = r21 * scale_.z; m[2][2] = r22 * scale_.z;
        m[0][3] = m[1][3] = m[2][3] = 0.0f;

        // Translation
        m[3][0] = translation_.x; m[3][1] = translation_.y; m[3][2] = translation_.z; m[3][3] = 1.0f;
    }

    void setTranslation(const Vec3& t) { translation_.x = t.x; translation_.y = t.y; translation_.z = t.z; rebuild(); }
    Translation translation() const { return translation_; }
    Vec3 rotation() const { return rotation_; }
    void setRotation(const Vec3& r) { rotation_ = r; rebuild(); }
    Vec3 scale() const { return scale_; }
    void setScale(const Vec3& s) { scale_ = s; rebuild(); }

    Matrix4 operator*(const Matrix4& o) const {
        Matrix4 r;
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++) {
                r.m[i][j] = 0;
                for (int k = 0; k < 4; k++)
                    r.m[i][j] += m[i][k] * o.m[k][j];
            }
        return r;
    }

    static Matrix4 Identity() { Matrix4 m; return m; }
    static Matrix4 fromTranslation(const Vec3& t) { Matrix4 m; m.setTranslation(t); return m; }
    static Matrix4 fromScale(const Vec3& s) { Matrix4 m; m.setScale(s); return m; }
    static Matrix4 fromRotation(const Vec3& r) { Matrix4 m; m.setRotation(r); return m; }

    QMatrix4x4 toQMatrix4x4() const {
        QMatrix4x4 qm;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                qm(i, j) = m[i][j];
        return qm;
    }
};

} // namespace ks

// Vector4 (simple 4-float vector for vertex data)
struct Vector4 {
    float x, y, z, w;
    Vector4() : x(0), y(0), z(0), w(0) {}
    Vector4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
};

// Global alias for Matrix4 used by parsers
using Matrix4 = ks::Matrix4;
using Vec4 = Vector4;
using Mat4 = Matrix4;
