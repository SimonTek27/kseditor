#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QVector3D>
#include <QMatrix4x4>
#include "dt_DifferentialModel.h"
#include "mech_BrakeThermalModel.h"

namespace ks {

namespace physics {

class RigidBody : public QObject
{
    Q_OBJECT
public:
    explicit RigidBody(QObject* parent = nullptr) : QObject(parent) {}
    ~RigidBody() {}

    void setMass(float mass) { m_mass = mass; }
    float mass() const { return m_mass; }

    void setPosition(const QVector3D& pos) { m_position = pos; }
    QVector3D position() const { return m_position; }

    void setRotation(const QVector3D& euler) { m_rotation = euler; }
    QVector3D rotation() const { return m_rotation; }

    void setVelocity(const QVector3D& vel) { m_velocity = vel; }
    QVector3D velocity() const { return m_velocity; }

    void setAngularVelocity(const QVector3D& angVel) { m_angularVelocity = angVel; }
    QVector3D angularVelocity() const { return m_angularVelocity; }

    void applyForce(const QVector3D& force, const QVector3D& point = QVector3D());
    void applyImpulse(const QVector3D& impulse, const QVector3D& point = QVector3D());

    void integrate(float dt);

    float kineticEnergy() const;
    float potentialEnergy(float gravity = 9.81f) const;

signals:
    void positionChanged();
    void velocityChanged();

private:
    float m_mass = 1.0f;
    QVector3D m_position;
    QVector3D m_rotation;
    QVector3D m_velocity;
    QVector3D m_angularVelocity;
    QVector3D m_accumulatedForce;
    QVector3D m_accumulatedTorque;
    QVector3D m_gravity = QVector3D(0, -9.81f, 0);
};

class CollisionShape : public QObject
{
    Q_OBJECT
public:
    explicit CollisionShape(QObject* parent = nullptr) : QObject(parent) {}
    ~CollisionShape() {}

    enum ShapeType { Box, Sphere, Capsule, Cylinder, Cone, ConvexHull, Compound };

    void setType(ShapeType type) { m_type = type; }
    ShapeType type() const { return m_type; }

    void setDimensions(const QVector3D& dims) { m_dimensions = dims; }
    QVector3D dimensions() const { return m_dimensions; }

    void setMargin(float margin) { m_margin = margin; }
    float margin() const { return m_margin; }

    float computeVolume() const;
    QVector3D computeInertia() const;

signals:
    void shapeModified();

private:
    ShapeType m_type = Box;
    QVector3D m_dimensions = {1, 1, 1};
    float m_margin = 0.01f;
};

class PhysicsWorld : public QObject
{
    Q_OBJECT
public:
    explicit PhysicsWorld(QObject* parent = nullptr) : QObject(parent) {}
    ~PhysicsWorld() {}

    void setGravity(const QVector3D& g) { m_gravity = g; }
    QVector3D gravity() const { return m_gravity; }

    void setSolverIterations(int iterations) { m_solverIterations = iterations; }
    int solverIterations() const { return m_solverIterations; }

    void setFixedTimeStep(float dt) { m_fixedTimeStep = dt; }
    float fixedTimeStep() const { return m_fixedTimeStep; }

    RigidBody* createBody(float mass);
    void addBody(RigidBody* body);
    void removeBody(RigidBody* body);
    QVector<RigidBody*> allBodies() const { return m_bodies; }

    void stepSimulation(float deltaTime);
    void debugDraw();

    enum BroadphaseType { Simple, SAP, DBVT };

    void setBroadphase(BroadphaseType type) { m_broadphase = type; }

signals:
    void stepCompleted();

private:
    void solveConstraints();

    QVector3D m_gravity = {0, -9.81f, 0};
    int m_solverIterations = 10;
    float m_fixedTimeStep = 1.0f / 60.0f;
    QVector<RigidBody*> m_bodies;
    BroadphaseType m_broadphase = SAP;
};

class CarPhysics : public QObject
{
    Q_OBJECT
public:
    explicit CarPhysics(QObject* parent = nullptr) : QObject(parent) {}
    ~CarPhysics() {}

    struct CarConfig {
        float mass = 1200.0f;
        float wheelbase = 2.7f;
        float trackWidth = 1.5f;
        float cgHeight = 0.5f;
        float frontWeightBias = 0.5f;
        float rearWeightBias = 0.5f;
        float yawInertia = 1.0f;

        float enginePower = 450.0f;
        float maxTorque = 600.0f;
        float gearRatios[7] = {3.5f, 2.5f, 1.8f, 1.4f, 1.1f, 0.9f, 0.7f};
        float finalDrive = 3.4f;
        float driveLayout = 0.5f; // 0=FWD, 0.5=RWD, 1=AWD

        float brakePower = 15000.0f;
        float handbrakePower = 8000.0f;

        float frontCamber = -1.5f;
        float rearCamber = -1.0f;
        float caster = 3.0f;
        float toe = 0.0f;

        float frontSpring = 50000.0f;
        float rearSpring = 45000.0f;
        float frontDamping = 3000.0f;
        float rearDamping = 2800.0f;
        float frontRideHeight = 0.15f;
        float rearRideHeight = 0.15f;
    };

    void setConfig(const CarConfig& config) { m_config = config; }
    CarConfig config() const { return m_config; }

    void setInput(float throttle, float brake, float steering, bool handbrake);

    void update(float dt);

    float getSpeed() const { return m_speed; }
    float getRPM() const { return m_rpm; }
    int getGear() const { return m_currentGear; }

    float getSlipAngle(bool front) const;
    float getTractionCircle() const;

    void shiftUp();
    void shiftDown();
    void shiftTo(int gear);

    void setDiffModel(const DifferentialModel& model);
    void setBrakeModel(const BrakeThermalModel& brake);
    void setAbsEnabled(bool enabled) { m_absEnabled = enabled; }
    void setTcEnabled(bool enabled) { m_tcEnabled = enabled; }

    struct Telemetry {
        float speed;
        float rpm;
        float throttle;
        float brake;
        float steering;
        float slipAngle;
        float gForce;
        float turboBoost;
        float rollAngle;
        float pitchAngle;
        float brakeTemp;
        float diffLock;
        float wheelSlipFL, wheelSlipFR, wheelSlipRL, wheelSlipRR;
    };

    Telemetry getTelemetry() const;

signals:
    void gearChanged(int gear);
    void telemetryUpdated();

private:
    CarConfig m_config;
    float m_throttle = 0.0f;
    float m_brake = 0.0f;
    float m_steering = 0.0f;
    bool m_handbrake = false;

    float m_speed = 0.0f;
    float m_rpm = 800.0f;
    int m_currentGear = 1;
    float m_clutch = 0.0f;
    float m_lateralVelocity = 0.0f;
    float m_slipAngle = 0.0f;
    float m_gForce = 0.0f;

    float m_yawRate = 0.0f;
    float m_rollAngle = 0.0f;
    float m_pitchAngle = 0.0f;

    float m_wheelSpeedFL = 0.0f, m_wheelSpeedFR = 0.0f;
    float m_wheelSpeedRL = 0.0f, m_wheelSpeedRR = 0.0f;
    float m_wheelSlipFL = 0.0f, m_wheelSlipFR = 0.0f;
    float m_wheelSlipRL = 0.0f, m_wheelSlipRR = 0.0f;

    bool m_absEnabled = false;
    bool m_tcEnabled = false;

    DifferentialModel m_diff;
    BrakeThermalModel m_brakeThermal;
};

class SuspensionModel : public QObject
{
    Q_OBJECT
public:
    explicit SuspensionModel(QObject* parent = nullptr) : QObject(parent) {}
    ~SuspensionModel() {}

    void setSpringConstant(float k) { m_spring = k; }
    float springConstant() const { return m_spring; }

    void setDamping(float c) { m_damping = c; }
    float damping() const { return m_damping; }

    void setRestLength(float length) { m_restLength = length; }
    float restLength() const { return m_restLength; }

    void setTravel(float travel) { m_travel = travel; }
    float travel() const { return m_travel; }

    float computeForce(float currentLength, float velocity);

    void setHardpoints(const QVector3D& upper, const QVector3D& lower) {
        m_upperHardpoint = upper;
        m_lowerHardpoint = lower;
    }

signals:
    void forceChanged();

private:
    float m_spring = 50000.0f;
    float m_damping = 3000.0f;
    float m_restLength = 0.4f;
    float m_travel = 0.2f;
    QVector3D m_upperHardpoint;
    QVector3D m_lowerHardpoint;
};

class TireModel : public QObject
{
    Q_OBJECT
public:
    explicit TireModel(QObject* parent = nullptr) : QObject(parent) {}
    ~TireModel() {}

    void setVerticalLoad(float load) { m_verticalLoad = load; }
    float verticalLoad() const { return m_verticalLoad; }

    void setSlipAngle(float angle) { m_slipAngle = angle; }
    float slipAngle() const { return m_slipAngle; }

    void setSlipRatio(float ratio) { m_slipRatio = ratio; }
    float slipRatio() const { return m_slipRatio; }

    QVector2D computeForces();

    void setFrictionCurve(float peakG, float slideG, float peakSlip, float endSlip) {
        m_peakG = peakG;
        m_slideG = slideG;
        m_peakSlip = peakSlip;
        m_endSlip = endSlip;
    }

signals:
    void forcesComputed();

private:
    float m_verticalLoad = 4000.0f;
    float m_slipAngle = 0.0f;
    float m_slipRatio = 0.0f;

    float m_peakG = 1.2f;
    float m_slideG = 0.8f;
    float m_peakSlip = 0.1f;
    float m_endSlip = 0.3f;
};

class Aerodynamics : public QObject
{
    Q_OBJECT
public:
    explicit Aerodynamics(QObject* parent = nullptr) : QObject(parent) {}
    ~Aerodynamics() {}

    void setDragCoefficient(float cd) { m_dragCoeff = cd; }
    float dragCoefficient() const { return m_dragCoeff; }

    void setLiftCoefficient(float cl) { m_liftCoeff = cl; }
    float liftCoefficient() const { return m_liftCoeff; }

    void setDragArea(float area) { m_dragArea = area; }
    float dragArea() const { return m_dragArea; }

    void setLiftArea(float area) { m_liftArea = area; }
    float liftArea() const { return m_liftArea; }

    void setBalance(float balance) { m_balance = balance; }
    float balance() const { return m_balance; }

    QVector3D computeDrag(float velocity);
    QVector3D computeLift(float velocity);

    void setWingAngle(float angle) { m_wingAngle = angle; }
    float wingAngle() const { return m_wingAngle; }

signals:
    void forcesUpdated();

private:
    float m_dragCoeff = 0.3f;
    float m_liftCoeff = 0.4f;
    float m_dragArea = 2.2f;
    float m_liftArea = 2.0f;
    float m_balance = 0.5f;
    float m_wingAngle = 0.0f;
    float m_airDensity = 1.225f;
};

} // namespace physics
} // namespace ks