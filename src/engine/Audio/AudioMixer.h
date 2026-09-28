#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class AudioMixer {
public:
    static AudioMixer& instance() { static AudioMixer s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
    void setBusVolume(const std::string&, float) {}
};
}}} // namespace
