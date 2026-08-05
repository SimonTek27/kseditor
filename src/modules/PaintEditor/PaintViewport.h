#pragma once

#include <QWidget>
#include <QImage>
#include <QVector3D>

#include "../../core/mesh/Viewport3DSystem.h"
#include "../../core/mesh/MeshRenderer.h"
#include "../../core/Graphics/SceneObject.h"
#include "../../core/Graphics/SceneMesh.h"
#include "PaintEditorModule.h"

namespace ks {
namespace paint {

class PaintViewport : public QWidget {
    Q_OBJECT
public:
    explicit PaintViewport(QWidget* parent = nullptr);
    ~PaintViewport() override;

    void setCarPath(const QString& path);
    void resetCamera();
    void focusOnModel();
    void setViewMode(const QString& mode);

    void applyPaintTexture(const QImage& texture);

signals:
    void partSelected(const QString& partId);

private:
    void convertToScene();
    void loadMesh(const QString& filePath);

    ks::paint::PaintEditor* m_paintEditor;
    Viewport3DWidget* m_viewport = nullptr;
    MeshRenderer* m_meshRenderer = nullptr;
    SceneObject* m_sceneRoot = nullptr;

    QString m_carPath;
    QString m_viewMode = "perspective";
    QImage m_paintTexture;
};

} // namespace paint
} // namespace ks