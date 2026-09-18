#pragma once

#include <QWidget>
#include <QPainter>
#include <QJsonObject>
#include <QPointF>
#include <QMap>
#include <QVector>
#include <QUuid>

namespace ks {
namespace graphics {

class ShaderGraphNode {
public:
    QUuid id;
    QString type;
    QString label;
    QPointF position;
    QMap<QString, QString> inputs;
    QMap<QString, QString> outputs;
};

class ShaderGraphConnection {
public:
    QUuid fromNodeId;
    QString fromPort;
    QUuid toNodeId;
    QString toPort;
};

class ShaderGraphWidget : public QWidget {
    Q_OBJECT
public:
    explicit ShaderGraphWidget(QWidget* parent = nullptr);
    ~ShaderGraphWidget() override = default;

    void addNode(const QString& type, const QPointF& pos);
    void removeNode(const QUuid& nodeId);
    void addConnection(const QUuid& fromNode, const QString& fromPort,
                       const QUuid& toNode, const QString& toPort);
    void removeConnection(int index);

    void clearAll();
    void loadFromJson(const QJsonObject& json);
    QJsonObject toJson() const;

    QVector<ShaderGraphNode>& nodes() { return m_nodes; }
    QVector<ShaderGraphConnection>& connections() { return m_connections; }

signals:
    void graphChanged();
    void nodeSelected(const QUuid& nodeId);
    void nodeMoved(const QUuid& nodeId, const QPointF& pos);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    QVector<ShaderGraphNode> m_nodes;
    QVector<ShaderGraphConnection> m_connections;
    QUuid m_selectedNode;
    bool m_dragging = false;
    QPointF m_dragOffset;
};

} // namespace graphics
} // namespace ks
