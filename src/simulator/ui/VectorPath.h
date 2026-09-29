#pragma once
/**
 * Immediate-mode vector paths for UI overlays (Qt-free).
 * Tessellates lines, polylines, cubic/quadratic beziers, arcs into
 * triangle strips (stroke) or triangle fans (convex fill).
 */
#include "NativeUiTypes.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace ks {
namespace sim {
namespace ui {

struct Vec2 {
    float x = 0, y = 0;
    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}
    Vec2 operator+(Vec2 o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(Vec2 o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    float length() const { return std::sqrt(x * x + y * y); }
    Vec2 normalized() const {
        float l = length();
        return l > 1e-6f ? Vec2{x / l, y / l} : Vec2{1, 0};
    }
};

enum class PathCmd : uint8_t {
    MoveTo,
    LineTo,
    QuadTo,   // c1, end
    CubicTo,  // c1, c2, end
    Close
};

struct PathVerb {
    PathCmd cmd = PathCmd::MoveTo;
    Vec2 p0, p1, p2; // endpoints / controls depending on cmd
};

struct StrokeStyle {
    Color color = Color::rgb(255, 255, 255);
    float width = 2.f;
    bool closed = false;
};

struct FillStyle {
    Color color = Color::rgba(1, 1, 1, 0.5f);
};

class VectorPath {
public:
    void clear() {
        m_verbs.clear();
        m_hasPoint = false;
    }

    void moveTo(float x, float y) {
        PathVerb v;
        v.cmd = PathCmd::MoveTo;
        v.p0 = {x, y};
        m_verbs.push_back(v);
        m_cursor = {x, y};
        m_hasPoint = true;
    }

    void lineTo(float x, float y) {
        if (!m_hasPoint) { moveTo(x, y); return; }
        PathVerb v;
        v.cmd = PathCmd::LineTo;
        v.p0 = {x, y};
        m_verbs.push_back(v);
        m_cursor = {x, y};
    }

    void quadTo(float cx, float cy, float x, float y) {
        if (!m_hasPoint) moveTo(x, y);
        PathVerb v;
        v.cmd = PathCmd::QuadTo;
        v.p0 = {cx, cy};
        v.p1 = {x, y};
        m_verbs.push_back(v);
        m_cursor = {x, y};
    }

    void cubicTo(float c1x, float c1y, float c2x, float c2y, float x, float y) {
        if (!m_hasPoint) moveTo(x, y);
        PathVerb v;
        v.cmd = PathCmd::CubicTo;
        v.p0 = {c1x, c1y};
        v.p1 = {c2x, c2y};
        v.p2 = {x, y};
        m_verbs.push_back(v);
        m_cursor = {x, y};
    }

    /** Arc from current point; angles in degrees, CCW positive. */
    void arcTo(float cx, float cy, float radius, float startDeg, float endDeg, int segments = 0) {
        if (radius < 1e-4f) return;
        if (segments <= 0) {
            float sweep = std::fabs(endDeg - startDeg);
            segments = std::max(4, static_cast<int>(sweep / 6.f));
        }
        const float s0 = startDeg * 0.01745329251f;
        const float s1 = endDeg * 0.01745329251f;
        for (int i = 0; i <= segments; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(segments);
            float a = s0 + (s1 - s0) * t;
            float x = cx + std::cos(a) * radius;
            float y = cy + std::sin(a) * radius;
            if (i == 0 && !m_hasPoint) moveTo(x, y);
            else lineTo(x, y);
        }
    }

    void close() {
        PathVerb v;
        v.cmd = PathCmd::Close;
        m_verbs.push_back(v);
    }

    /** Rounded rectangle as path. */
    void addRoundRect(float x, float y, float w, float h, float r) {
        r = std::min(r, std::min(w, h) * 0.5f);
        moveTo(x + r, y);
        lineTo(x + w - r, y);
        arcTo(x + w - r, y + r, r, -90.f, 0.f, 8);
        lineTo(x + w, y + h - r);
        arcTo(x + w - r, y + h - r, r, 0.f, 90.f, 8);
        lineTo(x + r, y + h);
        arcTo(x + r, y + h - r, r, 90.f, 180.f, 8);
        lineTo(x, y + r);
        arcTo(x + r, y + r, r, 180.f, 270.f, 8);
        close();
    }

    void addCircle(float cx, float cy, float radius, int segments = 32) {
        arcTo(cx, cy, radius, 0.f, 360.f, segments);
        close();
    }

    /** Flatten path to polyline points (tessellated curves). */
    std::vector<Vec2> flatten(float curveTol = 0.5f) const {
        std::vector<Vec2> pts;
        Vec2 start{}, cur{};
        bool have = false;
        for (const auto& v : m_verbs) {
            switch (v.cmd) {
            case PathCmd::MoveTo:
                cur = v.p0;
                start = cur;
                pts.push_back(cur);
                have = true;
                break;
            case PathCmd::LineTo:
                cur = v.p0;
                pts.push_back(cur);
                break;
            case PathCmd::QuadTo:
                flattenQuad(cur, v.p0, v.p1, curveTol, pts);
                cur = v.p1;
                break;
            case PathCmd::CubicTo:
                flattenCubic(cur, v.p0, v.p1, v.p2, curveTol, pts);
                cur = v.p2;
                break;
            case PathCmd::Close:
                if (have && (pts.empty() || pts.back().x != start.x || pts.back().y != start.y))
                    pts.push_back(start);
                cur = start;
                break;
            }
        }
        return pts;
    }

    /** Stroke flattened polyline as triangle strip into UI buffers. */
    void stroke(const StrokeStyle& style,
                std::vector<UiVertex>& verts, std::vector<uint32_t>& inds,
                float u0, float v0, float u1, float v1) const {
        auto pts = flatten();
        if (pts.size() < 2) return;
        if (style.closed && (pts.front().x != pts.back().x || pts.front().y != pts.back().y))
            pts.push_back(pts.front());

        const float half = style.width * 0.5f;
        const Color& c = style.color;

        for (size_t i = 0; i + 1 < pts.size(); ++i) {
            Vec2 a = pts[i];
            Vec2 b = pts[i + 1];
            Vec2 d = (b - a).normalized();
            Vec2 n{-d.y * half, d.x * half};

            // Join: average with previous segment normal for smoother corners
            if (i > 0) {
                Vec2 prev = pts[i - 1];
                Vec2 d0 = (a - prev).normalized();
                Vec2 n0{-d0.y * half, d0.x * half};
                Vec2 avg = n0 + n;
                float al = avg.length();
                if (al > 1e-4f) n = avg * (half / (al * 0.5f)); // miter-ish clamp later
                // simple clamp miter length
                if (n.length() > half * 4.f)
                    n = n.normalized() * (half * 4.f);
            }

            const uint32_t base = static_cast<uint32_t>(verts.size());
            auto push = [&](float x, float y) {
                verts.push_back({x, y, c.r, c.g, c.b, c.a, u0, v0});
            };
            push(a.x + n.x, a.y + n.y);
            push(a.x - n.x, a.y - n.y);
            push(b.x + n.x, b.y + n.y);
            push(b.x - n.x, b.y - n.y);
            // two triangles
            inds.push_back(base + 0);
            inds.push_back(base + 1);
            inds.push_back(base + 2);
            inds.push_back(base + 1);
            inds.push_back(base + 3);
            inds.push_back(base + 2);
            (void)u1; (void)v1;
        }
    }

    /** Convex fill via triangle fan (path should be closed & roughly convex). */
    void fillConvex(const FillStyle& style,
                    std::vector<UiVertex>& verts, std::vector<uint32_t>& inds,
                    float u0, float v0, float u1, float v1) const {
        auto pts = flatten();
        if (pts.size() < 3) return;
        // remove duplicate close point
        if (pts.size() > 1 && pts.front().x == pts.back().x && pts.front().y == pts.back().y)
            pts.pop_back();
        if (pts.size() < 3) return;

        const Color& c = style.color;
        const uint32_t base = static_cast<uint32_t>(verts.size());
        for (const auto& p : pts)
            verts.push_back({p.x, p.y, c.r, c.g, c.b, c.a, u0, v0});
        for (uint32_t i = 1; i + 1 < static_cast<uint32_t>(pts.size()); ++i) {
            inds.push_back(base + 0);
            inds.push_back(base + i);
            inds.push_back(base + i + 1);
        }
        (void)u1; (void)v1;
    }

    const std::vector<PathVerb>& verbs() const { return m_verbs; }

private:
    static void flattenQuad(Vec2 p0, Vec2 p1, Vec2 p2, float tol, std::vector<Vec2>& out) {
        // recursive subdivision
        struct Frame { Vec2 a, b, c; };
        std::vector<Frame> stack{{p0, p1, p2}};
        while (!stack.empty()) {
            Frame f = stack.back();
            stack.pop_back();
            Vec2 mid = (f.a + f.c) * 0.5f;
            Vec2 ctrl = f.b;
            float dx = ctrl.x - mid.x, dy = ctrl.y - mid.y;
            if (dx * dx + dy * dy <= tol * tol) {
                out.push_back(f.c);
            } else {
                Vec2 ab = (f.a + f.b) * 0.5f;
                Vec2 bc = (f.b + f.c) * 0.5f;
                Vec2 abc = (ab + bc) * 0.5f;
                stack.push_back({abc, bc, f.c});
                stack.push_back({f.a, ab, abc});
            }
        }
    }

    static void flattenCubic(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, float tol, std::vector<Vec2>& out) {
        struct Frame { Vec2 a, b, c, d; };
        std::vector<Frame> stack{{p0, p1, p2, p3}};
        while (!stack.empty()) {
            Frame f = stack.back();
            stack.pop_back();
            // flatness: distance of controls from chord
            auto dist = [](Vec2 a, Vec2 b, Vec2 p) {
                float dx = b.x - a.x, dy = b.y - a.y;
                float len2 = dx * dx + dy * dy;
                if (len2 < 1e-8f) return (p - a).length();
                float t = std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / len2, 0.f, 1.f);
                float qx = a.x + t * dx, qy = a.y + t * dy;
                return std::sqrt((p.x - qx) * (p.x - qx) + (p.y - qy) * (p.y - qy));
            };
            if (dist(f.a, f.d, f.b) <= tol && dist(f.a, f.d, f.c) <= tol) {
                out.push_back(f.d);
            } else {
                Vec2 ab = (f.a + f.b) * 0.5f;
                Vec2 bc = (f.b + f.c) * 0.5f;
                Vec2 cd = (f.c + f.d) * 0.5f;
                Vec2 abc = (ab + bc) * 0.5f;
                Vec2 bcd = (bc + cd) * 0.5f;
                Vec2 abcd = (abc + bcd) * 0.5f;
                stack.push_back({abcd, bcd, cd, f.d});
                stack.push_back({f.a, ab, abc, abcd});
            }
        }
    }

    std::vector<PathVerb> m_verbs;
    Vec2 m_cursor{};
    bool m_hasPoint = false;
};

} // namespace ui
} // namespace sim
} // namespace ks
