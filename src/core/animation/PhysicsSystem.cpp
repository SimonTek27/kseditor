#include "PhysicsSystem.h"
#include <cmath>
#include <algorithm>
#include <QRandomGenerator>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace ks {
namespace physics {

// ============================================================================
// SoftBodySimulator
// ============================================================================

SoftBodySimulator::SoftBodySimulator(QObject* parent) : QObject(parent) {}
SoftBodySimulator::~SoftBodySimulator() = default;

void SoftBodySimulator::setMesh(const QVector<QVector3D>& vertices, const QVector<int>& faces)
{
    m_positions = vertices;
    m_velocities.resize(vertices.size());
    m_faces = faces;
    m_restLengths.clear();
    m_edges.clear();
    buildConstraints();
}

void SoftBodySimulator::setConfig(const SoftBody& config)
{
    m_config = config;
}

void SoftBodySimulator::simulate(int frameStep)
{
    float dt = 1.0f / (60.0f * frameStep);
    applyForces(dt);
    satisfyConstraints(m_config.iterationCount);
    resolveCollisions();
    emit simulationStep(frameStep);
}

void SoftBodySimulator::addForce(const QVector3D& force)
{
    m_externalForce = force;
}

void SoftBodySimulator::setGravity(const QVector3D& gravity)
{
    m_externalForce = gravity;
}

void SoftBodySimulator::setWind(const QVector3D& wind, float noise)
{
    m_wind = wind;
    if (noise > 0.0f) {
        m_wind += QVector3D(
            (QRandomGenerator::global()->generateDouble() - 0.5f) * noise,
            (QRandomGenerator::global()->generateDouble() - 0.5f) * noise,
            (QRandomGenerator::global()->generateDouble() - 0.5f) * noise
        );
    }
}

void SoftBodySimulator::pinVertex(int index)
{
    if (!m_pinnedVertices.contains(index)) {
        m_pinnedVertices.append(index);
    }
}

void SoftBodySimulator::unpinVertex(int index)
{
    m_pinnedVertices.removeAll(index);
}

void SoftBodySimulator::pinVertexGroup(const QVector<int>& indices)
{
    for (int idx : indices) pinVertex(idx);
}

void SoftBodySimulator::reset()
{
    m_velocities.fill(QVector3D());
    m_externalForce = QVector3D();
    m_wind = QVector3D();
    m_pinnedVertices.clear();
}

void SoftBodySimulator::buildConstraints()
{
    QSet<QPair<int, int>> edgeSet;

    for (int i = 0; i < m_faces.size(); i += 3) {
        int i0 = m_faces[i];
        int i1 = m_faces[i + 1];
        int i2 = m_faces[i + 2];

        auto addEdge = [&](int a, int b) {
            if (a > b) std::swap(a, b);
            QPair<int, int> edge(a, b);
            if (!edgeSet.contains(edge)) {
                edgeSet.insert(edge);
                m_edges.append(edge);
                float len = (m_positions[a] - m_positions[b]).length();
                m_restLengths.append(len);
            }
        };

        addEdge(i0, i1);
        addEdge(i1, i2);
        addEdge(i2, i0);
    }
}

void SoftBodySimulator::satisfyConstraints(int iterations)
{
    for (int iter = 0; iter < iterations; ++iter) {
        for (int i = 0; i < m_edges.size(); ++i) {
            int v1 = m_edges[i].first;
            int v2 = m_edges[i].second;

            if (m_pinnedVertices.contains(v1) || m_pinnedVertices.contains(v2)) continue;

            QVector3D diff = m_positions[v2] - m_positions[v1];
            float dist = diff.length();
            if (dist < 1e-6f) continue;

            float restLen = m_restLengths[i];
            float correction = (dist - restLen) / dist * 0.5f * m_config.stiffness;

            m_positions[v1] += diff * correction;
            m_positions[v2] -= diff * correction;
        }
    }
}

void SoftBodySimulator::applyForces(float deltaTime)
{
    float dt2 = deltaTime * deltaTime;

    for (int i = 0; i < m_positions.size(); ++i) {
        if (m_pinnedVertices.contains(i)) continue;

        QVector3D force = m_externalForce + m_wind;
        m_velocities[i] += force * dt2;
        m_velocities[i] *= (1.0f - m_config.damping);
        m_positions[i] += m_velocities[i];
    }
}

void SoftBodySimulator::resolveCollisions()
{
    for (int i = 0; i < m_positions.size(); ++i) {
        if (m_pinnedVertices.contains(i)) continue;

        for (int j = i + 1; j < m_positions.size(); ++j) {
            if (m_pinnedVertices.contains(j)) continue;

            QVector3D diff = m_positions[i] - m_positions[j];
            float dist = diff.length();
            float minDist = m_config.collisionMargin * 2.0f;

            if (dist < minDist && dist > 1e-6f) {
                QVector3D correction = diff.normalized() * (minDist - dist) * 0.5f;
                m_positions[i] += correction;
                m_positions[j] -= correction;
            }
        }
    }
}

// ============================================================================
// ClothSimulator
// ============================================================================

ClothSimulator::ClothSimulator(QObject* parent) : QObject(parent), m_cloth(nullptr) {}
ClothSimulator::~ClothSimulator() = default;

void ClothSimulator::setCloth(Cloth* cloth)
{
    m_cloth = cloth;
    m_positions.resize(cloth->vertices.size());
    m_previousPositions.resize(cloth->vertices.size());
    for (int i = 0; i < cloth->vertices.size(); ++i) {
        m_positions[i] = cloth->vertices[i].position;
        m_previousPositions[i] = cloth->vertices[i].previousPosition;
    }
}

void ClothSimulator::simulate(float deltaTime)
{
    if (!m_cloth) return;

    integrateVerlet(deltaTime);
    satisfyConstraints(deltaTime);
    satisfyCollisionConstraints();

    for (int i = 0; i < m_cloth->vertices.size(); ++i) {
        m_cloth->vertices[i].position = m_positions[i];
        m_cloth->vertices[i].previousPosition = m_previousPositions[i];
    }

    emit clothUpdated();
}

void ClothSimulator::addCollisionSphere(const QVector3D& center, float radius)
{
    CollisionSphere sphere;
    sphere.center = center;
    sphere.radius = radius;
    m_collisionSpheres.append(sphere);
}

void ClothSimulator::addCollisionBox(const QVector<QVector3D>& corners)
{
    if (corners.size() < 8) return;

    CollisionBox box;
    box.corners = corners;

    QVector3D minV(1e9f, 1e9f, 1e9f), maxV(-1e9f, -1e9f, -1e9f);
    for (const auto& c : corners) {
        minV.setX(std::min(minV.x(), c.x()));
        minV.setY(std::min(minV.y(), c.y()));
        minV.setZ(std::min(minV.z(), c.z()));
        maxV.setX(std::max(maxV.x(), c.x()));
        maxV.setY(std::max(maxV.y(), c.y()));
        maxV.setZ(std::max(maxV.z(), c.z()));
    }
    box.min = minV;
    box.max = maxV;

    m_collisionBoxes.append(box);
}

void ClothSimulator::clearCollisions()
{
    m_collisionSpheres.clear();
}

void ClothSimulator::setPinnedVertices(const QVector<int>& indices)
{
    if (!m_cloth) return;
    for (int idx : indices) {
        if (idx >= 0 && idx < m_cloth->vertices.size()) {
            m_cloth->vertices[idx].pinned = true;
        }
    }
}

void ClothSimulator::integrateVerlet(float deltaTime)
{
    QVector3D gravity(m_cloth->gravity[0], m_cloth->gravity[1], m_cloth->gravity[2]);
    float dt2 = deltaTime * deltaTime;

    for (int i = 0; i < m_positions.size(); ++i) {
        if (m_cloth->vertices[i].pinned) continue;

        QVector3D temp = m_positions[i];
        m_positions[i] = 2.0f * m_positions[i] - m_previousPositions[i] + gravity * dt2;

        if (m_cloth->wind.lengthSquared() > 0.0f) {
            m_positions[i] += m_cloth->wind * dt2;
        }

        m_previousPositions[i] = temp;
    }
}

void ClothSimulator::satisfyConstraints(float deltaTime)
{
    float damping = std::exp(-m_cloth->damping * deltaTime);
    if (!m_cloth) return;

    for (int iter = 0; iter < 5; ++iter) {
        for (const auto& constraint : m_cloth->constraints) {
            QVector3D diff = m_positions[constraint.vertex2] - m_positions[constraint.vertex1];
            float dist = diff.length();
            if (dist < 1e-6f) continue;

            float correction = (dist - constraint.restLength) / dist * 0.5f * constraint.stiffness;
            QVector3D adjust = diff * correction;

            if (!m_cloth->vertices[constraint.vertex1].pinned)
                m_positions[constraint.vertex1] += adjust;
            if (!m_cloth->vertices[constraint.vertex2].pinned)
                m_positions[constraint.vertex2] -= adjust;
        }
    }
}

void ClothSimulator::satisfyCollisionConstraints()
{
    for (int i = 0; i < m_positions.size(); ++i) {
        if (m_cloth && m_cloth->vertices[i].pinned) continue;

        for (const auto& sphere : m_collisionSpheres) {
            QVector3D diff = m_positions[i] - sphere.center;
            float dist = diff.length();
            if (dist < sphere.radius) {
                m_positions[i] = sphere.center + diff.normalized() * sphere.radius;
            }
        }
    }
}

// ============================================================================
// ParticleSystem
// ============================================================================

void ParticleSystem::emitParticles(int count)
{
    for (int i = 0; i < count; ++i) {
        if (particles.size() >= maxCount) {
            for (int j = 0; j < particles.size(); ++j) {
                if (!particles[j].isAlive()) {
                    Particle p;
                    p.position = emitter.position;
                    p.lifetime = lifetime;
                    p.age = 0;
                    p.mass = 1.0f;
                    p.size = 1.0f;
                    p.color = QVector4D(1, 1, 1, 1);
                    particles[j] = p;
                    break;
                }
            }
            continue;
        }

        Particle p;
        p.position = emitter.position;
        p.lifetime = lifetime;
        p.age = 0;
        p.mass = 1.0f;
        p.size = 1.0f;
        p.color = QVector4D(1, 1, 1, 1);

        QVector3D dir = emitter.direction;
        if (emitter.angle > 0.0f) {
            float spread = qDegreesToRadians(emitter.angle);
            dir += QVector3D(
                (QRandomGenerator::global()->generateDouble() - 0.5f) * spread,
                (QRandomGenerator::global()->generateDouble() - 0.5f) * spread,
                (QRandomGenerator::global()->generateDouble() - 0.5f) * spread
            );
        }
        p.velocity = dir.normalized() * emitter.velocity;
        particles.append(p);
    }
}

void ParticleSystem::update(float deltaTime)
{
    QVector3D grav(physics.gravity[0], physics.gravity[1], physics.gravity[2]);
    QVector3D wind(physics.wind[0], physics.wind[1], physics.wind[2]);

    for (int i = 0; i < particles.size(); ++i) {
        auto& p = particles[i];
        if (!p.isAlive()) continue;

        p.age += deltaTime;
        p.acceleration = grav;
        if (physics.useWind) p.acceleration += wind;
        if (physics.damping > 0) p.velocity *= (1.0f - physics.damping * deltaTime);
        if (physics.brownian > 0) {
            p.acceleration += QVector3D(
                (QRandomGenerator::global()->generateDouble() - 0.5f) * physics.brownian,
                (QRandomGenerator::global()->generateDouble() - 0.5f) * physics.brownian,
                (QRandomGenerator::global()->generateDouble() - 0.5f) * physics.brownian
            );
        }

        p.velocity += p.acceleration * deltaTime;
        p.position += p.velocity * deltaTime;
    }

    particles.erase(std::remove_if(particles.begin(), particles.end(),
        [](const Particle& p) { return !p.isAlive(); }), particles.end());
}

void ParticleSystem::clear()
{
    particles.clear();
}

QVector4D ParticleSystem::colorRamp(float t)
{
    t = qBound(0.0f, t, 1.0f);
    if (t < 0.25f) return QVector4D(1, 1, 1, 1);
    if (t < 0.5f) return QVector4D(1, 0.8f, 0.2f, 1);
    if (t < 0.75f) return QVector4D(1, 0.4f, 0.1f, 1);
    return QVector4D(0.5f, 0.5f, 0.5f, 0);
}

// ============================================================================
// FluidSimulator
// ============================================================================

FluidSimulator::FluidSimulator(QObject* parent) : QObject(parent) {}
FluidSimulator::~FluidSimulator() = default;

void FluidSimulator::addParticles(const QVector<QVector3D>& positions)
{
    for (const auto& pos : positions) {
        FluidParticle p;
        p.position = pos;
        p.velocity = QVector3D();
        p.density = 0;
        p.pressure = 0;
        fluidParticles.append(p);
    }
}

void FluidSimulator::simulate(float deltaTime)
{
    computePressures();

    QVector3D grav(gravity[0], gravity[1], gravity[2]);

    for (int i = 0; i < fluidParticles.size(); ++i) {
        auto& p = fluidParticles[i];
        QVector3D force = grav * deltaTime;

        for (int j = 0; j < fluidParticles.size(); ++j) {
            if (i == j) continue;
            QVector3D diff = p.position - fluidParticles[j].position;
            float dist = diff.length();
            if (dist < smoothingRadius && dist > 1e-6f) {
                float pressureForce = (p.pressure + fluidParticles[j].pressure) / (2.0f * fluidParticles[j].density);
                QVector3D dir = diff.normalized();
                float kernel = 1.0f - dist / smoothingRadius;
                force -= dir * pressureForce * kernel * stiffness;
                if (viscosity > 0) {
                    force += (fluidParticles[j].velocity - p.velocity) * viscosity * kernel;
                }
            }
        }

        p.velocity += force * deltaTime;
        p.position += p.velocity * deltaTime;

        for (const auto& boundary : m_boundaries) {
            QVector3D diff = p.position - boundary;
            float dist = diff.length();
            if (dist < smoothingRadius) {
                p.position = boundary + diff.normalized() * smoothingRadius;
                p.velocity *= -0.5f;
            }
        }
    }

    emit fluidUpdated();
}

void FluidSimulator::setBoundaries(const QVector<QVector3D>& boundaries)
{
    m_boundaries = boundaries;
}

float FluidSimulator::getParticleDensity(int index) const
{
    if (index >= 0 && index < fluidParticles.size()) {
        return fluidParticles[index].density;
    }
    return 0;
}

float FluidSimulator::getParticlePressure(int index) const
{
    if (index >= 0 && index < fluidParticles.size()) {
        return fluidParticles[index].pressure;
    }
    return 0;
}

float FluidSimulator::computeDensity(const QVector3D& position)
{
    float density = 0;
    for (const auto& p : fluidParticles) {
        float dist = (position - p.position).length();
        if (dist < smoothingRadius) {
            float kernel = 1.0f - dist / smoothingRadius;
            density += kernel * kernel;
        }
    }
    return density;
}

QVector3D FluidSimulator::computeDensityGradient(const QVector3D& position)
{
    QVector3D gradient;
    for (const auto& p : fluidParticles) {
        float dist = (position - p.position).length();
        if (dist < smoothingRadius && dist > 1e-6f) {
            gradient += (position - p.position).normalized() * (1.0f - dist / smoothingRadius);
        }
    }
    return gradient;
}

void FluidSimulator::computePressures()
{
    for (auto& p : fluidParticles) {
        p.density = computeDensity(p.position);
        if (p.density > 0) {
            p.pressure = stiffness * (p.density - restDensity);
        }
    }
}

// ============================================================================
// HairSystem
// ============================================================================

HairSystem::HairSystem(QObject* parent) : QObject(parent) {}
HairSystem::~HairSystem() = default;

void HairSystem::addStrand(int rootVertex, int count)
{
    HairStrand strand;
    strand.rootVertex = rootVertex;

    QVector3D rootPos;
    for (int i = 0; i < count; ++i) {
        strand.points.append(rootPos + QVector3D(0, -segmentLength * i, 0));
        strand.velocities.append(QVector3D());
    }

    strands.append(strand);
}

void HairSystem::removeStrand(int index)
{
    if (index >= 0 && index < strands.size()) {
        strands.removeAt(index);
    }
}

void HairSystem::simulate(float deltaTime)
{
    QVector3D grav(gravity[0], gravity[1], gravity[2]);

    for (auto& strand : strands) {
        simulateStrand(strand, deltaTime);

        for (int i = 1; i < strand.points.size(); ++i) {
            QVector3D diff = strand.points[i] - strand.points[i - 1];
            float dist = diff.length();
            if (dist > segmentLength) {
                QVector3D correction = diff.normalized() * (dist - segmentLength) * 0.5f;
                strand.points[i] -= correction;
                if (i > 1) strand.points[i - 1] += correction;
            }
        }
    }

    emit hairUpdated();
}

QVector<QVector3D> HairSystem::getCompletedStrands() const
{
    QVector<QVector3D> result;
    for (const auto& strand : strands) {
        result.append(strand.points);
    }
    return result;
}

void HairSystem::simulateStrand(HairStrand& strand, float deltaTime)
{
    QVector3D grav(gravity[0], gravity[1], gravity[2]);

    for (int i = 1; i < strand.points.size(); ++i) {
        strand.velocities[i] += grav * deltaTime;
        strand.velocities[i] *= (1.0f - damping);
        strand.points[i] += strand.velocities[i] * deltaTime * dynamics;
    }
}

}
}
