#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class AudioGenerator {
public:
    static AudioGenerator& instance() { static AudioGenerator s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
