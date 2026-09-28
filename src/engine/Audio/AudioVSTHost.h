#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class AudioVSTHost {
public:
    static AudioVSTHost& instance() { static AudioVSTHost s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
