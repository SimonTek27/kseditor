#pragma once
#include <string>
namespace ks { namespace engine { namespace animation {
class AnimationTimeline {
public:
    static AnimationTimeline& instance() { static AnimationTimeline s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
