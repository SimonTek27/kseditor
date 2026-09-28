#pragma once
#include <string>
namespace ks { namespace engine { namespace mesh {
class PhysicsCollisionSystem {
public:
    static PhysicsCollisionSystem& instance() { static PhysicsCollisionSystem s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
