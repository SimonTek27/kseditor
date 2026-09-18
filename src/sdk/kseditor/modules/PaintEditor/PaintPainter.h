// tools_PaintPainter.h
#pragma once

#include <QObject>
#include <QWidget>
#include <QImage>
#include <QColor>
#include <QPoint>
#include <QVector>
#include <QRect>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QJsonObject>
#include <QJsonArray>

namespace ks {

struct PaintBrushConfig {
    enum Type {
        Brush,
        Airbrush,
        SquareBrush,
        Eraser,
        Smudge,
        Blur,
        Sharpen,
        Clone,
        Healing,
        Dodge,
        Burn,
        Fill,
        Gradient,
        Stamp
    };

    Type type = Brush;
    float radius = 20.0f;
    float hardness = 0.5f;
    float opacity = 1.0f;
    float strength = 1.0f;
    float spacing = 0.25f;
    float flow = 1.0f;
    float angle = 0.0f;
    float jitter = 0.0f;
    QColor color = Qt::red;
    QColor secondaryColor = Qt::white;
    QPoint cloneSource;
    bool useCloneImage = false;
    QImage stampTexture;

    bool isCloneOrHealing() const { return type == Clone || type == Healing; }
    bool isShapeBrush() const { return type == Brush || type == Airbrush || type == SquareBrush; }
};

struct UVCoord {
    float u, v;
};

class PaintPainter : public QObject {
    Q_OBJECT
    
public:
    explicit PaintPainter(QObject* parent = nullptr);
    ~PaintPainter();

    bool startPainting(int textureWidth, int textureHeight);
    void stopPainting();
    
    void paintAt(const QPoint& screenPos, const PaintBrushConfig& brush);
    void paintLine(const QPoint& from, const QPoint& to, const PaintBrushConfig& brush);
    void fillArea(const QPoint& screenPos, const QColor& color);
    void gradientFill(const QPoint& from, const QPoint& to,
                      const QColor& startColor, const QColor& endColor, bool radial = false);
    void applyBlurAt(const QPoint& center, int radius, float strength);
    void applySharpenAt(const QPoint& center, int radius, float strength);
    void applyDodgeAt(const QPoint& center, int radius, float strength);
    void applyBurnAt(const QPoint& center, int radius, float strength);
    void smudgeAt(const QPoint& from, const QPoint& to, int radius, float strength);
    void applyHealingAt(const QPoint& target, const QPoint& source, int radius);
    void applyStamp(const QPoint& pos, const QImage& stamp, float opacity = 1.0f);
    
    QImage getCurrentTexture() const { return m_texture; }
    void setTexture(const QImage& texture);
    bool saveTexture(const QString& path);
    void reloadTexture();
    void clearTexture();
    
    void setMask(const QImage& mask);
    QImage getMask() const { return m_mask; }

    void setCloneImage(const QImage& image);
    QImage getCloneImage() const { return m_cloneImage; }

signals:
    void textureUpdated(const QImage& texture);
    void paintingStarted();
    void paintingStopped();

private:
    QImage m_texture;
    QImage m_originalTexture;
    QImage m_mask;
    QImage m_cloneImage;
    bool m_isPainting = false;
    int m_width = 0;
    int m_height = 0;
    
    void applyBrushAtPixel(const QPoint& pixel, const PaintBrushConfig& brush);
    void applyAirbrushAtPixel(const QPoint& pixel, const PaintBrushConfig& brush);
    void applySmudgeAtPixel(const QPoint& from, const QPoint& to, const PaintBrushConfig& brush);
    void applyBlurAtPixel(const QPoint& pixel, const PaintBrushConfig& brush);
    void applySharpenAtPixel(const QPoint& pixel, const PaintBrushConfig& brush);
    void applyDodgeAtPixel(const QPoint& pixel, const PaintBrushConfig& brush);
    void applyBurnAtPixel(const QPoint& pixel, const PaintBrushConfig& brush);
    void applyCloneAtPixel(const QPoint& pixel, const PaintBrushConfig& brush);
    void applyHealingAtPixel(const QPoint& pixel, const QPoint& source, const PaintBrushConfig& brush);
    QRgb blendPixel(QRgb base, QRgb brush, float alpha);
    QRgb colorDodge(QRgb base, float strength);
    QRgb colorBurn(QRgb base, float strength);
    void floodFill(const QPoint& start, const QColor& fillColor, float tolerance);
    QVector<QPoint> getNeighbors(const QPoint& p);
};

class PaintPreset {
public:
    QString name;
    QVector<QColor> colors;
    QVector<QRect> regions;
    QJsonObject toJson() const;
    static PaintPreset fromJson(const QJsonObject& json);
};

class PaintPainterWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit PaintPainterWidget(QWidget* parent = nullptr);
    ~PaintPainterWidget();
    
    void setTexture(const QImage& texture);
    void setBrush(const PaintBrushConfig& brush);
    void setMask(const QImage& mask);
    QImage getTexture() const { return m_painter.getCurrentTexture(); }
    
    void loadPreset(const PaintPreset& preset);
    PaintPreset savePreset(const QString& name) const;

    void setCloneSource(const QPoint& pos) { m_cloneSource = pos; }
    QPoint cloneSource() const { return m_cloneSource; }
    void setCloneImage(const QImage& image) { m_painter.setCloneImage(image); }

public slots:
    void onColorSelected(const QColor& color);
    void onBrushSizeChanged(int size);
    void onBrushHardnessChanged(double hardness);

signals:
    void textureChanged(const QImage& texture);
    void brushChanged(const PaintBrushConfig& brush);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    PaintPainter m_painter;
    PaintBrushConfig m_currentBrush;
    QPoint m_lastPos;
    QPoint m_cloneSource;
    bool m_isDrawing = false;
    
    void updateBrushPreview();
};

} // namespace ks