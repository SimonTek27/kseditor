#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class AudioEngineSimIntegration {
public:
    static AudioEngineSimIntegration& instance() { static AudioEngineSimIntegration s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
