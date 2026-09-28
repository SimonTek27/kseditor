#pragma once
#include <string>
namespace ks { namespace engine { namespace mesh {
class MeshOperations {
public:
    static MeshOperations& instance() { static MeshOperations s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
