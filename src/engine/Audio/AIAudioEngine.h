#pragma once
/** Qt-free stub — original in git history. */
#include <string>
namespace ks { namespace engine { namespace audio {
class AIAudioEngine {
public:
    static AIAudioEngine& instance() { static AIAudioEngine s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
