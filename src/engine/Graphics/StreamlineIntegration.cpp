#include "StreamlineIntegration.h"
#include "StreamlineFunctions.h"
#include "VulkanRenderer.h"
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>

namespace ks::engine::graphics {

// ============================================================================
// Streamline Integration - Implementation
// ============================================================================

StreamlineIntegration::~StreamlineIntegration() {
    shutdown();
}

bool StreamlineIntegration::initialize() {
    if (m_initialized) return true;

    if (!initializeStreamline()) {
        qWarning() << "Streamline: Initialization failed:" << m_lastError;
        m_available = false;
        m_initialized = true; // mark initialized so we don't retry
        return false;
    }

    m_available = true;
    m_initialized = true;

    // Check feature support
    m_dlssSupported = checkDLSSSupport();
    m_reflexSupported = checkReflexSupport();
    m_nisSupported = checkNISSupport();

    qInfo() << "Streamline: initialized successfully";
    qInfo() << "  DLSS:" << (m_dlssSupported ? "supported" : "NOT supported");
    qInfo() << "  Reflex:" << (m_reflexSupported ? "supported" : "NOT supported");
    qInfo() << "  NIS:" << (m_nisSupported ? "supported" : "NOT supported");

    // Apply initial config
    if (m_config.dlss.enabled && m_dlssSupported) {
        setDLSSMode(m_config.dlss.quality);
    }
    if (m_config.reflex.enabled && m_reflexSupported) {
        setReflexMode(m_config.reflex.mode);
    }
    if (m_config.nis.enabled && m_nisSupported) {
        setNISMode(m_config.nis.quality);
    }

    return true;
}

void StreamlineIntegration::shutdown() {
    if (!m_initialized) return;

    if (m_dlssActive && g_sl.unsetFeature) {
        g_sl.unsetFeature(static_cast<uint32_t>(SLFeature::DLSS), 0, nullptr);
    }
    if (m_reflexActive && g_sl.unsetFeature) {
        g_sl.unsetFeature(static_cast<uint32_t>(SLFeature::Reflex), 0, nullptr);
    }
    if (m_nisActive && g_sl.unsetFeature) {
        g_sl.unsetFeature(static_cast<uint32_t>(SLFeature::NIS), 0, nullptr);
    }

    shutdownStreamline();

    m_dlssActive = false;
    m_reflexActive = false;
    m_nisActive = false;
    m_available = false;
    m_initialized = false;

    qInfo() << "Streamline: shutdown";
}

bool StreamlineIntegration::initializeStreamline() {
    if (!loadStreamlineLibrary()) {
        m_lastError = "Streamline DLL (sl.common.dll) not found";
        return false;
    }

    // Get Vulkan device info from the renderer
    auto* renderer = VulkanRenderer::instance();
    if (!renderer || !renderer->device()) {
        m_lastError = "Vulkan device not available";
        return false;
    }

    // Initialize Streamline with Vulkan device info
    struct SLInitParams {
        uint32_t structSize = sizeof(SLInitParams);
        uint32_t appVersion = VK_MAKE_VERSION(2, 1, 0);
        const char* appEngine = "ksEngine";
        VkInstance instance = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;
        VkQueue graphicsQueue = VK_NULL_HANDLE;
        uint32_t queueFamilyIndex = 0;
        const char* projectDir = nullptr;
    };

    SLInitParams params{};
    params.instance = renderer->vulkanInstance();
    params.physicalDevice = renderer->physicalDevice();
    params.device = renderer->device();
    params.graphicsQueue = renderer->graphicsQueue();
    params.queueFamilyIndex = renderer->graphicsQueueFamilyIndex();
    params.projectDir = nullptr;

    int result = g_sl.init(&params);
    if (result != kSLResultSuccess) {
        m_lastError = QString("slInit failed with code %1").arg(result);
        return false;
    }

    return true;
}

void StreamlineIntegration::shutdownStreamline() {
    if (g_sl.shutdown) {
        g_sl.shutdown();
    }
}

bool StreamlineIntegration::checkDLSSSupport() {
    if (!g_sl.getFeatureRequirements) return false;

    struct SLFeatureRequirements {
        uint32_t structSize = sizeof(SLFeatureRequirements);
        uint32_t featureID = 0;
        uint32_t result = 0; // 0 = supported
        char reason[256] = {};
    };

    SLFeatureRequirements req{};
    req.featureID = static_cast<uint32_t>(SLFeature::DLSS);
    int result = g_sl.getFeatureRequirements(static_cast<uint32_t>(SLFeature::DLSS), &req);
    return result == kSLResultSuccess && req.result == 0;
}

bool StreamlineIntegration::checkReflexSupport() {
    if (!g_sl.getFeatureRequirements) return false;

    struct SLFeatureRequirements {
        uint32_t structSize = sizeof(SLFeatureRequirements);
        uint32_t featureID = 0;
        uint32_t result = 0;
        char reason[256] = {};
    };

    SLFeatureRequirements req{};
    req.featureID = static_cast<uint32_t>(SLFeature::Reflex);
    int result = g_sl.getFeatureRequirements(static_cast<uint32_t>(SLFeature::Reflex), &req);
    return result == kSLResultSuccess && req.result == 0;
}

bool StreamlineIntegration::checkNISSupport() {
    if (!g_sl.getFeatureRequirements) return false;

    struct SLFeatureRequirements {
        uint32_t structSize = sizeof(SLFeatureRequirements);
        uint32_t featureID = 0;
        uint32_t result = 0;
        char reason[256] = {};
    };

    SLFeatureRequirements req{};
    req.featureID = static_cast<uint32_t>(SLFeature::NIS);
    int result = g_sl.getFeatureRequirements(static_cast<uint32_t>(SLFeature::NIS), &req);
    return result == kSLResultSuccess && req.result == 0;
}

// ============================================================================
// Configuration
// ============================================================================

void StreamlineIntegration::configure(const StreamlineConfig& config) {
    bool dlssChanged = config.dlss.quality != m_config.dlss.quality || config.dlss.enabled != m_config.dlss.enabled;
    bool reflexChanged = config.reflex.mode != m_config.reflex.mode || config.reflex.enabled != m_config.reflex.enabled;
    bool nisChanged = config.nis.quality != m_config.nis.quality || config.nis.enabled != m_config.nis.enabled;

    m_config = config;

    if (dlssChanged && m_dlssSupported) {
        setDLSSMode(m_config.dlss.enabled ? m_config.dlss.quality : DLSSConfig::Quality::Off);
    }
    if (reflexChanged && m_reflexSupported) {
        setReflexMode(m_config.reflex.enabled ? m_config.reflex.mode : ReflexConfig::Mode::Off);
    }
    if (nisChanged && m_nisSupported) {
        setNISMode(m_config.nis.enabled ? m_config.nis.quality : NISConfig::Quality::Off);
    }

    emit configChanged();
}

// ============================================================================
// DLSS
// ============================================================================

void StreamlineIntegration::setDLSSMode(DLSSConfig::Quality mode) {
    if (!m_dlssSupported) return;

    m_config.dlss.quality = mode;
    m_config.dlss.enabled = (mode != DLSSConfig::Quality::Off);

    if (g_sl.setDLSSMode) {
        int result = g_sl.setDLSSMode(static_cast<int>(mode));
        if (result == kSLResultSuccess) {
            m_dlssActive = (mode != DLSSConfig::Quality::Off);
            qDebug() << "Streamline: DLSS mode set to" << static_cast<int>(mode);
            emit dlssModeChanged(static_cast<int>(mode));
        } else {
            qWarning() << "Streamline: Failed to set DLSS mode:" << result;
        }
    }
}

void StreamlineIntegration::setDLSSInputResources(
    VkImage color, VkImageView colorView,
    VkImage depth, VkImageView depthView,
    VkImage motionVectors, VkImageView motionVectorsView,
    uint32_t renderWidth, uint32_t renderHeight,
    uint32_t displayWidth, uint32_t displayHeight)
{
    m_dlssColorInput = color;
    m_dlssColorInputView = colorView;
    m_dlssDepthInput = depth;
    m_dlssDepthInputView = depthView;
    m_dlssMotionVectorsInput = motionVectors;
    m_dlssMotionVectorsInputView = motionVectorsView;
    m_renderWidth = renderWidth;
    m_renderHeight = renderHeight;
    m_displayWidth = displayWidth;
    m_displayHeight = displayHeight;
    m_dlssResourcesDirty = true;
}

void StreamlineIntegration::setDLSSOutputResource(
    VkImage output, VkImageView outputView, VkImageLayout outputLayout)
{
    m_dlssOutputImage = output;
    m_dlssOutputView = outputView;
    m_dlssOutputLayout = outputLayout;
}

// ============================================================================
// Reflex
// ============================================================================

void StreamlineIntegration::setReflexMode(ReflexConfig::Mode mode) {
    if (!m_reflexSupported) return;

    m_config.reflex.mode = mode;
    m_config.reflex.enabled = (mode != ReflexConfig::Mode::Off);

    if (g_sl.setReflexMode) {
        int result = g_sl.setReflexMode(static_cast<int>(mode));
        if (result == kSLResultSuccess) {
            m_reflexActive = (mode != ReflexConfig::Mode::Off);
            qDebug() << "Streamline: Reflex mode set to" << static_cast<int>(mode);
            emit reflexModeChanged(static_cast<int>(mode));
        } else {
            qWarning() << "Streamline: Failed to set Reflex mode:" << result;
        }
    }
}

// ============================================================================
// NIS
// ============================================================================

void StreamlineIntegration::setNISMode(NISConfig::Quality mode) {
    if (!m_nisSupported) return;

    m_config.nis.quality = mode;
    m_config.nis.enabled = (mode != NISConfig::Quality::Off);

    if (g_sl.setNISMode) {
        int result = g_sl.setNISMode(static_cast<int>(mode));
        if (result == kSLResultSuccess) {
            m_nisActive = (mode != NISConfig::Quality::Off);
            qDebug() << "Streamline: NIS mode set to" << static_cast<int>(mode);
            emit nisModeChanged(static_cast<int>(mode));
        } else {
            qWarning() << "Streamline: Failed to set NIS mode:" << result;
        }
    }
}

// ============================================================================
// Feature Evaluation
// ============================================================================

void StreamlineIntegration::evaluateFeatures(VkCommandBuffer cmd, uint32_t frameIndex) {
    if (!m_available || !g_sl.evaluateFeature) return;

    // Evaluate DLSS if active and resources are set
    if (m_dlssActive && m_dlssResourcesDirty && m_dlssOutputImage) {
        struct SLDLSSEvaluateParams {
            uint32_t structSize = sizeof(SLDLSSEvaluateParams);
            VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
            SLResource inputColor;
            SLResource inputDepth;
            SLResource inputMotionVectors;
            SLResource outputColor;
            uint32_t renderWidth = 0;
            uint32_t renderHeight = 0;
            uint32_t displayWidth = 0;
            uint32_t displayHeight = 0;
            float sharpness = 0.0f;
        };

        SLDLSSEvaluateParams params{};
        params.cmdBuffer = cmd;
        params.renderWidth = m_renderWidth;
        params.renderHeight = m_renderHeight;
        params.displayWidth = m_displayWidth;
        params.displayHeight = m_displayHeight;
        params.sharpness = m_config.dlss.sharpness;

        // Set up input color resource
        params.inputColor.type = SLResourceType::Texture2D;
        params.inputColor.state = SLResourceState::RenderTarget;
        params.inputColor.vulkan.image = m_dlssColorInput;
        params.inputColor.vulkan.imageView = m_dlssColorInputView;
        params.inputColor.vulkan.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        // Set up input depth resource
        params.inputDepth.type = SLResourceType::Texture2D;
        params.inputDepth.state = SLResourceState::RenderTarget;
        params.inputDepth.vulkan.image = m_dlssDepthInput;
        params.inputDepth.vulkan.imageView = m_dlssDepthInputView;
        params.inputDepth.vulkan.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // Set up input motion vectors resource
        params.inputMotionVectors.type = SLResourceType::Texture2D;
        params.inputMotionVectors.state = SLResourceState::RenderTarget;
        params.inputMotionVectors.vulkan.image = m_dlssMotionVectorsInput;
        params.inputMotionVectors.vulkan.imageView = m_dlssMotionVectorsInputView;
        params.inputMotionVectors.vulkan.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // Set up output resource
        params.outputColor.type = SLResourceType::Texture2D;
        params.outputColor.state = SLResourceState::RenderTarget;
        params.outputColor.vulkan.image = m_dlssOutputImage;
        params.outputColor.vulkan.imageView = m_dlssOutputView;
        params.outputColor.vulkan.imageLayout = m_dlssOutputLayout;

        int result = g_sl.evaluateFeature(
            static_cast<uint32_t>(SLFeature::DLSS),
            frameIndex,
            &params,
            0, nullptr
        );

        if (result != kSLResultSuccess) {
            qWarning() << "Streamline: DLSS evaluate failed:" << result;
        }
    }

    // Evaluate NIS if active (NIS is typically combined with DLSS or used standalone)
    if (m_nisActive) {
        g_sl.evaluateFeature(
            static_cast<uint32_t>(SLFeature::NIS),
            frameIndex,
            nullptr,
            0, nullptr
        );
    }
}

// ============================================================================
// JSON Config Serialization
// ============================================================================

QJsonObject StreamlineIntegration::toJson() const {
    QJsonObject json;

    // DLSS
    QJsonObject dlss;
    dlss["enabled"] = m_config.dlss.enabled;
    dlss["quality"] = static_cast<int>(m_config.dlss.quality);
    dlss["sharpness"] = m_config.dlss.sharpness;
    dlss["supported"] = m_dlssSupported;
    dlss["active"] = m_dlssActive;
    json["dlss"] = dlss;

    // Reflex
    QJsonObject reflex;
    reflex["enabled"] = m_config.reflex.enabled;
    reflex["mode"] = static_cast<int>(m_config.reflex.mode);
    reflex["supported"] = m_reflexSupported;
    reflex["active"] = m_reflexActive;
    json["reflex"] = reflex;

    // NIS
    QJsonObject nis;
    nis["enabled"] = m_config.nis.enabled;
    nis["quality"] = static_cast<int>(m_config.nis.quality);
    nis["sharpness"] = m_config.nis.sharpness;
    nis["supported"] = m_nisSupported;
    nis["active"] = m_nisActive;
    json["nis"] = nis;

    // General
    json["available"] = m_available;
    json["lastError"] = m_lastError;

    return json;
}

void StreamlineIntegration::loadJson(const QJsonObject& json) {
    StreamlineConfig config;

    if (json.contains("dlss")) {
        QJsonObject dlss = json["dlss"].toObject();
        config.dlss.enabled = dlss["enabled"].toBool(false);
        config.dlss.quality = static_cast<DLSSConfig::Quality>(dlss["quality"].toInt(2));
        config.dlss.sharpness = dlss["sharpness"].toDouble(0.0);
    }

    if (json.contains("reflex")) {
        QJsonObject reflex = json["reflex"].toObject();
        config.reflex.enabled = reflex["enabled"].toBool(false);
        config.reflex.mode = static_cast<ReflexConfig::Mode>(reflex["mode"].toInt(1));
    }

    if (json.contains("nis")) {
        QJsonObject nis = json["nis"].toObject();
        config.nis.enabled = nis["enabled"].toBool(false);
        config.nis.quality = static_cast<NISConfig::Quality>(nis["quality"].toInt(1));
        config.nis.sharpness = nis["sharpness"].toDouble(0.5);
    }

    configure(config);
}

} // namespace ks::engine::graphics
