#pragma once

#include "PhysicsCoreTypes.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace ks::physics {

struct BrakeThermalState {
    float discTemp[4] = {300.0f, 300.0f, 300.0f, 300.0f};
    float padTemp[4] = {250.0f, 250.0f, 250.0f, 250.0f};
    float hubTemp[4] = {80.0f, 80.0f, 80.0f, 80.0f};
    float ambientTemp = 25.0f;
    float airflowSpeed = 0.0f;
    float optimalDiscTempMin = 250.0f;
    float optimalDiscTempMax = 500.0f;
    float criticalDiscTemp = 800.0f;
    float fadeOnsetTemp = 600.0f;
};

struct BrakeThermalConfig {
    float discMass = 4.0f;
    float padMass = 0.5f;
    float discHeatCapacity = 500.0f;
    float padHeatCapacity = 900.0f;
    float frictionCoeffBase = 0.42f;
    float frictionTempSensitivity = 0.0005f;
    float coolingCoeffFront = 45.0f;
    float coolingCoeffRear = 35.0f;
    float discArea = 0.03f;
    float conductionPadToDisc = 0.6f;
};

enum class BrakeFadeType { None, Thermal, Friction, Gas, Mechanical };

struct BrakeFadeState {
    BrakeFadeType fadeType = BrakeFadeType::None;
    float fadeLevel = 0.0f;
    float peakFriction = 0.42f;
    float pedalFirmness = 1.0f;
    float responseTime = 0.0f;
    float recoveryRate = 0.1f;
    float effectiveFriction(float baseFriction) const {
        return baseFriction * (1.0f - fadeLevel * 0.85f);
    }
    float brakingMultiplier() const { return std::clamp(1.0f - fadeLevel, 0.1f, 1.0f); }
};

struct BrakePadWearState {
    float padThickness[4] = {10.0f, 10.0f, 10.0f, 10.0f};
    float initialThickness = 10.0f;
    float wearRate[4] = {0.0f};
    float totalWear[4] = {0.0f};
    bool needsReplacement[4] = {false};
    float remainingLife(int wheel) const {
        return std::clamp(padThickness[wheel] / initialThickness, 0.0f, 1.0f);
    }
    bool isWornOut(int wheel) const { return padThickness[wheel] < 1.0f; }
};

struct BrakePadWearConfig {
    float baseWearRate = 0.001f;
    float pressureExponent = 1.1f;
    float temperatureExponent = 1.3f;
    float velocityExponent = 0.9f;
    float minimumThickness = 1.0f;
    float warningThickness = 3.0f;
};

struct BrakeDiscWearState {
    float discThickness[4] = {28.0f, 28.0f, 28.0f, 28.0f};
    float initialThickness = 28.0f;
    float surfaceRoughness[4] = {0.0f};
    float warpage[4] = {0.0f};
    bool isWarped[4] = {false};
    float remainingLife(int wheel) const {
        return std::clamp(discThickness[wheel] / initialThickness, 0.0f, 1.0f);
    }
};

struct BrakeWearData {
    int wheel = 0;
    float brakeTorque = 0.0f;
    float brakePressure = 0.0f;
    float wheelSpeed = 0.0f;
    float vehicleSpeed = 0.0f;
    float normalLoad = 0.0f;
    float dt = 0.016f;
};

class BrakeWearSystem {
public:
    BrakeWearSystem();

    void setThermalConfig(const BrakeThermalConfig& config);
    void setPadWearConfig(const BrakePadWearConfig& config);
    void setAmbientTemp(float temp);

    void update(const BrakeWearData& data);
    void applyBrake(int wheel, float pressure, float dt);

    const BrakeThermalState& thermalState() const { return m_thermal; }
    const BrakeFadeState& fadeState(int wheel) const { return m_fade[wheel]; }
    const BrakePadWearState& padWearState() const { return m_padWear; }
    const BrakeDiscWearState& discWearState() const { return m_discWear; }

    float discTemperature(int wheel) const { return m_thermal.discTemp[wheel]; }
    float padTemperature(int wheel) const { return m_thermal.padTemp[wheel]; }
    float effectiveFriction(int wheel) const;
    float brakingMultiplier(int wheel) const;
    bool isFaded(int wheel) const { return m_fade[wheel].fadeLevel > 0.1f; }
    bool needsPadReplacement(int wheel) const { return m_padWear.needsReplacement[wheel]; }

    void reset();

private:
    void updateThermal(int wheel, float brakeTorque, float wheelSpeed, float dt);
    void updateFade(int wheel, float dt);
    void updatePadWear(int wheel, float brakeTorque, float wheelSpeed, float dt);
    void updateDiscWear(int wheel, float brakeTorque, float wheelSpeed, float dt);
    float calculateBrakeTorque(float pressure, int wheel) const;
    float calculateHeatGeneration(float torque, float angularVelocity) const;
    float calculateCooling(int wheel, float speed) const;

    BrakeThermalConfig m_thermalConfig;
    BrakePadWearConfig m_padWearConfig;
    BrakeThermalState m_thermal;
    std::array<BrakeFadeState, 4> m_fade;
    BrakePadWearState m_padWear;
    BrakeDiscWearState m_discWear;
    bool m_braking[4] = {false};
};

} // namespace ks::physics
