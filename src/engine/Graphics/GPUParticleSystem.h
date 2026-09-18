#pragma once

#include "../EngineModule.h"
#include <QObject>
#include <QVector>
#include <QVector3D>
#include <QVector4D>
#include <QMatrix4x4>
#include <QUuid>
#include <QJsonObject>
#include <functional>

namespace ks::engine::graphics {

struct GPUParticle {
    QVector3D position;
    QVector3D velocity;
    QVector3D acceleration;
    QVector4D color;
    float size = 1.0f;
    float life = 1.0f;
    float maxLife = 1.0f;
    float rotation = 0.0f;
    float rotationSpeed = 0.0f;
    float drag = 0.0f;
    uint32_t alive = 1;
    uint32_t pad[2] = {0, 0};
};

struct ParticleEmitterConfig {
    enum class Shape { Point, Sphere, Cone, Box, Hemisphere, Torus };
    enum class SpawnMode { Burst, Rate };

    Shape shape = Shape::Sphere;
    SpawnMode spawnMode = SpawnMode::Rate;
    int burstCount = 100;
    float rate = 500.0f;
    float emissionTime = 0.0f;

    QVector3D position;
    QVector3D direction = {0, 1, 0};
    float spreadAngle = 45.0f;
    float initialVelocity = 5.0f;
    float initialVelocityVariance = 1.0f;

    float lifetime = 3.0f;
    float lifetimeVariance = 0.5f;

    float startSize = 0.1f;
    float endSize = 0.0f;
    float sizeVariance = 0.02f;

    QVector4D startColor = {1, 1, 1, 1};
    QVector4D endColor = {1, 1, 1, 0};
    bool useColorOverLife = false;

    float gravity = -9.81f;
    float drag = 0.1f;
    float turbulence = 0.0f;

    bool worldSpace = true;
    bool followEmitter = false;
    int maxParticles = 10000;
    int sortMode = 0;
};

struct ParticleRendererConfig {
    bool enableDistanceCulling = true;
    float maxRenderDistance = 500.0f;
    bool enableFrustumCulling = true;
    bool sortBackToFront = true;
    bool softParticles = true;
    float softParticleDepth = 1.0f;
    bool receiveShadows = false;
    bool castShadows = false;
};

class GPUParticleSystem : public QObject, public EngineModule {
    Q_OBJECT
public:
    static GPUParticleSystem& instance() { static GPUParticleSystem s; return s; }

    QString moduleName() const override { return "GPUParticleSystem"; }
    QString moduleId() const override { return "ks.gpuparticles"; }
    bool initialize() override;
    void shutdown() override;

    QUuid createEmitter(const ParticleEmitterConfig& config);
    void destroyEmitter(QUuid id);
    void setEmitterConfig(QUuid id, const ParticleEmitterConfig& config);
    ParticleEmitterConfig* emitterConfig(QUuid id);

    void startEmission(QUuid id);
    void stopEmission(QUuid id);
    void resetEmitter(QUuid id);

    void setEmitterPosition(QUuid id, const QVector3D& position);
    void setEmitterDirection(QUuid id, const QVector3D& direction);

    void update(float deltaTime);
    void render(const QMatrix4x4& view, const QMatrix4x4& projection, const QVector3D& cameraPos);

    int activeParticleCount() const { return m_activeCount; }
    int totalEmitterCount() const { return m_emitters.size(); }

    const QVector<GPUParticle>& particles() const { return m_particles; }

    void setGravity(const QVector3D& g) { m_gravity = g; }
    QVector3D gravity() const { return m_gravity; }

    void setWind(const QVector3D& w) { m_wind = w; }
    QVector3D wind() const { return m_wind; }

    QJsonObject serialize() const;
    bool deserialize(const QJsonObject& data);

signals:
    void emitterCreated(QUuid id);
    void emitterDestroyed(QUuid id);
    void particleCountChanged(int count);

private:
    struct Emitter {
        QUuid id;
        ParticleEmitterConfig config;
        bool active = false;
        float emissionAccumulator = 0.0f;
        float time = 0.0f;
        int particleOffset = 0;
        int particleCount = 0;
    };

    void emitParticles(Emitter& emitter, int count);
    void updateParticles(float deltaTime);
    void sortParticles(const QVector3D& cameraPos);
    int findFreeSlot();
    GPUParticle createParticle(const ParticleEmitterConfig& config, const Emitter& emitter);

    QVector<GPUParticle> m_particles;
    QVector<Emitter> m_emitters;
    int m_activeCount = 0;
    int m_maxTotalParticles = 100000;
    QVector3D m_gravity = {0, -9.81f, 0};
    QVector3D m_wind = {0, 0, 0};
    ParticleRendererConfig m_renderConfig;
};

} // namespace ks::engine::graphics
