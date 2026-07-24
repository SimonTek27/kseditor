#include "NodeGraphScene.h"
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsItem>
#include <QKeyEvent>

namespace ks {
namespace ui {

NodeGraphScene::NodeGraphScene(QObject* parent) : QGraphicsScene(parent)
{
    setSceneRect(-10000, -10000, 20000, 20000);
    m_rubberBand = nullptr;
}

NodeGraphScene::~NodeGraphScene() = default;

void NodeGraphScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        QGraphicsItem* item = itemAt(event->scenePos(), QTransform());
        if (!item && event->modifiers() == Qt::NoModifier) {
            m_rubberBandOrigin = event->scenePos();
            if (!m_rubberBand) {
                m_rubberBand = addRect(QRectF(), QPen(QColor("#E10600"), 1), QBrush(QColor(225, 6, 0, 30)));
            }
            m_rubberBand->setVisible(true);
            m_rubberBand->setZValue(1000);
            return;
        }
    }
    QGraphicsScene::mousePressEvent(event);
}

void NodeGraphScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_rubberBand && m_rubberBand->isVisible()) {
        QRectF rect = QRectF(m_rubberBandOrigin, event->scenePos()).normalized();
        m_rubberBand->setRect(rect);
        return;
    }
    QGraphicsScene::mouseMoveEvent(event);
}

void NodeGraphScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_rubberBand && m_rubberBand->isVisible()) {
        QRectF selectRect = m_rubberBand->rect();
        m_rubberBand->setVisible(false);
        setSelectionArea(QPainterPath());
        for (auto* item : items(selectRect, Qt::IntersectsItemShape, Qt::DescendingOrder)) {
            if (item->flags() & QGraphicsItem::ItemIsSelectable) {
                item->setSelected(true);
            }
        }
        return;
    }
    QGraphicsScene::mouseReleaseEvent(event);
}

} // namespace ui
} // namespace ks