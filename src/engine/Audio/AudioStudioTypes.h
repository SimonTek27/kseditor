#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class AudioStudioTypes {
public:
    static AudioStudioTypes& instance() { static AudioStudioTypes s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
