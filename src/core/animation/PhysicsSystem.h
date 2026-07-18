#pragma once
#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QMatrix4x4>

namespace ks {
namespace physics {

struct RigidBodyData {
    enum class Type { Static, Dynamic, Kinematic };
    Type bodyType = Type::Dynamic;

    float mass = 1.0f;
    float friction = 0.5f;
    float restitution = 0.3f;
    float linearDamping = 0.01f;
    float angularDamping = 0.01f;

    bool collisionEnabled = true;
    bool deformationsEnabled = true;

    struct CollisionShape {
        enum ShapeType { Sphere, Box, Capsule, Cylinder, ConvexHull, Mesh };
        ShapeType shapeType;
        QVector3D dimensions = {1, 1, 1};
        float radius = 0.5f;
    } collisionShape;

    QVector3D linearVelocity;
    QVector3D angularVelocity;

    bool isActive() const { return bodyType != Type::Static; }
};

struct SoftBody {
    enum class Config {
        Mass,
        Stiffness,
        Damping,
        Pressure,
        Volume
    };

    float mass = 1.0f;
    float stiffness = 0.5f;
    float damping = 0.01f;
    float pressure = 0.0f;
    float volume = 0.0f;

    bool useBendConstraints = true;
    bool useShapeConstraints = true;
    bool usePressure = false;

    int iterationCount = 5;
    float collisionMargin = 0.02f;
};

struct Cloth {
    struct Vertex {
        QVector3D position;
        QVector3D previousPosition;
        QVector3D acceleration;
        float mass = 1.0f;
        bool pinned = false;
    };

    struct Constraint {
        int vertex1;
        int vertex2;
        float restLength;
        float stiffness = 1.0f;
    };

    QVector<Vertex> vertices;
    QVector<Constraint> constraints;

    float gravity[3] = {0, -9.81f, 0};
    float structural = 1.0f;
    float shear = 1.0f;
    float bending = 0.1f;

    bool useDynamicMesh = true;
    float velocitySmooth = 0.9f;
    float damping = 0.01f;

    bool useCustomPhysics = false;
    int solverType = 0;

    QVector3D wind;
    float windNoise = 0.0f;
};

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
};

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

    Cloth* m_cloth;
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

struct ParticleSystem {
    public:
    struct Particle {
        QVector3D position;
        QVector3D velocity;
        QVector3D acceleration;
        float lifetime;
        float age;
        float mass;
        float size;
        QVector4D color;

        bool isAlive() const { return age < lifetime; }
    };

    struct Emitter {
        QVector3D position;
        QVector3D direction;
        float angle = 0.0f;
        float velocity = 1.0f;
        float rate = 100.0f;

        enum class Shape { Point, Circle, Sphere, Plane };
        Shape shape = Shape::Point;
    };

    struct Physics {
        float gravity[3] = {0, -9.81f, 0};
        float damping = 0.0f;
        float brownian = 0.0f;

        bool useWind = false;
        float wind[3] = {0, 0, 0};
    };

    QVector<Particle> particles;
    Emitter emitter;
    Physics physics;

    int maxCount = 1000;
    int lifetime = 100;

    void emitParticles(int count);
    void update(float deltaTime);
    void clear();

    QVector4D colorRamp(float t);
};

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

}
}