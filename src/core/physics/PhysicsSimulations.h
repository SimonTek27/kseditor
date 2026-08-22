#pragma once

#include "PhysicsEngine.h"
#include <QVector3D>
#include <QVector>
#include <QPair>

namespace ks {
namespace physics {

// ============================================================================
// Soft Body Physics Implementation
// ============================================================================

struct SoftBodySimulator : public QObject {
    Q_OBJECT

public:
    explicit SoftBodySimulator(QObject* parent = nullptr);
    ~SoftBodySimulator();

    void setMesh(const QVector<QVector3D>& vertices, const QVector<int>& faces);
    void setConfig(const SoftBody& config);

    void simulate(int frameStep);

    QVector<QVector3D> getPositions() const { return m_positions; }

    void addForce(const QVector3D& force);
    void setGravity(const QVector3D& gravity);
    void setWind(const QVector3D& wind, float noise = 0.0f);

    void pinVertex(int index);
    void unpinVertex(int index);
    void pinVertexGroup(const QVector<int>& indices);

    void reset();

signals:
    void simulationStep(int step);

private:
    void buildConstraints();
    void satisfyConstraints(int iterations);
    void applyForces(float deltaTime);
    void resolveCollisions();

    QVector<QVector3D> m_positions;
    QVector<QVector3D> m_velocities;
    QVector<int> m_faces;
    QVector<QPair<int, int>> m_edges;
    QVector<float> m_restLengths;
    QVector<int> m_pinnedVertices;

    SoftBody m_config;
    QVector3D m_externalForce;
    QVector3D m_wind;
    QVector3D m_gravity = QVector3D(0, -9.81f, 0);
};

// ============================================================================
// Cloth Physics Implementation
// ============================================================================

struct ClothSimulator : public QObject {
    Q_OBJECT

public:
    explicit ClothSimulator(QObject* parent = nullptr);
    ~ClothSimulator();

    void setCloth(Cloth* cloth);
    void simulate(float deltaTime);

    void addCollisionSphere(const QVector3D& center, float radius);
    void addCollisionBox(const QVector<QVector3D>& corners);
    void clearCollisions();

    void setPinnedVertices(const QVector<int>& indices);

signals:
    void clothUpdated();

private:
    void integrateVerlet(float deltaTime);
    void satisfyConstraints(float deltaTime);
    void satisfyCollisionConstraints();

    Cloth* m_cloth = nullptr;
    QVector<QVector3D> m_positions;
    QVector<QVector3D> m_previousPositions;

    struct CollisionSphere {
        QVector3D center;
        float radius;
    };
    QVector<CollisionSphere> m_collisionSpheres;

    struct CollisionBox {
        QVector<QVector3D> corners;
        QVector3D min;
        QVector3D max;
    };
    QVector<CollisionBox> m_collisionBoxes;
};

// ============================================================================
// Particle System Implementation
// ============================================================================

struct ParticleSystemSimulator : public QObject {
    Q_OBJECT

public:
    explicit ParticleSystemSimulator(QObject* parent = nullptr);
    ~ParticleSystemSimulator();

    void setEmitter(const ParticleSystem::Emitter& emitter);
    void setPhysics(const ParticleSystem::Physics& physics);
    void setMaxCount(int count);
    void setLifetime(int frames);

    void simulate(float deltaTime);

    const QVector<ParticleSystem::Particle>& particles() const { return m_particles; }

signals:
    void particlesUpdated();

private:
    QVector<ParticleSystem::Particle> m_particles;
    ParticleSystem::Emitter m_emitter;
    ParticleSystem::Physics m_physics;
    int m_maxCount = 1000;
    int m_lifetime = 100;
    int m_framesSinceEmit = 0;
};

// ============================================================================
// Fluid Physics Implementation
// ============================================================================

struct FluidSimulator : public QObject {
    Q_OBJECT

public:
    FluidSimulator(QObject* parent = nullptr);
    ~FluidSimulator();

    enum class SimulationMode { Off, Particles, Grid };
    SimulationMode mode = SimulationMode::Particles;

    struct FluidParticle {
        QVector3D position;
        QVector3D velocity;
        float density;
        float pressure;
    };

    float gravity[3] = {0, -9.81f, 0};
    float viscosity = 0.0f;
    float stiffness = 0.0f;
    float restDensity = 1000.0f;

    float smoothingRadius = 0.2f;
    int targetNumDensity = 64;

    float timeScale = 1.0f;
    int iterations = 2;

    QVector<FluidParticle> fluidParticles;

    void addParticles(const QVector<QVector3D>& positions);
    void simulate(float deltaTime);

    void setBoundaries(const QVector<QVector3D>& boundaries);

    float getParticleDensity(int index) const;
    float getParticlePressure(int index) const;

signals:
    void fluidUpdated();

private:
    float computeDensity(const QVector3D& position);
    QVector3D computeDensityGradient(const QVector3D& position);
    void computePressures();

    QVector<QVector3D> m_boundaries;
};

// ============================================================================
// Hair Physics Implementation
// ============================================================================

struct HairSystem : public QObject {
    Q_OBJECT

public:
    HairSystem(QObject* parent = nullptr);
    ~HairSystem();

    struct HairStrand {
        QVector<QVector3D> points;
        QVector<QVector3D> velocities;
        int rootVertex;
    };

    QVector<HairStrand> strands;

    float segmentLength = 0.1f;
    int segments = 5;

    float gravity[3] = {0, 0, 0};
    float dynamics = 0.5f;
    float damping = 0.1f;

    bool useCollision = true;
    float collisionRadius = 0.02f;

    void addStrand(int rootVertex, int count);
    void removeStrand(int index);

    void simulate(float deltaTime);

    QVector<QVector3D> getCompletedStrands() const;

signals:
    void hairUpdated();

private:
    void simulateStrand(HairStrand& strand, float deltaTime);

    QVector<int> m_rootVertices;
};

} // namespace physics
} // namespace ks