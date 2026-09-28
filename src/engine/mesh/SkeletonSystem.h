#pragma once
#include <string>
namespace ks { namespace engine { namespace mesh {
class SkeletonSystem {
public:
    static SkeletonSystem& instance() { static SkeletonSystem s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
