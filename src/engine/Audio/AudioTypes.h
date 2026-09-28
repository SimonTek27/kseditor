#pragma once
#include <string>
#include <cstdint>
namespace ks { namespace engine { namespace audio {
struct AudioFormat { int sampleRate=48000; int channels=2; };
class AudioTypes {
public:
    static AudioTypes& instance() { static AudioTypes s; return s; }
};
}}} // namespace
