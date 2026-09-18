#pragma once

#include "../EngineModule.h"
#include <QObject>
#include <QString>
#include <QJsonObject>
#include <vulkan/vulkan.h>

namespace ks::engine::graphics {

// ============================================================================
// Streamline Integration - NVIDIA DLSS, Reflex, NIS
// ============================================================================

struct DLSSConfig {
    bool enabled = false;
    enum class Quality {
        Off = 0,
        MaxPerformance = 1,
        Balanced = 2,
        MaxQuality = 3,
        UltraPerformance = 4,
        UltraQuality = 5,
        DLAA = 6
    };
    Quality quality = Quality::Balanced;
    float sharpness = 0.0f;
};

struct ReflexConfig {
    bool enabled = false;
    enum class Mode {
        Off = 0,
        Enabled = 1,
        EnabledPlusBoost = 2
    };
    Mode mode = Mode::Enabled;
};

struct NISConfig {
    bool enabled = false;
    enum class Quality {
        Off = 0,
        On = 1
    };
    Quality quality = Quality::On;
    float sharpness = 0.5f;
};

struct StreamlineConfig {
    DLSSConfig dlss;
    ReflexConfig reflex;
    NISConfig nis;
};

class StreamlineIntegration : public QObject, public EngineModule {
    Q_OBJECT
public:
    static StreamlineIntegration& instance() { static StreamlineIntegration s; return s; }

    QString moduleName() const override { return "StreamlineIntegration"; }
    QString moduleId() const override { return "ks.streamline"; }
    bool initialize() override;
    void shutdown() override;

    // Configuration
    void configure(const StreamlineConfig& config);
    const StreamlineConfig& config() const { return m_config; }

    // DLSS
    void setDLSSMode(DLSSConfig::Quality mode);
    bool isDLSSSupported() const { return m_dlssSupported; }
    bool isDLSSActive() const { return m_dlssActive; }

    // Reflex
    void setReflexMode(ReflexConfig::Mode mode);
    bool isReflexSupported() const { return m_reflexSupported; }
    bool isReflexActive() const { return m_reflexActive; }

    // NIS
    void setNISMode(NISConfig::Quality mode);
    bool isNISSupported() const { return m_nisSupported; }
    bool isNISActive() const { return m_nisActive; }

    // Per-frame evaluation (call after render pass begins, before draw calls)
    void evaluateFeatures(VkCommandBuffer cmd, uint32_t frameIndex);

    // Resource management for DLSS
    void setDLSSInputResources(
        VkImage color,
        VkImageView colorView,
        VkImage depth,
        VkImageView depthView,
        VkImage motionVectors,
        VkImageView motionVectorsView,
        uint32_t renderWidth,
        uint32_t renderHeight,
        uint32_t displayWidth,
        uint32_t displayHeight
    );

    void setDLSSOutputResource(
        VkImage output,
        VkImageView outputView,
        VkImageLayout outputLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );

    // Get output after evaluation
    VkImage dlssOutputImage() const { return m_dlssOutputImage; }
    VkImageView dlssOutputView() const { return m_dlssOutputView; }

    // Status
    bool isAvailable() const { return m_available; }
    QString lastError() const { return m_lastError; }

    // JSON config serialization
    QJsonObject toJson() const;
    void loadJson(const QJsonObject& json);

signals:
    void configChanged();
    void dlssModeChanged(int mode);
    void reflexModeChanged(int mode);
    void nisModeChanged(int mode);

private:
    StreamlineIntegration() = default;
    ~StreamlineIntegration();

    bool initializeStreamline();
    void shutdownStreamline();
    bool checkDLSSSupport();
    bool checkReflexSupport();
    bool checkNISSupport();

    StreamlineConfig m_config;
    bool m_available = false;
    QString m_lastError;

    // Feature support state
    bool m_dlssSupported = false;
    bool m_dlssActive = false;
    bool m_reflexSupported = false;
    bool m_reflexActive = false;
    bool m_nisSupported = false;
    bool m_nisActive = false;

    // DLSS resources
    VkImage m_dlssColorInput = VK_NULL_HANDLE;
    VkImageView m_dlssColorInputView = VK_NULL_HANDLE;
    VkImage m_dlssDepthInput = VK_NULL_HANDLE;
    VkImageView m_dlssDepthInputView = VK_NULL_HANDLE;
    VkImage m_dlssMotionVectorsInput = VK_NULL_HANDLE;
    VkImageView m_dlssMotionVectorsInputView = VK_NULL_HANDLE;
    VkImage m_dlssOutputImage = VK_NULL_HANDLE;
    VkImageView m_dlssOutputView = VK_NULL_HANDLE;
    VkImageLayout m_dlssOutputLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    uint32_t m_renderWidth = 0;
    uint32_t m_renderHeight = 0;
    uint32_t m_displayWidth = 0;
    uint32_t m_displayHeight = 0;
    bool m_dlssResourcesDirty = false;
};

} // namespace ks::engine::graphics
