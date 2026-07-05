#pragma once

#include "VulkanRenderer.h"
#include "VulkanShaderLoader.h"
#include "ShaderParamRegistry.h"
#include "../Config/KsConfigIntegration.h"
#include "../Config/KsConfigLoader.h"
#include "../Config/PPFilterPreset.h"
#include <QMap>
#include <QString>

class QVulkanWindow;

namespace ks { namespace graphics {
    class PPFilterRenderer;
    class VulkanShaderLoader;
} }

class KsVulkanIntegration : public QObject
{
    Q_OBJECT

public:
    static KsVulkanIntegration* instance();

    bool initialize(const QString& systemPath);

    bool isInitialized() const { return m_initialized; }

    QString systemPath() const { return m_systemPath; }

    ks::graphics::VulkanRenderer* renderer() { return m_renderer; }
    ks::graphics::PPFilterRenderer* ppFilterRenderer() { return m_ppFilterRenderer; }

    KsConfigLoader& configLoader() { return KsConfigLoader::instance(); }
    ks::graphics::ShaderParamRegistry& shaderRegistry() { return ks::graphics::ShaderParamRegistry::instance(); }

    const KsConfigLoader::GraphicsSettings& graphicsSettings() const {
        return KsConfigLoader::instance().graphicsSettings();
    }
    const KsConfigLoader::LightingSettings& lightingSettings() const {
        return KsConfigLoader::instance().lightingSettings();
    }

    bool loadKsShader(const QString& shaderName, VkShaderStageFlagBits stage);

    QString shaderDir() const { return m_systemPath + "/shaders"; }

    QString getShaderPath(const QString& shaderName, VkShaderStageFlagBits stage) const;

    void applyGraphicsSettings();

    void applyLightingSettings();

    void applyPPFilterPreset(const PPFilterPreset* preset);

    const PPFilterPreset* currentPPPreset() const { return m_currentPPPreset; }

    const QMap<QString, PPFilterPreset*>& ppFilterPresets() const { return m_ppPresets; }

    void bindMaterial(const ks::graphics::VulkanShaderLoader::MaterialParams& params);

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

    ks::graphics::VulkanRenderer* m_renderer = nullptr;
    ks::graphics::PPFilterRenderer* m_ppFilterRenderer = nullptr;

    PPFilterPreset* m_currentPPPreset = nullptr;
    QMap<QString, PPFilterPreset*> m_ppPresets;

    QMap<QString, QString> m_shaderPathMap;
};
