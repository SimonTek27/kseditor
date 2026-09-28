#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class AudioExporter {
public:
    static AudioExporter& instance() { static AudioExporter s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
