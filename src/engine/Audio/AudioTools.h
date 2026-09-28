#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class AudioTools {
public:
    static AudioTools& instance() { static AudioTools s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
