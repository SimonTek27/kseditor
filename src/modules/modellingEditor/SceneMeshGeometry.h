#pragma once

#include <QQuick3DGeometry>

namespace ks {

class SceneMeshGeometry : public QQuick3DGeometry {
    Q_OBJECT
    Q_PROPERTY(int objectId READ objectId WRITE setObjectId NOTIFY objectIdChanged)

public:
    explicit SceneMeshGeometry(QQuick3DObject* parent = nullptr);

    int objectId() const { return m_objectId; }
    void setObjectId(int id);

    Q_INVOKABLE void rebuild();

signals:
    void objectIdChanged();

private:
    int m_objectId = -1;
};

} // namespace ks
