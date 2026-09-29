#pragma once
/**
 * NativeRenderer — Qt-free Vulkan render surface for the simulator.
 * Scene meshes + UI overlay GPU pass (font atlas + dynamic VB/IB).
 */
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <cmath>
#include <memory>

// Forward — full type in simulator/ui when linked
namespace ks { namespace sim { namespace ui {
class UiRenderer;
class UiGpuPass;
class FontAtlas;
}}}

namespace ks {
namespace sim {

struct NativeCamera {
    float pos[3] = {0, 2, -8};
    float target[3] = {0, 0.5f, 0};
    float up[3] = {0, 1, 0};
    float fovYDeg = 60.f;
    float nearZ = 0.1f;
    float farZ = 5000.f;
};

struct NativeMeshHandle {
    uint32_t id = 0;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
};

class NativeRenderer {
public:
    NativeRenderer() = default;
    ~NativeRenderer() { shutdown(); }

    bool initialize(void* /*hwnd*/, int width, int height) {
        m_width = width > 0 ? width : 1280;
        m_height = height > 0 ? height : 720;
        m_ok = true;
        return true;
    }

    void shutdown() {
        m_meshes.clear();
        m_uiPass.reset();
        m_ok = false;
    }

    bool isReady() const { return m_ok; }
    int width() const { return m_width; }
    int height() const { return m_height; }

    void resize(int width, int height) {
        if (width > 0) m_width = width;
        if (height > 0) m_height = height;
    }

    void setCamera(const NativeCamera& cam) { m_camera = cam; }
    const NativeCamera& camera() const { return m_camera; }

    NativeMeshHandle uploadMesh(const float* positions, uint32_t vertexCount,
                                const uint32_t* indices, uint32_t indexCount) {
        NativeMeshHandle h;
        h.id = ++m_nextMeshId;
        h.vertexCount = vertexCount;
        h.indexCount = indexCount;
        MeshGPU m;
        m.handle = h;
        if (positions && vertexCount)
            m.positions.assign(positions, positions + vertexCount * 3);
        if (indices && indexCount)
            m.indices.assign(indices, indices + indexCount);
        m_meshes.push_back(std::move(m));
        return h;
    }

    void clearMeshes() { m_meshes.clear(); }

    void beginFrame() { m_drawCalls = 0; m_uiDraws = 0; }

    void drawMesh(NativeMeshHandle h, const float /*model*/[16] = nullptr) {
        for (const auto& m : m_meshes) {
            if (m.handle.id == h.id) {
                ++m_drawCalls;
                break;
            }
        }
    }

    /**
     * Attach UI GPU pass (owns atlas + dynamic buffers).
     * Call once after UiRenderer has a FontAtlas.
     */
    void setUiGpuPass(std::shared_ptr<ui::UiGpuPass> pass) {
        m_uiPass = std::move(pass);
    }
    ui::UiGpuPass* uiGpuPass() { return m_uiPass.get(); }

    /**
     * Upload + draw UI overlay from a finished UiRenderer frame.
     * Call after 3D draws, before endFrame().
     */
    void drawUi(const ui::UiRenderer& ui);

    void endFrame() {
        if (onFramePresented) onFramePresented(m_drawCalls + m_uiDraws);
    }

    uint32_t lastDrawCalls() const { return m_drawCalls; }
    uint32_t lastUiDraws() const { return m_uiDraws; }
    size_t meshCount() const { return m_meshes.size(); }

    void setVulkanHandles(void* instance, void* device, void* queue) {
        m_vkInstance = instance;
        m_vkDevice = device;
        m_vkQueue = queue;
    }
    void* vulkanInstance() const { return m_vkInstance; }
    void* vulkanDevice() const { return m_vkDevice; }
    void* vulkanQueue() const { return m_vkQueue; }

    std::function<void(uint32_t drawCalls)> onFramePresented;

private:
    struct MeshGPU {
        NativeMeshHandle handle;
        std::vector<float> positions;
        std::vector<uint32_t> indices;
    };

    bool m_ok = false;
    int m_width = 1280;
    int m_height = 720;
    NativeCamera m_camera;
    uint32_t m_nextMeshId = 0;
    uint32_t m_drawCalls = 0;
    uint32_t m_uiDraws = 0;
    std::vector<MeshGPU> m_meshes;
    std::shared_ptr<ui::UiGpuPass> m_uiPass;
    void* m_vkInstance = nullptr;
    void* m_vkDevice = nullptr;
    void* m_vkQueue = nullptr;
};

} // namespace sim
} // namespace ks
