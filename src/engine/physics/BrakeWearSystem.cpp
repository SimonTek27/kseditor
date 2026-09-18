#include "BrakeWearSystem.h"
#include <QJsonObject>
#include <QJsonArray>
#include <algorithm>
#include <cmath>

namespace ks::physics {

// ============================================================================
// BrakeWearSystem
// ============================================================================

BrakeWearSystem::BrakeWearSystem() {
    reset();
}

void BrakeWearSystem::setThermalConfig(const BrakeThermalConfig& config) {
    m_thermalConfig = config;
}

void BrakeWearSystem::setPadWearConfig(const BrakePadWearConfig& config) {
    m_padWearConfig = config;
}

void BrakeWearSystem::setAmbientTemp(float temp) {
    m_thermal.ambientTemp = temp;
}

void BrakeWearSystem::update(const BrakeWearData& data) {
    int w = data.wheel;
    if (w < 0 || w > 3) return;

    float torque = calculateBrakeTorque(data.brakePressure, w);

    updateThermal(w, torque, data.wheelSpeed, data.dt);
    updateFade(w, data.dt);
    updatePadWear(w, torque, data.wheelSpeed, data.dt);
    updateDiscWear(w, torque, data.wheelSpeed, data.dt);

    m_thermal.airflowSpeed = data.vehicleSpeed;
}

void BrakeWearSystem::applyBrake(int wheel, float pressure, float dt) {
    if (wheel < 0 || wheel > 3) return;

    float torque = calculateBrakeTorque(pressure, wheel);
    float angularVelocity = 50.0f; // Placeholder - should be actual wheel speed

    updateThermal(wheel, torque, angularVelocity, dt);
    updateFade(wheel, dt);
    updatePadWear(wheel, torque, angularVelocity, dt);
    updateDiscWear(wheel, torque, angularVelocity, dt);
}

void BrakeWearSystem::updateThermal(int wheel, float brakeTorque, float wheelSpeed, float dt) {
    // Heat generation from friction
    float heatGen = calculateHeatGeneration(brakeTorque, wheelSpeed);

    // Heat transfer: pad → disc
    float padToDisc = (m_thermal.padTemp[wheel] - m_thermal.discTemp[wheel]) *
                      m_thermalConfig.conductionPadToDisc * dt;

    // Update pad temperature
    float padHeatInput = heatGen - padToDisc;
    m_thermal.padTemp[wheel] += padHeatInput / (m_thermalConfig.padMass * m_thermalConfig.padHeatCapacity);

    // Update disc temperature
    float discCooling = calculateCooling(wheel, m_thermal.airflowSpeed);
    m_thermal.discTemp[wheel] += (padToDisc - discCooling) /
                                  (m_thermalConfig.discMass * m_thermalConfig.discHeatCapacity);

    // Hub temperature follows disc with delay
    float hubTarget = m_thermal.discTemp[wheel] * 0.3f + m_thermal.ambientTemp * 0.7f;
    m_thermal.hubTemp[wheel] += (hubTarget - m_thermal.hubTemp[wheel]) * 0.01f * dt;

    // Clamp temperatures
    m_thermal.padTemp[wheel] = std::clamp(m_thermal.padTemp[wheel],
                                           m_thermal.ambientTemp, 1000.0f);
    m_thermal.discTemp[wheel] = std::clamp(m_thermal.discTemp[wheel],
                                            m_thermal.ambientTemp, 1200.0f);
    m_thermal.hubTemp[wheel] = std::clamp(m_thermal.hubTemp[wheel],
                                           m_thermal.ambientTemp, 300.0f);
}

void BrakeWearSystem::updateFade(int wheel, float dt) {
    float discTemp = m_thermal.discTemp[wheel];
    BrakeFadeState& fadeState = m_fade[wheel];

    // Detect fade type and level
    float fadeOnsetTemp = 600.0f;
    float criticalDiscTemp = 800.0f;
    if (discTemp > fadeOnsetTemp) {
        float severity = (discTemp - fadeOnsetTemp) /
                         (criticalDiscTemp - fadeOnsetTemp);
        severity = std::clamp(severity, 0.0f, 1.0f);

        // Thermal fade
        if (discTemp > 600.0f) {
            fadeState.fadeType = BrakeFadeType::Thermal;
            fadeState.fadeLevel = std::min(1.0f, fadeState.fadeLevel + severity * 0.1f * dt);
        }
        // Gas fade (outgassing)
        else if (discTemp > 500.0f) {
            fadeState.fadeType = BrakeFadeType::Gas;
            fadeState.fadeLevel = std::min(1.0f, fadeState.fadeLevel + severity * 0.05f * dt);
        }
    } else {
        // Recovery when cooling
        fadeState.fadeLevel = std::max(0.0f, fadeState.fadeLevel - fadeState.recoveryRate * dt);
        if (fadeState.fadeLevel < 0.01f) {
            fadeState.fadeType = BrakeFadeType::None;
        }
    }

    // Update friction and feel
    float tempEffect = 1.0f;
    if (discTemp > 300.0f) {
        tempEffect = 1.0f + (discTemp - 300.0f) * m_thermalConfig.frictionTempSensitivity;
        tempEffect = std::clamp(tempEffect, 0.8f, 1.1f);
    }
    fadeState.peakFriction = m_thermalConfig.frictionCoeffBase * tempEffect;
    fadeState.pedalFirmness = 1.0f - fadeState.fadeLevel * 0.4f;
    fadeState.responseTime = fadeState.fadeLevel * 0.05f;
}

void BrakeWearSystem::updatePadWear(int wheel, float brakeTorque, float wheelSpeed, float dt) {
    if (m_padWear.padThickness[wheel] <= m_padWearConfig.minimumThickness) return;

    // Base wear from torque and speed
    float normalizedTorque = brakeTorque / 1000.0f;
    float normalizedSpeed = wheelSpeed / 100.0f;

    float wearRate = m_padWearConfig.baseWearRate *
                     std::pow(normalizedTorque, m_padWearConfig.pressureExponent) *
                     std::pow(normalizedSpeed, m_padWearConfig.velocityExponent);

    // Temperature effect on wear
    float temp = m_thermal.padTemp[wheel];
    float optimalTemp = 350.0f;
    float tempDeviation = std::abs(temp - optimalTemp) / 200.0f;
    wearRate *= std::pow(1.0f + tempDeviation, m_padWearConfig.temperatureExponent);

    // Material factor
    wearRate /= m_padWearConfig.padMaterialFactor;

    // Apply wear
    float wearAmount = wearRate * dt;
    m_padWear.padThickness[wheel] = std::max(0.0f, m_padWear.padThickness[wheel] - wearAmount);
    m_padWear.totalWear[wheel] += wearAmount;
    m_padWear.wearRate[wheel] = wearRate;
    m_padWear.needsReplacement[wheel] = m_padWear.padThickness[wheel] < m_padWearConfig.warningThickness;
}

void BrakeWearSystem::updateDiscWear(int wheel, float brakeTorque, float wheelSpeed, float dt) {
    if (m_discWear.discThickness[wheel] <= 15.0f) return;

    // Disc wears from heat and friction
    float heatWear = 0.0f;
    if (m_thermal.discTemp[wheel] > 500.0f) {
        heatWear = (m_thermal.discTemp[wheel] - 500.0f) * 0.00001f;
    }

    // Friction wear from pad contact
    float frictionWear = brakeTorque * wheelSpeed * 0.0000001f;

    float totalWear = (heatWear + frictionWear) * dt;
    m_discWear.discThickness[wheel] = std::max(15.0f, m_discWear.discThickness[wheel] - totalWear);

    // Surface roughness from heat cycling
    if (m_thermal.discTemp[wheel] > 600.0f) {
        m_discWear.surfaceRoughness[wheel] = std::min(1.0f,
            m_discWear.surfaceRoughness[wheel] + 0.001f * dt);
    } else {
        m_discWear.surfaceRoughness[wheel] = std::max(0.0f,
            m_discWear.surfaceRoughness[wheel] - 0.0005f * dt);
    }

    // Warpage from uneven heating
    float tempGradient = std::abs(m_thermal.discTemp[wheel] - m_thermal.hubTemp[wheel]);
    if (tempGradient > 200.0f) {
        m_discWear.warpage[wheel] = std::min(1.0f,
            m_discWear.warpage[wheel] + 0.0005f * dt);
    }

    m_discWear.isWarped[wheel] = m_discWear.warpage[wheel] > 0.5f;
}

float BrakeWearSystem::calculateBrakeTorque(float pressure, int wheel) const {
    // T = P * A * μ * r
    float area = m_thermalConfig.discArea;
    float friction = effectiveFriction(wheel);
    float radius = 0.15f; // Effective brake radius (m)

    return pressure * 100000.0f * area * friction * radius; // Convert bar to Pa
}

float BrakeWearSystem::calculateHeatGeneration(float torque, float angularVelocity) const {
    // Q = T * ω (friction power)
    return std::abs(torque * angularVelocity) * 0.9f; // 90% becomes heat
}

float BrakeWearSystem::calculateCooling(int wheel, float speed) const {
    float coolingCoeff = (wheel < 2) ? m_thermalConfig.coolingCoeffFront :
                                          m_thermalConfig.coolingCoeffRear;

    // Forced convection (increases with speed)
    float convection = coolingCoeff * (1.0f + speed * 0.03f);
    float area = m_thermalConfig.discArea * 2.0f; // Both sides of disc
    float deltaT = m_thermal.discTemp[wheel] - m_thermal.ambientTemp;

    return convection * area * deltaT;
}

float BrakeWearSystem::effectiveFriction(int wheel) const {
    float base = m_thermalConfig.frictionCoeffBase;
    float fadeEffect = m_fade[wheel].effectiveFriction(base);
    float roughnessEffect = 1.0f - m_discWear.surfaceRoughness[wheel] * 0.1f;
    return fadeEffect * roughnessEffect;
}

float BrakeWearSystem::brakingMultiplier(int wheel) const {
    float fadeEffect = m_fade[wheel].brakingMultiplier();
    float padEffect = m_padWear.remainingLife(wheel);
    float discEffect = m_discWear.remainingLife(wheel);
    return std::clamp(fadeEffect * padEffect * discEffect, 0.1f, 1.0f);
}

void BrakeWearSystem::reset() {
    m_thermal = BrakeThermalState{};
    m_fade.fill(BrakeFadeState{});
    m_padWear = BrakePadWearState{};
    m_discWear = BrakeDiscWearState{};
    m_braking[0] = m_braking[1] = m_braking[2] = m_braking[3] = false;
}

QJsonObject BrakeWearSystem::toJson() const {
    QJsonObject obj;

    QJsonArray discTemps;
    for (int i = 0; i < 4; ++i) discTemps.append(m_thermal.discTemp[i]);
    obj["discTemperatures"] = discTemps;

    QJsonArray padTemps;
    for (int i = 0; i < 4; ++i) padTemps.append(m_thermal.padTemp[i]);
    obj["padTemperatures"] = padTemps;

    QJsonArray padWear;
    for (int i = 0; i < 4; ++i) padWear.append(m_padWear.padThickness[i]);
    obj["padThickness"] = padWear;

    QJsonArray fadeLevels;
    for (int i = 0; i < 4; ++i) fadeLevels.append(m_fade[i].fadeLevel);
    obj["fadeLevels"] = fadeLevels;

    return obj;
}

void BrakeWearSystem::fromJson(const QJsonObject& obj) {
    QJsonArray discTemps = obj["discTemperatures"].toArray();
    for (int i = 0; i < 4 && i < discTemps.size(); ++i) {
        m_thermal.discTemp[i] = discTemps[i].toDouble(300.0);
    }

    QJsonArray padTemps = obj["padTemperatures"].toArray();
    for (int i = 0; i < 4 && i < padTemps.size(); ++i) {
        m_thermal.padTemp[i] = padTemps[i].toDouble(250.0);
    }

    QJsonArray padWear = obj["padThickness"].toArray();
    for (int i = 0; i < 4 && i < padWear.size(); ++i) {
        m_padWear.padThickness[i] = padWear[i].toDouble(10.0);
    }
}

} // namespace ks::physics
