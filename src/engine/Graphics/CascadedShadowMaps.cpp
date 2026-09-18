#include "CascadedShadowMaps.h"
#include <QDebug>
#include <QtMath>
#include <algorithm>

namespace ks::engine::graphics {

bool CascadedShadowMaps::initialize() {
    if (m_initialized) return true;
    m_splitDepths.resize(m_config.cascadeCount + 1);
    m_shadowData.cascades.resize(m_config.cascadeCount);
    m_initialized = true;
    qInfo() << "CascadedShadowMaps: initialized with" << m_config.cascadeCount << "cascades";
    return true;
}

void CascadedShadowMaps::shutdown() {
    if (!m_initialized) return;
    m_splitDepths.clear();
    m_shadowData.cascades.clear();
    m_initialized = false;
    qInfo() << "CascadedShadowMaps: shutdown";
}

void CascadedShadowMaps::configure(const CascadeConfig& config) {
    m_config = config;
    m_splitDepths.resize(m_config.cascadeCount + 1);
    m_shadowData.cascades.resize(m_config.cascadeCount);
}

void CascadedShadowMaps::update(const QVector3D& lightDirection, const QMatrix4x4& viewMatrix,
                                  const QMatrix4x4& projMatrix, float nearPlane, float farPlane) {
    m_lightDir = lightDirection.normalized();
    m_currentNear = nearPlane;
    m_currentFar = farPlane;
    computeSplitDepths(nearPlane, farPlane);
    computeCascadeFrusta(viewMatrix, projMatrix, nearPlane, farPlane);
    emit cascadesUpdated();
}

QMatrix4x4 CascadedShadowMaps::getCascadeViewProjMatrix(int cascadeIndex) const {
    if (cascadeIndex >= 0 && cascadeIndex < m_shadowData.cascades.size())
        return m_shadowData.cascades[cascadeIndex].viewProjMatrix;
    return QMatrix4x4();
}

float CascadedShadowMaps::getCascadeSplitDepth(int cascadeIndex) const {
    if (cascadeIndex >= 0 && cascadeIndex < m_splitDepths.size())
        return m_splitDepths[cascadeIndex + 1];
    return 1000.0f;
}

float CascadedShadowMaps::getCascadeTexelSize(int cascadeIndex) const {
    if (cascadeIndex >= 0 && cascadeIndex < m_shadowData.cascades.size())
        return m_shadowData.cascades[cascadeIndex].texelSize;
    return 1.0f;
}

bool CascadedShadowMaps::createResources(VkDevice device, VkPhysicalDevice physDev, VkRenderPass sceneRenderPass) {
    m_shadowData.shadowMaps.resize(m_config.cascadeCount);
    m_shadowData.shadowMapViews.resize(m_config.cascadeCount);
    m_shadowData.framebuffers.resize(m_config.cascadeCount);

    for (int i = 0; i < m_config.cascadeCount; ++i) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = {(uint32_t)m_config.shadowMapSize, (uint32_t)m_config.shadowMapSize, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_D32_SFLOAT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        vkCreateImage(device, &imageInfo, nullptr, &m_shadowData.shadowMaps[i]);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_shadowData.shadowMaps[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_D32_SFLOAT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(device, &viewInfo, nullptr, &m_shadowData.shadowMapViews[i]);
    }

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.compareEnable = VK_TRUE;
    samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    vkCreateSampler(device, &samplerInfo, nullptr, &m_shadowData.shadowSampler);

    emit resourcesCreated();
    qInfo() << "CascadedShadowMaps: resources created";
    return true;
}

void CascadedShadowMaps::destroyResources(VkDevice device) {
    for (auto& fb : m_shadowData.framebuffers) {
        if (fb) vkDestroyFramebuffer(device, fb, nullptr);
    }
    for (auto& view : m_shadowData.shadowMapViews) {
        if (view) vkDestroyImageView(device, view, nullptr);
    }
    for (auto& img : m_shadowData.shadowMaps) {
        if (img) vkDestroyImage(device, img, nullptr);
    }
    if (m_shadowData.shadowSampler) {
        vkDestroySampler(device, m_shadowData.shadowSampler, nullptr);
        m_shadowData.shadowSampler = VK_NULL_HANDLE;
    }
    m_shadowData.shadowMaps.clear();
    m_shadowData.shadowMapViews.clear();
    m_shadowData.framebuffers.clear();
    emit resourcesDestroyed();
}

void CascadedShadowMaps::renderCascades(VkCommandBuffer cmd, int cascadeIndex) {
    if (cascadeIndex < 0 || cascadeIndex >= m_config.cascadeCount) return;
}

void CascadedShadowMaps::computeSplitDepths(float nearPlane, float farPlane) {
    float range = farPlane - nearPlane;
    float ratio = farPlane / nearPlane;
    m_splitDepths[0] = nearPlane;
    for (int i = 1; i <= m_config.cascadeCount; ++i) {
        float p = (float)i / m_config.cascadeCount;
        float logSplit = nearPlane * qPow(ratio, p);
        float uniformSplit = nearPlane + range * p;
        m_splitDepths[i] = m_config.splitSchemeLambda * logSplit + (1.0f - m_config.splitSchemeLambda) * uniformSplit;
    }
}

void CascadedShadowMaps::computeCascadeFrusta(const QMatrix4x4& viewMatrix, const QMatrix4x4& projMatrix,
                                                float nearPlane, float farPlane) {
    for (int i = 0; i < m_config.cascadeCount; ++i) {
        float splitNear = m_splitDepths[i];
        float splitFar = m_splitDepths[i + 1];
        QMatrix4x4 cascadeProj = projMatrix;
        float tanHalfFOV = 1.0f / projMatrix(1, 1);
        float aspect = projMatrix(1, 1) / projMatrix(0, 0);
        float xn = tanHalfFOV * splitNear;
        float yn = xn / aspect;
        float xf = tanHalfFOV * splitFar;
        float yf = xf / aspect;
        cascadeProj(0, 0) = 2.0f * splitNear / (xf - xn);
        cascadeProj(0, 2) = (xf + xn) / (xf - xn);
        cascadeProj(1, 1) = 2.0f * splitNear / (yf - yn);
        cascadeProj(1, 2) = (yf + yn) / (yf - yn);
        m_cascadeProjMatrices.append(cascadeProj);
        computeCascadeBound(i, viewMatrix);
        m_shadowData.cascades[i].splitDepths = QVector4D(splitNear, splitFar, 0, 0);
        m_shadowData.cascades[i].resolution = (float)m_config.shadowMapSize;
        m_shadowData.cascades[i].lightDirection = m_lightDir;
    }
}

void CascadedShadowMaps::computeCascadeBound(int cascadeIndex, const QMatrix4x4& viewMatrix) {
    QMatrix4x4 invView = viewMatrix.inverted();
    QVector3D frustumCorners[8] = {
        {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
        {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1}
    };
    float splitNear = m_splitDepths[cascadeIndex];
    float splitFar = m_splitDepths[cascadeIndex + 1];
    QMatrix4x4 casProj = m_cascadeProjMatrices[cascadeIndex];
    QMatrix4x4 viewProj = casProj * viewMatrix;
    QMatrix4x4 invViewProj = viewProj.inverted();
    QVector3D center(0, 0, 0);
    for (int i = 0; i < 8; ++i) {
        QVector4D corner = invViewProj * QVector4D(frustumCorners[i], 1.0f);
        frustumCorners[i] = QVector3D(corner.x() / corner.w(), corner.y() / corner.w(), corner.z() / corner.w());
        center += frustumCorners[i];
    }
    center /= 8.0f;
    float radius = 0.0f;
    for (int i = 0; i < 8; ++i) {
        float dist = (frustumCorners[i] - center).length();
        radius = qMax(radius, dist);
    }
    radius = qCeil(radius * 16.0f) / 16.0f;
    m_shadowData.cascades[cascadeIndex].boundRadius = radius;
    m_shadowData.cascades[cascadeIndex].texelSize = 2.0f * radius / m_config.shadowMapSize;
    m_shadowData.cascades[cascadeIndex].viewProjMatrix = computeLightViewProj(m_lightDir, center, radius, casProj);
}

QMatrix4x4 CascadedShadowMaps::computeLightViewProj(const QVector3D& lightDir, const QVector3D& center,
                                                      float boundRadius, const QMatrix4x4& projMatrix) {
    QVector3D lightPos = center - lightDir * boundRadius * 2.0f;
    QMatrix4x4 lightView;
    lightView.lookAt(lightPos, center, QVector3D(0, 1, 0));
    float l = -boundRadius, r = boundRadius;
    float b = -boundRadius, t = boundRadius;
    float n = 0.01f, f = boundRadius * 4.0f;
    QMatrix4x4 lightProj;
    lightProj(0, 0) = 2.0f / (r - l);
    lightProj(0, 3) = -(r + l) / (r - l);
    lightProj(1, 1) = 2.0f / (t - b);
    lightProj(1, 3) = -(t + b) / (t - b);
    lightProj(2, 2) = -2.0f / (f - n);
    lightProj(2, 3) = -(f + n) / (f - n);
    lightProj(3, 3) = 1.0f;
    return lightProj * lightView;
}

} // namespace ks::engine::graphics
