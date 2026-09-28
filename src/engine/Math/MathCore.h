#pragma once
/** MathCore — Qt-free (std only). No Qt types. */

// Prevent Windows.h from defining min/max macros
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <cmath>
#include <cstring>
#include <algorithm>

// Math constants
namespace ks::math {
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 6.28318530717958647692f;
    constexpr float HALF_PI = 1.57079632679489661923f;
    constexpr float DEG_TO_RAD = PI / 180.0f;
    constexpr float RAD_TO_DEG = 180.0f / PI;
    constexpr float EPSILON = 1e-6f;
    constexpr float EPSILON_SQ = 1e-12f;
    constexpr float FLOAT_MAX = 3.402823466e+38f;
    constexpr float FLOAT_MIN = 1.175494351e-38f;

    inline float radians(float degrees) { return degrees * DEG_TO_RAD; }
    inline float degrees(float radians) { return radians * RAD_TO_DEG; }
    inline float clamp(float v, float lo, float hi) { return std::max(lo, std::min(hi, v)); }
    inline float lerp(float a, float b, float t) { return a + (b - a) * t; }
    inline float smoothstep(float a, float b, float t) {
        float x = clamp((t - a) / (b - a), 0.0f, 1.0f);
        return x * x * (3.0f - 2.0f * x);
    }
    inline float sign(float v) { return (v > 0.0f) ? 1.0f : (v < 0.0f) ? -1.0f : 0.0f; }
}

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

    float& operator[](int i) { return (&x)[i]; }
    const float& operator[](int i) const { return (&x)[i]; }
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
inline float distance(const Vec2& a, const Vec2& b) { return length(a - b); }
inline float distanceSq(const Vec2& a, const Vec2& b) { return lengthSq(a - b); }

inline Vec2 normalize(const Vec2& v) {
    float len = length(v);
    return (len > ks::math::EPSILON) ? v / len : Vec2(0.0f, 0.0f);
}

inline Vec2 lerp(const Vec2& a, const Vec2& b, float t) {
    return a * (1.0f - t) + b * t;
}

inline Vec2 perpendicular(const Vec2& v) { return Vec2(-v.y, v.x); }

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

    float& operator[](int i) { return (&x)[i]; }
    const float& operator[](int i) const { return (&x)[i]; }

    Vec3 normalized() const {
        float len = std::sqrt(x*x + y*y + z*z);
        return (len > ks::math::EPSILON) ? Vec3(x/len, y/len, z/len) : Vec3(0,0,0);
    }
    static Vec3 cross(const Vec3& a, const Vec3& b) {
        return Vec3(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x);
    }
};

inline Vec3 operator+(const Vec3& a, const Vec3& b) { return Vec3(a.x + b.x, a.y + b.y, a.z + b.z); }
inline Vec3 operator-(const Vec3& a, const Vec3& b) { return Vec3(a.x - b.x, a.y - b.y, a.z - b.z); }
inline Vec3 operator*(const Vec3& a, float s)       { return Vec3(a.x * s, a.y * s, a.z * s); }
inline Vec3 operator*(float s, const Vec3& a)       { return Vec3(a.x * s, a.y * s, a.z * s); }
inline Vec3 operator*(const Vec3& a, const Vec3& b) { return Vec3(a.x * b.x, a.y * b.y, a.z * b.z); }
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
inline float distance(const Vec3& a, const Vec3& b) { return length(a - b); }
inline float distanceSq(const Vec3& a, const Vec3& b) { return lengthSq(a - b); }

inline Vec3 normalize(const Vec3& v) {
    float len = length(v);
    return (len > ks::math::EPSILON) ? v / len : Vec3(0.0f, 0.0f, 0.0f);
}

inline Vec3 lerp(const Vec3& a, const Vec3& b, float t) {
    return a * (1.0f - t) + b * t;
}

inline Vec3 reflect(const Vec3& v, const Vec3& n) {
    return v - n * (2.0f * dot(v, n));
}

inline bool refract(const Vec3& v, const Vec3& n, float eta, Vec3& result) {
    float d = dot(v, n);
    float k = 1.0f - eta * eta * (1.0f - d * d);
    if (k < 0.0f) return false;
    result = v * eta - n * (eta * d + std::sqrt(k));
    return true;
}

inline Vec3 project(const Vec3& v, const Vec3& onto) {
    float lenSq = lengthSq(onto);
    return (lenSq > ks::math::EPSILON_SQ) ? onto * (dot(v, onto) / lenSq) : Vec3(0, 0, 0);
}

inline Vec3 reject(const Vec3& v, const Vec3& onto) {
    return v - project(v, onto);
}

inline float angleBetween(const Vec3& a, const Vec3& b) {
    float d = dot(a, b);
    float lenSq = lengthSq(a) * lengthSq(b);
    return (lenSq > ks::math::EPSILON_SQ) ? std::acos(ks::math::clamp(d / std::sqrt(lenSq), -1.0f, 1.0f)) : 0.0f;
}

inline Vec3 minVec(const Vec3& a, const Vec3& b) {
    return Vec3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
}

inline Vec3 maxVec(const Vec3& a, const Vec3& b) {
    return Vec3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
}

inline Vec3 abs(const Vec3& v) {
    return Vec3(std::abs(v.x), std::abs(v.y), std::abs(v.z));
}

using Vector2 = Vec2;
using Vector3 = Vec3;

struct Mat3 {
    float m[9];

    Mat3() { memset(m, 0, sizeof(m)); m[0] = m[4] = m[8] = 1.0f; }
    explicit Mat3(const float* data) { memcpy(m, data, sizeof(m)); }

    float& operator()(int row, int col) { return m[col * 3 + row]; }
    const float& operator()(int row, int col) const { return m[col * 3 + row]; }
    float* data() { return m; }
    const float* data() const { return m; }

    static Mat3 identity();
    static Mat3 fromRotationX(float angleRad);
    static Mat3 fromRotationY(float angleRad);
    static Mat3 fromRotationZ(float angleRad);
    static Mat3 fromScale(float sx, float sy, float sz);
    static Mat3 fromEuler(float xRad, float yRad, float zRad);

    Mat3 transposed() const;
    float determinant() const;
    Mat3 inverse() const;

    Vec3 operator*(const Vec3& v) const {
        return Vec3(
            m[0]*v.x + m[3]*v.y + m[6]*v.z,
            m[1]*v.x + m[4]*v.y + m[7]*v.z,
            m[2]*v.x + m[5]*v.y + m[8]*v.z
        );
    }

    Mat3 operator*(const Mat3& o) const {
        Mat3 r;
        for (int col = 0; col < 3; col++)
            for (int row = 0; row < 3; row++) {
                float sum = 0.0f;
                for (int k = 0; k < 3; k++)
                    sum += (*this)(row, k) * o(k, col);
                r(row, col) = sum;
            }
        return r;
    }
};

inline Mat3 Mat3::identity() { return Mat3(); }

inline Mat3 Mat3::fromRotationX(float a) {
    Mat3 r;
    float c = std::cos(a), s = std::sin(a);
    r(1,1)=c; r(1,2)=-s;
    r(2,1)=s; r(2,2)=c;
    return r;
}

inline Mat3 Mat3::fromRotationY(float a) {
    Mat3 r;
    float c = std::cos(a), s = std::sin(a);
    r(0,0)=c;  r(0,2)=s;
    r(2,0)=-s; r(2,2)=c;
    return r;
}

inline Mat3 Mat3::fromRotationZ(float a) {
    Mat3 r;
    float c = std::cos(a), s = std::sin(a);
    r(0,0)=c; r(0,1)=-s;
    r(1,0)=s; r(1,1)=c;
    return r;
}

inline Mat3 Mat3::fromScale(float sx, float sy, float sz) {
    Mat3 r;
    r(0,0)=sx; r(1,1)=sy; r(2,2)=sz;
    return r;
}

inline Mat3 Mat3::fromEuler(float x, float y, float z) {
    return fromRotationZ(z) * fromRotationY(y) * fromRotationX(x);
}

inline Mat3 Mat3::transposed() const {
    Mat3 r;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            r(i, j) = (*this)(j, i);
    return r;
}

inline float Mat3::determinant() const {
    return m[0]*(m[4]*m[8]-m[5]*m[7])
         - m[3]*(m[1]*m[8]-m[2]*m[7])
         + m[6]*(m[1]*m[5]-m[2]*m[4]);
}

inline Mat3 Mat3::inverse() const {
    float det = determinant();
    if (std::abs(det) < ks::math::EPSILON) return identity();
    float invDet = 1.0f / det;
    Mat3 r;
    r(0,0) = (m[4]*m[8]-m[5]*m[7])*invDet;
    r(0,1) = (m[2]*m[7]-m[1]*m[8])*invDet;
    r(0,2) = (m[1]*m[5]-m[2]*m[4])*invDet;
    r(1,0) = (m[5]*m[6]-m[3]*m[8])*invDet;
    r(1,1) = (m[0]*m[8]-m[2]*m[6])*invDet;
    r(1,2) = (m[2]*m[3]-m[0]*m[5])*invDet;
    r(2,0) = (m[3]*m[7]-m[4]*m[6])*invDet;
    r(2,1) = (m[1]*m[6]-m[0]*m[7])*invDet;
    r(2,2) = (m[0]*m[4]-m[1]*m[3])*invDet;
    return r;
}

struct Quat {
    float x, y, z, w;

    Quat() : x(0), y(0), z(0), w(1) {}
    Quat(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}

    static Quat identity() { return Quat(0, 0, 0, 1); }
    static Quat fromAxisAngle(const Vec3& axis, float angleRad);
    static Quat fromEuler(float xRad, float yRad, float zRad);
    static Quat fromEulerDeg(float xDeg, float yDeg, float zDeg) {
        return fromEuler(ks::math::radians(xDeg), ks::math::radians(yDeg), ks::math::radians(zDeg));
    }
    static Quat fromMatrix(const Mat3& m);
    static Quat fromTwoVectors(const Vec3& from, const Vec3& to);

    Quat conjugated() const { return Quat(-x, -y, -z, w); }
    Quat inverse() const;
    float length() const { return std::sqrt(x*x + y*y + z*z + w*w); }
    float lengthSq() const { return x*x + y*y + z*z + w*w; }
    Quat normalized() const;

    Quat operator*(const Quat& o) const {
        return Quat(
            w*o.x + x*o.w + y*o.z - z*o.y,
            w*o.y - x*o.z + y*o.w + z*o.x,
            w*o.z + x*o.y - y*o.x + z*o.w,
            w*o.w - x*o.x - y*o.y - z*o.z
        );
    }

    Vec3 operator*(const Vec3& v) const {
        Quat p(v.x, v.y, v.z, 0);
        Quat q = *this * p * conjugated();
        return Vec3(q.x, q.y, q.z);
    }

    Mat3 toMat3() const;
    Vec3 toEuler() const;
    Quat slerp(const Quat& to, float t) const;
    Quat operator/(float s) const { return Quat(x/s, y/s, z/s, w/s); }

    Vec3 axis() const {
        float len = std::sqrt(x*x + y*y + z*z);
        return (len > ks::math::EPSILON) ? Vec3(x/len, y/len, z/len) : Vec3(1, 0, 0);
    }
    float angle() const {
        return 2.0f * std::acos(ks::math::clamp(w, -1.0f, 1.0f));
    }
};

inline Quat Quat::fromAxisAngle(const Vec3& axis, float angleRad) {
    float halfAngle = angleRad * 0.5f;
    float s = std::sin(halfAngle);
    Vec3 n = normalize(axis);
    return Quat(n.x * s, n.y * s, n.z * s, std::cos(halfAngle));
}

inline Quat Quat::fromEuler(float xRad, float yRad, float zRad) {
    float cx = std::cos(xRad * 0.5f), sx = std::sin(xRad * 0.5f);
    float cy = std::cos(yRad * 0.5f), sy = std::sin(yRad * 0.5f);
    float cz = std::cos(zRad * 0.5f), sz = std::sin(zRad * 0.5f);
    return Quat(
        sx*cy*cz + cx*sy*sz,
        cx*sy*cz - sx*cy*sz,
        cx*cy*sz + sx*sy*cz,
        cx*cy*cz - sx*sy*sz
    );
}

inline Quat Quat::fromMatrix(const Mat3& m) {
    float trace = m(0,0) + m(1,1) + m(2,2);
    if (trace > 0.0f) {
        float s = 0.5f / std::sqrt(trace + 1.0f);
        return Quat((m(2,1) - m(1,2)) * s, (m(0,2) - m(2,0)) * s, (m(1,0) - m(0,1)) * s, 0.25f / s);
    } else if (m(0,0) > m(1,1) && m(0,0) > m(2,2)) {
        float s = 2.0f * std::sqrt(1.0f + m(0,0) - m(1,1) - m(2,2));
        return Quat(0.25f * s, (m(0,1) + m(1,0)) / s, (m(0,2) + m(2,0)) / s, (m(2,1) - m(1,2)) / s);
    } else if (m(1,1) > m(2,2)) {
        float s = 2.0f * std::sqrt(1.0f + m(1,1) - m(0,0) - m(2,2));
        return Quat((m(0,1) + m(1,0)) / s, 0.25f * s, (m(1,2) + m(2,1)) / s, (m(0,2) - m(2,0)) / s);
    } else {
        float s = 2.0f * std::sqrt(1.0f + m(2,2) - m(0,0) - m(1,1));
        return Quat((m(0,2) + m(2,0)) / s, (m(1,2) + m(2,1)) / s, 0.25f * s, (m(1,0) - m(0,1)) / s);
    }
}

inline Quat Quat::fromTwoVectors(const Vec3& from, const Vec3& to) {
    Vec3 f = normalize(from);
    Vec3 t = normalize(to);
    float d = dot(f, t);
    if (d > 0.9999f) return identity();
    if (d < -0.9999f) {
        Vec3 axis = Vec3::cross(f, Vec3(1, 0, 0));
        if (::lengthSq(axis) < ks::math::EPSILON_SQ) axis = Vec3::cross(f, Vec3(0, 1, 0));
        axis = normalize(axis);
        return Quat(axis.x, axis.y, axis.z, 0);
    }
    Vec3 axis = Vec3::cross(f, t);
    return Quat(axis.x, axis.y, axis.z, 1.0f + d).normalized();
}

inline Quat Quat::inverse() const {
    float lenSq = this->lengthSq();
    return (lenSq > ks::math::EPSILON_SQ) ? conjugated() / lenSq : identity();
}

inline Quat Quat::normalized() const {
    float len = length();
    return (len > ks::math::EPSILON) ? Quat(x/len, y/len, z/len, w/len) : identity();
}

inline Quat Quat::slerp(const Quat& to, float t) const {
    float cosOmega = x*to.x + y*to.y + z*to.z + w*to.w;
    Quat to2 = to;
    if (cosOmega < 0.0f) { cosOmega = -cosOmega; to2 = Quat(-to.x, -to.y, -to.z, -to.w); }
    float k0, k1;
    if (cosOmega > 0.9999f) {
        k0 = 1.0f - t;
        k1 = t;
    } else {
        float sinOmega = std::sqrt(1.0f - cosOmega*cosOmega);
        float omega = std::atan2(sinOmega, cosOmega);
        float invSin = 1.0f / sinOmega;
        k0 = std::sin((1.0f - t) * omega) * invSin;
        k1 = std::sin(t * omega) * invSin;
    }
    return Quat(x*k0 + to2.x*k1, y*k0 + to2.y*k1, z*k0 + to2.z*k1, w*k0 + to2.w*k1);
}

inline Mat3 Quat::toMat3() const {
    float x2 = x*x, y2 = y*y, z2 = z*z;
    float xy = x*y, xz = x*z, yz = y*z;
    float wx = w*x, wy = w*y, wz = w*z;
    Mat3 r;
    r(0,0) = 1.0f - 2.0f*(y2 + z2); r(0,1) = 2.0f*(xy - wz);        r(0,2) = 2.0f*(xz + wy);
    r(1,0) = 2.0f*(xy + wz);        r(1,1) = 1.0f - 2.0f*(x2 + z2); r(1,2) = 2.0f*(yz - wx);
    r(2,0) = 2.0f*(xz - wy);        r(2,1) = 2.0f*(yz + wx);        r(2,2) = 1.0f - 2.0f*(x2 + y2);
    return r;
}

inline Vec3 Quat::toEuler() const {
    float sinr = 2.0f*(w*x + y*z);
    float cosr = 1.0f - 2.0f*(x*x + y*y);
    float roll = std::atan2(sinr, cosr);
    float sinp = 2.0f*(w*y - z*x);
    float pitch = (std::abs(sinp) >= 1.0f) ? std::copysign(ks::math::HALF_PI, sinp) : std::asin(sinp);
    float siny = 2.0f*(w*z + x*y);
    float cosy = 1.0f - 2.0f*(y*y + z*z);
    float yaw = std::atan2(siny, cosy);
    return Vec3(roll, pitch, yaw);
}

struct Ray {
    Vec3 origin;
    Vec3 direction;
    Ray() = default;
    Ray(const Vec3& o, const Vec3& d) : origin(o), direction(normalize(d)) {}
    Vec3 at(float t) const { return origin + direction * t; }
};

struct Plane {
    Vec3 normal;
    float distance;
    Plane() : normal(0, 1, 0), distance(0) {}
    Plane(const Vec3& n, float d) : normal(normalize(n)), distance(d) {}
    Plane(const Vec3& a, const Vec3& b, const Vec3& c) {
        normal = normalize(cross(b - a, c - a));
        distance = dot(normal, a);
    }
    float signedDistance(const Vec3& p) const { return dot(normal, p) - distance; }
    Vec3 projectPoint(const Vec3& p) const { return p - normal * signedDistance(p); }
    bool isFrontFacing(const Vec3& viewDir) const { return dot(normal, viewDir) < 0.0f; }
};

struct AABB {
    Vec3 min;
    Vec3 max;
    AABB() : min(ks::math::FLOAT_MAX, ks::math::FLOAT_MAX, ks::math::FLOAT_MAX),
             max(-ks::math::FLOAT_MAX, -ks::math::FLOAT_MAX, -ks::math::FLOAT_MAX) {}
    AABB(const Vec3& p) : min(p), max(p) {}
    AABB(const Vec3& min_, const Vec3& max_) : min(min_), max(max_) {}
    Vec3 center() const { return (min + max) * 0.5f; }
    Vec3 extents() const { return (max - min) * 0.5f; }
    Vec3 size() const { return max - min; }
    bool isValid() const { return min.x <= max.x && min.y <= max.y && min.z <= max.z; }
    void expand(const Vec3& p) { min = minVec(min, p); max = maxVec(max, p); }
    void expand(const AABB& other) { min = minVec(min, other.min); max = maxVec(max, other.max); }
    bool contains(const Vec3& p) const {
        return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y && p.z >= min.z && p.z <= max.z;
    }
};

namespace ks {

struct Matrix4 {
    float m[4][4];
    Vec3 translation_;

    Matrix4() {
        memset(m, 0, sizeof(m));
        m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.0f;
    }

    float* data() { return &m[0][0]; }
    const float* data() const { return &m[0][0]; }

    static Matrix4 identity() { return Matrix4(); }
    static Matrix4 fromQuat(const Quat& q);
    static Matrix4 lookAt(const Vec3& eye, const Vec3& target, const Vec3& up);
    static Matrix4 perspective(float fovRad, float aspect, float nearZ, float farZ);
    static Matrix4 orthographic(float left, float right, float bottom, float top, float nearZ, float farZ);

    Mat3 toMat3() const {
        Mat3 r;
        r(0,0)=m[0][0]; r(0,1)=m[0][1]; r(0,2)=m[0][2];
        r(1,0)=m[1][0]; r(1,1)=m[1][1]; r(1,2)=m[1][2];
        r(2,0)=m[2][0]; r(2,1)=m[2][1]; r(2,2)=m[2][2];
        return r;
    }
    Quat toQuat() const { return Quat::fromMatrix(toMat3()); }
};

inline Matrix4 Matrix4::fromQuat(const Quat& q) {
    Mat3 r = q.toMat3();
    Matrix4 out;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            out.m[i][j] = r(i, j);
    out.m[3][3] = 1.0f;
    return out;
}

inline Matrix4 Matrix4::lookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
    Vec3 f = normalize(target - eye);
    Vec3 s = normalize(cross(f, up));
    Vec3 u = cross(s, f);
    Matrix4 out;
    out.m[0][0] = s.x; out.m[0][1] = s.y; out.m[0][2] = s.z;
    out.m[1][0] = u.x; out.m[1][1] = u.y; out.m[1][2] = u.z;
    out.m[2][0] = -f.x; out.m[2][1] = -f.y; out.m[2][2] = -f.z;
    out.m[3][0] = -dot(s, eye); out.m[3][1] = -dot(u, eye); out.m[3][2] = dot(f, eye);
    out.m[3][3] = 1.0f;
    out.m[0][3] = out.m[1][3] = out.m[2][3] = 0.0f;
    out.translation_ = Vec3(out.m[3][0], out.m[3][1], out.m[3][2]);
    return out;
}

inline Matrix4 Matrix4::perspective(float fovRad, float aspect, float nearZ, float farZ) {
    float f = 1.0f / std::tan(fovRad * 0.5f);
    Matrix4 out;
    memset(out.m, 0, sizeof(out.m));
    out.m[0][0] = f / aspect;
    out.m[1][1] = f;
    out.m[2][2] = (farZ + nearZ) / (nearZ - farZ);
    out.m[2][3] = -1.0f;
    out.m[3][2] = (2.0f * farZ * nearZ) / (nearZ - farZ);
    return out;
}

inline Matrix4 Matrix4::orthographic(float left, float right, float bottom, float top, float nearZ, float farZ) {
    Matrix4 out;
    memset(out.m, 0, sizeof(out.m));
    out.m[0][0] = 2.0f / (right - left);
    out.m[1][1] = 2.0f / (top - bottom);
    out.m[2][2] = -2.0f / (farZ - nearZ);
    out.m[3][0] = -(right + left) / (right - left);
    out.m[3][1] = -(top + bottom) / (top - bottom);
    out.m[3][2] = -(farZ + nearZ) / (farZ - nearZ);
    out.m[3][3] = 1.0f;
    return out;
}

} // namespace ks

struct Vector4 {
    float x, y, z, w;
    Vector4() : x(0), y(0), z(0), w(0) {}
    Vector4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
};

using Matrix4 = ks::Matrix4;
using Vec4 = Vector4;
using Mat4 = Matrix4;
