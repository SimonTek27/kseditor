#include "PhysicsEngine.h"
#include <algorithm>
#include <cmath>
#include <QDebug>

namespace ks { namespace physics {

void RigidBody::applyForce(const QVector3D& force, const QVector3D& point) {
    m_accumulatedForce += force;
    m_accumulatedTorque += QVector3D::crossProduct(point - m_position, force);
}

void RigidBody::applyImpulse(const QVector3D& impulse, const QVector3D& point) {
    m_velocity += impulse / m_mass;
    m_angularVelocity += QVector3D::crossProduct(point - m_position, impulse) / m_mass;
}

void RigidBody::integrate(float dt) {
    QVector3D acceleration = m_accumulatedForce / m_mass + m_gravity;
    m_velocity += acceleration * dt;
    m_position += m_velocity * dt;

    // Apply torque to angular velocity (using mass as inertia approximation)
    m_angularVelocity += (m_accumulatedTorque / m_mass) * dt;
    // Apply damping correctly (multiplicative, not additive)
    m_angularVelocity *= std::max(0.0f, 1.0f - 0.01f * dt);
    m_rotation += m_angularVelocity * dt;

    m_accumulatedForce = QVector3D();
    m_accumulatedTorque = QVector3D();

    emit positionChanged();
}

float RigidBody::kineticEnergy() const {
    return 0.5f * m_mass * m_velocity.lengthSquared();
}

float RigidBody::potentialEnergy(float gravity) const {
    return m_mass * gravity * m_position.y();
}

float CollisionShape::computeVolume() const {
    switch (m_type) {
        case Box: return m_dimensions.x() * m_dimensions.y() * m_dimensions.z();
        case Sphere: return (4.0f/3.0f) * 3.14159f * powf(m_dimensions.x(), 3);
        default: return 1.0f;
    }
}

QVector3D CollisionShape::computeInertia() const {
    float mass = 1.0f;
    float x = m_dimensions.x(), y = m_dimensions.y(), z = m_dimensions.z();
    return {mass * (y*y + z*z) / 12.0f, mass * (x*x + z*z) / 12.0f, mass * (x*x + y*y) / 12.0f};
}

void PhysicsWorld::addBody(RigidBody* body) {
    m_bodies.append(body);
}

RigidBody* PhysicsWorld::createBody(float mass) {
    auto* body = new RigidBody(this);
    body->setMass(mass);
    m_bodies.append(body);
    return body;
}

void PhysicsWorld::removeBody(RigidBody* body) {
    m_bodies.removeAll(body);
}

void PhysicsWorld::stepSimulation(float deltaTime) {
    // Broadphase: cull pairs using AABB before narrow phase
    QVector<QPair<int, int>> candidatePairs;

    if (m_broadphase == Simple || m_bodies.size() < 10) {
        // O(n^2) brute force for small scenes
        for (int i = 0; i < m_bodies.size(); ++i) {
            for (int j = i + 1; j < m_bodies.size(); ++j) {
                candidatePairs.append(QPair<float,float>(i, j));
            }
        }
    } else {
        // SAP (Sweep and Prune) on X-axis
        struct BodyExtent { int index; float min, max; };
        QVector<BodyExtent> extents;
        for (int i = 0; i < m_bodies.size(); ++i) {
            float halfSize = 0.5f; // default bounding half-size
            float x = m_bodies[i]->position().x();
            extents.append({i, x - halfSize, x + halfSize});
        }
        std::sort(extents.begin(), extents.end(),
                  [](const BodyExtent& a, const BodyExtent& b) { return a.min < b.min; });

        for (int i = 0; i < extents.size(); ++i) {
            for (int j = i + 1; j < extents.size(); ++j) {
                if (extents[j].min > extents[i].max) break;
                candidatePairs.append(QPair<float,float>(extents[i].index, extents[j].index));
            }
        }
    }

    // Narrow phase: integrate and solve constraints
    for (RigidBody* body : m_bodies) {
        body->integrate(deltaTime);
    }
    solveConstraints();
    emit stepCompleted();
}

void PhysicsWorld::debugDraw() {
    // Debug output for all bodies
    for (int i = 0; i < m_bodies.size(); ++i) {
        RigidBody* body = m_bodies[i];
        qDebug() << "Body" << i
                 << "pos:" << body->position()
                 << "vel:" << body->velocity()
                 << "KE:" << body->kineticEnergy();
    }
}

void PhysicsWorld::solveConstraints() {
    // Ground constraint - prevent bodies from falling through y=0
    for (RigidBody* body : m_bodies) {
        QVector3D pos = body->position();
        QVector3D vel = body->velocity();

        // Simple ground plane at y = 0
        if (pos.y() < 0.0f) {
            pos.setY(0.0f);
            if (vel.y() < 0.0f) {
                // Reflect velocity with energy loss (restitution = 0.3)
                vel.setY(-vel.y() * 0.3f);

                // Apply friction to horizontal velocity
                float friction = 0.98f;
                vel.setX(vel.x() * friction);
                vel.setZ(vel.z() * friction);
            }
            body->setPosition(pos);
            body->setVelocity(vel);
        }
    }

    // Iterative constraint solving
    for (int iter = 0; iter < m_solverIterations; ++iter) {
        for (int i = 0; i < m_bodies.size(); ++i) {
            for (int j = i + 1; j < m_bodies.size(); ++j) {
                RigidBody* a = m_bodies[i];
                RigidBody* b = m_bodies[j];

                QVector3D diff = b->position() - a->position();
                float dist = diff.length();
                float minDist = 1.0f; // minimum separation distance

                if (dist < minDist && dist > 0.001f) {
                    QVector3D normal = diff / dist;
                    float penetration = minDist - dist;
                    float totalMass = a->mass() + b->mass();
                    if (totalMass < 0.001f) continue;

                    // Positional correction
                    float ratioA = b->mass() / totalMass;
                    float ratioB = a->mass() / totalMass;
                    a->setPosition(a->position() - normal * penetration * ratioA * 0.5f);
                    b->setPosition(b->position() + normal * penetration * ratioB * 0.5f);

                    // Velocity separation
                    QVector3D relVel = b->velocity() - a->velocity();
                    float velAlongNormal = QVector3D::dotProduct(relVel, normal);
                    if (velAlongNormal < 0.0f) {
                        float restitution = 0.3f;
                        float j = -(1.0f + restitution) * velAlongNormal / totalMass;
                        QVector3D impulse = normal * j;
                        a->setVelocity(a->velocity() - impulse * b->mass());
                        b->setVelocity(b->velocity() + impulse * a->mass());
                    }
                }
            }
        }
    }
}

void CarPhysics::setInput(float throttle, float brake, float steering, bool handbrake) {
    m_throttle = throttle;
    m_brake = brake;
    m_steering = steering;
    m_handbrake = handbrake;
}

void CarPhysics::setDiffModel(const DifferentialModel& model) {
    m_diff.setConfig(model.getConfig());
}

void CarPhysics::setBrakeModel(const BrakeThermalModel& brake) {
    m_brakeThermal = brake;
}

void CarPhysics::update(float dt) {
    dt = std::min(dt, 0.05f); // clamp to prevent exploding physics at low FPS

    // --- Drivetrain ---
    float gearRatio = m_config.gearRatios[m_currentGear - 1] * m_config.finalDrive;
    float wheelRadius = 0.33f;

    // Wheel RPM from speed
    float wheelRpm = m_speed / 3.6f / wheelRadius * 60.0f / (2.0f * 3.14159f);
    m_rpm = 800.0f + wheelRpm * gearRatio;
    m_rpm = std::clamp(m_rpm, 800.0f, 8000.0f);

    // Clutch decay timer for shift smoothness
    m_clutch = std::max(0.0f, m_clutch - dt * 4.0f); // recovers over ~0.25s

    // Auto gear shift
    bool shifting = false;
    if (m_rpm > 7500.0f && m_currentGear < 7) {
        m_currentGear++;
        m_clutch = 1.0f; // clutch disengaged momentarily
        emit gearChanged(m_currentGear);
        shifting = true;
    } else if (m_rpm < 3000.0f && m_currentGear > 1) {
        m_currentGear--;
        m_clutch = 1.0f;
        emit gearChanged(m_currentGear);
        shifting = true;
    }

    // Engine torque curve (with clutch modulation)
    float normalizedRpm = (m_rpm - 800.0f) / 7200.0f;
    float torqueCurve = 4.0f * normalizedRpm * (1.0f - normalizedRpm);
    float engineTorque = m_config.maxTorque * torqueCurve * m_throttle * (1.0f - m_clutch * 0.8f);
    float driveshaftTorque = engineTorque * gearRatio;

    // --- Per-wheel speeds with yaw influence ---
    float yawEffectRL = m_yawRate * m_config.trackWidth * 0.5f;
    float yawEffectRR = -yawEffectRL;

    m_wheelSpeedFL = m_speed / 3.6f + yawEffectRL;
    m_wheelSpeedFR = m_speed / 3.6f + yawEffectRR;
    m_wheelSpeedRL = m_speed / 3.6f + yawEffectRL;
    m_wheelSpeedRR = m_speed / 3.6f + yawEffectRR;

    // --- Differential torque distribution ---
    float driveLayout = m_config.driveLayout;
    float leftDrivenTorque = 0.0f, rightDrivenTorque = 0.0f;
    float leftRearTorque = 0.0f, rightRearTorque = 0.0f;

    if (driveLayout < 0.33f) {
        // FWD
        m_diff.update(dt, driveshaftTorque, m_wheelSpeedFL, m_wheelSpeedFR);
        leftDrivenTorque = m_diff.getLeftTorque();
        rightDrivenTorque = m_diff.getRightTorque();
    } else if (driveLayout > 0.67f) {
        // AWD: split 60/40 front/rear
        float frontTorque = driveshaftTorque * 0.6f;
        float rearTorque = driveshaftTorque * 0.4f;
        m_diff.update(dt, frontTorque, m_wheelSpeedFL, m_wheelSpeedFR);
        leftDrivenTorque = m_diff.getLeftTorque();
        rightDrivenTorque = m_diff.getRightTorque();
        // rear diff
        DifferentialModel rearDiff;
        rearDiff.setConfig(m_diff.getConfig());
        rearDiff.update(dt, rearTorque, m_wheelSpeedRL, m_wheelSpeedRR);
        leftRearTorque = rearDiff.getLeftTorque();
        rightRearTorque = rearDiff.getRightTorque();
    } else {
        // RWD
        m_diff.update(dt, driveshaftTorque, m_wheelSpeedRL, m_wheelSpeedRR);
        leftRearTorque = m_diff.getLeftTorque();
        rightRearTorque = m_diff.getRightTorque();
    }

    // --- Wheel slip ratios ---
    auto calcSlip = [](float wheelSpeed, float carSpeed) -> float {
        carSpeed = std::max(carSpeed, 0.1f);
        return (wheelSpeed - carSpeed) / carSpeed;
    };

    float carSpeedMs = m_speed / 3.6f;
    m_wheelSlipFL = calcSlip(m_wheelSpeedFL, carSpeedMs + yawEffectRL);
    m_wheelSlipFR = calcSlip(m_wheelSpeedFR, carSpeedMs + yawEffectRR);
    m_wheelSlipRL = calcSlip(m_wheelSpeedRL, carSpeedMs + yawEffectRL);
    m_wheelSlipRR = calcSlip(m_wheelSpeedRR, carSpeedMs + yawEffectRR);

    // --- TC: cut torque if wheel slip exceeds threshold ---
    if (m_tcEnabled) {
        float maxSlip = 0.15f;
        if (m_wheelSlipFL > maxSlip || m_wheelSlipFR > maxSlip) {
            leftDrivenTorque *= 0.5f;
            rightDrivenTorque *= 0.5f;
        }
        if (m_wheelSlipRL > maxSlip || m_wheelSlipRR > maxSlip) {
            leftRearTorque *= 0.5f;
            rightRearTorque *= 0.5f;
        }
    }

    // --- Per-wheel traction force ---
    float tractionFL = leftDrivenTorque / wheelRadius;
    float tractionFR = rightDrivenTorque / wheelRadius;
    float tractionRL = leftRearTorque / wheelRadius;
    float tractionRR = rightRearTorque / wheelRadius;

    // --- Resistances ---
    float dragForce = 0.5f * 1.225f * 0.3f * 2.2f * carSpeedMs * carSpeedMs;
    float rollingResistance = m_config.mass * 9.81f * 0.015f;

    // --- Per-wheel braking with thermal model ---
    auto computeBrakeForce = [&](float pedal, float wheelSpeedMs, bool isRear) -> float {
        float baseBrake = pedal * (isRear ? m_config.handbrakePower : m_config.brakePower) * 0.25f;
        // Handbrake amplifies rear
        if (isRear && m_handbrake) {
            baseBrake = std::max(baseBrake, m_config.handbrakePower * 0.5f);
        }
        // Build BrakeThermalModel state
        BrakeThermalModel::BrakeState bs;
        bs.pedalPressure = pedal;
        bs.brakeTorque = baseBrake * wheelRadius;
        bs.discAngularVelocity = std::abs(wheelSpeedMs) / wheelRadius;
        m_brakeThermal.update(dt, bs);
        float fade = m_brakeThermal.calculateBrakeFade();
        return baseBrake * fade;
    };

    float brakeFL = computeBrakeForce(m_brake, m_wheelSpeedFL, false);
    float brakeFR = computeBrakeForce(m_brake, m_wheelSpeedFR, false);
    float brakeRL = computeBrakeForce(m_brake, m_wheelSpeedRL, true);
    float brakeRR = computeBrakeForce(m_brake, m_wheelSpeedRR, true);

    // --- ABS: reduce brake if wheel locks ---
    if (m_absEnabled) {
        auto applyAbs = [](float& brake, float slip) {
            if (slip < -0.2f) brake *= 0.3f;
        };
        applyAbs(brakeFL, m_wheelSlipFL);
        applyAbs(brakeFR, m_wheelSlipFR);
        applyAbs(brakeRL, m_wheelSlipRL);
        applyAbs(brakeRR, m_wheelSlipRR);
    }

    // --- Static weight distribution ---
    float totalWeight = m_config.mass * 9.81f;
    float frontStatic = totalWeight * m_config.frontWeightBias;
    float rearStatic = totalWeight * (1.0f - m_config.frontWeightBias);
    float wheelFLstatic = frontStatic * 0.5f;
    float wheelFRstatic = frontStatic * 0.5f;
    float wheelRLstatic = rearStatic * 0.5f;
    float wheelRRstatic = rearStatic * 0.5f;

    // --- Roll/pitch dynamics (weight transfer) ---
    float longAccel = (tractionFL + tractionFR + tractionRL + tractionRR
                       - dragForce - rollingResistance
                       - brakeFL - brakeFR - brakeRL - brakeRR) / m_config.mass;
    float latAccel = m_lateralVelocity / std::max(dt, 0.001f);

    // Pitch transfer
    float pitchTransfer = longAccel * m_config.cgHeight / m_config.wheelbase * totalWeight * 0.5f;
    // Roll transfer
    float rollTransfer = latAccel * m_config.cgHeight / m_config.trackWidth * totalWeight * 0.5f;

    // Update roll/pitch angles (spring-mass model)
    float targetRoll = std::atan2(latAccel, 9.81f) * 0.5f;
    float targetPitch = std::atan2(longAccel, 9.81f) * 0.3f;
    m_rollAngle += (targetRoll - m_rollAngle) * std::min(1.0f, 10.0f * dt);
    m_pitchAngle += (targetPitch - m_pitchAngle) * std::min(1.0f, 10.0f * dt);

    // Per-wheel normal forces
    float wheelFL_norm = wheelFLstatic - pitchTransfer - rollTransfer;
    float wheelFR_norm = wheelFRstatic - pitchTransfer + rollTransfer;
    float wheelRL_norm = wheelRLstatic + pitchTransfer - rollTransfer;
    float wheelRR_norm = wheelRRstatic + pitchTransfer + rollTransfer;

    // Clamp to prevent negative loads
    wheelFL_norm = std::max(wheelFL_norm, totalWeight * 0.02f);
    wheelFR_norm = std::max(wheelFR_norm, totalWeight * 0.02f);
    wheelRL_norm = std::max(wheelRL_norm, totalWeight * 0.02f);
    wheelRR_norm = std::max(wheelRR_norm, totalWeight * 0.02f);

    // --- Lateral forces ---
    float steeringAngle = m_steering * 0.35f; // ~20 deg max

    // Slip angles per axle
    float frontSlip = steeringAngle - std::atan2(m_lateralVelocity + m_yawRate * m_config.wheelbase * 0.5f,
                                                  std::max(carSpeedMs, 1.0f));
    float rearSlip = -std::atan2(m_lateralVelocity - m_yawRate * m_config.wheelbase * 0.5f,
                                                  std::max(carSpeedMs, 1.0f));

    float maxLatForce = 1.2f; // peak grip coefficient
    float frontLatForce = maxLatForce * wheelFL_norm * std::sin(frontSlip);
    float rearLatForce = maxLatForce * wheelRL_norm * std::sin(rearSlip);

    // --- Longitudinal summation ---
    float totalTraction = tractionFL + tractionFR + tractionRL + tractionRR;
    float totalBrakeForce = brakeFL + brakeFR + brakeRL + brakeRR;
    float netForce = totalTraction - dragForce - rollingResistance - totalBrakeForce;

    float acceleration = netForce / m_config.mass;
    m_speed += acceleration * dt * 3.6f; // convert m/s^2 to km/h
    m_speed = std::max(0.0f, m_speed);

    // --- Lateral dynamics ---
    float lateralAccelCar = (frontLatForce + rearLatForce) / m_config.mass;
    m_lateralVelocity += lateralAccelCar * dt;
    m_lateralVelocity *= std::max(0.0f, 1.0f - 2.0f * dt); // lateral damping

    // Yaw dynamics
    float yawTorque = frontLatForce * m_config.wheelbase * 0.5f
                      - rearLatForce * m_config.wheelbase * 0.5f;
    m_yawRate += yawTorque / m_config.yawInertia * dt;
    m_yawRate *= std::max(0.0f, 1.0f - 0.5f * dt);

    float sideSlip = std::atan2(m_lateralVelocity, std::max(1.0f, carSpeedMs));
    m_slipAngle = qRadiansToDegrees(sideSlip);

    // --- G-force ---
    float longG = acceleration / 9.81f;
    float latG = latAccel / 9.81f;
    m_gForce = std::sqrt(longG * longG + latG * latG);

    emit telemetryUpdated();
}

float CarPhysics::getSlipAngle(bool front) const {
    float maxSlip = front ? 12.0f : 10.0f;
    return qBound(-maxSlip, m_slipAngle, maxSlip);
}

float CarPhysics::getTractionCircle() const {
    float maxGrip = 1.2f;
    float accelG = m_throttle * m_config.enginePower / (m_config.mass * 9.81f);
    float latG = qAbs(m_gForce);
    float totalG = sqrtf(accelG * accelG + latG * latG);
    return 1.0f - qBound(0.0f, totalG / maxGrip, 1.0f);
}

void CarPhysics::shiftUp() {
    if (m_currentGear < 7) {
        m_currentGear++;
        m_rpm = qMax(800.0f, m_rpm * 0.7f);
        emit gearChanged(m_currentGear);
    }
}

void CarPhysics::shiftDown() {
    if (m_currentGear > 1) {
        m_currentGear--;
        m_rpm = qMin(8000.0f, m_rpm * 1.4f);
        emit gearChanged(m_currentGear);
    }
}

void CarPhysics::shiftTo(int gear) {
    if (gear >= 1 && gear <= 7 && gear != m_currentGear) {
        float rpmFactor = m_config.gearRatios[m_currentGear - 1] / m_config.gearRatios[gear - 1];
        m_currentGear = gear;
        m_rpm = qBound(800.0f, m_rpm * rpmFactor, 8000.0f);
        emit gearChanged(m_currentGear);
    }
}

CarPhysics::Telemetry CarPhysics::getTelemetry() const {
    Telemetry t;
    t.speed = m_speed;
    t.rpm = m_rpm;
    t.throttle = m_throttle;
    t.brake = m_brake;
    t.steering = m_steering;
    t.slipAngle = m_slipAngle;
    t.gForce = m_gForce;
    t.turboBoost = (m_rpm > 5000.0f) ? (m_rpm - 5000.0f) / 3000.0f : 0.0f;
    t.rollAngle = m_rollAngle;
    t.pitchAngle = m_pitchAngle;
    t.brakeTemp = m_brakeThermal.calculateDiscTemp();
    t.diffLock = m_diff.getLockingTorque();
    t.wheelSlipFL = m_wheelSlipFL;
    t.wheelSlipFR = m_wheelSlipFR;
    t.wheelSlipRL = m_wheelSlipRL;
    t.wheelSlipRR = m_wheelSlipRR;
    return t;
}

float SuspensionModel::computeForce(float currentLength, float velocity) {
    float displacement = m_restLength - currentLength;
    displacement = std::clamp(displacement, -m_travel, m_travel);

    float springForce = m_spring * displacement;
    float damperForce = m_damping * velocity;

    return springForce + damperForce;
}

QVector2D TireModel::computeForces() {
    float slip = m_slipAngle;
    float normalizedLoad = m_verticalLoad / 4000.0f;

    float maxForce = normalizedLoad * m_peakG * m_verticalLoad;

    float frictionFactor;
    if (std::abs(slip) < m_peakSlip) {
        frictionFactor = m_peakG * slip / m_peakSlip;
    } else if (std::abs(slip) < m_endSlip) {
        float x = (std::abs(slip) - m_peakSlip) / (m_endSlip - m_peakSlip);
        frictionFactor = m_peakG + (m_slideG - m_peakG) * x;
    } else {
        frictionFactor = m_slideG;
    }

    return QVector2D(frictionFactor * maxForce, 0);
}

QVector3D Aerodynamics::computeDrag(float velocity) {
    float dragForce = 0.5f * m_airDensity * velocity * velocity * m_dragCoeff * m_dragArea;
    return QVector3D(-dragForce, 0, 0);
}

QVector3D Aerodynamics::computeLift(float velocity) {
    float liftForce = 0.5f * m_airDensity * velocity * velocity * m_liftCoeff * m_liftArea;
    return QVector3D(0, liftForce, 0);
}

}} // ks::physics