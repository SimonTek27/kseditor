#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class CarAcoustics {
public:
    static CarAcoustics& instance() { static CarAcoustics s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
    void setRpm(float) {}
    void setThrottle(float) {}
};
}}} // namespace
