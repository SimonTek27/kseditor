#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class DSPLibrary {
public:
    static DSPLibrary& instance() { static DSPLibrary s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
