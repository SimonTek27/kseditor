#pragma once
#include <string>
namespace ks { namespace engine { namespace tools {
class CollisionMeshGenerator {
public:
    static CollisionMeshGenerator& instance() { static CollisionMeshGenerator s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
