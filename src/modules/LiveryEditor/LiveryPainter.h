// tools_LiveryPainter.h
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

struct LiveryPaintBrush {
    float radius = 20.0f;
    float hardness = 0.5f;
    float opacity = 1.0f;
    QColor color = Qt::red;
    enum Type { Color, Clone, Smudge, Erase };
    Type type = Color;
};

struct UVCoord {
    float u, v;
};

class LiveryPainter : public QObject {
    Q_OBJECT
    
public:
    explicit LiveryPainter(QObject* parent = nullptr);
    ~LiveryPainter();

    bool startPainting(int textureWidth, int textureHeight);
    void stopPainting();
    
    void paintAt(const QPoint& screenPos, const LiveryPaintBrush& brush);
    void paintLine(const QPoint& from, const QPoint& to, const LiveryPaintBrush& brush);
    void fillArea(const QPoint& screenPos, const QColor& color);
    
    QImage getCurrentTexture() const { return m_texture; }
    void setTexture(const QImage& texture);
    bool saveTexture(const QString& path);
    void reloadTexture();
    void clearTexture();
    
    void setMask(const QImage& mask);
    QImage getMask() const { return m_mask; }

signals:
    void textureUpdated(const QImage& texture);
    void paintingStarted();
    void paintingStopped();

private:
    QImage m_texture;
    QImage m_originalTexture;
    QImage m_mask;
    bool m_isPainting = false;
    int m_width = 0;
    int m_height = 0;
    
    void applyBrushAtPixel(const QPoint& pixel, const LiveryPaintBrush& brush);
    QRgb blendPixel(QRgb base, QRgb brush, float alpha);
    void floodFill(const QPoint& start, const QColor& fillColor, float tolerance);
    QVector<QPoint> getNeighbors(const QPoint& p);
};

class LiveryPreset {
public:
    QString name;
    QVector<QColor> colors;
    QVector<QRect> regions;
    QJsonObject toJson() const;
    static LiveryPreset fromJson(const QJsonObject& json);
};

class LiveryPainterWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit LiveryPainterWidget(QWidget* parent = nullptr);
    ~LiveryPainterWidget();
    
    void setTexture(const QImage& texture);
    void setBrush(const LiveryPaintBrush& brush);
    void setMask(const QImage& mask);
    QImage getTexture() const { return m_painter.getCurrentTexture(); }
    
    void loadPreset(const LiveryPreset& preset);
    LiveryPreset savePreset(const QString& name) const;

public slots:
    void onColorSelected(const QColor& color);
    void onBrushSizeChanged(int size);
    void onBrushHardnessChanged(double hardness);

signals:
    void textureChanged(const QImage& texture);
    void brushChanged(const LiveryPaintBrush& brush);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    LiveryPainter m_painter;
    LiveryPaintBrush m_currentBrush;
    QPoint m_lastPos;
    bool m_isDrawing = false;
    
    void updateBrushPreview();
};

} // namespace ks