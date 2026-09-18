#include "WeatherPhysics.h"
#include <algorithm>
#include <QDebug>

namespace ks {
namespace physics {

WeatherSimulator::WeatherSimulator(QObject* parent)
    : QObject(parent)
{
}

WeatherSimulator::~WeatherSimulator() {}

void WeatherSimulator::startSimulation() {
    m_running = true;
    emit simulationStarted();
}

void WeatherSimulator::stopSimulation() {
    m_running = false;
    emit simulationStopped();
}

void WeatherSimulator::reset() {
    m_weather = WeatherState();
    m_effects = WeatherEffects();
    m_running = false;
    m_currentPresetIndex = -1;
    m_currentPresetName.clear();
    emit simulationReset();
}

void WeatherSimulator::updateEffects() {
    m_effects.aquaplaningRisk = m_weather.trackWetness * 0.5f + static_cast<float>(m_weather.rainIntensity) * 0.001f;
    m_effects.aquaplaningRisk = std::clamp(m_effects.aquaplaningRisk, 0.0f, 1.0f);
    m_effects.trackGripReduction = m_weather.trackWetness * 0.3f + m_effects.aquaplaningRisk * 0.2f;
    m_effects.trackGripReduction = std::clamp(m_effects.trackGripReduction, 0.0f, 0.8f);
}

QVector<QString> WeatherSimulator::availablePresets() const {
    QVector<QString> names;
    for (const auto& p : m_presets) names.append(p.name);
    return names;
}

void WeatherSimulator::setWeatherState(const WeatherState& weather) {
    m_weather = weather;
    update(0.0);
    emit weatherChanged(m_weather);
}

bool WeatherSimulator::loadPresetsFromDirectory(const QString& directory) {
    m_presets = WeatherConfigParser::loadPresets(directory);

    // Add built-in defaults if no presets found
    if (m_presets.isEmpty()) {
        m_presets.append(WeatherConfigParser::getDefaultClear());
        m_presets.append(WeatherConfigParser::getDefaultCloudy());
        m_presets.append(WeatherConfigParser::getDefaultRain());
        m_presets.append(WeatherConfigParser::getDefaultStorm());
        m_presets.append(WeatherConfigParser::getDefaultNight());
    }

    qDebug() << "WeatherSimulator: Loaded" << m_presets.size() << "weather presets";
    return !m_presets.isEmpty();
}

bool WeatherSimulator::selectPreset(const QString& presetName) {
    for (int i = 0; i < m_presets.size(); ++i) {
        if (m_presets[i].name == presetName) {
            return selectPreset(i);
        }
    }
    return false;
}

bool WeatherSimulator::selectPreset(int index) {
    if (index < 0 || index >= m_presets.size()) return false;

    m_currentPresetIndex = index;
    m_currentPresetName = m_presets[index].name;
    m_weather = presetToWeatherState(m_presets[index]);
    update(0.0);

    qDebug() << "WeatherSimulator: Selected preset" << m_currentPresetName;
    emit weatherChanged(m_weather);
    emit presetSelected(m_currentPresetName);
    return true;
}

void WeatherSimulator::setTrackWetness(double wetness) {
    m_weather.trackWetness = std::clamp(static_cast<float>(wetness), 0.0f, 1.0f);
    emit weatherChanged(m_weather);
}

void WeatherSimulator::setRainIntensity(double mmh) {
    m_weather.rainIntensity = std::max(0.0, mmh);
    emit weatherChanged(m_weather);
}

void WeatherSimulator::setAirDensity(double density) {
    m_weather.airDensity = std::max(0.8, density);
    emit weatherChanged(m_weather);
}

void WeatherSimulator::update(double dt) {
    Q_UNUSED(dt);

    m_effects.aquaplaningRisk = m_weather.trackWetness * 0.5 + m_weather.rainIntensity * 0.001;
    m_effects.aquaplaningRisk = std::clamp(m_effects.aquaplaningRisk, 0.0f, 1.0f);

    m_effects.trackGripReduction = m_weather.trackWetness * 0.3 + m_effects.aquaplaningRisk * 0.2;
    m_effects.trackGripReduction = std::clamp(m_effects.trackGripReduction, 0.0f, 0.8f);

    double windRad = m_weather.windDirection * 3.14159265358979 / 180.0;
    double windComponent = m_weather.windSpeed * std::cos(windRad);
    m_effects.windForceX = windComponent * m_weather.airDensity;
    m_effects.windForceZ = m_weather.windSpeed * std::sin(windRad) * m_weather.airDensity;
}

WeatherState WeatherSimulator::presetToWeatherState(
    const WeatherConfigParser::WeatherPreset& preset) const
{
    WeatherState state;
    state.ambientTemp = preset.ambientTemperature;
    state.trackTemp = preset.roadTemperature;
    state.airDensity = 1.225;
    state.trackWetness = preset.rainIntensity * 0.8;
    state.rainIntensity = preset.rainIntensity;
    state.windSpeed = preset.windSpeed;
    state.windDirection = preset.windDirection;
    return state;
}

// ============================================================================
// WeatherConfigParser constructor
// ============================================================================

WeatherConfigParser::WeatherConfigParser(QObject* parent) : QObject(parent) {
}

// ============================================================================
// WeatherConfigParser static member initialization
// ============================================================================

QMap<QString, WeatherConfigParser::WeatherPreset> WeatherConfigParser::m_presets;

// ============================================================================
// Parsing operations
// ============================================================================

WeatherConfigParser::WeatherPreset WeatherConfigParser::parsePureConfig(const QString& configPath) {
    WeatherPreset preset;

    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return preset;
    }

    QTextStream stream(&file);
    QString content = stream.readAll();
    file.close();

    // Parse Pure INI format
    QRegularExpression sectionRegex("\\[(\\w+)\\]");
    QRegularExpression keyRegex("^(\\w+)\\s*=\\s*(.+)$");

    QString currentSection;
    QStringList lines = content.split('\n');

    for (const QString& line : lines) {
        QString trimmed = line.trimmed();

        // Skip comments
        if (trimmed.startsWith(';') || trimmed.startsWith('#')) continue;

        // Section header
        QRegularExpressionMatch sectionMatch = sectionRegex.match(trimmed);
        if (sectionMatch.hasMatch()) {
            currentSection = sectionMatch.captured(1);
            continue;
        }

        // Key-value pair
        QRegularExpressionMatch keyMatch = keyRegex.match(trimmed);
        if (keyMatch.hasMatch()) {
            QString key = keyMatch.captured(1);
            QString value = keyMatch.captured(2).trimmed();

            // Parse based on section and key
            if (currentSection == "WEATHER") {
                if (key == "NAME") preset.name = value;
                else if (key == "DESCRIPTION") preset.description = value;
                else if (key == "AMBIENTTemperature") preset.ambientTemperature = value.toFloat();
                else if (key == "ROAD_TEMPERATURE") preset.roadTemperature = value.toFloat();
                else if (key == "HUMIDITY") preset.humidity = value.toFloat();
                else if (key == "WIND_SPEED") preset.windSpeed = value.toFloat();
                else if (key == "WIND_DIRECTION") preset.windDirection = value.toFloat();
                else if (key == "RAIN_INTENSITY") preset.rainIntensity = value.toFloat();
                else if (key == "CLOUD_INTENSITY") preset.cloudIntensity = value.toFloat();
            } else if (currentSection == "TIME") {
                if (key == "TIME_OF_DAY") preset.timeOfDay = value.toFloat();
                else if (key == "TIME_MULTIPLIER") preset.timeMultiplier = value.toFloat();
            } else if (currentSection == "FOG") {
                if (key == "DENSITY") preset.fogDensity = value.toFloat();
                else if (key == "HEIGHT_FALLOFF") preset.fogHeightFalloff = value.toFloat();
                else if (key == "COLOR") {
                    QStringList rgb = value.split(',');
                    if (rgb.size() >= 3) {
                        preset.fogColor = QColor(rgb[0].toInt(), rgb[1].toInt(), rgb[2].toInt());
                    }
                }
            } else if (currentSection == "LIGHTING") {
                if (key == "SUN_INTENSITY") preset.sunIntensity = value.toFloat();
                else if (key == "AMBIENT_INTENSITY") preset.ambientIntensity = value.toFloat();
            } else if (currentSection == "POST_PROCESSING") {
                if (key == "EXPOSURE") preset.exposure = value.toFloat();
                else if (key == "SATURATION") preset.saturation = value.toFloat();
                else if (key == "CONTRAST") preset.contrast = value.toFloat();
                else if (key == "TEMPERATURE") preset.temperature = value.toFloat();
            }
        }
    }

    return preset;
}

WeatherConfigParser::WeatherPreset WeatherConfigParser::parseSolConfig(const QString& configPath) {
    WeatherPreset preset;

    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return preset;
    }

    QTextStream stream(&file);
    QString content = stream.readAll();
    file.close();

    // Sol uses a similar INI format but with different key names
    QRegularExpression keyRegex("^(\\w+)\\s*=\\s*(.+)$");
    QStringList lines = content.split('\n');

    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.startsWith(';') || trimmed.startsWith('#')) continue;

        QRegularExpressionMatch match = keyRegex.match(trimmed);
        if (match.hasMatch()) {
            QString key = match.captured(1);
            QString value = match.captured(2).trimmed();

            // Sol key mappings
            if (key == "sol_initial_weather") preset.name = value;
            else if (key == "sol_ambient_temperature") preset.ambientTemperature = value.toFloat();
            else if (key == "sol_road_temperature") preset.roadTemperature = value.toFloat();
            else if (key == "sol_humidity") preset.humidity = value.toFloat();
            else if (key == "sol_wind_speed") preset.windSpeed = value.toFloat();
            else if (key == "sol_wind_direction") preset.windDirection = value.toFloat();
            else if (key == "sol_rain") preset.rainIntensity = value.toFloat();
            else if (key == "sol_clouds") preset.cloudIntensity = value.toFloat();
            else if (key == "sol_time") preset.timeOfDay = value.toFloat();
        }
    }

    return preset;
}

WeatherConfigParser::WeatherPreset WeatherConfigParser::parseCspConfig(const QString& configPath) {
    WeatherPreset preset;

    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return preset;
    }

    QTextStream stream(&file);
    QString content = stream.readAll();
    file.close();

    // CSP uses YAML-like format in extra_cfg.yml
    QRegularExpression keyRegex("^(\\w[\\w\\s]*):\\s*(.+)$");
    QStringList lines = content.split('\n');

    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.startsWith('#')) continue;

        QRegularExpressionMatch match = keyRegex.match(trimmed);
        if (match.hasMatch()) {
            QString key = match.captured(1).trimmed();
            QString value = match.captured(2).trimmed();

            // CSP key mappings
            if (key == "WEATHER") preset.weatherController = value;
            else if (key == "USE_REAL_WEATHER") preset.useRealWeather = (value == "1" || value.toLower() == "true");
            else if (key == "USE_LIVE_CONDITIONS") preset.useLiveConditions = (value == "1" || value.toLower() == "true");
            else if (key == "TIME_OF_DAY") preset.timeOfDay = value.toFloat();
            else if (key == "TIME_MULTIPLIER") preset.timeMultiplier = value.toFloat();
        }
    }

    return preset;
}

WeatherConfigParser::WeatherPreset WeatherConfigParser::parseLuaScript(const QString& luaPath) {
    WeatherPreset preset;

    QFile file(luaPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return preset;
    }

    QTextStream stream(&file);
    QString content = stream.readAll();
    file.close();

    // Parse Lua script settings
    QRegularExpression settingRegex("ScriptSettings\\s*=\\s*\\{([\\s\\S]*?)\\}");
    QRegularExpressionMatch match = settingRegex.match(content);

    if (match.hasMatch()) {
        QString settingsBlock = match.captured(1);

        // Parse key-value pairs from Lua table
        QRegularExpression keyValueRegex("(\\w+)\\s*=\\s*([^,\\n]+)");
        QRegularExpressionMatchIterator it = keyValueRegex.globalMatch(settingsBlock);

        while (it.hasNext()) {
            QRegularExpressionMatch kvMatch = it.next();
            QString key = kvMatch.captured(1);
            QString value = kvMatch.captured(2).trimmed();

            if (key == "LINEAR_COLOR_SPACE_ENABLED") {
                // Handle boolean
            }
        }
    }

    return preset;
}

// ============================================================================
// Saving operations
// ============================================================================

bool WeatherConfigParser::savePureConfig(const WeatherPreset& preset, const QString& configPath) {
    QFile file(configPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    stream << "; Pure Weather Configuration\n";
    stream << "; Generated by ksEditor\n\n";

    stream << "[WEATHER]\n";
    stream << "NAME=" << preset.name << "\n";
    stream << "DESCRIPTION=" << preset.description << "\n";
    stream << "AMBIENT_TEMPERATURE=" << QString::number(preset.ambientTemperature, 'f', 1) << "\n";
    stream << "ROAD_TEMPERATURE=" << QString::number(preset.roadTemperature, 'f', 1) << "\n";
    stream << "HUMIDITY=" << QString::number(preset.humidity, 'f', 1) << "\n";
    stream << "WIND_SPEED=" << QString::number(preset.windSpeed, 'f', 1) << "\n";
    stream << "WIND_DIRECTION=" << QString::number(preset.windDirection, 'f', 1) << "\n";
    stream << "RAIN_INTENSITY=" << QString::number(preset.rainIntensity, 'f', 2) << "\n";
    stream << "CLOUD_INTENSITY=" << QString::number(preset.cloudIntensity, 'f', 2) << "\n\n";

    stream << "[TIME]\n";
    stream << "TIME_OF_DAY=" << QString::number(preset.timeOfDay, 'f', 2) << "\n";
    stream << "TIME_MULTIPLIER=" << QString::number(preset.timeMultiplier, 'f', 2) << "\n\n";

    stream << "[FOG]\n";
    stream << "DENSITY=" << QString::number(preset.fogDensity, 'f', 4) << "\n";
    stream << "HEIGHT_FALLOFF=" << QString::number(preset.fogHeightFalloff, 'f', 4) << "\n";
    stream << "COLOR=" << preset.fogColor.red() << "," << preset.fogColor.green() << "," << preset.fogColor.blue() << "\n\n";

    stream << "[LIGHTING]\n";
    stream << "SUN_INTENSITY=" << QString::number(preset.sunIntensity, 'f', 2) << "\n";
    stream << "AMBIENT_INTENSITY=" << QString::number(preset.ambientIntensity, 'f', 2) << "\n\n";

    stream << "[POST_PROCESSING]\n";
    stream << "EXPOSURE=" << QString::number(preset.exposure, 'f', 2) << "\n";
    stream << "SATURATION=" << QString::number(preset.saturation, 'f', 2) << "\n";
    stream << "CONTRAST=" << QString::number(preset.contrast, 'f', 2) << "\n";
    stream << "TEMPERATURE=" << QString::number(preset.temperature, 'f', 0) << "\n";

    file.close();
    return true;
}

bool WeatherConfigParser::saveSolConfig(const WeatherPreset& preset, const QString& configPath) {
    QFile file(configPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    stream << "; Sol Weather Configuration\n";
    stream << "; Generated by ksEditor\n\n";

    stream << "sol_initial_weather=" << preset.name << "\n";
    stream << "sol_ambient_temperature=" << QString::number(preset.ambientTemperature, 'f', 1) << "\n";
    stream << "sol_road_temperature=" << QString::number(preset.roadTemperature, 'f', 1) << "\n";
    stream << "sol_humidity=" << QString::number(preset.humidity, 'f', 1) << "\n";
    stream << "sol_wind_speed=" << QString::number(preset.windSpeed, 'f', 1) << "\n";
    stream << "sol_wind_direction=" << QString::number(preset.windDirection, 'f', 1) << "\n";
    stream << "sol_rain=" << QString::number(preset.rainIntensity, 'f', 2) << "\n";
    stream << "sol_clouds=" << QString::number(preset.cloudIntensity, 'f', 2) << "\n";
    stream << "sol_time=" << QString::number(preset.timeOfDay, 'f', 2) << "\n";

    file.close();
    return true;
}

bool WeatherConfigParser::saveCspConfig(const WeatherPreset& preset, const QString& configPath) {
    QFile file(configPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    stream << "# CSP Weather Configuration\n";
    stream << "# Generated by ksEditor\n\n";

    stream << "WEATHER: " << preset.weatherController << "\n";
    stream << "USE_REAL_WEATHER: " << (preset.useRealWeather ? "1" : "0") << "\n";
    stream << "USE_LIVE_CONDITIONS: " << (preset.useLiveConditions ? "1" : "0") << "\n";
    stream << "TIME_OF_DAY: " << QString::number(preset.timeOfDay, 'f', 2) << "\n";
    stream << "TIME_MULTIPLIER: " << QString::number(preset.timeMultiplier, 'f', 2) << "\n";

    file.close();
    return true;
}

// ============================================================================
// Weather zones
// ============================================================================

QVector<WeatherConfigParser::WeatherZone> WeatherConfigParser::parseWeatherZones(const QString& trackPath) {
    QVector<WeatherZone> zones;

    QString zonesPath = trackPath + "/data/weather_zones.ini";
    QFile file(zonesPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return zones;
    }

    QTextStream stream(&file);
    QString currentSection;
    WeatherZone currentZone;
    bool inZone = false;

    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();

        if (line.startsWith('[') && line.endsWith(']')) {
            if (inZone && !currentZone.name.isEmpty()) {
                zones.append(currentZone);
            }

            currentSection = line.mid(1, line.length() - 2);
            currentZone = WeatherZone();
            inZone = currentSection.startsWith("ZONE_");
        } else if (inZone && line.contains('=')) {
            int eqPos = line.indexOf('=');
            QString key = line.left(eqPos).trimmed().toUpper();
            QString value = line.mid(eqPos + 1).trimmed();

            if (key == "NAME") currentZone.name = value;
            else if (key == "POSITION_X") currentZone.position = value.toFloat();
            else if (key == "RADIUS") currentZone.radius = value.toFloat();
        }
    }

    if (inZone && !currentZone.name.isEmpty()) {
        zones.append(currentZone);
    }

    file.close();
    return zones;
}

bool WeatherConfigParser::saveWeatherZones(const QVector<WeatherZone>& zones, const QString& trackPath) {
    QString zonesPath = trackPath + "/data/weather_zones.ini";
    QFile file(zonesPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    stream << "; Weather Zone Definitions\n";
    stream << "; Generated by ksEditor\n\n";

    for (int i = 0; i < zones.size(); ++i) {
        const WeatherZone& zone = zones[i];
        stream << "[ZONE_" << i << "]\n";
        stream << "NAME=" << zone.name << "\n";
        stream << "POSITION_X=" << QString::number(zone.position, 'f', 2) << "\n";
        stream << "RADIUS=" << QString::number(zone.radius, 'f', 2) << "\n\n";
    }

    file.close();
    return true;
}

// ============================================================================
// Preset management
// ============================================================================

QVector<WeatherConfigParser::WeatherPreset> WeatherConfigParser::loadPresets(const QString& directory) {
    QVector<WeatherPreset> presets;

    QDir dir(directory);
    if (!dir.exists()) return presets;

    QStringList filters;
    filters << "*.ini" << "*.cfg" << "*.json";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);

    for (const QFileInfo& fileInfo : files) {
        WeatherPreset preset;

        if (fileInfo.suffix() == "ini" || fileInfo.suffix() == "cfg") {
            preset = parsePureConfig(fileInfo.absoluteFilePath());
        } else if (fileInfo.suffix() == "json") {
            QFile file(fileInfo.absoluteFilePath());
            if (file.open(QIODevice::ReadOnly)) {
                QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
                file.close();

                if (doc.isObject()) {
                    QJsonObject obj = doc.object();
                    preset.name = obj["name"].toString();
                    preset.ambientTemperature = obj["ambientTemperature"].toDouble();
                    preset.roadTemperature = obj["roadTemperature"].toDouble();
                    preset.rainIntensity = obj["rainIntensity"].toDouble();
                    preset.cloudIntensity = obj["cloudIntensity"].toDouble();
                    preset.timeOfDay = obj["timeOfDay"].toDouble();
                }
            }
        }

        if (!preset.name.isEmpty()) {
            presets.append(preset);
        }
    }

    return presets;
}

bool WeatherConfigParser::savePreset(const WeatherPreset& preset, const QString& directory) {
    QDir().mkpath(directory);

    QString filePath = directory + "/" + preset.name + ".ini";
    return savePureConfig(preset, filePath);
}

WeatherConfigParser::WeatherPreset WeatherConfigParser::getPreset(const QString& name) {
    return m_presets.value(name);
}

// ============================================================================
// Default presets
// ============================================================================

WeatherConfigParser::WeatherPreset WeatherConfigParser::getDefaultClear() {
    WeatherPreset preset;
    preset.name = "Clear";
    preset.description = "Clear sky with good visibility";
    preset.ambientTemperature = 22.0f;
    preset.roadTemperature = 28.0f;
    preset.humidity = 45.0f;
    preset.windSpeed = 5.0f;
    preset.windDirection = 180.0f;
    preset.rainIntensity = 0.0f;
    preset.cloudIntensity = 0.1f;
    preset.timeOfDay = 14.0f;
    preset.fogDensity = 0.005f;
    preset.sunIntensity = 1.2f;
    preset.ambientIntensity = 0.4f;
    return preset;
}

WeatherConfigParser::WeatherPreset WeatherConfigParser::getDefaultCloudy() {
    WeatherPreset preset;
    preset.name = "Cloudy";
    preset.description = "Overcast with moderate clouds";
    preset.ambientTemperature = 18.0f;
    preset.roadTemperature = 20.0f;
    preset.humidity = 65.0f;
    preset.windSpeed = 10.0f;
    preset.windDirection = 220.0f;
    preset.rainIntensity = 0.0f;
    preset.cloudIntensity = 0.7f;
    preset.timeOfDay = 12.0f;
    preset.fogDensity = 0.01f;
    preset.sunIntensity = 0.6f;
    preset.ambientIntensity = 0.5f;
    preset.fogColor = QColor(180, 180, 200);
    return preset;
}

WeatherConfigParser::WeatherPreset WeatherConfigParser::getDefaultRain() {
    WeatherPreset preset;
    preset.name = "Rain";
    preset.description = "Light to moderate rain";
    preset.ambientTemperature = 15.0f;
    preset.roadTemperature = 16.0f;
    preset.humidity = 85.0f;
    preset.windSpeed = 15.0f;
    preset.windDirection = 270.0f;
    preset.rainIntensity = 0.5f;
    preset.cloudIntensity = 0.9f;
    preset.timeOfDay = 10.0f;
    preset.fogDensity = 0.02f;
    preset.sunIntensity = 0.3f;
    preset.ambientIntensity = 0.4f;
    preset.fogColor = QColor(150, 150, 170);
    return preset;
}

WeatherConfigParser::WeatherPreset WeatherConfigParser::getDefaultStorm() {
    WeatherPreset preset;
    preset.name = "Storm";
    preset.description = "Heavy rain with thunderstorms";
    preset.ambientTemperature = 12.0f;
    preset.roadTemperature = 13.0f;
    preset.humidity = 95.0f;
    preset.windSpeed = 25.0f;
    preset.windDirection = 315.0f;
    preset.rainIntensity = 0.9f;
    preset.cloudIntensity = 1.0f;
    preset.timeOfDay = 16.0f;
    preset.fogDensity = 0.04f;
    preset.sunIntensity = 0.1f;
    preset.ambientIntensity = 0.3f;
    preset.fogColor = QColor(100, 100, 120);
    return preset;
}

WeatherConfigParser::WeatherPreset WeatherConfigParser::getDefaultNight() {
    WeatherPreset preset;
    preset.name = "Night";
    preset.description = "Clear night sky";
    preset.ambientTemperature = 10.0f;
    preset.roadTemperature = 8.0f;
    preset.humidity = 70.0f;
    preset.windSpeed = 3.0f;
    preset.windDirection = 180.0f;
    preset.rainIntensity = 0.0f;
    preset.cloudIntensity = 0.2f;
    preset.timeOfDay = 23.0f;
    preset.fogDensity = 0.008f;
    preset.sunIntensity = 0.0f;
    preset.ambientIntensity = 0.1f;
    preset.fogColor = QColor(20, 20, 40);
    return preset;
}

// ============================================================================
// Validation
// ============================================================================

bool WeatherConfigParser::validatePreset(const WeatherPreset& preset, QString* error) {
    if (preset.name.isEmpty()) {
        if (error) *error = "Weather preset name is empty";
        return false;
    }

    if (preset.ambientTemperature < -40.0f || preset.ambientTemperature > 60.0f) {
        if (error) *error = "Ambient temperature out of range (-40 to 60 C)";
        return false;
    }

    if (preset.roadTemperature < -40.0f || preset.roadTemperature > 80.0f) {
        if (error) *error = "Road temperature out of range (-40 to 80 C)";
        return false;
    }

    if (preset.humidity < 0.0f || preset.humidity > 100.0f) {
        if (error) *error = "Humidity out of range (0-100%)";
        return false;
    }

    if (preset.windSpeed < 0.0f || preset.windSpeed > 100.0f) {
        if (error) *error = "Wind speed out of range (0-100 km/h)";
        return false;
    }

    if (preset.rainIntensity < 0.0f || preset.rainIntensity > 1.0f) {
        if (error) *error = "Rain intensity out of range (0.0-1.0)";
        return false;
    }

    if (preset.timeOfDay < 0.0f || preset.timeOfDay > 24.0f) {
        if (error) *error = "Time of day out of range (0-24 hours)";
        return false;
    }

    return true;
}

// ============================================================================
// Utility
// ============================================================================

QString WeatherConfigParser::getWeatherName(float cloudIntensity, float rainIntensity) {
    if (rainIntensity > 0.7f) return "Storm";
    if (rainIntensity > 0.3f) return "Heavy Rain";
    if (rainIntensity > 0.0f) return "Light Rain";
    if (cloudIntensity > 0.8f) return "Overcast";
    if (cloudIntensity > 0.5f) return "Cloudy";
    if (cloudIntensity > 0.2f) return "Partly Cloudy";
    return "Clear";
}

QString WeatherConfigParser::getTimeOfDayName(float hour) {
    if (hour < 5.0f) return "Night";
    if (hour < 7.0f) return "Dawn";
    if (hour < 10.0f) return "Morning";
    if (hour < 14.0f) return "Midday";
    if (hour < 17.0f) return "Afternoon";
    if (hour < 19.0f) return "Evening";
    if (hour < 21.0f) return "Dusk";
    return "Night";
}

QColor WeatherConfigParser::getSkyColor(float timeOfDay, float cloudIntensity) {
    // Simple sky color calculation based on time
    float brightness = 1.0f;

    // Night
    if (timeOfDay < 5.0f || timeOfDay > 21.0f) {
        return QColor(10, 10, 30);
    }
    // Dawn/Dusk
    else if (timeOfDay < 7.0f || timeOfDay > 19.0f) {
        float t = (timeOfDay < 7.0f) ? (timeOfDay - 5.0f) / 2.0f : (21.0f - timeOfDay) / 2.0f;
        int r = (int)(255 * (0.3f + 0.5f * t));
        int g = (int)(150 * (0.3f + 0.3f * t));
        int b = (int)(100 * (0.3f + 0.2f * t));
        return QColor(qMin(255, r), qMin(255, g), qMin(255, b));
    }
    // Day
    else {
        float cloudDarkening = 1.0f - cloudIntensity * 0.4f;
        int r = (int)(135 * cloudDarkening);
        int g = (int)(206 * cloudDarkening);
        int b = (int)(235 * cloudDarkening);
        return QColor(r, g, b);
    }
}

// ============================================================================
// WeatherFxParser implementation
// ============================================================================

WeatherFxParser::WeatherFxConfig WeatherFxParser::parseScript(const QString& luaPath) {
    WeatherFxConfig config;
    config.scriptPath = luaPath;
    config.scriptName = QFileInfo(luaPath).baseName();

    QFile file(luaPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return config;
    }

    QTextStream stream(&file);
    QString content = stream.readAll();
    file.close();

    // Parse ScriptSettings table
    QRegularExpression settingsBlockRegex("ScriptSettings\\s*=\\s*ac\\.INIConfig\\.scriptSettings\\s*\\(\\s*\\)\\s*:\\s*mapConfig\\s*\\(\\s*\\{([\\s\\S]*?)\\}\\s*\\)");
    QRegularExpressionMatch blockMatch = settingsBlockRegex.match(content);

    if (blockMatch.hasMatch()) {
        QString settingsBlock = blockMatch.captured(1);

        // Parse individual settings
        QRegularExpression settingRegex("(\\w+)\\s*=\\s*\\{([\\s\\S]*?)\\}");
        QRegularExpressionMatchIterator it = settingRegex.globalMatch(settingsBlock);

        while (it.hasNext()) {
            QRegularExpressionMatch settingMatch = it.next();
            QString sectionName = settingMatch.captured(1);
            QString sectionContent = settingMatch.captured(2);

            // Parse key-value pairs in section
            QRegularExpression kvRegex("(\\w+)\\s*=\\s*([^,\\n]+)");
            QRegularExpressionMatchIterator kvIt = kvRegex.globalMatch(sectionContent);

            while (kvIt.hasNext()) {
                QRegularExpressionMatch kvMatch = kvIt.next();
                QString key = sectionName + "." + kvMatch.captured(1);
                QString value = kvMatch.captured(2).trimmed();

                // Determine type based on value
                if (value == "true" || value == "false") {
                    config.boolSettings[key] = (value == "true");
                } else if (value.contains('.')) {
                    config.floatSettings[key] = value.toFloat();
                } else {
                    bool ok;
                    int intVal = value.toInt(&ok);
                    if (ok) {
                        config.intSettings[key] = intVal;
                    } else {
                        config.stringSettings[key] = value;
                    }
                }
            }
        }
    }

    return config;
}

bool WeatherFxParser::saveScript(const WeatherFxConfig& config, const QString& luaPath) {
    QFile file(luaPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    QString content = stream.readAll();
    file.close();

    // Update ScriptSettings in the Lua content
    // This is a simplified version - production would need proper Lua AST manipulation
    QRegularExpression settingsBlockRegex("ScriptSettings\\s*=\\s*ac\\.INIConfig\\.scriptSettings\\s*\\(\\s*\\)\\s*:\\s*mapConfig\\s*\\(\\s*\\{[\\s\\S]*?\\}\\s*\\)");

    QString newSettings = "ScriptSettings = ac.INIConfig.scriptSettings():mapConfig({\n";

    // Group settings by section
    QMap<QString, QStringList> sections;
    for (auto it = config.boolSettings.begin(); it != config.boolSettings.end(); ++it) {
        QString section = it.key().section('.', 0, 0);
        QString key = it.key().section('.', 1);
        sections[section].append(key + " = " + (it.value() ? "true" : "false"));
    }
    for (auto it = config.floatSettings.begin(); it != config.floatSettings.end(); ++it) {
        QString section = it.key().section('.', 0, 0);
        QString key = it.key().section('.', 1);
        sections[section].append(key + " = " + QString::number(it.value()));
    }
    for (auto it = config.intSettings.begin(); it != config.intSettings.end(); ++it) {
        QString section = it.key().section('.', 0, 0);
        QString key = it.key().section('.', 1);
        sections[section].append(key + " = " + QString::number(it.value()));
    }

    for (auto it = sections.begin(); it != sections.end(); ++it) {
        newSettings += "  " + it.key() + " = {\n";
        for (const QString& line : it.value()) {
            newSettings += "    " + line + " ,\n";
        }
        newSettings += "  },\n";
    }
    newSettings += "})";

    content.replace(settingsBlockRegex, newSettings);

    // Write updated content
    QFile outFile(luaPath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream outStream(&outFile);
    outStream << content;
    outFile.close();

    return true;
}

QVector<QString> WeatherFxParser::getAvailableScripts(const QString& weatherDir) {
    QVector<QString> scripts;

    QDir dir(weatherDir);
    if (!dir.exists()) return scripts;

    QStringList filters;
    filters << "*.lua";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);

    for (const QFileInfo& fileInfo : files) {
        scripts.append(fileInfo.absoluteFilePath());
    }

    return scripts;
}

} // namespace physics
} // namespace ks

