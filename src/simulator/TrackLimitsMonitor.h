#pragma once
/**
 * Track-limits detection → RaceSessionManager::reportTrackLimitsViolation.
 */
#include "RaceSessionManager.h"
#include "MathTypes.h"
#include <cmath>
#include <vector>
#include <functional>
#include <algorithm>

namespace ks {
namespace sim {

struct TrackLimitsConfig {
    float maxLateralM = 8.0f;
    float graceSec = 0.35f;
    float cooldownSec = 1.5f;
    bool enabled = true;
};

class TrackLimitsMonitor {
public:
    void setConfig(const TrackLimitsConfig& c) { m_cfg = c; }
    const TrackLimitsConfig& config() const { return m_cfg; }
    void setCenterline(const std::vector<vec3>& pts) { m_center = pts; }

    void update(float dt, int carIndex, const vec3& pos, bool onTrackHint,
                RaceSessionManager* session) {
        if (!m_cfg.enabled || !session) return;
        bool off = !onTrackHint;
        if (!m_center.empty()) {
            float lat = lateralDistance(pos);
            if (lat > m_cfg.maxLateralM) off = true;
            else if (onTrackHint) off = false;
        }
        if (carIndex < 0) carIndex = 0;
        if ((int)m_state.size() <= carIndex)
            m_state.resize(static_cast<size_t>(carIndex) + 1);
        auto& st = m_state[static_cast<size_t>(carIndex)];
        st.cooldown = std::max(0.f, st.cooldown - dt);
        if (off) {
            st.offTimer += dt;
            if (st.offTimer >= m_cfg.graceSec && st.cooldown <= 0.f) {
                session->reportTrackLimitsViolation(carIndex);
                st.cooldown = m_cfg.cooldownSec;
                st.offTimer = 0.f;
                st.reports++;
                if (onViolation) onViolation(carIndex, st.reports);
            }
        } else {
            st.offTimer = 0.f;
        }
    }

    std::function<void(int carIndex, int totalReports)> onViolation;

private:
    float lateralDistance(const vec3& pos) const {
        if (m_center.size() < 2) return 0.f;
        float best = 1e9f;
        for (size_t i = 0; i + 1 < m_center.size(); ++i) {
            const vec3& a = m_center[i];
            const vec3& b = m_center[i + 1];
            float abx = b.x - a.x, abz = b.z - a.z;
            float apx = pos.x - a.x, apz = pos.z - a.z;
            float ab2 = abx * abx + abz * abz;
            float t = ab2 > 1e-6f ? std::clamp((apx * abx + apz * abz) / ab2, 0.f, 1.f) : 0.f;
            float cx = a.x + abx * t, cz = a.z + abz * t;
            float dx = pos.x - cx, dz = pos.z - cz;
            float d = std::sqrt(dx * dx + dz * dz);
            if (d < best) best = d;
        }
        return best;
    }
    struct CarState { float offTimer = 0.f; float cooldown = 0.f; int reports = 0; };
    TrackLimitsConfig m_cfg;
    std::vector<vec3> m_center;
    std::vector<CarState> m_state;
};

} // namespace sim
} // namespace ks
