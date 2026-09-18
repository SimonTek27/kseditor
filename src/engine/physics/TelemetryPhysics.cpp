#include "TelemetryPhysics.h"
#include <cmath>
#include <algorithm>

namespace ks::physics {

TelemetryAnalysis TelemetryAnalysisModel::analyzeLap(const QVector<TelemetryPoint>& lapData) const {
    TelemetryAnalysis analysis;

    if (lapData.isEmpty()) return analysis;

    float totalSpeed = 0.0f;
    float totalThrottle = 0.0f;
    float totalBrake = 0.0f;
    float totalTime = 0.0f;
    float throttleTime = 0.0f;
    float brakeTime = 0.0f;
    float coastTime = 0.0f;

    analysis.maxSpeed = 0.0f;
    analysis.maxLateralG = 0.0f;
    analysis.maxLongitudinalG = 0.0f;
    analysis.maxBrakingG = 0.0f;
    analysis.minCornerSpeed = 1000.0f;

    for (const auto& point : lapData) {
        totalSpeed += point.speed;
        totalThrottle += point.throttle;
        totalBrake += point.brake;

        if (point.speed > analysis.maxSpeed) {
            analysis.maxSpeed = point.speed;
            analysis.topSpeedDistance = point.lapDistance;
        }

        float latG = std::abs(point.lateralG);
        if (latG > analysis.maxLateralG) {
            analysis.maxLateralG = latG;
        }

        float longG = point.longitudinalG;
        if (longG > analysis.maxLongitudinalG) {
            analysis.maxLongitudinalG = longG;
        }
        if (longG < analysis.maxBrakingG) {
            analysis.maxBrakingG = longG;
        }

        if (latG > 0.5f && point.speed < analysis.minCornerSpeed) {
            analysis.minCornerSpeed = point.speed;
        }

        if (point.throttle > 0.1f) {
            throttleTime += 0.016f;
        } else if (point.brake > 0.1f) {
            brakeTime += 0.016f;
        } else {
            coastTime += 0.016f;
        }

        totalTime += 0.016f;
    }

    int n = lapData.size();
    analysis.avgSpeed = totalSpeed / n;
    analysis.avgThrottle = totalThrottle / n;
    analysis.avgBrake = totalBrake / n;
    analysis.coastTime = coastTime;
    analysis.throttleTime = throttleTime;
    analysis.brakeTime = brakeTime;

    return analysis;
}

LapComparison TelemetryAnalysisModel::compareLaps(
    const QVector<TelemetryPoint>& lap1,
    const QVector<TelemetryPoint>& lap2
) const {
    LapComparison comparison;

    if (lap1.isEmpty() || lap2.isEmpty()) return comparison;

    float time1 = lap1.last().timestamp - lap1.first().timestamp;
    float time2 = lap2.last().timestamp - lap2.first().timestamp;

    comparison.timeDifference = time1 - time2;
    comparison.fasterLap = (time1 < time2) ? "Lap 1" : "Lap 2";
    comparison.slowerLap = (time1 < time2) ? "Lap 2" : "Lap 1";

    float speedDiff = 0.0f;
    float throttleDiff = 0.0f;
    float brakeDiff = 0.0f;
    float latGDiff = 0.0f;

    int minSize = std::min(lap1.size(), lap2.size());
    for (int i = 0; i < minSize; ++i) {
        speedDiff += lap1[i].speed - lap2[i].speed;
        throttleDiff += lap1[i].throttle - lap2[i].throttle;
        brakeDiff += lap1[i].brake - lap2[i].brake;
        latGDiff += std::abs(lap1[i].lateralG) - std::abs(lap2[i].lateralG);
    }

    comparison.speedDifference = speedDiff / minSize;
    comparison.throttleDifference = throttleDiff / minSize;
    comparison.brakeDifference = brakeDiff / minSize;
    comparison.lateralGDifference = latGDiff / minSize;

    comparison.timeDelta.resize(minSize);
    for (int i = 0; i < minSize; ++i) {
        float t1 = lap1[i].timestamp - lap1.first().timestamp;
        float t2 = lap2[i].timestamp - lap2.first().timestamp;
        comparison.timeDelta[i] = t1 - t2;
    }

    return comparison;
}

DriverPerformance TelemetryAnalysisModel::evaluateDriver(
    const QVector<QVector<TelemetryPoint>>& laps
) const {
    DriverPerformance performance;

    if (laps.isEmpty()) return performance;

    QVector<float> lapTimes;
    for (const auto& lap : laps) {
        if (!lap.isEmpty()) {
            lapTimes.append(lap.last().timestamp - lap.first().timestamp);
        }
    }

    performance.consistency = calculateConsistency(lapTimes);

    if (!laps.isEmpty()) {
        performance.smoothness = calculateSmoothness(laps.first());
        performance.aggression = calculateAggression(laps.first());
    }

    performance.brakingPerformance = performance.smoothness * 0.8f + performance.aggression * 0.2f;
    performance.corneringPerformance = performance.smoothness * 0.7f + performance.aggression * 0.3f;
    performance.throttleControl = performance.smoothness * 0.9f;

    performance.rating = (
        performance.consistency * 25.0f +
        performance.smoothness * 25.0f +
        performance.aggression * 25.0f +
        performance.brakingPerformance * 25.0f
    );

    return performance;
}

float TelemetryAnalysisModel::calculateConsistency(const QVector<float>& lapTimes) const {
    if (lapTimes.size() < 2) return 1.0f;

    float mean = 0.0f;
    for (float time : lapTimes) {
        mean += time;
    }
    mean /= lapTimes.size();

    float variance = 0.0f;
    for (float time : lapTimes) {
        variance += (time - mean) * (time - mean);
    }
    variance /= lapTimes.size();

    float stdDev = std::sqrt(variance);
    float consistency = 1.0f - (stdDev / mean);
    return std::clamp(consistency, 0.0f, 1.0f);
}

float TelemetryAnalysisModel::calculateSmoothness(const QVector<TelemetryPoint>& lapData) const {
    if (lapData.size() < 2) return 0.5f;

    float steeringSmoothness = 0.0f;
    float throttleSmoothness = 0.0f;
    float brakeSmoothness = 0.0f;

    for (int i = 1; i < lapData.size(); ++i) {
        float dSteering = std::abs(lapData[i].steering - lapData[i-1].steering);
        float dThrottle = std::abs(lapData[i].throttle - lapData[i-1].throttle);
        float dBrake = std::abs(lapData[i].brake - lapData[i-1].brake);

        steeringSmoothness += dSteering;
        throttleSmoothness += dThrottle;
        brakeSmoothness += dBrake;
    }

    int n = lapData.size() - 1;
    steeringSmoothness /= n;
    throttleSmoothness /= n;
    brakeSmoothness /= n;

    float totalSmoothness = 1.0f - (steeringSmoothness + throttleSmoothness + brakeSmoothness) / 3.0f;
    return std::clamp(totalSmoothness, 0.0f, 1.0f);
}

float TelemetryAnalysisModel::calculateAggression(const QVector<TelemetryPoint>& lapData) const {
    if (lapData.isEmpty()) return 0.5f;

    float maxLatG = 0.0f;
    float maxBrake = 0.0f;
    float avgThrottle = 0.0f;

    for (const auto& point : lapData) {
        maxLatG = std::max(maxLatG, std::abs(point.lateralG));
        maxBrake = std::max(maxBrake, point.brake);
        avgThrottle += point.throttle;
    }

    avgThrottle /= lapData.size();

    float aggression = (maxLatG * 0.4f + maxBrake * 0.3f + avgThrottle * 0.3f);
    return std::clamp(aggression, 0.0f, 1.0f);
}

QVector<float> TelemetryAnalysisModel::calculateSpeedTrace(
    const QVector<TelemetryPoint>& lapData
) const {
    QVector<float> trace;
    trace.reserve(lapData.size());

    for (const auto& point : lapData) {
        trace.append(point.speed);
    }

    return trace;
}

QVector<float> TelemetryAnalysisModel::calculateThrottleTrace(
    const QVector<TelemetryPoint>& lapData
) const {
    QVector<float> trace;
    trace.reserve(lapData.size());

    for (const auto& point : lapData) {
        trace.append(point.throttle);
    }

    return trace;
}

QVector<float> TelemetryAnalysisModel::calculateBrakeTrace(
    const QVector<TelemetryPoint>& lapData
) const {
    QVector<float> trace;
    trace.reserve(lapData.size());

    for (const auto& point : lapData) {
        trace.append(point.brake);
    }

    return trace;
}

namespace {

float interpolateTelemetryValue(
    const QVector<TelemetryPoint>& data, float timestamp,
    float TelemetryPoint::*member
) {
    if (data.isEmpty()) return 0.0f;

    if (timestamp <= data.first().timestamp) return data.first().*member;
    if (timestamp >= data.last().timestamp) return data.last().*member;

    for (int i = 1; i < data.size(); ++i) {
        if (data[i].timestamp >= timestamp) {
            float t = (timestamp - data[i-1].timestamp) /
                     (data[i].timestamp - data[i-1].timestamp);
            return data[i-1].*member + t * (data[i].*member - data[i-1].*member);
        }
    }

    return data.last().*member;
}

} // anonymous namespace

} // namespace ks::physics
