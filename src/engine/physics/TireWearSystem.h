#pragma once

#include "PhysicsCoreTypes.h"
#include <string>
#include <algorithm>
#include <cmath>

namespace ks::physics {

enum class TireCompound { SuperSoft, Soft, Medium, Hard, Intermediate, Wet };

struct TireCompoundData {
    TireCompound compound = TireCompound::Medium;
    std::string name = "Medium";
    float gripFactor = 1.0f;
    float wearResistance = 1.0f;
    float optimalTempMin = 70.0f;
    float optimalTempMax = 100.0f;
    static TireCompoundData getCompoundData(TireCompound c);
};

struct TireThermalState {
    float carcassTemp = 80.0f;
    float surfaceTemp = 85.0f;
    float ambientTemp = 25.0f;
    float roadTemp = 30.0f;
};

struct TireThermalConfig {
    float heatCapacity = 800.0f;
    float coolingCoeff = 25.0f;
    float frictionHeatFactor = 0.15f;
};

struct TireWearConfig {
    float baseWearRate = 0.0001f;
    float slipWearFactor = 1.5f;
    float tempWearFactor = 1.2f;
};

struct TireWearDetailState {
    float wear = 0.0f;
    float graining = 0.0f;
    float blistering = 0.0f;
    float gripMultiplier() const {
        return std::clamp(1.0f - wear * 0.5f - graining * 0.2f - blistering * 0.3f, 0.3f, 1.0f);
    }
};

class TireWearSystem {
public:
    TireWearSystem() = default;

    void setConfig(const TireThermalConfig& thermalConfig, const TireWearConfig& wearConfig) {
        m_thermalConfig = thermalConfig;
        m_wearConfig = wearConfig;
    }
    void setCompound(TireCompound compound) {
        m_compound = TireCompoundData::getCompoundData(compound);
    }
    void setCompound(const TireCompoundData& data) { m_compound = data; }

    void update(float speed, float normalLoad, float slipAngle, float slipRatio,
                float lateralForce, float longitudinalForce, float dt);

    void setAmbientTemp(float temp) { m_thermal.ambientTemp = temp; }
    void setRoadTemp(float temp) { m_thermal.roadTemp = temp; }
    void setTrackGrip(float grip) { m_trackGrip = grip; }

    const TireThermalState& thermalState() const { return m_thermal; }
    const TireWearDetailState& wearState() const { return m_wearDetail; }
    float gripMultiplier() const { return m_wearDetail.gripMultiplier() * m_compound.gripFactor * m_trackGrip; }
    float effectiveMu() const { return 1.4f * gripMultiplier(); }
    bool needsPitStop() const { return m_wearDetail.wear > 0.75f || m_wearDetail.blistering > 0.5f; }
    float estimatedLapsRemaining() const;

    void reset();

private:
    void updateThermal(float speed, float normalLoad, float slipAngle, float slipRatio,
                       float lateralForce, float longitudinalForce, float dt);
    void updateWear(float normalLoad, float slipAngle, float slipRatio,
                    float lateralForce, float longitudinalForce, float dt);

    TireThermalConfig m_thermalConfig;
    TireWearConfig m_wearConfig;
    TireCompoundData m_compound;
    TireThermalState m_thermal;
    TireWearDetailState m_wearDetail;
    float m_trackGrip = 1.0f;
    float m_distanceTraveled = 0.0f;
};

} // namespace ks::physics
