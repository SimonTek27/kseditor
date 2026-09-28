#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class AudioGraph {
public:
    static AudioGraph& instance() { static AudioGraph s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
