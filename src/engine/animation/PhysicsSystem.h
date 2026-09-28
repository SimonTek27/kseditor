#pragma once
#include <string>
namespace ks { namespace engine { namespace animation {
class PhysicsSystem {
public:
    static PhysicsSystem& instance() { static PhysicsSystem s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
