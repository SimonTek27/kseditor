#include "PhysicsEngine.h"
#include <algorithm>
#include <cmath>

namespace ks {
namespace physics {

float CollisionShape::computeVolume() const {
    float vol = 1.0f;
    switch (m_type) {
        case Box:
            vol = m_dimensions.x * m_dimensions.y * m_dimensions.z;
            break;
        case Sphere:
            vol = (4.0f / 3.0f) * Constants::PI * m_dimensions.x * m_dimensions.x * m_dimensions.x;
            break;
        case Capsule:
            vol = Constants::PI * m_dimensions.x * m_dimensions.x *
                  (m_dimensions.y + (4.0f / 3.0f) * m_dimensions.x);
            break;
        case Cylinder:
            vol = Constants::PI * m_dimensions.x * m_dimensions.x * m_dimensions.y;
            break;
        case Cone:
            vol = (1.0f / 3.0f) * Constants::PI * m_dimensions.x * m_dimensions.x * m_dimensions.y;
            break;
        default:
            vol = m_dimensions.x * m_dimensions.y * m_dimensions.z;
            break;
    }
    return std::max(vol, 0.001f);
}

PhysVec3 CollisionShape::computeInertia() const {
    PhysVec3 inertia;
    float volume = computeVolume();
    float density = 1.0f / volume;
    float x = m_dimensions.x, y = m_dimensions.y, z = m_dimensions.z;
    switch (m_type) {
        case Box:
            inertia = PhysVec3(
                (1.0f / 12.0f) * density * (y * y + z * z),
                (1.0f / 12.0f) * density * (x * x + z * z),
                (1.0f / 12.0f) * density * (x * x + y * y));
            break;
        case Sphere:
            inertia = PhysVec3(0.4f * density * x * x, 0.4f * density * x * x, 0.4f * density * x * x);
            break;
        case Capsule: {
            float r = x, h = y;
            float ih = 0.0833f * density * (4.0f * r * r + h * h) + 0.5f * density * r * r;
            float ip = 0.5f * density * r * r;
            inertia = PhysVec3(ih, ip, ih);
            break;
        }
        case Cylinder:
            inertia = PhysVec3(
                (1.0f / 12.0f) * density * (3.0f * x * x + y * y),
                0.5f * density * x * x,
                (1.0f / 12.0f) * density * (3.0f * x * x + y * y));
            break;
        default:
            inertia = PhysVec3(1.0f, 1.0f, 1.0f);
            break;
    }
    return inertia;
}

void RigidBody::setTransform(const PhysMat4& transform) {
    m_position = transform.translation();
    if (onPositionChanged) onPositionChanged();
}

PhysMat4 RigidBody::transform() const {
    PhysMat4 mat;
    mat.setToIdentity();
    mat.setTranslation(m_position);
    return mat;
}

void RigidBody::applyForce(const PhysVec3& force, const PhysVec3& point) {
    if (m_isStatic || m_isKinematic) return;
    m_accumulatedForce += force;
    if (point.lengthSquared() >= 1e-12f) {
        PhysVec3 r = point - m_position;
        m_accumulatedTorque += cross(r, force);
    }
}

void RigidBody::applyImpulse(const PhysVec3& impulse, const PhysVec3& point) {
    if (m_isStatic || m_isKinematic) return;
    m_velocity += impulse / m_mass;
    if (point.lengthSquared() >= 1e-12f) {
        PhysVec3 r = point - m_position;
        PhysVec3 angularImpulse = cross(r, impulse);
        float invInertia = 1.0f / m_mass;
        m_angularVelocity += angularImpulse * invInertia;
    }
}

void RigidBody::applyTorque(const PhysVec3& torque) {
    if (m_isStatic || m_isKinematic) return;
    m_accumulatedTorque += torque;
}

void RigidBody::clearForces() {
    m_accumulatedForce = PhysVec3();
    m_accumulatedTorque = PhysVec3();
}

void RigidBody::integrate(float dt) {
    if (m_isStatic || !m_isActive) return;
    integrateVelocity(dt);
    integratePosition(dt);
}

void RigidBody::integrateVelocity(float dt) {
    if (m_isStatic || m_isKinematic || !m_isActive) return;
    if (m_mass > 0.0f) {
        PhysVec3 acceleration = m_accumulatedForce / m_mass;
        m_velocity += acceleration * dt;
        m_velocity *= (1.0f - 0.01f * dt);
    }
    float invInertia = 1.0f / m_mass;
    m_angularVelocity += m_accumulatedTorque * invInertia * dt;
    m_angularVelocity *= (1.0f - 0.01f * dt);
    clearForces();
}

void RigidBody::integratePosition(float dt) {
    if (m_isStatic || !m_isActive) return;
    if (!m_isKinematic) {
        m_position += m_velocity * dt;
        m_rotation += m_angularVelocity * dt;
    }
}

float RigidBody::kineticEnergy() const {
    float linearKe = 0.5f * m_mass * m_velocity.lengthSquared();
    float angularKe = 0.5f * m_mass * m_angularVelocity.lengthSquared() * 0.1f;
    return linearKe + angularKe;
}

float RigidBody::potentialEnergy(float gravity) const {
    return m_mass * gravity * m_position.y;
}

float RigidBody::totalEnergy(float gravity) const {
    return kineticEnergy() + potentialEnergy(gravity);
}

PhysicsWorld::~PhysicsWorld() { clearBodies(); }

RigidBody* PhysicsWorld::createBody(float mass) {
    auto* body = new RigidBody();
    body->setMass(mass);
    m_bodies.push_back(body);
    if (onBodyAdded) onBodyAdded(body);
    return body;
}

void PhysicsWorld::addBody(RigidBody* body) {
    if (body && std::find(m_bodies.begin(), m_bodies.end(), body) == m_bodies.end()) {
        m_bodies.push_back(body);
        if (onBodyAdded) onBodyAdded(body);
    }
}

void PhysicsWorld::removeBody(RigidBody* body) {
    auto it = std::remove(m_bodies.begin(), m_bodies.end(), body);
    if (it != m_bodies.end()) {
        m_bodies.erase(it, m_bodies.end());
        if (onBodyRemoved) onBodyRemoved(body);
        delete body;
    }
}

void PhysicsWorld::clearBodies() {
    for (auto* body : m_bodies) delete body;
    m_bodies.clear();
    m_bounds.clear();
}

void PhysicsWorld::stepSimulation(float deltaTime) {
    if (deltaTime <= 0.0f) return;
    m_accumulatedTime += deltaTime;
    while (m_accumulatedTime >= m_fixedTimeStep) {
        stepSimulationFixed(m_fixedTimeStep);
        m_accumulatedTime -= m_fixedTimeStep;
    }
    if (onStepCompleted) onStepCompleted();
}

void PhysicsWorld::stepSimulationFixed(float dt) {
    updateBroadphase();
    auto pairs = getCollisionPairs();
    for (const auto& pair : pairs) {
        PhysVec3 contactPoint, contactNormal;
        if (checkCollision(pair.first, pair.second, contactPoint, contactNormal)) {
            resolveCollision(pair.first, pair.second, contactPoint, contactNormal);
            if (onCollisionDetected)
                onCollisionDetected(pair.first, pair.second, contactPoint, contactNormal);
        }
    }
    for (auto* body : m_bodies) {
        if (body && body->isActive()) body->integrate(dt);
    }
    solveConstraints();
}

void PhysicsWorld::solveConstraints() {
    for (int iter = 0; iter < m_solverIterations; ++iter) {
        for (size_t i = 0; i < m_bodies.size(); ++i) {
            for (size_t j = i + 1; j < m_bodies.size(); ++j) {
                auto* a = m_bodies[i];
                auto* b = m_bodies[j];
                if (!a || !b || !a->isActive() || !b->isActive()) continue;
                PhysVec3 diff = b->position() - a->position();
                float dist = diff.length();
                float minDist = 0.1f;
                if (dist < minDist && dist > 0.001f) {
                    PhysVec3 dir = diff / dist;
                    float overlap = minDist - dist;
                    PhysVec3 correction = dir * (overlap * 0.5f);
                    a->setPosition(a->position() - correction);
                    b->setPosition(b->position() + correction);
                }
            }
        }
    }
}

bool PhysicsWorld::checkCollision(RigidBody* bodyA, RigidBody* bodyB,
                                  PhysVec3& contactPoint, PhysVec3& contactNormal) {
    if (!bodyA || !bodyB) return false;
    float radiusA = 0.5f, radiusB = 0.5f;
    if (bodyA->collisionShape()) radiusA = bodyA->collisionShape()->radius();
    if (bodyB->collisionShape()) radiusB = bodyB->collisionShape()->radius();
    PhysVec3 centerA = bodyA->position();
    PhysVec3 centerB = bodyB->position();
    PhysVec3 delta = centerA - centerB;
    float dist = delta.length();
    if (dist < radiusA + radiusB && dist > 0.001f) {
        contactNormal = delta / dist;
        contactPoint = centerA - contactNormal * radiusA;
        return true;
    }
    return false;
}

std::vector<std::pair<RigidBody*, RigidBody*>> PhysicsWorld::getCollisionPairs() {
    std::vector<std::pair<RigidBody*, RigidBody*>> pairs;
    for (size_t i = 0; i < m_bodies.size(); ++i) {
        for (size_t j = i + 1; j < m_bodies.size(); ++j) {
            auto* a = m_bodies[i];
            auto* b = m_bodies[j];
            if (!a || !b || !a->isActive() || !b->isActive()) continue;
            PhysVec3 cp, cn;
            if (checkCollision(a, b, cp, cn))
                pairs.emplace_back(a, b);
        }
    }
    return pairs;
}

void PhysicsWorld::resolveCollision(RigidBody* bodyA, RigidBody* bodyB,
                                    const PhysVec3& point, const PhysVec3& normal) {
    if (!bodyA || !bodyB) return;
    float restitution = std::min(bodyA->restitution(), bodyB->restitution());
    float friction = std::min(bodyA->friction(), bodyB->friction());
    PhysVec3 relVel = bodyA->velocity() - bodyB->velocity();
    float normalVel = dot(relVel, normal);
    if (normalVel > 0.0f) return;
    float invMassA = bodyA->mass() > 0.0f ? 1.0f / bodyA->mass() : 0.0f;
    float invMassB = bodyB->mass() > 0.0f ? 1.0f / bodyB->mass() : 0.0f;
    float totalInvMass = invMassA + invMassB;
    if (totalInvMass == 0.0f) return;
    float j = -(1.0f + restitution) * normalVel / totalInvMass;
    PhysVec3 impulse = normal * j;
    bodyA->applyImpulse(impulse, point);
    bodyB->applyImpulse(-impulse, point);
    PhysVec3 tangent = relVel - normal * normalVel;
    float tangentSpeed = tangent.length();
    if (tangentSpeed > 0.001f) {
        tangent = tangent / tangentSpeed;
        float frictionImpulse = friction * std::abs(j);
        bodyA->applyImpulse(tangent * frictionImpulse, point);
        bodyB->applyImpulse(-tangent * frictionImpulse, point);
    }
}

PhysicsWorld::RaycastResult PhysicsWorld::raycast(const PhysVec3& origin,
                                                  const PhysVec3& direction,
                                                  float maxDistance) {
    RaycastResult result;
    PhysVec3 dir = direction.normalized();
    for (auto* body : m_bodies) {
        if (!body || !body->isActive() || body->isStatic()) continue;
        PhysVec3 center = body->position();
        float radius = body->collisionShape() ? body->collisionShape()->radius() : 0.5f;
        PhysVec3 toCenter = center - origin;
        float proj = dot(toCenter, dir);
        if (proj < 0.0f || proj > maxDistance) continue;
        float closestDistSq = toCenter.lengthSquared() - proj * proj;
        if (closestDistSq < radius * radius) {
            float hitDist = proj - std::sqrt(radius * radius - closestDistSq);
            if (hitDist < result.distance || !result.hit) {
                result.hit = true;
                result.distance = hitDist;
                result.point = origin + dir * hitDist;
                result.normal = (result.point - center).normalized();
                result.body = body;
            }
        }
    }
    return result;
}

void PhysicsWorld::updateBroadphase() {
    m_bounds.clear();
    m_bounds.reserve(m_bodies.size());
    for (auto* body : m_bodies) {
        if (!body || !body->isActive()) continue;
        BodyBounds bounds;
        bounds.body = body;
        PhysVec3 pos = body->position();
        float radius = body->collisionShape() ? body->collisionShape()->radius() : 0.5f;
        bounds.min = pos - PhysVec3(radius, radius, radius);
        bounds.max = pos + PhysVec3(radius, radius, radius);
        m_bounds.push_back(bounds);
    }
}

void PhysicsWorld::debugDraw() {
    if (!m_debugMode) return;
}

void PhysicsWorld::broadphaseSAP() {
    std::sort(m_bounds.begin(), m_bounds.end(),
              [](const BodyBounds& a, const BodyBounds& b) { return a.min.x < b.min.x; });
}

void PhysicsWorld::broadphaseDBVT() {}

void ParticleSystemConfig::emitParticles(int count) {
    for (int i = 0; i < count && static_cast<int>(particles.size()) < maxCount; ++i) {
        Particle p;
        p.position = emitter.position;
        p.velocity = emitter.direction * emitter.velocity;
        p.lifetime = static_cast<float>(lifetime);
        p.age = 0.0f;
        p.mass = 1.0f;
        p.size = 0.1f;
        p.color = {1.0f, 1.0f, 1.0f, 1.0f};
        particles.push_back(p);
    }
}

void ParticleSystemConfig::update(float deltaTime) {
    if (emitter.rate > 0 && static_cast<int>(particles.size()) < maxCount) {
        int toEmit = std::min(static_cast<int>(emitter.rate * deltaTime),
                              maxCount - static_cast<int>(particles.size()));
        emitParticles(toEmit);
    }
    for (int i = static_cast<int>(particles.size()) - 1; i >= 0; --i) {
        auto& p = particles[i];
        p.age += deltaTime * 60.0f;
        if (!p.isAlive()) {
            particles.erase(particles.begin() + i);
            continue;
        }
        PhysVec3 accel = PhysVec3(physics.gravity[0], physics.gravity[1], physics.gravity[2]);
        if (physics.useWind)
            accel += PhysVec3(physics.wind[0], physics.wind[1], physics.wind[2]);
        p.velocity += accel * deltaTime;
        p.velocity *= (1.0f - physics.damping * deltaTime);
        p.position += p.velocity * deltaTime;
        float t = p.age / p.lifetime;
        p.color = colorRamp(t);
        p.size *= (1.0f + t * 0.5f);
    }
}

void ParticleSystemConfig::clear() { particles.clear(); }

PhysVec4 ParticleSystemConfig::colorRamp(float t) const {
    t = std::clamp(t, 0.0f, 1.0f);
    return PhysVec4(1.0f - t, 1.0f - t * 0.5f, 1.0f - t * 0.3f, 1.0f - t);
}

} // namespace physics
} // namespace ks
