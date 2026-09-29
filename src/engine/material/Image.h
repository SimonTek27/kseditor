#pragma once

// Qt-free raster image used by the material module (the QImage replacement).
//
// Storage is always straight (non premultiplied) RGBA8 in row-major order,
// which matches what QImage::Format_RGBA8888 held for the atlas code and what
// the paint code built with Format_ARGB32. The premultiplied formats Qt used
// internally are not reproduced: every painter operation here composites
// straight alpha directly, so no conversion step is needed (and the
// Format_ARGB32_Premultiplied distinction disappears).
//
// File IO goes through PngCodec (PNG/BMP/TGA), replacing QImage::load() and
// QImage::save().

#include "MaterialTypes.h"
#include "PngCodec.h"
#include "Transform.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ks {

class Image {
public:
    Image() = default;
    Image(int width, int height); // transparent black
    Image(const Size& size);

    static Image fromRgba(int width, int height, const std::uint8_t* data);

    bool isNull() const { return w <= 0 || h <= 0; }
    int width() const { return w; }
    int height() const { return h; }
    Size size() const { return Size(w, h); }
    Rect rect() const { return Rect(0, 0, w, h); }
    int byteCount() const { return static_cast<int>(m_data.size()); }

    std::uint8_t* bits() { return m_data.data(); }
    const std::uint8_t* bits() const { return m_data.data(); }
    const std::uint8_t* constBits() const { return m_data.data(); }

    bool contains(int x, int y) const { return x >= 0 && y >= 0 && x < w && y < h; }
    bool contains(const Point& p) const { return contains(p.x, p.y); }

    Rgb pixel(int x, int y) const;
    void setPixel(int x, int y, Rgb rgba);
    Rgb pixel(const Point& p) const { return pixel(p.x, p.y); }
    void setPixel(const Point& p, Rgb rgba) { setPixel(p.x, p.y, rgba); }

    Color colorAt(int x, int y) const { return Color::fromRgba32(pixel(x, y)); }
    void setColorAt(int x, int y, const Color& c) { setPixel(x, y, c.rgba()); }

    void fill(const Color& c);
    void fill(Rgb rgba) { fill(Color::fromRgba32(rgba)); }
    void clear() { fill(Color::transparent()); }

    Image copy() const { return *this; }
    Image copy(const Rect& region) const;

    Image scaled(int newWidth, int newHeight) const; // bilinear
    Image mirrored(bool horizontal, bool vertical) const;
    Image flippedVertical() const { return mirrored(false, true); }
    Image flippedHorizontal() const { return mirrored(true, false); }
    Image rgbSwapped() const;
    Image transformed(const Transform& t) const;
    void invertPixels();

    bool load(const std::string& path, std::string* err = nullptr);
    bool save(const std::string& path, std::string* err = nullptr) const;

    bool operator==(const Image& o) const { return w == o.w && h == o.h && m_data == o.m_data; }
    bool operator!=(const Image& o) const { return !(*this == o); }

private:
    int w = 0;
    int h = 0;
    std::vector<std::uint8_t> m_data;
};

} // namespace ks
