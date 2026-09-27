#include "TireWearSystem.h"
#include <algorithm>
#include <cmath>

namespace ks {
namespace physics {

TireCompoundData TireCompoundData::getCompoundData(TireCompound compound) {
    TireCompoundData data;
    data.compound = compound;
    switch (compound) {
    case TireCompound::SuperSoft:
        data.name = "SuperSoft"; data.gripFactor = 1.15f; data.wearResistance = 0.5f;
        data.optimalTempMin = 65.f; data.optimalTempMax = 95.f; break;
    case TireCompound::Soft:
        data.name = "Soft"; data.gripFactor = 1.08f; data.wearResistance = 0.7f;
        data.optimalTempMin = 70.f; data.optimalTempMax = 100.f; break;
    case TireCompound::Medium:
        data.name = "Medium"; data.gripFactor = 1.0f; data.wearResistance = 1.0f;
        data.optimalTempMin = 75.f; data.optimalTempMax = 105.f; break;
    case TireCompound::Hard:
        data.name = "Hard"; data.gripFactor = 0.92f; data.wearResistance = 1.4f;
        data.optimalTempMin = 80.f; data.optimalTempMax = 110.f; break;
    case TireCompound::Intermediate:
        data.name = "Intermediate"; data.gripFactor = 0.85f; data.wearResistance = 1.1f;
        data.optimalTempMin = 40.f; data.optimalTempMax = 80.f; break;
    case TireCompound::Wet:
        data.name = "Wet"; data.gripFactor = 0.75f; data.wearResistance = 1.2f;
        data.optimalTempMin = 20.f; data.optimalTempMax = 50.f; break;
    }
    return data;
}

void TireWearSystem::update(float speed, float normalLoad, float slipAngle, float slipRatio,
                            float lateralForce, float longitudinalForce, float dt) {
    dt = std::clamp(dt, 1e-4f, 0.05f);
    m_distanceTraveled += std::abs(speed) * dt;
    updateThermal(speed, normalLoad, slipAngle, slipRatio, lateralForce, longitudinalForce, dt);
    updateWear(normalLoad, slipAngle, slipRatio, lateralForce, longitudinalForce, dt);
}

void TireWearSystem::updateThermal(float speed, float, float slipAngle, float slipRatio,
                                   float latF, float lonF, float dt) {
    float heat = (std::abs(latF) * std::abs(slipAngle) + std::abs(lonF) * std::abs(slipRatio))
                 * m_thermalConfig.frictionHeatFactor;
    float cool = m_thermalConfig.coolingCoeff * (1.0f + 0.04f * std::max(speed, 0.0f))
                 * (m_thermal.surfaceTemp - m_thermal.ambientTemp);
    float dT = (heat - cool) * dt / std::max(m_thermalConfig.heatCapacity, 1.0f);
    m_thermal.surfaceTemp = std::clamp(m_thermal.surfaceTemp + dT, m_thermal.ambientTemp, 150.0f);
    m_thermal.carcassTemp = 0.9f * m_thermal.carcassTemp + 0.1f * m_thermal.surfaceTemp;
}

void TireWearSystem::updateWear(float normalLoad, float slipAngle, float slipRatio,
                                float, float, float dt) {
    float slip = std::abs(slipAngle) + std::abs(slipRatio);
    float loadF = std::max(normalLoad, 0.0f) / 4000.0f;
    float tempF = 1.0f;
    if (m_thermal.surfaceTemp > m_compound.optimalTempMax)
        tempF += (m_thermal.surfaceTemp - m_compound.optimalTempMax) * 0.02f * m_wearConfig.tempWearFactor;
    float rate = m_wearConfig.baseWearRate * loadF * (1.0f + slip * m_wearConfig.slipWearFactor)
                 * tempF / std::max(m_compound.wearResistance, 0.1f);
    m_wearDetail.wear = std::clamp(m_wearDetail.wear + rate * dt, 0.0f, 1.0f);
    if (m_thermal.surfaceTemp < m_compound.optimalTempMin - 10.f && slip > 0.05f)
        m_wearDetail.graining = std::clamp(m_wearDetail.graining + 0.01f * dt, 0.0f, 1.0f);
    if (m_thermal.surfaceTemp > m_compound.optimalTempMax + 20.f)
        m_wearDetail.blistering = std::clamp(m_wearDetail.blistering + 0.02f * dt, 0.0f, 1.0f);
}

float TireWearSystem::estimatedLapsRemaining() const {
    float wearPerLap = m_wearDetail.wear / std::max(1.0f, m_distanceTraveled / 5000.0f);
    if (wearPerLap <= 1e-6f) return 999.0f;
    return (1.0f - m_wearDetail.wear) / wearPerLap;
}

void TireWearSystem::reset() {
    m_thermal = TireThermalState{};
    m_wearDetail = TireWearDetailState{};
    m_distanceTraveled = 0.0f;
}

} // namespace physics
} // namespace ks
