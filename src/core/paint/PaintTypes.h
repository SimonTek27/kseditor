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
    Text,
    Pen,
    NodeEdit,
    RectShape,
    EllipseShape,
    StarShape,
    PolygonShape,
    SpiralShape,
    Box3DShape,
    Calligraphy,
    VectorSelect,
    GradientTool,
    PatternTool,
    CloneTool,
    SprayTool,
    TextOnPath,
    QuickSelection, ObjectSelection, MagicWand, ContentAwareFill, ContentAwareMove, PatchTool, HistoryBrush, ArtHistoryBrush,
    PuppetWarp, Liquify, VanishingPoint, PerspectiveWarp, CameraRaw, NeuralFilter, SkySelect, SelectSubject,
    PenPath, FreeformPen, CurvaturePen, History, ActionPlay,
    MixerBrush, PatternStamp, CloneStamp, HealingBrush, HistoryArtBrush
};
enum class StrokeType { Dots = 0, DragRect = 1, DragDot = 2, Spray = 3, Freehand = 4 };
enum class StencilWrapMode { Flat = 0, Surface = 1, Cylindrical = 2 };

enum class PaintBlendMode {
    Normal, Multiply, Screen, Overlay, Darken, Lighten, ColorDodge, ColorBurn, HardLight, SoftLight, Difference, Exclusion,
    VividLight, LinearLight, PinLight, HardMix, Divide, Subtract, DarkerColor, LighterColor, Hue, Saturation, Color, Luminosity, PassThrough, Dissolve
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
    case PaintTool::Pen:         return QStringLiteral("Pen / Bezier");
    case PaintTool::NodeEdit:    return QStringLiteral("Node Edit");
    case PaintTool::RectShape:   return QStringLiteral("Rectangle");
    case PaintTool::EllipseShape:return QStringLiteral("Ellipse");
    case PaintTool::StarShape:   return QStringLiteral("Star");
    case PaintTool::PolygonShape:return QStringLiteral("Polygon");
    case PaintTool::SpiralShape: return QStringLiteral("Spiral");
    case PaintTool::Box3DShape:  return QStringLiteral("3D Box");
    case PaintTool::Calligraphy: return QStringLiteral("Calligraphy");
    case PaintTool::VectorSelect:return QStringLiteral("Select Vectors");
    case PaintTool::GradientTool:return QStringLiteral("Gradient Edit");
    case PaintTool::PatternTool: return QStringLiteral("Pattern Fill");
    case PaintTool::CloneTool:   return QStringLiteral("Tiled Clones");
    case PaintTool::SprayTool:   return QStringLiteral("Spray Clones");
    case PaintTool::TextOnPath:  return QStringLiteral("Text on Path");
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
    case PaintTool::Pen:         return QStringLiteral("B");
    case PaintTool::NodeEdit:    return QStringLiteral("N2");
    case PaintTool::RectShape:   return QStringLiteral("Shift+R");
    case PaintTool::EllipseShape:return QStringLiteral("Shift+E");
    case PaintTool::StarShape:   return QStringLiteral("Shift+Y");
    case PaintTool::PolygonShape:return QStringLiteral("Shift+O");
    case PaintTool::SpiralShape: return QStringLiteral("Shift+I");
    case PaintTool::Box3DShape:  return QStringLiteral("Shift+X");
    case PaintTool::Calligraphy: return QStringLiteral("Shift+C");
    case PaintTool::VectorSelect:return QStringLiteral("S");
    case PaintTool::GradientTool:return QStringLiteral("G");
    case PaintTool::PatternTool: return QStringLiteral("Shift+P");
    case PaintTool::CloneTool:   return QStringLiteral("Alt+D");
    case PaintTool::SprayTool:   return QStringLiteral("Alt+S");
    case PaintTool::TextOnPath:  return QStringLiteral("Alt+T");
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