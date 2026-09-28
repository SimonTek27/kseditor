#pragma once
/** Qt-free cascaded shadow map placeholder for SimulatorApp. */

#include <cstdint>

namespace ks {
namespace sim {

class CascadedShadowMap {
public:
    bool initialize() { return true; }
    void shutdown() {}
    void resize(uint32_t, uint32_t) {}
    void setCascadeCount(int) {}
    bool isReady() const { return false; }
};

} // namespace sim
} // namespace ks
