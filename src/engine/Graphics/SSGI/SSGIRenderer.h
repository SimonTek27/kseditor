#pragma once
#include "../ComputePipeline.h"
#include <QMatrix4x4>
#include <QVector3D>
#include <QVariant>

namespace ks {

class VulkanRenderer;
class RenderGraph;

struct SSGIParams {
    float radius = 3.0f;           // World-space radius for GI sampling
    float intensity = 1.0f;        // GI contribution multiplier
    float bias = 0.01f;            // Depth bias to avoid self-intersection
    float maxDistance = 10.0f;     // Maximum ray march distance
    int frameIndex = 0;            // For temporal noise
    float temporalBlend = 0.1f;    // Temporal accumulation factor
};

class SSGIRenderer {
public:
    SSGIRenderer(VulkanRenderer* renderer);
    ~SSGIRenderer();

    bool initialize(VkExtent2D resolution);
    void destroy();

    // Resize when viewport changes
    void resize(VkExtent2D resolution);

    // Record SSGI compute passes into render graph
    void recordPasses(RenderGraph* graph, const QMatrix4x4& view,
                     const QMatrix4x4& proj, const QVector3D& cameraPos);

    // Get the final SSGI output texture for PBR integration
    VkImageView getGIOutput() const { return m_giOutputView; }

    void setParams(const SSGIParams& params) { m_params = params; }
    const SSGIParams& params() const { return m_params; }

private:
    bool createResources(VkExtent2D resolution);
    void createPipelines();
    void releaseResources();

    VulkanRenderer* m_renderer = nullptr;
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkQueue m_computeQueue = VK_NULL_HANDLE;
    VkExtent2D m_resolution = {0, 0};
    SSGIParams m_params;

    // Compute pipelines
    std::unique_ptr<ComputePipeline> m_ssgiPipeline;
    std::unique_ptr<ComputePipeline> m_blurPipeline;

    // Intermediate textures - raw GI (half resolution)
    VkImage m_giRawImage = VK_NULL_HANDLE;
    VkDeviceMemory m_giRawMemory = VK_NULL_HANDLE;
    VkImageView m_giRawView = VK_NULL_HANDLE;

    // Output texture (half resolution, filtered)
    VkImage m_giOutputImage = VK_NULL_HANDLE;
    VkDeviceMemory m_giOutputMemory = VK_NULL_HANDLE;
    VkImageView m_giOutputView = VK_NULL_HANDLE;

    // Sampler for GI texture
    VkSampler m_sampler = VK_NULL_HANDLE;

    // Render graph resource names
    static const char* kGgiRawName;
    static const char* kGgiOutputName;
};

} // namespace ks