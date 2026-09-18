#pragma once

#include "../EngineModule.h"
#include <QObject>
#include <QVector>
#include <QVector3D>
#include <QVector4D>
#include <QMatrix4x4>
#include <QUuid>
#include <QJsonObject>

namespace ks::engine::graphics {

struct VegetationType {
    QString name;
    QString meshPath;
    QString diffuseTexture;
    QString normalTexture;
    float windResponse = 1.0f;
    float windAmplitude = 0.3f;
    float windFrequency = 1.5f;
    float windAnimationBlend = 0.5f;
    bool isTree = true;
    bool isGrass = false;
    bool isBush = false;
    float minScale = 0.8f;
    float maxScale = 1.2f;
    bool castShadows = true;
    bool receiveShadows = true;
    int renderLayer = 0;
};

struct VegetationInstance {
    QUuid typeId;
    QVector3D position;
    QVector3D rotation;
    QVector3D scale = {1, 1, 1};
    float randomSeed = 0.0f;
    bool visible = true;
};

struct VegetationLOD {
    float distance = 50.0f;
    float speedTreeLOD = 0.5f;
    bool billboard = false;
    float billboardStart = 200.0f;
    bool impostor = false;
};

struct VegetationConfig {
    int maxInstancesPerType = 50000;
    int maxTotalInstances = 200000;
    bool enableLOD = true;
    bool enableFrustumCulling = true;
    bool enableOcclusionCulling = false;
    bool enableInstancing = true;
    int instancesPerDrawCall = 1024;
    bool enableWindAnimation = true;
    float globalWindStrength = 1.0f;
    float globalWindDirection = 0.0f;
    bool enableInteraction = true;
    float interactionRadius = 2.0f;
    float interactionStrength = 0.5f;
    float grassDensityFactor = 1.0f;
    bool enableDistanceCulling = true;
    float maxRenderDistance = 500.0f;
    bool enableSeasonalColors = false;
};

struct VegetationBucket {
    QVector<VegetationInstance> instances;
    int typeIndex = -1;
    bool dirty = true;
};

class VegetationSystem : public QObject, public EngineModule {
    Q_OBJECT
public:
    static VegetationSystem& instance() { static VegetationSystem s; return s; }

    QString moduleName() const override { return "VegetationSystem"; }
    QString moduleId() const override { return "ks.vegetation"; }
    bool initialize() override;
    void shutdown() override;

    void configure(const VegetationConfig& config);
    const VegetationConfig& config() const { return m_config; }

    int addVegetationType(const VegetationType& type);
    void removeVegetationType(int index);
    const QVector<VegetationType>& vegetationTypes() const { return m_types; }

    QUuid addInstance(int typeIndex, const QVector3D& position, const QVector3D& rotation,
                      const QVector3D& scale);
    void removeInstance(QUuid instanceId);
    void clearInstances();
    void clearInstancesOfType(int typeIndex);

    void addInstancesBatch(int typeIndex, const QVector<VegetationInstance>& instances);
    int instanceCount(int typeIndex) const;
    int totalInstanceCount() const;

    void setInstancePosition(QUuid instanceId, const QVector3D& position);
    void setInstanceVisible(QUuid instanceId, bool visible);

    void update(float deltaTime, const QVector3D& cameraPosition, const QVector3D& windDir);

    void fillArea(int typeIndex, const QVector3D& center, const QVector3D& size,
                  float density, const std::function<float(const QVector3D&)>& heightFunc);
    void removeInArea(const QVector3D& center, const QVector3D& size);

    QVector<VegetationInstance> getVisibleInstances() const;
    QVector<VegetationInstance> getInstancesInRadius(const QVector3D& center, float radius) const;

    void setWindDirection(const QVector3D& dir) { m_windDirection = dir.normalized(); }
    void setWindStrength(float strength) { m_config.globalWindStrength = strength; }

    float windPhase() const { return m_windPhase; }
    QVector3D windDirection() const { return m_windDirection; }

signals:
    void instanceAdded(QUuid id, int typeIndex);
    void instanceRemoved(QUuid id);
    void instancesChanged();
    void windChanged();

private:
    struct LODConfig {
        float distance;
        float scale;
        bool billboard;
    };

    void updateLODs(const QVector3D& cameraPosition);
    void buildRenderBuckets();
    int findNearestType(const QVector3D& position) const;

    VegetationConfig m_config;
    QVector<VegetationType> m_types;
    QVector<VegetationBucket> m_buckets;
    QVector<VegetationLOD> m_lodLevels;
    QVector<VegetationInstance> m_allInstances;
    QVector3D m_windDirection = {1, 0, 0};
    float m_windPhase = 0.0f;
};

} // namespace ks::engine::graphics
