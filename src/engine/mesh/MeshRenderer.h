#pragma once
#include <string>
namespace ks { namespace engine { namespace mesh {
class MeshRenderer {
public:
    static MeshRenderer& instance() { static MeshRenderer s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
