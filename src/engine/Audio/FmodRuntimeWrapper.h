#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class FmodRuntimeWrapper {
public:
    static FmodRuntimeWrapper& instance() { static FmodRuntimeWrapper s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
