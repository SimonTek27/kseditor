#include "LapTimeImpactCalculator.h"
#include <cmath>
#include <algorithm>

namespace ks {

LapTimeImpactCalculator::LapTimeImpactCalculator(QObject* parent)
    : QObject(parent) {}

LapTimeImpactCalculator::~LapTimeImpactCalculator() {}

LapTimeImpactCalculator::ImpactReport LapTimeImpactCalculator::estimateLapTimeImpact(
    const SetupParameter& oldSetup, const SetupParameter& newSetup,
    const CarStats& car, float trackLengthKm, int numCorners, float avgCornerSpeedKmh) {

    ImpactReport report;

    float suspGain = estimateSuspensionImpact(oldSetup, newSetup, car, numCorners);
    float aeroGain = estimateAeroImpact(oldSetup, newSetup, car, avgCornerSpeedKmh);
    float camberGain = estimateCamberImpact(oldSetup, newSetup, numCorners);
    float arbGain = estimateARBImpact(oldSetup, newSetup, numCorners);
    float brakeGain = estimateBrakeBiasImpact(oldSetup, newSetup, numCorners);

    auto addDelta = [&](const QString& name, float oldVal, float newVal, float gain, float conf) {
        SetupDelta d;
        d.parameter = name;
        d.oldValue = oldVal;
        d.newValue = newVal;
        d.estimatedGainSeconds = gain;
        d.confidence = conf;
        report.deltas.append(d);

        if (gain > 0.05f) {
            report.recommendedChanges.append(
                QString("%1: %2 -> %3 (gain ~%4s)")
                    .arg(name)
                    .arg(oldVal, 0, 'f', 1)
                    .arg(newVal, 0, 'f', 1)
                    .arg(gain, 0, 'f', 3));
        }
    };

    addDelta("Suspension", oldSetup.frontSpring, newSetup.frontSpring, suspGain, 0.65f);
    addDelta("Aero", oldSetup.frontWing + oldSetup.rearWing, newSetup.frontWing + newSetup.rearWing, aeroGain, 0.70f);
    addDelta("Camber", oldSetup.frontCamber, newSetup.frontCamber, camberGain, 0.55f);
    addDelta("ARB", oldSetup.frontARB + oldSetup.rearARB, newSetup.frontARB + newSetup.rearARB, arbGain, 0.50f);
    addDelta("Brake Bias", oldSetup.brakeBias, newSetup.brakeBias, brakeGain, 0.45f);

    report.totalEstimatedGain = suspGain + aeroGain + camberGain + arbGain + brakeGain;
    std::sort(report.deltas.begin(), report.deltas.end(),
              [](const SetupDelta& a, const SetupDelta& b) {
                  return a.estimatedGainSeconds > b.estimatedGainSeconds;
              });

    emit impactCalculated(report);
    return report;
}

float LapTimeImpactCalculator::estimateSuspensionImpact(
    const SetupParameter& oldSetup, const SetupParameter& newSetup,
    const CarStats& car, int numCorners) {

    float oldAvg = (oldSetup.frontSpring + oldSetup.rearSpring) / 2.0f;
    float newAvg = (newSetup.frontSpring + newSetup.rearSpring) / 2.0f;
    float springDelta = newAvg - oldAvg;

    float optimalSpring = car.tireGrip * 100.0f;
    float oldDev = std::abs(oldAvg - optimalSpring);
    float newDev = std::abs(newAvg - optimalSpring);

    float improvement = (oldDev - newDev) / std::max(1.0f, optimalSpring);
    float cornerTime = numCorners * 2.0f;
    return improvement * cornerTime * 0.1f;
}

float LapTimeImpactCalculator::estimateAeroImpact(
    const SetupParameter& oldSetup, const SetupParameter& newSetup,
    const CarStats& car, float avgCornerSpeedKmh) {

    float oldTotal = oldSetup.frontWing + oldSetup.rearWing;
    float newTotal = newSetup.frontWing + newSetup.rearWing;

    float speedMs = avgCornerSpeedKmh / 3.6f;
    float oldDownforce = car.downforceCoeff * oldTotal * speedMs * speedMs * 0.5f;
    float newDownforce = car.downforceCoeff * newTotal * speedMs * speedMs * 0.5f;

    float oldDrag = car.dragCoeff * oldTotal * 0.3f;
    float newDrag = car.dragCoeff * newTotal * 0.3f;

    float gripGain = (newDownforce - oldDownforce) / car.weightKg / 9.81f;
    float dragLoss = (newDrag - oldDrag) * speedMs * speedMs * 0.5f * car.frontalAreaM2;
    float dragTimePenalty = dragLoss / car.powerKw * 0.5f;

    return std::max(0.0f, gripGain * 0.2f - dragTimePenalty);
}

float LapTimeImpactCalculator::estimateCamberImpact(
    const SetupParameter& oldSetup, const SetupParameter& newSetup,
    int numCorners) {

    float oldCamber = (oldSetup.frontCamber + oldSetup.rearCamber) / 2.0f;
    float newCamber = (newSetup.frontCamber + newSetup.rearCamber) / 2.0f;

    // Optimal camber around -2.5 deg for most cars
    float oldDev = std::abs(oldCamber + 2.5f);
    float newDev = std::abs(newCamber + 2.5f);

    float improvement = (oldDev - newDev) / 2.5f;
    return improvement * numCorners * 0.05f;
}

float LapTimeImpactCalculator::estimateARBImpact(
    const SetupParameter& oldSetup, const SetupParameter& newSetup,
    int numCorners) {

    float oldAvg = (oldSetup.frontARB + oldSetup.rearARB) / 2.0f;
    float newAvg = (newSetup.frontARB + newSetup.rearARB) / 2.0f;

    float ratio = newAvg / std::max(0.1f, oldAvg);
    float optimalRatio = 1.0f;
    float improvement = std::abs(ratio - optimalRatio) * 0.5f;

    return improvement * numCorners * 0.02f;
}

float LapTimeImpactCalculator::estimateBrakeBiasImpact(
    const SetupParameter& oldSetup, const SetupParameter& newSetup,
    int numCorners) {

    float oldDev = std::abs(oldSetup.brakeBias - 0.60f);
    float newDev = std::abs(newSetup.brakeBias - 0.60f);

    float improvement = (oldDev - newDev) / 0.2f;
    return improvement * numCorners * 0.03f;
}

} // namespace ks