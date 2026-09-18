#pragma once

#include "../EngineModule.h"
#include <QObject>
#include <QVector>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>
#include <QMatrix4x4>
#include <vulkan/vulkan.h>

namespace ks::engine::graphics {

struct SSRConfig {
    bool enabled = true;
    int maxSteps = 64;
    int binarySearchSteps = 16;
    float thickness = 0.1f;
    float maxDistance = 100.0f;
    float edgeFadeFactor = 0.1f;
    float screenFadeFactor = 0.1f;
    float intensity = 1.0f;
    float roughnessThreshold = 0.4f;
    bool enableRoughnessFade = true;
    bool enableDistanceFade = true;
    bool enableTemporalFiltering = true;
    float temporalBlendFactor = 0.1f;
    bool enableHiZ = true;
    int hiZMaxMip = 6;
    bool enableEdgeDetection = true;
    float edgeThreshold = 0.1f;
    bool enableReflectionBlur = true;
    float blurRadius = 2.0f;
    enum class Quality { Low, Medium, High, Ultra };
    Quality quality = Quality::Medium;
    bool halfResolution = false;
    bool bilateralFilter = true;
    float normalReprojectionThreshold = 0.8f;
};

class SSRSystem : public QObject, public EngineModule {
    Q_OBJECT
public:
    static SSRSystem& instance() { static SSRSystem s; return s; }

    QString moduleName() const override { return "SSRSystem"; }
    QString moduleId() const override { return "ks.ssr"; }
    bool initialize() override;
    void shutdown() override;

    void configure(const SSRConfig& config);
    const SSRConfig& config() const { return m_config; }

    void setMaxSteps(int steps) { m_config.maxSteps = steps; }
    void setThickness(float t) { m_config.thickness = t; }
    void setIntensity(float i) { m_config.intensity = i; }
    void setRoughnessThreshold(float t) { m_config.roughnessThreshold = t; }

    void createResources(VkDevice device, VkPhysicalDevice physDev, uint32_t width, uint32_t height);
    void destroyResources(VkDevice device);

    void execute(VkCommandBuffer cmd, VkImageView colorBuffer, VkImageView normalBuffer,
                 VkImageView depthBuffer, VkImageView materialBuffer,
                 const QMatrix4x4& viewMatrix, const QMatrix4x4& projMatrix,
                 const QMatrix4x4& prevViewMatrix, const QMatrix4x4& prevProjMatrix);

    VkImageView reflectionResult() const { return m_reflectionView; }

    void setResolution(uint32_t width, uint32_t height);

signals:
    void configChanged();
    void resourcesCreated();
    void resourcesDestroyed();

private:
    void createReflectionTarget(VkDevice device, VkPhysicalDevice physDev, uint32_t width, uint32_t height);
    void createHistoryTarget(VkDevice device, VkPhysicalDevice physDev, uint32_t width, uint32_t height);
    void createEdgeDetectTarget(VkDevice device, VkPhysicalDevice physDev, uint32_t width, uint32_t height);
    void createBlurTarget(VkDevice device, VkPhysicalDevice physDev, uint32_t width, uint32_t height);

    SSRConfig m_config;
    VkImage m_reflectionImage = VK_NULL_HANDLE;
    VkImageView m_reflectionView = VK_NULL_HANDLE;
    VkDeviceMemory m_reflectionMemory = VK_NULL_HANDLE;
    VkImage m_historyImage = VK_NULL_HANDLE;
    VkImageView m_historyView = VK_NULL_HANDLE;
    VkDeviceMemory m_historyMemory = VK_NULL_HANDLE;
    VkImage m_edgeDetectImage = VK_NULL_HANDLE;
    VkImageView m_edgeDetectView = VK_NULL_HANDLE;
    VkDeviceMemory m_edgeDetectMemory = VK_NULL_HANDLE;
    VkImage m_blurImage = VK_NULL_HANDLE;
    VkImageView m_blurView = VK_NULL_HANDLE;
    VkDeviceMemory m_blurMemory = VK_NULL_HANDLE;
    VkSampler m_sampler = VK_NULL_HANDLE;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

} // namespace ks::engine::graphics
