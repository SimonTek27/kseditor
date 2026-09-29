#pragma once

// Qt-free painter for ks::Image (the QPainter replacement used by the
// material module).
//
// What is implemented, and how it maps to QPainter:
//   setCompositionMode  -> SourceOver / Source / Clear / DestinationIn /
//                          Lighten / Darken (composite on premultiplied
//                          values, like Qt does, then store straight alpha)
//   setOpacity          -> multiplies the source alpha
//   setClipRect         -> clip in device space (reset by end())
//   setTransform        -> 2D affine transform; ops append, so
//                          translate/rotate/scale then draw behaves like the
//                          usual Qt pattern (transform about the origin
//                          chain, image placed last)
//   setBrush / setPen   -> fill colour or gradient; pen is a 1px stroke,
//                          default black like QPainter, disable with
//                          setNoPen()
//   drawImage           -> nearest/bilinear sampling, hard pixel edges
//                          (Qt's SmoothPixmapTransform behaviour is used
//                          whenever a transform is active)
//   drawRect/ellipse    -> 4x4 supersampled coverage, so dropping
//                          setRenderHint(Antialiasing) does not change the
//                          look of stamps and brush masks
//   setRenderHint       -> not present; hints were no-ops for this module

#include "Image.h"
#include "MaterialTypes.h"
#include "Transform.h"

#include <functional>

namespace ks {

enum class CompositionMode {
    SourceOver,
    Source,
    Clear,
    DestinationIn,
    Lighten,
    Darken
};

class Gradient {
public:
    enum class Kind { Linear, Radial };

    static Gradient linear(const PointF& from, const PointF& to);
    static Gradient radial(const PointF& center, double radius);

    void setColorAt(double pos, const Color& color);

    Kind kind() const { return m_kind; }
    const PointF& from() const { return m_from; }
    const PointF& to() const { return m_to; }
    double radius() const { return m_radius; }

    // Colour at normalised position 0..1 along the gradient.
    Color at(double t) const;
    // Colour at a point in painter space.
    Color at(const PointF& p) const;

private:
    struct Stop {
        double pos;
        Color color;
    };

    Kind m_kind = Kind::Linear;
    PointF m_from;
    PointF m_to;
    PointF m_center;
    double m_radius = 1.0;
    std::vector<Stop> m_stops;
};

class Painter {
public:
    Painter() = default;
    explicit Painter(Image* target) { begin(target); }

    bool begin(Image* target);
    void end();
    bool isActive() const { return m_target != nullptr; }

    void setCompositionMode(CompositionMode mode) { m_mode = mode; }
    CompositionMode compositionMode() const { return m_mode; }

    void setOpacity(double opacity);
    double opacity() const { return m_opacity; }

    void setClipRect(const Rect& rect) { m_clip = rect; }
    void resetClip() { m_clip = Rect(); }
    Rect clipRect() const { return m_clip; }

    void setTransform(const Transform& t) { m_transform = t; m_inverseValid = false; }
    void resetTransform() { m_transform = Transform(); m_inverse = Transform(); m_inverseValid = true; }
    const Transform& transform() const { return m_transform; }

    void setBrush(const Color& color);
    void setBrush(const Gradient& gradient);
    void clearBrush() { m_hasBrush = false; m_hasBrushGradient = false; }

    void setPen(const Color& color) { m_pen = color; m_hasPen = true; }
    void setNoPen() { m_hasPen = false; }

    void drawImage(int x, int y, const Image& source) { drawImage(Point(x, y), source); }
    void drawImage(const Point& pos, const Image& source);
    void drawImage(const RectF& target, const Image& source);
    void drawImage(const Rect& target, const Image& source);
    // Scaled sub-rect blit (QPainter::drawImage(target, image, sourceRect)).
    void drawImage(const Rect& target, const Image& source, const Rect& sourceRect);

    void drawRect(const Rect& rect) { drawRect(RectF(rect.x, rect.y, rect.w, rect.h)); }
    void drawRect(const RectF& rect);

    void drawEllipse(const Rect& rect) { drawEllipse(RectF(rect.x, rect.y, rect.w, rect.h)); }
    void drawEllipse(const RectF& rect);
    void drawEllipse(const PointF& center, double rx, double ry)
    {
        drawEllipse(RectF(center.x - rx, center.y - ry, rx * 2, ry * 2));
    }

    void fillRect(const Rect& rect, const Color& color) { fillRect(RectF(rect.x, rect.y, rect.w, rect.h), color); }
    void fillRect(const RectF& rect, const Color& color);
    void fillRect(const Rect& rect, const Gradient& gradient) { fillRect(RectF(rect.x, rect.y, rect.w, rect.h), gradient); }
    void fillRect(const RectF& rect, const Gradient& gradient);

private:
    using ShapeFn = std::function<bool(const PointF&)>;

    void rasterizeShape(const ShapeFn& inside, const RectF& painterBounds, int samples,
                        const Color* solid, const Gradient* gradient);
    void blend(int x, int y, const Color& color, double alphaScale);
    Rect deviceBounds(const RectF& painterBounds) const;
    const Transform& inverse();

    Image* m_target = nullptr;
    CompositionMode m_mode = CompositionMode::SourceOver;
    double m_opacity = 1.0;
    Rect m_clip;
    Transform m_transform;
    Transform m_inverse;
    bool m_inverseValid = true;
    bool m_hasBrush = false;
    bool m_hasBrushGradient = false;
    Color m_brushColor;
    Gradient m_brushGradient;
    bool m_hasPen = false;
    Color m_pen{0, 0, 0, 255};
};

} // namespace ks
