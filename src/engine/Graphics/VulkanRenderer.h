#pragma once
/**
 * VulkanRenderer — Qt-free device/swapchain handle holder.
 * Prefer ks::sim::NativeRenderer for frame submission in the sim app.
 * This class stores opaque Vk pointers set by the platform bootstrap.
 */
#include <cstdint>
#include <string>

namespace ks {

class VulkanRenderer {
public:
    bool initialize() {
        m_initialized = true;
        return true;
    }

    bool createDevice(void* instance, void* surface) {
        m_instance = instance;
        m_surface = surface;
        // Real VkDevice creation is platform/bootstrap responsibility.
        m_deviceReady = (instance != nullptr);
        return m_deviceReady;
    }

    void createSwapChain(void* /*surface*/, int width, int height) {
        m_width = width > 0 ? width : m_width;
        m_height = height > 0 ? height : m_height;
        m_swapchainReady = m_deviceReady;
    }

    void recreateSwapChain(int width, int height) {
        createSwapChain(m_surface, width, height);
    }

    bool isInitialized() const { return m_initialized && m_deviceReady; }
    bool hasSwapchain() const { return m_swapchainReady; }
    int width() const { return m_width; }
    int height() const { return m_height; }

    void* instance() const { return m_instance; }
    void* device() const { return m_device; }
    void* surface() const { return m_surface; }

    void setDevice(void* device) { m_device = device; m_deviceReady = device != nullptr; }

    void shutdown() {
        m_initialized = false;
        m_deviceReady = false;
        m_swapchainReady = false;
        m_instance = m_device = m_surface = nullptr;
    }

private:
    bool m_initialized = false;
    bool m_deviceReady = false;
    bool m_swapchainReady = false;
    int m_width = 1280;
    int m_height = 720;
    void* m_instance = nullptr;
    void* m_device = nullptr;
    void* m_surface = nullptr;
};

} // namespace ks
