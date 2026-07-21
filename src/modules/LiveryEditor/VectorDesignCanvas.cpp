#include "VectorDesignCanvas.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QScrollBar>
#include <QJsonDocument>
#include <QtMath>
#include <QJsonArray>
#include <QJsonObject>

namespace ks {

VectorDesignCanvas::VectorDesignCanvas(QWidget* parent)
    : QGraphicsView(parent)
{
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);

    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    setDragMode(QGraphicsView::NoDrag);
    setInteractive(true);
    setMouseTracking(true);
    setMinimumSize(200, 200);

    setStyleSheet("QGraphicsView { background: #1a1a1a; border: 1px solid #444; }");

    setupScene();

    m_selectionRect = new SelectionRectItem();
    m_scene->addItem(m_selectionRect);
    m_selectionRect->setVisible(false);
}

VectorDesignCanvas::~VectorDesignCanvas()
{
}

void VectorDesignCanvas::setupScene()
{
    m_scene->setSceneRect(-1000, -1000, 2000, 2000);
}

void VectorDesignCanvas::setActiveTool(Tool tool)
{
    if (m_isDrawing) {
        if (m_currentShape) {
            m_scene->removeItem(m_currentShape);
            delete m_currentShape;
            m_currentShape = nullptr;
        }
        m_currentPoints.clear();
        if (m_polyPreview) {
            m_scene->removeItem(m_polyPreview);
            delete m_polyPreview;
            m_polyPreview = nullptr;
        }
        m_isDrawing = false;
    }
    m_activeTool = tool;
    setCursor(tool == SelectTool ? Qt::ArrowCursor : Qt::CrossCursor);
}

void VectorDesignCanvas::setFillColor(const QColor& color)
{
    m_fillColor = color;
}

void VectorDesignCanvas::setStrokeColor(const QColor& color)
{
    m_strokeColor = color;
}

void VectorDesignCanvas::setStrokeWidth(float width)
{
    m_strokeWidth = qMax(0.5f, width);
}

void VectorDesignCanvas::setDrawFilled(bool filled)
{
    m_drawFilled = filled;
}

void VectorDesignCanvas::clearAll()
{
    m_scene->clear();
    m_selectionRect = new SelectionRectItem();
    m_scene->addItem(m_selectionRect);
    m_selectionRect->setVisible(false);
    m_currentShape = nullptr;
    m_polyPreview = nullptr;
    m_currentPoints.clear();
    m_isDrawing = false;
    emit shapesChanged();
}

void VectorDesignCanvas::deleteSelected()
{
    QList<QGraphicsItem*> items = m_scene->selectedItems();
    for (QGraphicsItem* item : items) {
        VectorShapeItem* shapeItem = dynamic_cast<VectorShapeItem*>(item);
        if (shapeItem) {
            m_scene->removeItem(shapeItem);
            delete shapeItem;
        }
    }
    emit shapesChanged();
}

void VectorDesignCanvas::selectAll()
{
    for (QGraphicsItem* item : m_scene->items()) {
        VectorShapeItem* shapeItem = dynamic_cast<VectorShapeItem*>(item);
        if (shapeItem) {
            shapeItem->setSelected(true);
        }
    }
}

bool VectorDesignCanvas::hasSelection() const
{
    return !m_scene->selectedItems().isEmpty();
}

QImage VectorDesignCanvas::renderToImage(int width, int height) const
{
    QImage image(width, height, QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QRectF bounds;
    for (QGraphicsItem* item : m_scene->items()) {
        VectorShapeItem* shape = dynamic_cast<VectorShapeItem*>(item);
        if (shape) {
            if (bounds.isNull())
                bounds = shape->sceneBoundingRect();
            else
                bounds = bounds.united(shape->sceneBoundingRect());
        }
    }

    if (bounds.isNull()) {
        return image;
    }

    float margin = qMax(bounds.width(), bounds.height()) * 0.1f;
    bounds.adjust(-margin, -margin, margin, margin);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    m_scene->render(&painter, QRectF(QPointF(0, 0), QSizeF(width, height)), bounds);
    painter.end();

    return image;
}

QJsonArray VectorDesignCanvas::serializeShapes() const
{
    QJsonArray arr;
    for (QGraphicsItem* item : m_scene->items()) {
        VectorShapeItem* shape = dynamic_cast<VectorShapeItem*>(item);
        if (shape) {
            arr.append(shape->shapeData().toJson());
        }
    }
    return arr;
}

void VectorDesignCanvas::deserializeShapes(const QJsonArray& shapes)
{
    clearAll();

    for (const QJsonValue& val : shapes) {
        VectorShapeData data = VectorShapeData::fromJson(val.toObject());
        VectorShapeItem* item = createShapeItem(data);
        if (item) {
            m_scene->addItem(item);
        }
    }

    emit shapesChanged();
}

VectorShapeItem* VectorDesignCanvas::createShapeItem(const VectorShapeData& data)
{
    auto* item = new VectorShapeItem(data);
    item->setPos(data.position);
    return item;
}

QGraphicsItem* VectorDesignCanvas::itemAtPos(const QPointF& pos) const
{
    QList<QGraphicsItem*> items = m_scene->items(pos, Qt::IntersectsItemShape,
                                                  Qt::DescendingOrder);
    for (QGraphicsItem* item : items) {
        if (item->type() == VectorShapeItem::Type)
            return item;
    }
    return nullptr;
}

void VectorDesignCanvas::applyZoom(float factor, QPointF centerPos)
{
    float newZoom = qBound(MIN_ZOOM, m_zoom * factor, MAX_ZOOM);
    if (qFuzzyCompare(newZoom, m_zoom)) return;

    factor = newZoom / m_zoom;
    m_zoom = newZoom;

    scale(factor, factor);
    emit shapesChanged();
}

// ─── Mouse Handling ─────────────────────────────────────────────────

void VectorDesignCanvas::mousePressEvent(QMouseEvent* event)
{
    QPointF scenePos = mapToScene(event->pos());

    if (event->button() == Qt::MiddleButton) {
        m_isPanning = true;
        m_lastPanPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton) {
        switch (m_activeTool) {
        case SelectTool:
            handleSelectPress(event);
            break;
        case RectangleTool:
        case EllipseTool:
        case LineTool:
        case PolygonTool:
        case PenTool:
            handleDrawPress(event);
            break;
        }
        event->accept();
        return;
    }

    QGraphicsView::mousePressEvent(event);
}

void VectorDesignCanvas::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isPanning) {
        QPoint delta = event->pos() - m_lastPanPos;
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        m_lastPanPos = event->pos();
        event->accept();
        return;
    }

    QPointF scenePos = mapToScene(event->pos());

    if (m_activeTool == SelectTool) {
        QGraphicsItem* item = itemAtPos(scenePos);
        if (item && item->type() == VectorShapeItem::Type) {
            auto* shape = static_cast<VectorShapeItem*>(item);
            VectorShapeItem::Handle handle = shape->hitTestHandle(scenePos);
            switch (handle) {
            case VectorShapeItem::TopLeft:
            case VectorShapeItem::BottomRight:
                setCursor(Qt::SizeFDiagCursor); break;
            case VectorShapeItem::TopRight:
            case VectorShapeItem::BottomLeft:
                setCursor(Qt::SizeBDiagCursor); break;
            case VectorShapeItem::TopMiddle:
            case VectorShapeItem::BottomMiddle:
                setCursor(Qt::SizeVerCursor); break;
            case VectorShapeItem::LeftMiddle:
            case VectorShapeItem::RightMiddle:
                setCursor(Qt::SizeHorCursor); break;
            default:
                setCursor(Qt::ArrowCursor); break;
            }
        } else {
            setCursor(Qt::ArrowCursor);
        }
    }

    if (m_isDrawing) {
        handleDrawMove(event);
        event->accept();
        return;
    }

    if (m_isBoxSelecting) {
        m_currentPos = scenePos;
        QRectF selRect = QRectF(m_drawStartPos, m_currentPos).normalized();
        m_selectionRect->setSelectionRect(selRect);
        m_selectionRect->setVisible(true);
        event->accept();
        return;
    }

    QGraphicsView::mouseMoveEvent(event);
}

void VectorDesignCanvas::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = false;
        setCursor(m_activeTool == SelectTool ? Qt::ArrowCursor : Qt::CrossCursor);
        event->accept();
        return;
    }

    if (m_isBoxSelecting) {
        m_isBoxSelecting = false;
        m_selectionRect->setVisible(false);
        QRectF selRect = QRectF(m_drawStartPos, m_currentPos).normalized();
        QPainterPath selPath;
        selPath.addRect(selRect);
        m_scene->setSelectionArea(selPath);
        emit canvasSelectionChanged();
        event->accept();
        return;
    }

    if (m_isDragging) {
        m_isDragging = false;
        m_dragHandle = VectorShapeItem::NoHandle;
        emit shapesChanged();
        event->accept();
        return;
    }

    if (m_isDrawing) {
        handleDrawRelease(event);
        event->accept();
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void VectorDesignCanvas::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (m_activeTool == PolygonTool || m_activeTool == PenTool) {
        if (m_isDrawing && m_currentPoints.size() >= 2) {
            VectorShapeData data;
            data.type = (m_activeTool == PolygonTool) ? VectorShapeData::Polygon
                                                      : VectorShapeData::Path;
            data.points = m_currentPoints;
            data.fillColor = m_fillColor;
            data.strokeColor = m_strokeColor;
            data.strokeWidth = m_strokeWidth;
            data.filled = m_drawFilled;

            auto* item = createShapeItem(data);
            m_scene->addItem(item);

            if (m_polyPreview) {
                m_scene->removeItem(m_polyPreview);
                delete m_polyPreview;
                m_polyPreview = nullptr;
            }

            m_currentPoints.clear();
            m_isDrawing = false;
            emit shapeAdded(data);
            emit shapesChanged();
        }
    }
    event->accept();
}

void VectorDesignCanvas::wheelEvent(QWheelEvent* event)
{
    float factor = (event->angleDelta().y() > 0) ? 1.15f : (1.0f / 1.15f);
    applyZoom(factor, mapToScene(event->position().toPoint()));
    event->accept();
}

void VectorDesignCanvas::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        deleteSelected();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        if (m_isDrawing) {
            if (m_currentShape) {
                m_scene->removeItem(m_currentShape);
                delete m_currentShape;
                m_currentShape = nullptr;
            }
            if (m_polyPreview) {
                m_scene->removeItem(m_polyPreview);
                delete m_polyPreview;
                m_polyPreview = nullptr;
            }
            m_currentPoints.clear();
            m_isDrawing = false;
            event->accept();
            return;
        }
    }
    if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_A) {
        selectAll();
        event->accept();
        return;
    }
    QGraphicsView::keyPressEvent(event);
}

void VectorDesignCanvas::drawBackground(QPainter* painter, const QRectF& rect)
{
    painter->fillRect(rect, QColor(30, 30, 30));

    painter->setPen(QPen(QColor(45, 45, 45), 0.5));
    float gridSize = 20.0f * m_zoom;
    if (gridSize < 5) gridSize = 5;

    double left = qFloor(rect.left() / gridSize) * gridSize;
    double top = qFloor(rect.top() / gridSize) * gridSize;

    for (double x = left; x <= rect.right(); x += gridSize) {
        painter->drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
    }
    for (double y = top; y <= rect.bottom(); y += gridSize) {
        painter->drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
    }
}

// ─── Tool handling ──────────────────────────────────────────────────

void VectorDesignCanvas::handleSelectPress(QMouseEvent* event)
{
    QPointF scenePos = mapToScene(event->pos());
    QGraphicsItem* item = itemAtPos(scenePos);

    if (item && item->type() == VectorShapeItem::Type) {
        auto* shape = static_cast<VectorShapeItem*>(item);
        VectorShapeItem::Handle handle = shape->hitTestHandle(scenePos);

        if (handle != VectorShapeItem::NoHandle) {
            m_isDragging = true;
            m_dragHandle = handle;
            m_dragOrigRect = shape->shapeData().boundingRect();
            m_dragOrigPos = shape->shapeData().position;
            return;
        }

        m_scene->clearSelection();
        shape->setSelected(true);
        return;
    }

    m_scene->clearSelection();
    m_isBoxSelecting = true;
    m_drawStartPos = scenePos;
    emit canvasSelectionChanged();
}

void VectorDesignCanvas::handleDrawPress(QMouseEvent* event)
{
    QPointF scenePos = mapToScene(event->pos());

    if (m_activeTool == PolygonTool || m_activeTool == PenTool) {
        m_currentPoints.append(scenePos);
        m_isDrawing = true;

        if (m_currentPoints.size() >= 2) {
            VectorShapeData previewData;
            previewData.type = VectorShapeData::Path;
            previewData.points = m_currentPoints;
            previewData.fillColor = m_fillColor;
            previewData.strokeColor = m_strokeColor;
            previewData.strokeWidth = m_strokeWidth;
            previewData.filled = false;

            if (m_polyPreview) {
                m_scene->removeItem(m_polyPreview);
                delete m_polyPreview;
            }

            m_polyPreview = createShapeItem(previewData);
            m_scene->addItem(m_polyPreview);
        }
        return;
    }

    m_drawStartPos = scenePos;
    m_currentPos = scenePos;
    m_isDrawing = true;

    VectorShapeData data;
    switch (m_activeTool) {
    case RectangleTool:
        data.type = VectorShapeData::Rectangle;
        data.size = QPointF(0, 0);
        break;
    case EllipseTool:
        data.type = VectorShapeData::Ellipse;
        data.size = QPointF(0, 0);
        break;
    case LineTool:
        data.type = VectorShapeData::Line;
        data.points.append(scenePos);
        data.points.append(scenePos);
        break;
    default:
        break;
    }
    data.position = scenePos;
    data.fillColor = m_fillColor;
    data.strokeColor = m_strokeColor;
    data.strokeWidth = m_strokeWidth;
    data.filled = m_drawFilled;

    m_currentShape = createShapeItem(data);
    m_scene->addItem(m_currentShape);
}

void VectorDesignCanvas::handleDrawMove(QMouseEvent* event)
{
    QPointF scenePos = mapToScene(event->pos());

    if (m_activeTool == PolygonTool || m_activeTool == PenTool) {
        if (m_polyPreview && m_currentPoints.size() >= 1) {
            QVector<QPointF> previewPoints = m_currentPoints;
            previewPoints.append(scenePos);

            VectorShapeData previewData;
            previewData.type = VectorShapeData::Path;
            previewData.points = previewPoints;
            previewData.fillColor = m_fillColor;
            previewData.strokeColor = m_strokeColor;
            previewData.strokeWidth = m_strokeWidth;
            previewData.filled = false;

            if (m_polyPreview) {
                m_scene->removeItem(m_polyPreview);
                delete m_polyPreview;
            }

            m_polyPreview = createShapeItem(previewData);
            m_scene->addItem(m_polyPreview);
        }
        return;
    }

    if (!m_currentShape) return;
    m_currentPos = scenePos;

    VectorShapeData data = m_currentShape->shapeData();

    switch (m_activeTool) {
    case RectangleTool:
    case EllipseTool: {
        QRectF rect = QRectF(m_drawStartPos, m_currentPos).normalized();
        data.position = rect.topLeft();
        data.size = rect.size();
        break;
    }
    case LineTool:
        data.points.clear();
        data.points.append(m_drawStartPos);
        data.points.append(m_currentPos);
        break;
    default:
        break;
    }

    m_currentShape->setShapeData(data);
}

void VectorDesignCanvas::handleDrawRelease(QMouseEvent* event)
{
    Q_UNUSED(event);

    if (m_activeTool == PolygonTool || m_activeTool == PenTool) {
        return;
    }

    if (!m_currentShape) return;

    VectorShapeData data = m_currentShape->shapeData();

    if (qAbs(data.size.x()) < 3 && qAbs(data.size.y()) < 3 &&
        m_activeTool != LineTool) {
        m_scene->removeItem(m_currentShape);
        delete m_currentShape;
        m_currentShape = nullptr;
        m_isDrawing = false;
        return;
    }

    emit shapeAdded(data);
    emit shapesChanged();

    m_currentShape = nullptr;
    m_isDrawing = false;
}

} // namespace ks
