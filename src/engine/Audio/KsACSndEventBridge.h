#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class KsACSndEventBridge {
public:
    static KsACSndEventBridge& instance() { static KsACSndEventBridge s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
