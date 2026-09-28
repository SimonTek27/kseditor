#pragma once
/**
 * Qt-free Vulkan integration facade.
 * Full device/swapchain lives in NativeRenderer / VulkanRenderer.
 * This type no longer inherits QObject.
 */
#include <string>
#include <functional>
#include <cstdint>

namespace ks {

class VulkanIntegration {
public:
    struct GraphicsSettings {
        int maxFrameLatency = 0;
        float mipLodBias = 0.0f;
        float shadowMapBias0 = 0.000002f;
        float shadowMapBias1 = 0.000015f;
        float shadowMapBias2 = 0.0003f;
        float skyboxReflectionGain = 1.5f;
        bool allowUnsupportedDX10 = false;
    };

    struct LightingSettings {
        float ambientColor[3] = {0.1f, 0.1f, 0.12f};
        float sunColor[3] = {1.f, 0.95f, 0.9f};
        float sunDirection[3] = {0.3f, 0.8f, 0.2f};
    };

    static VulkanIntegration& instance() {
        static VulkanIntegration s;
        return s;
    }

    bool initialize() { m_ok = true; return true; }
    void shutdown() { m_ok = false; }
    bool isReady() const { return m_ok; }

    GraphicsSettings& graphics() { return m_gfx; }
    const GraphicsSettings& graphics() const { return m_gfx; }
    LightingSettings& lighting() { return m_light; }
    const LightingSettings& lighting() const { return m_light; }

    void beginFrame() {}
    void endFrame() { if (onFrameEnd) onFrameEnd(); }

    std::function<void()> onFrameEnd;

private:
    bool m_ok = false;
    GraphicsSettings m_gfx;
    LightingSettings m_light;
};

} // namespace ks
