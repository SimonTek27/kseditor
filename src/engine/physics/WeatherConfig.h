#pragma once

/**
 * @file WeatherConfig.h
 * @brief Weather configuration model for MVC architecture
 * @copyright KS Physics Engine
 */

#include "PhysicsCoreTypes.h"
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>
#include <QString>

namespace ks {
namespace physics {
namespace config {

struct WeatherKeyframe {
    float time = 0.0f;
    WeatherState state;
    QString interpolation = "linear";
};

struct WeatherSequence {
    QString name;
    float duration = 300.0f;
    bool loop = false;
    QVector<WeatherKeyframe> keyframes;
};

class WeatherConfig : public QObject {
    Q_OBJECT
    
public:
    explicit WeatherConfig(QObject* parent = nullptr);
    ~WeatherConfig() override = default;
    
    void addSequence(const WeatherSequence& sequence);
    void removeSequence(int index);
    void updateSequence(int index, const WeatherSequence& sequence);
    QVector<WeatherSequence> sequences() const { return m_sequences; }
    
    void addKeyframe(int sequenceIndex, const WeatherKeyframe& keyframe);
    void removeKeyframe(int sequenceIndex, int keyframeIndex);
    void updateKeyframe(int sequenceIndex, int keyframeIndex, const WeatherKeyframe& keyframe);
    
    int sequenceCount() const { return m_sequences.size(); }
    int keyframeCount(int sequenceIndex) const;
    WeatherState interpolateWeather(int sequenceIndex, float time) const;
    
    bool validate() const;
    QString validationError() const;
    
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);
    
    bool loadFromFile(const QString& filePath);
    bool saveToFile(const QString& filePath) const;
    
    void reset();
    void loadDefaults();
    
signals:
    void sequenceAdded(int index);
    void sequenceRemoved(int index);
    void sequenceUpdated(int index);
    void keyframeAdded(int sequenceIndex, int keyframeIndex);
    void keyframeRemoved(int sequenceIndex, int keyframeIndex);
    void keyframeUpdated(int sequenceIndex, int keyframeIndex);
    void configChanged();
    
private:
    WeatherState interpolateLinear(const WeatherState& a, const WeatherState& b, float t) const;
    WeatherState interpolateSmooth(const WeatherState& a, const WeatherState& b, float t) const;
    float clamp(float value, float min, float max) const;
    
    QVector<WeatherSequence> m_sequences;
    mutable QString m_validationError;
};

} // namespace config
} // namespace physics
} // namespace ks
