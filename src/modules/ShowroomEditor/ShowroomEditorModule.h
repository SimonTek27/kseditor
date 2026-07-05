#pragma once

#include "../../core/editor/EditorModule.h"
#include "ShowroomSystem.h"
#include "ShowroomViewport3D.h"
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QColorDialog>
#include <QTabWidget>

namespace ks {

class ShowroomPreviewWidget;

class ShowroomEditorModule : public EditorModule {
    Q_OBJECT
public:
    explicit ShowroomEditorModule(QWidget* parent = nullptr);
    ~ShowroomEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "Showroom Editor"; }
    QString moduleId() const override { return "showroomEditor"; }
    int getModulePriority() const override { return 20; }
    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;

protected:
    void onActivation() override;
    void onDeactivation() override;

public slots:
    void onCameraDistanceChanged(double value);
    void onCameraHeightChanged(double value);
    void onCameraAngleChanged(double value);
    void onCameraFovChanged(double value);
    void onRotateSpeedChanged(double value);
    void onAutoRotateToggled(bool checked);
    void onSunColorClicked();
    void onAmbientColorClicked();
    void onSunIntensityChanged(double value);
    void onAmbientIntensityChanged(double value);
    void onCameraSelected(int index);
    void onAddCamera();
    void onRemoveCamera();
    void onLightSelected(int index);
    void onAddLight();
    void onRemoveLight();
    void onLoadConfig();
    void onSaveConfig();
    void onGeneratePreview();
    void onResetDefaults();

private:
    void setupUi();
    void loadConfigToUI();
    void saveConfigFromUI();
    void updatePreview();

    QDockWidget* m_dockWidget = nullptr;
    QWidget* m_centralWidget = nullptr;

    // Camera settings
    QDoubleSpinBox* m_cameraDistanceSpin = nullptr;
    QDoubleSpinBox* m_cameraHeightSpin = nullptr;
    QDoubleSpinBox* m_cameraAngleSpin = nullptr;
    QDoubleSpinBox* m_cameraFovSpin = nullptr;
    QDoubleSpinBox* m_rotateSpeedSpin = nullptr;
    QCheckBox* m_autoRotateCheck = nullptr;

    // Lighting
    QPushButton* m_sunColorBtn = nullptr;
    QPushButton* m_ambientColorBtn = nullptr;
    QDoubleSpinBox* m_sunIntensitySpin = nullptr;
    QDoubleSpinBox* m_ambientIntensitySpin = nullptr;

    // Camera list
    QListWidget* m_cameraList = nullptr;
    QPushButton* m_addCameraBtn = nullptr;
    QPushButton* m_removeCameraBtn = nullptr;

    // Light list
    QListWidget* m_lightList = nullptr;
    QPushButton* m_addLightBtn = nullptr;
    QPushButton* m_removeLightBtn = nullptr;

    // Actions
    QPushButton* m_loadBtn = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_previewBtn = nullptr;
    QPushButton* m_resetBtn = nullptr;
    QLabel* m_statusLabel = nullptr;

    // Preview
    QTabWidget* m_previewTabs = nullptr;
    ShowroomPreviewWidget* m_previewWidget = nullptr;
    ShowroomViewport3D* m_viewport3D = nullptr;

    // Data
    ShowroomSystem::ShowroomConfig m_config;
    QVector<ShowroomSystem::ShowroomCamera> m_cameras;
    QVector<ShowroomSystem::ShowroomLight> m_lights;
    QString m_configPath;
};

} // namespace ks
