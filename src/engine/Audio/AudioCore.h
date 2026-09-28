#pragma once
/** Qt-free audio core with real state + optional backend hook. */
#include <string>
#include <cstdint>
#include <functional>
#include <cmath>
#include <algorithm>

namespace ks {
namespace engine {
namespace audio {

struct AudioListener {
    float x = 0, y = 0, z = 0;
    float fx = 0, fy = 0, fz = -1;
    float ux = 0, uy = 1, uz = 0;
};

class AudioCore {
public:
    static AudioCore& instance() {
        static AudioCore s;
        return s;
    }

    bool initialize() {
        m_ok = true;
        if (onReady) onReady();
        return true;
    }

    void shutdown() {
        m_ok = false;
        m_rpm = 0;
        m_throttle = 0;
    }

    bool isInitialized() const { return m_ok; }

    void setMasterVolume(float v) {
        m_master = std::clamp(v, 0.f, 1.f);
    }
    float masterVolume() const { return m_master; }

    void setListener(const AudioListener& l) { m_listener = l; }
    const AudioListener& listener() const { return m_listener; }

    void setEngineRpm(float rpm) { m_rpm = std::max(0.f, rpm); }
    void setThrottle(float t) { m_throttle = std::clamp(t, 0.f, 1.f); }
    float engineRpm() const { return m_rpm; }
    float throttle() const { return m_throttle; }

    /** Simple engine-noise amplitude model for UI/meters (0..1). */
    float engineLevel() const {
        if (!m_ok) return 0.f;
        float rpmN = std::clamp(m_rpm / 8000.f, 0.f, 1.5f);
        return std::clamp(m_master * (0.15f + 0.85f * rpmN * (0.3f + 0.7f * m_throttle)), 0.f, 1.f);
    }

    void update(float /*dt*/) {
        if (onEngineLevel) onEngineLevel(engineLevel());
    }

    std::function<void()> onReady;
    std::function<void(float level)> onEngineLevel;

private:
    bool m_ok = false;
    float m_master = 1.0f;
    float m_rpm = 0;
    float m_throttle = 0;
    AudioListener m_listener;
};

} // namespace audio
} // namespace engine
} // namespace ks
