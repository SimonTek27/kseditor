#pragma once

#include <QWidget>
#include <QImage>
#include <QPoint>
#include <QList>
#include <QTimer>
#include <QPainterPath>
#include "PaintTypes.h"

namespace ks {
namespace paint {

class PaintDocument;

class PaintCanvasWidget : public QWidget {
    Q_OBJECT
public:
    explicit PaintCanvasWidget(QWidget* parent = nullptr);

    void setDocument(PaintDocument* doc);
    PaintDocument* document() const { return m_document; }

    void setTool(PaintTool tool);
    PaintTool tool() const { return m_tool; }

    void setPrimaryColor(const QColor& color);
    void setSecondaryColor(const QColor& color);
    QColor primaryColor() const { return m_primaryColor; }

    void setBrushSize(float size);
    void setBrushHardness(float hardness);
    void setBrushOpacity(float opacity);
    void setBrushFlow(float flow);
    void setBrushStrength(float strength);

    void setZoom(float zoom);
    float zoom() const { return m_zoom; }
    void zoomToFit();
    void zoomIn();
    void zoomOut();
    void zoom100();

    // Painter access
    float brushSize() const { return m_brushSize; }
    float brushHardness() const { return m_brushHardness; }
    float brushOpacity() const { return m_brushOpacity; }

    QPoint imagePosFromView(const QPoint& viewPos) const;

    void setCloneSource(const QPoint& pos);
    QPoint cloneSource() const { return m_cloneSource; }
    bool hasCloneSource() const { return m_hasCloneSource; }

    void setFuzzyTolerance(float tolerance) { m_fuzzyTolerance = tolerance; }
    float fuzzyTolerance() const { return m_fuzzyTolerance; }

    void applySelectionRect(const QRect& rect);          // for rect/ellipse select
    void applyFuzzySelection(const QPoint& pos, bool subtract);
    void applyFreeSelection(const QList<QPoint>& poly, bool subtract);

    void setGradientMode(bool radial) { m_radialGradient = radial; }
    bool radialGradient() const { return m_radialGradient; }

signals:
    void imageEdited();
    void statusMessage(const QString& message);
    void zoomChanged(float zoom);
    void colorPicked(const QColor& color);
    void selectionChanged();
    void documentInteractionFinished();
    void textToolClicked(const QPoint& imagePos);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    QPointF viewToImage(const QPoint& viewPos) const;
    QPoint imageToView(const QPointF& imagePos) const;
    QRect imageRectToView(const QRect& imageRect) const;
    void startStroke(const QPoint& imagePos);
    void updateStroke(const QPoint& imagePos);
    void endStroke();
    void pickColor(const QPoint& imagePos);
    void drawCheckerboard(QPainter& p, const QRect& rect);
    void drawBrushCursor(QPainter& p);
    void drawFreeSelectPreview(QPainter& p);
    void drawSelection(QPainter& p);

    PaintDocument* m_document = nullptr;
    PaintTool m_tool = PaintTool::Brush;

    QColor m_primaryColor = QColor(0, 0, 0);
    QColor m_secondaryColor = QColor(255, 255, 255);

    float m_brushSize = 20.0f;
    float m_brushHardness = 0.5f;
    float m_brushOpacity = 1.0f;
    float m_brushFlow = 1.0f;
    float m_brushStrength = 1.0f;
    float m_fuzzyTolerance = 0.2f;

    float m_zoom = 1.0f;
    QPointF m_pan;             // pan offset in image coordinates at center
    bool m_panning = false;
    QPoint m_panLastView;
    bool m_spacePressed = false;

    bool m_dragging = false;
    QPoint m_dragStartView;
    QPoint m_dragStartImage;
    QPoint m_lastImage;
    bool m_strokeActive = false;
    QPoint m_cloneSource;
    bool m_hasCloneSource = false;

    QRect m_selectionRect;            // for drag-based select
    QList<QPoint> m_freeSelectPoints;
    bool m_selecting = false;

    bool m_radialGradient = false;

    // Marching ants animation
    QTimer* m_marchingAntsTimer = nullptr;
    qreal m_marchingAntsOffset = 0.0;
    QPainterPath m_selectionPath;
    void updateSelectionPath();
    void startMarchingAnts();
    void stopMarchingAnts();
};

} // namespace paint
} // namespace ks