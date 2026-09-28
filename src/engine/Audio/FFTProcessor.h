#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class FFTProcessor {
public:
    static FFTProcessor& instance() { static FFTProcessor s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
