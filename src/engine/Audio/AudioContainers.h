#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class AudioContainers {
public:
    static AudioContainers& instance() { static AudioContainers s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
