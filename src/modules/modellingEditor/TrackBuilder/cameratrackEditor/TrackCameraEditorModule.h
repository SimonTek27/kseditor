#pragma once

#include "../../../../core/editor/EditorModule.h"
#include <QDockWidget>
#include <QTableWidget>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>

namespace ks {

struct TrackCamera {
    enum class Type { TV, Onboard, Heli, Chase, Cockpit };
    enum class Shake { None, Low, Medium, High, Full };

    int id = 0;
    Type type = Type::TV;
    QString name;
    float position[3] = {0, 0, 0};
    float lookAt[3] = {0, 0, 0};
    float fov = 45.0f;
    float nearClip = 0.1f;
    float farClip = 5000.0f;
    float pitch = 0.0f;
    float yaw = 0.0f;
    float roll = 0.0f;
    float forwardOffset = 0.0f;
    float upOffset = 0.0f;
    float maxSpeed = 0.0f;
    Shake shake = Shake::Medium;
    bool isActive = true;
};

class TrackCameraEditorModule : public EditorModule {
    Q_OBJECT
public:
    explicit TrackCameraEditorModule(QWidget* parent = nullptr);
    ~TrackCameraEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "Camera Editor"; }
    QString moduleId() const override { return "cameraTrackEditor"; }
    int getModulePriority() const override { return 37; }
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
    void onTypeChanged(int index);
    void onPosXChanged(double v);
    void onPosYChanged(double v);
    void onPosZChanged(double v);
    void onLookXChanged(double v);
    void onLookYChanged(double v);
    void onLookZChanged(double v);
    void onFovChanged(double v);
    void onNearClipChanged(double v);
    void onFarClipChanged(double v);
    void onPitchChanged(double v);
    void onYawChanged(double v);
    void onRollChanged(double v);
    void onForwardOffsetChanged(double v);
    void onUpOffsetChanged(double v);
    void onMaxSpeedChanged(double v);
    void onShakeChanged(int index);
    void onActiveChanged(bool checked);
    void onLoadFile();
    void onSaveFile();
    void onResetDefaults();

private:
    void setupUi();
    void loadFileToUI();
    void saveFileFromUI();
    void updateTable();
    void selectCamera(int idx);
    void updatePropEditableState();

    static TrackCamera::Type typeFromString(const QString& s);
    static QString typeToString(TrackCamera::Type t);
    static TrackCamera::Shake shakeFromString(const QString& s);
    static QString shakeToString(TrackCamera::Shake s);

    QDockWidget* m_dockWidget = nullptr;
    QTableWidget* m_cameraTable = nullptr;
    QPushButton* m_addBtn = nullptr;
    QPushButton* m_removeBtn = nullptr;

    QComboBox* m_typeCombo = nullptr;
    QDoubleSpinBox* m_posXSpin = nullptr;
    QDoubleSpinBox* m_posYSpin = nullptr;
    QDoubleSpinBox* m_posZSpin = nullptr;
    QDoubleSpinBox* m_lookXSpin = nullptr;
    QDoubleSpinBox* m_lookYSpin = nullptr;
    QDoubleSpinBox* m_lookZSpin = nullptr;
    QDoubleSpinBox* m_fovSpin = nullptr;
    QDoubleSpinBox* m_nearSpin = nullptr;
    QDoubleSpinBox* m_farSpin = nullptr;
    QDoubleSpinBox* m_pitchSpin = nullptr;
    QDoubleSpinBox* m_yawSpin = nullptr;
    QDoubleSpinBox* m_rollSpin = nullptr;
    QDoubleSpinBox* m_forwardSpin = nullptr;
    QDoubleSpinBox* m_upSpin = nullptr;
    QDoubleSpinBox* m_maxSpeedSpin = nullptr;
    QComboBox* m_shakeCombo = nullptr;
    QCheckBox* m_activeCheck = nullptr;
    QPushButton* m_loadBtn = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_resetBtn = nullptr;
    QLabel* m_statusLabel = nullptr;

    QVector<TrackCamera> m_cameras;
    int m_selectedIndex = -1;
    QString m_filePath;
};

} // namespace ks
