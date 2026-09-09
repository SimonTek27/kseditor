#include "PhysicsEngine.h"
#include "PhysicsLogger.h"
#include <algorithm>
#include <cmath>
#include <QDebug>
#include <QMutex>

namespace ks {
namespace physics {

// ============================================================================
// CollisionShape Implementation
// ============================================================================

CollisionShape::CollisionShape(QObject* parent)
    : QObject(parent)
{
}

CollisionShape::~CollisionShape() = default;

float CollisionShape::computeVolume() const {
    float vol = 1.0f;
    switch (m_type) {
        case Box:
            vol = m_dimensions.x() * m_dimensions.y() * m_dimensions.z();
            break;
        case Sphere:
            vol = (4.0f / 3.0f) * Constants::PI * m_dimensions.x() * m_dimensions.x() * m_dimensions.x();
            break;
        case Capsule:
            vol = Constants::PI * m_dimensions.x() * m_dimensions.x() * 
                  (m_dimensions.y() + (4.0f / 3.0f) * m_dimensions.x());
            break;
        case Cylinder:
            vol = Constants::PI * m_dimensions.x() * m_dimensions.x() * m_dimensions.y();
            break;
        case Cone:
            vol = (1.0f / 3.0f) * Constants::PI * m_dimensions.x() * m_dimensions.x() * m_dimensions.y();
            break;
        default:
            vol = m_dimensions.x() * m_dimensions.y() * m_dimensions.z();
            break;
    }
    return std::max(vol, 0.001f);
}

QVector3D CollisionShape::computeInertia() const {
    QVector3D inertia;
    float volume = computeVolume();
    float density = 1.0f / volume;
    float x = m_dimensions.x();
    float y = m_dimensions.y();
    float z = m_dimensions.z();
    
    switch (m_type) {
        case Box:
            inertia = QVector3D(
                (1.0f / 12.0f) * density * (y * y + z * z),
                (1.0f / 12.0f) * density * (x * x + z * z),
                (1.0f / 12.0f) * density * (x * x + y * y)
            );
            break;
        case Sphere:
            inertia = QVector3D(
                0.4f * density * x * x,
                0.4f * density * x * x,
                0.4f * density * x * x
            );
            break;
        case Capsule: {
            float r = x, h = y;
            float ih = 0.0833f * density * (4.0f * r * r + h * h) + 0.5f * density * r * r;
            float ip = 0.5f * density * r * r;
            inertia = QVector3D(ih, ip, ih);
            break;
        }
        case Cylinder:
            inertia = QVector3D(
                (1.0f / 12.0f) * density * (3.0f * x * x + y * y),
                0.5f * density * x * x,
                (1.0f / 12.0f) * density * (3.0f * x * x + y * y)
            );
            break;
        default:
            inertia = QVector3D(1.0f, 1.0f, 1.0f);
            break;
    }
    return inertia;
}

bool CollisionShape::intersects(const CollisionShape& other, 
                                const QMatrix4x4& transformA,
                                const QMatrix4x4& transformB) const {
    if (m_type == Box && other.m_type == Box) {
        return intersectsBox(other, transformA, transformB);
    }
    if (m_type == Sphere && other.m_type == Sphere) {
        return intersectsSphere(other, transformA, transformB);
    }
    // Default fallback: use sphere-sphere collision
    return intersectsSphere(other, transformA, transformB);
}

bool CollisionShape::intersectsBox(const CollisionShape& other,
                                   const QMatrix4x4& transformA,
                                   const QMatrix4x4& transformB) const {
    // Simplified AABB intersection
    QVector3D centerA = transformA * QVector3D(0, 0, 0);
    QVector3D centerB = transformB * QVector3D(0, 0, 0);
    QVector3D halfA = m_dimensions * 0.5f;
    QVector3D halfB = other.m_dimensions * 0.5f;
    
    QVector3D delta = centerA - centerB;
    return std::abs(delta.x()) < halfA.x() + halfB.x() &&
           std::abs(delta.y()) < halfA.y() + halfB.y() &&
           std::abs(delta.z()) < halfA.z() + halfB.z();
}

bool CollisionShape::intersectsSphere(const CollisionShape& other,
                                      const QMatrix4x4& transformA,
                                      const QMatrix4x4& transformB) const {
    QVector3D centerA = transformA * QVector3D(0, 0, 0);
    QVector3D centerB = transformB * QVector3D(0, 0, 0);
    float radiusA = m_dimensions.x();
    float radiusB = other.m_dimensions.x();
    float distance = (centerA - centerB).length();
    return distance < radiusA + radiusB;
}

// ============================================================================
// RigidBody Implementation
// ============================================================================

RigidBody::RigidBody(QObject* parent)
    : QObject(parent)
{
    PHYSICS_DEBUG("RigidBody", "Created rigid body with mass: %1", m_mass);
}

RigidBody::~RigidBody() = default;

void RigidBody::setTransform(const QMatrix4x4& transform) {
    m_position = transform * QVector3D(0, 0, 0);
    // Extract rotation from matrix (simplified)
    QVector3D scale;
    float det = transform.determinant();
    if (std::abs(det) > 0.001f) {
        // Simplified: just use the matrix as is
        // In production, use proper quaternion extraction
    }
    emit positionChanged();
}

QMatrix4x4 RigidBody::transform() const {
    QMatrix4x4 mat;
    mat.translate(m_position);
    // Add rotation
    mat.rotate(m_rotation.x() * Constants::RAD_TO_DEG, QVector3D(1, 0, 0));
    mat.rotate(m_rotation.y() * Constants::RAD_TO_DEG, QVector3D(0, 1, 0));
    mat.rotate(m_rotation.z() * Constants::RAD_TO_DEG, QVector3D(0, 0, 1));
    return mat;
}

void RigidBody::applyForce(const QVector3D& force, const QVector3D& point) {
    if (m_isStatic || m_isKinematic) return;
    m_accumulatedForce += force;
    if (!point.isNull()) {
        QVector3D r = point - m_position;
        m_accumulatedTorque += QVector3D::crossProduct(r, force);
    }
}

void RigidBody::applyImpulse(const QVector3D& impulse, const QVector3D& point) {
    if (m_isStatic || m_isKinematic) return;
    m_velocity += impulse / m_mass;
    if (!point.isNull()) {
        QVector3D r = point - m_position;
        QVector3D angularImpulse = QVector3D::crossProduct(r, impulse);
        float invInertia = 1.0f / m_mass; // Simplified
        m_angularVelocity += angularImpulse * invInertia;
    }
}

void RigidBody::applyTorque(const QVector3D& torque) {
    if (m_isStatic || m_isKinematic) return;
    m_accumulatedTorque += torque;
}

void RigidBody::clearForces() {
    m_accumulatedForce = QVector3D();
    m_accumulatedTorque = QVector3D();
}

void RigidBody::integrate(float dt) {
    if (m_isStatic || !m_isActive) return;
    
    integrateVelocity(dt);
    integratePosition(dt);
}

void RigidBody::integrateVelocity(float dt) {
    if (m_isStatic || m_isKinematic || !m_isActive) return;
    
    if (m_mass > 0.0f) {
        QVector3D acceleration = m_accumulatedForce / m_mass;
        m_velocity += acceleration * dt;
        m_velocity *= (1.0f - 0.01f * dt);
    }
    
    // Simplified inertia
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
    return m_mass * gravity * m_position.y();
}

float RigidBody::totalEnergy(float gravity) const {
    return kineticEnergy() + potentialEnergy(gravity);
}

// ============================================================================
// PhysicsWorld Implementation
// ============================================================================

PhysicsWorld::PhysicsWorld(QObject* parent)
    : QObject(parent)
{
    PHYSICS_INFO("PhysicsWorld", "Physics world created");
}

PhysicsWorld::~PhysicsWorld() {
    clearBodies();
    PHYSICS_INFO("PhysicsWorld", "Physics world destroyed");
}

RigidBody* PhysicsWorld::createBody(float mass) {
    auto* body = new RigidBody(this);
    body->setMass(mass);
    m_bodies.append(body);
    emit bodyAdded(body);
    PHYSICS_DEBUG("PhysicsWorld", "Created body with mass: %1", mass);
    return body;
}

void PhysicsWorld::addBody(RigidBody* body) {
    if (body && !m_bodies.contains(body)) {
        m_bodies.append(body);
        emit bodyAdded(body);
        PHYSICS_DEBUG("PhysicsWorld", "Added body to world");
    }
}

void PhysicsWorld::removeBody(RigidBody* body) {
    if (m_bodies.removeAll(body) > 0) {
        emit bodyRemoved(body);
        delete body;
        PHYSICS_DEBUG("PhysicsWorld", "Removed body from world");
    }
}

void PhysicsWorld::clearBodies() {
    for (auto* body : m_bodies) {
        delete body;
    }
    m_bodies.clear();
    m_bounds.clear();
    PHYSICS_DEBUG("PhysicsWorld", "Cleared all bodies");
}

void PhysicsWorld::stepSimulation(float deltaTime) {
    if (deltaTime <= 0.0f) return;
    
    // Accumulate time for fixed timestep
    m_accumulatedTime += deltaTime;
    
    // Step at fixed timestep
    while (m_accumulatedTime >= m_fixedTimeStep) {
        stepSimulationFixed(m_fixedTimeStep);
        m_accumulatedTime -= m_fixedTimeStep;
    }
    
    emit stepCompleted();
}

void PhysicsWorld::stepSimulationFixed(float dt) {
    // Update broadphase
    updateBroadphase();
    
    // Find collisions
    auto pairs = getCollisionPairs();
    
    // Resolve collisions
    for (const auto& pair : pairs) {
        QVector3D contactPoint, contactNormal;
        if (checkCollision(pair.first, pair.second, contactPoint, contactNormal)) {
            resolveCollision(pair.first, pair.second, contactPoint, contactNormal);
            emit collisionDetected(pair.first, pair.second, contactPoint, contactNormal);
        }
    }
    
    // Integrate bodies
    for (auto* body : m_bodies) {
        if (body && body->isActive()) {
            body->integrate(dt);
        }
    }
    
    // Solve constraints
    solveConstraints();
}

void PhysicsWorld::solveConstraints() {
    for (int iter = 0; iter < m_solverIterations; ++iter) {
        for (int i = 0; i < m_bodies.size(); ++i) {
            for (int j = i + 1; j < m_bodies.size(); ++j) {
                auto* a = m_bodies[i];
                auto* b = m_bodies[j];
                if (!a || !b || !a->isActive() || !b->isActive()) continue;
                
                // Simple constraint: maintain minimum distance
                QVector3D diff = b->position() - a->position();
                float dist = diff.length();
                float minDist = 0.1f;
                
                if (dist < minDist && dist > 0.001f) {
                    QVector3D dir = diff / dist;
                    float overlap = minDist - dist;
                    QVector3D correction = dir * overlap * 0.5f;
                    a->setPosition(a->position() - correction);
                    b->setPosition(b->position() + correction);
                }
            }
        }
    }
}

bool PhysicsWorld::checkCollision(RigidBody* bodyA, RigidBody* bodyB,
                                  QVector3D& contactPoint, QVector3D& contactNormal) {
    if (!bodyA || !bodyB) return false;
    
    CollisionShape* shapeA = bodyA->collisionShape();
    CollisionShape* shapeB = bodyB->collisionShape();
    
    if (!shapeA || !shapeB) {
        // Default sphere collision
        float radiusA = 0.5f;
        float radiusB = 0.5f;
        QVector3D centerA = bodyA->position();
        QVector3D centerB = bodyB->position();
        QVector3D delta = centerA - centerB;
        float dist = delta.length();
        
        if (dist < radiusA + radiusB && dist > 0.001f) {
            contactNormal = delta / dist;
            contactPoint = centerA - contactNormal * radiusA;
            return true;
        }
        return false;
    }
    
    QMatrix4x4 transformA = bodyA->transform();
    QMatrix4x4 transformB = bodyB->transform();
    
    if (shapeA->intersects(*shapeB, transformA, transformB)) {
        // Approximate contact point and normal
        contactPoint = (bodyA->position() + bodyB->position()) * 0.5f;
        contactNormal = (bodyB->position() - bodyA->position()).normalized();
        return true;
    }
    
    return false;
}

QVector<QPair<RigidBody*, RigidBody*>> PhysicsWorld::getCollisionPairs() {
    QVector<QPair<RigidBody*, RigidBody*>> pairs;
    
    for (int i = 0; i < m_bodies.size(); ++i) {
        for (int j = i + 1; j < m_bodies.size(); ++j) {
            auto* a = m_bodies[i];
            auto* b = m_bodies[j];
            if (!a || !b || !a->isActive() || !b->isActive()) continue;
            
            QVector3D contactPoint, contactNormal;
            if (checkCollision(a, b, contactPoint, contactNormal)) {
                pairs.append(QPair<RigidBody*, RigidBody*>(a, b));
            }
        }
    }
    
    return pairs;
}

void PhysicsWorld::resolveCollision(RigidBody* bodyA, RigidBody* bodyB,
                                    const QVector3D& point, const QVector3D& normal) {
    if (!bodyA || !bodyB) return;
    
    float restitution = std::min(bodyA->restitution(), bodyB->restitution());
    float friction = std::min(bodyA->friction(), bodyB->friction());
    
    // Relative velocity at contact
    QVector3D velA = bodyA->velocity();
    QVector3D velB = bodyB->velocity();
    QVector3D relVel = velA - velB;
    float normalVel = QVector3D::dotProduct(relVel, normal);
    
    if (normalVel > 0.0f) return; // Already separating
    
    // Impulse calculation
    float invMassA = bodyA->mass() > 0.0f ? 1.0f / bodyA->mass() : 0.0f;
    float invMassB = bodyB->mass() > 0.0f ? 1.0f / bodyB->mass() : 0.0f;
    float totalInvMass = invMassA + invMassB;
    
    if (totalInvMass == 0.0f) return;
    
    float j = -(1.0f + restitution) * normalVel / totalInvMass;
    QVector3D impulse = normal * j;
    
    bodyA->applyImpulse(impulse, point);
    bodyB->applyImpulse(-impulse, point);
    
    // Friction
    QVector3D tangent = relVel - normal * normalVel;
    float tangentSpeed = tangent.length();
    if (tangentSpeed > 0.001f) {
        tangent /= tangentSpeed;
        float frictionImpulse = friction * std::abs(j);
        bodyA->applyImpulse(tangent * frictionImpulse, point);
        bodyB->applyImpulse(-tangent * frictionImpulse, point);
    }
}

PhysicsWorld::RaycastResult PhysicsWorld::raycast(const QVector3D& origin,
                                                  const QVector3D& direction,
                                                  float maxDistance) {
    RaycastResult result;
    QVector3D dir = direction.normalized();
    
    for (auto* body : m_bodies) {
        if (!body || !body->isActive() || body->isStatic()) continue;
        
        // Simplified sphere raycast
        QVector3D center = body->position();
        float radius = 0.5f;
        QVector3D toCenter = center - origin;
        float proj = QVector3D::dotProduct(toCenter, dir);
        
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
        
        // Simple AABB
        QVector3D pos = body->position();
        float radius = 0.5f;
        bounds.min = pos - QVector3D(radius, radius, radius);
        bounds.max = pos + QVector3D(radius, radius, radius);
        
        m_bounds.append(bounds);
    }
}

void PhysicsWorld::debugDraw() {
    if (!m_debugMode) return;
    
    // Debug output would go here
    // This could be connected to a visualization system
}

void PhysicsWorld::broadphaseSAP() {
    // Sweep and prune implementation
    // Sort bounds by min.x
    std::sort(m_bounds.begin(), m_bounds.end(),
              [](const BodyBounds& a, const BodyBounds& b) {
                  return a.min.x() < b.min.x();
              });
}

void PhysicsWorld::broadphaseDBVT() {
    // Dynamic Bounding Volume Tree implementation
    // Not implemented in this simplified version
}

// ============================================================================
// ParticleSystemConfig Implementation
// ============================================================================

void ParticleSystemConfig::emitParticles(int count) {
    for (int i = 0; i < count && particles.size() < maxCount; ++i) {
        Particle p;
        p.position = emitter.position;
        p.velocity = emitter.direction * emitter.velocity;
        p.lifetime = lifetime;
        p.age = 0.0f;
        p.mass = 1.0f;
        p.size = 0.1f;
        p.color = {1.0f, 1.0f, 1.0f, 1.0f};
        particles.append(p);
    }
}

void ParticleSystemConfig::update(float deltaTime) {
    // Emit new particles
    if (emitter.rate > 0 && particles.size() < maxCount) {
        int toEmit = std::min(static_cast<int>(emitter.rate * deltaTime),
                              maxCount - static_cast<int>(particles.size()));
        emitParticles(toEmit);
    }
    
    // Update existing particles
    for (int i = particles.size() - 1; i >= 0; --i) {
        auto& p = particles[i];
        p.age += deltaTime * 60.0f;
        
        if (!p.isAlive()) {
            particles.removeAt(i);
            continue;
        }
        
        // Apply physics
        QVector3D accel = QVector3D(physics.gravity[0], physics.gravity[1], physics.gravity[2]);
        if (physics.useWind) {
            accel += QVector3D(physics.wind[0], physics.wind[1], physics.wind[2]);
        }
        p.velocity += accel * deltaTime;
        p.velocity *= (1.0f - physics.damping * deltaTime);
        p.position += p.velocity * deltaTime;
        
        // Apply color ramp
        float t = p.age / p.lifetime;
        p.color = colorRamp(t);
        p.size *= (1.0f + t * 0.5f);
    }
}

void ParticleSystemConfig::clear() {
    particles.clear();
}

QVector4D ParticleSystemConfig::colorRamp(float t) const {
    t = std::clamp(t, 0.0f, 1.0f);
    float r = 1.0f - t;
    float g = 1.0f - t * 0.5f;
    float b = 1.0f - t * 0.3f;
    float a = 1.0f - t;
    return QVector4D(r, g, b, a);
}

} // namespace physics
} // namespace ks