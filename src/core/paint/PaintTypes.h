#pragma once

#include <QString>
#include <QColor>
#include <QImage>
#include <QVector>

namespace ks {
namespace paint {

// Paint tool set
enum class PaintTool {
    Move,
    Zoom,
    Pan,
    RectSelect,
    EllipseSelect,
    FreeSelect,
    FuzzySelect,
    ColorPicker,
    Brush,
    Pencil,
    Eraser,
    Airbrush,
    Smudge,
    Blur,
    Sharpen,
    Dodge,
    Burn,
    Clone,
    Healing,
    BucketFill,
    Gradient,
    Text
};

enum class PaintBlendMode {
    Normal,
    Multiply,
    Screen,
    Overlay,
    Darken,
    Lighten,
    ColorDodge,
    ColorBurn,
    HardLight,
    SoftLight,
    Difference,
    Exclusion
};

inline QString paintToolDisplayName(PaintTool tool)
{
    switch (tool) {
    case PaintTool::Move:        return QStringLiteral("Move");
    case PaintTool::Zoom:        return QStringLiteral("Zoom");
    case PaintTool::Pan:         return QStringLiteral("Pan");
    case PaintTool::RectSelect:  return QStringLiteral("Rectangle Select");
    case PaintTool::EllipseSelect: return QStringLiteral("Ellipse Select");
    case PaintTool::FreeSelect:  return QStringLiteral("Free Select");
    case PaintTool::FuzzySelect: return QStringLiteral("Fuzzy Select");
    case PaintTool::ColorPicker: return QStringLiteral("Color Picker");
    case PaintTool::Brush:       return QStringLiteral("Brush");
    case PaintTool::Pencil:      return QStringLiteral("Pencil");
    case PaintTool::Eraser:      return QStringLiteral("Eraser");
    case PaintTool::Airbrush:    return QStringLiteral("Airbrush");
    case PaintTool::Smudge:      return QStringLiteral("Smudge");
    case PaintTool::Blur:        return QStringLiteral("Blur");
    case PaintTool::Sharpen:     return QStringLiteral("Sharpen");
    case PaintTool::Dodge:       return QStringLiteral("Dodge");
    case PaintTool::Burn:        return QStringLiteral("Burn");
    case PaintTool::Clone:       return QStringLiteral("Clone");
    case PaintTool::Healing:     return QStringLiteral("Healing");
    case PaintTool::BucketFill:  return QStringLiteral("Bucket Fill");
    case PaintTool::Gradient:    return QStringLiteral("Gradient");
    case PaintTool::Text:        return QStringLiteral("Text");
    }
    return QStringLiteral("Tool");
}

inline QString paintToolShortcut(PaintTool tool)
{
    switch (tool) {
    case PaintTool::Move:        return QStringLiteral("M");
    case PaintTool::Zoom:        return QStringLiteral("Z");
    case PaintTool::Pan:         return QStringLiteral("H");
    case PaintTool::RectSelect:  return QStringLiteral("R");
    case PaintTool::EllipseSelect: return QStringLiteral("E");
    case PaintTool::FreeSelect:  return QStringLiteral("F");
    case PaintTool::FuzzySelect: return QStringLiteral("U");
    case PaintTool::ColorPicker: return QStringLiteral("O");
    case PaintTool::Brush:       return QStringLiteral("P");
    case PaintTool::Pencil:      return QStringLiteral("N");
    case PaintTool::Eraser:      return QStringLiteral("Shift+E");
    case PaintTool::Airbrush:    return QStringLiteral("A");
    case PaintTool::Smudge:      return QStringLiteral("S");
    case PaintTool::Blur:        return QStringLiteral("Shift+U");
    case PaintTool::Sharpen:     return QStringLiteral("Shift+D");
    case PaintTool::Dodge:       return QStringLiteral("D");
    case PaintTool::Burn:        return QStringLiteral("X");
    case PaintTool::Clone:       return QStringLiteral("C");
    case PaintTool::Healing:     return QStringLiteral("J");
    case PaintTool::BucketFill:  return QStringLiteral("Shift+B");
    case PaintTool::Gradient:    return QStringLiteral("L");
    case PaintTool::Text:        return QStringLiteral("T");
    }
    return QString();
}

inline bool paintToolIsSelect(PaintTool tool)
{
    return tool == PaintTool::RectSelect || tool == PaintTool::EllipseSelect
        || tool == PaintTool::FreeSelect || tool == PaintTool::FuzzySelect;
}

inline bool paintToolIsPaint(PaintTool tool)
{
    return tool == PaintTool::Brush || tool == PaintTool::Pencil
        || tool == PaintTool::Eraser || tool == PaintTool::Airbrush
        || tool == PaintTool::Smudge || tool == PaintTool::Blur
        || tool == PaintTool::Sharpen || tool == PaintTool::Dodge
        || tool == PaintTool::Burn || tool == PaintTool::Clone
        || tool == PaintTool::Healing;
}

struct PaintLayer {
    QString name;
    QImage image;                 // ARGB32, size == document size
    QImage mask;                  // Alpha mask (grayscale), same size; null = no mask
    bool maskEnabled = true;
    bool maskLinked = true;
    int offsetX = 0;
    int offsetY = 0;
    float opacity = 1.0f;
    PaintBlendMode blend = PaintBlendMode::Normal;
    bool visible = true;
    bool locked = false;
    bool hasMask() const { return !mask.isNull(); }
};

} // namespace paint
} // namespace ks