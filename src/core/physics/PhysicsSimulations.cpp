#include "PhysicsSimulations.h"
#include <algorithm>
#include <cmath>
#include <QDebug>

namespace ks { namespace physics {

// ============================================================================
// SoftBodySimulator implementation
// ============================================================================

SoftBodySimulator::SoftBodySimulator(QObject* parent)
    : QObject(parent)
{}

SoftBodySimulator::~SoftBodySimulator() {}

void SoftBodySimulator::setMesh(const QVector<QVector3D>& vertices, const QVector<int>& faces) {
    m_positions = vertices;
    m_faces = faces;
    m_velocities.resize(vertices.size());
    buildConstraints();
}

void SoftBodySimulator::setConfig(const SoftBody& config) {
    m_config = config;
}

void SoftBodySimulator::simulate(int frameStep) {
    applyForces(1.0f / 60.0f);
    satisfyConstraints(m_config.iterationCount);
    emit simulationStep(frameStep);
}

void SoftBodySimulator::addForce(const QVector3D& force) {
    m_externalForce += force;
}

void SoftBodySimulator::setGravity(const QVector3D& gravity) {
    m_config.mass; // placeholder
}

void SoftBodySimulator::setWind(const QVector3D& wind, float noise) {
    m_wind = wind;
}

void SoftBodySimulator::pinVertex(int index) {
    if (index >= 0 && index < m_pinnedVertices.size()) return;
    m_pinnedVertices.append(index);
}

void SoftBodySimulator::unpinVertex(int index) {
    m_pinnedVertices.removeAll(index);
}

void SoftBodySimulator::pinVertexGroup(const QVector<int>& indices) {
    for (int idx : indices) pinVertex(idx);
}

void SoftBodySimulator::reset() {
    m_velocities.fill(QVector3D());
}

void SoftBodySimulator::buildConstraints() {
    m_edges.clear();
    m_restLengths.clear();

    for (int i = 0; i < m_faces.size(); i += 3) {
        int a = m_faces[i];
        int b = m_faces[i + 1];
        int c = m_faces[i + 2];

        auto addEdge = [this](int v1, int v2) {
            if (v1 > v2) std::swap(v1, v2);
            for (const auto& e : m_edges) {
                if (e.first == v1 && e.second == v2) return;
            }
            m_edges.append(qMakePair(v1, v2));
            float len = (m_positions[v1] - m_positions[v2]).length();
            m_restLengths.append(len);
        };

        addEdge(a, b);
        addEdge(b, c);
        addEdge(c, a);
    }
}

void SoftBodySimulator::satisfyConstraints(int iterations) {
    for (int iter = 0; iter < iterations; ++iter) {
        for (int i = 0; i < m_edges.size(); ++i) {
            int v1 = m_edges[i].first;
            int v2 = m_edges[i].second;
            float restLen = m_restLengths[i];

            QVector3D delta = m_positions[v2] - m_positions[v1];
            float dist = delta.length();
            if (dist > 0.001f) {
                QVector3D correction = delta * (0.5f * (dist - restLen) / dist);
                if (!m_pinnedVertices.contains(v1))
                    m_positions[v1] += correction;
                if (!m_pinnedVertices.contains(v2))
                    m_positions[v2] -= correction;
            }
        }
    }
}

void SoftBodySimulator::applyForces(float deltaTime) {
    for (int i = 0; i < m_positions.size(); ++i) {
        if (m_pinnedVertices.contains(i)) continue;

        QVector3D force = m_externalForce + m_wind;
        force.setY(force.y() - 9.81f * m_config.mass);

        m_velocities[i] += force * (deltaTime / m_config.mass);
        m_velocities[i] *= (1.0f - m_config.damping * deltaTime);
        m_positions[i] += m_velocities[i] * deltaTime;
    }

    m_externalForce = QVector3D();
}

void SoftBodySimulator::resolveCollisions() {
    // Ground collision
    for (int i = 0; i < m_positions.size(); ++i) {
        if (m_pinnedVertices.contains(i)) continue;

        if (m_positions[i].y() < m_config.collisionMargin) {
            m_positions[i].setY(m_config.collisionMargin);
            if (m_velocities[i].y() < 0) {
                m_velocities[i].setY(-m_velocities[i].y() * 0.3f);
            }
        }
    }
}

// ============================================================================
// ClothSimulator implementation
// ============================================================================

ClothSimulator::ClothSimulator(QObject* parent)
    : QObject(parent), m_cloth(nullptr)
{}

ClothSimulator::~ClothSimulator() {}

void ClothSimulator::setCloth(Cloth* cloth) {
    m_cloth = cloth;
    if (cloth) {
        m_positions.resize(cloth->vertices.size());
        m_previousPositions.resize(cloth->vertices.size());
        for (int i = 0; i < cloth->vertices.size(); ++i) {
            m_positions[i] = cloth->vertices[i].position;
            m_previousPositions[i] = cloth->vertices[i].previousPosition;
        }
    }
}

void ClothSimulator::simulate(float deltaTime) {
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

void ClothSimulator::integrateVerlet(float deltaTime) {
    if (!m_cloth) return;

    for (int i = 0; i < m_cloth->vertices.size(); ++i) {
        if (m_cloth->vertices[i].pinned) continue;

        QVector3D vel = m_positions[i] - m_previousPositions[i];
        QVector3D accel = m_cloth->vertices[i].acceleration;

        accel.setY(accel.y() - 9.81f);

        m_previousPositions[i] = m_positions[i];
        m_positions[i] += vel * m_cloth->velocitySmooth + accel * deltaTime * deltaTime;
    }
}

void ClothSimulator::satisfyConstraints(float deltaTime) {
    if (!m_cloth) return;

    for (int iter = 0; iter < 5; ++iter) {
        for (const auto& c : m_cloth->constraints) {
            QVector3D p1 = m_positions[c.vertex1];
            QVector3D p2 = m_positions[c.vertex2];
            QVector3D delta = p2 - p1;
            float dist = delta.length();

            if (dist > 0.001f) {
                float diff = (dist - c.restLength) / dist;
                QVector3D correction = delta * 0.5f * diff * c.stiffness;

                if (!m_cloth->vertices[c.vertex1].pinned)
                    m_positions[c.vertex1] += correction;
                if (!m_cloth->vertices[c.vertex2].pinned)
                    m_positions[c.vertex2] -= correction;
            }
        }
    }
}

void ClothSimulator::satisfyCollisionConstraints() {
    for (int i = 0; i < m_positions.size(); ++i) {
        if (m_cloth && m_cloth->vertices[i].pinned) continue;

        if (m_positions[i].y() < 0.0f) {
            m_positions[i].setY(0.0f);
        }
    }
}

void ClothSimulator::addCollisionSphere(const QVector3D& center, float radius) {
    m_collisionSpheres.append({center, radius});
}

void ClothSimulator::addCollisionBox(const QVector<QVector3D>& corners) {
    QVector3D min = corners[0], max = corners[0];
    for (const auto& c : corners) {
        min = QVector3D(std::min(min.x(), c.x()), std::min(min.y(), c.y()), std::min(min.z(), c.z()));
        max = QVector3D(std::max(max.x(), c.x()), std::max(max.y(), c.y()), std::max(max.z(), c.z()));
    }
    m_collisionBoxes.append({corners, min, max});
}

void ClothSimulator::clearCollisions() {
    m_collisionSpheres.clear();
    m_collisionBoxes.clear();
}

void ClothSimulator::setPinnedVertices(const QVector<int>& indices) {
    if (!m_cloth) return;
    for (int i = 0; i < m_cloth->vertices.size(); ++i) {
        m_cloth->vertices[i].pinned = false;
    }
    for (int idx : indices) {
        if (idx >= 0 && idx < m_cloth->vertices.size()) {
            m_cloth->vertices[idx].pinned = true;
        }
    }
}

// ============================================================================
// ParticleSystemSimulator implementation
// ============================================================================

ParticleSystemSimulator::ParticleSystemSimulator(QObject* parent)
    : QObject(parent)
{}

ParticleSystemSimulator::~ParticleSystemSimulator() {}

void ParticleSystemSimulator::setEmitter(const ParticleSystem::Emitter& emitter) {
    m_emitter = emitter;
}

void ParticleSystemSimulator::setPhysics(const ParticleSystem::Physics& physics) {
    m_physics = physics;
}

void ParticleSystemSimulator::setMaxCount(int count) {
    m_maxCount = count;
}

void ParticleSystemSimulator::setLifetime(int frames) {
    m_lifetime = frames;
}

void ParticleSystemSimulator::simulate(float deltaTime) {
    // Emit new particles
    if (m_emitter.rate > 0 && m_particles.size() < m_maxCount) {
        int toEmit = std::min<int>(static_cast<int>(m_emitter.rate * deltaTime), m_maxCount - static_cast<int>(m_particles.size()));
        for (int i = 0; i < toEmit; ++i) {
            ParticleSystem::Particle p;
            p.position = m_emitter.position;
            p.velocity = m_emitter.direction * m_emitter.velocity;
            p.acceleration = QVector3D();
            p.lifetime = m_lifetime;
            p.age = 0;
            p.mass = 1.0f;
            p.size = 0.1f;
            p.color = QVector4D(1, 1, 1, 1);
            m_particles.append(p);
        }
    }

    // Update particles
    for (int i = m_particles.size() - 1; i >= 0; --i) {
        auto& p = m_particles[i];
        p.age += deltaTime * 60.0f;
        if (!p.isAlive()) {
            m_particles.removeAt(i);
            continue;
        }

        QVector3D accel = QVector3D(m_physics.gravity[0], m_physics.gravity[1], m_physics.gravity[2]);
        if (m_physics.useWind) {
            accel += QVector3D(m_physics.wind[0], m_physics.wind[1], m_physics.wind[2]);
        }
        p.velocity += accel * deltaTime;
        p.velocity *= (1.0f - m_physics.damping * deltaTime);
        p.position += p.velocity * deltaTime;
    }

    emit particlesUpdated();
}

// ============================================================================
// FluidSimulator implementation
// ============================================================================

FluidSimulator::FluidSimulator(QObject* parent)
    : QObject(parent)
{}

FluidSimulator::~FluidSimulator() {}

void FluidSimulator::addParticles(const QVector<QVector3D>& positions) {
    for (const auto& pos : positions) {
        FluidParticle p;
        p.position = pos;
        p.velocity = QVector3D();
        p.density = restDensity;
        p.pressure = 0;
        fluidParticles.append(p);
    }
}

void FluidSimulator::simulate(float deltaTime) {
    deltaTime *= timeScale;

    // Compute densities
    for (int i = 0; i < fluidParticles.size(); ++i) {
        fluidParticles[i].density = computeDensity(fluidParticles[i].position);
    }

    // Compute pressures
    computePressures();

    // Apply forces
    for (int i = 0; i < fluidParticles.size(); ++i) {
        auto& p = fluidParticles[i];
        QVector3D force = QVector3D(gravity[0], gravity[1], gravity[2]) * p.density;

        // Pressure gradient
        QVector3D grad = computeDensityGradient(p.position);
        force -= grad * stiffness;

        // Viscosity (simplified)
        force -= p.velocity * viscosity;

        p.velocity += force * (deltaTime / p.density);
        p.position += p.velocity * deltaTime;
    }

    // Boundary constraints
    for (auto& p : fluidParticles) {
        for (const auto& b : m_boundaries) {
            // Simplified - just keep particles within bounds
            if (p.position.y() < b.y()) p.position.setY(b.y());
        }
    }

    emit fluidUpdated();
}

void FluidSimulator::setBoundaries(const QVector<QVector3D>& boundaries) {
    m_boundaries = boundaries;
}

float FluidSimulator::getParticleDensity(int index) const {
    if (index >= 0 && index < fluidParticles.size())
        return fluidParticles[index].density;
    return 0;
}

float FluidSimulator::getParticlePressure(int index) const {
    if (index >= 0 && index < fluidParticles.size())
        return fluidParticles[index].pressure;
    return 0;
}

float FluidSimulator::computeDensity(const QVector3D& position) {
    float density = 0;
    float h = smoothingRadius;
    float h2 = h * h;

    for (const auto& p : fluidParticles) {
        float r2 = (p.position - position).lengthSquared();
        if (r2 < h2) {
            float w = (h2 - r2) / h2;
            density += w * w * w; // SPH poly6 kernel
        }
    }
    return density * restDensity;
}

QVector3D FluidSimulator::computeDensityGradient(const QVector3D& position) {
    QVector3D grad;
    float h = smoothingRadius;
    float h2 = h * h;

    for (const auto& p : fluidParticles) {
        QVector3D diff = position - p.position;
        float r2 = diff.lengthSquared();
        if (r2 < h2 && r2 > 0.0001f) {
            float r = std::sqrt(r2);
            float w = 6.0f * (h - r) / (3.14159f * h2 * h2 * r); // spiky kernel gradient
            grad += diff * w;
        }
    }
    return grad;
}

void FluidSimulator::computePressures() {
    for (auto& p : fluidParticles) {
        p.pressure = stiffness * (p.density - restDensity);
    }
}

// ============================================================================
// HairSystem implementation
// ============================================================================

HairSystem::HairSystem(QObject* parent)
    : QObject(parent)
{}

HairSystem::~HairSystem() {}

void HairSystem::addStrand(int rootVertex, int count) {
    HairStrand strand;
    strand.rootVertex = rootVertex;
    strand.points.resize(segments + 1);
    strand.velocities.resize(segments + 1);

    QVector3D base(0, 0, 0);
    for (int i = 0; i <= segments; ++i) {
        strand.points[i] = base + QVector3D(0, -i * segmentLength, 0);
        strand.velocities[i] = QVector3D();
    }

    strands.append(strand);
}

void HairSystem::removeStrand(int index) {
    if (index >= 0 && index < strands.size())
        strands.removeAt(index);
}

void HairSystem::simulate(float deltaTime) {
    for (auto& strand : strands) {
        simulateStrand(strand, deltaTime);
    }
}

void HairSystem::simulateStrand(HairStrand& strand, float deltaTime) {
    // Fixed root
    if (strand.points.isEmpty()) return;
    QVector3D rootPos = strand.points[0];

    // Verlet integration
    for (int i = 1; i < strand.points.size(); ++i) {
        QVector3D vel = strand.points[i] - strand.velocities[i];
        strand.velocities[i] = strand.points[i];

        QVector3D gravityVec(gravity[0], gravity[1], gravity[2]);
        QVector3D accel = gravityVec * dynamics;
        accel -= vel * damping;

        strand.points[i] += vel + accel * deltaTime * deltaTime;
    }

    // Constraint projection
    for (int iter = 0; iter < 4; ++iter) {
        strand.points[0] = rootPos;
        for (int i = 1; i < strand.points.size(); ++i) {
            QVector3D dir = strand.points[i] - strand.points[i-1];
            float dist = dir.length();
            if (dist > 0.001f) {
                dir /= dist;
                strand.points[i] = strand.points[i-1] + dir * segmentLength;
            }
        }
    }
}

QVector<QVector3D> HairSystem::getCompletedStrands() const {
    QVector<QVector3D> result;
    for (const auto& strand : strands) {
        for (const auto& p : strand.points) {
            result.append(p);
        }
    }
    return result;
}

} // namespace physics
} // namespace ks