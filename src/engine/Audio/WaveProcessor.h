#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class WaveProcessor {
public:
    static WaveProcessor& instance() { static WaveProcessor s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
