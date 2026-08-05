#pragma once

#include <QObject>
#include <QString>
#include "BooleanOps.h"

namespace ks {
class SceneGraph;
class SceneObject;

namespace editor {

class BoolOpQmlBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool available READ isAvailable CONSTANT)

public:
    explicit BoolOpQmlBridge(QObject* parent = nullptr);

    bool isAvailable() const;

    void setScene(SceneGraph* scene) { m_scene = scene; }
    SceneGraph* scene() const { return m_scene; }

public slots:
    bool unionMeshes(const QString& meshAId, const QString& meshBId);
    bool differenceMeshes(const QString& meshAId, const QString& meshBId);
    bool intersectMeshes(const QString& meshAId, const QString& meshBId);

    QString getLastResultInfo() const;
    int getResultVertexCount() const;

signals:
    void operationStarted();
    void operationCompleted(bool success, const QString& info);
    void progressUpdated(int percent);

private:
    geometry::GeoMeshData sceneMeshToGeometryMesh(SceneObject* obj) const;
    bool performOp(const QString& meshAId, const QString& meshBId, geometry::BooleanOperations::Operation op);

    SceneGraph* m_scene = nullptr;
    geometry::BoolOpResult m_lastResult;
};

} // namespace ks::editor
} // namespace ks
