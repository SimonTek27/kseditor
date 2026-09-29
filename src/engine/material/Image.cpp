#include "Image.h"

#include <algorithm>
#include <cmath>

namespace ks {

Image::Image(int width, int height)
{
    if (width > 0 && height > 0) {
        w = width;
        h = height;
        m_data.assign(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u, 0);
    }
}

Image::Image(const Size& size) : Image(size.w, size.h) {}

Image Image::fromRgba(int width, int height, const std::uint8_t* data)
{
    Image img(width, height);
    if (data && !img.m_data.empty())
        std::copy(data, data + img.m_data.size(), img.m_data.begin());
    return img;
}

Rgb Image::pixel(int x, int y) const
{
    if (!contains(x, y)) return 0;
    const std::size_t i = (static_cast<std::size_t>(y) * w + x) * 4;
    return packRgba(m_data[i], m_data[i + 1], m_data[i + 2], m_data[i + 3]);
}

void Image::setPixel(int x, int y, Rgb rgba)
{
    if (!contains(x, y)) return;
    const Color c = Color::fromRgba32(rgba);
    const std::size_t i = (static_cast<std::size_t>(y) * w + x) * 4;
    m_data[i] = c.r;
    m_data[i + 1] = c.g;
    m_data[i + 2] = c.b;
    m_data[i + 3] = c.a;
}

void Image::fill(const Color& c)
{
    if (m_data.empty()) return;
    std::size_t i = 0;
    while (i + 3 < m_data.size()) {
        m_data[i] = c.r;
        m_data[i + 1] = c.g;
        m_data[i + 2] = c.b;
        m_data[i + 3] = c.a;
        i += 4;
    }
}

Image Image::copy(const Rect& region) const
{
    // QImage::copy() with a null rectangle copies the whole image; the filter
    // entry points rely on that ("apply the filter to everything").
    if (region.w == 0 && region.h == 0) return *this;

    const Rect r = region.intersected(rect());
    if (r.isEmpty()) return Image();
    Image out(r.w, r.h);
    for (int y = 0; y < r.h; ++y) {
        const std::uint8_t* src = m_data.data() +
                                  (static_cast<std::size_t>(r.y + y) * w + r.x) * 4;
        std::uint8_t* dst = out.m_data.data() + static_cast<std::size_t>(y) * r.w * 4;
        std::copy(src, src + static_cast<std::size_t>(r.w) * 4, dst);
    }
    return out;
}

Image Image::scaled(int newWidth, int newHeight) const
{
    if (isNull() || newWidth <= 0 || newHeight <= 0) return Image();
    if (newWidth == w && newHeight == h) return *this;

    Image out(newWidth, newHeight);
    const double sx = static_cast<double>(w) / newWidth;
    const double sy = static_cast<double>(h) / newHeight;
    for (int y = 0; y < newHeight; ++y) {
        const double fy = (y + 0.5) * sy - 0.5;
        const int y0 = static_cast<int>(std::floor(fy));
        const double ty = fy - y0;
        const int y1 = std::min(y0 + 1, h - 1);
        const int yc = std::max(0, std::min(y0, h - 1));
        for (int x = 0; x < newWidth; ++x) {
            const double fx = (x + 0.5) * sx - 0.5;
            const int x0 = static_cast<int>(std::floor(fx));
            const double tx = fx - x0;
            const int x1 = std::min(x0 + 1, w - 1);
            const int xc = std::max(0, std::min(x0, w - 1));

            int acc[4] = {0, 0, 0, 0};
            const int xs[2] = {xc, x1};
            const int ys[2] = {yc, y1};
            const double wx[2] = {1.0 - tx, tx};
            const double wy[2] = {1.0 - ty, ty};
            for (int j = 0; j < 2; ++j) {
                for (int i = 0; i < 2; ++i) {
                    const std::size_t idx = (static_cast<std::size_t>(ys[j]) * w + xs[i]) * 4;
                    const double weight = wx[i] * wy[j];
                    for (int c = 0; c < 4; ++c)
                        acc[c] += static_cast<int>(m_data[idx + c] * weight * 256.0 + 0.5);
                }
            }
            const std::size_t o = (static_cast<std::size_t>(y) * newWidth + x) * 4;
            for (int c = 0; c < 4; ++c)
                out.m_data[o + c] = static_cast<std::uint8_t>(std::min(255, acc[c] / 256));
        }
    }
    return out;
}

Image Image::mirrored(bool horizontal, bool vertical) const
{
    if (isNull()) return Image();
    Image out(w, h);
    for (int y = 0; y < h; ++y) {
        const int sy = vertical ? (h - 1 - y) : y;
        for (int x = 0; x < w; ++x) {
            const int sx = horizontal ? (w - 1 - x) : x;
            const std::size_t src = (static_cast<std::size_t>(sy) * w + sx) * 4;
            const std::size_t dst = (static_cast<std::size_t>(y) * w + x) * 4;
            std::copy_n(m_data.data() + src, 4, out.m_data.data() + dst);
        }
    }
    return out;
}

Image Image::rgbSwapped() const
{
    Image out = *this;
    for (std::size_t i = 0; i + 3 < out.m_data.size(); i += 4)
        std::swap(out.m_data[i], out.m_data[i + 2]);
    return out;
}

Image Image::transformed(const Transform& t) const
{
    if (isNull()) return Image();

    const PointF c0 = t.map(PointF(0, 0));
    const PointF c1 = t.map(PointF(w, 0));
    const PointF c2 = t.map(PointF(0, h));
    const PointF c3 = t.map(PointF(w, h));
    const double minX = std::min(std::min(c0.x, c1.x), std::min(c2.x, c3.x));
    const double maxX = std::max(std::max(c0.x, c1.x), std::max(c2.x, c3.x));
    const double minY = std::min(std::min(c0.y, c1.y), std::min(c2.y, c3.y));
    const double maxY = std::max(std::max(c0.y, c1.y), std::max(c2.y, c3.y));

    const int outW = static_cast<int>(std::ceil(maxX - minX));
    const int outH = static_cast<int>(std::ceil(maxY - minY));
    if (outW <= 0 || outH <= 0) return Image();

    Image out(outW, outH);
    const Transform inv = t.inverted();
    for (int y = 0; y < outH; ++y) {
        for (int x = 0; x < outW; ++x) {
            const PointF src = inv.map(PointF(x + 0.5 + minX, y + 0.5 + minY));
            const int sx = static_cast<int>(std::floor(src.x));
            const int sy = static_cast<int>(std::floor(src.y));
            if (sx < 0 || sy < 0 || sx >= w || sy >= h) continue;
            const std::size_t si = (static_cast<std::size_t>(sy) * w + sx) * 4;
            const std::size_t di = (static_cast<std::size_t>(y) * outW + x) * 4;
            std::copy_n(m_data.data() + si, 4, out.m_data.data() + di);
        }
    }
    return out;
}

void Image::invertPixels()
{
    for (std::size_t i = 0; i + 3 < m_data.size(); i += 4) {
        m_data[i] = static_cast<std::uint8_t>(255 - m_data[i]);
        m_data[i + 1] = static_cast<std::uint8_t>(255 - m_data[i + 1]);
        m_data[i + 2] = static_cast<std::uint8_t>(255 - m_data[i + 2]);
    }
}

bool Image::load(const std::string& path, std::string* err)
{
    image::RawImage raw;
    if (!image::loadImageFile(path, &raw, err)) return false;
    w = raw.width;
    h = raw.height;
    m_data = std::move(raw.rgba);
    return true;
}

bool Image::save(const std::string& path, std::string* err) const
{
    if (isNull()) return false;
    image::RawImage raw;
    raw.width = w;
    raw.height = h;
    raw.rgba = m_data;
    return image::saveImageFile(path, raw, err);
}

} // namespace ks
