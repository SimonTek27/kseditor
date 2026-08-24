#pragma once

#include <QImage>
#include <QPoint>
#include <QColor>
#include <QVariantMap>
#include "PaintTypes.h"

namespace ks {
namespace paint {

struct PaintBrush {
    PaintTool tool = PaintTool::Brush;
    float radius = 20.0f;
    float hardness = 0.5f;
    float opacity = 1.0f;
    float strength = 1.0f;
    float flow = 1.0f;
    float pressure = 1.0f;
    QColor color = Qt::black;
    QColor secondaryColor = Qt::white;
    QPoint cloneSource;
    bool hasCloneSource = false;
    QImage stampTexture;
    bool eraseAlpha = false; // for eraser
    StrokeType strokeType = StrokeType::Freehand;
    float sprayScatter = 0.5f; // scatter amount for Spray mode
    int sprayDensity = 10;     // dots per stamp for Spray mode
};

class PaintPainter {
public:
    // Applies a brush dab (paintAt) or line (paintLine) to image, honoring mask (selection).
    static void paintAt(QImage& image, const QImage& mask, const QPoint& pos, const PaintBrush& brush);
    static void paintLine(QImage& image, const QImage& mask, const QPoint& from, const QPoint& to, const PaintBrush& brush);

    // DragRect mode: draws a rectangle from start to end
    static void paintRect(QImage& image, const QImage& mask, const QPoint& from, const QPoint& to, const PaintBrush& brush, bool filled = false);

    // Filter strokes
    static void blurAt(QImage& image, const QImage& mask, const QPoint& pos, float radius, float strength);
    static void sharpenAt(QImage& image, const QImage& mask, const QPoint& pos, float radius, float strength);
    static void dodgeAt(QImage& image, const QImage& mask, const QPoint& pos, float radius, float strength);
    static void burnAt(QImage& image, const QImage& mask, const QPoint& pos, float radius, float strength);
    static void smudgeAt(QImage& image, const QImage& mask, const QPoint& from, const QPoint& to, float radius, float strength);
    static void cloneAt(QImage& image, const QImage& mask, const QPoint& pos, const QPoint& source, float radius, float opacity);
    static void healAt(QImage& image, const QImage& mask, const QPoint& pos, const QPoint& source, float radius, float opacity);

    // Full-canvas fills
    static void fill(QImage& image, const QImage& mask, const QPoint& start, const QColor& color, float tolerance);
    static void gradient(QImage& image, const QImage& mask,
                         const QPoint& from, const QPoint& to,
                         const QColor& startColor, const QColor& endColor, bool radial);

    // Image-level operations (filters for whole layer / selection)
    static QImage applyFilter(const QImage& src, const QString& filter, const QVariantMap& params = {});

    // Utils
    static QPoint clampTo(const QPoint& p, const QSize& size);
    static bool inside(const QPoint& p, const QSize& size);

private:
    // Helper for single brush dab rendering (used by paintAt and Spray mode)
    static void paintSingleDab(QImage& image, const QImage& mask, const QPoint& pos, const PaintBrush& brush,
                               float effRadius, float effOpacity, int innerR, bool eraser, bool airbrush, float pressure);
};

} // namespace paint
} // namespace ks