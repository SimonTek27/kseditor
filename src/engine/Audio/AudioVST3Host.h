#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class AudioVST3Host {
public:
    static AudioVST3Host& instance() { static AudioVST3Host s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
