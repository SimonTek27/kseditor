#include "NodeGraphScene.h"
#include <QGraphicsSceneMouseEvent>

namespace ks {
namespace ui {

NodeGraphScene::NodeGraphScene(QObject* parent) : QGraphicsScene(parent)
{
    setSceneRect(-10000, -10000, 20000, 20000);
}

void NodeGraphScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsScene::mousePressEvent(event);
}

void NodeGraphScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsScene::mouseMoveEvent(event);
}

void NodeGraphScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsScene::mouseReleaseEvent(event);
}

NodeGraphScene::~NodeGraphScene() = default;

} // namespace ui
} // namespace ks