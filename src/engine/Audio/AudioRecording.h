#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class AudioRecording {
public:
    static AudioRecording& instance() { static AudioRecording s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
