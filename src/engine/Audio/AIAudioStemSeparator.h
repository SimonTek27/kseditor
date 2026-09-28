#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class AIAudioStemSeparator {
public:
    static AIAudioStemSeparator& instance() { static AIAudioStemSeparator s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
