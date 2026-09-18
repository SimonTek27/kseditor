#pragma once

#include "../EngineModule.h"
#include <QObject>
#include <QVector>
#include <QVector3D>
#include <QVector4D>
#include <QMatrix4x4>
#include <vulkan/vulkan.h>

namespace ks::engine::graphics {

struct CascadeConfig {
    int cascadeCount = 4;
    float splitSchemeLambda = 0.75f;
    float maxShadowDistance = 200.0f;
    float shadowBias = 0.005f;
    float normalBias = 0.02f;
    int shadowMapSize = 2048;
    bool enableSoftShadows = true;
    float penumbraFilterSize = 1.0f;
    int pcfSamples = 4;
    bool enableContactHardening = true;
    float maxPenumbra = 4.0f;
    bool enableMomentShadowMaps = false;
    bool enableVSM = false;
    float vsmLightBleedReduction = 0.1f;
    float vsmMinVariance = 0.00002f;
};

struct CascadeData {
    QMatrix4x4 viewProjMatrix;
    QMatrix4x4 viewMatrix;
    QMatrix4x4 projMatrix;
    QVector4D splitDepths;
    float texelSize = 0.0f;
    float resolution = 2048.0f;
    QVector3D lightDirection;
    float boundRadius = 0.0f;
};

struct ShadowRenderData {
    QVector<CascadeData> cascades;
    QVector<VkImage> shadowMaps;
    QVector<VkImageView> shadowMapViews;
    QVector<VkFramebuffer> framebuffers;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkSampler shadowSampler = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorLayout = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
};

class CascadedShadowMaps : public QObject, public EngineModule {
    Q_OBJECT
public:
    static CascadedShadowMaps& instance() { static CascadedShadowMaps s; return s; }

    QString moduleName() const override { return "CascadedShadowMaps"; }
    QString moduleId() const override { return "ks.csm"; }
    bool initialize() override;
    void shutdown() override;

    void configure(const CascadeConfig& config);
    const CascadeConfig& config() const { return m_config; }

    void update(const QVector3D& lightDirection, const QMatrix4x4& viewMatrix,
                const QMatrix4x4& projMatrix, float nearPlane, float farPlane);

    const QVector<CascadeData>& cascades() const { return m_shadowData.cascades; }
    int cascadeCount() const { return m_config.cascadeCount; }

    QMatrix4x4 getCascadeViewProjMatrix(int cascadeIndex) const;
    float getCascadeSplitDepth(int cascadeIndex) const;
    float getCascadeTexelSize(int cascadeIndex) const;

    bool createResources(VkDevice device, VkPhysicalDevice physDev, VkRenderPass sceneRenderPass);
    void destroyResources(VkDevice device);
    void renderCascades(VkCommandBuffer cmd, int cascadeIndex);

    const ShadowRenderData& shadowData() const { return m_shadowData; }
    VkDescriptorSet getDescriptorSet() const { return m_shadowData.descriptorSet; }
    VkDescriptorSetLayout getDescriptorLayout() const { return m_shadowData.descriptorLayout; }
    VkPipelineLayout getPipelineLayout() const { return m_shadowData.pipelineLayout; }

    void setLightDirection(const QVector3D& dir) { m_lightDir = dir.normalized(); }
    QVector3D lightDirection() const { return m_lightDir; }

    void setFilterRadius(float radius) { m_config.penumbraFilterSize = radius; }
    void setPCFSamples(int samples) { m_config.pcfSamples = samples; }

signals:
    void cascadesUpdated();
    void resourcesCreated();
    void resourcesDestroyed();

private:
    void computeSplitDepths(float nearPlane, float farPlane);
    void computeCascadeFrusta(const QMatrix4x4& viewMatrix, const QMatrix4x4& projMatrix,
                              float nearPlane, float farPlane);
    void computeCascadeBound(int cascadeIndex, const QMatrix4x4& viewMatrix);
    QMatrix4x4 computeLightViewProj(const QVector3D& lightDir, const QVector3D& center,
                                     float boundRadius, const QMatrix4x4& projMatrix);

    CascadeConfig m_config;
    ShadowRenderData m_shadowData;
    QVector<float> m_splitDepths;
    QVector<QMatrix4x4> m_cascadeProjMatrices;
    QVector3D m_lightDir = {0.5f, -1.0f, 0.3f};
    float m_currentNear = 0.1f;
    float m_currentFar = 1000.0f;
};

} // namespace ks::engine::graphics
