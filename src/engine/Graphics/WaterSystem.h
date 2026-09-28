#pragma once
#include <string>
namespace ks { namespace engine { namespace graphics {
class WaterSystem {
public:
    static WaterSystem& instance() { static WaterSystem s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
    std::string name() const { return "WaterSystem"; }
};
}}} // namespace
