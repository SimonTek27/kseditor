#include "SSRSystem.h"
#include <QDebug>

namespace ks::engine::graphics {

bool SSRSystem::initialize() {
    if (m_initialized) return true;
    m_initialized = true;
    qInfo() << "SSRSystem: initialized";
    return true;
}

void SSRSystem::shutdown() {
    if (!m_initialized) return;
    m_initialized = false;
    qInfo() << "SSRSystem: shutdown";
}

void SSRSystem::configure(const SSRConfig& config) {
    m_config = config;
    emit configChanged();
}

void SSRSystem::createResources(VkDevice device, VkPhysicalDevice physDev, uint32_t width, uint32_t height) {
    m_width = width;
    m_height = height;
    createReflectionTarget(device, physDev, width, height);
    createHistoryTarget(device, physDev, width, height);
    createEdgeDetectTarget(device, physDev, width, height);
    createBlurTarget(device, physDev, width, height);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    vkCreateSampler(device, &samplerInfo, nullptr, &m_sampler);

    emit resourcesCreated();
    qInfo() << "SSRSystem: resources created" << width << "x" << height;
}

void SSRSystem::destroyResources(VkDevice device) {
    if (m_sampler) { vkDestroySampler(device, m_sampler, nullptr); m_sampler = VK_NULL_HANDLE; }
    if (m_reflectionView) { vkDestroyImageView(device, m_reflectionView, nullptr); m_reflectionView = VK_NULL_HANDLE; }
    if (m_reflectionImage) { vkDestroyImage(device, m_reflectionImage, nullptr); m_reflectionImage = VK_NULL_HANDLE; }
    if (m_reflectionMemory) { vkFreeMemory(device, m_reflectionMemory, nullptr); m_reflectionMemory = VK_NULL_HANDLE; }
    if (m_historyView) { vkDestroyImageView(device, m_historyView, nullptr); m_historyView = VK_NULL_HANDLE; }
    if (m_historyImage) { vkDestroyImage(device, m_historyImage, nullptr); m_historyImage = VK_NULL_HANDLE; }
    if (m_historyMemory) { vkFreeMemory(device, m_historyMemory, nullptr); m_historyMemory = VK_NULL_HANDLE; }
    if (m_edgeDetectView) { vkDestroyImageView(device, m_edgeDetectView, nullptr); m_edgeDetectView = VK_NULL_HANDLE; }
    if (m_edgeDetectImage) { vkDestroyImage(device, m_edgeDetectImage, nullptr); m_edgeDetectImage = VK_NULL_HANDLE; }
    if (m_edgeDetectMemory) { vkFreeMemory(device, m_edgeDetectMemory, nullptr); m_edgeDetectMemory = VK_NULL_HANDLE; }
    if (m_blurView) { vkDestroyImageView(device, m_blurView, nullptr); m_blurView = VK_NULL_HANDLE; }
    if (m_blurImage) { vkDestroyImage(device, m_blurImage, nullptr); m_blurImage = VK_NULL_HANDLE; }
    if (m_blurMemory) { vkFreeMemory(device, m_blurMemory, nullptr); m_blurMemory = VK_NULL_HANDLE; }
    emit resourcesDestroyed();
}

void SSRSystem::execute(VkCommandBuffer cmd, VkImageView colorBuffer, VkImageView normalBuffer,
                         VkImageView depthBuffer, VkImageView materialBuffer,
                         const QMatrix4x4& viewMatrix, const QMatrix4x4& projMatrix,
                         const QMatrix4x4& prevViewMatrix, const QMatrix4x4& prevProjMatrix) {
    if (!m_config.enabled) return;
}

void SSRSystem::setResolution(uint32_t width, uint32_t height) {
    m_width = width;
    m_height = height;
}

void SSRSystem::createReflectionTarget(VkDevice device, VkPhysicalDevice physDev, uint32_t width, uint32_t height) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = {width, height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    vkCreateImage(device, &imageInfo, nullptr, &m_reflectionImage);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_reflectionImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    vkCreateImageView(device, &viewInfo, nullptr, &m_reflectionView);
}

void SSRSystem::createHistoryTarget(VkDevice device, VkPhysicalDevice physDev, uint32_t width, uint32_t height) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = {width, height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    vkCreateImage(device, &imageInfo, nullptr, &m_historyImage);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_historyImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    vkCreateImageView(device, &viewInfo, nullptr, &m_historyView);
}

void SSRSystem::createEdgeDetectTarget(VkDevice device, VkPhysicalDevice physDev, uint32_t width, uint32_t height) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = {width, height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    vkCreateImage(device, &imageInfo, nullptr, &m_edgeDetectImage);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_edgeDetectImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    vkCreateImageView(device, &viewInfo, nullptr, &m_edgeDetectView);
}

void SSRSystem::createBlurTarget(VkDevice device, VkPhysicalDevice physDev, uint32_t width, uint32_t height) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = {width, height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    vkCreateImage(device, &imageInfo, nullptr, &m_blurImage);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_blurImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    vkCreateImageView(device, &viewInfo, nullptr, &m_blurView);
}

} // namespace ks::engine::graphics
