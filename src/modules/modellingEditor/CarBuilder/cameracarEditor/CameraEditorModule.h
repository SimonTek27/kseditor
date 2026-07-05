#pragma once

#include "../../../../core/editor/EditorModule.h"
#include <QDockWidget>
#include <QTableWidget>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QTabWidget>

namespace ks {

struct CameraEditorData {
    float pos[3] = {0, 0, 0};
    float target[3] = {0, 0, 0};
    float up[3] = {0, 1, 0};
    float fov = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    float speed = 1.0f;
    int index = 0;
};

class CameraEditorModule : public EditorModule {
    Q_OBJECT
public:
    explicit CameraEditorModule(QWidget* parent = nullptr);
    ~CameraEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "Camera Editor"; }
    QString moduleId() const override { return "cameraEditor"; }
    int getModulePriority() const override { return 30; }
    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;

protected:
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onCameraSelected(int row);
    void onAddCamera();
    void onRemoveCamera();
    void onDuplicateCamera();
    void onCameraTypeChanged(const QString& type);
    void onPosXChanged(double v);
    void onPosYChanged(double v);
    void onPosZChanged(double v);
    void onTargetXChanged(double v);
    void onTargetYChanged(double v);
    void onTargetZChanged(double v);
    void onFovChanged(double v);
    void onNearChanged(double v);
    void onFarChanged(double v);
    void onSpeedChanged(double v);
    void onLoadFile();
    void onSaveFile();
    void onResetDefaults();
    void onMoveToPosition();

private:
    void setupUi();
    void loadFileToUI();
    void saveFileFromUI();
    void updateCameraTable();
    void selectCamera(int index);

    QDockWidget* m_dockWidget = nullptr;
    QTabWidget* m_tabWidget = nullptr;

    // Camera table
    QTableWidget* m_cameraTable = nullptr;
    QPushButton* m_addCameraBtn = nullptr;
    QPushButton* m_removeCameraBtn = nullptr;
    QPushButton* m_duplicateCameraBtn = nullptr;

    // Camera properties
    QComboBox* m_cameraTypeCombo = nullptr;
    QDoubleSpinBox* m_posXSpin = nullptr;
    QDoubleSpinBox* m_posYSpin = nullptr;
    QDoubleSpinBox* m_posZSpin = nullptr;
    QDoubleSpinBox* m_targetXSpin = nullptr;
    QDoubleSpinBox* m_targetYSpin = nullptr;
    QDoubleSpinBox* m_targetZSpin = nullptr;
    QDoubleSpinBox* m_fovSpin = nullptr;
    QDoubleSpinBox* m_nearSpin = nullptr;
    QDoubleSpinBox* m_farSpin = nullptr;
    QDoubleSpinBox* m_speedSpin = nullptr;

    // Actions
    QPushButton* m_loadBtn = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_resetBtn = nullptr;
    QPushButton* m_moveBtn = nullptr;
    QLabel* m_statusLabel = nullptr;

    // Data
    QVector<CameraEditorData> m_cameras;
    int m_selectedCameraIndex = -1;
    QString m_filePath;
    QString m_cameraType;
};

} // namespace ks
