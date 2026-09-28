#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class AudioSpatial {
public:
    static AudioSpatial& instance() { static AudioSpatial s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
