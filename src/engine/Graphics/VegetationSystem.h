#pragma once
#include <string>
namespace ks { namespace engine { namespace graphics {
class VegetationSystem {
public:
    static VegetationSystem& instance() { static VegetationSystem s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
    std::string name() const { return "VegetationSystem"; }
};
}}} // namespace
