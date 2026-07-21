#pragma once

#include "core/editor/ModuleGuiBase.h"
#include <QTabWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QGroupBox>
#include <QFormLayout>
#include <QScrollArea>
#include <QSlider>
#include <QProgressBar>

namespace ks {
namespace ffb {

class FfbEditorModule : public ModuleGuiBase {
    Q_OBJECT
public:
    explicit FfbEditorModule(QWidget* parent = nullptr);
    ~FfbEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;

    QString moduleName() const override { return "Force Feedback Editor"; }
    QString moduleId() const override { return "ffbEditor"; }
    int getModulePriority() const override { return 48; }

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

protected:
    void buildUI() override;
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onDeviceSelected(int index);
    void onEffectTypeChanged(int index);
    void onGainChanged(double value);
    void onSpringStrengthChanged(double value);
    void onDamperStrengthChanged(double value);
    void onFrictionChanged(double value);
    void onInertiaChanged(double value);
    void onTestFFB();
    void onStopFFB();
    void onLoadProfile();
    void onSaveProfile();
    void onResetDefaults();

private:
    void setupDeviceTab();
    void setupEffectsTab();
    void setupProfilesTab();
    void populateDeviceList();

    QTabWidget* m_tabWidget = nullptr;

    QWidget* m_deviceTab = nullptr;
    QComboBox* m_deviceCombo = nullptr;
    QLabel* m_deviceInfoLabel = nullptr;
    QPushButton* m_testBtn = nullptr;
    QPushButton* m_stopBtn = nullptr;
    QProgressBar* m_testProgress = nullptr;

    QWidget* m_effectsTab = nullptr;
    QComboBox* m_effectTypeCombo = nullptr;
    QDoubleSpinBox* m_gainSpin = nullptr;
    QDoubleSpinBox* m_springSpin = nullptr;
    QDoubleSpinBox* m_damperSpin = nullptr;
    QDoubleSpinBox* m_frictionSpin = nullptr;
    QDoubleSpinBox* m_inertiaSpin = nullptr;
    QSlider* m_springSlider = nullptr;
    QSlider* m_damperSlider = nullptr;
    QSlider* m_frictionSlider = nullptr;

    QWidget* m_profilesTab = nullptr;
    QTableWidget* m_profileTable = nullptr;
    QPushButton* m_loadProfileBtn = nullptr;
    QPushButton* m_saveProfileBtn = nullptr;
    QPushButton* m_resetDefaultsBtn = nullptr;
};

} // namespace ffb
} // namespace ks
