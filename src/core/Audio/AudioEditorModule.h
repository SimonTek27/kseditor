#pragma once

#include "core/editor/ModuleGuiBase.h"
#include <QTabWidget>
#include <QTreeWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QLabel>
#include <QSplitter>
#include <QGroupBox>
#include <QFormLayout>
#include <QScrollArea>
#include <QListWidget>
#include <QProgressBar>

namespace ks {
namespace audio {

class AudioEditorModule : public ModuleGuiBase {
    Q_OBJECT
public:
    explicit AudioEditorModule(QWidget* parent = nullptr);
    ~AudioEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;

    QString moduleName() const override { return "Audio Editor"; }
    QString moduleId() const override { return "audio"; }
    int getModulePriority() const override { return 65; }

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

protected:
    void buildUI() override;
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onLoadAudioFile();
    void onPlayAudio();
    void onStopAudio();
    void onRecordAudio();
    void onVolumeChanged(int value);
    void onAddEffect();
    void onRemoveEffect();
    void onEffectSelected(QTreeWidgetItem* item, int column);
    void onBypassEffect(QTreeWidgetItem* item, int column);
    void onEffectParamChanged();
    void onConnectToSim();
    void onDisconnectFromSim();
    void onAddSoundBank();
    void onRemoveSoundBank();
    void onExportSoundBank();
    void onImportSoundBank();
    void onBankSelected(QTreeWidgetItem* item, int column);
    void onAddSample();
    void onRemoveSample();
    void onAddTrack();
    void onRemoveTrack();
    void onMixerLevelChanged(int channel, int value);
    void onMasterVolumeChanged(int value);
    void onOutputDeviceChanged(int index);
    void onSampleRateChanged(int index);
    void onBufferSizeChanged(int index);

private:
    void setupEngineSoundTab();
    void setupRecordingTab();
    void setupEffectsTab();
    void setupSoundBanksTab();
    void setupSettingsTab();
    void refreshEffects();
    void refreshBanks();
    void refreshSamples();

    QTabWidget* m_tabWidget = nullptr;

    QWidget* m_engineSoundTab = nullptr;
    QTreeWidget* m_trackTree = nullptr;
    QPushButton* m_addTrackBtn = nullptr;
    QPushButton* m_removeTrackBtn = nullptr;
    QPushButton* m_playBtn = nullptr;
    QPushButton* m_stopBtn = nullptr;
    QSlider* m_volumeSlider = nullptr;
    QLabel* m_volumeLabel = nullptr;
    QPushButton* m_loadAudioBtn = nullptr;
    QLabel* m_audioInfoLabel = nullptr;
    QListWidget* m_mixerList = nullptr;
    QSlider* m_masterVolumeSlider = nullptr;
    QLabel* m_masterVolumeLabel = nullptr;
    QLabel* m_waveformLabel = nullptr;

    QWidget* m_recordingTab = nullptr;
    QPushButton* m_recordBtn = nullptr;
    QLabel* m_recordingStatusLabel = nullptr;
    QComboBox* m_inputDeviceCombo = nullptr;
    QComboBox* m_sampleRateCombo = nullptr;
    QSpinBox* m_recordingDurationSpin = nullptr;
    QSlider* m_inputLevelSlider = nullptr;
    QLabel* m_inputLevelLabel = nullptr;
    QCheckBox* m_monitorInputCheck = nullptr;
    QLabel* m_recordingInfoLabel = nullptr;

    QWidget* m_effectsTab = nullptr;
    QTreeWidget* m_effectChainTree = nullptr;
    QPushButton* m_addEffectBtn = nullptr;
    QPushButton* m_removeEffectBtn = nullptr;
    QComboBox* m_effectTypeCombo = nullptr;
    QGroupBox* m_effectParamGroup = nullptr;
    QFormLayout* m_effectParamLayout = nullptr;
    QLabel* m_effectInfoLabel = nullptr;

    QWidget* m_soundBanksTab = nullptr;
    QTreeWidget* m_bankTree = nullptr;
    QPushButton* m_addBankBtn = nullptr;
    QPushButton* m_removeBankBtn = nullptr;
    QPushButton* m_exportBankBtn = nullptr;
    QPushButton* m_importBankBtn = nullptr;
    QListWidget* m_sampleList = nullptr;
    QPushButton* m_addSampleBtn = nullptr;
    QPushButton* m_removeSampleBtn = nullptr;
    QLabel* m_bankInfoLabel = nullptr;

    QWidget* m_settingsTab = nullptr;
    QComboBox* m_outputDeviceCombo = nullptr;
    QComboBox* m_settingsSampleRateCombo = nullptr;
    QComboBox* m_bufferSizeCombo = nullptr;
    QCheckBox* m_exclusiveModeCheck = nullptr;
    QCheckBox* m_autoConnectCheck = nullptr;
    QSpinBox* m_latencySpin = nullptr;
    QPushButton* m_connectToSimBtn = nullptr;
    QPushButton* m_disconnectFromSimBtn = nullptr;
    QLabel* m_connectionStatusLabel = nullptr;
};

} // namespace audio
} // namespace ks
