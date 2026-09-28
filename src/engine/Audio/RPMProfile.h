#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class RPMProfile {
public:
    static RPMProfile& instance() { static RPMProfile s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
