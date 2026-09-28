#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class RPMRecorder {
public:
    static RPMRecorder& instance() { static RPMRecorder s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
