#include "EditorUI.h"
#include <QUuid>

namespace ks { namespace ui {

void EditorWindow::setGeometry(int x, int y, int width, int height) {
    m_x = x; m_y = y; m_width = width; m_height = height;
}

void EditorWindow::show() {
    m_visible = true;
    emit windowShown();
}

void EditorWindow::hide() {
    m_visible = false;
    emit windowHidden();
}

void EditorWindow::close() {
    m_visible = false;
    emit windowClosed();
}

void MenuBar::addMenu(const QString& name) {
    m_menus[name] = QStringList();
}

void MenuBar::addAction(const QString& menu, const QString& action, const QString& shortcut) {
    if (m_menus.contains(menu)) {
        m_menus[menu].append(action);
        if (!shortcut.isEmpty()) m_shortcuts[action] = shortcut;
    }
}

void Toolbar::addButton(const QString& id, const QString& icon, const QString& tooltip) {
    m_buttons.append({id, icon, tooltip});
}

void Toolbar::addSeparator() {
    m_buttons.append({"__separator__", "", ""});
}

void Toolbar::addDropdown(const QString& id, const QStringList& options) {
    m_dropdowns[id] = options;
}

void PropertyPanel::addProperty(const QString& name, const QString& type, const QVariant& defaultValue) {
    Property p;
    p.name = name;
    p.type = type;
    p.value = defaultValue;
    m_properties[name] = p;
}

void PropertyPanel::setValue(const QString& property, const QVariant& value) {
    if (m_properties.contains(property)) {
        m_properties[property].value = value;
        emit propertyChanged(property, value);
    }
}

QVariant PropertyPanel::getValue(const QString& property) const {
    return m_properties.value(property).value;
}

void PropertyPanel::clear() {
    m_properties.clear();
    m_object = nullptr;
}

void Timeline::play() { m_state = Playing; emit stateChanged(m_state); }
void Timeline::pause() { m_state = Paused; emit stateChanged(m_state); }
void Timeline::stop() { m_state = Stopped; m_currentTime = 0; emit stateChanged(m_state); }

void Timeline::setPlayhead(float time) {
    m_currentTime = qBound(0.0f, time, m_duration);
    emit timeChanged(m_currentTime);
}

void Viewport::focusOn(const QVector3D& target, float distance) {
    m_focusTarget = target;
    m_focusDistance = distance;
}

void NodeGraph::addNode(const QString& type, const QString& title, const QPointF& pos) {
    Node node;
    node.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    node.type = type;
    node.title = title;
    node.position = pos;
    m_nodes[node.id] = node;
    emit nodeAdded(node.id);
}

void NodeGraph::removeNode(const QString& nodeId) {
    if (m_nodes.remove(nodeId)) {
        for (int i = m_connections.size() - 1; i >= 0; --i) {
            if (m_connections[i].fromNode == nodeId || m_connections[i].toNode == nodeId) {
                m_connections.removeAt(i);
            }
        }
        emit nodeRemoved(nodeId);
    }
}

void NodeGraph::connect(const QString& fromNode, const QString& fromSocket,
                        const QString& toNode, const QString& toSocket) {
    Connection c;
    c.fromNode = fromNode; c.fromSocket = fromSocket;
    c.toNode = toNode; c.toSocket = toSocket;
    m_connections.append(c);
    emit connectionAdded();
}

void NodeGraph::disconnect(const QString& fromNode, const QString& fromSocket,
                          const QString& toNode, const QString& toSocket) {
    for (int i = m_connections.size() - 1; i >= 0; --i) {
        const Connection& c = m_connections[i];
        if (c.fromNode == fromNode && c.fromSocket == fromSocket &&
            c.toNode == toNode && c.toSocket == toSocket) {
            m_connections.removeAt(i);
            emit connectionAdded();
            return;
        }
    }
}

void NodeGraph::selectNode(const QString& nodeId, bool addToSelection) {
    for (auto& node : m_nodes) {
        node.selected = (node.id == nodeId && addToSelection) ? true : false;
    }
    emit selectionChanged();
}

void NodeGraph::deleteSelected() {
    QList<QString> toDelete;
    for (const Node& node : m_nodes.values()) {
        if (node.selected) toDelete.append(node.id);
    }
    for (const QString& id : toDelete) removeNode(id);
}

}} // ks::ui