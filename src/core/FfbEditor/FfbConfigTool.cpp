#include "FfbConfigTool.h"
#include "../../core/FileFormat/INIParser.h"
#include "../sys/LogManager.h"
#include <cmath>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// Explicit include to ensure WheelPreset is visible
#include "FfbConfigTool.h"

FfbConfigTool::FfbSettings FfbConfigTool::loadSettings(const QString& settingsPath)
{
    FfbSettings settings;

    INIParser ini;
    if (!ini.load(settingsPath)) {
        LOG_WARNING("FfbConfigTool", QString("Failed to load FFB settings from: %1").arg(settingsPath));
        return settings;
    }

    settings.gain = static_cast<float>(ini.real("FFB", "GAIN", 100.0));
    settings.filter = static_cast<float>(ini.real("FFB", "FILTER", 0.0));
    settings.minimumForce = static_cast<float>(ini.real("FFB", "MINIMUM_FORCE", 0.0));
    settings.kerbEffect = static_cast<float>(ini.real("FFB", "KERB_EFFECT", 0.0));
    settings.roadEffect = static_cast<float>(ini.real("FFB", "ROAD_EFFECT", 0.0));
    settings.slipEffect = static_cast<float>(ini.real("FFB", "SLIP_EFFECT", 0.0));
    settings.absEffect = static_cast<float>(ini.real("FFB", "ABS_EFFECT", 0.0));
    settings.enhUndersteer = static_cast<float>(ini.real("FFB", "ENHANCE_UNDERSTEER", 0.0));
    settings.centreBoostGain = static_cast<float>(ini.real("FFB", "CENTRE_BOOST_GAIN", 0.0));
    settings.centreBoostRange = static_cast<float>(ini.real("FFB", "CENTRE_BOOST_RANGE", 0.0));
    settings.enableGyro = ini.boolean("FFB", "ENABLE_GYRO", false);
    settings.gyroStrength = static_cast<float>(ini.real("FFB", "GYRO_STRENGTH", 0.0));

    LOG_INFO("FfbConfigTool", QString("Loaded FFB settings from: %1").arg(settingsPath));
    return settings;
}

bool FfbConfigTool::saveSettings(const FfbSettings& settings, const QString& settingsPath)
{
    INIParser ini;

    if (QFile::exists(settingsPath)) {
        ini.load(settingsPath);
    }

    ini.setValue("FFB", "GAIN", static_cast<double>(settings.gain));
    ini.setValue("FFB", "FILTER", static_cast<double>(settings.filter));
    ini.setValue("FFB", "MINIMUM_FORCE", static_cast<double>(settings.minimumForce));
    ini.setValue("FFB", "KERB_EFFECT", static_cast<double>(settings.kerbEffect));
    ini.setValue("FFB", "ROAD_EFFECT", static_cast<double>(settings.roadEffect));
    ini.setValue("FFB", "SLIP_EFFECT", static_cast<double>(settings.slipEffect));
    ini.setValue("FFB", "ABS_EFFECT", static_cast<double>(settings.absEffect));
    ini.setValue("FFB", "ENHANCE_UNDERSTEER", static_cast<double>(settings.enhUndersteer));
    ini.setValue("FFB", "CENTRE_BOOST_GAIN", static_cast<double>(settings.centreBoostGain));
    ini.setValue("FFB", "CENTRE_BOOST_RANGE", static_cast<double>(settings.centreBoostRange));
    ini.setValue("FFB", "ENABLE_GYRO", settings.enableGyro);
    ini.setValue("FFB", "GYRO_STRENGTH", static_cast<double>(settings.gyroStrength));

    if (!ini.save(settingsPath)) {
        LOG_ERROR("FfbConfigTool", QString("Failed to save FFB settings to: %1").arg(settingsPath));
        return false;
    }

    LOG_INFO("FfbConfigTool", QString("Saved FFB settings to: %1").arg(settingsPath));
    return true;
}

QVector<FfbConfigTool::WheelPreset> FfbConfigTool::getPresets()
{
    QVector<WheelPreset> presets;
    presets.append(getLogitechG27Preset());
    presets.append(getLogitechG29Preset());
    presets.append(getThrustmasterT300Preset());
    presets.append(getThrustmasterT818Preset());
    presets.append(getFanatecCSLPreset());
    presets.append(getFanatecCSW25Preset());
    presets.append(getSimucubePreset());
    return presets;
}

FfbConfigTool::WheelPreset FfbConfigTool::getPreset(const QString& presetName)
{
    auto presets = getPresets();
    for (const auto& preset : presets) {
        if (preset.name == presetName) {
            return preset;
        }
    }
    return WheelPreset();
}

bool FfbConfigTool::applyPreset(FfbSettings& settings, const WheelPreset& preset)
{
    settings = preset.settings;
    LOG_INFO("FfbConfigTool", QString("Applied preset: %1").arg(preset.name));
    return true;
}

bool FfbConfigTool::savePreset(const WheelPreset& preset)
{
    QString presetsDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/ffb_presets";
    QDir().mkpath(presetsDir);

    QString filePath = presetsDir + "/" + preset.name + ".json";

    QJsonObject json;
    json["name"] = preset.name;
    json["wheelModel"] = preset.wheelModel;
    json["manufacturer"] = preset.manufacturer;
    json["description"] = preset.description;
    json["isBuiltIn"] = preset.isBuiltIn;

    QJsonObject settingsJson;
    settingsJson["gain"] = static_cast<double>(preset.settings.gain);
    settingsJson["filter"] = static_cast<double>(preset.settings.filter);
    settingsJson["minimumForce"] = static_cast<double>(preset.settings.minimumForce);
    settingsJson["kerbEffect"] = static_cast<double>(preset.settings.kerbEffect);
    settingsJson["roadEffect"] = static_cast<double>(preset.settings.roadEffect);
    settingsJson["slipEffect"] = static_cast<double>(preset.settings.slipEffect);
    settingsJson["absEffect"] = static_cast<double>(preset.settings.absEffect);
    settingsJson["enhUndersteer"] = static_cast<double>(preset.settings.enhUndersteer);
    settingsJson["centreBoostGain"] = static_cast<double>(preset.settings.centreBoostGain);
    settingsJson["centreBoostRange"] = static_cast<double>(preset.settings.centreBoostRange);
    settingsJson["enableGyro"] = preset.settings.enableGyro;
    settingsJson["gyroStrength"] = static_cast<double>(preset.settings.gyroStrength);
    json["settings"] = settingsJson;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_ERROR("FfbConfigTool", QString("Failed to save preset to: %1").arg(filePath));
        return false;
    }

    file.write(QJsonDocument(json).toJson());
    LOG_INFO("FfbConfigTool", QString("Saved preset: %1 to %2").arg(preset.name, filePath));
    return true;
}

FfbConfigTool::WheelPreset FfbConfigTool::getLogitechG27Preset()
{
    WheelPreset preset;
    preset.name = "Logitech G27";
    preset.wheelModel = "G27";
    preset.manufacturer = "Logitech";
    preset.description = "Logitech G27 racing wheel preset - balanced settings for 900 degree rotation";
    preset.isBuiltIn = true;
    preset.settings.gain = 100.0f;
    preset.settings.filter = 0.0f;
    preset.settings.minimumForce = 0.0f;
    preset.settings.kerbEffect = 40.0f;
    preset.settings.roadEffect = 30.0f;
    preset.settings.slipEffect = 50.0f;
    preset.settings.absEffect = 30.0f;
    preset.settings.enhUndersteer = 0.0f;
    preset.settings.centreBoostGain = 0.0f;
    preset.settings.centreBoostRange = 0.0f;
    preset.settings.enableGyro = false;
    preset.settings.gyroStrength = 0.0f;
    return preset;
}

FfbConfigTool::WheelPreset FfbConfigTool::getLogitechG29Preset()
{
    WheelPreset preset;
    preset.name = "Logitech G29";
    preset.wheelModel = "G29";
    preset.manufacturer = "Logitech";
    preset.description = "Logitech G29/G920 racing wheel preset - optimized for dual-motor force feedback";
    preset.isBuiltIn = true;
    preset.settings.gain = 100.0f;
    preset.settings.filter = 0.0f;
    preset.settings.minimumForce = 5.0f;
    preset.settings.kerbEffect = 50.0f;
    preset.settings.roadEffect = 35.0f;
    preset.settings.slipEffect = 55.0f;
    preset.settings.absEffect = 35.0f;
    preset.settings.enhUndersteer = 10.0f;
    preset.settings.centreBoostGain = 0.0f;
    preset.settings.centreBoostRange = 0.0f;
    preset.settings.enableGyro = false;
    preset.settings.gyroStrength = 0.0f;
    return preset;
}

FfbConfigTool::WheelPreset FfbConfigTool::getThrustmasterT300Preset()
{
    WheelPreset preset;
    preset.name = "Thrustmaster T300";
    preset.wheelModel = "T300RS";
    preset.manufacturer = "Thrustmaster";
    preset.description = "Thrustmaster T300RS preset - belt-driven wheel with strong FFB";
    preset.isBuiltIn = true;
    preset.settings.gain = 85.0f;
    preset.settings.filter = 0.0f;
    preset.settings.minimumForce = 10.0f;
    preset.settings.kerbEffect = 60.0f;
    preset.settings.roadEffect = 45.0f;
    preset.settings.slipEffect = 65.0f;
    preset.settings.absEffect = 40.0f;
    preset.settings.enhUndersteer = 15.0f;
    preset.settings.centreBoostGain = 10.0f;
    preset.settings.centreBoostRange = 30.0f;
    preset.settings.enableGyro = false;
    preset.settings.gyroStrength = 0.0f;
    return preset;
}

FfbConfigTool::WheelPreset FfbConfigTool::getThrustmasterT818Preset()
{
    WheelPreset preset;
    preset.name = "Thrustmaster T818";
    preset.wheelModel = "T818";
    preset.manufacturer = "Thrustmaster";
    preset.description = "Thrustmaster T818 direct drive - high torque with smooth response";
    preset.isBuiltIn = true;
    preset.settings.gain = 75.0f;
    preset.settings.filter = 5.0f;
    preset.settings.minimumForce = 15.0f;
    preset.settings.kerbEffect = 70.0f;
    preset.settings.roadEffect = 55.0f;
    preset.settings.slipEffect = 75.0f;
    preset.settings.absEffect = 50.0f;
    preset.settings.enhUndersteer = 20.0f;
    preset.settings.centreBoostGain = 15.0f;
    preset.settings.centreBoostRange = 25.0f;
    preset.settings.enableGyro = false;
    preset.settings.gyroStrength = 0.0f;
    return preset;
}

FfbConfigTool::WheelPreset FfbConfigTool::getFanatecCSLPreset()
{
    WheelPreset preset;
    preset.name = "Fanatec CSL";
    preset.wheelModel = "CSL";
    preset.manufacturer = "Fanatec";
    preset.description = "Fanatec CSL pedal set with wheel - entry-level belt drive";
    preset.isBuiltIn = true;
    preset.settings.gain = 90.0f;
    preset.settings.filter = 0.0f;
    preset.settings.minimumForce = 8.0f;
    preset.settings.kerbEffect = 55.0f;
    preset.settings.roadEffect = 40.0f;
    preset.settings.slipEffect = 60.0f;
    preset.settings.absEffect = 35.0f;
    preset.settings.enhUndersteer = 12.0f;
    preset.settings.centreBoostGain = 5.0f;
    preset.settings.centreBoostRange = 20.0f;
    preset.settings.enableGyro = false;
    preset.settings.gyroStrength = 0.0f;
    return preset;
}

FfbConfigTool::WheelPreset FfbConfigTool::getFanatecCSW25Preset()
{
    WheelPreset preset;
    preset.name = "Fanatec CSW 2.5";
    preset.wheelModel = "CSW25";
    preset.manufacturer = "Fanatec";
    preset.description = "Fanatec ClubSport Wheel 2.5 - strong belt-driven FFB";
    preset.isBuiltIn = true;
    preset.settings.gain = 80.0f;
    preset.settings.filter = 0.0f;
    preset.settings.minimumForce = 12.0f;
    preset.settings.kerbEffect = 65.0f;
    preset.settings.roadEffect = 50.0f;
    preset.settings.slipEffect = 70.0f;
    preset.settings.absEffect = 45.0f;
    preset.settings.enhUndersteer = 18.0f;
    preset.settings.centreBoostGain = 10.0f;
    preset.settings.centreBoostRange = 25.0f;
    preset.settings.enableGyro = false;
    preset.settings.gyroStrength = 0.0f;
    return preset;
}

FfbConfigTool::WheelPreset FfbConfigTool::getSimucubePreset()
{
    WheelPreset preset;
    preset.name = "Simucube";
    preset.wheelModel = "Simucube";
    preset.manufacturer = "Simucube";
    preset.description = "Simucube direct drive base - high fidelity FFB with minimal filtering";
    preset.isBuiltIn = true;
    preset.settings.gain = 70.0f;
    preset.settings.filter = 0.0f;
    preset.settings.minimumForce = 20.0f;
    preset.settings.kerbEffect = 80.0f;
    preset.settings.roadEffect = 65.0f;
    preset.settings.slipEffect = 85.0f;
    preset.settings.absEffect = 55.0f;
    preset.settings.enhUndersteer = 25.0f;
    preset.settings.centreBoostGain = 20.0f;
    preset.settings.centreBoostRange = 20.0f;
    preset.settings.enableGyro = false;
    preset.settings.gyroStrength = 0.0f;
    return preset;
}

float FfbConfigTool::calculateOptimalGain(const FfbSettings& settings)
{
    float optimalGain = 100.0f;

    float filterPenalty = settings.filter * 0.3f;
    float minimumForceBonus = settings.minimumForce * 0.2f;
    optimalGain -= filterPenalty;
    optimalGain += minimumForceBonus;

    if (settings.kerbEffect > 50.0f) {
        optimalGain -= (settings.kerbEffect - 50.0f) * 0.2f;
    }

    optimalGain = qBound(0.0f, optimalGain, 100.0f);
    return optimalGain;
}

float FfbConfigTool::calculateOptimalFilter(const FfbSettings& settings)
{
    float optimalFilter = 0.0f;

    if (settings.gain > 90.0f) {
        optimalFilter = (settings.gain - 90.0f) * 0.5f;
    }

    optimalFilter = qBound(0.0f, optimalFilter, 50.0f);
    return optimalFilter;
}

void FfbConfigTool::optimizeForWheel(FfbSettings& settings, const QString& wheelModel)
{
    QString manufacturer = getWheelManufacturer(wheelModel);

    if (manufacturer == "Logitech") {
        settings.minimumForce = qMax(settings.minimumForce, 5.0f);
        settings.centreBoostGain = 0.0f;
        settings.centreBoostRange = 0.0f;
    } else if (manufacturer == "Thrustmaster") {
        settings.minimumForce = qMax(settings.minimumForce, 10.0f);
        settings.centreBoostGain = qMax(settings.centreBoostGain, 5.0f);
    } else if (manufacturer == "Fanatec") {
        settings.minimumForce = qMax(settings.minimumForce, 8.0f);
    } else if (manufacturer == "Simucube" || manufacturer == "Other DD") {
        settings.minimumForce = qMax(settings.minimumForce, 15.0f);
        settings.filter = qMin(settings.filter, 5.0f);
    }

    settings.gain = calculateOptimalGain(settings);
    settings.filter = calculateOptimalFilter(settings);

    LOG_INFO("FfbConfigTool", QString("Optimized settings for wheel: %1 (%2)").arg(wheelModel, manufacturer));
}

bool FfbConfigTool::validateSettings(const FfbSettings& settings, QString* error)
{
    if (settings.gain < 0.0f || settings.gain > 100.0f) {
        if (error) *error = "Gain must be between 0 and 100%";
        return false;
    }
    if (settings.filter < 0.0f || settings.filter > 100.0f) {
        if (error) *error = "Filter must be between 0 and 100%";
        return false;
    }
    if (settings.minimumForce < 0.0f || settings.minimumForce > 100.0f) {
        if (error) *error = "Minimum force must be between 0 and 100%";
        return false;
    }
    if (settings.kerbEffect < 0.0f || settings.kerbEffect > 100.0f) {
        if (error) *error = "Kerb effect must be between 0 and 100%";
        return false;
    }
    if (settings.roadEffect < 0.0f || settings.roadEffect > 100.0f) {
        if (error) *error = "Road effect must be between 0 and 100%";
        return false;
    }
    if (settings.slipEffect < 0.0f || settings.slipEffect > 100.0f) {
        if (error) *error = "Slip effect must be between 0 and 100%";
        return false;
    }
    if (settings.absEffect < 0.0f || settings.absEffect > 100.0f) {
        if (error) *error = "ABS effect must be between 0 and 100%";
        return false;
    }
    if (settings.enhUndersteer < 0.0f || settings.enhUndersteer > 100.0f) {
        if (error) *error = "Enhance understeer must be between 0 and 100%";
        return false;
    }
    if (settings.centreBoostGain < 0.0f || settings.centreBoostGain > 100.0f) {
        if (error) *error = "Centre boost gain must be between 0 and 100%";
        return false;
    }
    if (settings.centreBoostRange < 0.0f || settings.centreBoostRange > 100.0f) {
        if (error) *error = "Centre boost range must be between 0 and 100%";
        return false;
    }
    if (settings.gyroStrength < 0.0f || settings.gyroStrength > 100.0f) {
        if (error) *error = "Gyro strength must be between 0 and 100%";
        return false;
    }
    return true;
}

QString FfbConfigTool::getWheelManufacturer(const QString& wheelModel)
{
    QString lower = wheelModel.toLower();
    if (lower.contains("logitech") || lower.contains("g27") || lower.contains("g29") || lower.contains("g920") || lower.contains("g923")) {
        return "Logitech";
    }
    if (lower.contains("thrustmaster") || lower.contains("t300") || lower.contains("t818") || lower.contains("t150") || lower.contains("ts-pc") || lower.contains("ts-xw")) {
        return "Thrustmaster";
    }
    if (lower.contains("fanatec") || lower.contains("csl") || lower.contains("csw") || lower.contains("clubsport") || lower.contains("podium")) {
        return "Fanatec";
    }
    if (lower.contains("simucube") || lower.contains("accuforce") || lower.contains("vrs")) {
        return "Other DD";
    }
    return "Unknown";
}

QStringList FfbConfigTool::getSupportedWheels()
{
    return {
        "Logitech G27",
        "Logitech G29",
        "Logitech G920",
        "Logitech G923",
        "Thrustmaster T150",
        "Thrustmaster T300RS",
        "Thrustmaster T818",
        "Thrustmaster TS-PC",
        "Thrustmaster TS-XW",
        "Fanatec CSL",
        "Fanatec CSW 2.5",
        "Fanatec ClubSport",
        "Fanatec Podium",
        "Simucube 2",
        "AccuForce",
        "VRS DirectForce"
    };
}

// ── FfbConfigManager ──────────────────────────────────────────────

FfbConfigManager::FfbConfigManager(const QString& acPath)
    : m_acPath(acPath)
{
}

bool FfbConfigManager::loadSettings()
{
    QString settingsPath = m_acPath + "/cfg/cfg.ini";
    m_settings = FfbConfigTool::loadSettings(settingsPath);
    return true;
}

bool FfbConfigManager::saveSettings()
{
    QString settingsPath = m_acPath + "/cfg/cfg.ini";
    return FfbConfigTool::saveSettings(m_settings, settingsPath);
}

bool FfbConfigManager::applyPreset(const QString& presetName)
{
    FfbConfigTool::WheelPreset preset = FfbConfigTool::getPreset(presetName);
    if (preset.name.isEmpty()) {
        LOG_WARNING("FfbConfigManager", QString("Preset not found: %1").arg(presetName));
        return false;
    }
    return FfbConfigTool::applyPreset(m_settings, preset);
}

bool FfbConfigManager::optimizeForWheel(const QString& wheelModel)
{
    FfbConfigTool::optimizeForWheel(m_settings, wheelModel);
    return true;
}
