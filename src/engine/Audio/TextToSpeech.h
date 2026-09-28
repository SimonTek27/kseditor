#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class TextToSpeech {
public:
    static TextToSpeech& instance() { static TextToSpeech s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
    void speak(const std::string&) {}
};
}}} // namespace
