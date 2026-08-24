#pragma once

#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QObject>

namespace ks {
namespace ui {

class NodeGraphScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit NodeGraphScene(QObject* parent = nullptr);
    ~NodeGraphScene() override;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    QGraphicsRectItem* m_rubberBand = nullptr;
    QPointF m_rubberBandOrigin;
};

} // namespace ui
} // namespace ks