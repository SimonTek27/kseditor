#include "SSGIRenderer.h"
#include "VulkanFunctions.h"
#include "VulkanRenderer.h"
#include "RenderGraph.h"
#include <QDebug>

namespace ks {

const char* SSGIRenderer::kGgiRawName = "ssgi_raw";
const char* SSGIRenderer::kGgiOutputName = "ssgi_output";

SSGIRenderer::SSGIRenderer(VulkanRenderer* renderer)
    : m_renderer(renderer)
{
}

SSGIRenderer::~SSGIRenderer() {
    destroy();
}

bool SSGIRenderer::initialize(VkExtent2D resolution) {
    m_device = m_renderer->device();
    m_physicalDevice = m_renderer->physicalDevice();
    m_computeQueue = m_renderer->computeQueue();

    if (!createResources(resolution)) {
        qWarning() << "SSGI: Failed to create resources";
        return false;
    }

    if (!createPipelines()) {
        qWarning() << "SSGI: Failed to create pipelines";
        releaseResources();
        return false;
    }

    m_resolution = resolution;
    qInfo() << "SSGI initialized at resolution" << m_resolution.width << "x" << m_resolution.height;
    return true;
}

void SSGIRenderer::destroy() {
    releaseResources();
}

void SSGIRenderer::resize(VkExtent2D resolution) {
    releaseResources();
    if (!createResources(resolution)) {
        qWarning() << "SSGI: Failed to recreate resources after resize";
        return;
    }
    m_resolution = resolution;
}

void SSGIRenderer::recordPasses(RenderGraph* graph, const QMatrix4x4& view,
                               const QMatrix4x4& proj, const QVector3D& cameraPos) {
    if (!graph || !m_ssgiPipeline || !m_blurPipeline) return;

    // Build push constants for SSGI compute shader
    SSGIParams paramsCopy = m_params;
    paramsCopy.frameIndex++; // Increment for temporal accumulation

    // We'll set the params via the pipeline's setUniform or directly in the pass
    // For now, record the passes - the actual descriptor binding will happen via the render graph

    // 1. Add SSGI compute pass
    // Inputs: depth, normal, albedo (these should already be attached to the graph)
    // Output: giRaw

    RenderGraph::Pass ssgiPass;
    ssgiPass.name = "ssgi_compute";
    ssgiPass.isCompute = true;

    // Input resources - these must exist in the graph already
    ssgiPass.inputs = {
        std::string(kGgiRawName), // Will be resolved by name
        // Actually we need to reference the actual G-buffer textures
        // For now, use placeholder names that the user will connect
    };
    // Output
    ssgiPass.outputs = { kGgiRawName };

    // We need to set up the compute pipeline and descriptor sets
    // The render graph will handle binding based on our specifications

    graph->addPass(ssgiPass);

    // 2. Add SSGI blur pass
    RenderGraph::Pass blurPass;
    blurPass.name = "ssgi_blur";
    blurPass.isCompute = true;
    blurPass.inputs = { kGgiRawName };
    blurPass.outputs = { kGgiOutputName };

    graph->addPass(blurPass);
}

void SSGIRenderer::createPipelines() {
    // SSGI compute pipeline
    m_ssgiPipeline = std::make_unique<ComputePipeline>(m_renderer);

    // We need to set up the descriptor bindings for the SSGI shader
    // The SSGI shader expects:
    // Set 0: Bindings 0=depth, 1=normal, 2=albedo, 3=output
    // Push constants: SSGIParams

    // For now, we'll configure minimal bindings and let the user expand
    // The ComputePipeline base class will handle the actual descriptor writing

    // Initialize SSGI pipeline with shader path
    QString ssgiShaderPath = QString("%1/shaders/ssgi.comp.glsl")
        .arg(m_renderer->shaderDir());

    if (!m_ssgiPipeline->initialize(ssgiShaderPath)) {
        qWarning() << "SSGI: Failed to initialize SSGI pipeline";
    }

    // Blur pipeline
    m_blurPipeline = std::make_unique<ComputePipeline>(m_renderer);

    QString blurShaderPath = QString("%1/shaders/ssgi_blur.comp.glsl")
        .arg(m_renderer->shaderDir());

    if (!m_blurPipeline->initialize(blurShaderPath)) {
        qWarning() << "SSGI: Failed to initialize blur pipeline";
    }
}

bool SSGIRenderer::createResources(VkExtent2D resolution) {
    VkDevice device = m_device;
    VkPhysicalDevice physDev = m_physicalDevice;

    // Use half resolution for performance
    VkExtent2D halfRes = { resolution.width / 2, resolution.height / 2 };

    VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;

    // Create raw GI image
    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent = { halfRes.width, halfRes.height, 1 };
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.format = format;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    VkResult result = g_vk.createImage(device, &imageCreateInfo, nullptr, &m_giRawImage);
    if (result != VK_SUCCESS) {
        qWarning() << "SSGI: Failed to create GI raw image:" << result;
        return false;
    }

    // Allocate memory
    VkMemoryRequirements memReqs;
    g_vk.getImageMemoryRequirements(device, m_giRawImage, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = m_renderer->findMemoryType(memReqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    result = g_vk.allocateMemory(device, &allocInfo, nullptr, &m_giRawMemory);
    if (result != VK_SUCCESS) {
        qWarning() << "SSGI: Failed to allocate GI raw image memory:" << result;
        return false;
    }

    result = g_vk.bindImageMemory(device, m_giRawImage, m_giRawMemory);
    if (result != VK_SUCCESS) {
        qWarning() << "SSGI: Failed to bind GI raw image memory:" << result;
        return false;
    }

    // Create image view for raw GI
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_giRawImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    result = g_vk.createImageView(device, &viewInfo, nullptr, &m_giRawView);
    if (result != VK_SUCCESS) {
        qWarning() << "SSGI: Failed to create GI raw image view:" << result;
        return false;
    }

    // Create output GI image (filtered result)
    result = g_vk.createImage(device, &imageCreateInfo, nullptr, &m_giOutputImage);
    if (result != VK_SUCCESS) {
        qWarning() << "SSGI: Failed to create GI output image:" << result;
        return false;
    }

    result = g_vk.allocateMemory(device, &allocInfo, nullptr, &m_giOutputMemory);
    if (result != VK_SUCCESS) {
        qWarning() << "SSGI: Failed to allocate GI output image memory:" << result;
        return false;
    }

    result = g_vk.bindImageMemory(device, m_giOutputImage, m_giOutputMemory);
    if (result != VK_SUCCESS) {
        qWarning() << "SSGI: Failed to bind GI output image memory:" << result;
        return false;
    }

    // Create output image view
    result = g_vk.createImageView(device, &viewInfo, nullptr, &m_giOutputView);
    if (result != VK_SUCCESS) {
        qWarning() << "SSGI: Failed to create GI output image view:" << result;
        return false;
    }

    // Create sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    result = g_vk.createSampler(device, &samplerInfo, nullptr, &m_sampler);
    if (result != VK_SUCCESS) {
        qWarning() << "SSGI: Failed to create sampler:" << result;
        return false;
    }

    qInfo() << "SSGI resources created at half-resolution" << halfRes.width << "x" << halfRes.height;
    return true;
}

void SSGIRenderer::releaseResources() {
    if (m_device == VK_NULL_HANDLE) return;

    VkDevice device = m_device;

    // Free sampler
    if (m_sampler != VK_NULL_HANDLE) {
        g_vk.destroySampler(device, m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }

    // Free output view
    if (m_giOutputView != VK_NULL_HANDLE) {
        g_vk.destroyImageView(device, m_giOutputView, nullptr);
        m_giOutputView = VK_NULL_HANDLE;
    }

    // Free output image memory
    if (m_giOutputImage != VK_NULL_HANDLE) {
        g_vk.destroyImage(device, m_giOutputImage, nullptr);
        m_giOutputImage = VK_NULL_HANDLE;
    }
    if (m_giOutputMemory != VK_NULL_HANDLE) {
        g_vk.freeMemory(device, m_giOutputMemory, nullptr);
        m_giOutputMemory = VK_NULL_HANDLE;
    }

    // Free raw view
    if (m_giRawView != VK_NULL_HANDLE) {
        g_vk.destroyImageView(device, m_giRawView, nullptr);
        m_giRawView = VK_NULL_HANDLE;
    }

    // Free raw image memory
    if (m_giRawImage != VK_NULL_HANDLE) {
        g_vk.destroyImage(device, m_giRawImage, nullptr);
        m_giRawImage = VK_NULL_HANDLE;
    }
    if (m_giRawMemory != VK_NULL_HANDLE) {
        g_vk.freeMemory(device, m_giRawMemory, nullptr);
        m_giRawMemory = VK_NULL_HANDLE;
    }

    m_resolution = {0, 0};
}
} // namespace ks