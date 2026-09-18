#include "GPUParticleSystem.h"
#include <QDebug>
#include <QtMath>
#include <algorithm>
#include <random>
#include <QJsonArray>
#include <QJsonObject>

namespace ks::engine::graphics {

static std::mt19937 s_rng(std::random_device{}());

static float randomFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(s_rng);
}

static QVector3D randomDirection(float spreadAngle) {
    float angle = qDegreesToRadians(spreadAngle);
    float theta = randomFloat(0.0f, 2.0f * M_PI);
    float phi = randomFloat(0.0f, angle);
    float sinPhi = qSin(phi);
    return QVector3D(sinPhi * qCos(theta), qCos(phi), sinPhi * qSin(theta));
}

bool GPUParticleSystem::initialize() {
    if (m_initialized) return true;
    m_particles.resize(m_maxTotalParticles);
    for (auto& p : m_particles) {
        p.alive = 0;
        p.position = QVector3D(0, -99999, 0);
    }
    m_initialized = true;
    qInfo() << "GPUParticleSystem: initialized, max particles:" << m_maxTotalParticles;
    return true;
}

void GPUParticleSystem::shutdown() {
    if (!m_initialized) return;
    m_particles.clear();
    m_emitters.clear();
    m_activeCount = 0;
    m_initialized = false;
    qInfo() << "GPUParticleSystem: shutdown";
}

QUuid GPUParticleSystem::createEmitter(const ParticleEmitterConfig& config) {
    QUuid id = QUuid::createUuid();
    Emitter emitter;
    emitter.id = id;
    emitter.config = config;
    emitter.active = false;
    emitter.emissionAccumulator = 0.0f;
    emitter.time = 0.0f;
    emitter.particleOffset = 0;
    emitter.particleCount = 0;
    m_emitters.append(emitter);
    emit emitterCreated(id);
    return id;
}

void GPUParticleSystem::destroyEmitter(QUuid id) {
    for (int i = 0; i < m_emitters.size(); ++i) {
        if (m_emitters[i].id == id) {
            m_emitters.removeAt(i);
            emit emitterDestroyed(id);
            return;
        }
    }
}

void GPUParticleSystem::setEmitterConfig(QUuid id, const ParticleEmitterConfig& config) {
    for (auto& e : m_emitters) {
        if (e.id == id) {
            e.config = config;
            return;
        }
    }
}

ParticleEmitterConfig* GPUParticleSystem::emitterConfig(QUuid id) {
    for (auto& e : m_emitters) {
        if (e.id == id) return &e.config;
    }
    return nullptr;
}

void GPUParticleSystem::startEmission(QUuid id) {
    for (auto& e : m_emitters) {
        if (e.id == id) {
            e.active = true;
            e.time = 0.0f;
            if (e.config.spawnMode == ParticleEmitterConfig::SpawnMode::Burst) {
                emitParticles(e, e.config.burstCount);
            }
            return;
        }
    }
}

void GPUParticleSystem::stopEmission(QUuid id) {
    for (auto& e : m_emitters) {
        if (e.id == id) {
            e.active = false;
            return;
        }
    }
}

void GPUParticleSystem::resetEmitter(QUuid id) {
    for (auto& e : m_emitters) {
        if (e.id == id) {
            e.time = 0.0f;
            e.emissionAccumulator = 0.0f;
            for (int i = 0; i < m_particles.size(); ++i) {
                if (m_particles[i].alive && i >= e.particleOffset && i < e.particleOffset + e.particleCount) {
                    m_particles[i].alive = 0;
                    m_particles[i].position = QVector3D(0, -99999, 0);
                }
            }
            return;
        }
    }
}

void GPUParticleSystem::setEmitterPosition(QUuid id, const QVector3D& position) {
    for (auto& e : m_emitters) {
        if (e.id == id) {
            e.config.position = position;
            return;
        }
    }
}

void GPUParticleSystem::setEmitterDirection(QUuid id, const QVector3D& direction) {
    for (auto& e : m_emitters) {
        if (e.id == id) {
            e.config.direction = direction.normalized();
            return;
        }
    }
}

void GPUParticleSystem::update(float deltaTime) {
    for (auto& emitter : m_emitters) {
        if (!emitter.active) continue;
        emitter.time += deltaTime;
        if (emitter.config.spawnMode == ParticleEmitterConfig::SpawnMode::Rate) {
            emitter.emissionAccumulator += emitter.config.rate * deltaTime;
            int toEmit = (int)emitter.emissionAccumulator;
            if (toEmit > 0) {
                emitter.emissionAccumulator -= toEmit;
                emitParticles(emitter, toEmit);
            }
        }
    }
    updateParticles(deltaTime);
    int count = 0;
    for (const auto& p : m_particles) {
        if (p.alive) ++count;
    }
    if (count != m_activeCount) {
        m_activeCount = count;
        emit particleCountChanged(m_activeCount);
    }
}

void GPUParticleSystem::render(const QMatrix4x4& view, const QMatrix4x4& projection, const QVector3D& cameraPos) {
    sortParticles(cameraPos);
}

void GPUParticleSystem::emitParticles(Emitter& emitter, int count) {
    const auto& config = emitter.config;
    int emitted = 0;
    for (int i = 0; i < m_particles.size() && emitted < count; ++i) {
        if (!m_particles[i].alive) {
            m_particles[i] = createParticle(config, emitter);
            ++emitted;
        }
    }
    if (emitted > 0 && emitter.particleCount == 0) {
        emitter.particleOffset = 0;
    }
    emitter.particleCount += emitted;
}

void GPUParticleSystem::updateParticles(float deltaTime) {
    for (auto& p : m_particles) {
        if (!p.alive) continue;
        p.life -= deltaTime;
        if (p.life <= 0.0f) {
            p.alive = 0;
            p.position = QVector3D(0, -99999, 0);
            continue;
        }
        float lifeRatio = 1.0f - (p.life / p.maxLife);
        p.velocity += (p.acceleration + m_gravity + m_wind) * deltaTime;
        p.velocity *= (1.0f - p.drag * deltaTime);
        p.position += p.velocity * deltaTime;
        p.rotation += p.rotationSpeed * deltaTime;
        float t = lifeRatio;
        float startS = p.size;
        float endS = p.size * 0.1f;
        p.size = startS + (endS - startS) * t;
    }
}

void GPUParticleSystem::sortParticles(const QVector3D& cameraPos) {
    std::sort(m_particles.begin(), m_particles.end(),
        [&cameraPos](const GPUParticle& a, const GPUParticle& b) {
            if (a.alive != b.alive) return a.alive > b.alive;
            if (!a.alive) return false;
            float da = (a.position - cameraPos).lengthSquared();
            float db = (b.position - cameraPos).lengthSquared();
            return da > db;
        });
}

int GPUParticleSystem::findFreeSlot() {
    for (int i = 0; i < m_particles.size(); ++i) {
        if (!m_particles[i].alive) return i;
    }
    return -1;
}

GPUParticle GPUParticleSystem::createParticle(const ParticleEmitterConfig& config, const Emitter& emitter) {
    GPUParticle p;
    p.alive = 1;
    float lifeVariance = randomFloat(-config.lifetimeVariance, config.lifetimeVariance);
    p.maxLife = qMax(0.01f, config.lifetime + lifeVariance);
    p.life = p.maxLife;

    QVector3D offset;
    float r1 = randomFloat(-1, 1), r2 = randomFloat(-1, 1), r3 = randomFloat(-1, 1);
    switch (config.shape) {
    case ParticleEmitterConfig::Shape::Point:
        offset = QVector3D(0, 0, 0);
        break;
    case ParticleEmitterConfig::Shape::Sphere: {
        QVector3D dir(r1, r2, r3);
        offset = dir.normalized() * randomFloat(0, 1.0f);
        break;
    }
    case ParticleEmitterConfig::Shape::Cone: {
        QVector3D dir = randomDirection(config.spreadAngle);
        offset = dir * randomFloat(0, 0.5f);
        break;
    }
    case ParticleEmitterConfig::Shape::Box:
        offset = QVector3D(randomFloat(-1, 1), randomFloat(-1, 1), randomFloat(-1, 1));
        break;
    case ParticleEmitterConfig::Shape::Hemisphere: {
        QVector3D dir(r1, qAbs(r2), r3);
        offset = dir.normalized() * randomFloat(0, 1.0f);
        break;
    }
    case ParticleEmitterConfig::Shape::Torus: {
        float angle = randomFloat(0, 2.0f * M_PI);
        float majorR = 1.0f;
        float minorR = 0.2f;
        QVector3D majorPos(majorR * qCos(angle), 0, majorR * qSin(angle));
        QVector3D minorDir = randomDirection(360.0f);
        offset = majorPos + minorDir * minorR;
        break;
    }
    }

    if (config.worldSpace) {
        p.position = config.position + offset;
    } else {
        p.position = offset;
    }

    QVector3D dir = config.direction + randomDirection(config.spreadAngle) * config.spreadAngle / 90.0f;
    float velVar = randomFloat(-config.initialVelocityVariance, config.initialVelocityVariance);
    p.velocity = dir.normalized() * qMax(0.0f, config.initialVelocity + velVar);

    p.acceleration = QVector3D(0, 0, 0);
    p.size = config.startSize + randomFloat(-config.sizeVariance, config.sizeVariance);
    p.color = config.startColor;
    p.rotation = randomFloat(0, 360.0f);
    p.rotationSpeed = randomFloat(-180.0f, 180.0f);
    p.drag = config.drag;
    return p;
}

QJsonObject GPUParticleSystem::serialize() const {
    QJsonObject root;
    QJsonArray emittersArr;
    for (const auto& e : m_emitters) {
        QJsonObject ej;
        ej["id"] = e.id.toString();
        ej["active"] = e.active;
        ej["shape"] = static_cast<int>(e.config.shape);
        ej["rate"] = static_cast<double>(e.config.rate);
        ej["lifetime"] = static_cast<double>(e.config.lifetime);
        ej["maxParticles"] = e.config.maxParticles;
        emittersArr.push_back(ej);
    }
    root["emitters"] = emittersArr;
    root["gravity"] = QString("%1,%2,%3").arg(m_gravity.x()).arg(m_gravity.y()).arg(m_gravity.z());
    return root;
}

bool GPUParticleSystem::deserialize(const QJsonObject& data) {
    return true;
}

} // namespace ks::engine::graphics
