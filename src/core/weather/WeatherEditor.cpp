#include "WeatherEditorModule.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include <QDateTime>
#include <QDir>
#include <QImage>
#include <QPainter>
#include <QRandomGenerator>
#include <QTextStream>

namespace ks {
namespace weather {

WeatherEditor::WeatherEditor(QObject* parent)
    : QObject(parent)
    , m_currentFile("")
    , m_dirty(false)
{
    m_config.name = "New Weather Config";
    m_config.version = 1;
}

bool WeatherEditor::createNewConfig(const QString& name)
{
    m_config = WeatherConfig();
    m_config.name = name;
    m_config.version = 1;
    m_currentFile.clear();
    m_dirty = true;
    emit configChanged();
    return true;
}

bool WeatherEditor::loadConfig(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return false;
    
    QByteArray data = file.readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) return false;
    
    QJsonObject obj = doc.object();
    m_config.version = obj["version"].toInt(1);
    m_config.name = obj["name"].toString();
    m_config.description = obj["description"].toString();
    m_config.author = obj["author"].toString();
    m_config.created = QDateTime::fromString(obj["created"].toString(), Qt::ISODate);
    m_config.modified = QDateTime::fromString(obj["modified"].toString(), Qt::ISODate);
    m_config.trackName = obj["trackName"].toString();
    m_config.baseTime = obj["baseTime"].toDouble(12.0);
    m_config.timeMultiplier = obj["timeMultiplier"].toDouble(1.0);
    m_config.dynamicWeather = obj["dynamicWeather"].toBool(true);
    m_config.weatherChangeInterval = obj["weatherChangeInterval"].toDouble(2.0);
    m_config.solConfigPath = obj["solConfigPath"].toString();
    m_config.weatherLuaPath = obj["weatherLuaPath"].toString();

    // Deserialize sequences
    m_config.sequences.clear();
    QJsonArray seqArray = obj["sequences"].toArray();
    for (const auto& seqVal : seqArray) {
        QJsonObject seqObj = seqVal.toObject();
        WeatherSequence seq;
        seq.id = seqObj["id"].toString();
        seq.name = seqObj["name"].toString();
        seq.description = seqObj["description"].toString();
        seq.startTime = seqObj["startTime"].toDouble();
        seq.duration = seqObj["duration"].toDouble();
        seq.loop = seqObj["loop"].toBool(true);
        seq.enabled = seqObj["enabled"].toBool(true);

        QJsonArray kfArray = seqObj["keyframes"].toArray();
        for (const auto& kfVal : kfArray) {
            QJsonObject kfObj = kfVal.toObject();
            WeatherKeyframe kf;
            kf.id = kfObj["id"].toString();
            kf.time = kfObj["time"].toDouble();
            kf.type = kfObj["type"].toString();
            kf.cloudCoverage = kfObj["cloudCoverage"].toDouble();
            kf.precipitation = kfObj["precipitation"].toDouble();
            kf.windSpeed = kfObj["windSpeed"].toDouble();
            kf.windDirection = kfObj["windDirection"].toDouble();
            kf.temperature = kfObj["temperature"].toDouble();
            kf.humidity = kfObj["humidity"].toDouble();
            kf.pressure = kfObj["pressure"].toDouble();
            kf.visibility = kfObj["visibility"].toDouble();
            kf.transitionType = kfObj["transitionType"].toString();

            QJsonObject valsObj = kfObj["values"].toObject();
            for (auto it = valsObj.begin(); it != valsObj.end(); ++it) {
                kf.values[it.key()] = it.value().toDouble();
            }
            seq.keyframes.append(kf);
        }

        QJsonArray peArray = seqObj["particleEffects"].toArray();
        for (const auto& peVal : peArray) {
            QJsonObject peObj = peVal.toObject();
            ParticleEffect pe;
            pe.id = peObj["id"].toString();
            pe.type = peObj["type"].toString();
            pe.intensity = peObj["intensity"].toDouble(1.0);
            pe.coverage = peObj["coverage"].toDouble(1.0);
            pe.particleSize = peObj["particleSize"].toDouble(1.0);
            pe.speed = peObj["speed"].toDouble(1.0);
            pe.texturePath = peObj["texturePath"].toString();
            seq.particleEffects.append(pe);
        }

        m_config.sequences.append(seq);
    }
    
    m_currentFile = filePath;
    m_dirty = false;
    emit configChanged();
    return true;
}

bool WeatherEditor::saveConfig(const QString& filePath)
{
    QString path = filePath.isEmpty() ? m_currentFile : filePath;
    if (path.isEmpty()) return false;
    
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;
    
    QJsonDocument doc(serialize());
    file.write(doc.toJson(QJsonDocument::Indented));
    m_currentFile = path;
    m_dirty = false;
    return true;
}

bool WeatherEditor::saveConfigAs(const QString& filePath)
{
    return saveConfig(filePath);
}

QString WeatherEditor::addSequence(const QString& name, double startTime, double duration)
{
    WeatherSequence seq;
    seq.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    seq.name = name;
    seq.startTime = startTime;
    seq.duration = duration;
    seq.enabled = true;
    m_config.sequences.append(seq);
    m_dirty = true;
    emit sequenceAdded(seq.id);
    emit configChanged();
    return seq.id;
}

bool WeatherEditor::removeSequence(const QString& sequenceId)
{
    for (int i = 0; i < m_config.sequences.size(); ++i) {
        if (m_config.sequences[i].id == sequenceId) {
            m_config.sequences.removeAt(i);
            m_dirty = true;
            emit sequenceRemoved(sequenceId);
            emit configChanged();
            return true;
        }
    }
    return false;
}

bool WeatherEditor::duplicateSequence(const QString& sequenceId, const QString& newName)
{
    for (int i = 0; i < m_config.sequences.size(); ++i) {
        if (m_config.sequences[i].id == sequenceId) {
            WeatherSequence seq = m_config.sequences[i];
            seq.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            seq.name = newName;
            m_config.sequences.insert(i + 1, seq);
            m_dirty = true;
            emit sequenceAdded(seq.id);
            emit configChanged();
            return true;
        }
    }
    return false;
}

bool WeatherEditor::reorderSequences(const QVector<QString>& sequenceIds)
{
    QVector<WeatherSequence> newSeqs;
    for (const QString& id : sequenceIds) {
        for (const auto& seq : m_config.sequences) {
            if (seq.id == id) {
                newSeqs.append(seq);
                break;
            }
        }
    }
    if (newSeqs.size() == sequenceIds.size()) {
        m_config.sequences = newSeqs;
        m_dirty = true;
        emit configChanged();
        return true;
    }
    return false;
}

QString WeatherEditor::addKeyframe(const QString& sequenceId, double time, const QString& type)
{
    for (auto& seq : m_config.sequences) {
        if (seq.id == sequenceId) {
            WeatherKeyframe kf;
            kf.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            kf.time = time;
            kf.type = type;
            kf.values = WeatherKeyframe::defaultValuesForType(type);
            seq.keyframes.append(kf);
            m_dirty = true;
            emit keyframeAdded(sequenceId, kf.id);
            emit configChanged();
            return kf.id;
        }
    }
    return QString();
}

bool WeatherEditor::removeKeyframe(const QString& sequenceId, const QString& keyframeId)
{
    for (auto& seq : m_config.sequences) {
        if (seq.id == sequenceId) {
            for (int i = 0; i < seq.keyframes.size(); ++i) {
                if (seq.keyframes[i].id == keyframeId) {
                    seq.keyframes.removeAt(i);
                    m_dirty = true;
                    emit keyframeRemoved(sequenceId, keyframeId);
                    emit configChanged();
                    return true;
                }
            }
        }
    }
    return false;
}

bool WeatherEditor::updateKeyframe(const QString& sequenceId, const QString& keyframeId, const WeatherKeyframe& keyframe)
{
    for (auto& seq : m_config.sequences) {
        if (seq.id == sequenceId) {
            for (int i = 0; i < seq.keyframes.size(); ++i) {
                if (seq.keyframes[i].id == keyframeId) {
                    seq.keyframes[i] = keyframe;
                    m_dirty = true;
                    emit keyframeChanged(sequenceId, keyframeId);
                    emit configChanged();
                    return true;
                }
            }
        }
    }
    return false;
}

bool WeatherEditor::moveKeyframe(const QString& sequenceId, const QString& keyframeId, double newTime)
{
    for (auto& seq : m_config.sequences) {
        if (seq.id == sequenceId) {
            for (auto& kf : seq.keyframes) {
                if (kf.id == keyframeId) {
                    kf.time = newTime;
                    m_dirty = true;
                    emit keyframeChanged(sequenceId, keyframeId);
                    emit configChanged();
                    return true;
                }
            }
        }
    }
    return false;
}

WeatherKeyframe WeatherEditor::interpolateKeyframe(const WeatherSequence& sequence, double time) const
{
    const WeatherKeyframe* prev = nullptr;
    const WeatherKeyframe* next = nullptr;
    
    for (const auto& kf : sequence.keyframes) {
        if (kf.time <= time) {
            prev = &kf;
        } else {
            next = &kf;
            break;
        }
    }
    
    if (!prev && !next) return WeatherKeyframe();
    if (!prev) return *next;
    if (!next) return *prev;
    
    double t = (next->time == prev->time) ? 0.0 : (time - prev->time) / (next->time - prev->time);
    t = qBound(0.0, t, 1.0);
    
    WeatherKeyframe result = *prev;
    result.time = time;
    
    for (auto it = result.values.begin(); it != result.values.end(); ++it) {
        double prevVal = prev->values.value(it.key(), 0.0);
        double nextVal = next->values.value(it.key(), 0.0);
        it.value() = prevVal + (nextVal - prevVal) * t;
    }
    
    return result;
}

QVector<WeatherKeyframe> WeatherEditor::generateInterpolatedKeyframes(const WeatherSequence& sequence, double interval) const
{
    QVector<WeatherKeyframe> result;
    if (sequence.keyframes.isEmpty()) return result;
    
    double start = sequence.startTime;
    double end = sequence.startTime + sequence.duration;
    double time = start;
    
    while (time <= end) {
        result.append(interpolateKeyframe(sequence, time));
        time += interval;
    }
    return result;
}

QString WeatherEditor::addParticleEffect(const QString& sequenceId, const ParticleEffect& effect)
{
    for (auto& seq : m_config.sequences) {
        if (seq.id == sequenceId) {
            ParticleEffect eff = effect;
            eff.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            seq.particleEffects.append(eff);
            m_dirty = true;
            emit particleEffectAdded(sequenceId, eff.id);
            emit configChanged();
            return eff.id;
        }
    }
    return QString();
}

bool WeatherEditor::removeParticleEffect(const QString& sequenceId, const QString& effectId)
{
    for (auto& seq : m_config.sequences) {
        if (seq.id == sequenceId) {
            for (int i = 0; i < seq.particleEffects.size(); ++i) {
                if (seq.particleEffects[i].id == effectId) {
                    seq.particleEffects.removeAt(i);
                    m_dirty = true;
                    emit particleEffectRemoved(sequenceId, effectId);
                    emit configChanged();
                    return true;
                }
            }
        }
    }
    return false;
}

bool WeatherEditor::updateParticleEffect(const QString& sequenceId, const QString& effectId, const ParticleEffect& effect)
{
    for (auto& seq : m_config.sequences) {
        if (seq.id == sequenceId) {
            for (int i = 0; i < seq.particleEffects.size(); ++i) {
                if (seq.particleEffects[i].id == effectId) {
                    seq.particleEffects[i] = effect;
                    m_dirty = true;
                    emit particleEffectChanged(sequenceId, effectId);
                    emit configChanged();
                    return true;
                }
            }
        }
    }
    return false;
}

void WeatherEditor::loadBuiltinPresets()
{
    m_presets.clear();

    // Clear Sky
    {
        WeatherPreset preset;
        preset.name = "Clear Sky";
        preset.category = "clear";
        preset.description = "Sunny day with no clouds";
        preset.config.name = "Clear Sky";
        preset.config.baseTime = 12.0;
        preset.config.dynamicWeather = false;
        preset.skyColor = QColor(135, 206, 235);
        preset.fogColor = QColor(200, 200, 220);
        preset.fogDensity = 0.0;
        WeatherSequence seq;
        seq.name = "default";
        seq.startTime = 0;
        seq.duration = 24;
        WeatherKeyframe kf;
        kf.time = 12.0;
        kf.type = "clear";
        kf.cloudCoverage = 0.05;
        kf.precipitation = 0.0;
        kf.windSpeed = 2.0;
        kf.windDirection = 180.0;
        kf.temperature = 25.0;
        kf.humidity = 0.4;
        kf.pressure = 1015.0;
        kf.visibility = 30.0;
        kf.transitionType = "linear";
        seq.keyframes.append(kf);
        preset.config.sequences.append(seq);
        m_presets.append(preset);
    }

    // Overcast
    {
        WeatherPreset preset;
        preset.name = "Overcast";
        preset.category = "cloudy";
        preset.description = "Heavy cloud cover, no direct sunlight";
        preset.config.name = "Overcast";
        preset.config.baseTime = 14.0;
        preset.config.dynamicWeather = false;
        preset.skyColor = QColor(160, 170, 180);
        preset.fogColor = QColor(180, 185, 190);
        preset.fogDensity = 0.1;
        WeatherSequence seq;
        seq.name = "default";
        seq.startTime = 0;
        seq.duration = 24;
        WeatherKeyframe kf;
        kf.time = 14.0;
        kf.type = "overcast";
        kf.cloudCoverage = 0.9;
        kf.precipitation = 0.0;
        kf.windSpeed = 10.0;
        kf.windDirection = 220.0;
        kf.temperature = 15.0;
        kf.humidity = 0.7;
        kf.pressure = 1008.0;
        kf.visibility = 8.0;
        kf.transitionType = "linear";
        seq.keyframes.append(kf);
        preset.config.sequences.append(seq);
        m_presets.append(preset);
    }

    // Light Rain
    {
        WeatherPreset preset;
        preset.name = "Light Rain";
        preset.category = "rain";
        preset.description = "Light drizzle with scattered clouds";
        preset.config.name = "Light Rain";
        preset.config.baseTime = 10.0;
        preset.config.dynamicWeather = false;
        preset.skyColor = QColor(140, 150, 165);
        preset.fogColor = QColor(170, 175, 185);
        preset.fogDensity = 0.2;
        WeatherSequence seq;
        seq.name = "default";
        seq.startTime = 0;
        seq.duration = 24;
        WeatherKeyframe kf;
        kf.time = 10.0;
        kf.type = "light_rain";
        kf.cloudCoverage = 0.7;
        kf.precipitation = 0.3;
        kf.windSpeed = 8.0;
        kf.windDirection = 200.0;
        kf.temperature = 12.0;
        kf.humidity = 0.8;
        kf.pressure = 1005.0;
        kf.visibility = 6.0;
        kf.transitionType = "linear";
        seq.keyframes.append(kf);
        preset.config.sequences.append(seq);
        m_presets.append(preset);
    }

    // Heavy Rain
    {
        WeatherPreset preset;
        preset.name = "Heavy Rain";
        preset.category = "rain";
        preset.description = "Heavy downpour with strong wind";
        preset.config.name = "Heavy Rain";
        preset.config.baseTime = 14.0;
        preset.config.dynamicWeather = false;
        preset.skyColor = QColor(80, 85, 95);
        preset.fogColor = QColor(130, 135, 140);
        preset.fogDensity = 0.4;
        WeatherSequence seq;
        seq.name = "default";
        seq.startTime = 0;
        seq.duration = 24;
        WeatherKeyframe kf;
        kf.time = 14.0;
        kf.type = "heavy_rain";
        kf.cloudCoverage = 0.9;
        kf.precipitation = 0.8;
        kf.windSpeed = 20.0;
        kf.windDirection = 250.0;
        kf.temperature = 8.0;
        kf.humidity = 0.95;
        kf.pressure = 998.0;
        kf.visibility = 2.0;
        kf.transitionType = "linear";
        seq.keyframes.append(kf);
        preset.config.sequences.append(seq);
        m_presets.append(preset);
    }

    // Thunderstorm
    {
        WeatherPreset preset;
        preset.name = "Thunderstorm";
        preset.category = "storm";
        preset.description = "Severe storm with heavy rain, strong wind and lightning";
        preset.config.name = "Thunderstorm";
        preset.config.baseTime = 16.0;
        preset.config.dynamicWeather = false;
        preset.skyColor = QColor(50, 50, 60);
        preset.fogColor = QColor(90, 95, 100);
        preset.fogDensity = 0.6;
        WeatherSequence seq;
        seq.name = "default";
        seq.startTime = 0;
        seq.duration = 24;
        WeatherKeyframe kf;
        kf.time = 16.0;
        kf.type = "storm";
        kf.cloudCoverage = 0.95;
        kf.precipitation = 1.0;
        kf.windSpeed = 30.0;
        kf.windDirection = 270.0;
        kf.temperature = 5.0;
        kf.humidity = 1.0;
        kf.pressure = 990.0;
        kf.visibility = 0.5;
        kf.transitionType = "linear";
        seq.keyframes.append(kf);
        preset.config.sequences.append(seq);
        m_presets.append(preset);
    }

    // Fog
    {
        WeatherPreset preset;
        preset.name = "Fog";
        preset.category = "fog";
        preset.description = "Dense fog with very low visibility";
        preset.config.name = "Fog";
        preset.config.baseTime = 8.0;
        preset.config.dynamicWeather = false;
        preset.skyColor = QColor(190, 195, 200);
        preset.fogColor = QColor(200, 200, 205);
        preset.fogDensity = 0.8;
        WeatherSequence seq;
        seq.name = "default";
        seq.startTime = 0;
        seq.duration = 24;
        WeatherKeyframe kf;
        kf.time = 8.0;
        kf.type = "fog";
        kf.cloudCoverage = 0.3;
        kf.precipitation = 0.0;
        kf.windSpeed = 1.0;
        kf.windDirection = 0.0;
        kf.temperature = 10.0;
        kf.humidity = 0.95;
        kf.pressure = 1018.0;
        kf.visibility = 0.3;
        kf.transitionType = "linear";
        seq.keyframes.append(kf);
        preset.config.sequences.append(seq);
        m_presets.append(preset);
    }

    // Snow
    {
        WeatherPreset preset;
        preset.name = "Snow";
        preset.category = "snow";
        preset.description = "Snowfall with freezing temperatures";
        preset.config.name = "Snow";
        preset.config.baseTime = 12.0;
        preset.config.dynamicWeather = false;
        preset.skyColor = QColor(200, 200, 210);
        preset.fogColor = QColor(210, 210, 215);
        preset.fogDensity = 0.3;
        WeatherSequence seq;
        seq.name = "default";
        seq.startTime = 0;
        seq.duration = 24;
        WeatherKeyframe kf;
        kf.time = 12.0;
        kf.type = "snow";
        kf.cloudCoverage = 0.85;
        kf.precipitation = 0.5;
        kf.windSpeed = 5.0;
        kf.windDirection = 90.0;
        kf.temperature = -5.0;
        kf.humidity = 0.8;
        kf.pressure = 1012.0;
        kf.visibility = 4.0;
        kf.transitionType = "linear";
        seq.keyframes.append(kf);
        preset.config.sequences.append(seq);
        m_presets.append(preset);
    }

    // Night
    {
        WeatherPreset preset;
        preset.name = "Night";
        preset.category = "clear";
        preset.description = "Clear night sky with moonlight";
        preset.config.name = "Night";
        preset.config.baseTime = 0.0;
        preset.config.dynamicWeather = false;
        preset.skyColor = QColor(10, 10, 30);
        preset.fogColor = QColor(30, 30, 50);
        preset.fogDensity = 0.05;
        WeatherSequence seq;
        seq.name = "default";
        seq.startTime = 0;
        seq.duration = 24;
        WeatherKeyframe kf;
        kf.time = 0.0;
        kf.type = "clear";
        kf.cloudCoverage = 0.0;
        kf.precipitation = 0.0;
        kf.windSpeed = 1.0;
        kf.windDirection = 0.0;
        kf.temperature = 15.0;
        kf.humidity = 0.5;
        kf.pressure = 1015.0;
        kf.visibility = 20.0;
        kf.transitionType = "linear";
        seq.keyframes.append(kf);
        preset.config.sequences.append(seq);
        m_presets.append(preset);
    }

    // Sunset
    {
        WeatherPreset preset;
        preset.name = "Sunset";
        preset.category = "clear";
        preset.description = "Golden hour sunset with warm colors";
        preset.config.name = "Sunset";
        preset.config.baseTime = 18.0;
        preset.config.dynamicWeather = false;
        preset.skyColor = QColor(255, 140, 80);
        preset.fogColor = QColor(255, 200, 150);
        preset.fogDensity = 0.08;
        WeatherSequence seq;
        seq.name = "default";
        seq.startTime = 0;
        seq.duration = 24;
        WeatherKeyframe kf;
        kf.time = 18.0;
        kf.type = "clear";
        kf.cloudCoverage = 0.2;
        kf.precipitation = 0.0;
        kf.windSpeed = 3.0;
        kf.windDirection = 160.0;
        kf.temperature = 22.0;
        kf.humidity = 0.45;
        kf.pressure = 1014.0;
        kf.visibility = 25.0;
        kf.transitionType = "linear";
        seq.keyframes.append(kf);
        preset.config.sequences.append(seq);
        m_presets.append(preset);
    }
}

QVector<WeatherPreset> WeatherEditor::getBuiltinPresets() const
{
    return m_presets;
}

bool WeatherEditor::applyPreset(const QString& presetName)
{
    for (const auto& preset : m_presets) {
        if (preset.name == presetName) {
            m_config = preset.config;
            m_dirty = true;
            emit configChanged();
            return true;
        }
    }
    return false;
}

bool WeatherEditor::saveAsPreset(const QString& presetName, const QString& category)
{
    WeatherPreset preset;
    preset.name = presetName;
    preset.category = category;
    preset.config = m_config;
    m_presets.append(preset);
    return true;
}

void WeatherEditor::loadPresetLibrary(const QString& libraryPath)
{
    QFile file(libraryPath);
    if (!file.open(QIODevice::ReadOnly)) return;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isArray()) return;
    
    m_presets.clear();
    for (const auto& val : doc.array()) {
        WeatherPreset preset;
        preset.deserialize(val.toObject());
        m_presets.append(preset);
    }
}

void WeatherEditor::savePresetLibrary(const QString& libraryPath) const
{
    QJsonArray arr;
    for (const auto& preset : m_presets) {
        arr.append(preset.serialize());
    }
    QFile file(libraryPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
    }
}

QImage WeatherEditor::generatePreview(int width, int height, double time) const
{
    WeatherPreviewRenderer renderer;
    QImage preview = renderer.renderPreview(m_config, time, width, height);
    emit const_cast<WeatherEditor*>(this)->previewGenerated(preview);
    return preview;
}

QImage WeatherEditor::generatePreviewFrame(const WeatherConfig& config, double time, int width, int height) const
{
    WeatherPreviewRenderer renderer;
    return renderer.renderPreview(config, time, width, height);
}

bool WeatherEditor::exportToCSP(const QString& outputDir)
{
    emit exportProgress(0, tr("Exporting CSP weather config..."));

    QDir dir(outputDir);
    if (!dir.exists()) QDir().mkpath(outputDir);

    // Generate CSP extra_cfg.yml style config
    WeatherConfigParser parser;
    WeatherConfigParser::WeatherPreset preset;
    preset.name = m_config.name;
    preset.description = m_config.description;
    preset.author = m_config.author;
    preset.timeOfDay = static_cast<float>(m_config.baseTime);
    preset.timeMultiplier = static_cast<float>(m_config.timeMultiplier);

    // Extract weather conditions from first keyframe
    if (!m_config.sequences.isEmpty() && !m_config.sequences.first().keyframes.isEmpty()) {
        const auto& kf = m_config.sequences.first().keyframes.first();
        preset.ambientTemperature = static_cast<float>(kf.temperature);
        preset.humidity = static_cast<float>(kf.humidity * 100.0);
        preset.windSpeed = static_cast<float>(kf.windSpeed);
        preset.windDirection = static_cast<float>(kf.windDirection);
        preset.rainIntensity = static_cast<float>(kf.precipitation);
        preset.cloudIntensity = static_cast<float>(kf.cloudCoverage);
    }

    QString configPath = dir.filePath("weather_config.ini");
    bool ok = WeatherConfigParser::saveCspConfig(preset, configPath);

    emit exportProgress(100, ok ? tr("CSP export complete") : tr("CSP export failed"));
    emit exportFinished(ok, ok ? tr("CSP config exported to %1").arg(configPath) : tr("Failed to write CSP config"));
    return ok;
}

bool WeatherEditor::exportToACC(const QString& outputDir)
{
    emit exportProgress(0, tr("Exporting ACC weather config..."));

    QDir dir(outputDir);
    if (!dir.exists()) QDir().mkpath(outputDir);

    WeatherConfigParser parser;
    WeatherConfigParser::WeatherPreset preset;
    preset.name = m_config.name;
    preset.description = m_config.description;
    preset.timeOfDay = static_cast<float>(m_config.baseTime);
    preset.timeMultiplier = static_cast<float>(m_config.timeMultiplier);

    if (!m_config.sequences.isEmpty() && !m_config.sequences.first().keyframes.isEmpty()) {
        const auto& kf = m_config.sequences.first().keyframes.first();
        preset.ambientTemperature = static_cast<float>(kf.temperature);
        preset.humidity = static_cast<float>(kf.humidity * 100.0);
        preset.windSpeed = static_cast<float>(kf.windSpeed);
        preset.windDirection = static_cast<float>(kf.windDirection);
        preset.rainIntensity = static_cast<float>(kf.precipitation);
        preset.cloudIntensity = static_cast<float>(kf.cloudCoverage);
    }

    // Write INI format for ACC weather
    QString configPath = dir.filePath("weather.ini");
    bool ok = WeatherConfigParser::savePureConfig(preset, configPath);

    // Also write JSON with full timeline
    QString jsonPath = dir.filePath("weather_timeline.json");
    QFile jsonFile(jsonPath);
    if (jsonFile.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(serialize());
        jsonFile.write(doc.toJson(QJsonDocument::Indented));
        jsonFile.close();
    }

    emit exportProgress(100, ok ? tr("ACC export complete") : tr("ACC export failed"));
    emit exportFinished(ok, ok ? tr("ACC config exported to %1").arg(configPath) : tr("Failed to write ACC config"));
    return ok;
}

bool WeatherEditor::exportToSOL(const QString& outputDir)
{
    emit exportProgress(0, tr("Exporting SOL weather config..."));

    QDir dir(outputDir);
    if (!dir.exists()) QDir().mkpath(outputDir);

    WeatherConfigParser parser;
    WeatherConfigParser::WeatherPreset preset;
    preset.name = m_config.name;
    preset.description = m_config.description;
    preset.timeOfDay = static_cast<float>(m_config.baseTime);
    preset.timeMultiplier = static_cast<float>(m_config.timeMultiplier);

    if (!m_config.sequences.isEmpty() && !m_config.sequences.first().keyframes.isEmpty()) {
        const auto& kf = m_config.sequences.first().keyframes.first();
        preset.ambientTemperature = static_cast<float>(kf.temperature);
        preset.humidity = static_cast<float>(kf.humidity * 100.0);
        preset.windSpeed = static_cast<float>(kf.windSpeed);
        preset.windDirection = static_cast<float>(kf.windDirection);
        preset.rainIntensity = static_cast<float>(kf.precipitation);
        preset.cloudIntensity = static_cast<float>(kf.cloudCoverage);
    }

    // Write Sol format
    QString configPath = dir.filePath("sol_weather.ini");
    bool ok = WeatherConfigParser::saveSolConfig(preset, configPath);

    // Write Lua weather script
    if (!m_config.weatherLuaPath.isEmpty()) {
        QFile::copy(m_config.weatherLuaPath, dir.filePath("weather.lua"));
    }

    emit exportProgress(100, ok ? tr("SOL export complete") : tr("SOL export failed"));
    emit exportFinished(ok, ok ? tr("SOL config exported to %1").arg(configPath) : tr("Failed to write SOL config"));
    return ok;
}

bool WeatherEditor::exportToLua(const QString& filePath)
{
    emit exportProgress(0, tr("Exporting Lua weather script..."));

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit exportFinished(false, tr("Cannot open file for writing: %1").arg(filePath));
        return false;
    }

    QTextStream out(&file);
    out << "-- Weather Lua Configuration\n";
    out << "-- Generated by ksEditor\n";
    out << "-- WeatherFX Script for CSP\n\n";

    out << "ScriptSettings = ac.INIConfig.scriptSettings():mapConfig({\n";

    // Group settings by section
    QMap<QString, QStringList> sections;

    sections["TIME"].append("TIME_OF_DAY = " + QString::number(m_config.baseTime, 'f', 2));
    sections["TIME"].append("TIME_MULTIPLIER = " + QString::number(m_config.timeMultiplier, 'f', 2));

    sections["WEATHER"].append("NAME = \"" + m_config.name + "\"");
    sections["WEATHER"].append("DESCRIPTION = \"" + m_config.description + "\"");

    if (!m_config.sequences.isEmpty() && !m_config.sequences.first().keyframes.isEmpty()) {
        const auto& kf = m_config.sequences.first().keyframes.first();
        sections["CONDITIONS"].append("TEMPERATURE = " + QString::number(kf.temperature, 'f', 1));
        sections["CONDITIONS"].append("HUMIDITY = " + QString::number(kf.humidity * 100.0, 'f', 1));
        sections["CONDITIONS"].append("WIND_SPEED = " + QString::number(kf.windSpeed, 'f', 1));
        sections["CONDITIONS"].append("WIND_DIRECTION = " + QString::number(kf.windDirection, 'f', 1));
        sections["CONDITIONS"].append("RAIN = " + QString::number(kf.precipitation, 'f', 2));
        sections["CONDITIONS"].append("CLOUDS = " + QString::number(kf.cloudCoverage, 'f', 2));
    }

    for (auto it = sections.begin(); it != sections.end(); ++it) {
        out << "  " << it.key() << " = {\n";
        for (const QString& line : it.value()) {
            out << "    " << line << " ,\n";
        }
        out << "  },\n";
    }

    out << "})\n";

    file.close();
    emit exportProgress(100, tr("Lua export complete"));
    emit exportFinished(true, tr("Lua script exported to %1").arg(filePath));
    return true;
}

bool WeatherEditor::exportTimeline(const QString& filePath, const QString& format)
{
    QString fmt = format.toLower();
    if (fmt == "lua") {
        return exportToLua(filePath);
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) return false;

    if (fmt == "json" || fmt.isEmpty()) {
        QJsonDocument doc(serialize());
        file.write(doc.toJson(QJsonDocument::Indented));
    } else {
        file.close();
        return false;
    }

    file.close();
    return true;
}

bool WeatherEditor::validateConfig(QStringList* errors)
{
    bool valid = true;
    if (m_config.sequences.isEmpty()) {
        valid = false;
        if (errors) errors->append("No sequences defined");
    }
    return valid;
}

bool WeatherEditor::checkKeyframeContinuity(QStringList* warnings)
{
    bool ok = true;
    for (const auto& seq : m_config.sequences) {
        if (seq.keyframes.size() < 2) continue;
        for (int i = 1; i < seq.keyframes.size(); ++i) {
            double gap = seq.keyframes[i].time - seq.keyframes[i-1].time;
            if (gap > 10.0) {
                if (warnings)
                    warnings->append(QString("Large time gap (%.1fs) between keyframes %1 and %2 in sequence '%3'")
                        .arg(gap).arg(seq.keyframes[i-1].id).arg(seq.keyframes[i].id).arg(seq.name));
                ok = false;
            }
        }
    }
    return ok;
}

bool WeatherEditor::checkTimeConflicts(QStringList* errors)
{
    bool ok = true;
    for (const auto& seq : m_config.sequences) {
        QMap<double, int> timeCount;
        for (const auto& kf : seq.keyframes) {
            timeCount[kf.time]++;
        }
        for (auto it = timeCount.begin(); it != timeCount.end(); ++it) {
            if (it.value() > 1) {
                if (errors)
                    errors->append(QString("Time conflict at %.1fs in sequence '%1': %2 keyframes at same time")
                        .arg(it.key()).arg(seq.name).arg(it.value()));
                ok = false;
            }
        }
    }
    return ok;
}

QJsonObject WeatherEditor::serialize() const
{
    QJsonObject obj;
    obj["version"] = m_config.version;
    obj["name"] = m_config.name;
    obj["description"] = m_config.description;
    obj["author"] = m_config.author;
    obj["created"] = m_config.created.toString(Qt::ISODate);
    obj["modified"] = m_config.modified.toString(Qt::ISODate);
    obj["trackName"] = m_config.trackName;
    obj["baseTime"] = m_config.baseTime;
    obj["timeMultiplier"] = m_config.timeMultiplier;
    obj["dynamicWeather"] = m_config.dynamicWeather;
    obj["weatherChangeInterval"] = m_config.weatherChangeInterval;
    obj["solConfigPath"] = m_config.solConfigPath;
    obj["weatherLuaPath"] = m_config.weatherLuaPath;

    // Serialize sequences
    QJsonArray seqArray;
    for (const auto& seq : m_config.sequences) {
        QJsonObject seqObj;
        seqObj["id"] = seq.id;
        seqObj["name"] = seq.name;
        seqObj["description"] = seq.description;
        seqObj["startTime"] = seq.startTime;
        seqObj["duration"] = seq.duration;
        seqObj["loop"] = seq.loop;
        seqObj["enabled"] = seq.enabled;

        QJsonArray kfArray;
        for (const auto& kf : seq.keyframes) {
            QJsonObject kfObj;
            kfObj["id"] = kf.id;
            kfObj["time"] = kf.time;
            kfObj["type"] = kf.type;
            kfObj["cloudCoverage"] = kf.cloudCoverage;
            kfObj["precipitation"] = kf.precipitation;
            kfObj["windSpeed"] = kf.windSpeed;
            kfObj["windDirection"] = kf.windDirection;
            kfObj["temperature"] = kf.temperature;
            kfObj["humidity"] = kf.humidity;
            kfObj["pressure"] = kf.pressure;
            kfObj["visibility"] = kf.visibility;
            kfObj["transitionType"] = kf.transitionType;

            QJsonObject valsObj;
            for (auto it = kf.values.begin(); it != kf.values.end(); ++it) {
                valsObj[it.key()] = it.value();
            }
            kfObj["values"] = valsObj;
            kfArray.append(kfObj);
        }
        seqObj["keyframes"] = kfArray;

        QJsonArray peArray;
        for (const auto& pe : seq.particleEffects) {
            QJsonObject peObj;
            peObj["id"] = pe.id;
            peObj["type"] = pe.type;
            peObj["intensity"] = pe.intensity;
            peObj["coverage"] = pe.coverage;
            peObj["particleSize"] = pe.particleSize;
            peObj["speed"] = pe.speed;
            peObj["texturePath"] = pe.texturePath;
            peArray.append(peObj);
        }
        seqObj["particleEffects"] = peArray;
        seqArray.append(seqObj);
    }
    obj["sequences"] = seqArray;

    return obj;
}

void WeatherEditor::onTimeChanged(double time)
{
    m_config.baseTime = time;
    m_dirty = true;
    emit configChanged();
}

void WeatherEditor::onSequenceSelectionChanged(const QString& sequenceId)
{
    m_selectedSequence = sequenceId;
    for (const auto& seq : m_config.sequences) {
        if (seq.id == sequenceId) {
            emit sequenceSelected(sequenceId);
            return;
        }
    }
}

void WeatherEditor::onKeyframeSelectionChanged(const QString& sequenceId, const QString& keyframeId)
{
    m_selectedSequence = sequenceId;
    m_selectedKeyframe = keyframeId;
    for (const auto& seq : m_config.sequences) {
        if (seq.id == sequenceId) {
            for (const auto& kf : seq.keyframes) {
                if (kf.id == keyframeId) {
                    emit keyframeSelected(sequenceId, keyframeId);
                    return;
                }
            }
        }
    }
}

void WeatherEditor::onAutoSaveTimer()
{
    if (m_dirty && !m_currentFile.isEmpty()) {
        saveConfig(m_currentFile);
    }
}

// WeatherPreset implementation
QJsonObject WeatherPreset::serialize() const
{
    QJsonObject obj;
    obj["name"] = name;
    obj["category"] = category;
    obj["description"] = description;
    obj["skyColor"] = QJsonObject{{"r", skyColor.red()}, {"g", skyColor.green()}, {"b", skyColor.blue()}};
    obj["fogColor"] = QJsonObject{{"r", fogColor.red()}, {"g", fogColor.green()}, {"b", fogColor.blue()}};
    obj["fogDensity"] = fogDensity;
    obj["exposure"] = exposure;
    obj["gamma"] = gamma;
    return obj;
}

void WeatherPreset::deserialize(const QJsonObject& obj)
{
    name = obj["name"].toString();
    category = obj["category"].toString();
    description = obj["description"].toString();
    
    QJsonObject skyObj = obj["skyColor"].toObject();
    skyColor = QColor(skyObj["r"].toInt(), skyObj["g"].toInt(), skyObj["b"].toInt());
    
    QJsonObject fogObj = obj["fogColor"].toObject();
    fogColor = QColor(fogObj["r"].toInt(), fogObj["g"].toInt(), fogObj["b"].toInt());
    
    fogDensity = obj["fogDensity"].toDouble();
    exposure = obj["exposure"].toDouble();
    gamma = obj["gamma"].toDouble();
}

} // namespace weather
} // namespace ks