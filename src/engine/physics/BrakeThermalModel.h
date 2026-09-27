#pragma once

/**
 * @file BrakeThermalModel.h
 * @brief Disc/pad thermal model per corner — Qt-free
 */

#include <array>
#include <algorithm>
#include <cmath>

namespace ks {
namespace physics {

struct BrakeThermalConfig {
    float discMass = 8.0f;
    float discSpecificHeat = 500.0f;
    float coolingCoeff = 15.0f;
    float ambientTemp = 25.0f;
    float fadeStartTemp = 400.0f;
    float fadeFullTemp = 700.0f;
    float maxTemp = 900.0f;
};

struct BrakeThermalState {
    float discTemp = 80.0f;
    float padTemp = 60.0f;
    float fade = 0.0f;
    float energyAccum = 0.0f;
};

class BrakeThermalModel {
public:
    BrakeThermalModel() = default;

    void setConfig(const BrakeThermalConfig& c) { m_cfg = c; }
    const BrakeThermalConfig& config() const { return m_cfg; }

    void update(float dt, int wheel, float brakeTorqueNm, float wheelOmega, float speedMs);

    BrakeThermalState state(int wheel) const {
        return (wheel >= 0 && wheel < 4) ? m_state[wheel] : BrakeThermalState{};
    }

    float fade(int wheel) const { return state(wheel).fade; }
    float discTemp(int wheel) const { return state(wheel).discTemp; }

    float effectiveTorque(int wheel, float commandedTorque) const {
        return commandedTorque * (1.0f - fade(wheel));
    }

    void reset();

private:
    BrakeThermalConfig m_cfg;
    std::array<BrakeThermalState, 4> m_state{};
};

} // namespace physics
} // namespace ks
