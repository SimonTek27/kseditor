#pragma once
#include <string>
namespace ks { namespace engine { namespace graphics {
class GPUParticleSystem {
public:
    static GPUParticleSystem& instance() { static GPUParticleSystem s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
    std::string name() const { return "GPUParticleSystem"; }
};
}}} // namespace
