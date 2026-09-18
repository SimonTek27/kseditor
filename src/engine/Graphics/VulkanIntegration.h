#pragma once

#include "VulkanRenderer.h"
#include "VulkanShaderLoader.h"
#include "ShaderParamRegistry.h"
#include "SSGI/SSGIRenderer.h"
#include <QMap>
#include <QString>

class QVulkanWindow;
class PPFilterPreset;

namespace ks {
    class PPFilterRenderer;
    class VulkanShaderLoader;
    class VulkanRenderer;
    class ShaderParamRegistry;
}

class KsVulkanIntegration : public QObject
{
    Q_OBJECT

public:
    struct GraphicsSettings {
        int maxFrameLatency = 0;
        float mipLodBias = 0.0f;
        float shadowMapBias0 = 0.000002f;
        float shadowMapBias1 = 0.000015f;
        float shadowMapBias2 = 0.0003f;
        float skyboxReflectionGain = 1.5f;
        bool allowUnsupportedDX10 = false;
    };

    struct LightingSettings {
        float ambientColor[3] = {};
        float ambientIntensity = 100.0f;
        float sunDirection[3] = {};
        float sunIntensity = 100.0f;
        float sunColor[3] = {};
        float hdrExposure = 1.0f;
        int toneMapping = 0;
    };

    static KsVulkanIntegration* instance();

    bool initialize(const QString& systemPath);

    bool isInitialized() const { return m_initialized; }

    QString systemPath() const { return m_systemPath; }

    ks::VulkanRenderer* renderer() { return m_renderer; }
    ks::PPFilterRenderer* ppFilterRenderer() { return m_ppFilterRenderer; }

    ks::ShaderParamRegistry& shaderRegistry() { return ks::ShaderParamRegistry::instance(); }

    const GraphicsSettings& graphicsSettings() const { return m_graphicsSettings; }
    const LightingSettings& lightingSettings() const { return m_lightingSettings; }

    void setGraphicsSettings(const GraphicsSettings& s) { m_graphicsSettings = s; }
    void setLightingSettings(const LightingSettings& s) { m_lightingSettings = s; }

    bool loadKsShader(const QString& shaderName, VkShaderStageFlagBits stage);

    QString shaderDir() const { return m_systemPath + "/shaders"; }

    QString getShaderPath(const QString& shaderName, VkShaderStageFlagBits stage) const;

    void applyGraphicsSettings();

    void applyLightingSettings();

    void applyPPFilterPreset(const PPFilterPreset* preset);

    const PPFilterPreset* currentPPPreset() const { return m_currentPPPreset; }

    const QMap<QString, PPFilterPreset*>& ppFilterPresets() const { return m_ppPresets; }

    ks::SSGIRenderer* ssgiRenderer() { return m_ssgiRenderer; }
    void setSSGIEnabled(bool enabled) { m_ssgiEnabled = enabled; }
    bool isSSGIEnabled() const { return m_ssgiEnabled; }

    void bindMaterial(const ks::VulkanShaderLoader::MaterialParams& params);

    void bindCamera(const QMatrix4x4& view, const QMatrix4x4& projection,
                   const QVector3D& cameraPos, float nearPlane = 0.1f, float farPlane = 1000.0f);

    QString getConfigPath(const QString& filename) const {
        return m_systemPath + "/cfg/" + filename;
    }

    QString ppFiltersDir() const { return m_systemPath + "/cfg/ppfilters"; }

    QVariant getShaderParam(const QString& shader, const QString& param,
                           VkShaderStageFlagBits stage = VK_SHADER_STAGE_FRAGMENT_BIT) const;

    void setGlobalParam(const QString& name, const QVariant& value);

    QVariant getGlobalParam(const QString& name) const;

signals:
    void initialized(bool success);
    void graphicsSettingsChanged();
    void lightingSettingsChanged();
    void ppFilterPresetChanged(const QString& presetName);
    void shaderLoaded(const QString& shaderName);
    void shaderLoadFailed(const QString& shaderName, const QString& error);

private:
    explicit KsVulkanIntegration(QObject* parent = nullptr);
    ~KsVulkanIntegration();

    KsVulkanIntegration(const KsVulkanIntegration&) = delete;
    KsVulkanIntegration& operator=(const KsVulkanIntegration&) = delete;

    bool loadPPFilterPresets();
    bool setupShaderDirectories();

    static KsVulkanIntegration* s_instance;

    bool m_initialized = false;
    QString m_systemPath;
    QString m_shadersDir;
    QString m_spirvShadersDir;

    ks::VulkanRenderer* m_renderer = nullptr;
    ks::PPFilterRenderer* m_ppFilterRenderer = nullptr;

    GraphicsSettings m_graphicsSettings;
    LightingSettings m_lightingSettings;

    PPFilterPreset* m_currentPPPreset = nullptr;
    QMap<QString, PPFilterPreset*> m_ppPresets;

    QMap<QString, QString> m_shaderPathMap;
};
