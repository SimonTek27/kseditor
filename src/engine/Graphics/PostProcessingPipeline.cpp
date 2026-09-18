#include "PostProcessingPipeline.h"
#include <QDebug>
#include <QtMath>
#include <algorithm>
#include <random>

namespace ks::engine::graphics {

bool PostProcessingPipeline::initialize() {
    if (m_initialized) return true;
    m_initialized = true;
    qInfo() << "PostProcessingPipeline: initialized";
    return true;
}

void PostProcessingPipeline::shutdown() {
    if (!m_initialized) return;
    m_ssaoKernel.clear();
    m_bloomDownsample.clear();
    m_bloomDownsampleViews.clear();
    m_bloomUpsample.clear();
    m_bloomUpsampleViews.clear();
    m_gbufferFramebuffers.clear();
    m_initialized = false;
    qInfo() << "PostProcessingPipeline: shutdown";
}

void PostProcessingPipeline::configure(const PostProcessingConfig& config) {
    m_config = config;
    emit configChanged();
}

void PostProcessingPipeline::createGBuffer(VkDevice device, VkPhysicalDevice physDev, uint32_t width, uint32_t height) {
    auto createImage = [&](VkFormat format, VkImageUsageFlags usage, VkImage& image, VkImageView& view) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = {width, height, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateImage(device, &imageInfo, nullptr, &image);
        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(device, image, &memReqs);
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(physDev, &memProps);
        uint32_t memType = UINT32_MAX;
        for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
            if ((memReqs.memoryTypeBits & (1 << i)) &&
                (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
                memType = i;
                break;
            }
        }
        if (memType == UINT32_MAX) { memType = 0; }
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = memType;
        VkDeviceMemory memory;
        vkAllocateMemory(device, &allocInfo, nullptr, &memory);
        vkBindImageMemory(device, image, memory, 0);
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(device, &viewInfo, nullptr, &view);
    };

    createImage(m_gbuffer.albedoFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                m_gbuffer.albedo, m_gbuffer.albedoView);
    createImage(m_gbuffer.normalFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                m_gbuffer.normal, m_gbuffer.normalView);
    createImage(m_gbuffer.materialFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                m_gbuffer.materialParams, m_gbuffer.materialParamsView);
    createImage(m_gbuffer.depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                m_gbuffer.depth, m_gbuffer.depthView);
    createImage(m_gbuffer.velocityFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                m_gbuffer.velocity, m_gbuffer.velocityView);

    createSSAOKernel();
    createNoiseTexture(device, physDev);
    createBloomDownsampleTargets(device, physDev, width, height);
    createBloomUpsampleTargets(device, physDev, width, height);

    emit gbufferCreated();
    qInfo() << "PostProcessingPipeline: GBuffer created" << width << "x" << height;
}

void PostProcessingPipeline::destroyGBuffer(VkDevice device) {
    auto destroyImage = [&](VkImage& image, VkImageView& view) {
        if (view) { vkDestroyImageView(device, view, nullptr); view = VK_NULL_HANDLE; }
        if (image) { vkDestroyImage(device, image, nullptr); image = VK_NULL_HANDLE; }
    };
    destroyImage(m_gbuffer.albedo, m_gbuffer.albedoView);
    destroyImage(m_gbuffer.normal, m_gbuffer.normalView);
    destroyImage(m_gbuffer.materialParams, m_gbuffer.materialParamsView);
    destroyImage(m_gbuffer.depth, m_gbuffer.depthView);
    destroyImage(m_gbuffer.velocity, m_gbuffer.velocityView);
    destroyImage(m_ssaoNoise, m_ssaoNoiseView);
    for (auto& v : m_bloomDownsampleViews) if (v) vkDestroyImageView(device, v, nullptr);
    for (auto& i : m_bloomDownsample) if (i) vkDestroyImage(device, i, nullptr);
    for (auto& v : m_bloomUpsampleViews) if (v) vkDestroyImageView(device, v, nullptr);
    for (auto& i : m_bloomUpsample) if (i) vkDestroyImage(device, i, nullptr);
    m_bloomDownsample.clear();
    m_bloomDownsampleViews.clear();
    m_bloomUpsample.clear();
    m_bloomUpsampleViews.clear();
    emit gbufferDestroyed();
}

void PostProcessingPipeline::beginGBufferPass(VkCommandBuffer cmd) {
}

void PostProcessingPipeline::endGBufferPass(VkCommandBuffer cmd) {
}

void PostProcessingPipeline::executeSSAO(VkCommandBuffer cmd) {
    if (!m_config.ssao.enabled) return;
}

void PostProcessingPipeline::executeBloom(VkCommandBuffer cmd) {
    if (!m_config.bloom.enabled) return;
}

void PostProcessingPipeline::executeMotionBlur(VkCommandBuffer cmd) {
    if (!m_config.motionBlur.enabled) return;
}

void PostProcessingPipeline::executeToneMapping(VkCommandBuffer cmd) {
}

void PostProcessingPipeline::executeDOF(VkCommandBuffer cmd) {
    if (!m_config.dof.enabled) return;
}

void PostProcessingPipeline::executeFXAA(VkCommandBuffer cmd) {
    if (!m_config.fxaa.enabled) return;
}

void PostProcessingPipeline::executeColorGrading(VkCommandBuffer cmd) {
    if (!m_config.colorGrading.enabled) return;
}

void PostProcessingPipeline::executePostProcess(VkCommandBuffer cmd) {
    executeSSAO(cmd);
    executeDOF(cmd);
    executeBloom(cmd);
    executeMotionBlur(cmd);
    executeColorGrading(cmd);
    executeToneMapping(cmd);
    executeFXAA(cmd);
}

void PostProcessingPipeline::createNoiseTexture(VkDevice device, VkPhysicalDevice physDev) {
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    QVector<QVector4D> noiseData;
    for (int i = 0; i < 16; ++i) {
        noiseData.append(QVector4D(dist(rng), dist(rng), 0.0f, 0.0f));
    }
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = {4, 4, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    vkCreateImage(device, &imageInfo, nullptr, &m_ssaoNoise);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_ssaoNoise;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    vkCreateImageView(device, &viewInfo, nullptr, &m_ssaoNoiseView);
}

void PostProcessingPipeline::createSSAOKernel() {
    m_ssaoKernel.clear();
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    std::uniform_real_distribution<float> distAbs(0.0f, 1.0f);
    for (int i = 0; i < m_config.ssao.kernelSize; ++i) {
        QVector3D sample(dist(rng), dist(rng), distAbs(rng));
        sample.normalize();
        sample *= distAbs(rng);
        float scale = float(i) / float(m_config.ssao.kernelSize);
        scale = 0.1f + scale * scale * 0.9f;
        sample *= scale;
        m_ssaoKernel.append(sample);
    }
}

void PostProcessingPipeline::createBloomDownsampleTargets(VkDevice device, VkPhysicalDevice physDev, uint32_t width, uint32_t height) {
    m_bloomDownsample.clear();
    m_bloomDownsampleViews.clear();
    uint32_t mipW = width / 2;
    uint32_t mipH = height / 2;
    for (int i = 0; i < m_config.bloom.mipLevels && mipW >= 2 && mipH >= 2; ++i) {
        VkImage image = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = {mipW, mipH, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        vkCreateImage(device, &imageInfo, nullptr, &image);
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(device, &viewInfo, nullptr, &view);
        m_bloomDownsample.append(image);
        m_bloomDownsampleViews.append(view);
        mipW /= 2;
        mipH /= 2;
    }
}

void PostProcessingPipeline::createBloomUpsampleTargets(VkDevice device, VkPhysicalDevice physDev, uint32_t width, uint32_t height) {
    m_bloomUpsample.clear();
    m_bloomUpsampleViews.clear();
    int downsampleCount = m_bloomDownsample.size();
    for (int i = downsampleCount - 2; i >= 0; --i) {
        VkImage image = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = {(uint32_t)width, (uint32_t)height, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        vkCreateImage(device, &imageInfo, nullptr, &image);
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(device, &viewInfo, nullptr, &view);
        m_bloomUpsample.append(image);
        m_bloomUpsampleViews.append(view);
        width /= 2;
        height /= 2;
    }
}

} // namespace ks::engine::graphics
