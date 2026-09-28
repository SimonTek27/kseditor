#pragma once
#include "AudioCore.h"
#include <string>

namespace ks {
namespace engine {
namespace audio {

class DrivingAudio {
public:
    static DrivingAudio& instance() { static DrivingAudio s; return s; }
    bool initialize() { return AudioCore::instance().initialize(); }
    void shutdown() { AudioCore::instance().shutdown(); }
    void setRpm(float rpm) { AudioCore::instance().setEngineRpm(rpm); }
    void setThrottle(float t) { AudioCore::instance().setThrottle(t); }
    void setSurface(const std::string& /*name*/) {}
    void update(float dt) { AudioCore::instance().update(dt); }
};

} // namespace audio
} // namespace engine
} // namespace ks
