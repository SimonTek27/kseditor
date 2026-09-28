#pragma once
#include <string>
namespace ks { namespace engine { namespace mesh {
class PhysicsMeshGenerator {
public:
    static PhysicsMeshGenerator& instance() { static PhysicsMeshGenerator s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
