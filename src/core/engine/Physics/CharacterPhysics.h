#pragma once

#include "PhysicsEngine.h"
#include <QObject>
#include <QVector3D>
#include <QVector>

namespace ks::physics {

struct CharacterState {
    QVector3D position, velocity, acceleration;
    QVector3D angularVelocity;
    QVector3D rotation;
    QVector3D previousVelocity;
    float speed = 0, mass = 75.0f;
    bool grounded = true, canJump = true;
    float fallVelocity = 0.0f;
    float jumpHeight = 1.5f, stepHeight = 0.3f;
    float moveSpeed = 5.0f, runSpeed = 10.0f, turnSpeed = 720.0f;
};

struct CharacterCollisionInfo { bool hit = false; QVector3D normal, contactPoint; float distance = 0; };

class CharacterSimulator : public ISimulator {
    Q_OBJECT
public:
    explicit CharacterSimulator(QObject* parent = nullptr);
    ~CharacterSimulator() override;
    void startSimulation() override;
    void stopSimulation() override;
    void reset() override;
    SimulationState getState() const override;
    bool isRunning() const override;
    void updatePhysics(double dt);
    void setMoveDirection(const QVector3D& dir);
    void setJump(bool jump);
    void setMovementDirection(float forward, float right) { m_moveForward = forward; m_moveRight = right; }
    void jump();
    CharacterState getCharacterState() const;
    void setMass(double kg);
    double mass() const;
    CharacterCollisionInfo checkGroundCollision(const QVector3D& displacement);

signals:
    void stateUpdated(const SimulationState& state);

private:
    CharacterState m_state;
    bool m_running = false;
    QVector3D m_moveDirection;
    bool m_throttle = false, m_brake = false, m_jump = false;
    float m_moveForward = 0, m_moveRight = 0;
    static CharacterSimulator* s_instance;
};

} // namespace ks::physics
