#pragma once

#include "AiBehaviorModel.h"
#include <QString>
#include <QVector>
#include <QMap>
#include <QJsonObject>
#include <memory>

namespace ks {

struct TelemetrySample {
    float timestamp = 0.0f;
    float speed = 0.0f;
    float rpm = 0.0f;
    int gear = 0;
    float throttle = 0.0f;
    float brake = 0.0f;
    float steering = 0.0f;
    float pos[3] = {0, 0, 0};
    float gForce[3] = {0, 0, 0};
    float lapTime = 0.0f;
};

struct CornerTelemetry {
    int cornerIndex;
    float entrySpeed;
    float apexSpeed;
    float exitSpeed;
    float brakingPoint;
    float turnInPoint;
    float brakePressure;
    float throttleAtApex;
    float steeringAngle;
    float timeLost;
};

struct TrackTelemetryData {
    QString trackName;
    int lapCount;
    double bestLapTime;
    double averageLapTime;
    QVector<CornerTelemetry> corners;
    QVector<float> speedTrace;
    QVector<float> throttleTrace;
    QVector<float> brakeTrace;
    double consistency;
    double drivingScore;
};

class AITelemetryTrainer {
public:
    AITelemetryTrainer();

    void ingestTelemetry(const QString& track, const QVector<TelemetrySample>& samples);
    void analyzeTrack(const QString& track);
    AiBehaviorModel::AiDriverProfile generateOptimizedProfile(const QString& track) const;
    AiBehaviorModel::AiDriverProfile generateProfileForDifficulty(const QString& track, int difficulty) const;

    double getTrackKnowledge(const QString& track) const;
    int getTotalLapsAnalyzed() const { return m_totalLapsAnalyzed; }
    int getTrainedTrackCount() const { return m_trackData.size(); }

    QVector<TrackTelemetryData> getAllTrackData() const;
    TrackTelemetryData getTrackData(const QString& track) const;
    QStringList getKnownTracks() const;

    QJsonObject exportTrainingData() const;
    bool importTrainingData(const QJsonObject& data);

    void clear();

private:
    AiBehaviorModel::AiDriverProfile mapTelemetryToProfile(const TrackTelemetryData& data) const;
    double calculateSkillFromTelemetry(const TrackTelemetryData& data) const;
    double calculateAggressionFromTelemetry(const TrackTelemetryData& data) const;
    double calculateConsistencyFromTelemetry(const TrackTelemetryData& data) const;
    double calculateBrakingEfficiency(const TrackTelemetryData& data) const;
    double calculateApexEfficiency(const TrackTelemetryData& data) const;

    QVector<CornerTelemetry> detectCorners(const QVector<TelemetrySample>& samples) const;
    double calculateCorrelation(const QVector<float>& a, const QVector<float>& b) const;

    QMap<QString, TrackTelemetryData> m_trackData;
    int m_totalLapsAnalyzed = 0;
    static constexpr int kMinLapsForTraining = 3;
    static constexpr int kMaxLapsToRetain = 50;
    static constexpr double kReferenceGrip = 1.3;
};

} // namespace ks
