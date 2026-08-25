#pragma once

#include <QGraphicsItem>
#include <QPainterPath>
#include <QPolygonF>
#include <QColor>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>
#include <QPoint>
#include <QRect>
#include <QPen>
#include <QBrush>

namespace ks {

// ─── Vector Shape Data ──────────────────────────────────────────────────

struct VectorShapeData {
    enum ShapeType { Rectangle, Ellipse, Line, Polygon, Path };

    ShapeType type = Rectangle;
    QPointF position;
    QPointF size;
    QPolygonF points;
    QColor fillColor = Qt::red;
    QColor strokeColor = Qt::black;
    float strokeWidth = 1.0f;
    float opacity = 1.0f;
    float rotation = 0.0f;
    bool filled = true;
    float pressure = 1.0f;
    float tiltX = 0.0f;
    float tiltY = 0.0f;

    QJsonObject toJson() const;
    static VectorShapeData fromJson(const QJsonObject& json);
    QPainterPath toPainterPath() const;
    QRectF boundingRect() const;
};

// ─── Vector Shape Graphics Items ────────────────────────────────────────

class VectorShapeItem : public QGraphicsItem {
public:
    enum { Type = QGraphicsItem::UserType + 100 };
    int type() const override { return Type; }

    explicit VectorShapeItem(const VectorShapeData& data, QGraphicsItem* parent = nullptr);
    ~VectorShapeItem() override = default;

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

    VectorShapeData shapeData() const { return m_data; }
    void setShapeData(const VectorShapeData& data);

    void updateFromDrag(const QPointF& delta);
    void updateFromResize(int handle, const QPointF& pos, const QRectF& origRect);

    enum Handle { NoHandle = -1, TopLeft, TopRight, BottomLeft, BottomRight,
                  TopMiddle, BottomMiddle, LeftMiddle, RightMiddle };
    Handle hitTestHandle(const QPointF& pos) const;
    QRectF handleRect(Handle handle) const;

private:
    VectorShapeData m_data;

    QPainterPath buildPath() const;
};

// ─── Selection Rectangle Item ──────────────────────────────────────────

class SelectionRectItem : public QGraphicsItem {
public:
    enum { Type = QGraphicsItem::UserType + 200 };
    int type() const override { return Type; }

    explicit SelectionRectItem(QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

    void setSelectionRect(const QRectF& rect);
    QRectF selectionRect() const { return m_rect; }

private:
    QRectF m_rect;
};

} // namespace ks
