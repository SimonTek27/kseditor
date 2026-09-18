#pragma once

#include "../EngineModule.h"
#include <QObject>
#include <QVector>
#include <QVector3D>
#include <QVector4D>
#include <QMatrix4x4>
#include <QUuid>
#include <QJsonObject>
#include <vulkan/vulkan.h>

namespace ks::engine::graphics {

struct DecalConfig {
    int maxDecals = 1000;
    bool enableDeferredDecals = true;
    bool enableProjectorDecals = false;
    bool enableDeferredBlending = true;
    float decalFadeDistance = 100.0f;
    float normalBias = 0.01f;
    int maxDecalsPerObject = 16;
};

struct Decal {
    QUuid id;
    QString name;
    QString diffuseTexture;
    QString normalTexture;
    QString roughnessTexture;
    QString metallicTexture;
    QString emissiveTexture;
    QMatrix4x4 transform;
    QMatrix4x4 inverseTransform;
    QVector3D position;
    QVector3D size = {1, 1, 1};
    QVector3D rotation;
    QVector4D colorTint = {1, 1, 1, 1};
    float opacity = 1.0f;
    float projectionDepth = 1.0f;
    bool visible = true;
    bool castShadows = false;
    bool receiveShadows = false;
    int renderOrder = 0;
    enum class ProjectionMode { Box, Sphere, Projector };
    ProjectionMode projectionMode = ProjectionMode::Box;
    enum class BlendMode { Alpha, Additive, Multiply, Normal };
    BlendMode blendMode = BlendMode::Alpha;
    float fadeDistance = 0.0f;
    bool fadeByDistance = false;
    bool wearDecal = false;
    float wearAmount = 0.0f;
    bool damageDecal = false;
    float damageAmount = 0.0f;
    bool dirtDecal = false;
    float dirtAmount = 0.0f;
    bool paintDecal = false;
    QVector4D paintColor;
};

struct DecalBucket {
    QVector<Decal> decals;
    bool dirty = true;
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexMemory = VK_NULL_HANDLE;
    int indexCount = 0;
};

class DecalSystem : public QObject, public EngineModule {
    Q_OBJECT
public:
    static DecalSystem& instance() { static DecalSystem s; return s; }

    QString moduleName() const override { return "DecalSystem"; }
    QString moduleId() const override { return "ks.decal"; }
    bool initialize() override;
    void shutdown() override;

    void configure(const DecalConfig& config);
    const DecalConfig& config() const { return m_config; }

    QUuid addDecal(const Decal& decal);
    void removeDecal(QUuid id);
    void updateDecal(QUuid id, const Decal& decal);
    Decal* getDecal(QUuid id);
    const Decal* getDecal(QUuid id) const;
    int decalCount() const { return m_allDecals.size(); }
    void clearDecals();

    void setDecalPosition(QUuid id, const QVector3D& position);
    void setDecalOpacity(QUuid id, float opacity);
    void setDecalVisible(QUuid id, bool visible);

    QVector<Decal> getDecalsInRadius(const QVector3D& center, float radius) const;
    QVector<Decal> getDecalsOnObject(const QString& objectName) const;

    void addWearDecal(const QVector3D& position, const QVector3D& normal, float amount);
    void addDamageDecal(const QVector3D& position, const QVector3D& normal, float amount);
    void addDirtDecal(const QVector3D& position, const QVector3D& normal, float amount);
    void addPaintDecal(const QVector3D& position, const QVector3D& normal, const QVector4D& color);

    void update(float deltaTime, const QVector3D& cameraPosition);

    bool createResources(VkDevice device, VkPhysicalDevice physDev);
    void destroyResources(VkDevice device);
    void renderDecals(VkCommandBuffer cmd);

    void updateDecalBuffer(int bucketIndex);

signals:
    void decalAdded(QUuid id);
    void decalRemoved(QUuid id);
    void decalChanged(QUuid id);
    void decalsCleared();

private:
    void buildDecalGeometry(const Decal& decal, QVector<QVector3D>& vertices, QVector<quint32>& indices);
    void updateAllBuckets();
    int bucketIndex(const Decal& decal) const;

    DecalConfig m_config;
    QVector<Decal> m_allDecals;
    QVector<DecalBucket> m_buckets;
    QVector<QVector3D> m_quadVertices;
    QVector<quint32> m_quadIndices;
};

} // namespace ks::engine::graphics
