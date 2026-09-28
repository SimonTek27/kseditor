#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class AudioModuleBridge {
public:
    static AudioModuleBridge& instance() { static AudioModuleBridge s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
