#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class LADSPAHost {
public:
    static LADSPAHost& instance() { static LADSPAHost s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
