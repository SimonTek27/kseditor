#include "FfbEditorQmlBridge.h"
#include "FfbConfigTool.h"
#include "../sys/LogManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

namespace ks {

FfbEditorQmlBridge* FfbEditorQmlBridge::s_instance = nullptr;

FfbEditorQmlBridge* FfbEditorQmlBridge::instance() {
    if (!s_instance) {
        s_instance = new FfbEditorQmlBridge();
    }
    return s_instance;
}

void FfbEditorQmlBridge::setGain(float v) {
    if (qFuzzyCompare(m_settings.gain, v)) return;
    m_settings.gain = v;
    markUnsaved();
    emit settingsChanged();
}

void FfbEditorQmlBridge::setFilter(float v) {
    if (qFuzzyCompare(m_settings.filter, v)) return;
    m_settings.filter = v;
    markUnsaved();
    emit settingsChanged();
}

void FfbEditorQmlBridge::setMinimumForce(float v) {
    if (qFuzzyCompare(m_settings.minimumForce, v)) return;
    m_settings.minimumForce = v;
    markUnsaved();
    emit settingsChanged();
}

void FfbEditorQmlBridge::setKerbEffect(float v) {
    if (qFuzzyCompare(m_settings.kerbEffect, v)) return;
    m_settings.kerbEffect = v;
    markUnsaved();
    emit settingsChanged();
}

void FfbEditorQmlBridge::setRoadEffect(float v) {
    if (qFuzzyCompare(m_settings.roadEffect, v)) return;
    m_settings.roadEffect = v;
    markUnsaved();
    emit settingsChanged();
}

void FfbEditorQmlBridge::setSlipEffect(float v) {
    if (qFuzzyCompare(m_settings.slipEffect, v)) return;
    m_settings.slipEffect = v;
    markUnsaved();
    emit settingsChanged();
}

void FfbEditorQmlBridge::setAbsEffect(float v) {
    if (qFuzzyCompare(m_settings.absEffect, v)) return;
    m_settings.absEffect = v;
    markUnsaved();
    emit settingsChanged();
}

void FfbEditorQmlBridge::setEnhUndersteer(float v) {
    if (qFuzzyCompare(m_settings.enhUndersteer, v)) return;
    m_settings.enhUndersteer = v;
    markUnsaved();
    emit settingsChanged();
}

void FfbEditorQmlBridge::setCentreBoostGain(float v) {
    if (qFuzzyCompare(m_settings.centreBoostGain, v)) return;
    m_settings.centreBoostGain = v;
    markUnsaved();
    emit settingsChanged();
}

void FfbEditorQmlBridge::setCentreBoostRange(float v) {
    if (qFuzzyCompare(m_settings.centreBoostRange, v)) return;
    m_settings.centreBoostRange = v;
    markUnsaved();
    emit settingsChanged();
}

void FfbEditorQmlBridge::setEnableGyro(bool v) {
    if (m_settings.enableGyro == v) return;
    m_settings.enableGyro = v;
    markUnsaved();
    emit settingsChanged();
}

void FfbEditorQmlBridge::setGyroStrength(float v) {
    if (qFuzzyCompare(m_settings.gyroStrength, v)) return;
    m_settings.gyroStrength = v;
    markUnsaved();
    emit settingsChanged();
}

bool FfbEditorQmlBridge::loadSettings(const QString& path) {
    m_settings = FfbConfigTool::loadSettings(path);
    m_currentFile = path;
    m_hasUnsavedChanges = false;
    emit currentFileChanged();
    emit unsavedChangesChanged();
    emit settingsChanged();
    emit settingsLoaded(path);
    LOG_INFO("FfbEditorQmlBridge", QString("Loaded settings from: %1").arg(path));
    return true;
}

bool FfbEditorQmlBridge::saveSettings(const QString& path) {
    QString savePath = path.isEmpty() ? m_currentFile : path;
    if (savePath.isEmpty()) {
        LOG_ERROR("FfbEditorQmlBridge", "No path specified for save");
        return false;
    }

    if (!FfbConfigTool::saveSettings(m_settings, savePath)) {
        return false;
    }

    m_currentFile = savePath;
    m_hasUnsavedChanges = false;
    emit currentFileChanged();
    emit unsavedChangesChanged();
    emit settingsSaved(savePath);
    return true;
}

bool FfbEditorQmlBridge::loadSettingsFromAc(const QString& acPath) {
    QString settingsPath = acPath + "/cfg/cfg.ini";
    return loadSettings(settingsPath);
}

bool FfbEditorQmlBridge::saveSettingsToAc(const QString& acPath) {
    QString settingsPath = acPath + "/cfg/cfg.ini";
    return saveSettings(settingsPath);
}

QVariantList FfbEditorQmlBridge::getPresets() {
    QVariantList result;
    auto presets = FfbConfigTool::getPresets();
    for (const auto& preset : presets) {
        QVariantMap m;
        m["name"] = preset.name;
        m["wheelModel"] = preset.wheelModel;
        m["manufacturer"] = preset.manufacturer;
        m["description"] = preset.description;
        m["isBuiltIn"] = preset.isBuiltIn;
        result.append(m);
    }
    return result;
}

bool FfbEditorQmlBridge::applyPreset(const QString& presetName) {
    FfbConfigTool::WheelPreset preset = FfbConfigTool::getPreset(presetName);
    if (preset.name.isEmpty()) {
        emit validationFailed(QString("Preset not found: %1").arg(presetName));
        return false;
    }

    FfbConfigTool::applyPreset(m_settings, preset);
    markUnsaved();
    emit settingsChanged();
    emit presetApplied(presetName);
    return true;
}

bool FfbEditorQmlBridge::saveCustomPreset(const QString& name, const QString& wheelModel, const QString& manufacturer, const QString& description) {
    FfbConfigTool::WheelPreset preset;
    preset.name = name;
    preset.wheelModel = wheelModel;
    preset.manufacturer = manufacturer;
    preset.description = description;
    preset.isBuiltIn = false;
    preset.settings = m_settings;

    return FfbConfigTool::savePreset(preset);
}

QVariantMap FfbEditorQmlBridge::getPresetSettings(const QString& presetName) {
    FfbConfigTool::WheelPreset preset = FfbConfigTool::getPreset(presetName);
    QVariantMap m;
    m["gain"] = static_cast<double>(preset.settings.gain);
    m["filter"] = static_cast<double>(preset.settings.filter);
    m["minimumForce"] = static_cast<double>(preset.settings.minimumForce);
    m["kerbEffect"] = static_cast<double>(preset.settings.kerbEffect);
    m["roadEffect"] = static_cast<double>(preset.settings.roadEffect);
    m["slipEffect"] = static_cast<double>(preset.settings.slipEffect);
    m["absEffect"] = static_cast<double>(preset.settings.absEffect);
    m["enhUndersteer"] = static_cast<double>(preset.settings.enhUndersteer);
    m["centreBoostGain"] = static_cast<double>(preset.settings.centreBoostGain);
    m["centreBoostRange"] = static_cast<double>(preset.settings.centreBoostRange);
    m["enableGyro"] = preset.settings.enableGyro;
    m["gyroStrength"] = static_cast<double>(preset.settings.gyroStrength);
    return m;
}

bool FfbEditorQmlBridge::optimizeForWheel(const QString& wheelModel) {
    FfbConfigTool::optimizeForWheel(m_settings, wheelModel);
    markUnsaved();
    emit settingsChanged();
    return true;
}

QStringList FfbEditorQmlBridge::getSupportedWheels() {
    return FfbConfigTool::getSupportedWheels();
}

QString FfbEditorQmlBridge::getWheelManufacturer(const QString& wheelModel) {
    return FfbConfigTool::getWheelManufacturer(wheelModel);
}

QVariantMap FfbEditorQmlBridge::validateSettings() {
    QVariantMap result;
    QString error;
    bool valid = FfbConfigTool::validateSettings(m_settings, &error);
    result["valid"] = valid;
    result["error"] = error;
    if (!valid) {
        emit validationFailed(error);
    }
    return result;
}

void FfbEditorQmlBridge::resetToDefaults() {
    m_settings = FfbConfigTool::FfbSettings();
    markUnsaved();
    emit settingsChanged();
}

void FfbEditorQmlBridge::markSaved() {
    m_hasUnsavedChanges = false;
    emit unsavedChangesChanged();
}

void FfbEditorQmlBridge::markUnsaved() {
    if (!m_hasUnsavedChanges) {
        m_hasUnsavedChanges = true;
        emit unsavedChangesChanged();
    }
}

// ============================================================================
// FfbEditorModule
// ============================================================================

FfbEditorModule::FfbEditorModule(QWidget* parent)
    : EditorModule(parent)
{}

bool FfbEditorModule::initialize()
{
    LOG_INFO("FfbEditorModule", "Initializing FFB Editor module");
    return true;
}

void FfbEditorModule::shutdown()
{
    LOG_INFO("FfbEditorModule", "Shutting down FFB Editor module");
}

void FfbEditorModule::importFile(const QString& filePath)
{
    if (auto* bridge = FfbEditorQmlBridge::instance()) {
        bridge->loadSettings(filePath);
    }
}

void FfbEditorModule::exportFile(const QString& filePath)
{
    if (auto* bridge = FfbEditorQmlBridge::instance()) {
        bridge->saveSettings(filePath);
    }
}

QJsonObject FfbEditorModule::serializeProject() const
{
    QJsonObject data;
    auto* bridge = FfbEditorQmlBridge::instance();
    if (bridge) {
        data["currentFile"] = bridge->currentFile();
    }
    return data;
}

void FfbEditorModule::deserializeProject(const QJsonObject& data)
{
    if (data.contains("currentFile")) {
        QString filePath = data["currentFile"].toString();
        if (!filePath.isEmpty()) {
            importFile(filePath);
        }
    }
}

} // namespace ks
