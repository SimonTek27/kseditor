#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QMatrix4x4>
#include <QMatrix3x3>
#include <QVector3D>
#include <QQuaternion>

namespace ks {
namespace animation {

// ============================================================================
// Physics-driven Animation (animation-specific physics)
// ============================================================================

class PhysicsAnimation {
public:
    struct Body {
        QString name;
        QVector3D position;
        QVector3D velocity;
        QVector3D angularVelocity;
        QQuaternion rotation;
        double mass = 1.0;
        QMatrix3x3 inertiaTensor;
        bool kinematic = false;
    };

    struct Constraint {
        enum Type { Fixed, Hinge, Slider, ConeTwist, Generic6DOF };
        Type type = Fixed;
        QString bodyA;
        QString bodyB;
        QVector3D pivotA;
        QVector3D pivotB;
        QVector3D axis;
        QVector3D limitsMin;
        QVector3D limitsMax;
        double stiffness = 1.0;
        double damping = 0.1;
    };

    struct Spring {
        QString bodyA;
        QString bodyB;
        QVector3D anchorA;
        QVector3D anchorB;
        double restLength = 1.0;
        double stiffness = 100.0;
        double damping = 1.0;
    };

    explicit PhysicsAnimation();
    ~PhysicsAnimation() = default;

    void addBody(const Body& body);
    void removeBody(const QString& name);
    void addConstraint(const Constraint& constraint);
    void removeConstraint(const QString& name);
    void addSpring(const Spring& spring);

    void step(double deltaTime);
    void setGravity(const QVector3D& gravity) { m_gravity = gravity; }
    QVector3D gravity() const { return m_gravity; }

    QMap<QString, Body> bodies() const { return m_bodies; }
    QVector<Constraint> constraints() const { return m_constraints; }
    QVector<Spring> springs() const { return m_springs; }

    // Ragdoll helpers
    static QVector<Body> createRagdoll(const QVector3D& rootPos, double height, double mass);
    static QVector<Constraint> createRagdollConstraints(const QVector<Body>& bodies);

private:
    void integrateBodies(double dt);
    void solveConstraints(double dt);
    void solveSprings(double dt);
    void applyGravity(double dt);

    QMap<QString, Body> m_bodies;
    QVector<Constraint> m_constraints;
    QVector<Spring> m_springs;
    QVector3D m_gravity = QVector3D(0, -9.81, 0);
    double m_timeStep = 1.0 / 60.0;
    int m_solverIterations = 10;
};

} // namespace animation
} // namespace ks

Q_DECLARE_METATYPE(ks::animation::PhysicsAnimation::Body)
Q_DECLARE_METATYPE(ks::animation::PhysicsAnimation::Constraint)
Q_DECLARE_METATYPE(ks::animation::PhysicsAnimation::Spring)