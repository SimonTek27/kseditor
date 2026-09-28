#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class AudioClip {
public:
    static AudioClip& instance() { static AudioClip s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
