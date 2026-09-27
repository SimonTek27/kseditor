#pragma once

/**
 * @file HybridSystem.h
 * @brief Minimal ERS / hybrid energy store — Qt-free
 */

#include <algorithm>
#include <cmath>

namespace ks {
namespace physics {

class HybridSystem {
public:
    enum class Mode { Off = 0, Deploy, Harvest, Attack };

    void setEnabled(bool e) { m_enabled = e; }
    bool isEnabled() const { return m_enabled; }

    void setMode(Mode m) { m_mode = m; }
    Mode mode() const { return m_mode; }
    void activateAttack() { m_mode = Mode::Attack; m_attackTimer = 5.0f; }

    float soc() const { return m_soc; }
    float maxDeployKw() const { return m_maxDeployKw; }

    /** Extra motor torque contribution (Nm-scale proxy). Harvest returns negative. */
    float update(float dt, float throttle, float brake, float speedMs) {
        if (!m_enabled) return 0.0f;
        dt = std::clamp(dt, 1e-4f, 0.05f);

        if (m_attackTimer > 0.0f) {
            m_attackTimer -= dt;
            if (m_attackTimer <= 0.0f && m_mode == Mode::Attack)
                m_mode = Mode::Deploy;
        }

        float powerKw = 0.0f;
        if (m_mode == Mode::Deploy || m_mode == Mode::Attack) {
            if (throttle > 0.2f && m_soc > 0.05f) {
                const float maxKw = (m_mode == Mode::Attack) ? m_maxDeployKw * 1.2f : m_maxDeployKw;
                powerKw = maxKw * std::clamp(throttle, 0.0f, 1.0f);
                m_soc -= (powerKw / 3600.0f) * dt * 0.15f;
            }
        } else if (m_mode == Mode::Harvest) {
            if (brake > 0.1f || throttle < 0.05f) {
                powerKw = -m_maxHarvestKw * std::clamp(std::max(brake, 0.3f), 0.0f, 1.0f);
                m_soc -= (powerKw / 3600.0f) * dt * 0.12f;
            }
        }

        m_soc = std::clamp(m_soc, 0.0f, 1.0f);

        // crude Nm from kW at speed
        const float omega = std::max(speedMs, 1.0f) / 0.33f;
        return (powerKw * 1000.0f) / std::max(omega, 1.0f);
    }

    void reset() {
        m_soc = 1.0f;
        m_mode = Mode::Off;
        m_attackTimer = 0.0f;
    }

    void setLimits(float deployKw, float harvestKw) {
        m_maxDeployKw = deployKw;
        m_maxHarvestKw = harvestKw;
    }

private:
    bool m_enabled = false;
    Mode m_mode = Mode::Off;
    float m_soc = 1.0f;
    float m_maxDeployKw = 120.0f;
    float m_maxHarvestKw = 150.0f;
    float m_attackTimer = 0.0f;
};

} // namespace physics
} // namespace ks
