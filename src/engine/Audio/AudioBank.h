#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class AudioBank {
public:
    static AudioBank& instance() { static AudioBank s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
