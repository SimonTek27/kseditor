#include "AITelemetryTrainer.h"
#include <QJsonArray>
#include <QtMath>
#include <algorithm>
#include <cmath>

namespace ks {

AITelemetryTrainer::AITelemetryTrainer() = default;

void AITelemetryTrainer::ingestTelemetry(const QString& track, const QVector<TelemetrySample>& samples)
{
    if (samples.isEmpty() || track.isEmpty()) return;

    auto& data = m_trackData[track];
    data.trackName = track;
    data.lapCount++;
    m_totalLapsAnalyzed++;

    double lapTime = samples.last().timestamp - samples.first().timestamp;
    if (lapTime <= 0) return;

    if (data.bestLapTime <= 0 || lapTime < data.bestLapTime)
        data.bestLapTime = lapTime;

    data.averageLapTime = ((data.averageLapTime * (data.lapCount - 1)) + lapTime) / data.lapCount;

    QVector<CornerTelemetry> corners = detectCorners(samples);
    if (data.corners.isEmpty() || lapTime <= data.bestLapTime + 0.5)
        data.corners = corners;

    data.speedTrace.clear();
    data.throttleTrace.clear();
    data.brakeTrace.clear();
    for (const auto& s : samples) {
        data.speedTrace.append(s.speed);
        data.throttleTrace.append(s.throttle);
        data.brakeTrace.append(s.brake);
    }

    double variance = 0;
    if (data.lapCount > 1) {
        double diff = lapTime - data.averageLapTime;
        variance = diff * diff;
    }
    data.consistency = (variance < 1.0) ? 1.0 - (variance * 0.1) : qMax(0.1, 1.0 - variance);
    data.drivingScore = qBound(0.0, (1.0 - variance * 0.05) * 100.0, 100.0);

    if (data.lapCount > kMaxLapsToRetain) {
        data.averageLapTime = data.bestLapTime * 1.05;
    }
}

void AITelemetryTrainer::analyzeTrack(const QString& track)
{
    if (!m_trackData.contains(track)) return;
    auto& data = m_trackData[track];

    double totalBrakeEfficiency = 0;
    double totalApexEfficiency = 0;
    int cornerCount = 0;

    for (const auto& corner : data.corners) {
        if (corner.entrySpeed > 0) {
            double speedDrop = (corner.entrySpeed - corner.apexSpeed) / corner.entrySpeed;
            totalBrakeEfficiency += 1.0 - qMin(1.0, speedDrop);
        }
        if (corner.apexSpeed > 0 && corner.exitSpeed > 0) {
            totalApexEfficiency += corner.exitSpeed / qMax(1.0, corner.apexSpeed);
        }
        cornerCount++;
    }

    if (cornerCount > 0) {
        data.consistency = qBound(0.0, (totalBrakeEfficiency / cornerCount) * 0.5
                                  + (totalApexEfficiency / cornerCount) * 0.5, 1.0);
    }
}

AiBehaviorModel::AiDriverProfile AITelemetryTrainer::generateOptimizedProfile(const QString& track) const
{
    if (!m_trackData.contains(track))
        return AiBehaviorModel::getVeteranDriver();

    const auto& data = m_trackData[track];
    return mapTelemetryToProfile(data);
}

AiBehaviorModel::AiDriverProfile AITelemetryTrainer::generateProfileForDifficulty(const QString& track, int difficulty) const
{
    AiBehaviorModel::AiDriverProfile profile;
    profile.name = QString("AI_%1").arg(track);

    if (m_trackData.contains(track)) {
        profile = mapTelemetryToProfile(m_trackData[track]);
    } else {
        profile.skill = 0.7f;
        profile.aggression = 0.5f;
        profile.consistency = 0.7f;
    }

    double factor = difficulty / 100.0;
    profile.skill *= static_cast<float>(factor);
    profile.aggression *= static_cast<float>(0.3 + factor * 0.7);
    profile.consistency *= static_cast<float>(factor);
    profile.mistakeRate = static_cast<float>(1.0 - factor) * 0.3f;
    profile.tireManagement *= static_cast<float>(factor);
    profile.fuelManagement *= static_cast<float>(factor);
    profile.wetSkill *= static_cast<float>(factor);
    profile.qualifyingPace = static_cast<float>(0.3 + factor * 0.7);
    profile.racePace = static_cast<float>(0.3 + factor * 0.7);

    return profile;
}

double AITelemetryTrainer::getTrackKnowledge(const QString& track) const
{
    if (!m_trackData.contains(track)) return 0.0;
    const auto& data = m_trackData[track];
    return qMin(1.0, data.lapCount / 50.0);
}

QVector<TrackTelemetryData> AITelemetryTrainer::getAllTrackData() const
{
    QVector<TrackTelemetryData> result;
    for (auto it = m_trackData.begin(); it != m_trackData.end(); ++it)
        result.append(it.value());
    return result;
}

TrackTelemetryData AITelemetryTrainer::getTrackData(const QString& track) const
{
    static TrackTelemetryData empty;
    auto it = m_trackData.find(track);
    return it != m_trackData.end() ? it.value() : empty;
}

QStringList AITelemetryTrainer::getKnownTracks() const
{
    return m_trackData.keys();
}

QJsonObject AITelemetryTrainer::exportTrainingData() const
{
    QJsonObject root;
    root["totalLaps"] = m_totalLapsAnalyzed;

    QJsonObject tracks;
    for (auto it = m_trackData.begin(); it != m_trackData.end(); ++it) {
        QJsonObject track;
        const auto& data = it.value();
        track["trackName"] = data.trackName;
        track["lapCount"] = data.lapCount;
        track["bestLapTime"] = data.bestLapTime;
        track["averageLapTime"] = data.averageLapTime;
        track["consistency"] = data.consistency;
        track["drivingScore"] = data.drivingScore;

        QJsonArray corners;
        for (const auto& c : data.corners) {
            QJsonObject co;
            co["cornerIndex"] = c.cornerIndex;
            co["entrySpeed"] = c.entrySpeed;
            co["apexSpeed"] = c.apexSpeed;
            co["exitSpeed"] = c.exitSpeed;
            co["brakingPoint"] = c.brakingPoint;
            co["brakePressure"] = c.brakePressure;
            co["steeringAngle"] = c.steeringAngle;
            corners.append(co);
        }
        track["corners"] = corners;
        tracks[it.key()] = track;
    }
    root["tracks"] = tracks;
    return root;
}

bool AITelemetryTrainer::importTrainingData(const QJsonObject& data)
{
    if (data.isEmpty()) return false;

    m_totalLapsAnalyzed = data["totalLaps"].toInt();
    QJsonObject tracks = data["tracks"].toObject();

    for (auto it = tracks.begin(); it != tracks.end(); ++it) {
        QJsonObject track = it.value().toObject();
        TrackTelemetryData td;
        td.trackName = track["trackName"].toString();
        td.lapCount = track["lapCount"].toInt();
        td.bestLapTime = track["bestLapTime"].toDouble();
        td.averageLapTime = track["averageLapTime"].toDouble();
        td.consistency = track["consistency"].toDouble();
        td.drivingScore = track["drivingScore"].toDouble();

        QJsonArray corners = track["corners"].toArray();
        for (const auto& cv : corners) {
            QJsonObject co = cv.toObject();
            CornerTelemetry ct;
            ct.cornerIndex = co["cornerIndex"].toInt();
            ct.entrySpeed = co["entrySpeed"].toDouble();
            ct.apexSpeed = co["apexSpeed"].toDouble();
            ct.exitSpeed = co["exitSpeed"].toDouble();
            ct.brakingPoint = co["brakingPoint"].toDouble();
            ct.brakePressure = co["brakePressure"].toDouble();
            ct.steeringAngle = co["steeringAngle"].toDouble();
            td.corners.append(ct);
        }

        m_trackData[it.key()] = td;
    }

    return true;
}

void AITelemetryTrainer::clear()
{
    m_trackData.clear();
    m_totalLapsAnalyzed = 0;
}

AiBehaviorModel::AiDriverProfile AITelemetryTrainer::mapTelemetryToProfile(const TrackTelemetryData& data) const
{
    AiBehaviorModel::AiDriverProfile profile;
    profile.name = "AI_" + data.trackName;

    if (data.lapCount < kMinLapsForTraining) {
        profile.skill = 0.6f;
        profile.aggression = 0.5f;
        profile.consistency = 0.6f;
        profile.mistakeRate = 0.1f;
        profile.tireManagement = 0.6f;
        profile.fuelManagement = 0.6f;
        profile.wetSkill = 0.5f;
        profile.qualifyingPace = 0.7f;
        profile.racePace = 0.65f;
        return profile;
    }

    double skill = calculateSkillFromTelemetry(data);
    double aggression = calculateAggressionFromTelemetry(data);
    double consistency = calculateConsistencyFromTelemetry(data);
    double brakeEff = calculateBrakingEfficiency(data);
    double apexEff = calculateApexEfficiency(data);

    profile.skill = static_cast<float>(qBound(0.1, skill, 1.0));
    profile.aggression = static_cast<float>(qBound(0.1, aggression, 1.0));
    profile.consistency = static_cast<float>(qBound(0.1, consistency, 1.0));
    profile.mistakeRate = static_cast<float>(qBound(0.01, 1.0 - consistency * 0.5, 0.5));
    profile.tireManagement = static_cast<float>(qBound(0.1, consistency * 0.7 + brakeEff * 0.3, 1.0));
    profile.fuelManagement = static_cast<float>(qBound(0.1, consistency * 0.8 + 0.1, 1.0));
    profile.wetSkill = static_cast<float>(qBound(0.1, skill * 0.7 + 0.1, 1.0));
    profile.qualifyingPace = static_cast<float>(qBound(0.1, skill * 0.9 + apexEff * 0.1, 1.0));
    profile.racePace = static_cast<float>(qBound(0.1, skill * 0.7 + consistency * 0.3, 1.0));

    return profile;
}

double AITelemetryTrainer::calculateSkillFromTelemetry(const TrackTelemetryData& data) const
{
    if (data.lapCount < kMinLapsForTraining) return 0.6;

    double consistencyWeight = 0.3;
    double cornerEfficiencyWeight = 0.4;
    double brakeEfficiencyWeight = 0.3;

    double brakeEff = calculateBrakingEfficiency(data);
    double apexEff = calculateApexEfficiency(data);

    double baseConsistency = qMin(1.0, data.consistency);
    double skill = baseConsistency * consistencyWeight
                 + apexEff * cornerEfficiencyWeight
                 + brakeEff * brakeEfficiencyWeight;

    double lapTimeBonus = 0;
    if (data.averageLapTime > 0 && data.bestLapTime > 0) {
        double delta = (data.averageLapTime - data.bestLapTime) / data.bestLapTime;
        lapTimeBonus = qMax(0.0, 1.0 - delta * 10.0);
    }
    skill = skill * 0.7 + lapTimeBonus * 0.3;

    return qMin(1.0, skill);
}

double AITelemetryTrainer::calculateAggressionFromTelemetry(const TrackTelemetryData& data) const
{
    if (data.throttleTrace.isEmpty() || data.brakeTrace.isEmpty()) return 0.5;

    double avgThrottle = 0;
    for (float t : data.throttleTrace) avgThrottle += t;
    avgThrottle /= data.throttleTrace.size();

    double avgBrake = 0;
    for (float b : data.brakeTrace) avgBrake += b;
    avgBrake /= data.brakeTrace.size();

    double maxSpeed = 0;
    for (float s : data.speedTrace) maxSpeed = qMax(maxSpeed, (double)s);

    double speedAggression = qMin(1.0, maxSpeed / 300.0);
    double throttleAggression = qMin(1.0, avgThrottle * 1.5);
    double brakeAggression = qMin(1.0, avgBrake * 2.0);

    return qBound(0.1, speedAggression * 0.4 + throttleAggression * 0.35 + brakeAggression * 0.25, 1.0);
}

double AITelemetryTrainer::calculateConsistencyFromTelemetry(const TrackTelemetryData& data) const
{
    return qBound(0.1, data.consistency, 1.0);
}

double AITelemetryTrainer::calculateBrakingEfficiency(const TrackTelemetryData& data) const
{
    if (data.corners.isEmpty()) return 0.5;

    double totalEff = 0;
    for (const auto& corner : data.corners) {
        if (corner.entrySpeed > 0) {
            double speedDrop = (corner.entrySpeed - corner.apexSpeed) / corner.entrySpeed;
            totalEff += 1.0 - qMin(1.0, speedDrop);
        }
    }
    return qBound(0.1, totalEff / data.corners.size(), 1.0);
}

double AITelemetryTrainer::calculateApexEfficiency(const TrackTelemetryData& data) const
{
    if (data.corners.isEmpty()) return 0.5;

    double totalEff = 0;
    for (const auto& corner : data.corners) {
        if (corner.apexSpeed > 0 && corner.exitSpeed > 0) {
            totalEff += corner.exitSpeed / qMax(1.0, corner.apexSpeed);
        }
    }
    return qBound(0.1, totalEff / data.corners.size(), 1.0);
}

QVector<CornerTelemetry> AITelemetryTrainer::detectCorners(const QVector<TelemetrySample>& samples) const
{
    QVector<CornerTelemetry> corners;
    if (samples.size() < 10) return corners;

    int cornerIndex = 0;
    int brakeStart = -1;
    bool inCorner = false;

    for (int i = 1; i < samples.size(); ++i) {
        float steering = std::abs(samples[i].steering);

        if (samples[i].brake > 0.05f && !inCorner) {
            brakeStart = i;
            inCorner = true;
        }

        if (steering > 0.15f && !inCorner && i > 0) {
            brakeStart = i - 1;
            inCorner = true;
        }

        if (inCorner && steering < 0.08f && samples[i].throttle > 0.7f) {
            int entryIdx = brakeStart >= 0 ? brakeStart : qMax(0, i - 10);

            int apexIdx = entryIdx;
            float minSpeed = samples[entryIdx].speed;
            for (int j = entryIdx; j <= i && j < samples.size(); ++j) {
                if (samples[j].speed < minSpeed) {
                    minSpeed = samples[j].speed;
                    apexIdx = j;
                }
            }

            int exitIdx = qMin(i + 5, samples.size() - 1);

            CornerTelemetry ct;
            ct.cornerIndex = ++cornerIndex;
            ct.entrySpeed = samples[entryIdx].speed;
            ct.apexSpeed = samples[apexIdx].speed;
            ct.exitSpeed = samples[exitIdx].speed;
            ct.brakingPoint = samples[entryIdx].timestamp;
            ct.turnInPoint = samples[apexIdx].timestamp;
            ct.brakePressure = samples[apexIdx].brake;
            ct.throttleAtApex = samples[apexIdx].throttle;
            ct.steeringAngle = samples[apexIdx].steering;

            double theoreticalMinSpeed = std::sqrt(kReferenceGrip * 9.81 * (1.0 / qMax(0.01, (double)std::abs(samples[apexIdx].steering)))) * 3.6;
            ct.timeLost = qMax(0.0, (theoreticalMinSpeed - ct.apexSpeed) / qMax(1.0, theoreticalMinSpeed) * 0.5);

            corners.append(ct);
            inCorner = false;
            brakeStart = -1;
        }
    }

    return corners;
}

double AITelemetryTrainer::calculateCorrelation(const QVector<float>& a, const QVector<float>& b) const
{
    int n = qMin(a.size(), b.size());
    if (n < 2) return 0;

    double sumA = 0, sumB = 0, sumAB = 0, sumA2 = 0, sumB2 = 0;
    for (int i = 0; i < n; ++i) {
        sumA += a[i];
        sumB += b[i];
        sumAB += a[i] * b[i];
        sumA2 += a[i] * a[i];
        sumB2 += b[i] * b[i];
    }

    double denom = std::sqrt((n * sumA2 - sumA * sumA) * (n * sumB2 - sumB * sumB));
    if (denom == 0) return 0;
    return (n * sumAB - sumA * sumB) / denom;
}

} // namespace ks
