#pragma once

#include "../../../core/editor/EditorModule.h"
#include <QDockWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QTabWidget>

namespace ks {

struct SurfaceData {
    QString key;
    float friction = 0.8f;
    float damping = 0.0f;
    QString wav;
    float wavPitch = 0.0f;
    QString ffEffect;
    float dirtAdditive = 0.0f;
    bool isValidTrack = true;
    float blackFlagTime = 0.0f;
    float sinHeight = 0.0f;
    float sinLength = 0.0f;
    bool isPitlane = false;
    float vibrationGain = 0.0f;
    float vibrationLength = 0.0f;
};

class TrackSurfaceEditorModule : public EditorModule {
    Q_OBJECT
public:
    explicit TrackSurfaceEditorModule(QWidget* parent = nullptr);
    ~TrackSurfaceEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "Track Surface Editor"; }
    QString moduleId() const override { return "trackSurfaceEditor"; }
    int getModulePriority() const override { return 28; }
    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;

protected:
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onSurfaceSelected(int row);
    void onAddSurface();
    void onRemoveSurface();
    void onDuplicateSurface();
    void onFrictionChanged(double value);
    void onDampingChanged(double value);
    void onSoundFileChanged(const QString& text);
    void onSoundPitchChanged(double value);
    void onFFEffectChanged(const QString& text);
    void onDirtAdditiveChanged(double value);
    void onIsValidTrackToggled(bool checked);
    void onBlackFlagTimeChanged(double value);
    void onSinHeightChanged(double value);
    void onSinLengthChanged(double value);
    void onIsPitlaneToggled(bool checked);
    void onVibrationGainChanged(double value);
    void onVibrationLengthChanged(double value);
    void onLoadFile();
    void onSaveFile();
    void onResetDefaults();
    void onSurfaceKeyChanged(const QString& text);

private:
    void setupUi();
    void loadFileToUI();
    void saveFileFromUI();
    void updateSurfaceTable();
    void selectSurface(int index);

    QDockWidget* m_dockWidget = nullptr;
    QTabWidget* m_tabWidget = nullptr;

    // Surface table
    QTableWidget* m_surfaceTable = nullptr;
    QPushButton* m_addSurfaceBtn = nullptr;
    QPushButton* m_removeSurfaceBtn = nullptr;
    QPushButton* m_duplicateSurfaceBtn = nullptr;

    // Surface properties
    QLineEdit* m_surfaceKeyEdit = nullptr;
    QDoubleSpinBox* m_frictionSpin = nullptr;
    QDoubleSpinBox* m_dampingSpin = nullptr;
    QLineEdit* m_soundFileEdit = nullptr;
    QDoubleSpinBox* m_soundPitchSpin = nullptr;
    QComboBox* m_ffEffectCombo = nullptr;
    QDoubleSpinBox* m_dirtAdditiveSpin = nullptr;
    QCheckBox* m_isValidTrackCheck = nullptr;
    QDoubleSpinBox* m_blackFlagTimeSpin = nullptr;
    QDoubleSpinBox* m_sinHeightSpin = nullptr;
    QDoubleSpinBox* m_sinLengthSpin = nullptr;
    QCheckBox* m_isPitlaneCheck = nullptr;
    QDoubleSpinBox* m_vibrationGainSpin = nullptr;
    QDoubleSpinBox* m_vibrationLengthSpin = nullptr;

    // Actions
    QPushButton* m_loadBtn = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_resetBtn = nullptr;
    QLabel* m_statusLabel = nullptr;

    // Data
    QVector<SurfaceData> m_surfaces;
    int m_selectedSurfaceIndex = -1;
    QString m_filePath;
};

} // namespace ks
