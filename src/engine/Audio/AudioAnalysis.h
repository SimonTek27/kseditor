#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class AudioAnalysis {
public:
    static AudioAnalysis& instance() { static AudioAnalysis s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
