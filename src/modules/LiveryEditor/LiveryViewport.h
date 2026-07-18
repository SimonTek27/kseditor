#pragma once

#include <QWidget>
#include <QImage>
#include <QVector3D>

#include "../../core/mesh/Viewport3DSystem.h"
#include "../../core/mesh/MeshRenderer.h"
#include "../../core/Graphics/SceneObject.h"
#include "../../core/Graphics/SceneMesh.h"
#include "LiveryEditorModule.h"

namespace ks {

class LiveryViewport : public QWidget {
    Q_OBJECT
public:
    explicit LiveryViewport(QWidget* parent = nullptr);
    ~LiveryViewport() override;

    void setCarPath(const QString& path);
    void resetCamera();
    void focusOnModel();
    void setViewMode(const QString& mode);

    void applyLiveryTexture(const QImage& texture);

signals:
    void partSelected(const QString& partId);

private:
    void convertToScene();
    void loadMesh(const QString& filePath);

    LiveryEditor* m_liveryEditor;
    Viewport3DWidget* m_viewport = nullptr;
    MeshRenderer* m_meshRenderer = nullptr;
    SceneObject* m_sceneRoot = nullptr;

    QString m_carPath;
    QString m_viewMode = "perspective";
    QImage m_liveryTexture;
};

} // namespace ks
