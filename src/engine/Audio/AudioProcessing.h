#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class AudioProcessing {
public:
    static AudioProcessing& instance() { static AudioProcessing s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
