#include "Painter.h"

#include <algorithm>
#include <cmath>

namespace ks {

// ---------------------------------------------------------------------------
// Gradient
// ---------------------------------------------------------------------------

Gradient Gradient::linear(const PointF& from, const PointF& to)
{
    Gradient g;
    g.m_kind = Kind::Linear;
    g.m_from = from;
    g.m_to = to;
    return g;
}

Gradient Gradient::radial(const PointF& center, double radius)
{
    Gradient g;
    g.m_kind = Kind::Radial;
    g.m_center = center;
    g.m_radius = radius > 0.0 ? radius : 1.0;
    return g;
}

void Gradient::setColorAt(double pos, const Color& color)
{
    const double p = std::min(1.0, std::max(0.0, pos));
    auto it = std::lower_bound(m_stops.begin(), m_stops.end(), p,
                               [](const Stop& s, double v) { return s.pos < v; });
    if (it != m_stops.end() && it->pos == p) {
        it->color = color;
        return;
    }
    m_stops.insert(it, Stop{p, color});
}

Color Gradient::at(double t) const
{
    if (m_stops.empty()) return Color::transparent();
    const double p = std::min(1.0, std::max(0.0, t));
    if (p <= m_stops.front().pos) return m_stops.front().color;
    if (p >= m_stops.back().pos) return m_stops.back().color;
    for (std::size_t i = 1; i < m_stops.size(); ++i) {
        if (p <= m_stops[i].pos) {
            const Stop& a = m_stops[i - 1];
            const Stop& b = m_stops[i];
            const double span = b.pos - a.pos;
            const double f = span > 0.0 ? (p - a.pos) / span : 0.0;
            return Color(static_cast<int>(a.color.r + (b.color.r - a.color.r) * f),
                         static_cast<int>(a.color.g + (b.color.g - a.color.g) * f),
                         static_cast<int>(a.color.b + (b.color.b - a.color.b) * f),
                         static_cast<int>(a.color.a + (b.color.a - a.color.a) * f));
        }
    }
    return m_stops.back().color;
}

Color Gradient::at(const PointF& p) const
{
    double t = 0.0;
    if (m_kind == Kind::Linear) {
        const double dx = m_to.x - m_from.x;
        const double dy = m_to.y - m_from.y;
        const double len2 = dx * dx + dy * dy;
        t = len2 > 0.0 ? ((p.x - m_from.x) * dx + (p.y - m_from.y) * dy) / len2 : 0.0;
    } else {
        const double dx = p.x - m_center.x;
        const double dy = p.y - m_center.y;
        t = std::sqrt(dx * dx + dy * dy) / m_radius;
    }
    return at(t);
}

// ---------------------------------------------------------------------------
// Painter
// ---------------------------------------------------------------------------

bool Painter::begin(Image* target)
{
    m_target = target;
    m_mode = CompositionMode::SourceOver;
    m_opacity = 1.0;
    m_clip = Rect();
    m_transform = Transform();
    m_inverse = Transform();
    m_inverseValid = true;
    m_hasBrush = false;
    m_hasBrushGradient = false;
    m_hasPen = true; // QPainter starts with a 1px black pen
    m_pen = Color(0, 0, 0, 255);
    return target != nullptr;
}

void Painter::end()
{
    m_target = nullptr;
}

void Painter::setOpacity(double opacity)
{
    m_opacity = std::min(1.0, std::max(0.0, opacity));
}

void Painter::setBrush(const Color& color)
{
    m_hasBrush = true;
    m_hasBrushGradient = false;
    m_brushColor = color;
    m_brushGradient = Gradient();
}

void Painter::setBrush(const Gradient& gradient)
{
    m_hasBrush = true;
    m_hasBrushGradient = true;
    m_brushGradient = gradient;
}

const Transform& Painter::inverse()
{
    if (!m_inverseValid) {
        m_inverse = m_transform.inverted();
        m_inverseValid = true;
    }
    return m_inverse;
}

Rect Painter::deviceBounds(const RectF& pb) const
{
    const PointF c0 = m_transform.map(pb.topLeft());
    const PointF c1 = m_transform.map(PointF(pb.x + pb.w, pb.y));
    const PointF c2 = m_transform.map(PointF(pb.x, pb.y + pb.h));
    const PointF c3 = m_transform.map(PointF(pb.x + pb.w, pb.y + pb.h));
    const double minX = std::min(std::min(c0.x, c1.x), std::min(c2.x, c3.x));
    const double maxX = std::max(std::max(c0.x, c1.x), std::max(c2.x, c3.x));
    const double minY = std::min(std::min(c0.y, c1.y), std::min(c2.y, c3.y));
    const double maxY = std::max(std::max(c0.y, c1.y), std::max(c2.y, c3.y));

    const int bx = static_cast<int>(std::floor(minX)) - 1;
    const int by = static_cast<int>(std::floor(minY)) - 1;
    const int bw = static_cast<int>(std::ceil(maxX)) - static_cast<int>(std::floor(minX)) + 3;
    const int bh = static_cast<int>(std::ceil(maxY)) - static_cast<int>(std::floor(minY)) + 3;

    Rect limit = m_target->rect();
    if (!m_clip.isNull()) limit = limit.intersected(m_clip);
    return Rect(bx, by, bw, bh).intersected(limit);
}

void Painter::blend(int x, int y, const Color& color, double alphaScale)
{
    if (!m_target || alphaScale <= 0.0) return;
    if (!m_target->contains(x, y)) return;
    if (!m_clip.isNull() && !m_clip.contains(x, y)) return;

    const double sa = (color.a / 255.0) * alphaScale;
    const Color dc = m_target->colorAt(x, y);
    const double da = dc.a / 255.0;

    const double sr = color.r * sa;
    const double sg = color.g * sa;
    const double sb = color.b * sa;
    const double dr = dc.r * da;
    const double dg = dc.g * da;
    const double db = dc.b * da;

    double orr = 0, og = 0, ob = 0, oa = 0;
    switch (m_mode) {
    case CompositionMode::SourceOver:
        oa = sa + da * (1.0 - sa);
        orr = sr + dr * (1.0 - sa);
        og = sg + dg * (1.0 - sa);
        ob = sb + db * (1.0 - sa);
        break;
    case CompositionMode::Source:
        oa = sa;
        orr = sr;
        og = sg;
        ob = sb;
        break;
    case CompositionMode::Clear:
        oa = 0;
        orr = og = ob = 0;
        break;
    case CompositionMode::DestinationIn:
        oa = da * sa;
        orr = dr * sa;
        og = dg * sa;
        ob = db * sa;
        break;
    case CompositionMode::Lighten:
        oa = std::max(sa, da);
        orr = std::max(sr, dr);
        og = std::max(sg, dg);
        ob = std::max(sb, db);
        break;
    case CompositionMode::Darken:
        oa = std::min(sa, da);
        orr = std::min(sr, dr);
        og = std::min(sg, dg);
        ob = std::min(sb, db);
        break;
    }

    if (oa <= 0.0) {
        m_target->setPixel(x, y, Color::transparent().rgba());
        return;
    }
    const int r = static_cast<int>(std::min(255.0, orr / oa + 0.5));
    const int g = static_cast<int>(std::min(255.0, og / oa + 0.5));
    const int b = static_cast<int>(std::min(255.0, ob / oa + 0.5));
    const int a = static_cast<int>(std::min(255.0, oa * 255.0 + 0.5));
    m_target->setPixel(x, y, Color(r, g, b, a).rgba());
}

void Painter::rasterizeShape(const ShapeFn& inside, const RectF& painterBounds, int samples,
                             const Color* solid, const Gradient* gradient)
{
    if (!m_target || (!solid && !gradient)) return;
    const Rect dev = deviceBounds(painterBounds);
    if (dev.isEmpty()) return;
    const Transform& inv = inverse();
    const double step = 1.0 / samples;
    const double coverageScale = 1.0 / (samples * samples);

    for (int y = dev.y; y < dev.y + dev.h; ++y) {
        for (int x = dev.x; x < dev.x + dev.w; ++x) {
            int hits = 0;
            for (int sy = 0; sy < samples; ++sy) {
                for (int sx = 0; sx < samples; ++sx) {
                    const PointF d(x + (sx + 0.5) * step, y + (sy + 0.5) * step);
                    if (inside(inv.map(d))) ++hits;
                }
            }
            if (hits == 0) continue;
            const Color c = gradient ? gradient->at(inv.map(PointF(x + 0.5, y + 0.5))) : *solid;
            blend(x, y, c, hits * coverageScale * m_opacity);
        }
    }
}

void Painter::fillRect(const RectF& rect, const Color& color)
{
    if (!m_target || rect.isEmpty()) return;
    const auto inside = [rect](const PointF& p) {
        return p.x >= rect.x && p.y >= rect.y && p.x < rect.x + rect.w && p.y < rect.y + rect.h;
    };
    const int samples = m_transform.isIdentity() ? 1 : 4;
    rasterizeShape(inside, rect, samples, &color, nullptr);
}

void Painter::fillRect(const RectF& rect, const Gradient& gradient)
{
    if (!m_target || rect.isEmpty()) return;
    const auto inside = [rect](const PointF& p) {
        return p.x >= rect.x && p.y >= rect.y && p.x < rect.x + rect.w && p.y < rect.y + rect.h;
    };
    const int samples = m_transform.isIdentity() ? 1 : 4;
    rasterizeShape(inside, rect, samples, nullptr, &gradient);
}

void Painter::drawRect(const RectF& rect)
{
    if (!m_target || rect.isEmpty()) return;
    if (m_hasBrush) {
        const auto inside = [rect](const PointF& p) {
            return p.x >= rect.x && p.y >= rect.y && p.x < rect.x + rect.w &&
                   p.y < rect.y + rect.h;
        };
        const int samples = m_transform.isIdentity() ? 1 : 4;
        rasterizeShape(inside, rect, samples, &m_brushColor,
                       m_hasBrushGradient ? &m_brushGradient : nullptr);
    }
    if (m_hasPen) {
        const auto outline = [rect](const PointF& p) {
            const double e = 0.5;
            const bool outer = p.x >= rect.x - e && p.y >= rect.y - e &&
                               p.x < rect.x + rect.w + e && p.y < rect.y + rect.h + e;
            const bool inner = p.x >= rect.x + e && p.y >= rect.y + e &&
                               p.x < rect.x + rect.w - e && p.y < rect.y + rect.h - e;
            return outer && !inner;
        };
        const RectF expanded(rect.x - 1, rect.y - 1, rect.w + 2, rect.h + 2);
        rasterizeShape(outline, expanded, 4, &m_pen, nullptr);
    }
}

void Painter::drawEllipse(const RectF& rect)
{
    if (!m_target || rect.isEmpty()) return;
    const double cx = rect.x + rect.w * 0.5;
    const double cy = rect.y + rect.h * 0.5;
    const double rx = rect.w * 0.5;
    const double ry = rect.h * 0.5;
    if (rx <= 0.0 || ry <= 0.0) return;

    if (m_hasBrush) {
        const auto inside = [=](const PointF& p) {
            const double dx = (p.x - cx) / rx;
            const double dy = (p.y - cy) / ry;
            return dx * dx + dy * dy <= 1.0;
        };
        rasterizeShape(inside, rect, 4, &m_brushColor,
                       m_hasBrushGradient ? &m_brushGradient : nullptr);
    }
    if (m_hasPen) {
        const double half = 0.5 / std::max(rx, ry);
        const auto outline = [=](const PointF& p) {
            const double dx = (p.x - cx) / rx;
            const double dy = (p.y - cy) / ry;
            const double d = std::sqrt(dx * dx + dy * dy);
            return std::fabs(d - 1.0) <= half;
        };
        const RectF expanded(rect.x - 1, rect.y - 1, rect.w + 2, rect.h + 2);
        rasterizeShape(outline, expanded, 4, &m_pen, nullptr);
    }
}

void Painter::drawImage(const Point& pos, const Image& source)
{
    drawImage(RectF(pos.x, pos.y, source.width(), source.height()), source);
}

void Painter::drawImage(const Rect& target, const Image& source)
{
    drawImage(RectF(target.x, target.y, target.w, target.h), source);
}

void Painter::drawImage(const Rect& target, const Image& source, const Rect& sourceRect)
{
    drawImage(RectF(target.x, target.y, target.w, target.h), source.copy(sourceRect));
}

void Painter::drawImage(const RectF& target, const Image& source)
{
    if (!m_target || source.isNull() || target.isEmpty()) return;
    const Rect dev = deviceBounds(target);
    if (dev.isEmpty()) return;
    const Transform& inv = inverse();
    const double sw = source.width();
    const double sh = source.height();

    auto samplePremul = [&](int i, int j, double out[4]) {
        i = std::max(0, std::min(i, source.width() - 1));
        j = std::max(0, std::min(j, source.height() - 1));
        const Color c = source.colorAt(i, j);
        const double a = c.a / 255.0;
        out[0] = c.r * a;
        out[1] = c.g * a;
        out[2] = c.b * a;
        out[3] = c.a;
    };

    for (int y = dev.y; y < dev.y + dev.h; ++y) {
        for (int x = dev.x; x < dev.x + dev.w; ++x) {
            const PointF p = inv.map(PointF(x + 0.5, y + 0.5));
            const double cu = (p.x - target.x) / target.w * sw - 0.5;
            const double cv = (p.y - target.y) / target.h * sh - 0.5;
            if (cu < -0.5 || cv < -0.5 || cu > sw - 0.5 || cv > sh - 0.5) continue;

            const int i0 = static_cast<int>(std::floor(cu));
            const int j0 = static_cast<int>(std::floor(cv));
            const double fx = cu - i0;
            const double fy = cv - j0;

            double a00[4], a10[4], a01[4], a11[4];
            samplePremul(i0, j0, a00);
            samplePremul(i0 + 1, j0, a10);
            samplePremul(i0, j0 + 1, a01);
            samplePremul(i0 + 1, j0 + 1, a11);

            const double w00 = (1.0 - fx) * (1.0 - fy);
            const double w10 = fx * (1.0 - fy);
            const double w01 = (1.0 - fx) * fy;
            const double w11 = fx * fy;
            const double pr = a00[0] * w00 + a10[0] * w10 + a01[0] * w01 + a11[0] * w11;
            const double pg = a00[1] * w00 + a10[1] * w10 + a01[1] * w01 + a11[1] * w11;
            const double pb = a00[2] * w00 + a10[2] * w10 + a01[2] * w01 + a11[2] * w11;
            const double pa = a00[3] * w00 + a10[3] * w10 + a01[3] * w01 + a11[3] * w11;
            if (pa <= 0.0) continue;

            const Color sample(static_cast<int>(std::min(255.0, pr * 255.0 / pa + 0.5)),
                               static_cast<int>(std::min(255.0, pg * 255.0 / pa + 0.5)),
                               static_cast<int>(std::min(255.0, pb * 255.0 / pa + 0.5)),
                               static_cast<int>(std::min(255.0, pa + 0.5)));
            blend(x, y, sample, m_opacity);
        }
    }
}

} // namespace ks
