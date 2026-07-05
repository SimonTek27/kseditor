#pragma once

#include <QAbstractListModel>
#include <QVector3D>
#include <QVector>
#include <QColor>
#include "core/Graphics/SceneGraph.h"
#include "core/Graphics/SceneObject.h"
#include "core/Graphics/SceneMesh.h"

namespace ks {

class SceneObjectListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int totalVertices READ totalVertices NOTIFY countChanged)
    Q_PROPERTY(int totalTriangles READ totalTriangles NOTIFY countChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        ObjectIdRole,
        TypeRole,
        PositionRole,
        RotationRole,
        ScaleRole,
        SelectedRole,
        VertexCountRole,
        TriangleCountRole,
        HasMeshRole,
        VisibleRole,
        ObjectPtrRole,
        BaseColorRole,
        MetallicRole,
        RoughnessRole,
        OpacityRole
    };

    explicit SceneObjectListModel(QObject* parent = nullptr);

    void setSceneGraph(SceneGraph* scene);
    SceneGraph* sceneGraph() const { return m_scene; }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE int objectIdAt(int row) const;
    Q_INVOKABLE QVector3D positionAt(int row) const;
    Q_INVOKABLE QString nameAt(int row) const;

    int totalVertices() const;
    int totalTriangles() const;

    Q_INVOKABLE void refresh();

signals:
    void countChanged();

private slots:
    void onSceneChanged();

private:
    SceneGraph* m_scene = nullptr;
    QVector<SceneObject*> m_objects;
    void syncObjectList();
};

} // namespace ks
