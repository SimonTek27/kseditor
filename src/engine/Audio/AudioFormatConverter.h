#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class AudioFormatConverter {
public:
    static AudioFormatConverter& instance() { static AudioFormatConverter s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
