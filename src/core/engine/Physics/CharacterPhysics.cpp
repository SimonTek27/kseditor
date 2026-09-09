#include "CharacterPhysics.h"
#include "PhysicsEngine.h"
#include <QDebug>
#include <QtMath>
#include <algorithm>

namespace ks {
namespace physics {

CharacterSimulator::CharacterSimulator(QObject* parent)
    : ISimulator(parent)
{
    m_state.position = QVector3D(0, 2, 0);
    m_state.velocity = QVector3D();
    m_state.previousVelocity = QVector3D();
    m_state.acceleration = QVector3D();
    m_state.angularVelocity = QVector3D();
    m_state.rotation = QVector3D();
    m_state.speed = 0.0f;
    m_state.mass = 70.0f;
    m_state.grounded = true;
    m_state.fallVelocity = 0.0f;
    m_state.jumpHeight = 1.0;
    m_state.stepHeight = 0.5;
    m_state.canJump = true;
    m_state.moveSpeed = 5.0;
    m_state.runSpeed = 10.0;
    m_state.turnSpeed = 90.0;
    m_moveDirection = QVector3D(1, 0, 0);
}

CharacterSimulator::~CharacterSimulator() {}

void CharacterSimulator::startSimulation() {
    if (m_running) return;
    m_running = true;
    qDebug() << "CharacterSimulator: Simulation started";
    emit simulationStarted();
}

void CharacterSimulator::stopSimulation() {
    if (!m_running) return;
    m_running = false;
    qDebug() << "CharacterSimulator: Simulation stopped";
    emit simulationStopped();
}

void CharacterSimulator::reset() {
    m_state = CharacterState();
    m_state.position = QVector3D(0, 2, 0);
    m_state.previousVelocity = QVector3D();
    m_state.grounded = true;
    m_state.canJump = true;
    m_throttle = false;
    m_brake = false;
    m_jump = false;
    m_moveDirection = QVector3D(1, 0, 0);
    qDebug() << "CharacterSimulator: Reset";
}

bool CharacterSimulator::isRunning() const {
    return m_running;
}

ks::physics::SimulationState CharacterSimulator::getState() const {
    ks::physics::SimulationState state;
    state.position = m_state.position;
    state.velocity = m_state.velocity;
    state.acceleration = m_state.acceleration;
    state.angularVelocity = m_state.angularVelocity;
    state.rotation = m_state.rotation;
    state.speed = m_state.speed;
    return state;
}

void CharacterSimulator::setMoveDirection(const QVector3D& dir) {
    m_moveDirection = dir;
}

void CharacterSimulator::setJump(bool jump) {
    m_jump = jump;
}

void CharacterSimulator::setMass(double kg) {
    m_state.mass = static_cast<float>(kg);
}

double CharacterSimulator::mass() const {
    return m_state.mass;
}

void CharacterSimulator::updatePhysics(double dt) {
    if (!m_running) return;

    dt = qMin(dt, 0.016);
    float invMass = 1.0f / m_state.mass;

    QVector3D gravity = QVector3D(0, -9.81f, 0) * dt;

    float forwardSpeed = m_throttle ? m_state.runSpeed : m_state.moveSpeed;
    QVector3D moveDir = m_moveDirection.normalized() * forwardSpeed;
    QVector3D desiredVelocity = moveDir;

    QVector3D acceleration = gravity / dt;

    if (!m_state.grounded) {
        m_state.velocity += gravity;
        m_state.position += m_state.velocity * dt;
    } else {
        m_state.velocity = QVector3D(desiredVelocity.x(), 0, desiredVelocity.z());

        if (m_jump && m_state.canJump) {
            m_state.velocity.setY(m_state.jumpHeight * 10.0f);
            m_state.grounded = false;
            m_state.canJump = false;
        }
    }

    m_state.position += m_state.velocity * dt;

    m_state.speed = m_state.velocity.length();
    m_state.acceleration = (m_state.velocity - m_state.previousVelocity) / dt;
    m_state.previousVelocity = m_state.velocity;

    m_state.rotation.setY(qRadiansToDegrees(std::atan2(m_moveDirection.z(), m_moveDirection.x())));

    emit stateUpdated(getState());
}

CharacterCollisionInfo CharacterSimulator::checkGroundCollision(const QVector3D& displacement) {
    CharacterCollisionInfo info;
    info.hit = false;

    if (m_state.position.y() + displacement.y() < 0.5f) {
        info.hit = true;
        info.normal = QVector3D(0, 1, 0);
        info.contactPoint = QVector3D(m_state.position.x(), 0, m_state.position.z());
        info.distance = m_state.position.y();

        m_state.position.setY(0.5f);
        m_state.velocity.setY(0.0f);
        m_state.grounded = true;
        m_state.canJump = true;
    }

    return info;
}

CharacterState CharacterSimulator::getCharacterState() const {
    return m_state;
}

} // namespace physics
} // namespace ks
