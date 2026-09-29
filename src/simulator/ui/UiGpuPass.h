#pragma once
/**
 * UiGpuPass — uploads FontAtlas + dynamic UI vertices/indices and records draws.
 * Vulkan handles are opaque void*; platform bootstrap can bind real Vk* via setVulkanContext.
 * Works offline (CPU mirror) when no device is set — still validates batch sizes.
 */
#include "UiRenderer.h"
#include <cstdint>
#include <vector>
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <functional>

namespace ks {
namespace sim {
namespace ui {

struct UiGpuVulkanContext {
    void* device = nullptr;          // VkDevice
    void* physicalDevice = nullptr;  // VkPhysicalDevice
    void* queue = nullptr;           // VkQueue
    void* commandBuffer = nullptr;   // VkCommandBuffer (current frame)
    void* renderPass = nullptr;      // VkRenderPass for UI subpass
    uint32_t queueFamily = 0;
};

class UiGpuPass {
public:
    static constexpr size_t kMaxVertices = 65536;
    static constexpr size_t kMaxIndices  = 98304;

    bool initialize(const FontAtlas& atlas) {
        m_atlasW = atlas.width();
        m_atlasH = atlas.height();
        m_atlasCpu = atlas.pixelsR8();
        m_atlasDirty = true;
        m_ok = true;
        std::fprintf(stderr, "UiGpuPass: atlas %dx%d R8 ready (%zu bytes)\n",
                     m_atlasW, m_atlasH, m_atlasCpu.size());
        return true;
    }

    void shutdown() {
        m_ok = false;
        m_vertices.clear();
        m_indices.clear();
        m_atlasCpu.clear();
        // Real Vk destroy left to platform if handles were injected
        m_vb = m_ib = m_atlasImage = m_atlasView = m_sampler = m_pipeline = nullptr;
    }

    void setVulkanContext(const UiGpuVulkanContext& ctx) {
        m_ctx = ctx;
    }

    /** Upload atlas once (or when replaced). Platform implements real Vk path via hooks. */
    void ensureAtlasUploaded() {
        if (!m_atlasDirty || !m_ok) return;
        if (onUploadAtlas)
            onUploadAtlas(m_atlasCpu.data(), m_atlasW, m_atlasH);
        m_atlasDirty = false;
    }

    /** Copy UiRenderer batch into dynamic buffers (CPU + optional GPU hook). */
    void uploadFrame(const UiRenderer& ui) {
        if (!m_ok) return;
        ensureAtlasUploaded();

        const auto& verts = ui.vertices();
        const auto& inds  = ui.indices();
        const size_t vc = std::min(verts.size(), kMaxVertices);
        const size_t ic = std::min(inds.size(), kMaxIndices);

        m_vertices.resize(vc);
        if (vc) std::memcpy(m_vertices.data(), verts.data(), vc * sizeof(UiVertex));
        m_indices.resize(ic);
        if (ic) std::memcpy(m_indices.data(), inds.data(), ic * sizeof(uint32_t));

        m_screenW = static_cast<float>(ui.viewportWidth());
        m_screenH = static_cast<float>(ui.viewportHeight());

        if (onUploadDynamic)
            onUploadDynamic(m_vertices.data(), static_cast<uint32_t>(vc),
                            m_indices.data(), static_cast<uint32_t>(ic));

        m_indexCount = static_cast<uint32_t>(ic);
        m_vertexCount = static_cast<uint32_t>(vc);
    }

    /** Record draw (or no-op CPU). Call inside render pass after 3D. */
    void draw() {
        if (!m_ok || m_indexCount == 0) return;
        if (onDraw) {
            onDraw(m_vertexCount, m_indexCount, m_screenW, m_screenH);
            return;
        }
        // Fallback: count as soft draw for validation
        ++m_softDraws;
    }

    uint32_t vertexCount() const { return m_vertexCount; }
    uint32_t indexCount() const { return m_indexCount; }
    uint32_t softDrawCount() const { return m_softDraws; }
    const std::vector<UiVertex>& cpuVertices() const { return m_vertices; }
    const std::vector<uint32_t>& cpuIndices() const { return m_indices; }
    const uint8_t* atlasPixels() const { return m_atlasCpu.data(); }
    int atlasWidth() const { return m_atlasW; }
    int atlasHeight() const { return m_atlasH; }

    // --- Platform hooks (bind real Vulkan in bootstrap) ---
    std::function<void(const uint8_t* r8, int w, int h)> onUploadAtlas;
    std::function<void(const UiVertex* v, uint32_t vc, const uint32_t* i, uint32_t ic)> onUploadDynamic;
    std::function<void(uint32_t vc, uint32_t ic, float screenW, float screenH)> onDraw;

    // Opaque resource slots (filled by platform)
    void* m_vb = nullptr;
    void* m_ib = nullptr;
    void* m_atlasImage = nullptr;
    void* m_atlasView = nullptr;
    void* m_sampler = nullptr;
    void* m_pipeline = nullptr;
    void* m_descSet = nullptr;

private:
    bool m_ok = false;
    bool m_atlasDirty = true;
    int m_atlasW = 0, m_atlasH = 0;
    std::vector<uint8_t> m_atlasCpu;
    std::vector<UiVertex> m_vertices;
    std::vector<uint32_t> m_indices;
    uint32_t m_vertexCount = 0;
    uint32_t m_indexCount = 0;
    float m_screenW = 1280.f, m_screenH = 720.f;
    uint32_t m_softDraws = 0;
    UiGpuVulkanContext m_ctx{};
};

} // namespace ui
} // namespace sim
} // namespace ks
