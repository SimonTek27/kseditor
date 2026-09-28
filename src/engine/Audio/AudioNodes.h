#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class AudioNodes {
public:
    static AudioNodes& instance() { static AudioNodes s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
