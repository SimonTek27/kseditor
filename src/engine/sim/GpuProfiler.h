#pragma once

/**
 * @file GpuProfiler.h
 * @brief Vulkan GPU timestamp profiler — Qt-free
 */

#include <array>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#if defined(HAS_VULKAN) || defined(VK_VERSION_1_0)
#  include <vulkan/vulkan.h>
#  define KS_GPU_PROFILER_VK 1
#else
#  define KS_GPU_PROFILER_VK 0
struct VkDevice_T;
struct VkPhysicalDevice_T;
struct VkCommandBuffer_T;
struct VkQueryPool_T;
using VkDevice = VkDevice_T*;
using VkPhysicalDevice = VkPhysicalDevice_T*;
using VkCommandBuffer = VkCommandBuffer_T*;
using VkQueryPool = VkQueryPool_T*;
using VkResult = int;
#endif

namespace ks {
namespace sim {

class GpuProfiler {
public:
    enum class Pass : int {
        FrameBegin = 0,
        Shadow,
        MainPass,
        UI,
        FrameEnd,
        Count
    };

    static constexpr int kPassCount = static_cast<int>(Pass::Count);
    static constexpr int kFramesInFlight = 2;
    static constexpr int kQueriesPerFrame = kPassCount;

    GpuProfiler() = default;
    ~GpuProfiler();

    GpuProfiler(const GpuProfiler&) = delete;
    GpuProfiler& operator=(const GpuProfiler&) = delete;

    bool init(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex);
    void shutdown();

    void setEnabled(bool e) { m_enabled = e; }
    bool isEnabled() const { return m_enabled && m_ready; }

    void beginFrame(VkCommandBuffer cmd);
    void writeTimestamp(VkCommandBuffer cmd, Pass pass);
    void endFrame();

    double frameTimeMs() const { return m_frameTimeMs; }
    double avgFrameTimeMs() const { return m_avgFrameTimeMs; }
    double passTimeMs(Pass p) const {
        const int i = static_cast<int>(p);
        return (i >= 0 && i < kPassCount) ? m_passMs[static_cast<size_t>(i)] : 0.0;
    }
    int fps() const {
        return m_frameTimeMs > 0.01 ? static_cast<int>(1000.0 / m_frameTimeMs) : 0;
    }

    static const char* passName(Pass p);

private:
    void resolveFrame(int slot);

    bool m_enabled = true;
    bool m_ready = false;
    VkDevice m_device = nullptr;
    float m_timestampPeriodNs = 1.0f;

#if KS_GPU_PROFILER_VK
    std::array<VkQueryPool, kFramesInFlight> m_pools{};
#else
    std::array<void*, kFramesInFlight> m_pools{};
#endif

    int m_frameIndex = 0;
    int m_resolveIndex = -1;
    std::array<bool, kFramesInFlight> m_slotUsed{};

    double m_frameTimeMs = 0.0;
    double m_avgFrameTimeMs = 0.0;
    std::array<double, kPassCount> m_passMs{};
    std::mutex m_mutex;
};

struct GpuPassScope {
    GpuProfiler& profiler;
    GpuProfiler::Pass pass;
    VkCommandBuffer cmd;
    GpuPassScope(GpuProfiler& p, VkCommandBuffer c, GpuProfiler::Pass pass_)
        : profiler(p), pass(pass_), cmd(c) {
        profiler.writeTimestamp(cmd, pass);
    }
};

} // namespace sim
} // namespace ks

#define KS_GPU_TIMESTAMP(profiler, cmd, pass) \
    do { if ((profiler).isEnabled()) (profiler).writeTimestamp((cmd), (pass)); } while (0)
