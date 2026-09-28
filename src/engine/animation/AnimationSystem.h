#pragma once
#include <string>
namespace ks { namespace engine { namespace animation {
class AnimationSystem {
public:
    static AnimationSystem& instance() { static AnimationSystem s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
    void update(float /*dt*/) {}
};
}}} // namespace
