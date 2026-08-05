#include "PhysicsSystem.h"
#include <cmath>
#include <algorithm>

namespace ks {
namespace animation {

// ============================================================================
// PhysicsAnimation
// ============================================================================

PhysicsAnimation::PhysicsAnimation() = default;

void PhysicsAnimation::addBody(const Body& body)
{
    m_bodies[body.name] = body;
}

void PhysicsAnimation::removeBody(const QString& name)
{
    m_bodies.remove(name);
}

void PhysicsAnimation::addConstraint(const Constraint& constraint)
{
    m_constraints.append(constraint);
}

void PhysicsAnimation::removeConstraint(const QString& name)
{
    for (int i = m_constraints.size() - 1; i >= 0; --i) {
        if (m_constraints[i].bodyA == name || m_constraints[i].bodyB == name) {
            m_constraints.removeAt(i);
        }
    }
}

void PhysicsAnimation::addSpring(const Spring& spring)
{
    m_springs.append(spring);
}

void PhysicsAnimation::step(double deltaTime)
{
    applyGravity(deltaTime);
    integrateBodies(deltaTime);
    solveConstraints(deltaTime);
    solveSprings(deltaTime);
}

QVector<PhysicsAnimation::Body> PhysicsAnimation::createRagdoll(const QVector3D& rootPos, double height, double mass)
{
    QVector<Body> bodies;

    // Head
    Body head;
    head.name = "Head";
    head.position = rootPos + QVector3D(0, height * 0.85, 0);
    head.mass = mass * 0.07;
    bodies.append(head);

    // Torso
    Body torso;
    torso.name = "Torso";
    torso.position = rootPos + QVector3D(0, height * 0.55, 0);
    torso.mass = mass * 0.5;
    bodies.append(torso);

    // Left arm segments
    Body lUpperArm;
    lUpperArm.name = "L_UpperArm";
    lUpperArm.position = rootPos + QVector3D(-height * 0.15, height * 0.65, 0);
    lUpperArm.mass = mass * 0.03;
    bodies.append(lUpperArm);

    Body lLowerArm;
    lLowerArm.name = "L_LowerArm";
    lLowerArm.position = rootPos + QVector3D(-height * 0.3, height * 0.6, 0);
    lLowerArm.mass = mass * 0.02;
    bodies.append(lLowerArm);

    Body lHand;
    lHand.name = "L_Hand";
    lHand.position = rootPos + QVector3D(-height * 0.4, height * 0.5, 0);
    lHand.mass = mass * 0.01;
    bodies.append(lHand);

    // Right arm segments
    Body rUpperArm;
    rUpperArm.name = "R_UpperArm";
    rUpperArm.position = rootPos + QVector3D(height * 0.15, height * 0.65, 0);
    rUpperArm.mass = mass * 0.03;
    bodies.append(rUpperArm);

    Body rLowerArm;
    rLowerArm.name = "R_LowerArm";
    rLowerArm.position = rootPos + QVector3D(height * 0.3, height * 0.6, 0);
    rLowerArm.mass = mass * 0.02;
    bodies.append(rLowerArm);

    Body rHand;
    rHand.name = "R_Hand";
    rHand.position = rootPos + QVector3D(height * 0.4, height * 0.5, 0);
    rHand.mass = mass * 0.01;
    bodies.append(rHand);

    // Left leg segments
    Body lThigh;
    lThigh.name = "L_Thigh";
    lThigh.position = rootPos + QVector3D(-height * 0.08, height * 0.35, 0);
    lThigh.mass = mass * 0.1;
    bodies.append(lThigh);

    Body lShin;
    lShin.name = "L_Shin";
    lShin.position = rootPos + QVector3D(-height * 0.08, height * 0.15, 0);
    lShin.mass = mass * 0.05;
    bodies.append(lShin);

    Body lFoot;
    lFoot.name = "L_Foot";
    lFoot.position = rootPos + QVector3D(-height * 0.08, 0, 0);
    lFoot.mass = mass * 0.02;
    bodies.append(lFoot);

    // Right leg segments
    Body rThigh;
    rThigh.name = "R_Thigh";
    rThigh.position = rootPos + QVector3D(height * 0.08, height * 0.35, 0);
    rThigh.mass = mass * 0.1;
    bodies.append(rThigh);

    Body rShin;
    rShin.name = "R_Shin";
    rShin.position = rootPos + QVector3D(height * 0.08, height * 0.15, 0);
    rShin.mass = mass * 0.05;
    bodies.append(rShin);

    Body rFoot;
    rFoot.name = "R_Foot";
    rFoot.position = rootPos + QVector3D(height * 0.08, 0, 0);
    rFoot.mass = mass * 0.02;
    bodies.append(rFoot);

    return bodies;
}

QVector<PhysicsAnimation::Constraint> PhysicsAnimation::createRagdollConstraints(const QVector<Body>& bodies)
{
    QVector<Constraint> constraints;

    auto findBody = [&](const QString& name) -> int {
        for (int i = 0; i < bodies.size(); ++i) {
            if (bodies[i].name == name) return i;
        }
        return -1;
    };

    auto addConstraint = [&](const QString& a, const QString& b, Constraint::Type type,
                             const QVector3D& pivotA = QVector3D(), const QVector3D& pivotB = QVector3D(),
                             const QVector3D& axis = QVector3D()) {
        Constraint c;
        c.type = type;
        c.bodyA = a;
        c.bodyB = b;
        c.pivotA = pivotA;
        c.pivotB = pivotB;
        c.axis = axis;
        c.stiffness = 1.0;
        c.damping = 0.1;
        constraints.append(c);
    };

    // Head to Torso
    addConstraint("Head", "Torso", Constraint::ConeTwist, QVector3D(0, -0.1, 0), QVector3D(0, 0.2, 0));

    // Arms
    addConstraint("Torso", "L_UpperArm", Constraint::ConeTwist, QVector3D(-0.2, 0.2, 0), QVector3D(0, 0.15, 0));
    addConstraint("L_UpperArm", "L_LowerArm", Constraint::Hinge, QVector3D(0, -0.15, 0), QVector3D(0, 0.15, 0), QVector3D(1, 0, 0));
    addConstraint("L_LowerArm", "L_Hand", Constraint::ConeTwist, QVector3D(0, -0.15, 0), QVector3D(0, 0.1, 0));

    addConstraint("Torso", "R_UpperArm", Constraint::ConeTwist, QVector3D(0.2, 0.2, 0), QVector3D(0, 0.15, 0));
    addConstraint("R_UpperArm", "R_LowerArm", Constraint::Hinge, QVector3D(0, -0.15, 0), QVector3D(0, 0.15, 0), QVector3D(1, 0, 0));
    addConstraint("R_LowerArm", "R_Hand", Constraint::ConeTwist, QVector3D(0, -0.15, 0), QVector3D(0, 0.1, 0));

    // Legs
    addConstraint("Torso", "L_Thigh", Constraint::ConeTwist, QVector3D(-0.1, -0.2, 0), QVector3D(0, 0.2, 0));
    addConstraint("L_Thigh", "L_Shin", Constraint::Hinge, QVector3D(0, -0.2, 0), QVector3D(0, 0.2, 0), QVector3D(1, 0, 0));
    addConstraint("L_Shin", "L_Foot", Constraint::ConeTwist, QVector3D(0, -0.2, 0), QVector3D(0, 0.1, 0));

    addConstraint("Torso", "R_Thigh", Constraint::ConeTwist, QVector3D(0.1, -0.2, 0), QVector3D(0, 0.2, 0));
    addConstraint("R_Thigh", "R_Shin", Constraint::Hinge, QVector3D(0, -0.2, 0), QVector3D(0, 0.2, 0), QVector3D(1, 0, 0));
    addConstraint("R_Shin", "R_Foot", Constraint::ConeTwist, QVector3D(0, -0.2, 0), QVector3D(0, 0.1, 0));

    return constraints;
}

void PhysicsAnimation::integrateBodies(double dt)
{
    for (auto& body : m_bodies) {
        if (body.kinematic) continue;

        // Integrate velocity
        body.velocity += m_gravity * dt;
        body.position += body.velocity * dt;

        // Integrate angular velocity
        body.rotation = QQuaternion::fromEulerAngles(body.angularVelocity * dt) * body.rotation;
    }
}

void PhysicsAnimation::solveConstraints(double dt)
{
    for (int iter = 0; iter < m_solverIterations; ++iter) {
        for (auto& constraint : m_constraints) {
            auto bodyAIt = m_bodies.find(constraint.bodyA);
            Body* bodyA = bodyAIt != m_bodies.end() ? &bodyAIt.value() : nullptr;
            auto bodyBIt = m_bodies.find(constraint.bodyB);
            Body* bodyB = bodyBIt != m_bodies.end() ? &bodyBIt.value() : nullptr;
            
            if (!bodyA || (!bodyB && constraint.type != Constraint::Fixed)) continue;
            if (bodyA->kinematic && (!bodyB || bodyB->kinematic)) continue;

            QVector3D posA = bodyA->position + bodyA->rotation.rotatedVector(constraint.pivotA);
            QVector3D posB = bodyB ? bodyB->position + bodyB->rotation.rotatedVector(constraint.pivotB) : constraint.pivotB;

            QVector3D diff = posB - posA;
            float dist = diff.length();

            if (dist < 1e-6) continue;

            float correction = constraint.stiffness * dist;
            QVector3D dir = diff / dist;

            if (!bodyA->kinematic) {
                bodyA->position += dir * correction * 0.5;
            }
            if (bodyB && !bodyB->kinematic) {
                bodyB->position -= dir * correction * 0.5;
            }
        }
    }
}

void PhysicsAnimation::solveSprings(double dt)
{
    for (auto& spring : m_springs) {
        auto bodyAIt = m_bodies.find(spring.bodyA);
        Body* bodyA = bodyAIt != m_bodies.end() ? &bodyAIt.value() : nullptr;
        auto bodyBIt = m_bodies.find(spring.bodyB);
        Body* bodyB = bodyBIt != m_bodies.end() ? &bodyBIt.value() : nullptr;
        
        if (!bodyA || !bodyB) continue;
        if (bodyA->kinematic && bodyB->kinematic) continue;

        QVector3D posA = bodyA->position + bodyA->rotation.rotatedVector(spring.anchorA);
        QVector3D posB = bodyB->position + bodyB->rotation.rotatedVector(spring.anchorB);
        
        QVector3D diff = posB - posA;
        float dist = diff.length();
        
        if (dist < 1e-6) continue;

        float forceMag = spring.stiffness * (dist - spring.restLength) + spring.damping * 0;
        QVector3D force = diff / dist * forceMag;

        if (!bodyA->kinematic) bodyA->velocity += force / bodyA->mass * dt;
        if (!bodyB->kinematic) bodyB->velocity -= force / bodyB->mass * dt;
    }
}

void PhysicsAnimation::applyGravity(double dt)
{
    // Gravity is already applied in integrateBodies
}

} // namespace animation
} // namespace ks