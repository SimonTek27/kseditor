#include "SceneObjectListModel.h"

namespace ks {

SceneObjectListModel::SceneObjectListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

void SceneObjectListModel::setSceneGraph(SceneGraph* scene) {
    m_scene = scene;
    syncObjectList();
    emit countChanged();
}

void SceneObjectListModel::syncObjectList() {
    beginResetModel();
    m_objects.clear();
    if (m_scene) {
        auto all = m_scene->allObjects();
        for (SceneObject* obj : all) {
            if (obj->type() == SceneObject::Type::Mesh && obj->hasMesh()) {
                m_objects.append(obj);
            }
        }
    }
    endResetModel();
}

int SceneObjectListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_objects.size();
}

QVariant SceneObjectListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_objects.size())
        return QVariant();

    SceneObject* obj = m_objects[index.row()];
    if (!obj) return QVariant();

    switch (role) {
    case NameRole:
        return obj->name();
    case ObjectIdRole:
        return obj->id();
    case TypeRole: {
        switch (obj->type()) {
        case SceneObject::Type::Node: return "Node";
        case SceneObject::Type::Mesh: return "Mesh";
        case SceneObject::Type::Light: return "Light";
        case SceneObject::Type::Camera: return "Camera";
        case SceneObject::Type::Bone: return "Bone";
        default: return "Unknown";
        }
    }
    case PositionRole: {
        auto t = obj->transform().translation();
        return QVector3D(t.x, t.y, t.z);
    }
    case RotationRole: {
        auto r = obj->transform().rotation();
        return QVector3D(r.x, r.y, r.z);
    }
    case ScaleRole: {
        auto s = obj->transform().scale();
        return QVector3D(s.x, s.y, s.z);
    }
    case SelectedRole:
        return obj->isSelected();
    case VertexCountRole: {
        if (obj->mesh()) return (int)obj->mesh()->vertices().size();
        return 0;
    }
    case TriangleCountRole: {
        if (obj->mesh()) return (int)(obj->mesh()->indices().size() / 3);
        return 0;
    }
    case HasMeshRole:
        return obj->hasMesh();
    case VisibleRole:
        return obj->isVisible();
    case ObjectPtrRole:
        return QVariant::fromValue(reinterpret_cast<quintptr>(obj));
    case BaseColorRole:
        return QVariant::fromValue(obj->baseColor());
    case MetallicRole:
        return obj->metallic();
    case RoughnessRole:
        return obj->roughness();
    case OpacityRole:
        return obj->opacity();
    }
    return QVariant();
}

QHash<int, QByteArray> SceneObjectListModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "objectName";
    roles[ObjectIdRole] = "objectId";
    roles[TypeRole] = "objectType";
    roles[PositionRole] = "objectPosition";
    roles[RotationRole] = "objectRotation";
    roles[ScaleRole] = "objectScale";
    roles[SelectedRole] = "objectSelected";
    roles[VertexCountRole] = "vertexCount";
    roles[TriangleCountRole] = "triangleCount";
    roles[HasMeshRole] = "hasMesh";
    roles[VisibleRole] = "objectVisible";
    roles[BaseColorRole] = "baseColor";
    roles[MetallicRole] = "metallic";
    roles[RoughnessRole] = "roughness";
    roles[OpacityRole] = "opacity";
    return roles;
}

int SceneObjectListModel::objectIdAt(int row) const {
    if (row < 0 || row >= m_objects.size()) return -1;
    return m_objects[row] ? m_objects[row]->id() : -1;
}

QVector3D SceneObjectListModel::positionAt(int row) const {
    if (row < 0 || row >= m_objects.size()) return QVector3D();
    auto t = m_objects[row]->transform().translation();
    return QVector3D(t.x, t.y, t.z);
}

QString SceneObjectListModel::nameAt(int row) const {
    if (row < 0 || row >= m_objects.size()) return QString();
    return m_objects[row]->name();
}

void SceneObjectListModel::refresh() {
    syncObjectList();
    emit countChanged();
}

void SceneObjectListModel::onSceneChanged() {
    refresh();
}

int SceneObjectListModel::totalVertices() const {
    int count = 0;
    for (SceneObject* obj : m_objects) {
        if (obj && obj->mesh())
            count += obj->mesh()->vertices().size();
    }
    return count;
}

int SceneObjectListModel::totalTriangles() const {
    int count = 0;
    for (SceneObject* obj : m_objects) {
        if (obj && obj->mesh())
            count += obj->mesh()->indices().size() / 3;
    }
    return count;
}

} // namespace ks
