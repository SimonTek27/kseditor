#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class WaveformEngine {
public:
    static WaveformEngine& instance() { static WaveformEngine s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
