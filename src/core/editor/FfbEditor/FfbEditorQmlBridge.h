#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVariantList>
#include "../EditorModule.h"
#include "../ModuleGuiBase.h"
#include "FfbConfigTool.h"

namespace ks {

class FfbEditorQmlBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(float gain READ gain WRITE setGain NOTIFY settingsChanged)
    Q_PROPERTY(float filter READ filter WRITE setFilter NOTIFY settingsChanged)
    Q_PROPERTY(float minimumForce READ minimumForce WRITE setMinimumForce NOTIFY settingsChanged)
    Q_PROPERTY(float kerbEffect READ kerbEffect WRITE setKerbEffect NOTIFY settingsChanged)
    Q_PROPERTY(float roadEffect READ roadEffect WRITE setRoadEffect NOTIFY settingsChanged)
    Q_PROPERTY(float slipEffect READ slipEffect WRITE setSlipEffect NOTIFY settingsChanged)
    Q_PROPERTY(float absEffect READ absEffect WRITE setAbsEffect NOTIFY settingsChanged)
    Q_PROPERTY(float enhUndersteer READ enhUndersteer WRITE setEnhUndersteer NOTIFY settingsChanged)
    Q_PROPERTY(float centreBoostGain READ centreBoostGain WRITE setCentreBoostGain NOTIFY settingsChanged)
    Q_PROPERTY(float centreBoostRange READ centreBoostRange WRITE setCentreBoostRange NOTIFY settingsChanged)
    Q_PROPERTY(bool enableGyro READ enableGyro WRITE setEnableGyro NOTIFY settingsChanged)
    Q_PROPERTY(float gyroStrength READ gyroStrength WRITE setGyroStrength NOTIFY settingsChanged)
    Q_PROPERTY(QString currentFile READ currentFile NOTIFY currentFileChanged)
    Q_PROPERTY(bool hasUnsavedChanges READ hasUnsavedChanges NOTIFY unsavedChangesChanged)

public:
    static FfbEditorQmlBridge* instance();

    // Properties
    float gain() const { return m_settings.gain; }
    float filter() const { return m_settings.filter; }
    float minimumForce() const { return m_settings.minimumForce; }
    float kerbEffect() const { return m_settings.kerbEffect; }
    float roadEffect() const { return m_settings.roadEffect; }
    float slipEffect() const { return m_settings.slipEffect; }
    float absEffect() const { return m_settings.absEffect; }
    float enhUndersteer() const { return m_settings.enhUndersteer; }
    float centreBoostGain() const { return m_settings.centreBoostGain; }
    float centreBoostRange() const { return m_settings.centreBoostRange; }
    bool enableGyro() const { return m_settings.enableGyro; }
    float gyroStrength() const { return m_settings.gyroStrength; }
    QString currentFile() const { return m_currentFile; }
    bool hasUnsavedChanges() const { return m_hasUnsavedChanges; }

    // Setters
    void setGain(float v);
    void setFilter(float v);
    void setMinimumForce(float v);
    void setKerbEffect(float v);
    void setRoadEffect(float v);
    void setSlipEffect(float v);
    void setAbsEffect(float v);
    void setEnhUndersteer(float v);
    void setCentreBoostGain(float v);
    void setCentreBoostRange(float v);
    void setEnableGyro(bool v);
    void setGyroStrength(float v);

    // Q_INVOKABLE methods
    Q_INVOKABLE bool loadSettings(const QString& path);
    Q_INVOKABLE bool saveSettings(const QString& path);
    Q_INVOKABLE bool loadSettingsFromAc(const QString& acPath);
    Q_INVOKABLE bool saveSettingsToAc(const QString& acPath);
    Q_INVOKABLE QVariantList getPresets();
    Q_INVOKABLE bool applyPreset(const QString& presetName);
    Q_INVOKABLE bool saveCustomPreset(const QString& name, const QString& wheelModel, const QString& manufacturer, const QString& description);
    Q_INVOKABLE QVariantMap getPresetSettings(const QString& presetName);
    Q_INVOKABLE bool optimizeForWheel(const QString& wheelModel);
    Q_INVOKABLE QStringList getSupportedWheels();
    Q_INVOKABLE QString getWheelManufacturer(const QString& wheelModel);
    Q_INVOKABLE QVariantMap validateSettings();
    Q_INVOKABLE void resetToDefaults();
    Q_INVOKABLE void markSaved();

signals:
    void settingsChanged();
    void currentFileChanged();
    void unsavedChangesChanged();
    void settingsLoaded(const QString& path);
    void settingsSaved(const QString& path);
    void presetApplied(const QString& presetName);
    void validationFailed(const QString& error);

private:
    static FfbEditorQmlBridge* s_instance;
    FfbEditorQmlBridge(QObject* parent = nullptr) : QObject(parent) {}

    void markUnsaved();

    FfbConfigTool::FfbSettings m_settings;
    QString m_currentFile;
    bool m_hasUnsavedChanges = false;
};

class FfbEditorModule : public ModuleGuiBase {
    Q_OBJECT
public:
    explicit FfbEditorModule(QWidget* parent = nullptr);
    ~FfbEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "FFB Editor"; }
    QString moduleId() const override { return "ffbEditor"; }
    int getModulePriority() const override { return 40; }

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;

protected:
    void buildUI() override;

private slots:
    void onGainChanged(int v);
    void onFilterChanged(int v);
    void onMinForceChanged(int v);
    void onKerbEffectChanged(int v);
    void onRoadEffectChanged(int v);
    void onSlipEffectChanged(int v);
    void onAbsEffectChanged(int v);
    void onUndersteerChanged(int v);
    void onCenterBoostGainChanged(int v);
    void onCenterBoostRangeChanged(int v);
    void onGyroToggled(bool on);
    void onGyroStrengthChanged(int v);
    void onApplyPreset(const QString& preset);
    void onLoadFromFile();
    void onSaveToFile();
    void onResetDefaults();

private:
    void syncFromBridge();
    void syncToBridge();

    QTabWidget* m_tabWidget = nullptr;

    // Settings tab
    QSlider* m_gainSlider = nullptr;
    QLabel* m_gainLabel = nullptr;
    QSlider* m_filterSlider = nullptr;
    QLabel* m_filterLabel = nullptr;
    QSlider* m_minForceSlider = nullptr;
    QLabel* m_minForceLabel = nullptr;
    QSlider* m_kerbSlider = nullptr;
    QLabel* m_kerbLabel = nullptr;
    QSlider* m_roadSlider = nullptr;
    QLabel* m_roadLabel = nullptr;
    QSlider* m_slipSlider = nullptr;
    QLabel* m_slipLabel = nullptr;
    QSlider* m_absSlider = nullptr;
    QLabel* m_absLabel = nullptr;
    QSlider* m_understeerSlider = nullptr;
    QLabel* m_understeerLabel = nullptr;
    QSlider* m_cenGainSlider = nullptr;
    QLabel* m_cenGainLabel = nullptr;
    QSlider* m_cenRangeSlider = nullptr;
    QLabel* m_cenRangeLabel = nullptr;
    QCheckBox* m_gyroCheck = nullptr;
    QSlider* m_gyroSlider = nullptr;
    QLabel* m_gyroLabel = nullptr;

    // Presets tab
    QListWidget* m_presetList = nullptr;
    QPushButton* m_applyPresetBtn = nullptr;
    QPushButton* m_loadBtn = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_resetBtn = nullptr;
    QTextEdit* m_statusLog = nullptr;
};

} // namespace ks
