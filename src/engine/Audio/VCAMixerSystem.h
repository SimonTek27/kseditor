#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class VCAMixerSystem {
public:
    static VCAMixerSystem& instance() { static VCAMixerSystem s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
