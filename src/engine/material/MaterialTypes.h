#pragma once

// Qt-free value types used by the material module: packed RGBA color and the
// small geometry vocabulary that used to be QColor/QRgb/QPoint/QSize/QRect.
// Plain aggregates, no Qt, header only.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

namespace ks {

// Packed pixel, 0xAARRGGBB, same layout as QColor::rgba().
using Rgb = std::uint32_t;

constexpr Rgb packRgba(int r, int g, int b, int a)
{
    return (static_cast<Rgb>(static_cast<std::uint8_t>(a) ) << 24) |
           (static_cast<Rgb>(static_cast<std::uint8_t>(r) ) << 16) |
           (static_cast<Rgb>(static_cast<std::uint8_t>(g) ) << 8)  |
            static_cast<Rgb>(static_cast<std::uint8_t>(b));
}

constexpr int clampChannel(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

struct Color {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;

    Color() = default;
    Color(int red, int green, int blue, int alpha = 255)
        : r(static_cast<std::uint8_t>(clampChannel(red)))
        , g(static_cast<std::uint8_t>(clampChannel(green)))
        , b(static_cast<std::uint8_t>(clampChannel(blue)))
        , a(static_cast<std::uint8_t>(clampChannel(alpha))) {}

    static constexpr Color fromRgba32(Rgb v)
    {
        Color c;
        c.r = static_cast<std::uint8_t>((v >> 16) & 0xFF);
        c.g = static_cast<std::uint8_t>((v >> 8) & 0xFF);
        c.b = static_cast<std::uint8_t>(v & 0xFF);
        c.a = static_cast<std::uint8_t>((v >> 24) & 0xFF);
        return c;
    }

    static Color fromHsv(int hue, int sat, int val, int alpha = 255)
    {
        // hue 0..359, sat/val 0..255 (QColor::fromHsv units).
        const double h = (static_cast<double>(hue % 360) + 360.0) * (6.0 / 360.0);
        const double s = std::min(255, std::max(0, sat)) / 255.0;
        const double v = std::min(255, std::max(0, val)) / 255.0;
        const double c = v * s;
        const double x = c * (1.0 - std::fabs(std::fmod(h, 2.0) - 1.0));
        const double m = v - c;
        double rr = 0, gg = 0, bb = 0;
        const int sector = static_cast<int>(h) % 6;
        switch (sector) {
        case 0: rr = c; gg = x; bb = 0; break;
        case 1: rr = x; gg = c; bb = 0; break;
        case 2: rr = 0; gg = c; bb = x; break;
        case 3: rr = 0; gg = x; bb = c; break;
        case 4: rr = x; gg = 0; bb = c; break;
        default: rr = c; gg = 0; bb = x; break;
        }
        return Color(static_cast<int>((rr + m) * 255.0 + 0.5),
                     static_cast<int>((gg + m) * 255.0 + 0.5),
                     static_cast<int>((bb + m) * 255.0 + 0.5), alpha);
    }

    static Color fromHsvF(double hue, double sat, double val, double alpha = 1.0)
    {
        return fromHsv(static_cast<int>(hue * 360.0),
                       static_cast<int>(sat * 255.0),
                       static_cast<int>(val * 255.0),
                       static_cast<int>(alpha * 255.0));
    }

    // QColor::getHsv() equivalent: hue 0..359 (never -1; QColor returned -1
    // for achromatic colours, which the call sites ignored anyway because the
    // saturation is 0 in that case), sat/val/alpha 0..255.
    void toHsv(int* h, int* s, int* v, int* outA) const
    {
        const int mx = std::max<int>(r, std::max<int>(g, b));
        const int mn = std::min<int>(r, std::min<int>(g, b));
        const int range = mx - mn;

        int hue = 0;
        if (range != 0) {
            if (mx == r) hue = (60 * (static_cast<int>(g) - static_cast<int>(b)) / range + 360) % 360;
            else if (mx == g) hue = (60 * (static_cast<int>(b) - static_cast<int>(r)) / range + 180) % 360;
            else hue = (60 * (static_cast<int>(r) - static_cast<int>(g)) / range + 300) % 360;
        }
        if (h) *h = hue;
        if (s) *s = mx == 0 ? 0 : range * 255 / mx;
        if (v) *v = mx;
        if (outA) *outA = static_cast<int>(a);
    }

    int red() const { return r; }
    int green() const { return g; }
    int blue() const { return b; }
    int alpha() const { return a; }
    double redF() const { return r / 255.0; }
    double greenF() const { return g / 255.0; }
    double blueF() const { return b / 255.0; }
    double alphaF() const { return a / 255.0; }

    Rgb rgba() const { return packRgba(r, g, b, a); }

    std::string name() const
    {
        static const char* hex = "0123456789abcdef";
        std::string out = "#......";
        out[1] = hex[r >> 4]; out[2] = hex[r & 15];
        out[3] = hex[g >> 4]; out[4] = hex[g & 15];
        out[5] = hex[b >> 4]; out[6] = hex[b & 15];
        return out;
    }

    static Color white() { return Color(255, 255, 255); }
    static Color black() { return Color(0, 0, 0); }
    static Color transparent() { return Color(0, 0, 0, 0); }

    bool operator==(const Color& o) const { return r == o.r && g == o.g && b == o.b && a == o.a; }
    bool operator!=(const Color& o) const { return !(*this == o); }
};

struct Point {
    int x = 0;
    int y = 0;

    Point() = default;
    Point(int x, int y) : x(x), y(y) {}

    Point operator+(const Point& o) const { return Point(x + o.x, y + o.y); }
    Point operator-(const Point& o) const { return Point(x - o.x, y - o.y); }
    Point operator*(int s) const { return Point(x * s, y * s); }
    Point& operator+=(const Point& o) { x += o.x; y += o.y; return *this; }
    Point& operator-=(const Point& o) { x -= o.x; y -= o.y; return *this; }
    bool operator==(const Point& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Point& o) const { return !(*this == o); }
    bool operator<(const Point& o) const { return y != o.y ? y < o.y : x < o.x; }

    double length() const { return std::sqrt(static_cast<double>(x) * x + static_cast<double>(y) * y); }
    int manhattanLength() const { return std::abs(x) + std::abs(y); }
};

struct PointF {
    double x = 0;
    double y = 0;

    PointF() = default;
    PointF(double x, double y) : x(x), y(y) {}
    explicit PointF(const Point& p) : x(p.x), y(p.y) {}

    PointF operator+(const PointF& o) const { return PointF(x + o.x, y + o.y); }
    PointF operator-(const PointF& o) const { return PointF(x - o.x, y - o.y); }
    PointF operator*(double s) const { return PointF(x * s, y * s); }
    PointF& operator+=(const PointF& o) { x += o.x; y += o.y; return *this; }
    bool operator==(const PointF& o) const { return x == o.x && y == o.y; }

    Point toPoint() const
    {
        return Point(static_cast<int>(std::floor(x + 0.5)), static_cast<int>(std::floor(y + 0.5)));
    }
    double length() const { return std::sqrt(x * x + y * y); }
};

struct Size {
    int w = 0;
    int h = 0;

    Size() = default;
    Size(int w, int h) : w(w), h(h) {}

    bool isEmpty() const { return w <= 0 || h <= 0; }
    bool operator==(const Size& o) const { return w == o.w && h == o.h; }
};

struct SizeF {
    double w = 0;
    double h = 0;

    SizeF() = default;
    SizeF(double w, double h) : w(w), h(h) {}
    bool operator==(const SizeF& o) const { return w == o.w && h == o.h; }
};

struct Rect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;

    Rect() = default;
    Rect(int x, int y, int w, int h) : x(x), y(y), w(w), h(h) {}
    Rect(const Point& topLeft, const Size& size) : x(topLeft.x), y(topLeft.y), w(size.w), h(size.h) {}
    Rect(const Point& topLeft, const Point& bottomRight)
        : x(topLeft.x), y(topLeft.y)
        , w(bottomRight.x - topLeft.x), h(bottomRight.y - topLeft.y) {}

    int width() const { return w; }
    int height() const { return h; }
    int left() const { return x; }
    int top() const { return y; }
    int right() const { return x + w - 1; }
    int bottom() const { return y + h - 1; }

    Point topLeft() const { return Point(x, y); }
    Point bottomRight() const { return Point(x + w, y + h); }
    Point center() const { return Point(x + w / 2, y + h / 2); }
    Size size() const { return Size(w, h); }

    bool isNull() const { return x == 0 && y == 0 && w == 0 && h == 0; }
    bool isEmpty() const { return w <= 0 || h <= 0; }

    bool contains(const Point& p) const
    {
        return p.x >= x && p.y >= y && p.x < x + w && p.y < y + h;
    }

    bool contains(int px, int py) const
    {
        return px >= x && py >= y && px < x + w && py < y + h;
    }

    Rect intersected(const Rect& o) const
    {
        const int nx = std::max(x, o.x);
        const int ny = std::max(y, o.y);
        const int nr = std::min(x + w, o.x + o.w);
        const int nb = std::min(y + h, o.y + o.h);
        if (nr <= nx || nb <= ny) return Rect();
        return Rect(nx, ny, nr - nx, nb - ny);
    }

    Rect united(const Rect& o) const
    {
        if (isEmpty()) return o;
        if (o.isEmpty()) return *this;
        const int nx = std::min(x, o.x);
        const int ny = std::min(y, o.y);
        const int nr = std::max(x + w, o.x + o.w);
        const int nb = std::max(y + h, o.y + o.h);
        return Rect(nx, ny, nr - nx, nb - ny);
    }

    bool operator==(const Rect& o) const { return x == o.x && y == o.y && w == o.w && h == o.h; }
    bool operator!=(const Rect& o) const { return !(*this == o); }
};

struct RectF {
    double x = 0;
    double y = 0;
    double w = 0;
    double h = 0;

    RectF() = default;
    RectF(double x, double y, double w, double h) : x(x), y(y), w(w), h(h) {}
    RectF(const Point& topLeft, const SizeF& size)
        : x(topLeft.x), y(topLeft.y), w(size.w), h(size.h) {}
    RectF(const PointF& topLeft, const PointF& bottomRight)
        : x(topLeft.x), y(topLeft.y), w(bottomRight.x - topLeft.x), h(bottomRight.y - topLeft.y) {}

    double width() const { return w; }
    double height() const { return h; }
    PointF topLeft() const { return PointF(x, y); }
    PointF bottomRight() const { return PointF(x + w, y + h); }
    PointF center() const { return PointF(x + w * 0.5, y + h * 0.5); }
    bool isEmpty() const { return w <= 0 || h <= 0; }

    Rect toRect() const
    {
        return Rect(static_cast<int>(std::floor(x)), static_cast<int>(std::floor(y)),
                    static_cast<int>(std::ceil(w)), static_cast<int>(std::ceil(h)));
    }

    RectF intersected(const RectF& o) const
    {
        const double nx = std::max(x, o.x);
        const double ny = std::max(y, o.y);
        const double nr = std::min(x + w, o.x + o.w);
        const double nb = std::min(y + h, o.y + o.h);
        if (nr <= nx || nb <= ny) return RectF();
        return RectF(nx, ny, nr - nx, nb - ny);
    }

    bool operator==(const RectF& o) const { return x == o.x && y == o.y && w == o.w && h == o.h; }
};

} // namespace ks
