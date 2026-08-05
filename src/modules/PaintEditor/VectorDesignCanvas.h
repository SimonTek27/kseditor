#pragma once

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QImage>
#include <QVector>
#include <QPoint>
#include <QColor>
#include "VectorShape.h"

namespace ks {

class VectorDesignCanvas : public QGraphicsView {
    Q_OBJECT

public:
    enum Tool {
        SelectTool,
        RectangleTool,
        EllipseTool,
        LineTool,
        PolygonTool,
        PenTool
    };

    explicit VectorDesignCanvas(QWidget* parent = nullptr);
    ~VectorDesignCanvas() override;

    void setActiveTool(Tool tool);
    Tool activeTool() const { return m_activeTool; }

    void setFillColor(const QColor& color);
    QColor fillColor() const { return m_fillColor; }

    void setStrokeColor(const QColor& color);
    QColor strokeColor() const { return m_strokeColor; }

    void setStrokeWidth(float width);
    float strokeWidth() const { return m_strokeWidth; }

    void setDrawFilled(bool filled);
    bool drawFilled() const { return m_drawFilled; }

    void clearAll();
    void deleteSelected();
    void selectAll();

    QImage renderToImage(int width, int height) const;

    QGraphicsScene* vectorScene() const { return m_scene; }

    QJsonArray serializeShapes() const;
    void deserializeShapes(const QJsonArray& shapes);

    bool hasSelection() const;

signals:
    void shapesChanged();
    void canvasSelectionChanged();
    void shapeAdded(const VectorShapeData& data);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void drawBackground(QPainter* painter, const QRectF& rect) override;

private:
    QGraphicsScene* m_scene;
    Tool m_activeTool = SelectTool;

    QColor m_fillColor = Qt::red;
    QColor m_strokeColor = Qt::black;
    float m_strokeWidth = 2.0f;
    bool m_drawFilled = true;

    // Drawing state
    bool m_isDrawing = false;
    bool m_isDragging = false;
    bool m_isPanning = false;
    bool m_isBoxSelecting = false;

    QPointF m_drawStartPos;
    QPointF m_currentPos;
    QPointF m_lastPanPos;
    QRectF m_boxSelectRect;

    VectorShapeItem* m_currentShape = nullptr;

    // Polygon/pen tool state
    QVector<QPointF> m_currentPoints;
    VectorShapeItem* m_polyPreview = nullptr;

    VectorShapeItem::Handle m_dragHandle = VectorShapeItem::NoHandle;
    QRectF m_dragOrigRect;
    QPointF m_dragOrigPos;

    // Zoom
    float m_zoom = 1.0f;
    static constexpr float MIN_ZOOM = 0.1f;
    static constexpr float MAX_ZOOM = 10.0f;

    SelectionRectItem* m_selectionRect = nullptr;

    void setupScene();
    void applyZoom(float factor, QPointF centerPos);

    VectorShapeItem* createShapeItem(const VectorShapeData& data);
    void handleSelectPress(QMouseEvent* event);
    void handleDrawPress(QMouseEvent* event);
    void handleDrawMove(QMouseEvent* event);
    void handleDrawRelease(QMouseEvent* event);

    QGraphicsItem* itemAtPos(const QPointF& pos) const;
};

} // namespace ks
