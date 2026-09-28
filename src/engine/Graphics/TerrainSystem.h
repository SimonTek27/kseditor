#pragma once
/** Qt-free stub. Rendering path: engine/sim NativeRenderer. */
#include <string>
namespace ks { namespace engine { namespace graphics {
class TerrainSystem {
public:
    static TerrainSystem& instance() { static TerrainSystem s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
    std::string name() const { return "TerrainSystem"; }
};
}}} // namespace
