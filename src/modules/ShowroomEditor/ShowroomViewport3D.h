#pragma once

#include <QWidget>
#include <QString>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QCheckBox>

#include "../../core/mesh/Viewport3DSystem.h"
#include "../../core/mesh/MeshRenderer.h"
#include "../../core/Graphics/SceneObject.h"
#include "../../core/Graphics/SceneMesh.h"
#include "ShowroomSystem.h"

namespace ks {

class ShowroomViewport3D : public QWidget {
    Q_OBJECT
public:
    explicit ShowroomViewport3D(QWidget* parent = nullptr);
    ~ShowroomViewport3D() override;

    void loadCarMesh(const QString& filePath);
    void loadCarFromFolder(const QString& carFolder);
    void clearCar();

    void syncConfig(const ShowroomSystem::ShowroomConfig& config);
    void syncCamera(double distance, double height, double angle, double fov);
    void syncLight(const QColor& sunColor, double sunIntensity,
                   const QColor& ambientColor, double ambientIntensity);

    Viewport3DWidget* viewportWidget() const { return m_viewport; }

signals:
    void carLoaded(const QString& name, int vertexCount, int faceCount);
    void loadError(const QString& error);

private slots:
    void onOpenFile();
    void onResetView();
    void onRenderModeChanged(int index);
    void onCameraModeChanged(int index);
    void onMeshLoaded(const QString& name, int vertexCount, int faceCount);
    void onMeshLoadError(const QString& error);

private:
    void buildUI();
    void buildControls();
    void convertToScene();
    void applyColorsFromConfig();
    SceneObject* createSceneNode(const QString& name, SceneObject::Type type,
                                  SceneObject* parent = nullptr);

    Viewport3DWidget* m_viewport = nullptr;
    MeshRenderer* m_meshRenderer = nullptr;
    SceneObject* m_sceneRoot = nullptr;

    QPushButton* m_openBtn = nullptr;
    QPushButton* m_resetViewBtn = nullptr;
    QComboBox* m_renderModeCombo = nullptr;
    QComboBox* m_cameraModeCombo = nullptr;
    QLabel* m_infoLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QCheckBox* m_showGridCheck = nullptr;
    QCheckBox* m_showAxesCheck = nullptr;

    ShowroomSystem::ShowroomConfig m_config;
    QVector<ShowroomSystem::ShowroomCamera> m_cameras;
    QVector<ShowroomSystem::ShowroomLight> m_lights;
    int m_currentVertexCount = 0;
    int m_currentFaceCount = 0;
};

} // namespace ks
