#include "VectorShape.h"
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QTransform>
#include <QtMath>

namespace ks {

// ═══════════════════════════════════════════════════════════════════════
// VectorShapeData
// ═══════════════════════════════════════════════════════════════════════

QJsonObject VectorShapeData::toJson() const
{
    QJsonObject json;
    json["type"] = static_cast<int>(type);
    json["x"] = position.x();
    json["y"] = position.y();
    json["w"] = size.x();
    json["h"] = size.y();
    json["fill"] = fillColor.name(QColor::HexArgb);
    json["stroke"] = strokeColor.name(QColor::HexArgb);
    json["strokeWidth"] = strokeWidth;
    json["opacity"] = opacity;
    json["rotation"] = rotation;
    json["filled"] = filled;
    json["pressure"] = pressure;
    json["tiltX"] = tiltX;
    json["tiltY"] = tiltY;

    QJsonArray ptsArr;
    for (const QPointF& pt : points) {
        QJsonObject p;
        p["x"] = pt.x();
        p["y"] = pt.y();
        ptsArr.append(p);
    }
    json["points"] = ptsArr;

    return json;
}

VectorShapeData VectorShapeData::fromJson(const QJsonObject& json)
{
    VectorShapeData data;
    data.type = static_cast<ShapeType>(json["type"].toInt(0));
    data.position = QPointF(json["x"].toDouble(0), json["y"].toDouble(0));
    data.size = QPointF(json["w"].toDouble(100), json["h"].toDouble(100));
    data.fillColor = QColor(json["fill"].toString("#ff000000"));
    data.strokeColor = QColor(json["stroke"].toString("#ff000000"));
    data.strokeWidth = json["strokeWidth"].toDouble(1.0);
    data.opacity = json["opacity"].toDouble(1.0);
    data.rotation = json["rotation"].toDouble(0.0);
    data.filled = json["filled"].toBool(true);
    data.pressure = json["pressure"].toDouble(1.0);
    data.tiltX = json["tiltX"].toDouble(0.0);
    data.tiltY = json["tiltY"].toDouble(0.0);

    QJsonArray ptsArr = json["points"].toArray();
    for (const QJsonValue& val : ptsArr) {
        QJsonObject p = val.toObject();
        data.points.append(QPointF(p["x"].toDouble(0), p["y"].toDouble(0)));
    }

    return data;
}

QPainterPath VectorShapeData::toPainterPath() const
{
    QPainterPath path;

    switch (type) {
    case Rectangle: {
        QRectF rect(position, QSizeF(size.x(), size.y()));
        if (filled)
            path.addRoundedRect(rect, 2, 2);
        else {
            path.addRoundedRect(rect, 2, 2);
            QPainterPathStroker stroker;
            stroker.setWidth(strokeWidth);
            path = stroker.createStroke(path);
        }
        break;
    }
    case Ellipse: {
        QRectF rect(position, QSizeF(size.x(), size.y()));
        path.addEllipse(rect);
        if (!filled) {
            QPainterPathStroker stroker;
            stroker.setWidth(strokeWidth);
            path = stroker.createStroke(path);
        }
        break;
    }
    case Line: {
        if (points.size() >= 2) {
            path.moveTo(points[0]);
            for (int i = 1; i < points.size(); ++i)
                path.lineTo(points[i]);
        }
        break;
    }
    case Polygon: {
        if (!points.isEmpty()) {
            path.moveTo(points[0]);
            for (int i = 1; i < points.size(); ++i)
                path.lineTo(points[i]);
            path.closeSubpath();
            if (!filled) {
                QPainterPathStroker stroker;
                stroker.setWidth(strokeWidth);
                path = stroker.createStroke(path);
            }
        }
        break;
    }
    case Path: {
        if (!points.isEmpty()) {
            path.moveTo(points[0]);
            for (int i = 1; i < points.size(); ++i)
                path.lineTo(points[i]);
        }
        break;
    }
    }

    return path;
}

QRectF VectorShapeData::boundingRect() const
{
    switch (type) {
    case Rectangle:
    case Ellipse:
        return QRectF(position, QSizeF(size.x(), size.y()));
    case Line:
    case Polygon:
    case Path:
        if (!points.isEmpty()) {
            QRectF r(points[0], QSizeF(0, 0));
            for (const QPointF& pt : points)
                r = r.united(QRectF(pt, QSizeF(0, 0)));
            if (r.width() < 1) r.setWidth(1);
            if (r.height() < 1) r.setHeight(1);
            return r;
        }
        return QRectF(position, QSizeF(1, 1));
    }
    return QRectF(position, QSizeF(size.x(), size.y()));
}

// ═══════════════════════════════════════════════════════════════════════
// VectorShapeItem
// ═══════════════════════════════════════════════════════════════════════

static constexpr float HANDLE_SIZE = 8.0f;

VectorShapeItem::VectorShapeItem(const VectorShapeData& data, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_data(data)
{
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setPos(m_data.position);
    setRotation(m_data.rotation);
}

QRectF VectorShapeItem::boundingRect() const
{
    float margin = HANDLE_SIZE + 2;
    QRectF r = m_data.boundingRect();
    r.adjust(-margin, -margin, margin, margin);
    return r;
}

void VectorShapeItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                             QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing, true);

    QPainterPath path = buildPath();

    if (m_data.filled && m_data.type != VectorShapeData::Line) {
        QBrush brush(m_data.fillColor);
        painter->setBrush(brush);
        painter->setPen(QPen(m_data.strokeColor, m_data.strokeWidth));
    } else {
        painter->setBrush(Qt::NoBrush);
        QPen pen(m_data.type == VectorShapeData::Line ? m_data.fillColor : m_data.strokeColor,
                 m_data.strokeWidth);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        painter->setPen(pen);
    }

    painter->setOpacity(m_data.opacity);

    switch (m_data.type) {
    case VectorShapeData::Rectangle:
    case VectorShapeData::Ellipse:
        painter->drawPath(path);
        break;
    case VectorShapeData::Line:
        if (m_data.points.size() >= 2) {
            painter->drawLine(m_data.points[0], m_data.points[1]);
        }
        break;
    case VectorShapeData::Polygon:
    case VectorShapeData::Path:
        painter->drawPath(path);
        break;
    }

    if (isSelected()) {
        painter->setOpacity(1.0);
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(QColor(0, 120, 215), 1, Qt::DashLine));
        painter->drawRect(m_data.boundingRect());

        for (int h = TopLeft; h <= RightMiddle; ++h) {
            Handle handle = static_cast<Handle>(h);
            painter->setPen(QPen(Qt::white, 1));
            painter->setBrush(QColor(0, 120, 215));
            painter->drawRect(handleRect(handle));
        }
    }
}

void VectorShapeItem::setShapeData(const VectorShapeData& data)
{
    prepareGeometryChange();
    m_data = data;
    setPos(m_data.position);
    setRotation(m_data.rotation);
    update();
}

void VectorShapeItem::updateFromDrag(const QPointF& delta)
{
    prepareGeometryChange();
    m_data.position += delta;
    setPos(m_data.position);
    update();
}

void VectorShapeItem::updateFromResize(int handle, const QPointF& pos, const QRectF& origRect)
{
    Q_UNUSED(handle);
    Q_UNUSED(pos);
    Q_UNUSED(origRect);
}

VectorShapeItem::Handle VectorShapeItem::hitTestHandle(const QPointF& pos) const
{
    QRectF r = m_data.boundingRect();
    float hs = HANDLE_SIZE;

    auto testHandle = [&](Handle h, const QPointF& center) {
        QRectF hr(center.x() - hs/2, center.y() - hs/2, hs, hs);
        return hr.contains(pos);
    };

    if (testHandle(TopLeft, r.topLeft())) return TopLeft;
    if (testHandle(TopRight, r.topRight())) return TopRight;
    if (testHandle(BottomLeft, r.bottomLeft())) return BottomLeft;
    if (testHandle(BottomRight, r.bottomRight())) return BottomRight;
    if (testHandle(TopMiddle, QPointF(r.center().x(), r.top()))) return TopMiddle;
    if (testHandle(BottomMiddle, QPointF(r.center().x(), r.bottom()))) return BottomMiddle;
    if (testHandle(LeftMiddle, QPointF(r.left(), r.center().y()))) return LeftMiddle;
    if (testHandle(RightMiddle, QPointF(r.right(), r.center().y()))) return RightMiddle;

    return NoHandle;
}

QRectF VectorShapeItem::handleRect(Handle handle) const
{
    QRectF r = m_data.boundingRect();
    float hs = HANDLE_SIZE;
    QPointF center;

    switch (handle) {
    case TopLeft: center = r.topLeft(); break;
    case TopRight: center = r.topRight(); break;
    case BottomLeft: center = r.bottomLeft(); break;
    case BottomRight: center = r.bottomRight(); break;
    case TopMiddle: center = QPointF(r.center().x(), r.top()); break;
    case BottomMiddle: center = QPointF(r.center().x(), r.bottom()); break;
    case LeftMiddle: center = QPointF(r.left(), r.center().y()); break;
    case RightMiddle: center = QPointF(r.right(), r.center().y()); break;
    default: center = r.center(); break;
    }

    return QRectF(center.x() - hs/2, center.y() - hs/2, hs, hs);
}

QPainterPath VectorShapeItem::buildPath() const
{
    return m_data.toPainterPath();
}

// ═══════════════════════════════════════════════════════════════════════
// SelectionRectItem
// ═══════════════════════════════════════════════════════════════════════

SelectionRectItem::SelectionRectItem(QGraphicsItem* parent)
    : QGraphicsItem(parent)
{
}

QRectF SelectionRectItem::boundingRect() const
{
    return m_rect;
}

void SelectionRectItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                               QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setPen(QPen(QColor(0, 120, 215), 1, Qt::DashLine));
    painter->setBrush(QColor(0, 120, 215, 30));
    painter->drawRect(m_rect);
}

void SelectionRectItem::setSelectionRect(const QRectF& rect)
{
    prepareGeometryChange();
    m_rect = rect;
    update();
}

} // namespace ks
