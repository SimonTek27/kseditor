#pragma once

#ifdef KSENGINE_NO_QT

#include <string>
#include <string_view>
#include <vector>
#include <deque>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <functional>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <memory>
#include <mutex>
#include <chrono>
#include <atomic>
#include <numeric>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <functional>
#include <iostream>
#include <iomanip>
#include <sys/stat.h>
#include <cstdio>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#endif

// ============================================================================
// Qt macro stubs
// ============================================================================
#define Q_OBJECT
#define Q_PROPERTY(x)
#define Q_INVOKABLE
#define Q_DISABLE_COPY(x)
#define Q_UNUSED(x) ((void)(x))
#define Q_DECLARE_METATYPE(x)
#define Q_ENUM(x)
#define QML_ELEMENT
#define signals public
#define slots
#define emit

// ============================================================================
// Basic Qt type aliases
// ============================================================================
using QString = std::string;
using QStringList = std::vector<std::string>;
using QByteArray = std::vector<char>;
using uchar = unsigned char;
using uint = unsigned int;
using quint8 = uint8_t;
using quint16 = uint16_t;
using quint32 = uint32_t;
using quint64 = uint64_t;
using qint8 = int8_t;
using qint16 = int16_t;
using qint32 = int32_t;
using qint64 = int64_t;

// QString compatibility
namespace QStringCompat {
    inline std::string fromStdString(const std::string& s) { return s; }
    inline std::string fromUtf8(const char* s) { return s ? std::string(s) : std::string(); }
    inline std::string fromUtf8(const char* s, int len) { return s ? std::string(s, len) : std::string(); }
    inline std::string fromLatin1(const char* s) { return s ? std::string(s) : std::string(); }
    inline std::string fromLocal8Bit(const char* s) { return s ? std::string(s) : std::string(); }
    inline std::string number(int n) { return std::to_string(n); }
    inline std::string number(double n) { return std::to_string(n); }
    inline std::string number(float n) { return std::to_string(n); }
    inline std::string number(long long n) { return std::to_string(n); }
}

// Make fromStdString/fromUtf8 etc. callable on std::string as well
namespace std {
    inline std::string fromStdString(const std::string& s) { return s; }
}

// ============================================================================
// Math types (match Qt API surface)
// ============================================================================
struct QVector2D {
    float xp = 0, yp = 0;
    QVector2D() = default;
    QVector2D(float x, float y) : xp(x), yp(y) {}
    float x() const { return xp; }
    float y() const { return yp; }
    void setX(float x) { xp = x; }
    void setY(float y) { yp = y; }
    float length() const { return std::sqrt(xp*xp + yp*yp); }
    QVector2D normalized() const { float l = length(); return l > 0 ? QVector2D(xp/l, yp/l) : QVector2D(); }
    float dotProduct(const QVector2D& o) const { return xp*o.xp + yp*o.yp; }
    static float dotProduct(const QVector2D& a, const QVector2D& b) { return a.xp*b.xp + a.yp*b.yp; }
    QVector2D operator+(const QVector2D& o) const { return {xp+o.xp, yp+o.yp}; }
    QVector2D operator-(const QVector2D& o) const { return {xp-o.xp, yp-o.yp}; }
    QVector2D operator*(float s) const { return {xp*s, yp*s}; }
    bool operator==(const QVector2D& o) const { return xp==o.xp && yp==o.yp; }
    bool operator!=(const QVector2D& o) const { return !(*this == o); }
};

struct QVector3D {
    float xp = 0, yp = 0, zp = 0;
    QVector3D() = default;
    QVector3D(float x, float y, float z) : xp(x), yp(y), zp(z) {}
    float x() const { return xp; }
    float y() const { return yp; }
    float z() const { return zp; }
    void setX(float x) { xp = x; }
    void setY(float y) { yp = y; }
    void setZ(float z) { zp = z; }
    float length() const { return std::sqrt(xp*xp + yp*yp + zp*zp); }
    float lengthSquared() const { return xp*xp + yp*yp + zp*zp; }
    QVector3D normalized() const { float l = length(); return l > 0 ? QVector3D(xp/l, yp/l, zp/l) : QVector3D(); }
    float dotProduct(const QVector3D& o) const { return xp*o.xp + yp*o.yp + zp*o.zp; }
    static float dotProduct(const QVector3D& a, const QVector3D& b) { return a.xp*b.xp + a.yp*b.yp + a.zp*b.zp; }
    static QVector3D crossProduct(const QVector3D& a, const QVector3D& b) {
        return {a.yp*b.zp - a.zp*b.yp, a.zp*b.xp - a.xp*b.zp, a.xp*b.yp - a.yp*b.xp};
    }
    QVector3D cross(const QVector3D& o) const { return crossProduct(*this, o); }
    QVector3D operator+(const QVector3D& o) const { return {xp+o.xp, yp+o.yp, zp+o.zp}; }
    QVector3D operator-(const QVector3D& o) const { return {xp-o.xp, yp-o.yp, zp-o.zp}; }
    QVector3D operator*(float s) const { return {xp*s, yp*s, zp*s}; }
    QVector3D operator/(float s) const { return {xp/s, yp/s, zp/s}; }
    QVector3D& operator+=(const QVector3D& o) { xp+=o.xp; yp+=o.yp; zp+=o.zp; return *this; }
    QVector3D& operator-=(const QVector3D& o) { xp-=o.xp; yp-=o.yp; zp-=o.zp; return *this; }
    QVector3D& operator*=(float s) { xp*=s; yp*=s; zp*=s; return *this; }
    bool operator==(const QVector3D& o) const { return xp==o.xp && yp==o.yp && zp==o.zp; }
    bool operator!=(const QVector3D& o) const { return !(*this == o); }
    bool fuzzyCompare(const QVector3D& o) const {
        return std::abs(xp-o.xp) < 0.001f && std::abs(yp-o.yp) < 0.001f && std::abs(zp-o.zp) < 0.001f;
    }
};

struct QVector4D {
    float xp = 0, yp = 0, zp = 0, wp = 0;
    QVector4D() = default;
    QVector4D(float x, float y, float z, float w) : xp(x), yp(y), zp(z), wp(w) {}
    QVector4D(const QVector3D& v, float w) : xp(v.x()), yp(v.y()), zp(v.z()), wp(w) {}
    float x() const { return xp; }
    float y() const { return yp; }
    float z() const { return zp; }
    float w() const { return wp; }
    void setX(float x) { xp = x; }
    void setY(float y) { yp = y; }
    void setZ(float z) { zp = z; }
    void setW(float w) { wp = w; }
    QVector3D toVector3D() const { return {xp, yp, zp}; }
    QVector4D operator+(const QVector4D& o) const { return {xp+o.xp, yp+o.yp, zp+o.zp, wp+o.wp}; }
    QVector4D operator-(const QVector4D& o) const { return {xp-o.xp, yp-o.yp, zp-o.zp, wp-o.wp}; }
    QVector4D operator*(float s) const { return {xp*s, yp*s, zp*s, wp*s}; }
    bool operator==(const QVector4D& o) const { return xp==o.xp && yp==o.yp && zp==o.zp && wp==o.wp; }
};

struct QQuaternion {
    float xp = 0, yp = 0, zp = 0, wp = 1;
    QQuaternion() = default;
    QQuaternion(float x, float y, float z, float w) : xp(x), yp(y), zp(z), wp(w) {}
    QQuaternion(const QVector3D& axis, float angle) {
        float half = angle * 0.5f;
        float s = std::sin(half);
        float len = axis.length();
        if (len > 0) { float ax = axis.x()/len, ay = axis.y()/len, az = axis.z()/len;
            xp = ax*s; yp = ay*s; zp = az*s; wp = std::cos(half);
        }
    }
    static QQuaternion fromAxisAndAngle(const QVector3D& axis, float angle) { return QQuaternion(axis, angle * 3.14159265f / 180.0f); }
    static QQuaternion fromAxisAndAngle(float x, float y, float z, float angle) { return QQuaternion(QVector3D(x,y,z), angle * 3.14159265f / 180.0f); }
    static QQuaternion fromEulerAngles(float pitch, float yaw, float roll) {
        float cp = std::cos(pitch*0.5f), sp = std::sin(pitch*0.5f);
        float cy = std::cos(yaw*0.5f), sy = std::sin(yaw*0.5f);
        float cr = std::cos(roll*0.5f), sr = std::sin(roll*0.5f);
        return {sr*cp*cy - cr*sy*sp, cr*sp*cy + sr*sy*cp, cr*cp*sy - sr*sp*cy, cr*cp*cy + sr*sp*sy};
    }
    QVector3D toEulerAngles() const { return {0, 0, 0}; }
    static QQuaternion slerp(const QQuaternion& a, const QQuaternion& b, float t) { (void)a; (void)b; (void)t; return {}; }
    static float dotProduct(const QQuaternion& a, const QQuaternion& b) { return a.xp*b.xp + a.yp*b.yp + a.zp*b.zp + a.wp*b.wp; }
    QQuaternion normalized() const { float l = std::sqrt(xp*xp+yp*yp+zp*zp+wp*wp); return l>0 ? QQuaternion(xp/l,yp/l,zp/l,wp/l) : QQuaternion(); }
    QQuaternion conjugated() const { return {-xp,-yp,-zp,wp}; }
    QVector3D rotatedVector(const QVector3D& v) const {
        QVector3D qv{xp,yp,zp};
        QVector3D uv = qv.cross(v);
        QVector3D uuv = qv.cross(uv);
        return v + (uv * wp + uuv) * 2.0f;
    }
    QQuaternion operator*(const QQuaternion& o) const {
        return {wp*o.xp + xp*o.wp + yp*o.zp - zp*o.yp,
                wp*o.yp - xp*o.zp + yp*o.wp + zp*o.xp,
                wp*o.zp + xp*o.yp - yp*o.xp + zp*o.wp,
                wp*o.wp - xp*o.xp - yp*o.yp - zp*o.zp};
    }
    bool operator==(const QQuaternion& o) const { return xp==o.xp && yp==o.yp && zp==o.zp && wp==o.wp; }
};

struct QMatrix4x4 {
    float m[16] = {};
    QMatrix4x4() { m[0]=1; m[5]=1; m[10]=1; m[15]=1; }
    QMatrix4x4(const float* data) { memcpy(m, data, sizeof(m)); }

    float& operator()(int row, int col) { return m[col * 4 + row]; }
    float operator()(int row, int col) const { return m[col * 4 + row]; }

    QVector4D column(int i) const { return {m[i*4], m[i*4+1], m[i*4+2], m[i*4+3]}; }
    void setColumn(int i, const QVector4D& v) { m[i*4]=v.x(); m[i*4+1]=v.y(); m[i*4+2]=v.z(); m[i*4+3]=v.w(); }
    QVector4D row(int i) const { return {m[i], m[i+4], m[i+8], m[i+12]}; }

    const float* constData() const { return m; }
    float* data() { return m; }

    QMatrix4x4 inverted(bool* ok = nullptr) const {
        QMatrix4x4 inv;
        float det;
        float* o = inv.m;
        const float* s = m;
        o[0]  =  s[5]*s[10]*s[15] - s[5]*s[11]*s[14] - s[9]*s[6]*s[15] + s[9]*s[7]*s[14] + s[13]*s[6]*s[11] - s[13]*s[7]*s[10];
        o[4]  = -s[4]*s[10]*s[15] + s[4]*s[11]*s[14] + s[8]*s[6]*s[15] - s[8]*s[7]*s[14] - s[12]*s[6]*s[11] + s[12]*s[7]*s[10];
        o[8]  =  s[4]*s[9]*s[15]  - s[4]*s[11]*s[13] - s[8]*s[5]*s[15] + s[8]*s[7]*s[13] + s[12]*s[5]*s[11] - s[12]*s[7]*s[9];
        o[12] = -s[4]*s[9]*s[14]  + s[4]*s[10]*s[13] + s[8]*s[5]*s[14] - s[8]*s[6]*s[13] - s[12]*s[5]*s[10] + s[12]*s[6]*s[9];
        o[1]  = -s[1]*s[10]*s[15] + s[1]*s[11]*s[14] + s[9]*s[2]*s[15] - s[9]*s[3]*s[14] - s[13]*s[2]*s[11] + s[13]*s[3]*s[10];
        o[5]  =  s[0]*s[10]*s[15] - s[0]*s[11]*s[14] - s[8]*s[2]*s[15] + s[8]*s[3]*s[14] + s[12]*s[2]*s[11] - s[12]*s[3]*s[10];
        o[9]  = -s[0]*s[9]*s[15]  + s[0]*s[11]*s[13] + s[8]*s[1]*s[15] - s[8]*s[3]*s[13] - s[12]*s[1]*s[11] + s[12]*s[3]*s[9];
        o[13] =  s[0]*s[9]*s[14]  - s[0]*s[10]*s[13] - s[8]*s[1]*s[14] + s[8]*s[2]*s[13] + s[12]*s[1]*s[10] - s[12]*s[2]*s[9];
        o[2]  =  s[1]*s[6]*s[15]  - s[1]*s[7]*s[14]  - s[5]*s[2]*s[15] + s[5]*s[3]*s[14] + s[13]*s[2]*s[7]  - s[13]*s[3]*s[6];
        o[6]  = -s[0]*s[6]*s[15]  + s[0]*s[7]*s[14]  + s[4]*s[2]*s[15] - s[4]*s[3]*s[14] - s[12]*s[2]*s[7]  + s[12]*s[3]*s[6];
        o[10] =  s[0]*s[5]*s[15]  - s[0]*s[7]*s[13]  - s[4]*s[1]*s[15] + s[4]*s[3]*s[13] + s[12]*s[1]*s[7]  - s[12]*s[3]*s[5];
        o[14] = -s[0]*s[5]*s[14]  + s[0]*s[6]*s[13]  + s[4]*s[1]*s[14] - s[4]*s[2]*s[13] - s[12]*s[1]*s[6]  + s[12]*s[2]*s[5];
        o[3]  = -s[1]*s[6]*s[11]  + s[1]*s[7]*s[10]  + s[5]*s[2]*s[11] - s[5]*s[3]*s[10] - s[9]*s[2]*s[7]   + s[9]*s[3]*s[6];
        o[7]  =  s[0]*s[6]*s[11]  - s[0]*s[7]*s[10]  - s[4]*s[2]*s[11] + s[4]*s[3]*s[10] + s[8]*s[2]*s[7]   - s[8]*s[3]*s[6];
        o[11] = -s[0]*s[5]*s[11]  + s[0]*s[7]*s[9]   + s[4]*s[1]*s[11] - s[4]*s[3]*s[9]  - s[8]*s[1]*s[7]   + s[8]*s[3]*s[5];
        o[15] =  s[0]*s[5]*s[10]  - s[0]*s[6]*s[9]   - s[4]*s[1]*s[10] + s[4]*s[2]*s[9]  + s[8]*s[1]*s[6]   - s[8]*s[2]*s[5];
        det = s[0]*o[0] + s[1]*o[4] + s[2]*o[8] + s[3]*o[12];
        if (std::abs(det) < 1e-10f) { if (ok) *ok = false; return QMatrix4x4(); }
        det = 1.0f / det;
        for (int i = 0; i < 16; i++) o[i] *= det;
        if (ok) *ok = true;
        return inv;
    }

    QMatrix4x4 transposed() const {
        QMatrix4x4 r;
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                r(i,j) = (*this)(j,i);
        return r;
    }

    QMatrix4x4 operator*(const QMatrix4x4& o) const {
        QMatrix4x4 r;
        memset(r.m, 0, sizeof(r.m));
        for (int c = 0; c < 4; c++)
            for (int row = 0; row < 4; row++)
                for (int k = 0; k < 4; k++)
                    r(row, c) += (*this)(row, k) * o(k, c);
        return r;
    }

    QVector4D operator*(const QVector4D& v) const {
        return {
            m[0]*v.x() + m[4]*v.y() + m[8]*v.z() + m[12]*v.w(),
            m[1]*v.x() + m[5]*v.y() + m[9]*v.z() + m[13]*v.w(),
            m[2]*v.x() + m[6]*v.y() + m[10]*v.z() + m[14]*v.w(),
            m[3]*v.x() + m[7]*v.y() + m[11]*v.z() + m[15]*v.w()
        };
    }

    QVector3D operator*(const QVector3D& v) const {
        return {
            m[0]*v.x() + m[4]*v.y() + m[8]*v.z() + m[12],
            m[1]*v.x() + m[5]*v.y() + m[9]*v.z() + m[13],
            m[2]*v.x() + m[6]*v.y() + m[10]*v.z() + m[14]
        };
    }

    static QMatrix4x4 fromScale(float sx, float sy, float sz) {
        QMatrix4x4 r;
        r(0,0)=sx; r(1,1)=sy; r(2,2)=sz;
        return r;
    }

    static QMatrix4x4 fromTranslation(float x, float y, float z) {
        QMatrix4x4 r;
        r(0,3)=x; r(1,3)=y; r(2,3)=z;
        return r;
    }

    static QMatrix4x4 lookAt(const QVector3D& eye, const QVector3D& center, const QVector3D& up) {
        QVector3D f = (center - eye).normalized();
        QVector3D s = QVector3D::crossProduct(f, up).normalized();
        QVector3D u = QVector3D::crossProduct(s, f);
        QMatrix4x4 r;
        r(0,0)=s.x();  r(0,1)=s.y();  r(0,2)=s.z();  r(0,3)=-QVector3D::dotProduct(s, eye);
        r(1,0)=u.x();  r(1,1)=u.y();  r(1,2)=u.z();  r(1,3)=-QVector3D::dotProduct(u, eye);
        r(2,0)=-f.x(); r(2,1)=-f.y(); r(2,2)=-f.z(); r(2,3)=QVector3D::dotProduct(f, eye);
        return r;
    }

    static QMatrix4x4 perspective(float fovY, float aspect, float zNear, float zFar) {
        float tanHalf = std::tan(fovY * 0.5f);
        QMatrix4x4 r;
        memset(r.m, 0, sizeof(r.m));
        r(0,0) = 1.0f / (aspect * tanHalf);
        r(1,1) = 1.0f / tanHalf;
        r(2,2) = -(zFar + zNear) / (zFar - zNear);
        r(2,3) = -(2.0f * zFar * zNear) / (zFar - zNear);
        r(3,2) = -1.0f;
        return r;
    }

    static QMatrix4x4 ortho(float left, float right, float bottom, float top, float nearPlane, float farPlane) {
        QMatrix4x4 r;
        memset(r.m, 0, sizeof(r.m));
        r(0,0) = 2.0f / (right - left);
        r(1,1) = 2.0f / (top - bottom);
        r(2,2) = -1.0f / (farPlane - nearPlane);
        r(0,3) = -(right + left) / (right - left);
        r(1,3) = -(top + bottom) / (top - bottom);
        r(2,3) = -nearPlane / (farPlane - nearPlane);
        return r;
    }

    QMatrix4x4& translate(const QVector3D& v) { *this = *this * fromTranslation(v.x(), v.y(), v.z()); return *this; }
    QMatrix4x4& scale(float sx, float sy, float sz) { *this = *this * fromScale(sx, sy, sz); return *this; }
};

struct QMatrix2x2 { float m[4] = {}; };
struct QMatrix3x3 { float m[9] = {}; };

inline QMatrix2x2 operator*(const QMatrix2x2& a, const QMatrix2x2& b) { (void)a; (void)b; return {}; }
inline QMatrix3x3 operator*(const QMatrix3x3& a, const QMatrix3x3& b) { (void)a; (void)b; return {}; }

// ============================================================================
// Math utility functions
// ============================================================================
inline float qBound(float min, float val, float max) { return std::max(min, std::min(max, val)); }
inline double qBound(double min, double val, double max) { return std::max(min, std::min(max, val)); }
inline int qBound(int min, int val, int max) { return std::max(min, std::min(max, val)); }
inline float qMax(float a, float b) { return std::max(a, b); }
inline float qMin(float a, float b) { return std::min(a, b); }
inline int qMax(int a, int b) { return std::max(a, b); }
inline int qMin(int a, int b) { return std::min(a, b); }
inline double qMax(double a, double b) { return std::max(a, b); }
inline double qMin(double a, double b) { return std::min(a, b); }
inline float qAbs(float f) { return std::abs(f); }
inline int qAbs(int i) { return std::abs(i); }
inline double qAbs(double d) { return std::abs(d); }
inline bool qFuzzyCompare(float a, float b) { return std::abs(a - b) < 0.0001f; }

// ============================================================================
// Color
// ============================================================================
struct QColor {
    float r=0, g=0, b=0, a=1;
    QColor() = default;
    QColor(int r, int g, int b, int a=255) : r(r/255.0f), g(g/255.0f), b(b/255.0f), a(a/255.0f) {}
    QColor(float r, float g, float b, float a=1.0f) : r(r), g(g), b(b), a(a) {}
    int red() const { return int(r*255); }
    int green() const { return int(g*255); }
    int blue() const { return int(b*255); }
    int alpha() const { return int(a*255); }
    float redF() const { return r; }
    float greenF() const { return g; }
    float blueF() const { return b; }
    float alphaF() const { return a; }
    void setRed(int v) { r = v/255.0f; }
    void setGreen(int v) { g = v/255.0f; }
    void setBlue(int v) { b = v/255.0f; }
    void setAlpha(int v) { a = v/255.0f; }
    bool isValid() const { return true; }
    uint32_t rgba() const {
        return (uint32_t(int(r*255)) | (uint32_t(int(g*255)) << 8) |
                (uint32_t(int(b*255)) << 16) | (uint32_t(int(a*255)) << 24));
    }
    static QColor fromHsvF(float h, float s, float v, float a=1.0f) {
        (void)h; (void)s; (void)v;
        return QColor(1.0f, 1.0f, 1.0f, a);
    }
    static QColor fromRgb(int r, int g, int b, int a=255) { return QColor(r, g, b, a); }
    static QColor fromRgbF(float r, float g, float b, float a=1.0f) { return QColor(r, g, b, a); }
};
namespace Qt { static const QColor black(0, 0, 0); static const QColor white(255, 255, 255); static const QColor red(255, 0, 0); static const QColor green(0, 255, 0); static const QColor blue(0, 0, 255); static const QColor transparent(0, 0, 0, 0); }

// ============================================================================
// Containers
// ============================================================================
template<typename T> using QVector = std::vector<T>;
template<typename T> using QList = std::vector<T>;
template<typename T> using QSet = std::unordered_set<T>;
template<typename T> using QStack = std::vector<T>;
template<typename T> using QQueue = std::deque<T>;
template<typename K, typename V> using QMap = std::map<K, V>;
template<typename K, typename V> using QHash = std::unordered_map<K, V>;
template<typename A, typename B> using QPair = std::pair<A, B>;
using QStringList = std::vector<std::string>;

// ============================================================================
// QVariant (minimal stub)
// ============================================================================
class QVariant {
public:
    enum Type { Invalid, Bool, Int, UInt, LongLong, Double, String, ByteArray, StringList, Map, List };
    QVariant() = default;
    QVariant(bool v) : m_type(Bool), m_bool(v) {}
    QVariant(int v) : m_type(Int), m_int(v) {}
    QVariant(uint v) : m_type(UInt), m_uint(v) {}
    QVariant(long long v) : m_type(LongLong), m_ll(v) {}
    QVariant(float v) : m_type(Double), m_double(v) {}
    QVariant(double v) : m_type(Double), m_double(v) {}
    QVariant(const std::string& v) : m_type(String), m_string(v) {}
    QVariant(const char* v) : m_type(String), m_string(v ? v : "") {}
    bool isValid() const { return m_type != Invalid; }
    bool toBool() const { return m_bool; }
    int toInt(bool* ok = nullptr) const { if(ok) *ok = m_type==Int; return m_int; }
    uint toUInt(bool* ok = nullptr) const { if(ok) *ok = m_type==UInt; return m_uint; }
    float toFloat(bool* ok = nullptr) const { if(ok) *ok = m_type==Double; return float(m_double); }
    double toDouble(bool* ok = nullptr) const { if(ok) *ok = m_type==Double; return m_double; }
    long long toLongLong(bool* ok = nullptr) const { if(ok) *ok = m_type==LongLong; return m_ll; }
    std::string toString() const { return m_string; }
    Type type() const { return m_type; }
    template<typename T> static QVariant fromValue(const T& v) { (void)v; return QVariant(); }
private:
    Type m_type = Invalid;
    bool m_bool = false;
    int m_int = 0;
    uint m_uint = 0;
    long long m_ll = 0;
    double m_double = 0;
    std::string m_string;
};

using QVariantList = std::vector<QVariant>;
using QVariantMap = std::map<std::string, QVariant>;

// ============================================================================
// QUuid
// ============================================================================
class QUuid {
public:
    enum QuadrantRemoveFlags { WithBraces, WithoutBraces };
    QUuid() = default;
    static QUuid createUuid() {
        static int counter = 0;
        char buf[37];
        snprintf(buf, sizeof(buf), "{00000000-0000-0000-0000-%012d}", counter++);
        return QUuid(std::string(buf));
    }
    explicit QUuid(const std::string& s) : m_uuid(s) {}
    bool isNull() const { return m_uuid.empty(); }
    std::string toString() const { return m_uuid; }
    std::string toString(QuadrantRemoveFlags) const { return m_uuid; }
    bool operator==(const QUuid& o) const { return m_uuid == o.m_uuid; }
    bool operator!=(const QUuid& o) const { return m_uuid != o.m_uuid; }
    bool operator<(const QUuid& o) const { return m_uuid < o.m_uuid; }
private:
    std::string m_uuid;
};

// ============================================================================
// QImage (minimal stub)
// ============================================================================
class QImage {
public:
    enum Format { Format_Invalid, Format_RGB32, Format_ARGB32, Format_ARGB32_Premultiplied,
                  Format_RGB888, Format_RGBA8888, Format_RGBX8888, Format_Grayscale8 };
    QImage() = default;
    QImage(int w, int h, Format f) : m_width(w), m_height(h), m_format(f) {
        m_data.resize(w * h * 4, 0);
    }
    QImage(const unsigned char* data, int w, int h, Format f) : m_width(w), m_height(h), m_format(f) {
        m_data.assign(data, data + w * h * 4);
    }
    int width() const { return m_width; }
    int height() const { return m_height; }
    Format format() const { return m_format; }
    bool isNull() const { return m_width == 0 || m_height == 0; }
    bool load(const std::string& path) { (void)path; return false; }
    bool save(const std::string& path) const { (void)path; return false; }
    unsigned char* scanLine(int y) { return m_data.data() + y * m_width * 4; }
    const unsigned char* scanLine(int y) const { return m_data.data() + y * m_width * 4; }
    QImage scaled(int w, int h) const { (void)w; (void)h; return *this; }
    QImage copy(int x, int y, int w, int h) const { (void)x; (void)y; (void)w; (void)h; return *this; }
    void fill(uint32_t pixel) { std::fill(m_data.begin(), m_data.end(), (unsigned char)(pixel & 0xFF)); }
    void fill(const QColor& c) { fill(c.rgba()); }
    bool allGray() const { return false; }
    int depth() const { return 32; }
    QByteArray bits() const { return QByteArray(m_data.begin(), m_data.end()); }
    QImage convertedTo(Format f) const { (void)f; return *this; }
    bool hasAlphaChannel() const { return m_format == Format_ARGB32 || m_format == Format_ARGB32_Premultiplied || m_format == Format_RGBA8888; }
    static const char* formatToName(Format f) { (void)f; return "RGBA8"; }
private:
    int m_width = 0, m_height = 0;
    Format m_format = Format_Invalid;
    std::vector<unsigned char> m_data;
};

// Forward declare for QMetaObject
class QObject;

// ============================================================================
// QMetaObject stub
// ============================================================================
class QMetaObject {
public:
    struct SuperData { SuperData* next = nullptr; };
    struct Connection { };
    QMetaObject() = default;
    virtual ~QMetaObject() = default;
    virtual const char* className() const { return ""; }
    int indexOfSignal(const char*) const { return -1; }
    int indexOfSlot(const char*) const { return -1; }
};

// ============================================================================
// QObject stub
// ============================================================================
class QObject {
public:
    QObject(QObject* parent = nullptr) { (void)parent; }
    virtual ~QObject() = default;
    void deleteLater() { delete this; }
    bool connect(const QObject*, const char*, const QObject*, const char*) { return true; }
    template<typename Func1, typename Func2>
    bool connect(const QObject* sender, Func1, const QObject* receiver, Func2) {
        (void)sender; (void)receiver; return true;
    }
    template<typename Func1, typename Func2>
    bool connect(Func1, Func2) { return true; }
    template<typename Func>
    bool connect(Func) { return true; }
    template<typename Func>
    bool connect(void*, Func) { return true; }
    bool disconnect(const QObject*, const char*, const QObject*, const char*) { return true; }
    template<typename Func1, typename Func2>
    bool disconnect(Func1, Func2) { return true; }
    QObject* parent() const { return nullptr; }
    void setParent(QObject*) {}
    const QMetaObject* metaObject() const { return nullptr; }
    void* qt_metacast(const char*) { return nullptr; }
    int qt_metacall(void*, int, void**) { return -1; }
};

inline QObject* qobject_cast(QObject*) { return nullptr; }
template<typename T> T* qobject_cast(QObject*) { return nullptr; }

// ============================================================================
// QTimer stub
// ============================================================================
class QTimer : public QObject {
public:
    QTimer(QObject* parent = nullptr) : QObject(parent) {}
    void start(int msec = 0) { (void)msec; }
    void stop() {}
    bool isActive() const { return false; }
    int interval() const { return 0; }
    void setInterval(int msec) { (void)msec; }
    void setSingleShot(bool) {}
    template<typename Func>
    void connect(Func) {}
    template<typename Func1, typename Func2>
    void connect(Func1, Func2) {}
    static void singleShot(int msec, QObject* context, std::function<void()> slot) {
        (void)msec; (void)context;
        if (slot) slot();
    }
    static void singleShot(int msec, std::function<void()> slot) {
        (void)msec;
        if (slot) slot();
    }
};

// ============================================================================
// QSettings stub
// ============================================================================
class QSettings : public QObject {
public:
    QSettings(QObject* parent = nullptr) : QObject(parent) {}
    QSettings(const std::string& organization, const std::string& application, QObject* parent = nullptr)
        : QObject(parent) { (void)organization; (void)application; }
    QVariant value(const std::string& key) const { (void)key; return QVariant(); }
    QVariant value(const std::string& key, const QVariant& defaultValue) const { (void)key; return defaultValue; }
    void setValue(const std::string& key, const QVariant& value) { (void)key; (void)value; }
    bool contains(const std::string& key) const { (void)key; return false; }
    void remove(const std::string& key) { (void)key; }
    void sync() {}
    std::string fileName() const { return ""; }
};

// ============================================================================
// QElapsedTimer stub
// ============================================================================
class QElapsedTimer {
public:
    void start() { m_start = std::chrono::steady_clock::now(); }
    qint64 elapsed() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - m_start).count();
    }
    qint64 nsecsElapsed() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(now - m_start).count();
    }
    bool hasExpired(qint64 msecs) const { return elapsed() > msecs; }
private:
    std::chrono::steady_clock::time_point m_start = std::chrono::steady_clock::now();
};

// ============================================================================
// QMutex / QMutexLocker stubs
// ============================================================================
class QMutex {
public:
    void lock() { m_mutex.lock(); }
    bool tryLock() { return m_mutex.try_lock(); }
    void unlock() { m_mutex.unlock(); }
private:
    std::mutex m_mutex;
};

class QMutexLocker {
public:
    QMutexLocker(QMutex* m) : m_mutex(m) { if (m_mutex) m_mutex->lock(); }
    QMutexLocker(QMutex& m) : m_mutex(&m) { m_mutex->lock(); }
    ~QMutexLocker() { if (m_mutex) m_mutex->unlock(); }
private:
    QMutex* m_mutex = nullptr;
};

class QReadWriteLock {
public:
    void lockForRead() { m_mutex.lock(); }
    void lockForWrite() { m_mutex.lock(); }
    void unlock() { m_mutex.unlock(); }
    bool tryLockForRead() { return m_mutex.try_lock(); }
    bool tryLockForWrite() { return m_mutex.try_lock(); }
private:
    std::mutex m_mutex;
};

class QReadLocker {
public:
    QReadLocker(QReadWriteLock* l) : m_lock(l) { if (m_lock) m_lock->lockForRead(); }
    ~QReadLocker() { if (m_lock) m_lock->unlock(); }
private:
    QReadWriteLock* m_lock;
};

class QWriteLocker {
public:
    QWriteLocker(QReadWriteLock* l) : m_lock(l) { if (m_lock) m_lock->lockForWrite(); }
    ~QWriteLocker() { if (m_lock) m_lock->unlock(); }
private:
    QReadWriteLock* m_lock;
};

// ============================================================================
// QScopedPointer / QSharedPointer stubs
// ============================================================================
template<typename T>
class QScopedPointer {
public:
    QScopedPointer(T* p = nullptr) : m_ptr(p) {}
    ~QScopedPointer() { delete m_ptr; }
    T* operator->() const { return m_ptr; }
    T& operator*() const { return *m_ptr; }
    T* get() const { return m_ptr; }
    T* take() { T* p = m_ptr; m_ptr = nullptr; return p; }
    void reset(T* p = nullptr) { delete m_ptr; m_ptr = p; }
    bool isNull() const { return m_ptr == nullptr; }
    operator bool() const { return m_ptr != nullptr; }
private:
    T* m_ptr = nullptr;
};

template<typename T>
class QSharedPointer {
public:
    QSharedPointer() = default;
    QSharedPointer(T* p) : m_ptr(p) {}
    T* operator->() const { return m_ptr.get(); }
    T& operator*() const { return *m_ptr; }
    T* get() const { return m_ptr.get(); }
    bool isNull() const { return m_ptr == nullptr; }
    operator bool() const { return m_ptr != nullptr; }
    int refCount() const { return 1; }
private:
    std::shared_ptr<T> m_ptr;
};

// ============================================================================
// QPoint / QPointF / QRect / QRectF / QSize
// ============================================================================
struct QPoint {
    int xp = 0, yp = 0;
    QPoint() = default;
    QPoint(int x, int y) : xp(x), yp(y) {}
    int x() const { return xp; }
    int y() const { return yp; }
    void setX(int x) { xp = x; }
    void setY(int y) { yp = y; }
    bool operator==(const QPoint& o) const { return xp==o.xp && yp==o.yp; }
};

struct QPointF {
    float xp = 0, yp = 0;
    QPointF() = default;
    QPointF(float x, float y) : xp(x), yp(y) {}
    float x() const { return xp; }
    float y() const { return yp; }
};

struct QRect {
    int xp=0, yp=0, w=0, h=0;
    QRect() = default;
    QRect(int x, int y, int w, int h) : xp(x), yp(y), w(w), h(h) {}
    int x() const { return xp; }
    int y() const { return yp; }
    int width() const { return w; }
    int height() const { return h; }
    int left() const { return xp; }
    int top() const { return yp; }
    int right() const { return xp + w; }
    int bottom() const { return yp + h; }
    bool contains(int x, int y) const { return x >= xp && x < xp+w && y >= yp && y < yp+h; }
    bool isEmpty() const { return w <= 0 || h <= 0; }
    bool isValid() const { return w > 0 && h > 0; }
    QRect adjusted(int dx1, int dy1, int dx2, int dy2) const { return {xp+dx1, yp+dy1, w+dx2-dx1, h+dy2-dy1}; }
};

struct QRectF {
    float xp=0, yp=0, w=0, h=0;
    QRectF() = default;
    QRectF(float x, float y, float w, float h) : xp(x), yp(y), w(w), h(h) {}
    float x() const { return xp; }
    float y() const { return yp; }
    float width() const { return w; }
    float height() const { return h; }
    float left() const { return xp; }
    float top() const { return yp; }
    float right() const { return xp + w; }
    float bottom() const { return yp + h; }
    bool contains(float x, float y) const { return x >= xp && x < xp+w && y >= yp && y < yp+h; }
    QRect toRect() const { return QRect(int(xp), int(yp), int(w), int(h)); }
};

struct QSize {
    int wp=0, hp=0;
    QSize() = default;
    QSize(int w, int h) : wp(w), hp(h) {}
    int width() const { return wp; }
    int height() const { return hp; }
    void setWidth(int w) { wp = w; }
    void setHeight(int h) { hp = h; }
    bool isEmpty() const { return wp <= 0 || hp <= 0; }
    bool isValid() const { return wp > 0 && hp > 0; }
    QSize scaled(int w, int h) const { (void)w; (void)h; return *this; }
};

struct QSizeF {
    float wp=0, hp=0;
    QSizeF() = default;
    QSizeF(float w, float h) : wp(w), hp(h) {}
    float width() const { return wp; }
    float height() const { return hp; }
};

// ============================================================================
// QString-like utility functions
// ============================================================================
namespace QStringCompat {
    inline std::string arg(const std::string& s, int fieldWidth, char fillChar = ' ') {
        (void)fieldWidth; (void)fillChar; return s;
    }
    inline std::string toLower(const std::string& s) {
        std::string r = s;
        std::transform(r.begin(), r.end(), r.begin(), ::tolower);
        return r;
    }
    inline std::string toUpper(const std::string& s) {
        std::string r = s;
        std::transform(r.begin(), r.end(), r.begin(), ::toupper);
        return r;
    }
    inline bool contains(const std::string& s, const std::string& sub) { return s.find(sub) != std::string::npos; }
    inline bool startsWith(const std::string& s, const std::string& prefix) { return s.substr(0, prefix.size()) == prefix; }
    inline bool endsWith(const std::string& s, const std::string& suffix) {
        return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
    }
    inline std::string trimmed(const std::string& s) {
        auto start = s.find_first_not_of(" \t\r\n");
        auto end = s.find_last_not_of(" \t\r\n");
        return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
    }
    inline std::string left(const std::string& s, int n) { return s.substr(0, n); }
    inline std::string right(const std::string& s, int n) { return s.size() >= (size_t)n ? s.substr(s.size()-n) : s; }
    inline std::string mid(const std::string& s, int pos, int n = -1) {
        return n < 0 ? s.substr(pos) : s.substr(pos, n);
    }
    inline int indexOf(const std::string& s, const std::string& sub) {
        auto pos = s.find(sub);
        return pos == std::string::npos ? -1 : (int)pos;
    }
    inline int lastIndexOf(const std::string& s, const std::string& sub) {
        auto pos = s.rfind(sub);
        return pos == std::string::npos ? -1 : (int)pos;
    }
    inline std::string replace(const std::string& s, const std::string& from, const std::string& to) {
        std::string r = s;
        size_t pos = 0;
        while ((pos = r.find(from, pos)) != std::string::npos) {
            r.replace(pos, from.length(), to);
            pos += to.length();
        }
        return r;
    }
    inline std::vector<std::string> split(const std::string& s, const std::string& delimiter) {
        std::vector<std::string> tokens;
        size_t start = 0;
        size_t end = s.find(delimiter);
        while (end != std::string::npos) {
            tokens.push_back(s.substr(start, end - start));
            start = end + delimiter.length();
            end = s.find(delimiter, start);
        }
        tokens.push_back(s.substr(start));
        return tokens;
    }
    inline std::string joined(const std::vector<std::string>& parts, const std::string& sep) {
        std::string result;
        for (size_t i = 0; i < parts.size(); i++) {
            if (i > 0) result += sep;
            result += parts[i];
        }
        return result;
    }
    inline std::string number(double n, char format = 'f', int precision = -1) {
        std::ostringstream oss;
        if (precision >= 0) oss << std::fixed << std::setprecision(precision);
        else oss << std::defaultfloat;
        oss << n;
        return oss.str();
    }
    inline std::string number(float n, char format = 'f', int precision = -1) { return number(double(n), format, precision); }
    inline bool isEmpty(const std::string& s) { return s.empty(); }
    inline int length(const std::string& s) { return (int)s.size(); }
    inline std::string simplified(const std::string& s) {
        std::string result;
        bool inSpace = false;
        for (char c : s) {
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                if (!inSpace) { result += ' '; inSpace = true; }
            } else { result += c; inSpace = false; }
        }
        while (!result.empty() && result.back() == ' ') result.pop_back();
        return result;
    }
}

// operators for QString-like usage on std::string
inline bool operator==(const std::string& a, const char* b) { return a == std::string(b ? b : ""); }
inline bool operator==(const char* a, const std::string& b) { return std::string(a ? a : "") == b; }
inline bool operator!=(const std::string& a, const char* b) { return !(a == b); }
inline bool operator!=(const char* a, const std::string& b) { return !(a == b); }
inline bool operator<(const std::string& a, const char* b) { return a < std::string(b ? b : ""); }

// ============================================================================
// String literal operator for QStringLiteral
// ============================================================================
inline std::string QStringLiteral(const char* s) { return s ? std::string(s) : std::string(); }

// ============================================================================
// QDebug stub
// ============================================================================
class QDebugStream {
public:
    template<typename T> QDebugStream& operator<<(const T& t) {
        std::cerr << t;
        return *this;
    }
    ~QDebugStream() { std::cerr << std::endl; }
};

inline QDebugStream qDebug() { return QDebugStream(); }
inline QDebugStream qWarning() { return QDebugStream(); }
inline QDebugStream qInfo() { return QDebugStream(); }
inline QDebugStream qCritical() { return QDebugStream(); }
inline void qFatal(const char* msg) { fprintf(stderr, "FATAL: %s\n", msg); abort(); }
inline std::string qPrintable(const std::string& s) { return s; }
inline std::string qPrintable(const char* s) { return s ? std::string(s) : std::string(); }

// ============================================================================
// QJson stubs (minimal)
// ============================================================================
struct QJsonParseError {
    enum ParseError { NoError = 0 };
    ParseError error = NoError;
    int offset = 0;
    QString errorString() const { return ""; }
};

class QJsonValue {
public:
    QJsonValue() = default;
    QJsonValue(const std::string& s) : m_string(s) {}
    QJsonValue(double v) : m_double(v) {}
    QJsonValue(int v) : m_int(v) {}
    QJsonValue(bool v) : m_bool(v) {}
    bool isString() const { return true; }
    bool isDouble() const { return true; }
    bool isBool() const { return true; }
    bool isObject() const { return false; }
    bool isArray() const { return false; }
    bool isNull() const { return false; }
    std::string toString() const { return m_string; }
    double toDouble(double def = 0) const { return m_double != 0 ? m_double : def; }
    int toInt(int def = 0) const { return m_int != 0 ? m_int : def; }
    bool toBool(bool def = false) const { return m_bool; }
    static QJsonValue fromVariant(const QVariant& v) { (void)v; return QJsonValue(); }
private:
    std::string m_string;
    double m_double = 0;
    int m_int = 0;
    bool m_bool = false;
};

class QJsonArray {
public:
    QJsonArray() = default;
    void append(const QJsonValue& v) { m_values.push_back(v); }
    int size() const { return (int)m_values.size(); }
    QJsonValue operator[](int i) const { return m_values[i]; }
    QJsonValue at(int i) const { return m_values[i]; }
    static QJsonArray fromStringList(const std::vector<std::string>& list) { (void)list; return QJsonArray(); }
    static QJsonArray fromVariantList(const QVariantList& list) { (void)list; return QJsonArray(); }
private:
    std::vector<QJsonValue> m_values;
};

class QJsonObject {
public:
    QJsonObject() = default;
    void insert(const std::string& key, const QJsonValue& value) { m_values[key] = value; }
    QJsonValue value(const std::string& key) const {
        auto it = m_values.find(key);
        return it != m_values.end() ? it->second : QJsonValue();
    }
    QJsonValue operator[](const std::string& key) const { return value(key); }
    bool contains(const std::string& key) const { return m_values.count(key) > 0; }
    bool isEmpty() const { return m_values.empty(); }
    QStringList keys() const { QStringList r; for (auto& kv : m_values) r.push_back(kv.first); return r; }
    static QJsonObject fromVariantMap(const QVariantMap& map) { (void)map; return QJsonObject(); }
    bool remove(const std::string& key) { return m_values.erase(key) > 0; }
    class const_iterator {
    public:
        const_iterator(std::map<std::string, QJsonValue>::const_iterator it) : m_it(it) {}
        bool operator!=(const const_iterator& o) const { return m_it != o.m_it; }
        const_iterator& operator++() { ++m_it; return *this; }
        std::string key() const { return m_it->first; }
        QJsonValue value() const { return m_it->second; }
    private:
        std::map<std::string, QJsonValue>::const_iterator m_it;
    };
    const_iterator constBegin() const { return const_iterator(m_values.begin()); }
    const_iterator constEnd() const { return const_iterator(m_values.end()); }
private:
    std::map<std::string, QJsonValue> m_values;
};

class QJsonDocument {
public:
    QJsonDocument() = default;
    QJsonDocument(const QJsonObject& obj) : m_object(obj) {}
    QJsonObject object() const { return m_object; }
    QByteArray toJson(int indent = -1) const { (void)indent; return QByteArray(); }
    enum JsonFormat { Compact, Indented };
    QByteArray toJson(JsonFormat f) const { (void)f; return QByteArray(); }
    bool isEmpty() const { return m_object.isEmpty(); }
    static QJsonDocument fromJson(const QByteArray& json, QJsonParseError* error = nullptr) {
        (void)json;
        if (error) error->error = QJsonParseError::NoError;
        return QJsonDocument();
    }
    static QJsonDocument fromVariant(const QVariant& v) { (void)v; return QJsonDocument(); }
private:
    QJsonObject m_object;
};

// ============================================================================
// QUrl stub
// ============================================================================
class QUrl {
public:
    QUrl() = default;
    QUrl(const std::string& s) : m_url(s) {}
    std::string toString() const { return m_url; }
    std::string path() const { return m_url; }
    std::string toLocalFile() const { return m_url; }
    QByteArray toPercentEncoding() const { return QByteArray(m_url.begin(), m_url.end()); }
    bool isValid() const { return !m_url.empty(); }
    bool isLocalFile() const { return true; }
    void setPath(const std::string& p) { m_url = p; }
private:
    std::string m_url;
};

// ============================================================================
// QFlags stub
// ============================================================================
template<typename T>
class QFlags {
public:
    QFlags() : m_value(0) {}
    QFlags(T v) : m_value(static_cast<int>(v)) {}
    QFlags(int v) : m_value(v) {}
    operator int() const { return m_value; }
    QFlags operator|(QFlags o) const { return QFlags(m_value | o.m_value); }
    QFlags operator&(int mask) const { return QFlags(m_value & mask); }
    bool testFlag(T f) const { return (m_value & static_cast<int>(f)) != 0; }
private:
    int m_value;
};

// ============================================================================
// Standalone functions that need to be available
// ============================================================================
template<typename Iter>
inline void qDeleteAll(Iter begin, Iter end) {
    for (auto it = begin; it != end; ++it) delete *it;
}

// ============================================================================
// QCoreApplication stub
// ============================================================================
class QCoreApplication : public QObject {
public:
    static std::string applicationDirPath() { return "."; }
    static std::string applicationFilePath() { return ""; }
    static std::string applicationName() { return ""; }
    static void setApplicationName(const std::string&) {}
    static std::string organizationName() { return ""; }
    static void setOrganizationName(const std::string&) {}
    static int argc() { return 0; }
    static char** argv() { return nullptr; }
};

// ============================================================================
// QLibrary stub (dynamic library loading)
// ============================================================================
class QLibrary {
public:
    QLibrary() = default;
    QLibrary(const std::string& fileName) : m_fileName(fileName) {}
    bool load() {
#ifdef _WIN32
        m_handle = LoadLibraryA(m_fileName.c_str());
        return m_handle != nullptr;
#else
        return false;
#endif
    }
    void unload() {
#ifdef _WIN32
        if (m_handle) { FreeLibrary((HMODULE)m_handle); m_handle = nullptr; }
#endif
    }
    void* resolve(const char* symbolName) {
#ifdef _WIN32
        if (!m_handle) return nullptr;
        return (void*)GetProcAddress((HMODULE)m_handle, symbolName);
#else
        return nullptr;
#endif
    }
    void setFileName(const std::string& name) { m_fileName = name; }
    std::string fileName() const { return m_fileName; }
    bool isLoaded() const {
#ifdef _WIN32
        return m_handle != nullptr;
#else
        return false;
#endif
    }
private:
    std::string m_fileName;
    void* m_handle = nullptr;
};

// ============================================================================
// QFile stub
// ============================================================================
class QFile {
public:
    enum OpenMode { NotOpen = 0x0001, ReadOnly = 0x0004, WriteOnly = 0x0008, ReadWrite = 0x000a, Append = 0x0010, Truncate = 0x0400, Text = 0x0010, Unbuffered = 0x0020 };
    QFile() = default;
    QFile(const std::string& name) : m_fileName(name) {}
    QFile(const std::string& name, QObject*) : m_fileName(name) {}
    bool open(int mode = ReadOnly) {
        (void)mode;
#ifdef _WIN32
        fopen_s(&m_file, m_fileName.c_str(), "rb");
#else
        m_file = fopen(m_fileName.c_str(), "rb");
#endif
        return m_file != nullptr;
    }
    void close() { if (m_file) { fclose(m_file); m_file = nullptr; } }
    bool isOpen() const { return m_file != nullptr; }
    bool exists() const {
        std::ifstream f(m_fileName);
        return f.good();
    }
    std::string fileName() const { return m_fileName; }
    void setFileName(const std::string& name) { m_fileName = name; }
    QByteArray read(qint64 maxSize = -1) {
        if (!m_file) return {};
        if (maxSize < 0) {
            fseek(m_file, 0, SEEK_END);
            long sz = ftell(m_file);
            fseek(m_file, 0, SEEK_SET);
            QByteArray result(sz, 0);
            fread(result.data(), 1, sz, m_file);
            return result;
        }
        QByteArray result(maxSize, 0);
        size_t n = fread(result.data(), 1, maxSize, m_file);
        result.resize(n);
        return result;
    }
    QByteArray readAll() { return read(-1); }
    QByteArray readLine(qint64 maxSize = 0) {
        if (!m_file) return {};
        char buf[4096];
        if (fgets(buf, sizeof(buf), m_file)) {
            return QByteArray(buf, buf + strlen(buf));
        }
        return {};
    }
    bool canReadLine() const { return false; }
    qint64 write(const QByteArray& data) {
        if (!m_file) return -1;
        return (qint64)fwrite(data.data(), 1, data.size(), m_file);
    }
    qint64 write(const char* data, qint64 size) {
        if (!m_file) return -1;
        return (qint64)fwrite(data, 1, size, m_file);
    }
    qint64 size() const {
        if (!m_file) return 0;
        long pos = ftell(m_file);
        fseek(m_file, 0, SEEK_END);
        long sz = ftell(m_file);
        fseek(m_file, pos, SEEK_SET);
        return sz;
    }
    qint64 pos() const { return m_file ? ftell(m_file) : 0; }
    bool seek(qint64 pos) { return m_file && fseek(m_file, (long)pos, SEEK_SET) == 0; }
    static bool exists(const std::string& name) {
        std::ifstream f(name);
        return f.good();
    }
    static bool copy(const std::string& src, const std::string& dst) {
        std::ifstream srcFile(src, std::ios::binary);
        if (!srcFile) return false;
        std::ofstream dstFile(dst, std::ios::binary);
        if (!dstFile) return false;
        dstFile << srcFile.rdbuf();
        return dstFile.good();
    }
    static bool remove(const std::string& name) {
        return std::remove(name.c_str()) == 0;
    }
    void setObjectName(const std::string&) {}
private:
    std::string m_fileName;
    FILE* m_file = nullptr;
};

// ============================================================================
// QDir stub
// ============================================================================
class QDir {
public:
    enum Filters { Dirs = 0x001, Files = 0x004, NoDot = 0x0200, NoDotDot = 0x0400, NoDotAndDotDot = 0x0600 };
    enum SortFlags { };
    QDir() = default;
    QDir(const std::string& path) : m_path(path) {}
    bool exists() const {
#ifdef _WIN32
        DWORD attr = GetFileAttributesA(m_path.c_str());
        return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
#else
        struct stat st;
        return stat(m_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
#endif
    }
    bool exists(const std::string& name) const {
        std::string fullPath = m_path + "/" + name;
        std::ifstream f(fullPath);
        return f.good();
    }
    bool mkpath(const std::string& path) const {
#ifdef _WIN32
        return CreateDirectoryA(path.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS;
#else
        return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
#endif
    }
    bool mkpath() const { return mkpath(m_path); }
    std::string path() const { return m_path; }
    std::string absolutePath() const { return m_path; }
    std::string canonicalPath() const { return m_path; }
    std::string dirName() const {
        auto pos = m_path.find_last_of("/\\");
        return pos != std::string::npos ? m_path.substr(pos + 1) : m_path;
    }
    std::string filePath(const std::string& name) const { return m_path + "/" + name; }
    std::string absoluteFilePath(const std::string& name) const { return m_path + "/" + name; }
    std::string toNativeSeparators(const std::string& path) { return path; }
    std::string fromNativeSeparators(const std::string& path) { return path; }
    QStringList entryList(int filters = Dirs | Files, int sort = 0) const {
        (void)filters; (void)sort; return {};
    }
    QStringList entryList(const QStringList& nameFilters, int filters = Dirs | Files, int sort = 0) const {
        (void)nameFilters; (void)filters; (void)sort; return {};
    }
    bool cdUp() { return false; }
    bool cd(const std::string& dir) { m_path += "/" + dir; return true; }
    void setPath(const std::string& path) { m_path = path; }
    static std::string cleanPath(const std::string& path) { return path; }
    static std::string homePath() {
#ifdef _WIN32
        char buf[MAX_PATH];
        GetEnvironmentVariableA("USERPROFILE", buf, MAX_PATH);
        return buf;
#else
        const char* home = getenv("HOME");
        return home ? home : "/";
#endif
    }
    static std::string currentPath() {
#ifdef _WIN32
        char buf[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, buf);
        return buf;
#else
        char buf[1024];
        getcwd(buf, sizeof(buf));
        return buf;
#endif
    }
    static std::string tempPath() {
#ifdef _WIN32
        char buf[MAX_PATH];
        GetTempPathA(MAX_PATH, buf);
        return buf;
#else
        return "/tmp";
#endif
    }
    bool removeRecursively() { return false; }
    bool remove(const std::string&) { return false; }
    bool rename(const std::string&, const std::string&) { return false; }
    void setNameFilters(const QStringList&) {}
private:
    std::string m_path;
};

// ============================================================================
// QFileInfo stub
// ============================================================================
class QFileInfo {
public:
    QFileInfo() = default;
    QFileInfo(const std::string& path) : m_filePath(path) {}
    QFileInfo(const QDir& dir, const std::string& name) : m_filePath(dir.path() + "/" + name) {}
    bool exists() const { return QFile::exists(m_filePath); }
    std::string filePath() const { return m_filePath; }
    std::string absoluteFilePath() const { return m_filePath; }
    std::string fileName() const {
        auto pos = m_filePath.find_last_of("/\\");
        return pos != std::string::npos ? m_filePath.substr(pos + 1) : m_filePath;
    }
    std::string baseName() const {
        std::string fn = fileName();
        auto pos = fn.find_last_of('.');
        return pos != std::string::npos ? fn.substr(0, pos) : fn;
    }
    std::string completeBaseName() const { return baseName(); }
    std::string suffix() const {
        std::string fn = fileName();
        auto pos = fn.find_last_of('.');
        return pos != std::string::npos ? fn.substr(pos + 1) : "";
    }
    std::string completeSuffix() const { return suffix(); }
    std::string path() const {
        auto pos = m_filePath.find_last_of("/\\");
        return pos != std::string::npos ? m_filePath.substr(0, pos) : ".";
    }
    bool isDir() const { return false; }
    bool isFile() const { return true; }
    bool isExecutable() const { return false; }
    bool isReadable() const { return true; }
    bool isWritable() const { return true; }
    bool isHidden() const { return false; }
    qint64 size() const { return 0; }
    std::string owner() const { return ""; }
    std::string group() const { return ""; }
    void refresh() {}
    QDir dir() const { return QDir(path()); }
    QDir absoluteDir() const { return QDir(path()); }
    bool symmetryLinkTarget() const { return false; }
private:
    std::string m_filePath;
};

// ============================================================================
// QFileSystemWatcher stub
// ============================================================================
class QFileSystemWatcher : public QObject {
public:
    QFileSystemWatcher(QObject* parent = nullptr) : QObject(parent) {}
    QFileSystemWatcher(const std::string& path, QObject* parent = nullptr) : QObject(parent) { addPath(path); }
    QFileSystemWatcher(const QStringList& paths, QObject* parent = nullptr) : QObject(parent) { for (auto& p : paths) addPath(p); }
    void addPath(const std::string&) {}
    void addPaths(const QStringList&) {}
    void removePath(const std::string&) {}
    void removePaths(const QStringList&) {}
    QStringList directories() const { return {}; }
    QStringList files() const { return {}; }
signals:
    void directoryChanged(const std::string& path);
    void fileChanged(const std::string& path);
};

// ============================================================================
// QRegularExpression stub
// ============================================================================
class QRegularExpressionMatch {
public:
    QRegularExpressionMatch() = default;
    bool hasMatch() const { return false; }
    bool hasCaptured(int i = 0) const { (void)i; return false; }
    std::string captured(int i = 0) const { (void)i; return ""; }
    std::string captured(const std::string&) const { return ""; }
    int capturedStart(int i = 0) const { (void)i; return -1; }
    int capturedLength(int i = 0) const { (void)i; return 0; }
    QStringList capturedTexts() const { return {}; }
private:
};

class QRegularExpression {
public:
    enum PatternOption { NoPatternOption = 0, CaseInsensitiveOption = 0x01, MultilineOption = 0x02, DotMatchesEverythingOption = 0x04 };
    enum MatchType { NormalMatch = 0, PartialMatch = 1 };
    enum MatchOption { NoMatchOption = 0, AnchoredMatchOption = 0x01 };
    QRegularExpression() = default;
    QRegularExpression(const std::string& pattern, int options = NoPatternOption) : m_pattern(pattern) { (void)options; }
    QRegularExpression(const QRegularExpression& other) = default;
    QRegularExpression& operator=(const QRegularExpression& other) = default;
    bool isValid() const { return true; }
    bool isEmpty() const { return m_pattern.empty(); }
    std::string pattern() const { return m_pattern; }
    void setPattern(const std::string& pattern) { m_pattern = pattern; }
    void setPatternOptions(int) {}
    int patternOptions() const { return 0; }
    QRegularExpressionMatch match(const std::string& subject, int offset = 0, MatchType type = NormalMatch, int options = NoMatchOption) const {
        (void)subject; (void)offset; (void)type; (void)options;
        return QRegularExpressionMatch();
    }
    QRegularExpressionMatch match(const std::string& subject, int offset, int options) const {
        return match(subject, offset, NormalMatch, options);
    }
    QStringList captureTexts() const { return {}; }
    bool globalMatch() const { return false; }
    std::string errorString() const { return ""; }
    static std::string escape(const std::string& str) { return str; }
    QRegularExpression& operator=(const QRegularExpression&& other) { m_pattern = other.m_pattern; return *this; }
private:
    std::string m_pattern;
};

// ============================================================================
// QProcess stub
// ============================================================================
class QProcess : public QObject {
public:
    enum ProcessState { NotRunning, Starting, Running };
    enum ProcessError { FailedToStart, Crashed, Timedout, WriteError, ReadError, UnknownError };
    enum ExitStatus { NormalExit, CrashExit };
    QProcess(QObject* parent = nullptr) : QObject(parent) {}
    void start(const std::string& program, const QStringList& arguments = {}) {
        (void)program; (void)arguments;
    }
    void start(const std::string& command) { (void)command; }
    void waitForFinished(int msecs = 30000) { (void)msecs; }
    bool waitForStarted(int msecs = 30000) { (void)msecs; return false; }
    bool waitForReadyRead(int msecs = 30000) { (void)msecs; return false; }
    QByteArray readAll() { return {}; }
    QByteArray readAllStandardOutput() { return {}; }
    QByteArray readAllStandardError() { return {}; }
    QByteArray readLineStandardOutput() { return {}; }
    QByteArray readLineStandardError() { return {}; }
    QByteArray readLine() { return {}; }
    void write(const QByteArray& data) { (void)data; }
    void write(const char* data, qint64 size = -1) { (void)data; (void)size; }
    void closeWriteChannel() {}
    void closeReadChannel(int) {}
    ProcessState state() const { return NotRunning; }
    int exitCode() const { return 0; }
    bool canReadLine() const { return false; }
    bool atEnd() const { return true; }
    void kill() {}
    void terminate() {}
    void setWorkingDirectory(const std::string& dir) { (void)dir; }
    std::string workingDirectory() const { return ""; }
    void setProcessChannelMode(int) {}
    void setReadChannel(int) {}
    bool isOpen() const { return false; }
    void startRead() {}
    void close() {}
    bool isOpenForRead() const { return false; }
    bool isOpenForWrite() const { return false; }
    qint64 bytesToWrite() const { return 0; }
    void setEnvironment(const QStringList&) {}
    static QStringList systemEnvironment() { return {}; }
signals:
    void finished(int exitCode, ExitStatus exitStatus);
    void readyReadStandardOutput();
};

// ============================================================================
// QMimeDatabase / QMimeType stubs
// ============================================================================
class QMimeType {
public:
    QMimeType() = default;
    std::string name() const { return "application/octet-stream"; }
    std::string comment() const { return ""; }
    std::string globPatterns() const { return ""; }
    bool isValid() const { return false; }
};

class QMimeDatabase {
public:
    QMimeType mimeTypeForFile(const std::string& fileName) const { (void)fileName; return QMimeType(); }
    QMimeType mimeTypeForFile(const QFileInfo& fileInfo) const { (void)fileInfo; return QMimeType(); }
    QMimeType mimeTypeForName(const std::string& mimeType) const { (void)mimeType; return QMimeType(); }
    QMimeType mimeTypeForData(const QByteArray& data) const { (void)data; return QMimeType(); }
    QMimeType mimeTypeForUrl(const QUrl& url) const { (void)url; return QMimeType(); }
    std::string mimeTypeForFile(const std::string& fileName, int mode) const { (void)fileName; (void)mode; return ""; }
};

// ============================================================================
// QRunnable / QThreadPool stubs
// ============================================================================
class QRunnable {
public:
    virtual ~QRunnable() = default;
    virtual void run() = 0;
    void setAutoDelete(bool autoDelete) { m_autoDelete = autoDelete; }
    bool autoDelete() const { return m_autoDelete; }
private:
    bool m_autoDelete = true;
};

class QThreadPool {
public:
    static QThreadPool* globalInstance() { static QThreadPool pool; return &pool; }
    void start(QRunnable* task, int priority = 0) { (void)task; (void)priority; }
    void waitForDone(int msecs = -1) { (void)msecs; }
    int activeThreadCount() const { return 0; }
    int maxThreadCount() const { return 1; }
    void setMaxThreadCount(int n) { (void)n; }
    int threadCount() const { return 0; }
};

// ============================================================================
// QTextStream stub
// ============================================================================
// Forward-declare QIODevice first
class QIODevice;

// ============================================================================
// QTextStream stub
// ============================================================================
class QTextStream {
public:
    QTextStream() = default;
    QTextStream(QIODevice* device) { (void)device; }
    QTextStream(FILE* file) { (void)file; }
    QTextStream(QByteArray* array) { (void)array; }
    QTextStream(const QByteArray& array) { (void)array; }
    template<typename T> QTextStream& operator<<(const T& t) { std::cout << t; return *this; }
    template<typename T> QTextStream& operator>>(T& t) { (void)t; return *this; }
    void setCodec(const char*) {}
    void setDevice(QIODevice*) {}
    void flush() { std::cout.flush(); }
    bool atEnd() const { return true; }
    void skipWhiteSpace() {}
    std::string readLine(qint64 maxlen = 0) { (void)maxlen; return ""; }
    std::string readAll() { return ""; }
    std::string read(qint64 maxlen) { (void)maxlen; return ""; }
    qint64 pos() const { return 0; }
    bool seek(qint64) { return false; }
    void setFieldWidth(int) {}
    void setFieldAlignment(int) {}
    void setRealNumberPrecision(int) {}
    void setRealNumberNotation(int) {}
    void setIntegerBase(int) {}
private:
};

inline QTextStream& operator<<(QTextStream& s, const std::string& str) { std::cout << str; return s; }

// ============================================================================
// QStandardPaths stub
// ============================================================================
class QStandardPaths {
public:
    enum StandardLocation { DesktopLocation, DocumentsLocation, PicturesLocation, MusicLocation,
        MoviesLocation, DownloadLocation, ApplicationsLocation, GenericDataLocation,
        AppDataLocation, ConfigLocation, CacheLocation, GenericCacheLocation, RuntimeLocation,
        HomeLocation, TempLocation, StateLocation, AppLocalDataLocation, AppConfigLocation };
    static std::string writableLocation(int type) { (void)type; return "."; }
    static std::string locate(int type, const std::string& fileName, int options = 0) { (void)type; (void)fileName; (void)options; return ""; }
    static QStringList locateAll(int type, const std::string& fileName, int options = 0) { (void)type; (void)fileName; (void)options; return {}; }
    static std::string displayName(int type) { (void)type; return ""; }
    static std::string findExecutable(const std::string& executableName) { (void)executableName; return ""; }
};

// ============================================================================
// QFileDevice stub
// ============================================================================
class QFileDevice {
public:
    virtual ~QFileDevice() = default;
    virtual bool open(int mode) { (void)mode; return false; }
    virtual void close() {}
    virtual bool isOpen() const { return false; }
    virtual QByteArray read(qint64 maxSize = -1) { (void)maxSize; return {}; }
    virtual QByteArray readAll() { return {}; }
    virtual QByteArray readLine(qint64 maxSize = 0) { (void)maxSize; return {}; }
    virtual qint64 write(const QByteArray& data) { (void)data; return -1; }
    virtual qint64 write(const char* data, qint64 size) { (void)data; (void)size; return -1; }
    virtual qint64 size() const { return 0; }
    virtual qint64 pos() const { return 0; }
    virtual bool seek(qint64 pos) { (void)pos; return false; }
    virtual bool atEnd() const { return true; }
    virtual bool canReadLine() const { return false; }
    virtual void flush() {}
    virtual void setFileName(const std::string&) {}
    virtual std::string fileName() const { return ""; }
    virtual qint64 bytesAvailable() const { return 0; }
    virtual qint64 bytesToWrite() const { return 0; }
};

// ============================================================================
// QIODevice stub
// ============================================================================
class QIODevice {
public:
    virtual ~QIODevice() = default;
    virtual bool open(int mode) { (void)mode; return false; }
    virtual void close() {}
    virtual bool isOpen() const { return false; }
    virtual QByteArray read(qint64 maxSize = -1) { (void)maxSize; return {}; }
    virtual QByteArray readAll() { return {}; }
    virtual QByteArray readLine(qint64 maxSize = 0) { (void)maxSize; return {}; }
    virtual qint64 write(const QByteArray& data) { (void)data; return -1; }
    virtual qint64 write(const char* data, qint64 size) { (void)data; (void)size; return -1; }
    virtual qint64 size() const { return 0; }
    virtual qint64 pos() const { return 0; }
    virtual bool seek(qint64 pos) { (void)pos; return false; }
    virtual bool atEnd() const { return true; }
    virtual bool canReadLine() const { return false; }
    virtual void flush() {}
    virtual qint64 bytesAvailable() const { return 0; }
    virtual qint64 bytesToWrite() const { return 0; }
    void setTextModeEnabled(bool) {}
    bool isTextModeEnabled() const { return false; }
};

// ============================================================================
// QBuffer stub
// ============================================================================
class QBuffer : public QIODevice {
public:
    QBuffer(QByteArray* buf = nullptr, QObject* parent = nullptr) : QIODevice() { (void)buf; (void)parent; }
    QByteArray& buffer() { return m_buffer; }
    const QByteArray& buffer() const { return m_buffer; }
    void setBuffer(QByteArray* buf) { (void)buf; }
    QByteArray data() const { return m_buffer; }
    void setData(const QByteArray& data) { m_buffer = data; }
    void setData(const char* data, int size) { m_buffer.assign(data, data + size); }
private:
    QByteArray m_buffer;
};

// ============================================================================
// QDataStream stub
// ============================================================================
class QDataStream {
public:
    enum ByteOrder { BigEndian, LittleEndian };
    enum Version { Qt_1_0 = 1, Qt_2_0 = 2, Qt_3_0 = 3, Qt_4_0 = 4, Qt_4_6 = 6, Qt_4_9 = 9, Qt_5_0 = 12 };
    QDataStream() = default;
    QDataStream(QIODevice* device) { (void)device; }
    QDataStream(QByteArray* array, int mode = 0) { (void)array; (void)mode; }
    QDataStream(const QByteArray& array) { (void)array; }
    QDataStream& operator<<(quint8 v) { (void)v; return *this; }
    QDataStream& operator<<(qint8 v) { (void)v; return *this; }
    QDataStream& operator<<(quint16 v) { (void)v; return *this; }
    QDataStream& operator<<(qint16 v) { (void)v; return *this; }
    QDataStream& operator<<(quint32 v) { (void)v; return *this; }
    QDataStream& operator<<(qint32 v) { (void)v; return *this; }
    QDataStream& operator<<(quint64 v) { (void)v; return *this; }
    QDataStream& operator<<(qint64 v) { (void)v; return *this; }
    QDataStream& operator<<(float v) { (void)v; return *this; }
    QDataStream& operator<<(double v) { (void)v; return *this; }
    QDataStream& operator<<(const std::string& v) { (void)v; return *this; }
    QDataStream& operator>>(quint8& v) { v = 0; return *this; }
    QDataStream& operator>>(qint8& v) { v = 0; return *this; }
    QDataStream& operator>>(quint16& v) { v = 0; return *this; }
    QDataStream& operator>>(qint16& v) { v = 0; return *this; }
    QDataStream& operator>>(quint32& v) { v = 0; return *this; }
    QDataStream& operator>>(qint32& v) { v = 0; return *this; }
    QDataStream& operator>>(quint64& v) { v = 0; return *this; }
    QDataStream& operator>>(qint64& v) { v = 0; return *this; }
    QDataStream& operator>>(float& v) { v = 0; return *this; }
    QDataStream& operator>>(double& v) { v = 0; return *this; }
    QDataStream& operator>>(std::string& v) { v = ""; return *this; }
    void setVersion(int v) { (void)v; }
    int version() const { return Qt_5_0; }
    void setByteOrder(int) {}
    void setDevice(QIODevice*) {}
    void flush() {}
    bool atEnd() const { return true; }
    qint64 skipRawData(int len) { (void)len; return 0; }
    qint64 writeRawData(const char* data, int len) { (void)data; (void)len; return 0; }
    qint64 readRawData(char* data, int len) { (void)data; (void)len; return 0; }
};

// ============================================================================
// QEvent stubs
// ============================================================================
class QEvent {
public:
    enum Type { None = 0, Timer = 1, MouseButtonPress = 2, MouseButtonRelease = 3,
        MouseButtonDblClick = 4, MouseMove = 5, KeyPress = 6, KeyRelease = 7,
        FocusIn = 8, FocusOut = 9, FocusAboutToChange = 23,
        Paint = 12, Move = 13, Resize = 14, Close = 19, Show = 17, Hide = 18,
        Enter = 10, Leave = 11, Wheel = 31, WindowActivate = 24,
        WindowDeactivate = 25, ShowToParent = 26, HideToParent = 27,
        ApplicationActivated = 22, ApplicationDeactivated = 23,
        Clipboard = 20, DragEnter = 60, DragMove = 61, DragLeave = 62,
        Drop = 63, ThemeChange = 210, StyleChange = 211,
        InputMethod = 194, TouchBegin = 188, TouchUpdate = 189, TouchEnd = 190,
        AccessibleDescription = 195, AccessibleText = 196,
        User = 1000, MaxUser = 65535, Max = 65536 };
    QEvent(Type type) : m_type(type) {}
    Type type() const { return m_type; }
    void accept() { m_accepted = true; }
    void ignore() { m_accepted = false; }
    bool isAccepted() const { return m_accepted; }
    void setAccepted(bool accepted) { m_accepted = accepted; }
    int timestamp() const { return 0; }
    bool spontaneous() const { return false; }
private:
    Type m_type;
    bool m_accepted = true;
};

class QMouseEvent : public QEvent {
public:
    QMouseEvent(Type type, const QPointF& pos, int button = 0, int buttons = 0, int modifiers = 0)
        : QEvent(type), m_pos(pos), m_button(button), m_buttons(buttons), m_modifiers(modifiers) {}
    QPointF pos() const { return m_pos; }
    QPointF localPos() const { return m_pos; }
    QPointF globalPos() const { return m_pos; }
    int button() const { return m_button; }
    int buttons() const { return m_buttons; }
    int modifiers() const { return m_modifiers; }
private:
    QPointF m_pos;
    int m_button;
    int m_buttons;
    int m_modifiers;
};

class QKeyEvent : public QEvent {
public:
    QKeyEvent(Type type, int key, int modifiers, const std::string& text = "", bool autoRepeat = false, int count = 1)
        : QEvent(type), m_key(key), m_modifiers(modifiers), m_text(text), m_autoRepeat(autoRepeat), m_count(count) {}
    int key() const { return m_key; }
    int modifiers() const { return m_modifiers; }
    std::string text() const { return m_text; }
    bool isAutoRepeat() const { return m_autoRepeat; }
    int count() const { return m_count; }
    bool nativeScanCode() const { return false; }
private:
    int m_key;
    int m_modifiers;
    std::string m_text;
    bool m_autoRepeat;
    int m_count;
};

class QWheelEvent : public QEvent {
public:
    QWheelEvent(const QPointF& pos, const QPoint& delta, int modifiers = 0)
        : QEvent(Wheel), m_pos(pos), m_delta(delta), m_modifiers(modifiers) {}
    QPointF pos() const { return m_pos; }
    QPointF globalPos() const { return m_pos; }
    QPoint delta() const { return m_delta; }
    int angleDelta() const { return m_delta.y(); }
    int xDelta() const { return m_delta.x(); }
    int yDelta() const { return m_delta.y(); }
    int modifiers() const { return m_modifiers; }
private:
    QPointF m_pos;
    QPoint m_delta;
    int m_modifiers;
};

class QResizeEvent : public QEvent {
public:
    QResizeEvent(const QSize& size, const QSize& oldSize) : QEvent(Resize), m_size(size), m_oldSize(oldSize) {}
    QSize size() const { return m_size; }
    QSize oldSize() const { return m_oldSize; }
private:
    QSize m_size, m_oldSize;
};

class QPaintEvent : public QEvent {
public:
    QPaintEvent(const QRect& rect) : QEvent(Paint), m_rect(rect) {}
    QPaintEvent(const QRegion& region) : QEvent(Paint), m_rect(region) {}
    QRect rect() const { return m_rect; }
private:
    QRect m_rect;
};

class QCloseEvent : public QEvent {
public:
    QCloseEvent() : QEvent(Close) {}
};

class QShowEvent : public QEvent {
public:
    QShowEvent() : QEvent(Show) {}
};

class QHideEvent : public QEvent {
public:
    QHideEvent() : QEvent(Hide) {}
};

class QMoveEvent : public QEvent {
public:
    QMoveEvent(const QPoint& pos, const QPoint& oldPos) : QEvent(Move), m_pos(pos), m_oldPos(oldPos) {}
    QPoint pos() const { return m_pos; }
    QPoint oldPos() const { return m_oldPos; }
private:
    QPoint m_pos, m_oldPos;
};

class QFocusEvent : public QEvent {
public:
    QFocusEvent(Type type) : QEvent(type) {}
    bool gotFocus() const { return type() == FocusIn; }
    bool lostFocus() const { return type() == FocusOut; }
};

class QEnterEvent : public QEvent {
public:
    QEnterEvent(const QPointF& pos) : QEvent(Enter), m_pos(pos) {}
    QPointF pos() const { return m_pos; }
    QPointF globalPos() const { return m_pos; }
private:
    QPointF m_pos;
};

class QTimerEvent : public QEvent {
public:
    QTimerEvent(int timerId) : QEvent(Timer), m_timerId(timerId) {}
    int timerId() const { return m_timerId; }
private:
    int m_timerId;
};

// ============================================================================
// QPainter stub
// ============================================================================
class QPainter {
public:
    QPainter() = default;
    QPainter(QObject* widget) { (void)widget; }
    bool begin(QObject*) { return true; }
    bool end() { return true; }
    bool isActive() const { return false; }
    void setPen(int) {}
    void setPen(const QPen&) {}
    void setBrush(const QBrush&) {}
    void setFont(const QFont&) {}
    void setOpacity(float) {}
    float opacity() const { return 1.0f; }
    void drawLine(const QPoint&, const QPoint&) {}
    void drawLine(const QPointF&, const QPointF&) {}
    void drawRect(const QRect&) {}
    void drawRect(const QRectF&) {}
    void drawEllipse(const QRect&) {}
    void drawEllipse(const QRectF&) {}
    void drawEllipse(const QPoint&, int, int) {}
    void drawEllipse(const QPointF&, float, float) {}
    void drawText(const QPoint&, const std::string&) {}
    void drawText(const QRect&, int, const std::string&) {}
    void drawPixmap(const QPoint&, const QPixmap&) {}
    void drawPixmap(const QRect&, const QPixmap&, const QRect& = QRect()) {}
    void drawImage(const QPoint&, const QImage&) {}
    void drawImage(const QRect&, const QImage&, const QRect& = QRect()) {}
    void fillRect(const QRect&, const QBrush&) {}
    void fillRect(const QRect&, const QColor&) {}
    void fillRect(int, int, int, int, const QColor&) {}
    void fillRect(const QRectF&, const QColor&) {}
    void drawPath(const QPainterPath&) {}
    void drawPolygon(const QPoint*, int) {}
    void drawPolygon(const QPointF*, int) {}
    void save() {}
    void restore() {}
    void translate(float, float) {}
    void translate(const QPointF&) {}
    void scale(float, float) {}
    void rotate(float) {}
    void setTransform(const QMatrix4x4&) {}
    QMatrix4x4 transform() const { return {}; }
    void setClipRect(const QRect&) {}
    void setClipRegion(const QRegion&) {}
    void setClipPath(const QPainterPath&) {}
    void resetClip() {}
    void setRenderHint(int, bool = true) {}
    void setCompositionMode(int) {}
    void setViewTransformEnabled(bool) {}
    void setViewport(const QRect&) {}
    QRect viewport() const { return {}; }
    QRect window() const { return {}; }
    void setWindow(const QRect&) {}
    void setWorldTransform(const QMatrix4x4&, bool = false) {}
    static const int Antialiasing = 0x01;
    static const int TextAntialiasing = 0x02;
    static const int SmoothPixmapTransform = 0x04;
    static const int HighQualityAntialiasing = 0x08;
};

// ============================================================================
// QRegion stub
// ============================================================================
struct QRegion {
    QRegion() = default;
    QRegion(const QRect& rect) : m_rect(rect) {}
    QRegion(const QPolygon&) {}
    bool isEmpty() const { return m_rect.isEmpty(); }
    bool contains(const QPoint&) const { return true; }
    QRegion united(const QRegion&) const { return {}; }
    QRegion intersected(const QRegion&) const { return {}; }
    QRegion subtracted(const QRegion&) const { return {}; }
    QRect boundingRect() const { return m_rect; }
private:
    QRect m_rect;
};

// ============================================================================
// QPainterPath stub
// ============================================================================
struct QPainterPath {
    QPainterPath() = default;
    void moveTo(float, float) {}
    void lineTo(float, float) {}
    void arcTo(float, float, float, float, float, float) {}
    void cubicTo(float, float, float, float, float, float) {}
    void quadTo(float, float, float, float) {}
    void closeSubpath() {}
    void addRect(float, float, float, float) {}
    void addEllipse(float, float, float, float) {}
    void addEllipse(const QPointF&, float, float) {}
    void addPath(const QPainterPath&) {}
    void addRegion(const QRegion&) {}
    bool isEmpty() const { return true; }
    float length() const { return 0; }
    QRectF boundingRect() const { return {}; }
    void setFillRule(int) {}
    void setElementPositionAt(int, float, float) {}
    QPainterPath toReversed() const { return {}; }
    QPainterPath intersected(const QPainterPath&) const { return {}; }
    QPainterPath united(const QPainterPath&) const { return {}; }
    static const int OddEvenFill = 0;
    static const int WindingFill = 1;
};

// ============================================================================
// QBrush / QPen / QFont stubs
// ============================================================================
struct QBrush {
    QBrush() = default;
    QBrush(const QColor& color) : m_color(color) {}
    QBrush(int style) { (void)style; }
    QColor color() const { return m_color; }
    void setColor(const QColor& c) { m_color = c; }
    void setStyle(int) {}
    void setTexture(const QPixmap&) {}
    void setTextureImage(const QImage&) {}
    bool isNull() const { return false; }
private:
    QColor m_color;
};

struct QPen {
    QPen() = default;
    QPen(const QColor& color) : m_color(color) {}
    QPen(float width) : m_width(width) {}
    QPen(const QBrush& brush, float width) : m_brush(brush), m_width(width) {}
    QColor color() const { return m_color; }
    void setColor(const QColor& c) { m_color = c; }
    void setWidth(float w) { m_width = w; }
    float widthF() const { return m_width; }
    int width() const { return (int)m_width; }
    void setStyle(int) {}
    void setCapStyle(int) {}
    void setJoinStyle(int) {}
    bool isCosmetic() const { return false; }
    void setCosmetic(bool) {}
    QBrush brush() const { return m_brush; }
private:
    QColor m_color;
    float m_width = 1.0f;
    QBrush m_brush;
};

struct QFont {
    QFont() = default;
    QFont(const std::string& family, int pointSize = -1, int weight = -1)
        : m_family(family), m_pointSize(pointSize), m_weight(weight) {}
    std::string family() const { return m_family; }
    void setFamily(const std::string& f) { m_family = f; }
    int pointSize() const { return m_pointSize; }
    void setPointSize(int size) { m_pointSize = size; }
    int pixelSize() const { return -1; }
    void setPixelSize(int) {}
    int weight() const { return m_weight; }
    void setWeight(int w) { m_weight = w; }
    bool bold() const { return m_bold; }
    void setBold(bool b) { m_bold = b; }
    bool italic() const { return m_italic; }
    void setItalic(bool i) { m_italic = i; }
    bool underline() const { return false; }
    void setUnderline(bool) {}
    bool strikethrough() const { return false; }
    void setStrikethrough(bool) {}
    void setFixedPitch(bool) {}
    bool fixedPitch() const { return false; }
    void setKerning(bool) {}
    bool kerning() const { return true; }
    std::string styleName() const { return ""; }
    void setStyleName(const std::string&) {}
    static const int Light = 25;
    static const int Normal = 50;
    static const int DemiBold = 63;
    static const int Bold = 75;
    static const int Black = 87;
private:
    std::string m_family;
    int m_pointSize = -1;
    int m_weight = -1;
    bool m_bold = false;
    bool m_italic = false;
};

// ============================================================================
// QFontMetrics / QFontInfo stubs
// ============================================================================
struct QFontMetrics {
    QFontMetrics(const QFont&) {}
    int width(const std::string&) const { return 0; }
    int height() const { return 12; }
    int ascent() const { return 10; }
    int descent() const { return 2; }
    int leading() const { return 2; }
    int horizontalAdvance(const std::string&) const { return 0; }
    int boundingRect(char) const { return 0; }
    QRect boundingRect(const QRect&, int, const std::string&, int = 0, QStringList* = nullptr) const { return {}; }
};

struct QFontInfo {
    QFontInfo(const QFont&) {}
    std::string family() const { return ""; }
    std::string styleName() const { return ""; }
    int pixelSize() const { return 12; }
    int pointSize() const { return 10; }
    bool fixedPitch() const { return false; }
};

// ============================================================================
// QPixmap stub
// ============================================================================
class QPixmap {
public:
    QPixmap() = default;
    QPixmap(int w, int h) : m_width(w), m_height(h) {}
    QPixmap(const std::string& fileName) { (void)fileName; }
    QPixmap(const QSize& size) : m_width(size.width()), m_height(size.height()) {}
    int width() const { return m_width; }
    int height() const { return m_height; }
    QSize size() const { return {m_width, m_height}; }
    bool isNull() const { return m_width == 0 || m_height == 0; }
    bool load(const std::string&) { return false; }
    bool loadFromData(const unsigned char*, int) { return false; }
    bool save(const std::string&) const { return false; }
    QImage toImage() const { return {}; }
    void fill(const QColor&) {}
    void swap(QPixmap&) {}
    QPixmap scaled(int w, int h) const { return {w, h}; }
    QPixmap copy(int, int, int, int) const { return {}; }
private:
    int m_width = 0, m_height = 0;
};

// ============================================================================
// QGuiApplication stub
// ============================================================================
class QGuiApplication : public QCoreApplication {
public:
    QGuiApplication(int& argc, char** argv) : QCoreApplication() { (void)argc; (void)argv; }
    static QGuiApplication* instance() { return nullptr; }
};

// ============================================================================
// QApplication stub (inherits QGuiApplication)
// ============================================================================
class QApplication : public QGuiApplication {
public:
    QApplication(int& argc, char** argv) : QGuiApplication(argc, argv) {}
    static QApplication* instance() { return nullptr; }
};

// ============================================================================
// QEventLoop stub
// ============================================================================
class QEventLoop : public QObject {
public:
    QEventLoop(QObject* parent = nullptr) : QObject(parent) {}
    void exec(int = 0) {}
    void exit(int returnCode = 0) { (void)returnCode; }
    bool isRunning() const { return false; }
    void quit() {}
    void processEvents(int = 0) {}
    void processEvents(int, int) {}
};

// ============================================================================
// QElapsedTimer for non-QObject contexts (already defined above as standalone)
// ============================================================================

// ============================================================================
// QStringView stub
// ============================================================================
class QStringView {
public:
    QStringView() = default;
    QStringView(const std::string& s) : m_string(s) {}
    QStringView(const char* s) : m_string(s ? s : "") {}
    std::string toString() const { return m_string; }
    const char* data() const { return m_string.data(); }
    int size() const { return (int)m_string.size(); }
    bool isEmpty() const { return m_string.empty(); }
    std::string toStdString() const { return m_string; }
private:
    std::string m_string;
};

// ============================================================================
// Key definitions (Windows virtual key codes for Qt compat)
// ============================================================================
namespace Qt {
    enum Key {
        Key_Escape = 0x01000000, Key_Tab = 0x01000001, Key_Backtab = 0x01000002,
        Key_Backspace = 0x01000003, Key_Return = 0x01000004, Key_Enter = 0x01000005,
        Key_Insert = 0x01000006, Key_Delete = 0x01000007, Key_Pause = 0x01000008,
        Key_Print = 0x01000009, Key_SysReq = 0x0100000a, Key_Clear = 0x0100000b,
        Key_Home = 0x01000010, Key_End = 0x01000011, Key_Left = 0x01000012,
        Key_Up = 0x01000013, Key_Right = 0x01000014, Key_Down = 0x01000015,
        Key_Space = 0x20, Key_Any = Key_Space,
        Key_A = 0x41, Key_B = 0x42, Key_C = 0x43, Key_D = 0x44, Key_E = 0x45,
        Key_F = 0x46, Key_G = 0x47, Key_H = 0x48, Key_I = 0x49, Key_J = 0x4a,
        Key_K = 0x4b, Key_L = 0x4c, Key_M = 0x4d, Key_N = 0x4e, Key_O = 0x4f,
        Key_P = 0x50, Key_Q = 0x51, Key_R = 0x52, Key_S = 0x53, Key_T = 0x54,
        Key_U = 0x55, Key_V = 0x56, Key_W = 0x57, Key_X = 0x58, Key_Y = 0x59,
        Key_Z = 0x5a, Key_0 = 0x30, Key_1 = 0x31, Key_2 = 0x32, Key_3 = 0x33,
        Key_4 = 0x34, Key_5 = 0x35, Key_6 = 0x36, Key_7 = 0x37, Key_8 = 0x38, Key_9 = 0x39,
        Key_F1 = 0x01000030, Key_F2 = 0x01000031, Key_F3 = 0x01000032, Key_F4 = 0x01000033,
        Key_F5 = 0x01000034, Key_F6 = 0x01000035, Key_F7 = 0x01000036, Key_F8 = 0x01000037,
        Key_F9 = 0x01000038, Key_F10 = 0x01000039, Key_F11 = 0x0100003a, Key_F12 = 0x0100003b,
        Key_Control = 0x01000021, Key_Shift = 0x01000022, Key_Alt = 0x01000023, Key_Meta = 0x01000024,
        Key_CapsLock = 0x01000025, Key_NumLock = 0x01000026, Key_ScrollLock = 0x01000027,
        Key_LeftButton = 0x01000012, Key_RightButton = 0x01000013, Key_MiddleButton = 0x01000014,
    };

    enum MouseButton { NoButton = 0x00000000, LeftButton = 0x00000001, RightButton = 0x00000002, MiddleButton = 0x00000004 };
    enum Modifier { NoModifier = 0x00000000, ShiftModifier = 0x02000000, ControlModifier = 0x04000000, AltModifier = 0x08000000, MetaModifier = 0x10000000 };
    enum WindowFlags { Window = 0x00000001, Dialog = 0x00000002, Sheet = 0x00000004, Popup = 0x00000008,
        Tool = 0x00000010, ToolTip = 0x00000020, SplashScreen = 0x00000040, Desktop = 0x00000080,
        SubWindow = 0x00000100, ForeignWindow = 0x00000200, CoverWindow = 0x00000400,
        WindowType_Mask = 0x000000ff, WindowFlags_Mask = 0xffff0000,
        CustomizeWindowHint = 0x00000000, WindowTitleHint = 0x00000100, WindowSystemMenuHint = 0x00000200,
        WindowMinimizeButtonHint = 0x00000400, WindowMaximizeButtonHint = 0x00000800,
        WindowMinMaxButtonsHint = WindowMinimizeButtonHint | WindowMaximizeButtonHint,
        WindowCloseButtonHint = 0x00001000, WindowStaysOnTopHint = 0x00004000,
        FramelessWindowHint = 0x00000800, WindowStaysOnBottomHint = 0x00040000,
        TransparentForInput = 0x00080000, WindowDoesNotAcceptFocus = 0x00100000,
        NoDropShadowWindowHint = 0x00400000, WindowFullscreenButtonHint = 0x40000000,
    };
    enum TextFormat { PlainText = 0, RichText = 1, AutoText = 2, MarkdownText = 3 };
    enum AlignmentFlag { AlignLeft = 0x0001, AlignRight = 0x0002, AlignHCenter = 0x0004, AlignJustify = 0x0008,
        AlignTop = 0x0010, AlignBottom = 0x0020, AlignVCenter = 0x0040,
        AlignCenter = AlignHCenter | AlignVCenter, AlignAbsolute = 0x0010, AlignLeading = AlignLeft, AlignTrailing = AlignRight };
    enum Orientation { Horizontal = 0x1, Vertical = 0x2 };
    enum WindowState { WindowNoState = 0, WindowMinimized = 1, WindowMaximized = 2, WindowFullScreen = 4, WindowActive = 8 };
    enum SortOrder { AscendingOrder = 0, DescendingOrder = 1 };
    enum ItemFlag { NoItemFlags = 0, ItemIsSelectable = 1, ItemIsEditable = 2, ItemIsDragEnabled = 4, ItemIsDropEnabled = 8, ItemIsUserCheckable = 16, ItemIsEnabled = 32, ItemIsTristate = 64 };
    enum MatchFlag { MatchExactly = 0, MatchContains = 1, MatchStartsWith = 2, MatchEndsWith = 3, MatchWildcard = 4, MatchRegExp = 5, MatchCaseSensitively = 0x0001, MatchWrap = 0x0002, MatchRecursive = 0x0004 };
    enum ItemDataRole { DisplayRole = 0, DecorationRole = 1, EditRole = 2, ToolTipRole = 3, StatusTipRole = 4, WhatsThisRole = 5, FontRole = 6, TextAlignmentRole = 7, BackgroundRole = 8, ForegroundRole = 9, CheckStateRole = 10, SizeHintRole = 11, UserRole = 0x0100 };
    enum CheckState { Unchecked = 0, PartiallyChecked = 1, Checked = 2 };
    enum WindowType { Widget = 0x00000000 };
    enum ApplicationAttribute { AA_EnableHighDpiScaling = 13, AA_DisableHighDpiScaling = 14, AA_UseHighDpiPixmaps = 15 };
}

// ============================================================================
// QApplication statics
// ============================================================================
inline QApplication* qApp = nullptr;

// ============================================================================
// QString <-> std::string interop additions
// ============================================================================
namespace std {
    inline std::string to_string(const QString& s) { return s; }
}

// ============================================================================
// Additional forward declarations for types used in engine headers
// ============================================================================
class QAbstractItemModel;
class QItemSelectionModel;
class QSortFilterProxyModel;
class QStringListModel;
class QAbstractTableModel;
class QAbstractListModel;
class QTreeView;
class QTableView;
class QListView;
class QItemDelegate;
class QStyledItemDelegate;
class QItemSelection;
class QUndoStack;
class QUndoCommand;
class QUndoGroup;
class QUndoView;
class QSpinBox;
class QDoubleSpinBox;
class QComboBox;
class QCheckBox;
class QRadioButton;
class QSlider;
class QProgressBar;
class QSplitter;
class QStackedWidget;
class QTabWidget;
class QGroupBox;
class QFrame;
class QScrollArea;
class QScrollBar;
class QHeaderView;
class QToolBar;
class QMenuBar;
class QStatusBar;
class QDockWidget;
class QFileDialog;
class QMessageBox;
class QInputDialog;
class QColorDialog;
class QFontDialog;
class QStyle;
class QStyleOption;
class QDesktopWidget;
class QButtonGroup;
class QAction;
class QActionGroup;
class QShortcut;
class QWidgetAction;
class QWindowContainer;
class QMdiArea;
class QMdiSubWindow;
class QCalendarWidget;
class QDateEdit;
class QTimeEdit;
class QDateTimeEdit;
class QTextBrowser;
class QColumnView;
class QDataWidgetMapper;
class QCompleter;
class QSystemTrayIcon;
class QProcess;

// ============================================================================
// Forward-declare sl namespace types used in StreamlineFunctions.h
// ============================================================================
namespace sl {
    using Result = int;
}

#endif // KSENGINE_NO_QT
