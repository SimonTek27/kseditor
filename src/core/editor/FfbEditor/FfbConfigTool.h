#pragma once

#include <QString>
#include <QMap>
#include <QVector>

namespace ks {

/**
 * @brief FFB Configuration Tool for Assetto Corsa
 *
 * Manages force feedback settings for racing wheels.
 * Based on:
 * - AC FFB documentation
 * - Community FFB guides
 * - Wheel manufacturer recommendations
 *
 * Features:
 * - FFB preset management
 * - Wheel-specific configurations
 * - Gain/filter optimization
 * - Effect parameter tuning
 * - Export/import settings
 */
class FfbConfigTool {
public:
    struct FfbSettings {
        float gain = 100.0f;            // 0-100%
        float filter = 0.0f;            // 0-100%
        float minimumForce = 0.0f;      // 0-100%
        float kerbEffect = 0.0f;        // 0-100%
        float roadEffect = 0.0f;        // 0-100%
        float slipEffect = 0.0f;        // 0-100%
        float absEffect = 0.0f;         // 0-100%
        float enhUndersteer = 0.0f;     // 0-100%
        float centreBoostGain = 0.0f;   // 0-100%
        float centreBoostRange = 0.0f;  // 0-100%
        bool enableGyro = false;
        float gyroStrength = 0.0f;      // 0-100%
    };

    struct WheelPreset {
        QString name;
        QString wheelModel;
        QString manufacturer;
        FfbSettings settings;
        QString description;
        bool isBuiltIn = false;
    };

    // Settings operations
    static FfbSettings loadSettings(const QString& settingsPath);
    static bool saveSettings(const FfbSettings& settings, const QString& settingsPath);

    // Preset operations
    static QVector<WheelPreset> getPresets();
    static WheelPreset getPreset(const QString& presetName);
    static bool applyPreset(FfbSettings& settings, const WheelPreset& preset);
    static bool savePreset(const WheelPreset& preset);

    // Built-in presets
    static WheelPreset getLogitechG27Preset();
    static WheelPreset getLogitechG29Preset();
    static WheelPreset getThrustmasterT300Preset();
    static WheelPreset getThrustmasterT818Preset();
    static WheelPreset getFanatecCSLPreset();
    static WheelPreset getFanatecCSW25Preset();
    static WheelPreset getSimucubePreset();

    // Optimization
    static float calculateOptimalGain(const FfbSettings& settings);
    static float calculateOptimalFilter(const FfbSettings& settings);
    static void optimizeForWheel(FfbSettings& settings, const QString& wheelModel);

    // Validation
    static bool validateSettings(const FfbSettings& settings, QString* error = nullptr);

    // Utility
    static QString getWheelManufacturer(const QString& wheelModel);
    static QStringList getSupportedWheels();
};

/**
 * @brief FFB Config Manager - High-level interface
 */
class FfbConfigManager {
public:
    explicit FfbConfigManager(const QString& acPath);

    // Operations
    bool loadSettings();
    bool saveSettings();
    bool applyPreset(const QString& presetName);

    // Access
    FfbConfigTool::FfbSettings& settings() { return m_settings; }

    // Optimization
    bool optimizeForWheel(const QString& wheelModel);

private:
    QString m_acPath;
    FfbConfigTool::FfbSettings m_settings;
};

} // namespace ks
