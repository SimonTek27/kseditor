#pragma once

#include <QVector>
#include <QString>

namespace ks::physics {

struct TelemetryPoint { float timestamp = 0, speed = 0, rpm = 0; int gear = 1; float throttle = 0, brake = 0, steering = 0, lateralG = 0, longitudinalG = 0; float tyreTemp[4] = {30}, tyrePressure[4] = {2.2f}; float fuel = 100, lapDistance = 0; };

struct TelemetryAnalysis { float maxSpeed = 0, avgSpeed = 0, minCornerSpeed = 0, maxLateralG = 0, maxLongitudinalG = 0, maxBrakingG = 0; float avgThrottle = 0, avgBrake = 0, coastTime = 0, throttleTime = 0, brakeTime = 0; int gearChanges = 0; float fuelConsumption = 0, topSpeedDistance = 0; QString fastestSector, slowestSector; };

struct LapComparison { float timeDifference = 0, speedDifference = 0, throttleDifference = 0, brakeDifference = 0, lateralGDifference = 0; QString fasterLap, slowerLap; QVector<float> timeDelta; };

struct DriverPerformance { float consistency = 0, aggression = 0, smoothness = 0, brakingPerformance = 0, corneringPerformance = 0, throttleControl = 0, rating = 0; };

class TelemetryAnalysisModel {
public:
    TelemetryAnalysis analyzeLap(const QVector<TelemetryPoint>& data) const;
    LapComparison compareLaps(const QVector<TelemetryPoint>& lap1, const QVector<TelemetryPoint>& lap2) const;
    DriverPerformance evaluateDriver(const QVector<QVector<TelemetryPoint>>& laps) const;
    float calculateConsistency(const QVector<float>& lapTimes) const;
    float calculateSmoothness(const QVector<TelemetryPoint>& data) const;
    float calculateAggression(const QVector<TelemetryPoint>& data) const;
    QVector<float> calculateSpeedTrace(const QVector<TelemetryPoint>& data) const;
    QVector<float> calculateThrottleTrace(const QVector<TelemetryPoint>& data) const;
    QVector<float> calculateBrakeTrace(const QVector<TelemetryPoint>& data) const;
};

} // namespace ks::physics
