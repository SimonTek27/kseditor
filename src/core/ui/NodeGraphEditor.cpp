#include "NodeGraphEditor.h"
#include "NodeGraphScene.h"
#include <QToolBar>
#include <QSplitter>
#include <QVBoxLayout>
#include <QAbstractItemModel>
#include <QGraphicsSceneMouseEvent>
#include <QContextMenuEvent>
#include <QStyleOptionGraphicsItem>
#include <QPainterPath>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QAction>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
#include <QGraphicsDropShadowEffect>
#include <algorithm>
#include <cmath>
#include <limits>

namespace ks {
namespace ui {

// ============================================================================
// NodeGraphView
// ============================================================================

NodeGraphView::NodeGraphView(NodeGraphScene* scene, QWidget* parent)
    : QGraphicsView(scene, parent), m_scene(scene)
{
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setViewportUpdateMode(FullViewportUpdate);
    setDragMode(NoDrag);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setTransformationAnchor(AnchorUnderMouse);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
}

NodeGraphView::~NodeGraphView() = default;

void NodeGraphView::fitInView(const QRectF& rect, bool animate)
{
    Q_UNUSED(animate);
    if (rect.isNull() && m_scene) {
        QGraphicsView::fitInView(m_scene->itemsBoundingRect(), Qt::KeepAspectRatio);
    } else if (!rect.isNull()) {
        QGraphicsView::fitInView(rect, Qt::KeepAspectRatio);
    }
    m_zoom = transform().m11();
    emit zoomChanged(m_zoom);
}

void NodeGraphView::centerOn(const QPointF& pos) { QGraphicsView::centerOn(pos); }

void NodeGraphView::setZoom(float zoom, bool animate)
{
    Q_UNUSED(animate);
    zoom = qBound(0.1f, zoom, 5.0f);
    if (qFuzzyCompare(m_zoom, zoom)) return;
    m_zoom = zoom;
    setTransform(QTransform::fromScale(zoom, zoom));
    updateMiniMap();
    emit zoomChanged(m_zoom);
}

void NodeGraphView::setGridVisible(bool visible) { m_gridVisible = visible; update(); }
void NodeGraphView::setGridSize(float size) { m_gridSize = size; update(); }
void NodeGraphView::setSnapToGrid(bool enabled) { m_snapToGrid = enabled; }
void NodeGraphView::setMiniMapVisible(bool visible) { m_miniMapVisible = visible; }
void NodeGraphView::setInteractionMode(InteractionMode mode) { m_mode = mode; }

void NodeGraphView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton || (event->button() == Qt::LeftButton && m_mode == InteractionMode::Pan)) {
        m_panning = true;
        m_lastPanPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton && m_mode == InteractionMode::BoxSelect) {
        m_boxSelecting = true;
        m_boxSelectRect = QRectF(mapToScene(event->pos()), mapToScene(event->pos()));
        event->accept();
        return;
    }

    QGraphicsView::mousePressEvent(event);
}

void NodeGraphView::mouseMoveEvent(QMouseEvent* event)
{
    if (m_panning) {
        QPointF delta = QPointF(event->pos() - m_lastPanPos) / m_zoom;
        handlePan(delta);
        m_lastPanPos = event->pos();
        event->accept();
        return;
    }

    if (m_boxSelecting) {
        m_boxSelectRect.setBottomRight(mapToScene(event->pos()));
        update();
        event->accept();
        return;
    }

    if (m_connecting) {
        m_connectingToPos = mapToScene(event->pos());
        m_scene->update();
        event->accept();
        return;
    }

    QGraphicsView::mouseMoveEvent(event);
}

void NodeGraphView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton || event->button() == Qt::LeftButton) {
        if (m_panning) {
            m_panning = false;
            setCursor(Qt::ArrowCursor);
            event->accept();
            return;
        }

        if (m_boxSelecting) {
            m_boxSelecting = false;
            handleBoxSelect(m_boxSelectRect);
            m_boxSelectRect = QRectF();
            update();
            event->accept();
            return;
        }

        if (m_connecting) {
            completeConnection();
            event->accept();
            return;
        }
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void NodeGraphView::mouseDoubleClickEvent(QMouseEvent* event) { QGraphicsView::mouseDoubleClickEvent(event); }

void NodeGraphView::wheelEvent(QWheelEvent* event)
{
    const float zoomFactor = 1.15f;
    if (event->angleDelta().y() > 0) {
        setZoom(m_zoom * zoomFactor);
    } else {
        setZoom(m_zoom / zoomFactor);
    }
    event->accept();
}

void NodeGraphView::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_F) {
        fitInView();
        event->accept();
        return;
    }
    QGraphicsView::keyPressEvent(event);
}

void NodeGraphView::keyReleaseEvent(QKeyEvent* event) { QGraphicsView::keyReleaseEvent(event); }

void NodeGraphView::drawBackground(QPainter* painter, const QRectF& rect)
{
    QGraphicsView::drawBackground(painter, rect);

    if (!m_gridVisible) return;

    painter->save();
    painter->setPen(QPen(QColor(40, 40, 50), 0));

    const float grid = m_gridSize;
    const float majorGrid = grid * 5;

    const qreal left = std::floor(rect.left() / grid) * grid;
    const qreal right = std::ceil(rect.right() / grid) * grid;
    const qreal top = std::floor(rect.top() / grid) * grid;
    const qreal bottom = std::ceil(rect.bottom() / grid) * grid;

    // Minor grid
    QVector<QLineF> minorLines;
    for (qreal x = left; x <= right; x += grid) {
        if (std::fmod(std::abs(x), majorGrid) < 0.01) continue;
        minorLines.append(QLineF(x, rect.top(), x, rect.bottom()));
    }
    for (qreal y = top; y <= bottom; y += grid) {
        if (std::fmod(std::abs(y), majorGrid) < 0.01) continue;
        minorLines.append(QLineF(rect.left(), y, rect.right(), y));
    }
    painter->drawLines(minorLines);

    // Major grid
    painter->setPen(QPen(QColor(60, 60, 70), 0));
    QVector<QLineF> majorLines;
    for (qreal x = left; x <= right; x += grid) {
        if (std::fmod(std::abs(x), majorGrid) < 0.01) {
            majorLines.append(QLineF(x, rect.top(), x, rect.bottom()));
        }
    }
    for (qreal y = top; y <= bottom; y += grid) {
        if (std::fmod(std::abs(y), majorGrid) < 0.01) {
            majorLines.append(QLineF(rect.left(), y, rect.right(), y));
        }
    }
    painter->drawLines(majorLines);

    painter->restore();
}

void NodeGraphView::contextMenuEvent(QContextMenuEvent* event) { QGraphicsView::contextMenuEvent(event); }
void NodeGraphView::dragEnterEvent(QDragEnterEvent* event) { QGraphicsView::dragEnterEvent(event); }
void NodeGraphView::dragMoveEvent(QDragMoveEvent* event) { QGraphicsView::dragMoveEvent(event); }
void NodeGraphView::dropEvent(QDropEvent* event) { QGraphicsView::dropEvent(event); }

void NodeGraphView::handlePan(const QPointF& delta)
{
    horizontalScrollBar()->setValue(horizontalScrollBar()->value() - static_cast<int>(delta.x()));
    verticalScrollBar()->setValue(verticalScrollBar()->value() - static_cast<int>(delta.y()));
}

void NodeGraphView::handleBoxSelect(const QRectF& rect)
{
    QRectF sceneRect = rect.normalized();
    for (QGraphicsItem* item : m_scene->items(sceneRect)) {
        if (auto* nodeItem = qgraphicsitem_cast<GraphNodeItem*>(item)) {
            nodeItem->setSelected(true);
        }
    }
}

void NodeGraphView::handleConnection(const QPointF& pos) { Q_UNUSED(pos); }

void NodeGraphView::completeConnection()
{
    m_connecting = false;
    m_scene->update();
}

void NodeGraphView::cancelConnection()
{
    m_connecting = false;
    m_scene->update();
}

void NodeGraphView::updateMiniMap()
{
    if (!m_miniMap || !m_scene) return;
    m_miniMap->update();
}

// ============================================================================
// GraphNodeItem
// ============================================================================

GraphNodeItem::GraphNodeItem(const QUuid& nodeId, NodeGraphScene* scene)
    : m_nodeId(nodeId), m_scene(scene)
{
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setAcceptHoverEvents(true);
    setZValue(1);
}

QRectF GraphNodeItem::boundingRect() const
{
    float headerH = HEADER_HEIGHT;
    int inputCount = m_nodeData.inputPortIds.size();
    int outputCount = m_nodeData.outputPortIds.size();
    int maxPorts = qMax(inputCount, outputCount);
    float bodyH = qMax(static_cast<float>(maxPorts) * PORT_SPACING + 8.0f, MIN_HEIGHT - headerH);
    float totalH = headerH + bodyH;
    return QRectF(0, 0, MIN_WIDTH, totalH);
}

void GraphNodeItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing);

    QRectF rect = boundingRect();
    float headerH = HEADER_HEIGHT;

    // Drop shadow
    if (m_selected || m_hovered) {
        QGraphicsDropShadowEffect shadow;
        shadow.setBlurRadius(m_selected ? 20 : 10);
        shadow.setColor(m_selected ? QColor(100, 180, 255, 120) : QColor(0, 0, 0, 80));
        shadow.setOffset(0, 2);
    }

    // Node body
    QPainterPath bodyPath;
    bodyPath.addRoundedRect(rect, 6, 6);
    painter->fillPath(bodyPath, QColor(40, 40, 50));
    painter->setPen(QPen(m_selected ? QColor(100, 180, 255) : QColor(70, 70, 80), m_selected ? 2 : 1));
    painter->drawPath(bodyPath);

    // Header
    QPainterPath headerPath;
    headerPath.addRoundedRect(QRectF(rect.topLeft(), QSizeF(rect.width(), headerH)), 6, 6);
    QColor headerColor = m_nodeData.headerColor.isValid() ? m_nodeData.headerColor : QColor(60, 65, 80);
    if (m_error) headerColor = QColor(140, 40, 40);
    painter->fillPath(headerPath, headerColor);
    painter->setPen(Qt::NoPen);
    // Cover bottom corners of header
    painter->fillRect(QRectF(rect.left(), rect.top() + headerH - 6, rect.width(), 6), headerColor);

    // Title text
    painter->setPen(QColor(220, 220, 230));
    QFont font = painter->font();
    font.setPointSize(9);
    font.setBold(true);
    painter->setFont(font);
    QString title = m_nodeData.title.isEmpty() ? m_nodeData.typeName : m_nodeData.title;
    painter->drawText(QRectF(rect.left() + 8, rect.top() + 2, rect.width() - 16, headerH - 4),
                      Qt::AlignLeft | Qt::AlignVCenter, title);

    // Ports
    int inputCount = m_nodeData.inputPortIds.size();
    int outputCount = m_nodeData.outputPortIds.size();

    for (int i = 0; i < inputCount; ++i) {
        float y = headerH + (i + 1) * PORT_SPACING;
        QColor typeColor = portTypeColor(m_nodeData.properties.value(QStringLiteral("inputType_%1").arg(i)).toString());
        drawPort(painter, QString(), QPointF(0, y), true, false, false, typeColor);
    }

    for (int i = 0; i < outputCount; ++i) {
        float y = headerH + (i + 1) * PORT_SPACING;
        QColor typeColor = portTypeColor(m_nodeData.properties.value(QStringLiteral("outputType_%1").arg(i)).toString());
        drawPort(painter, QString(), QPointF(rect.width(), y), false, false, false, typeColor);
    }

    // Error indicator
    if (m_error) {
        painter->setPen(QColor(255, 80, 80));
        QFont errorFont;
        errorFont.setPointSize(8);
        painter->setFont(errorFont);
        painter->drawText(QRectF(rect.left() + 8, rect.bottom() - 18, rect.width() - 16, 16),
                          Qt::AlignLeft | Qt::AlignBottom, m_nodeData.errorMessage);
    }
}

void GraphNodeItem::updateGeometry() { prepareGeometryChange(); update(); }

void GraphNodeItem::setSelected(bool selected) { m_selected = selected; update(); }

void GraphNodeItem::setError(bool error, const QString& message)
{
    m_error = error;
    m_nodeData.errorMessage = message;
    update();
}

void GraphNodeItem::setMinimized(bool minimized)
{
    m_nodeData.minimized = minimized;
    prepareGeometryChange();
    update();
}

GraphNodeItem::PortHit GraphNodeItem::hitTestPort(const QPointF& scenePos) const
{
    QPointF localPos = mapFromScene(scenePos);
    float headerH = HEADER_HEIGHT;

    // Check input ports
    for (int i = 0; i < m_nodeData.inputPortIds.size(); ++i) {
        float y = headerH + (i + 1) * PORT_SPACING;
        QPointF portPos(0, y);
        float dist = std::hypot(localPos.x() - portPos.x(), localPos.y() - portPos.y());
        if (dist <= PORT_RADIUS) {
            return {m_nodeData.inputPortIds[i], true, mapToScene(portPos)};
        }
    }

    // Check output ports
    for (int i = 0; i < m_nodeData.outputPortIds.size(); ++i) {
        float y = headerH + (i + 1) * PORT_SPACING;
        QPointF portPos(MIN_WIDTH, y);
        float dist = std::hypot(localPos.x() - portPos.x(), localPos.y() - portPos.y());
        if (dist <= PORT_RADIUS) {
            return {m_nodeData.outputPortIds[i], false, mapToScene(portPos)};
        }
    }

    return {};
}

void GraphNodeItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStartPos = event->pos();
    }
    QGraphicsItem::mousePressEvent(event);
}

void GraphNodeItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_dragging) {
        QPointF delta = event->pos() - m_dragStartPos;
        moveBy(delta.x(), delta.y());
        m_dragStartPos = event->pos();
    }
    QGraphicsItem::mouseMoveEvent(event);
}

void GraphNodeItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    m_dragging = false;
    QGraphicsItem::mouseReleaseEvent(event);
}

void GraphNodeItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    Q_UNUSED(event);
    m_nodeData.minimized = !m_nodeData.minimized;
    prepareGeometryChange();
    update();
}

void GraphNodeItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    m_hovered = true;
    PortHit hit = hitTestPort(event->scenePos());
    if (!hit.portId.isNull()) {
        setCursor(Qt::CrossCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
    update();
    QGraphicsItem::hoverMoveEvent(event);
}

void GraphNodeItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    m_hovered = false;
    setCursor(Qt::ArrowCursor);
    update();
    QGraphicsItem::hoverLeaveEvent(event);
}

void GraphNodeItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    Q_UNUSED(event);
}

void GraphNodeItem::drawPort(QPainter* painter, const QString& name, const QPointF& center,
                              bool isInput, bool isConnected, bool isHovered, const QColor& typeColor)
{
    painter->save();

    // Port circle
    QColor fillColor = isConnected ? typeColor : QColor(30, 30, 40);
    if (isHovered) fillColor = typeColor.lighter(130);

    painter->setBrush(fillColor);
    painter->setPen(QPen(typeColor, isHovered ? 2 : 1.5));
    painter->drawEllipse(center, PORT_RADIUS / 2, PORT_RADIUS / 2);

    // Port label
    if (!name.isEmpty()) {
        painter->setPen(QColor(180, 180, 190));
        QFont font = painter->font();
        font.setPointSize(8);
        painter->setFont(font);
        QPointF textPos = isInput ? QPointF(center.x() + PORT_RADIUS + 4, center.y())
                                  : QPointF(center.x() - PORT_RADIUS - 4, center.y());
        Qt::Alignment align = isInput ? Qt::AlignLeft | Qt::AlignVCenter : Qt::AlignRight | Qt::AlignVCenter;
        painter->drawText(QRectF(textPos.x() - 50, textPos.y() - 8, 100, 16), align, name);
    }

    painter->restore();
}

QColor GraphNodeItem::portTypeColor(const QString& type) const
{
    if (type == "float" || type == "number") return QColor(100, 200, 100);
    if (type == "vec2" || type == "vec3" || type == "vec4") return QColor(100, 150, 255);
    if (type == "color") return QColor(255, 150, 100);
    if (type == "texture") return QColor(200, 100, 200);
    if (type == "bool") return QColor(255, 200, 50);
    if (type == "execution") return QColor(255, 100, 100);
    if (type == "string") return QColor(200, 200, 100);
    return QColor(150, 150, 160);
}

// ============================================================================
// GraphConnectionItem
// ============================================================================

GraphConnectionItem::GraphConnectionItem(const QUuid& connectionId, NodeGraphScene* scene)
    : m_connectionId(connectionId), m_scene(scene)
{
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setAcceptHoverEvents(true);
    setZValue(0);
}

void GraphConnectionItem::updatePath() { rebuildPath(); }

void GraphConnectionItem::setSelected(bool selected) { m_selected = selected; update(); }

void GraphConnectionItem::setHovered(bool hovered) { m_hovered = hovered; update(); }

void GraphConnectionItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_selected = !m_selected;
        update();
    }
    QGraphicsPathItem::mousePressEvent(event);
}

void GraphConnectionItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) { Q_UNUSED(event); }

void GraphConnectionItem::rebuildPath()
{
    QPointF start = portScenePos(m_connectionData.fromNodeId, m_connectionData.fromPortId, false);
    QPointF end = portScenePos(m_connectionData.toNodeId, m_connectionData.toPortId, true);

    if (start.isNull() || end.isNull()) return;

    qreal dx = std::abs(end.x() - start.x()) * 0.5;
    dx = qMax(dx, 50.0);

    QPainterPath path;
    path.moveTo(start);
    path.cubicTo(start + QPointF(dx, 0), end - QPointF(dx, 0), end);

    setPath(path);

    QColor penColor = m_selected ? QColor(255, 200, 50) : (m_hovered ? QColor(180, 220, 255) : QColor(150, 150, 180));
    setPen(QPen(penColor, m_selected ? 2.5 : 2.0));
    setBrush(Qt::NoBrush);
}

QPointF GraphConnectionItem::portScenePos(const QUuid& nodeId, const QUuid& portId, bool isInput) const
{
    Q_UNUSED(nodeId);
    Q_UNUSED(portId);
    Q_UNUSED(isInput);
    return {};
}

// ============================================================================
// GraphMiniMap
// ============================================================================

GraphMiniMap::GraphMiniMap(NodeGraphScene* scene, NodeGraphView* view, QWidget* parent)
    : QWidget(parent), m_scene(scene), m_view(view)
{
    setFixedSize(160, 120);
    setStyleSheet("background: rgba(30, 30, 40, 200); border: 1px solid #555; border-radius: 4px;");
}

void GraphMiniMap::setScene(NodeGraphScene* scene) { m_scene = scene; }
void GraphMiniMap::setView(NodeGraphView* view) { m_view = view; }

void GraphMiniMap::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    if (!m_scene || !m_view) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw scene bounds
    QRectF sceneBounds = m_scene->itemsBoundingRect();
    if (sceneBounds.isNull()) return;

    QRectF widgetBounds = sceneToWidget(sceneBounds);
    painter.setPen(QPen(QColor(80, 80, 100), 1));
    painter.setBrush(QColor(40, 40, 50));
    painter.drawRect(widgetBounds);

    // Draw nodes as simple rectangles
    for (QGraphicsItem* item : m_scene->items()) {
        if (auto* nodeItem = qgraphicsitem_cast<GraphNodeItem*>(item)) {
            QRectF nodeRect = sceneToWidget(nodeItem->boundingRect().translated(nodeItem->pos()));
            painter.fillRect(nodeRect, QColor(80, 100, 140));
        }
    }

    // Draw viewport rect
    QRectF viewport = m_view->mapToScene(m_view->viewport()->rect()).boundingRect();
    QRectF vpWidget = sceneToWidget(viewport);
    painter.setPen(QPen(QColor(100, 180, 255), 1.5));
    painter.setBrush(QColor(100, 180, 255, 20));
    painter.drawRect(vpWidget);
}

void GraphMiniMap::mousePressEvent(QMouseEvent* event)
{
    m_dragging = true;
    m_dragStart = event->pos();
    if (m_view && m_scene) {
        QPointF scenePos = widgetToScene(QRectF(event->pos(), QSizeF(0, 0))).topLeft();
        m_view->centerOn(scenePos);
    }
}

void GraphMiniMap::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && m_view && m_scene) {
        QPointF scenePos = widgetToScene(QRectF(event->pos(), QSizeF(0, 0))).topLeft();
        m_view->centerOn(scenePos);
    }
}

void GraphMiniMap::mouseReleaseEvent(QMouseEvent* event) { Q_UNUSED(event); m_dragging = false; }

void GraphMiniMap::resizeEvent(QResizeEvent* event) { QWidget::resizeEvent(event); update(); }

QRectF GraphMiniMap::sceneToWidget(const QRectF& rect) const
{
    if (!m_scene) return rect;
    QRectF bounds = m_scene->itemsBoundingRect();
    if (bounds.isNull()) return rect;

    float scaleX = width() / bounds.width();
    float scaleY = height() / bounds.height();
    float scale = qMin(scaleX, scaleY) * 0.9f;

    QPointF offset((width() - bounds.width() * scale) / 2, (height() - bounds.height() * scale) / 2);
    return QRectF(
        (rect.left() - bounds.left()) * scale + offset.x(),
        (rect.top() - bounds.top()) * scale + offset.y(),
        rect.width() * scale,
        rect.height() * scale
    );
}

QRectF GraphMiniMap::widgetToScene(const QRectF& rect) const
{
    if (!m_scene) return rect;
    QRectF bounds = m_scene->itemsBoundingRect();
    if (bounds.isNull()) return rect;

    float scaleX = bounds.width() / width();
    float scaleY = bounds.height() / height();
    float scale = qMax(scaleX, scaleY) / 0.9f;

    QPointF offset = bounds.topLeft();
    return QRectF(
        rect.left() * scale + offset.x(),
        rect.top() * scale + offset.y(),
        rect.width() * scale,
        rect.height() * scale
    );
}

// ============================================================================
// PropertyEditor
// ============================================================================

PropertyEditor::PropertyEditor(QWidget* parent) : QWidget(parent)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
}

void PropertyEditor::setObject(QObject* object)
{
    Q_UNUSED(object);
}

void PropertyEditor::setProperties(const QMap<QString, QVariant>& properties)
{
    m_properties.clear();
    qDeleteAll(m_editors);
    m_editors.clear();

    for (auto it = properties.begin(); it != properties.end(); ++it) {
        addProperty(it.key(), it.value());
    }
    rebuildUI();
}

void PropertyEditor::clear()
{
    m_properties.clear();
    qDeleteAll(m_editors);
    m_editors.clear();
    QLayoutItem* item;
    while ((item = m_layout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
}

void PropertyEditor::addProperty(const QString& name, const QVariant& value,
                                  const QString& description, const QString& category)
{
    PropertyInfo pi;
    pi.name = name;
    pi.value = value;
    pi.description = description;
    pi.category = category;
    m_properties[name] = pi;
}

void PropertyEditor::removeProperty(const QString& name)
{
    m_properties.remove(name);
    if (m_editors.contains(name)) {
        delete m_editors[name];
        m_editors.remove(name);
    }
}

void PropertyEditor::setPropertyValue(const QString& name, const QVariant& value)
{
    if (m_properties.contains(name)) {
        m_properties[name].value = value;
        if (m_editors.contains(name)) {
            // Update editor widget
        }
    }
}

QVariant PropertyEditor::propertyValue(const QString& name) const { return m_properties.value(name).value; }

void PropertyEditor::setPropertyReadOnly(const QString& name, bool readOnly) { if (m_properties.contains(name)) m_properties[name].readOnly = readOnly; }
void PropertyEditor::setPropertyVisible(const QString& name, bool visible) { if (m_properties.contains(name)) m_properties[name].visible = visible; }
void PropertyEditor::setPropertyRange(const QString& name, double min, double max, double step) { if (m_properties.contains(name)) { m_properties[name].min = min; m_properties[name].max = max; m_properties[name].step = step; } }
void PropertyEditor::setPropertyOptions(const QString& name, const QStringList& options) { if (m_properties.contains(name)) m_properties[name].options = options; }
void PropertyEditor::setPropertyColor(const QString& name, bool alpha) { if (m_properties.contains(name)) { m_properties[name].isColor = true; m_properties[name].alpha = alpha; } }

void PropertyEditor::rebuildUI()
{
    qDeleteAll(m_editors);
    m_editors.clear();

    for (auto it = m_properties.begin(); it != m_properties.end(); ++it) {
        if (!it.value().visible) continue;
        QWidget* editor = createEditorForType(it.key(), it.value().value);
        if (editor) {
            m_editors[it.key()] = editor;
            m_layout->addWidget(editor);
        }
    }
    m_layout->addStretch();
}

QWidget* PropertyEditor::createEditorForType(const QString& name, const QVariant& value)
{
    Q_UNUSED(name);
    Q_UNUSED(value);
    return nullptr;
}

// ============================================================================
// VirtualTreeModel
// ============================================================================

VirtualTreeModel::VirtualTreeModel(QObject* parent) : QAbstractItemModel(parent) {}

void VirtualTreeModel::setRootItems(const QVector<TreeItem>& items)
{
    beginResetModel();
    // Build tree from items
    for (const auto& item : items) {
        auto* node = new Node;
        node->item = item;
        m_rootChildren.append(node);
    }
    endResetModel();
}

void VirtualTreeModel::appendItems(const QVector<TreeItem>& items, const QModelIndex& parent)
{
    Node* parentNode = findNode(parent);
    if (!parentNode) {
        // Append to root
        int start = m_rootChildren.size();
        beginInsertRows(QModelIndex(), start, start + items.size() - 1);
        for (const auto& item : items) {
            auto* node = new Node;
            node->item = item;
            node->parent = nullptr;
            m_rootChildren.append(node);
        }
        endInsertRows();
    }
}

void VirtualTreeModel::removeItems(const QModelIndex& parent, int row, int count)
{
    Q_UNUSED(parent);
    beginRemoveRows(QModelIndex(), row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        delete m_rootChildren[row];
        m_rootChildren.removeAt(row);
    }
    endRemoveRows();
}

void VirtualTreeModel::setItemExpanded(const QModelIndex& index, bool expanded)
{
    Node* node = findNode(index);
    if (node) {
        node->item.expanded = expanded;
        if (expanded) emit itemExpanded(index);
        else emit itemCollapsed(index);
    }
}

bool VirtualTreeModel::isItemExpanded(const QModelIndex& index) const
{
    Node* node = findNode(index);
    return node ? node->item.expanded : false;
}

void VirtualTreeModel::setLazyLoadCallback(std::function<void(const TreeItem&)> callback) { m_lazyLoadCallback = callback; }

void VirtualTreeModel::loadChildren(const QModelIndex& parent)
{
    Node* node = findNode(parent);
    if (node && m_lazyLoadCallback && !node->loaded) {
        node->loading = true;
        emit lazyLoadRequested(parent);
        m_lazyLoadCallback(node->item);
        node->loaded = true;
        node->loading = false;
    }
}

QModelIndex VirtualTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (column != 0) return QModelIndex();

    Node* parentNode = findNode(parent);
    QVector<Node*>& children = parentNode ? parentNode->children : const_cast<QList<Node*>&>(m_rootChildren);

    if (row < 0 || row >= children.size()) return QModelIndex();
    return createIndex(row, 0, children[row]);
}

QModelIndex VirtualTreeModel::parent(const QModelIndex& index) const
{
    if (!index.isValid()) return QModelIndex();
    Node* node = static_cast<Node*>(index.internalPointer());
    if (!node || !node->parent) return QModelIndex();

    Node* parentNode = node->parent;
    for (int i = 0; i < m_rootChildren.size(); ++i) {
        if (m_rootChildren[i] == parentNode) {
            return createIndex(i, 0, parentNode);
        }
    }
    return QModelIndex();
}

int VirtualTreeModel::rowCount(const QModelIndex& parent) const
{
    Node* node = findNode(parent);
    if (node) return node->children.size();
    return m_rootChildren.size();
}

int VirtualTreeModel::columnCount(const QModelIndex& parent) const { Q_UNUSED(parent); return 1; }

QVariant VirtualTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) return QVariant();
    Node* node = static_cast<Node*>(index.internalPointer());
    if (!node) return QVariant();

    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        return node->item.name;
    case Qt::DecorationRole:
        return node->item.icon;
    case Qt::ToolTipRole:
        return node->item.toolTip;
    case Qt::ForegroundRole:
        return node->item.foreground;
    case Qt::BackgroundRole:
        return node->item.background;
    default:
        return QVariant();
    }
}

QVariant VirtualTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(section);
    Q_UNUSED(orientation);
    Q_UNUSED(role);
    return QVariant();
}

Qt::ItemFlags VirtualTreeModel::flags(const QModelIndex& index) const
{
    if (!index.isValid()) return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVector<int> VirtualTreeModel::expandedRows() const
{
    QVector<int> rows;
    for (int i = 0; i < m_rootChildren.size(); ++i) {
        if (m_rootChildren[i]->item.expanded) rows.append(i);
    }
    return rows;
}

VirtualTreeModel::Node* VirtualTreeModel::findNode(const QModelIndex& index) const
{
    if (!index.isValid()) return nullptr;
    return static_cast<Node*>(index.internalPointer());
}

VirtualTreeModel::Node* VirtualTreeModel::findNodeById(Node* node, const QString& id) const
{
    if (!node) return nullptr;
    if (node->item.id == id) return node;
    for (Node* child : node->children) {
        Node* found = findNodeById(child, id);
        if (found) return found;
    }
    return nullptr;
}

void VirtualTreeModel::deleteNode(Node* node)
{
    if (!node) return;
    for (Node* child : node->children) {
        deleteNode(child);
    }
    delete node;
}

void VirtualTreeModel::updateExpandedState(Node* node, bool expanded)
{
    if (node) node->item.expanded = expanded;
}

// ============================================================================
// ColorEditorWidget
// ============================================================================

ColorEditorWidget::ColorEditorWidget(QWidget* parent) : QWidget(parent)
{
    setMinimumSize(200, 180);
}

void ColorEditorWidget::setColor(const QColor& color) { m_color = color; update(); }
void ColorEditorWidget::setMode(Mode mode) { m_mode = mode; update(); }
void ColorEditorWidget::setAlphaVisible(bool visible) { m_alphaVisible = visible; update(); }
void ColorEditorWidget::setPresets(const QVector<QColor>& presets) { m_presets = presets; update(); }
void ColorEditorWidget::addPreset(const QColor& color) { m_presets.append(color); update(); }

void ColorEditorWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRect wheelRect(10, 10, width() - 20, height() - 60);
    drawColorWheel(&painter, wheelRect);

    QRect alphaRect(10, height() - 40, width() - 20, 20);
    if (m_alphaVisible) drawAlphaBar(&painter, alphaRect);

    QRect presetRect(10, height() - 15, width() - 20, 10);
    drawPresets(&painter, presetRect);
}

void ColorEditorWidget::mousePressEvent(QMouseEvent* event)
{
    QColor newColor = colorFromWheelPos(event->pos());
    if (newColor.isValid()) {
        m_color.setHsv(newColor.hue(), newColor.saturation(), m_color.value(), m_color.alpha());
        emit colorChanged(m_color);
    }
    update();
}

void ColorEditorWidget::mouseMoveEvent(QMouseEvent* event) { Q_UNUSED(event); }
void ColorEditorWidget::mouseReleaseEvent(QMouseEvent* event) { Q_UNUSED(event); }

void ColorEditorWidget::drawColorWheel(QPainter* painter, const QRect& rect)
{
    painter->save();
    painter->setClipRect(rect);

    int cx = rect.center().x();
    int cy = rect.center().y();
    int radius = qMin(rect.width(), rect.height()) / 2 - 4;

    // Draw hue wheel
    for (int angle = 0; angle < 360; angle += 2) {
        QColor color;
        color.setHsv(angle, 255, 255);
        painter->setPen(QPen(color, 3));
        double rad = angle * M_PI / 180.0;
        double innerR = radius * 0.6;
        painter->drawLine(
            QPointF(cx + innerR * std::cos(rad), cy + innerR * std::sin(rad)),
            QPointF(cx + radius * std::cos(rad), cy + radius * std::sin(rad))
        );
    }

    // Draw SV square in center
    int sqSize = static_cast<int>(radius * 0.5);
    QRect sqRect(cx - sqSize, cy - sqSize, sqSize * 2, sqSize * 2);
    for (int x = 0; x < sqSize * 2; ++x) {
        for (int y = 0; y < sqSize * 2; ++y) {
            QColor c;
            c.setHsv(m_color.hue(), x * 255 / (sqSize * 2), (sqSize * 2 - y) * 255 / (sqSize * 2));
            painter->setPen(c);
            painter->drawPoint(sqRect.topLeft() + QPoint(x, y));
        }
    }

    painter->restore();
}

void ColorEditorWidget::drawAlphaBar(QPainter* painter, const QRect& rect)
{
    painter->save();

    // Checkerboard background
    int squareSize = 6;
    for (int x = rect.left(); x < rect.right(); x += squareSize) {
        for (int y = rect.top(); y < rect.bottom(); y += squareSize) {
            bool light = ((x - rect.left()) / squareSize + (y - rect.top()) / squareSize) % 2 == 0;
            painter->fillRect(x, y, squareSize, squareSize, light ? QColor(200, 200, 200) : QColor(150, 150, 150));
        }
    }

    // Alpha gradient
    QLinearGradient gradient(rect.topLeft(), rect.topRight());
    gradient.setColorAt(0, QColor(m_color.red(), m_color.green(), m_color.blue(), 0));
    gradient.setColorAt(1, QColor(m_color.red(), m_color.green(), m_color.blue(), 255));
    painter->fillRect(rect, gradient);

    painter->setPen(QPen(Qt::white, 1.5));
    int alphaX = rect.left() + static_cast<int>(m_color.alpha() / 255.0 * rect.width());
    painter->drawLine(alphaX, rect.top(), alphaX, rect.bottom());

    painter->restore();
}

void ColorEditorWidget::drawPresets(QPainter* painter, const QRect& rect)
{
    painter->save();
    int presetSize = qMin(10, rect.height());
    int spacing = 2;
    int x = rect.left();
    for (const QColor& c : m_presets) {
        if (x + presetSize > rect.right()) break;
        painter->fillRect(x, rect.top(), presetSize, presetSize, c);
        painter->setPen(QPen(Qt::gray, 0.5));
        painter->drawRect(x, rect.top(), presetSize, presetSize);
        x += presetSize + spacing;
    }
    painter->restore();
}

QColor ColorEditorWidget::colorFromWheelPos(const QPointF& pos) const
{
    Q_UNUSED(pos);
    return m_color;
}

// ============================================================================
// CurveEditorWidget
// ============================================================================

CurveEditorWidget::CurveEditorWidget(QWidget* parent) : QWidget(parent)
{
    setMinimumSize(200, 150);
    setFocusPolicy(Qt::StrongFocus);
}

void CurveEditorWidget::setCurve(const QVector<CurvePoint>& points) { m_points = points; update(); }

void CurveEditorWidget::setRange(float xMin, float xMax, float yMin, float yMax)
{
    m_xMin = xMin; m_xMax = xMax; m_yMin = yMin; m_yMax = yMax;
    update();
}

float CurveEditorWidget::evaluate(float x) const
{
    if (m_points.size() < 2) return 0.0f;

    // Find surrounding points
    for (int i = 0; i < m_points.size() - 1; ++i) {
        if (x >= m_points[i].x && x <= m_points[i + 1].x) {
            float t = (x - m_points[i].x) / (m_points[i + 1].x - m_points[i].x);
            return cubicInterpolate(t, m_points[i], m_points[i + 1]);
        }
    }

    return m_points.first().y;
}

void CurveEditorWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Grid
    if (m_gridVisible) {
        painter.setPen(QPen(m_gridColor, 0.5));
        for (int i = 0; i <= 10; ++i) {
            float x = width() * i / 10.0f;
            float y = height() * i / 10.0f;
            painter.drawLine(QPointF(x, 0), QPointF(x, height()));
            painter.drawLine(QPointF(0, y), QPointF(width(), y));
        }
    }

    // Curve
    if (m_points.size() >= 2) {
        QPainterPath path;
        QPointF first = dataToWidget(m_points[0].x, m_points[0].y);
        path.moveTo(first);

        for (int i = 0; i < m_points.size() - 1; ++i) {
            QPointF p0 = dataToWidget(m_points[i].x, m_points[i].y);
            QPointF p1 = dataToWidget(m_points[i + 1].x, m_points[i + 1].y);

            if (m_points[i].interpolation == "linear") {
                path.lineTo(p1);
            } else {
                // Cubic bezier approximation
                QPointF cp1 = p0 + QPointF(p0.x() + m_points[i].outTangentX * (p1.x() - p0.x()),
                                           p0.y() + m_points[i].outTangentY * (p1.y() - p0.y()));
                QPointF cp2 = p1 - QPointF(m_points[i + 1].inTangentX * (p1.x() - p0.x()),
                                           m_points[i + 1].inTangentY * (p1.y() - p0.y()));
                path.cubicTo(cp1, cp2, p1);
            }
        }

        painter.setPen(QPen(m_curveColor, 2));
        painter.drawPath(path);
    }

    // Points
    for (int i = 0; i < m_points.size(); ++i) {
        QPointF pos = dataToWidget(m_points[i].x, m_points[i].y);
        QColor color = (i == m_selectedPoint) ? m_selectedPointColor : m_curveColor;
        painter.setBrush(color);
        painter.setPen(QPen(Qt::white, 1));
        painter.drawEllipse(pos, 5, 5);
    }
}

void CurveEditorWidget::mousePressEvent(QMouseEvent* event)
{
    int idx = hitTestPoint(event->pos());
    if (idx >= 0) {
        m_selectedPoint = idx;
        m_draggingPoint = true;
        emit pointSelected(idx);
    } else {
        m_selectedPoint = -1;
    }
    update();
}

void CurveEditorWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_draggingPoint && m_selectedPoint >= 0) {
        QPointF data = widgetToData(event->pos());
        m_points[m_selectedPoint].x = data.x();
        m_points[m_selectedPoint].y = data.y();
        emit pointMoved(m_selectedPoint, m_points[m_selectedPoint]);
        update();
    }
}

void CurveEditorWidget::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    m_draggingPoint = false;
}

void CurveEditorWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    QPointF data = widgetToData(event->pos());
    addPoint(data.x(), data.y());
}

void CurveEditorWidget::wheelEvent(QWheelEvent* event)
{
    Q_UNUSED(event);
}

void CurveEditorWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Delete && m_selectedPoint >= 0) {
        removePoint(m_selectedPoint);
        m_selectedPoint = -1;
        update();
    }
}

void CurveEditorWidget::resizeEvent(QResizeEvent* event) { QWidget::resizeEvent(event); update(); }

QPointF CurveEditorWidget::dataToWidget(float x, float y) const
{
    float wx = (x - m_xMin) / (m_xMax - m_xMin) * width();
    float wy = (1.0f - (y - m_yMin) / (m_yMax - m_yMin)) * height();
    return QPointF(wx, wy);
}

QPointF CurveEditorWidget::widgetToData(const QPointF& pos) const
{
    float dx = pos.x() / width() * (m_xMax - m_xMin) + m_xMin;
    float dy = (1.0f - pos.y() / height()) * (m_yMax - m_yMin) + m_yMin;
    return QPointF(dx, dy);
}

int CurveEditorWidget::hitTestPoint(const QPointF& pos) const
{
    for (int i = 0; i < m_points.size(); ++i) {
        QPointF pt = dataToWidget(m_points[i].x, m_points[i].y);
        if (std::hypot(pos.x() - pt.x(), pos.y() - pt.y()) < 8.0) return i;
    }
    return -1;
}

int CurveEditorWidget::hitTestTangent(const QPointF&) const { return -1; }

void CurveEditorWidget::addPoint(float x, float y)
{
    CurvePoint p;
    p.x = x;
    p.y = y;
    p.autoTangents = true;

    // Insert in sorted order
    int insertIdx = m_points.size();
    for (int i = 0; i < m_points.size(); ++i) {
        if (m_points[i].x > x) {
            insertIdx = i;
            break;
        }
    }
    m_points.insert(insertIdx, p);
    update();
    emit curveChanged(m_points);
}

void CurveEditorWidget::removePoint(int index)
{
    if (index >= 0 && index < m_points.size()) {
        m_points.removeAt(index);
        update();
        emit curveChanged(m_points);
    }
}

void CurveEditorWidget::updateTangents(int index)
{
    if (index < 0 || index >= m_points.size()) return;
    CurvePoint& p = m_points[index];

    if (p.autoTangents && index > 0 && index < m_points.size() - 1) {
        float dx = m_points[index + 1].x - m_points[index - 1].x;
        float dy = m_points[index + 1].y - m_points[index - 1].y;
        float len = std::max(std::hypot(dx, dy), 0.001f);
        p.inTangentX = dx / len * 0.3f;
        p.inTangentY = dy / len * 0.3f;
        p.outTangentX = dx / len * 0.3f;
        p.outTangentY = dy / len * 0.3f;
    }
}

float CurveEditorWidget::cubicInterpolate(float t, const CurvePoint& p0, const CurvePoint& p1) const
{
    // Simple cubic interpolation
    float t2 = t * t;
    float t3 = t2 * t;
    float h1 = 2 * t3 - 3 * t2 + 1;
    float h2 = -2 * t3 + 3 * t2;
    float h3 = t3 - 2 * t2 + t;
    float h4 = t3 - t2;

    float p = h1 * p0.y + h2 * p1.y + h3 * (p0.outTangentY * (p1.x - p0.x)) + h4 * (p1.inTangentY * (p1.x - p0.x));
    return p;
}

// ============================================================================
// GradientEditorWidget
// ============================================================================

GradientEditorWidget::GradientEditorWidget(QWidget* parent) : QWidget(parent)
{
    setMinimumSize(200, 50);
    setFixedHeight(50);
}

void GradientEditorWidget::setGradient(const QVector<GradientStop>& stops) { m_stops = stops; update(); }

QGradientStops GradientEditorWidget::toQGradientStops() const
{
    QGradientStops stops;
    for (const auto& s : m_stops) {
        QColor c = s.color;
        c.setAlphaF(s.alpha);
        stops.append(QGradientStop(s.position, c));
    }
    return stops;
}

void GradientEditorWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRect barRect(10, 10, width() - 20, 20);

    // Checkerboard
    int sq = 4;
    for (int x = barRect.left(); x < barRect.right(); x += sq) {
        for (int y = barRect.top(); y < barRect.bottom(); y += sq) {
            bool light = ((x - barRect.left()) / sq + (y - barRect.top()) / sq) % 2 == 0;
            painter.fillRect(x, y, sq, sq, light ? QColor(200, 200, 200) : QColor(150, 150, 150));
        }
    }

    // Gradient
    QLinearGradient gradient(barRect.topLeft(), barRect.topRight());
    QGradientStops qStops = toQGradientStops();
    if (qStops.isEmpty()) {
        gradient.setColorAt(0, Qt::black);
        gradient.setColorAt(1, Qt::white);
    } else {
        for (const auto& s : qStops) gradient.setColorAt(s.first, s.second);
    }
    painter.fillRect(barRect, gradient);

    // Stop markers
    for (int i = 0; i < m_stops.size(); ++i) {
        int x = barRect.left() + static_cast<int>(m_stops[i].position * barRect.width());
        QColor markerColor = (i == m_selectedStop) ? Qt::white : Qt::lightGray;
        painter.setPen(QPen(markerColor, 1.5));
        painter.setBrush(m_stops[i].color);
        QPolygon triangle;
        triangle << QPoint(x - 4, barRect.bottom() + 2) << QPoint(x + 4, barRect.bottom() + 2) << QPoint(x, barRect.bottom() - 2);
        painter.drawPolygon(triangle);
    }
}

void GradientEditorWidget::mousePressEvent(QMouseEvent* event)
{
    int idx = hitTestStop(event->pos());
    if (idx >= 0) {
        m_selectedStop = idx;
        m_dragging = true;
    } else {
        // Add new stop
        QRect barRect(10, 10, width() - 20, 20);
        float pos = static_cast<float>(event->pos().x() - barRect.left()) / barRect.width();
        pos = qBound(0.0f, pos, 1.0f);
        addStop(pos);
    }
    update();
}

void GradientEditorWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && m_selectedStop >= 0) {
        QRect barRect(10, 10, width() - 20, 20);
        float pos = static_cast<float>(event->pos().x() - barRect.left()) / barRect.width();
        pos = qBound(0.0f, pos, 1.0f);
        updateStopPosition(m_selectedStop, pos);
        update();
    }
}

void GradientEditorWidget::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    m_dragging = false;
}

void GradientEditorWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    int idx = hitTestStop(event->pos());
    if (idx >= 0) {
        removeStop(idx);
        update();
    }
}

void GradientEditorWidget::contextMenuEvent(QContextMenuEvent* event) { Q_UNUSED(event); }

int GradientEditorWidget::hitTestStop(const QPoint& pos) const
{
    QRect barRect(10, 10, width() - 20, 20);
    for (int i = 0; i < m_stops.size(); ++i) {
        int x = barRect.left() + static_cast<int>(m_stops[i].position * barRect.width());
        if (std::abs(pos.x() - x) < 8 && pos.y() >= barRect.top() - 10 && pos.y() <= barRect.bottom() + 10) {
            return i;
        }
    }
    return -1;
}

void GradientEditorWidget::addStop(float position)
{
    GradientStop stop;
    stop.position = position;
    stop.color = Qt::white;
    stop.alpha = 1.0f;

    // Interpolate color at position
    if (m_stops.size() >= 2) {
        for (int i = 0; i < m_stops.size() - 1; ++i) {
            if (position >= m_stops[i].position && position <= m_stops[i + 1].position) {
                float t = (position - m_stops[i].position) / (m_stops[i + 1].position - m_stops[i].position);
                stop.color = QColor::fromRgbF(
                    m_stops[i].color.redF() * (1 - t) + m_stops[i + 1].color.redF() * t,
                    m_stops[i].color.greenF() * (1 - t) + m_stops[i + 1].color.greenF() * t,
                    m_stops[i].color.blueF() * (1 - t) + m_stops[i + 1].color.blueF() * t
                );
                break;
            }
        }
    }

    m_stops.append(stop);
    std::sort(m_stops.begin(), m_stops.end(), [](const GradientStop& a, const GradientStop& b) {
        return a.position < b.position;
    });
    emit gradientChanged(m_stops);
}

void GradientEditorWidget::removeStop(int index)
{
    if (index >= 0 && index < m_stops.size() && m_stops.size() > 2) {
        m_stops.removeAt(index);
        emit gradientChanged(m_stops);
    }
}

void GradientEditorWidget::updateStopPosition(int index, float position)
{
    if (index >= 0 && index < m_stops.size()) {
        m_stops[index].position = position;
        std::sort(m_stops.begin(), m_stops.end(), [](const GradientStop& a, const GradientStop& b) {
            return a.position < b.position;
        });
        emit stopMoved(index, position);
        emit gradientChanged(m_stops);
    }
}

// ============================================================================
// NodeGraphWidget
// ============================================================================

NodeGraphWidget::NodeGraphWidget(QWidget* parent) : QWidget(parent)
{
    m_scene = new NodeGraphScene(this);
    m_view = new NodeGraphView(m_scene, this);
    m_miniMap = new GraphMiniMap(m_scene, m_view, this);

    m_toolbar = new QToolBar(this);
    m_nodeContextMenu = new QMenu(this);
    m_connectionContextMenu = new QMenu(this);
    m_backgroundContextMenu = new QMenu(this);

    setupUI();
    setupConnections();
    setupContextMenus();
}

void NodeGraphWidget::setupUI()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->addWidget(m_view);

    layout->addWidget(m_toolbar);
    layout->addWidget(m_splitter);

    // Toolbar actions
    m_toolbar->addAction("Add Node", this, [this]() {
        addNode("Default", QPointF(0, 0));
    });
    m_toolbar->addAction("Fit All", this, [this]() {
        m_view->fitInView();
    });
}

void NodeGraphWidget::setupConnections()
{
    if (!m_scene || !m_view) return;
    connect(m_scene, &QGraphicsScene::selectionChanged, this, [this]() {
        QList<QGraphicsItem*> selected = m_scene->selectedItems();
        for (auto* item : selected) {
            if (auto* node = qgraphicsitem_cast<GraphNodeItem*>(item))
                emit nodeSelected(node->nodeId());
        }
    });
}

void NodeGraphWidget::setupContextMenus()
{
    m_nodeContextMenu->addAction("Delete", this, [this]() { removeSelectedNodes(); });
    m_nodeContextMenu->addAction("Duplicate", this, [this]() { duplicateSelectedNodes(); });
    m_nodeContextMenu->addAction("Minimize", this, [this]() {
        for (QGraphicsItem* item : m_scene->selectedItems()) {
            if (auto* node = qgraphicsitem_cast<GraphNodeItem*>(item)) {
                node->setMinimized(!node->isSelected());
            }
        }
    });
}

QUuid NodeGraphWidget::addNode(const QString& typeName, const QPointF& position)
{
    QUuid nodeId = QUuid::createUuid();

    auto* item = new GraphNodeItem(nodeId, m_scene);
    item->setPos(position);

    GraphNode node;
    node.id = nodeId;
    node.typeName = typeName;
    node.title = typeName;
    node.position = position;
    node.headerColor = QColor(60, 65, 80);

    // Add default input/output ports
    NodePort inPort;
    inPort.id = QUuid::createUuid();
    inPort.name = "Input";
    inPort.type = "float";
    inPort.isInput = true;
    node.inputPortIds.append(inPort.id);

    NodePort outPort;
    outPort.id = QUuid::createUuid();
    outPort.name = "Output";
    outPort.type = "float";
    outPort.isInput = false;
    node.outputPortIds.append(outPort.id);

    m_scene->addItem(item);
    emit graphChanged();
    return nodeId;
}

void NodeGraphWidget::removeSelectedNodes()
{
    for (QGraphicsItem* item : m_scene->selectedItems()) {
        if (auto* node = qgraphicsitem_cast<GraphNodeItem*>(item)) {
            m_scene->removeItem(node);
            delete node;
        }
    }
    emit graphChanged();
}

void NodeGraphWidget::duplicateSelectedNodes()
{
    QList<QGraphicsItem*> selected = m_scene->selectedItems();
    for (QGraphicsItem* item : selected) {
        if (auto* node = qgraphicsitem_cast<GraphNodeItem*>(item)) {
            addNode("Copy", node->pos() + QPointF(20, 20));
        }
    }
}

QJsonObject NodeGraphWidget::toJson() const
{
    QJsonObject json;
    json["name"] = "NodeGraph";
    json["version"] = "1.0";

    QJsonArray nodesArray;
    for (QGraphicsItem* item : m_scene->items()) {
        if (auto* nodeItem = qgraphicsitem_cast<GraphNodeItem*>(item)) {
            QJsonObject nodeJson;
            nodeJson["id"] = nodeItem->nodeId().toString();
            nodeJson["position"] = QJsonObject{{"x", nodeItem->pos().x()}, {"y", nodeItem->pos().y()}};
            nodesArray.append(nodeJson);
        }
    }
    json["nodes"] = nodesArray;

    QJsonArray connectionsArray;
    for (QGraphicsItem* item : m_scene->items()) {
        if (auto* connItem = qgraphicsitem_cast<GraphConnectionItem*>(item)) {
            QJsonObject connJson;
            connJson["id"] = connItem->connectionId().toString();
            connectionsArray.append(connJson);
        }
    }
    json["connections"] = connectionsArray;

    return json;
}

bool NodeGraphWidget::fromJson(const QJsonObject& json)
{
    if (json.isEmpty()) return false;

    QJsonArray nodesArray = json["nodes"].toArray();
    for (const auto& nodeVal : nodesArray) {
        QJsonObject nodeJson = nodeVal.toObject();
        QUuid id = QUuid::fromString(nodeJson["id"].toString());
        QJsonObject pos = nodeJson["position"].toObject();
        addNode(id.toString(), QPointF(pos["x"].toDouble(), pos["y"].toDouble()));
    }
    return true;
}

bool NodeGraphWidget::loadFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return false;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    return fromJson(doc.object());
}

bool NodeGraphWidget::saveToFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) return false;
    QJsonDocument doc(toJson());
    file.write(doc.toJson());
    return true;
}

void NodeGraphWidget::findAndSelect(const QString& query)
{
    Q_UNUSED(query);
}

void NodeGraphWidget::centerOnNode(const QUuid& nodeId)
{
    for (QGraphicsItem* item : m_scene->items()) {
        if (auto* nodeItem = qgraphicsitem_cast<GraphNodeItem*>(item)) {
            if (nodeItem->nodeId() == nodeId) {
                m_view->centerOn(nodeItem->pos());
                return;
            }
        }
    }
}

} // namespace ui
} // namespace ks
