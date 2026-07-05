#include "TireTemperaturePredictor.h"
#include <cmath>
#include <algorithm>

namespace ks {

TireTemperaturePredictor::TireTemperaturePredictor(QObject* parent)
    : QObject(parent) {}

TireTemperaturePredictor::~TireTemperaturePredictor() {}

TireTemperaturePredictor::TirePrediction TireTemperaturePredictor::predictTireTemps(
    const TireState& current, const TrackConditions& track) {

    TirePrediction pred;
    float heatGen = calculateHeatGeneration(track);

    float avgSpeed = track.avgCornerSpeed * 3.6f;
    float cooling = calculateCoolingRate(current.middleTemp, avgSpeed, track);

    float tempDelta = heatGen - cooling;

    pred.predictedState.innerTemp = current.innerTemp + tempDelta * 0.3f;
    pred.predictedState.middleTemp = current.middleTemp + tempDelta * 0.4f;
    pred.predictedState.outerTemp = current.outerTemp + tempDelta * 0.3f;

    float pressureDelta = calculatePressureBuild(tempDelta);
    pred.predictedState.pressure = current.pressure + pressureDelta;
    pred.predictedState.wear = current.wear + estimateWearRate(current.pressure, current.middleTemp) * 0.01f;

    pred.optimalPressure = calculateOptimalPressure(track);
    pred.pressureDelta = pred.predictedState.pressure - pred.optimalPressure;

    float optimalTemp = 85.0f + (track.trackTemp - 20.0f) * 0.5f;
    float maxDev = std::max({std::abs(pred.predictedState.innerTemp - optimalTemp),
                             std::abs(pred.predictedState.middleTemp - optimalTemp),
                             std::abs(pred.predictedState.outerTemp - optimalTemp)});
    pred.tempRangeOk = std::max(0.0f, 1.0f - maxDev / 30.0f);

    float maxTemp = std::max({pred.predictedState.innerTemp, pred.predictedState.middleTemp, pred.predictedState.outerTemp});
    pred.overheatingRisk = std::max(0.0f, std::min(1.0f, (maxTemp - 105.0f) / 20.0f));

    float minTemp = std::min({pred.predictedState.innerTemp, pred.predictedState.middleTemp, pred.predictedState.outerTemp});
    pred.underTempRisk = std::max(0.0f, std::min(1.0f, (70.0f - minTemp) / 15.0f));

    emit predictionReady(pred);
    return pred;
}

float TireTemperaturePredictor::calculateOptimalPressure(const TrackConditions& track) {
    float basePressure = 2.1f;
    float tempAdjust = (track.trackTemp - 20.0f) * 0.015f;
    float cornerAdjust = track.numCorners > 12 ? 0.1f : -0.05f;
    return basePressure + tempAdjust + cornerAdjust;
}

float TireTemperaturePredictor::estimateWearRate(float pressure, float temp) {
    float optimalPressure = 2.1f;
    float pressureDev = std::abs(pressure - optimalPressure);
    float pressureWear = pressureDev * 0.5f;

    float optimalTemp = 85.0f;
    float tempDev = std::abs(temp - optimalTemp);
    float tempWear = tempDev * 0.02f;

    return pressureWear + tempWear;
}

float TireTemperaturePredictor::calculateHeatGeneration(const TrackConditions& track) {
    float cornerHeat = track.numCorners * track.avgCornerSpeed * 0.05f;
    float trackTempFactor = 1.0f + (track.trackTemp - 20.0f) * 0.01f;
    return cornerHeat * trackTempFactor;
}

float TireTemperaturePredictor::calculateCoolingRate(float temp, float speed, const TrackConditions& track) {
    float ambientDelta = temp - track.ambientTemp;
    float speedCooling = speed * 0.001f;
    return ambientDelta * speedCooling * 0.1f;
}

float TireTemperaturePredictor::calculatePressureBuild(float tempDelta) {
    return tempDelta * 0.003f;
}

} // namespace ks