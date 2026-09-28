#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class AudioEffects {
public:
    static AudioEffects& instance() { static AudioEffects s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
