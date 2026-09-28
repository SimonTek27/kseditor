#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class AudioBankGenerator {
public:
    static AudioBankGenerator& instance() { static AudioBankGenerator s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
