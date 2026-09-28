#pragma once
/** Qt-free stub. Prefer ks::sim::NativeRenderer for real Vulkan. */
#include <cstdint>
#include <string>
namespace ks {
class VulkanRenderer {
public:
    bool initialize() { return true; }
    bool createDevice(void*, void*) { return false; }
    void createSwapChain(void*, int, int) {}
    void recreateSwapChain(int, int) {}
    bool isInitialized() const { return false; }
    void shutdown() {}
};
} // namespace ks
