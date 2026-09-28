#pragma once
#include <string>
namespace ks { namespace engine { namespace mesh {
class AdvancedMeshOps {
public:
    static AdvancedMeshOps& instance() { static AdvancedMeshOps s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
