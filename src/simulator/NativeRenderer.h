#pragma once

// Qt-free native Vulkan renderer for the SimulatorApp runtime.
//
// Why this exists: ks::VulkanRenderer (src/engine/Graphics/VulkanRenderer.h)
// is a QObject that uses QVulkanWindow/QVulkanInstance/QString/QMap/QImage
// throughout, and ks::engine::graphics::RenderSystem (the class that used to
// stand in for "the frame loop" in SimulationLoop::tick()) is a QObject too
// — and was, on inspection, an empty stub: its beginFrame()/endFrame() never
// recorded or submitted a single Vulkan command. Both of those are real
// blockers for "SimulatorApp must start without Qt", independent of any
// feature work (shadows, bloom, ...) built on top of them.
//
// NativeRenderer is a from-scratch replacement for the *subset* SimulatorApp
// actually needs: device/swapchain management (device/surface are created by
// SimulatorApp.cpp itself already, with raw vkCreateInstance/vkCreateWin32SurfaceKHR),
// a real per-frame command buffer with an actual render pass, and mesh
// storage using std::string/std::vector instead of QString/QVector/QMap.
//
// What this does NOT yet solve: SimulationLoop::loadTrack()/loadCar() build
// their scene meshes by parsing .kn5 files through KN5Parser, which returns
// Qt-typed data (QVector<SceneVertex> etc.) and uses QString::fromStdString
// internally. That is a second, separate Qt dependency in the content
// pipeline, not the renderer — rewriting KN5Parser itself is out of scope
// here (it's shared with the Qt editor and touching it blind, without a way
// to compile-test, risks breaking real AC content loading). The intended
// shape of the fix is an offline "bake" step: the Qt-based editor converts
// .kn5 -> a simple Qt-free mesh cache (see NativeMesh::loadFromFile below),
// and SimulatorApp only ever reads the baked format at runtime, never KN5Parser
// or QString. NativeRenderer::loadMeshFromFile() already speaks that baked format.
//
// A second producer of the same .nmsh format exists: ks::engine::terrain
// (src/engine/terrain/TerrainMesh.h) generates real triangle meshes from
// TrackTerrainEditor's heightmap data and can write them straight to .nmsh
// via TrackTerrainEditor::exportForSimulator() — so hand-edited terrain
// reaches SimulatorApp through this exact same loading path as baked KN5
// content, no separate runtime format needed.

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <memory>
#include "MathTypes.h"

namespace ks::sim {

class CascadedShadowMap;

namespace ui {
class UiGpuPass;
class UiRenderer;
}

// Matches ks::VulkanRenderer::Vertex's layout (position, normal, uv, color =
// 3+3+2+4 floats) so baked meshes and shadow-pass vertex binding stay
// compatible with the existing shader set (pbr.vert, ksShadow.vert).
struct NativeVertex {
    float px = 0, py = 0, pz = 0;
    float nx = 0, ny = 0, nz = 0;
    float u = 0, v = 0;
    float r = 1, g = 1, b = 1, a = 1;
};
static_assert(sizeof(NativeVertex) == sizeof(float) * 12, "NativeVertex must stay tightly packed to match the shadow pipeline's vertex stride");

struct NativeMesh {
    std::vector<NativeVertex> vertices;
    std::vector<uint32_t> indices;
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexMemory = VK_NULL_HANDLE;
};

struct DirectionalLight {
    vec3 direction{0.3f, -0.8f, 0.2f};
    vec3 color{1.0f, 0.95f, 0.9f};
    float intensity = 1.0f;
};

class NativeRenderer {
public:
    ~NativeRenderer();
    NativeRenderer() = default;
    NativeRenderer(const NativeRenderer&) = delete;
    NativeRenderer& operator=(const NativeRenderer&) = delete;

    // instance/surface are created by the caller (SimulatorApp.cpp already
    // does this with raw vkCreateInstance/vkCreateWin32SurfaceKHR).
    bool createDevice(VkInstance instance, VkSurfaceKHR surface);
    bool createSwapChain(VkSurfaceKHR surface, uint32_t width, uint32_t height);
    bool recreateSwapChain(uint32_t width, uint32_t height);
    // Depth-only shadow cascades are wired in here if a shadow map is
    // attached (see attachShadowMap()) — geometry submitted via drawMesh()
    // between beginFrame()/endFrame() is automatically also rendered into
    // each active cascade before the main color pass.
    void attachShadowMap(CascadedShadowMap* shadowMap) { m_shadowMap = shadowMap; }

    bool isInitialized() const { return m_device != VK_NULL_HANDLE; }
    void shutdown();

    // shaderDir must contain precompiled pbr.vert.spv/pbr.frag.spv (or a
    // simpler forward-lit fallback — see NativeRenderer.cpp) and
    // ksShadow.vert.spv/ksShadow.frag.spv if a shadow map is attached.
    bool loadPipelines(const std::string& shaderDir);

    // Loads a pre-baked, Qt-free mesh cache (see the file note above for why
    // this exists instead of parsing .kn5 directly). Format: a tiny custom
    // binary — 4-byte magic "NMSH", uint32 vertexCount, uint32 indexCount,
    // then the raw NativeVertex array, then the raw uint32 index array.
    bool loadMeshFromFile(const std::string& name, const std::string& path);
    // Reads <dir>/manifest.txt (one mesh name per line, as written by the
    // kn5baker tool) and calls loadMeshFromFile(name, dir+"/"+name+".nmsh")
    // for each. Returns the number of meshes successfully loaded. Meshes
    // loaded this way are remembered as "static scene" meshes — see
    // drawStaticScene() — since baked track/prop geometry is already in
    // world space (kn5.worldMatrix was applied at bake time) and should be
    // drawn at identity every frame, unlike instanced meshes (e.g. a car)
    // that need a per-frame model matrix from drawMesh().
    int loadMeshesFromManifest(const std::string& dir);
    // Same as setMesh() but also registers the mesh as static-scene geometry,
    // so drawStaticScene() submits it every frame at the identity transform.
    // Used by SimulationLoop when it swaps in a freshly parsed .kn5 track.
    void setStaticMesh(const std::string& name, const NativeMesh& mesh);
    // Destroys every static-scene mesh (their GPU buffers included) and
    // forgets them. Called before uploading a newly loaded track so the old
    // layout does not linger on top of the new one. Instanced meshes such as
    // "player_car" are left untouched.
    void clearStaticScene();
    // Queues every mesh loaded via loadMeshesFromManifest() at the identity
    // transform. Call once per frame, before or after any drawMesh() calls
    // for instanced objects.
    void drawStaticScene();
    void setMesh(const std::string& name, const NativeMesh& mesh);
    void destroyMesh(const std::string& name);

    // near/far are forwarded to the shadow cascade split computation so the
    // cascades cover exactly the depth range the camera renders.
    void setCamera(const mat4& view, const mat4& proj,
                   float nearPlane = 0.5f, float farPlane = 500.0f) {
        m_view = view;
        m_proj = proj;
        m_camNear = nearPlane;
        m_camFar = farPlane;
        mat4 invView = view.inverse();
        m_camPosWS = vec3(invView(0, 3), invView(1, 3), invView(2, 3));
    }
    void setSun(const DirectionalLight& light) { m_sun = light; }

    // Deferred lighting path. Off by default: endFrame() keeps running the
    // exact forward pipeline it always has until this is turned on, so
    // enabling it is a pure opt-in and cannot regress the default image.
    // When on, the draw list goes through a GBuffer MRT (albedo / normal /
    // world position) and a fullscreen lighting + volumetric fog pass
    // instead. Resources are built lazily on the first frame that needs
    // them and torn down with the swapchain on resize.
    void setDeferred(bool enabled) { m_deferred = enabled; }
    bool isDeferred() const { return m_deferred; }

    // Temporal AA. Implies the deferred path (the resolve pass needs the
    // GBuffer world positions to reproject), so setTaa(true) alone is enough
    // to get deferred + TAA: the main-pass projection is jittered by a
    // Halton(2,3) sub-pixel offset and the lighting result is blended
    // against the reprojected previous frame.
    void setTaa(bool enabled) { m_taa = enabled; }
    bool isTaa() const { return m_taa; }

    // Real frame lifecycle. beginFrame() acquires a swapchain image and
    // resets the per-frame draw list; drawMesh() *queues* a (mesh, model
    // matrix) pair rather than drawing immediately, because the shadow
    // cascades and the main color pass both need to render the exact same
    // set of instances — recording them once and replaying the list twice
    // (once per shadow cascade, once for the main pass) is what endFrame()
    // does. Submission/present happens at the end of endFrame().
    bool beginFrame();
    void drawMesh(const std::string& name, const mat4& modelMatrix);
    void endFrame();

    // UI overlay GPU pass. SimulationLoop owns the pass (it builds the font
    // atlas + per-frame dynamic VB/IB); NativeRenderer keeps it alive and
    // pumps a frame through it while its render pass is open.
    void setUiGpuPass(std::shared_ptr<ui::UiGpuPass> pass) { m_uiPass = std::move(pass); }
    ui::UiGpuPass* uiGpuPass() { return m_uiPass.get(); }
    void drawUi(const ui::UiRenderer& ui);

    // Viewport hint from SimulationLoop::setViewportSize(). The swapchain
    // itself is recreated by SimulatorApp's WM_SIZE handler.
    void resize(int width, int height) {
        if (width > 0) m_viewportW = width;
        if (height > 0) m_viewportH = height;
    }
    int width() const { return m_viewportW; }
    int height() const { return m_viewportH; }

    VkPhysicalDevice physicalDevice() const { return m_physicalDevice; }
    VkDevice device() const { return m_device; }
    VkQueue graphicsQueue() const { return m_graphicsQueue; }
    VkCommandPool commandPool() const { return m_commandPool; }
    VkCommandBuffer currentCommandBuffer() const { return m_commandBuffer; }

private:
    bool createCommandPoolAndBuffer();
    bool createSyncObjects();
    bool createDepthResources(uint32_t width, uint32_t height);
    void destroySwapChain();
    void uploadMesh(NativeMesh& mesh);

    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    uint32_t m_graphicsQueueFamily = 0;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;

    VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
    VkFormat m_swapChainFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkExtent2D m_swapChainExtent{};
    std::vector<VkImage> m_swapChainImages;
    std::vector<VkImageView> m_swapChainImageViews;
    std::vector<VkFramebuffer> m_swapChainFramebuffers;
    VkRenderPass m_renderPass = VK_NULL_HANDLE;

    VkImage m_depthImage = VK_NULL_HANDLE;
    VkDeviceMemory m_depthMemory = VK_NULL_HANDLE;
    VkImageView m_depthView = VK_NULL_HANDLE;

    VkSemaphore m_imageAvailable = VK_NULL_HANDLE;
    VkSemaphore m_renderFinished = VK_NULL_HANDLE;
    VkFence m_inFlightFence = VK_NULL_HANDLE;
    uint32_t m_currentImageIndex = 0;

    VkShaderModule m_vertModule = VK_NULL_HANDLE;
    VkShaderModule m_fragModule = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;

    // Descriptor set 0: FrameData UBO (sun + cascade matrices) + shadow
    // cascade array sampler — matches native_forward.frag exactly. Written
    // once per frame in endFrame() before the main draw list, since the
    // cascade matrices are only known after CascadedShadowMap::update() runs.
    VkDescriptorSetLayout m_frameSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet m_frameSet = VK_NULL_HANDLE;
    VkBuffer m_frameUBO = VK_NULL_HANDLE;
    VkDeviceMemory m_frameUBOMemory = VK_NULL_HANDLE;
    void* m_frameUBOMapped = nullptr;

    // 1x1 white shadow-cascade-array stand-in, bound when no CascadedShadowMap
    // is attached, so the descriptor set is always valid to bind (the shader
    // always declares the binding; this just makes every fragment "lit").
    VkImage m_dummyShadowImage = VK_NULL_HANDLE;
    VkDeviceMemory m_dummyShadowMemory = VK_NULL_HANDLE;
    VkImageView m_dummyShadowView = VK_NULL_HANDLE;
    VkSampler m_dummyShadowSampler = VK_NULL_HANDLE;

    bool createFrameDescriptorResources();
    bool createDummyShadowTexture();
    void writeFrameDescriptorSet(VkImageView shadowView, VkSampler shadowSampler);

    // --- Deferred path (opt-in, setDeferred / setTaa) ---
    bool ensureDeferredResources();
    void destroyDeferredResources();
    void writeLightingDescriptorSet(VkImageView shadowView, VkSampler shadowSampler);
    void writeResolveDescriptorSet();

    static constexpr int kGBufferCount = 3;
    static constexpr int kHistoryCount = 2;
    static constexpr int kJitterSamples = 8;
    static constexpr float kTaaFeedback = 0.9f;

    bool m_deferred = false;
    bool m_taa = false;
    bool m_deferredReady = false;
    bool m_deferredFailed = false;
    bool m_historyValid = false;
    bool m_prevViewProjValid = false;
    int m_jitterIndex = 0;
    int m_historyIndex = 0;
    mat4 m_prevViewProj;
    std::string m_shaderDir;

    VkFormat m_gbufferFormats[kGBufferCount] = {
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_FORMAT_R16G16B16A16_SFLOAT,
    };
    VkImage m_gbufferImages[kGBufferCount]{};
    VkDeviceMemory m_gbufferMemory[kGBufferCount]{};
    VkImageView m_gbufferViews[kGBufferCount]{};
    VkImage m_gbufferDepthImage = VK_NULL_HANDLE;
    VkDeviceMemory m_gbufferDepthMemory = VK_NULL_HANDLE;
    VkImageView m_gbufferDepthView = VK_NULL_HANDLE;

    // Lighting output, sampled by the resolve pass, plus two ping-ponged
    // temporal history targets (RGBA16F so accumulated light survives).
    VkImage m_hdrImage = VK_NULL_HANDLE;
    VkDeviceMemory m_hdrMemory = VK_NULL_HANDLE;
    VkImageView m_hdrView = VK_NULL_HANDLE;
    VkImage m_historyImages[kHistoryCount]{};
    VkDeviceMemory m_historyMemory[kHistoryCount]{};
    VkImageView m_historyViews[kHistoryCount]{};

    VkRenderPass m_gbufferRenderPass = VK_NULL_HANDLE;
    VkRenderPass m_lightingRenderPass = VK_NULL_HANDLE;
    VkRenderPass m_resolveRenderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> m_gbufferFramebuffers;
    VkFramebuffer m_lightingFramebuffer = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> m_resolveFramebuffers;

    VkDescriptorSetLayout m_lightingSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_lightingPool = VK_NULL_HANDLE;
    VkDescriptorSet m_lightingSet = VK_NULL_HANDLE;
    VkSampler m_gbufferSampler = VK_NULL_HANDLE;
    VkPipelineLayout m_lightingLayout = VK_NULL_HANDLE;
    VkPipeline m_gbufferPipeline = VK_NULL_HANDLE;
    VkPipeline m_lightingPipeline = VK_NULL_HANDLE;
    VkShaderModule m_gbufferVertModule = VK_NULL_HANDLE;
    VkShaderModule m_gbufferFragModule = VK_NULL_HANDLE;
    VkShaderModule m_lightingVertModule = VK_NULL_HANDLE;
    VkShaderModule m_lightingFragModule = VK_NULL_HANDLE;

    VkDescriptorSetLayout m_resolveSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_resolvePool = VK_NULL_HANDLE;
    VkDescriptorSet m_resolveSet = VK_NULL_HANDLE;
    VkSampler m_hdrSampler = VK_NULL_HANDLE;
    VkPipelineLayout m_resolveLayout = VK_NULL_HANDLE;
    VkPipeline m_resolvePipeline = VK_NULL_HANDLE;
    VkShaderModule m_resolveFragModule = VK_NULL_HANDLE;

    std::unordered_map<std::string, NativeMesh> m_meshes;
    std::vector<std::string> m_staticSceneMeshNames;

    struct QueuedDraw { std::string meshName; mat4 model; };
    std::vector<QueuedDraw> m_drawList;

    mat4 m_view;
    mat4 m_proj;
    vec3 m_camPosWS;
    float m_camNear = 0.5f;
    float m_camFar = 500.0f;
    DirectionalLight m_sun;
    CascadedShadowMap* m_shadowMap = nullptr;

    std::shared_ptr<ui::UiGpuPass> m_uiPass;
    int m_viewportW = 1280;
    int m_viewportH = 720;
};

} // namespace ks::sim
