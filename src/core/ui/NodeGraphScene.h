#pragma once

#include <QGraphicsScene>
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
};

} // namespace ui
} // namespace ks