#include "WeatherConfig.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

namespace ks {
namespace physics {
namespace config {

WeatherConfig::WeatherConfig(QObject* parent) : QObject(parent) {
    loadDefaults();
}

void WeatherConfig::addSequence(const WeatherSequence& sequence) {
    m_sequences.append(sequence);
    emit sequenceAdded(m_sequences.size() - 1);
    emit configChanged();
}

void WeatherConfig::removeSequence(int index) {
    if (index >= 0 && index < m_sequences.size()) {
        m_sequences.removeAt(index);
        emit sequenceRemoved(index);
        emit configChanged();
    }
}

void WeatherConfig::updateSequence(int index, const WeatherSequence& sequence) {
    if (index >= 0 && index < m_sequences.size()) {
        m_sequences[index] = sequence;
        emit sequenceUpdated(index);
        emit configChanged();
    }
}

void WeatherConfig::addKeyframe(int sequenceIndex, const WeatherKeyframe& keyframe) {
    if (sequenceIndex >= 0 && sequenceIndex < m_sequences.size()) {
        auto& seq = m_sequences[sequenceIndex];
        
        int insertIndex = 0;
        for (int i = 0; i < seq.keyframes.size(); ++i) {
            if (seq.keyframes[i].time <= keyframe.time) {
                insertIndex = i + 1;
            } else {
                break;
            }
        }
        
        seq.keyframes.insert(insertIndex, keyframe);
        emit keyframeAdded(sequenceIndex, insertIndex);
        emit configChanged();
    }
}

void WeatherConfig::removeKeyframe(int sequenceIndex, int keyframeIndex) {
    if (sequenceIndex >= 0 && sequenceIndex < m_sequences.size()) {
        auto& seq = m_sequences[sequenceIndex];
        if (keyframeIndex >= 0 && keyframeIndex < seq.keyframes.size()) {
            seq.keyframes.removeAt(keyframeIndex);
            emit keyframeRemoved(sequenceIndex, keyframeIndex);
            emit configChanged();
        }
    }
}

void WeatherConfig::updateKeyframe(int sequenceIndex, int keyframeIndex, const WeatherKeyframe& keyframe) {
    if (sequenceIndex >= 0 && sequenceIndex < m_sequences.size()) {
        auto& seq = m_sequences[sequenceIndex];
        if (keyframeIndex >= 0 && keyframeIndex < seq.keyframes.size()) {
            seq.keyframes[keyframeIndex] = keyframe;
            emit keyframeUpdated(sequenceIndex, keyframeIndex);
            emit configChanged();
        }
    }
}

int WeatherConfig::keyframeCount(int sequenceIndex) const {
    if (sequenceIndex >= 0 && sequenceIndex < m_sequences.size()) {
        return m_sequences[sequenceIndex].keyframes.size();
    }
    return 0;
}

WeatherState WeatherConfig::interpolateWeather(int sequenceIndex, float time) const {
    if (sequenceIndex < 0 || sequenceIndex >= m_sequences.size()) {
        return WeatherState();
    }
    
    const auto& seq = m_sequences[sequenceIndex];
    if (seq.keyframes.isEmpty()) {
        return WeatherState();
    }
    
    if (time <= seq.keyframes.first().time) {
        return seq.keyframes.first().state;
    }
    if (time >= seq.keyframes.last().time) {
        if (seq.loop) {
            float totalDuration = seq.keyframes.last().time;
            if (totalDuration > 0) {
                time = std::fmod(time, totalDuration);
            }
        } else {
            return seq.keyframes.last().state;
        }
    }
    
    for (int i = 0; i < seq.keyframes.size() - 1; ++i) {
        const auto& kf1 = seq.keyframes[i];
        const auto& kf2 = seq.keyframes[i + 1];
        
        if (time >= kf1.time && time <= kf2.time) {
            float t = (time - kf1.time) / (kf2.time - kf1.time);
            
            if (kf1.interpolation == "step") {
                return kf1.state;
            } else if (kf1.interpolation == "smooth") {
                return interpolateSmooth(kf1.state, kf2.state, t);
            } else {
                return interpolateLinear(kf1.state, kf2.state, t);
            }
        }
    }
    
    return seq.keyframes.last().state;
}

bool WeatherConfig::validate() const {
    for (int i = 0; i < m_sequences.size(); ++i) {
        const auto& seq = m_sequences[i];
        
        if (seq.name.isEmpty()) {
            m_validationError = QString("Sequence %1 has empty name").arg(i);
            return false;
        }
        
        if (seq.keyframes.size() < 2) {
            m_validationError = QString("Sequence '%1' needs at least 2 keyframes").arg(seq.name);
            return false;
        }
        
        for (int j = 0; j < seq.keyframes.size() - 1; ++j) {
            if (seq.keyframes[j].time >= seq.keyframes[j + 1].time) {
                m_validationError = QString("Sequence '%1': keyframes not in time order").arg(seq.name);
                return false;
            }
        }
    }
    
    m_validationError.clear();
    return true;
}

QString WeatherConfig::validationError() const {
    return m_validationError;
}

QJsonObject WeatherConfig::toJson() const {
    QJsonObject json;
    
    QJsonArray sequencesArray;
    for (const auto& seq : m_sequences) {
        QJsonObject seqJson;
        seqJson["name"] = seq.name;
        seqJson["duration"] = seq.duration;
        seqJson["loop"] = seq.loop;
        
        QJsonArray keyframesArray;
        for (const auto& kf : seq.keyframes) {
            QJsonObject kfJson;
            kfJson["time"] = kf.time;
            kfJson["interpolation"] = kf.interpolation;
            
            QJsonObject stateJson;
            stateJson["ambientTemp"] = kf.state.ambientTemp;
            stateJson["trackTemp"] = kf.state.trackTemp;
            stateJson["trackWetness"] = kf.state.trackWetness;
            stateJson["rainIntensity"] = kf.state.rainIntensity;
            stateJson["windSpeed"] = kf.state.windSpeed;
            stateJson["windDirection"] = kf.state.windDirection;
            stateJson["cloudCover"] = kf.state.cloudCover;
            
            kfJson["state"] = stateJson;
            keyframesArray.append(kfJson);
        }
        
        seqJson["keyframes"] = keyframesArray;
        sequencesArray.append(seqJson);
    }
    
    json["sequences"] = sequencesArray;
    return json;
}

void WeatherConfig::fromJson(const QJsonObject& json) {
    m_sequences.clear();
    
    QJsonArray sequencesArray = json["sequences"].toArray();
    for (const auto& seqVal : sequencesArray) {
        QJsonObject seqJson = QJsonValue(seqVal).toObject();
        
        WeatherSequence seq;
        seq.name = seqJson["name"].toString();
        seq.duration = seqJson["duration"].toVariant().toFloat();
        seq.loop = seqJson["loop"].toBool();
        
        QJsonArray keyframesArray = seqJson["keyframes"].toArray();
        for (const auto& kfVal : keyframesArray) {
            QJsonObject kfJson = QJsonValue(kfVal).toObject();
            
            WeatherKeyframe kf;
            kf.time = kfJson["time"].toVariant().toFloat();
            kf.interpolation = kfJson["interpolation"].toString();
            
            QJsonObject stateJson = kfJson["state"].toObject();
            kf.state.ambientTemp = stateJson["ambientTemp"].toVariant().toFloat();
            kf.state.trackTemp = stateJson["trackTemp"].toVariant().toFloat();
            kf.state.trackWetness = stateJson["trackWetness"].toVariant().toFloat();
            kf.state.rainIntensity = stateJson["rainIntensity"].toVariant().toFloat();
            kf.state.windSpeed = stateJson["windSpeed"].toVariant().toFloat();
            kf.state.windDirection = stateJson["windDirection"].toVariant().toFloat();
            kf.state.cloudCover = stateJson["cloudCover"].toVariant().toFloat();
            
            seq.keyframes.append(kf);
        }
        
        m_sequences.append(seq);
    }
    
    emit configChanged();
}

bool WeatherConfig::loadFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "WeatherConfig: Cannot open file" << filePath;
        return false;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull()) {
        qWarning() << "WeatherConfig: Invalid JSON in" << filePath;
        return false;
    }
    
    fromJson(doc.object());
    return true;
}

bool WeatherConfig::saveToFile(const QString& filePath) const {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "WeatherConfig: Cannot write to file" << filePath;
        return false;
    }
    
    QJsonDocument doc(toJson());
    file.write(doc.toJson());
    return true;
}

void WeatherConfig::reset() {
    m_sequences.clear();
    emit configChanged();
}

void WeatherConfig::loadDefaults() {
    m_sequences.clear();
    
    WeatherSequence clear;
    clear.name = "Clear";
    clear.duration = 600.0f;
    clear.loop = true;
    
    WeatherKeyframe kf1;
    kf1.time = 0.0f;
    kf1.state.ambientTemp = 25.0f;
    kf1.state.trackTemp = 30.0f;
    kf1.state.trackWetness = 0.0f;
    kf1.state.rainIntensity = 0.0f;
    kf1.state.windSpeed = 5.0f;
    kf1.state.cloudCover = 0.2f;
    clear.keyframes.append(kf1);
    
    WeatherKeyframe kf2;
    kf2.time = 300.0f;
    kf2.state.ambientTemp = 28.0f;
    kf2.state.trackTemp = 35.0f;
    kf2.state.trackWetness = 0.0f;
    kf2.state.rainIntensity = 0.0f;
    kf2.state.windSpeed = 8.0f;
    kf2.state.cloudCover = 0.3f;
    clear.keyframes.append(kf2);
    
    m_sequences.append(clear);
    
    WeatherSequence rain;
    rain.name = "Rain";
    rain.duration = 600.0f;
    rain.loop = true;
    
    WeatherKeyframe rf1;
    rf1.time = 0.0f;
    rf1.state.ambientTemp = 20.0f;
    rf1.state.trackTemp = 22.0f;
    rf1.state.trackWetness = 0.5f;
    rf1.state.rainIntensity = 5.0f;
    rf1.state.windSpeed = 10.0f;
    rf1.state.cloudCover = 0.8f;
    rain.keyframes.append(rf1);
    
    WeatherKeyframe rf2;
    rf2.time = 300.0f;
    rf2.state.ambientTemp = 18.0f;
    rf2.state.trackTemp = 20.0f;
    rf2.state.trackWetness = 0.8f;
    rf2.state.rainIntensity = 15.0f;
    rf2.state.windSpeed = 15.0f;
    rf2.state.cloudCover = 0.9f;
    rain.keyframes.append(rf2);
    
    m_sequences.append(rain);
}

WeatherState WeatherConfig::interpolateLinear(const WeatherState& a, const WeatherState& b, float t) const {
    WeatherState result;
    result.ambientTemp = a.ambientTemp + (b.ambientTemp - a.ambientTemp) * t;
    result.trackTemp = a.trackTemp + (b.trackTemp - a.trackTemp) * t;
    result.trackWetness = a.trackWetness + (b.trackWetness - a.trackWetness) * t;
    result.rainIntensity = a.rainIntensity + (b.rainIntensity - a.rainIntensity) * t;
    result.windSpeed = a.windSpeed + (b.windSpeed - a.windSpeed) * t;
    result.windDirection = a.windDirection + (b.windDirection - a.windDirection) * t;
    result.cloudCover = a.cloudCover + (b.cloudCover - a.cloudCover) * t;
    return result;
}

WeatherState WeatherConfig::interpolateSmooth(const WeatherState& a, const WeatherState& b, float t) const {
    float smoothT = t * t * (3.0f - 2.0f * t);
    return interpolateLinear(a, b, smoothT);
}

float WeatherConfig::clamp(float value, float min, float max) const {
    return std::max(min, std::min(max, value));
}

} // namespace config
} // namespace physics
} // namespace ks
