#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class AudioEffectsAdvanced {
public:
    static AudioEffectsAdvanced& instance() { static AudioEffectsAdvanced s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
