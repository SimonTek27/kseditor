#pragma once

#include <QString>
#include <QMap>
#include <QVector>
#include <QJsonObject>
#include <QColor>

/**
 * @brief Weather Configuration Parser for Assetto Corsa
 *
 * Parses weather configurations from Pure, Sol, and CSP weather systems.
 * Based on community tools:
 * - acc-weatherfx-base (github.com/ac-custom-shaders-patch/acc-weatherfx-base)
 * - Pure (Peter Boese Patreon)
 * - Sol (Dynamic weather system)
 *
 * Supports:
 * - WeatherFX Lua script configs
 * - Pure weather presets
 * - Sol weather configurations
 * - CSP extension weather settings
 */
class WeatherConfigParser {
public:
    struct WeatherPreset {
        QString name;
        QString description;
        QString author;
        QString version;

        // Ambient conditions
        float ambientTemperature = 20.0f;
        float roadTemperature = 25.0f;
        float humidity = 50.0f;
        float windSpeed = 5.0f;
        float windDirection = 0.0f;
        float rainIntensity = 0.0f;
        float cloudIntensity = 0.5f;
        float cloudCutoff = 0.5f;
        float cloudColor = 0.5f;
        float cloudWidth = 1.0f;
        float cloudHeight = 1.0f;
        float cloudRadius = 100.0f;
        int cloudNumber = 10;
        float cloudSpeed = 0.5f;

        // Time settings
        float timeOfDay = 12.0f; // 0-24 hours
        float timeMultiplier = 1.0f;

        // Visual settings
        QColor fogColor = QColor(200, 200, 220);
        float fogDensity = 0.01f;
        float fogHeightFalloff = 0.02f;
        float fogDistance = 8000.0f;

        // Lighting
        float sunIntensity = 1.0f;
        float ambientIntensity = 0.3f;
        QColor sunColor = QColor(255, 250, 240);
        QColor ambientColor = QColor(180, 200, 220);
        QColor skyColor = QColor(135, 206, 235);
        QColor horizonColor = QColor(255, 127, 80);

        // Post-processing
        float exposure = 1.0f;
        float saturation = 1.0f;
        float contrast = 1.0f;
        float temperature = 6500.0f; // Kelvin
        float temperatureCoeff = 1.0f;

        // CSP-specific
        bool useRealWeather = false;
        bool useLiveConditions = false;
        QString weatherController = "Pure"; // Pure, Sol, Default
    };

    struct WeatherZone {
        QString name;
        float position[3];
        float radius;
        WeatherPreset conditions;
    };

    // Parsing operations
    static WeatherPreset parsePureConfig(const QString& configPath);
    static WeatherPreset parseSolConfig(const QString& configPath);
    static WeatherPreset parseCspConfig(const QString& configPath);
    static WeatherPreset parseLuaScript(const QString& luaPath);

    // Saving operations
    static bool savePureConfig(const WeatherPreset& preset, const QString& configPath);
    static bool saveSolConfig(const WeatherPreset& preset, const QString& configPath);
    static bool saveCspConfig(const WeatherPreset& preset, const QString& configPath);

    // Weather zones
    static QVector<WeatherZone> parseWeatherZones(const QString& trackPath);
    static bool saveWeatherZones(const QVector<WeatherZone>& zones, const QString& trackPath);

    // Preset management
    static QVector<WeatherPreset> loadPresets(const QString& directory);
    static bool savePreset(const WeatherPreset& preset, const QString& directory);
    static WeatherPreset getPreset(const QString& name);

    // Default presets
    static WeatherPreset getDefaultClear();
    static WeatherPreset getDefaultCloudy();
    static WeatherPreset getDefaultRain();
    static WeatherPreset getDefaultStorm();
    static WeatherPreset getDefaultNight();

    // Validation
    static bool validatePreset(const WeatherPreset& preset, QString* error = nullptr);

    // Utility
    static QString getWeatherName(float cloudIntensity, float rainIntensity);
    static QString getTimeOfDayName(float hour);
    static QColor getSkyColor(float timeOfDay, float cloudIntensity);

private:
    static QMap<QString, WeatherPreset> m_presets;
};

/**
 * @brief Weather FX Script Parser
 *
 * Parses WeatherFX Lua scripts for CSP integration.
 */
class WeatherFxParser {
public:
    struct WeatherFxConfig {
        QString scriptName;
        QString scriptPath;
        QMap<QString, QString> stringSettings;
        QMap<QString, float> floatSettings;
        QMap<QString, bool> boolSettings;
        QMap<QString, int> intSettings;
    };

    static WeatherFxConfig parseScript(const QString& luaPath);
    static bool saveScript(const WeatherFxConfig& config, const QString& luaPath);
    static QVector<QString> getAvailableScripts(const QString& weatherDir);
};
