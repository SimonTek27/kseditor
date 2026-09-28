#pragma once
#include <string>
namespace ks { namespace engine { namespace mesh {
class MeshDataBridge {
public:
    static MeshDataBridge& instance() { static MeshDataBridge s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
