#pragma once

#include "../EngineModule.h"
#include <QObject>
#include <QVector>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>
#include <QMatrix4x4>
#include <QJsonObject>
#include <vulkan/vulkan.h>

namespace ks::engine::graphics {

struct SSAOConfig {
    bool enabled = true;
    int kernelSize = 64;
    float radius = 0.5f;
    float bias = 0.025f;
    float power = 2.0f;
    float intensity = 1.0f;
    float noiseScale = 4.0f;
    int blurPasses = 2;
    float blurRadius = 3.0f;
    enum class Quality { Low, Medium, High, Ultra };
    Quality quality = Quality::Medium;
};

struct BloomConfig {
    bool enabled = true;
    float threshold = 1.0f;
    float knee = 0.5f;
    float intensity = 0.3f;
    float scatter = 0.7f;
    int mipLevels = 6;
    bool autoExposure = true;
    float exposureSpeed = 2.0f;
    float minExposure = 0.1f;
    float maxExposure = 10.0f;
};

struct MotionBlurConfig {
    bool enabled = false;
    float strength = 0.5f;
    int sampleCount = 8;
    float maxBlurLength = 0.02f;
    bool neighborMax = true;
};

struct ToneMappingConfig {
    enum class Mode { Linear, Reinhard, ACES, Uncharted2, Filmic };
    Mode mode = Mode::ACES;
    float exposure = 1.0f;
    float gamma = 2.2f;
    float whitePoint = 4.0f;
    float saturation = 1.0f;
    float contrast = 1.0f;
    float shadows = 0.0f;
    float highlights = 0.0f;
    QVector3D colorFilter = {1.0f, 1.0f, 1.0f};
    float temperature = 0.0f;
    float tint = 0.0f;
};

struct DOFConfig {
    bool enabled = false;
    float focusDistance = 10.0f;
    float aperture = 2.8f;
    float focalLength = 50.0f;
    float maxBlurSize = 8.0f;
    enum class Quality { Low, Medium, High };
    Quality quality = Quality::Medium;
};

struct FXAAConfig {
    bool enabled = true;
    float subpixelQuality = 0.75f;
    float edgeThreshold = 0.166f;
    float edgeThresholdMin = 0.0833f;
};

struct ColorGradingConfig {
    bool enabled = false;
    float brightness = 0.0f;
    float contrast = 1.0f;
    float saturation = 1.0f;
    QVector3D shadowsColor = {1, 1, 1};
    QVector3D midtonesColor = {1, 1, 1};
    QVector3D highlightsColor = {1, 1, 1};
    QVector3D shadowsHueShift = {0, 0, 0};
    QVector3D highlightsHueShift = {0, 0, 0};
    float lift = 0.0f;
    float gamma = 1.0f;
    float gain = 1.0f;
};

struct PostProcessingConfig {
    SSAOConfig ssao;
    BloomConfig bloom;
    MotionBlurConfig motionBlur;
    ToneMappingConfig toneMapping;
    DOFConfig dof;
    FXAAConfig fxaa;
    ColorGradingConfig colorGrading;
    bool enableVignette = true;
    float vignetteIntensity = 0.3f;
    float vignetteRadius = 0.8f;
    bool enableChromaticAberration = false;
    float chromaticAberrationStrength = 0.5f;
    bool enableFilmGrain = false;
    float filmGrainIntensity = 0.05f;
};

struct GBufferTextures {
    VkImage albedo = VK_NULL_HANDLE;
    VkImage normal = VK_NULL_HANDLE;
    VkImage materialParams = VK_NULL_HANDLE;
    VkImage depth = VK_NULL_HANDLE;
    VkImage velocity = VK_NULL_HANDLE;
    VkImageView albedoView = VK_NULL_HANDLE;
    VkImageView normalView = VK_NULL_HANDLE;
    VkImageView materialParamsView = VK_NULL_HANDLE;
    VkImageView depthView = VK_NULL_HANDLE;
    VkImageView velocityView = VK_NULL_HANDLE;
    VkFormat albedoFormat = VK_FORMAT_R8G8B8A8_SRGB;
    VkFormat normalFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    VkFormat materialFormat = VK_FORMAT_R8G8B8A8_UNORM;
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
    VkFormat velocityFormat = VK_FORMAT_R16G16_SFLOAT;
};

struct PostProcessPass {
    QString name;
    bool enabled = true;
    std::function<void(VkCommandBuffer, const GBufferTextures&, VkImageView)> execute;
};

class PostProcessingPipeline : public QObject, public EngineModule {
    Q_OBJECT
public:
    static PostProcessingPipeline& instance() { static PostProcessingPipeline s; return s; }

    QString moduleName() const override { return "PostProcessingPipeline"; }
    QString moduleId() const override { return "ks.postprocessing"; }
    bool initialize() override;
    void shutdown() override;

    void configure(const PostProcessingConfig& config);
    const PostProcessingConfig& config() const { return m_config; }

    void setSSAOConfig(const SSAOConfig& cfg) { m_config.ssao = cfg; }
    void setBloomConfig(const BloomConfig& cfg) { m_config.bloom = cfg; }
    void setMotionBlurConfig(const MotionBlurConfig& cfg) { m_config.motionBlur = cfg; }
    void setToneMappingConfig(const ToneMappingConfig& cfg) { m_config.toneMapping = cfg; }
    void setDOFConfig(const DOFConfig& cfg) { m_config.dof = cfg; }
    void setFXAAConfig(const FXAAConfig& cfg) { m_config.fxaa = cfg; }
    void setColorGradingConfig(const ColorGradingConfig& cfg) { m_config.colorGrading = cfg; }

    void enableSSAO(bool e) { m_config.ssao.enabled = e; }
    void enableBloom(bool e) { m_config.bloom.enabled = e; }
    void enableMotionBlur(bool e) { m_config.motionBlur.enabled = e; }
    void enableDOF(bool e) { m_config.dof.enabled = e; }
    void enableFXAA(bool e) { m_config.fxaa.enabled = e; }

    void createGBuffer(VkDevice device, VkPhysicalDevice physDev, uint32_t width, uint32_t height);
    void destroyGBuffer(VkDevice device);

    void beginGBufferPass(VkCommandBuffer cmd);
    void endGBufferPass(VkCommandBuffer cmd);

    void executeSSAO(VkCommandBuffer cmd);
    void executeBloom(VkCommandBuffer cmd);
    void executeMotionBlur(VkCommandBuffer cmd);
    void executeToneMapping(VkCommandBuffer cmd);
    void executeDOF(VkCommandBuffer cmd);
    void executeFXAA(VkCommandBuffer cmd);
    void executeColorGrading(VkCommandBuffer cmd);
    void executePostProcess(VkCommandBuffer cmd);

    void setPreviousFrameVelocity(VkImageView velocity) { m_prevVelocity = velocity; }
    void setExposure(float e) { m_config.toneMapping.exposure = e; }
    float currentEV100() const { return m_ev100; }

    VkRenderPass gbufferRenderPass() const { return m_gbufferRenderPass; }
    const GBufferTextures& gbuffer() const { return m_gbuffer; }

signals:
    void configChanged();
    void gbufferCreated();
    void gbufferDestroyed();

private:
    void createNoiseTexture(VkDevice device, VkPhysicalDevice physDev);
    void createSSAOKernel();
    void createBloomDownsampleTargets(VkDevice device, VkPhysicalDevice physDev, uint32_t width, uint32_t height);
    void createBloomUpsampleTargets(VkDevice device, VkPhysicalDevice physDev, uint32_t width, uint32_t height);

    PostProcessingConfig m_config;
    GBufferTextures m_gbuffer;
    VkRenderPass m_gbufferRenderPass = VK_NULL_HANDLE;
    QVector<VkFramebuffer> m_gbufferFramebuffers;
    QVector<QVector3D> m_ssaoKernel;
    VkImage m_ssaoNoise = VK_NULL_HANDLE;
    VkImageView m_ssaoNoiseView = VK_NULL_HANDLE;
    VkImageView m_prevVelocity = VK_NULL_HANDLE;
    QVector<VkImage> m_bloomDownsample;
    QVector<VkImageView> m_bloomDownsampleViews;
    QVector<VkImage> m_bloomUpsample;
    QVector<VkImageView> m_bloomUpsampleViews;
    float m_ev100 = 0.0f;
    float m_exposureAdaptation = 1.0f;
};

} // namespace ks::engine::graphics
