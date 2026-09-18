#pragma once

#include "../EngineModule.h"
#include <QObject>
#include <QVector>
#include <QVector3D>
#include <QVector4D>
#include <QMatrix4x4>
#include <QJsonObject>
#include <vulkan/vulkan.h>

namespace ks::engine::graphics {

struct WaveLayer {
    float amplitude = 0.5f;
    float wavelength = 10.0f;
    float speed = 1.0f;
    QVector2D direction = {1.0f, 0.0f};
    float steepness = 0.5f;
};

struct WaterConfig {
    float seaLevel = 0.0f;
    float tileSize = 256.0f;
    int tileCount = 8;
    int meshResolution = 64;
    QVector3D deepColor = {0.0f, 0.05f, 0.15f};
    QVector3D shallowColor = {0.0f, 0.25f, 0.35f};
    QVector3D foamColor = {0.9f, 0.95f, 1.0f};
    QVector3D reflectionTint = {0.8f, 0.85f, 0.9f};
    float opacity = 0.9f;
    float fresnelPower = 5.0f;
    float refractionStrength = 0.1f;
    float reflectivity = 0.5f;
    float shininess = 256.0f;
    float foamThreshold = 0.6f;
    float foamIntensity = 0.8f;
    float shoreFoamWidth = 5.0f;
    float shorelineFoamIntensity = 1.0f;
    float causticsIntensity = 0.3f;
    float causticsScale = 50.0f;
    float depthFade = 10.0f;
    float maxDepth = 50.0f;
    bool enableReflections = true;
    bool enableRefractions = true;
    bool enableFoam = true;
    bool enableCaustics = true;
    bool enableDepthFog = true;
    bool enableFlowMap = false;
    float normalStrength = 1.0f;
    float texScale = 0.02f;
    float timeSpeed = 1.0f;
};

struct WaterTile {
    int tileX = 0;
    int tileZ = 0;
    float distanceToCamera = 0.0f;
    int lodLevel = 0;
    bool visible = true;
};

class WaterSystem : public QObject, public EngineModule {
    Q_OBJECT
public:
    static WaterSystem& instance() { static WaterSystem s; return s; }

    QString moduleName() const override { return "WaterSystem"; }
    QString moduleId() const override { return "ks.water"; }
    bool initialize() override;
    void shutdown() override;

    void configure(const WaterConfig& config);
    const WaterConfig& config() const { return m_config; }

    void addWaveLayer(const WaveLayer& wave);
    void removeWaveLayer(int index);
    void clearWaveLayers();
    const QVector<WaveLayer>& waveLayers() const { return m_waveLayers; }

    float getWaterHeight(float x, float z, float time) const;
    QVector3D getWaterNormal(float x, float z, float time) const;
    QVector3D getWaterVelocity(float x, float z, float time) const;
    bool isUnderwater(const QVector3D& position) const;
    float getDepth(const QVector3D& position) const;
    bool isShoreline(const QVector3D& position, float time, float threshold = 0.1f) const;

    float getFoamIntensity(float x, float z, float time) const;
    QVector3D getFoamUV(float x, float z, float time) const;

    void setTime(float time) { m_time = time; }
    float time() const { return m_time; }

    void setSeaLevel(float level) { m_config.seaLevel = level; }
    float seaLevel() const { return m_config.seaLevel; }

    void update(float deltaTime, const QVector3D& cameraPosition);
    void getVisibleTiles(const QVector3D& cameraPos, QVector<WaterTile>& visibleTiles) const;

    VkPipeline createWaterPipeline(VkDevice device, VkRenderPass renderPass, VkPipelineLayout layout);
    void updateWaterUniforms(VkCommandBuffer cmd, const QMatrix4x4& view, const QMatrix4x4& proj,
                             const QVector3D& cameraPos, const QVector3D& sunDir, const QVector3D& sunColor);

signals:
    void waterConfigChanged();
    void waveAdded(int index);
    void waveRemoved(int index);

private:
    void buildTileMesh(int resolution);
    void buildAllTiles();

    WaterConfig m_config;
    QVector<WaveLayer> m_waveLayers;
    QVector<WaterTile> m_tiles;
    float m_time = 0.0f;
    QVector<QVector3D> m_tileVertices;
    QVector<QVector3D> m_tileNormals;
    QVector<QVector2D> m_tileUVs;
    QVector<quint32> m_tileIndices;
    bool m_meshBuilt = false;
};

} // namespace ks::engine::graphics
