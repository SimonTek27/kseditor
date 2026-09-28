#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class AudioTimeStretch {
public:
    static AudioTimeStretch& instance() { static AudioTimeStretch s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
