#pragma once
/** Qt-free audio core facade. */
#include <string>
#include <cstdint>
#include <functional>

namespace ks {
namespace engine {
namespace audio {

struct AudioListener {
    float x=0,y=0,z=0;
    float fx=0,fy=0,fz=-1;
    float ux=0,uy=1,uz=0;
};

class AudioCore {
public:
    static AudioCore& instance() { static AudioCore s; return s; }
    bool initialize() { m_ok = true; return true; }
    void shutdown() { m_ok = false; }
    bool isInitialized() const { return m_ok; }
    void setMasterVolume(float v) { m_master = v < 0 ? 0 : (v > 1 ? 1 : v); }
    float masterVolume() const { return m_master; }
    void setListener(const AudioListener& l) { m_listener = l; }
    const AudioListener& listener() const { return m_listener; }
    void setEngineRpm(float rpm) { m_rpm = rpm; }
    void setThrottle(float t) { m_throttle = t; }
    void update(float /*dt*/) {}
    std::function<void()> onReady;
private:
    bool m_ok = false;
    float m_master = 1.0f;
    float m_rpm = 0, m_throttle = 0;
    AudioListener m_listener;
};

} // namespace audio
} // namespace engine
} // namespace ks
