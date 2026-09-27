#include "BrakeThermalModel.h"

namespace ks {
namespace physics {

void BrakeThermalModel::reset() {
    for (auto& s : m_state) {
        s = BrakeThermalState{};
        s.discTemp = 80.0f;
        s.padTemp = 60.0f;
    }
}

void BrakeThermalModel::update(float dt, int wheel, float brakeTorqueNm, float wheelOmega, float speedMs) {
    if (wheel < 0 || wheel >= 4) return;
    dt = std::clamp(dt, 1e-4f, 0.05f);
    auto& s = m_state[wheel];

    float power = std::abs(brakeTorqueNm * wheelOmega);
    s.energyAccum = power * dt;

    float heatCapacity = std::max(m_cfg.discMass * m_cfg.discSpecificHeat, 1.0f);
    float dT = (s.energyAccum / heatCapacity) * 1.0f;

    float cool = m_cfg.coolingCoeff * (1.0f + 0.08f * std::max(speedMs, 0.0f));
    float dTcool = -cool * (s.discTemp - m_cfg.ambientTemp) * dt / heatCapacity * 50.0f;

    s.discTemp = std::clamp(s.discTemp + dT + dTcool, m_cfg.ambientTemp, m_cfg.maxTemp);
    s.padTemp += (s.discTemp - s.padTemp) * 0.15f * dt;

    if (s.discTemp <= m_cfg.fadeStartTemp) {
        s.fade = 0.0f;
    } else if (s.discTemp >= m_cfg.fadeFullTemp) {
        s.fade = 1.0f;
    } else {
        s.fade = (s.discTemp - m_cfg.fadeStartTemp) / (m_cfg.fadeFullTemp - m_cfg.fadeStartTemp);
    }
}

} // namespace physics
} // namespace ks
