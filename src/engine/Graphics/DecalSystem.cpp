#include "DecalSystem.h"
#include <QDebug>
#include <QtMath>
#include <QQuaternion>
#include <algorithm>

namespace ks::engine::graphics {

bool DecalSystem::initialize() {
    if (m_initialized) return true;
    m_quadVertices = {
        {-0.5f, -0.5f, 0}, {0.5f, -0.5f, 0}, {0.5f, 0.5f, 0}, {-0.5f, 0.5f, 0}
    };
    m_quadIndices = {0, 1, 2, 0, 2, 3};
    m_initialized = true;
    qInfo() << "DecalSystem: initialized";
    return true;
}

void DecalSystem::shutdown() {
    if (!m_initialized) return;
    m_allDecals.clear();
    m_buckets.clear();
    m_initialized = false;
    qInfo() << "DecalSystem: shutdown";
}

void DecalSystem::configure(const DecalConfig& config) {
    m_config = config;
}

QUuid DecalSystem::addDecal(const Decal& decal) {
    if (m_allDecals.size() >= m_config.maxDecals) return QUuid();
    Decal d = decal;
    if (d.id.isNull()) d.id = QUuid::createUuid();
    QMatrix4x4 t;
    t.translate(d.position);
    t.rotate(QQuaternion::fromEulerAngles(d.rotation));
    t.scale(d.size);
    d.transform = t;
    d.inverseTransform = t.inverted();
    m_allDecals.append(d);
    int bi = bucketIndex(d);
    if (bi >= 0 && bi < m_buckets.size()) {
        m_buckets[bi].decals.append(d);
        m_buckets[bi].dirty = true;
    }
    emit decalAdded(d.id);
    return d.id;
}

void DecalSystem::removeDecal(QUuid id) {
    for (int i = 0; i < m_allDecals.size(); ++i) {
        if (m_allDecals[i].id == id) {
            m_allDecals.removeAt(i);
            for (auto& bucket : m_buckets) {
                for (int j = 0; j < bucket.decals.size(); ++j) {
                    if (bucket.decals[j].id == id) {
                        bucket.decals.removeAt(j);
                        bucket.dirty = true;
                        break;
                    }
                }
            }
            emit decalRemoved(id);
            return;
        }
    }
}

void DecalSystem::updateDecal(QUuid id, const Decal& decal) {
    for (auto& d : m_allDecals) {
        if (d.id == id) {
            d = decal;
            d.id = id;
            QMatrix4x4 t;
            t.translate(d.position);
            t.rotate(QQuaternion::fromEulerAngles(d.rotation));
            t.scale(d.size);
            d.transform = t;
            d.inverseTransform = t.inverted();
            emit decalChanged(id);
            return;
        }
    }
}

Decal* DecalSystem::getDecal(QUuid id) {
    for (auto& d : m_allDecals) {
        if (d.id == id) return &d;
    }
    return nullptr;
}

const Decal* DecalSystem::getDecal(QUuid id) const {
    for (const auto& d : m_allDecals) {
        if (d.id == id) return &d;
    }
    return nullptr;
}

void DecalSystem::clearDecals() {
    m_allDecals.clear();
    for (auto& bucket : m_buckets) {
        bucket.decals.clear();
        bucket.dirty = true;
    }
    emit decalsCleared();
}

void DecalSystem::setDecalPosition(QUuid id, const QVector3D& position) {
    for (auto& d : m_allDecals) {
        if (d.id == id) {
            d.position = position;
            QMatrix4x4 t;
            t.translate(position);
            t.rotate(QQuaternion::fromEulerAngles(d.rotation));
            t.scale(d.size);
            d.transform = t;
            d.inverseTransform = t.inverted();
            emit decalChanged(id);
            return;
        }
    }
}

void DecalSystem::setDecalOpacity(QUuid id, float opacity) {
    for (auto& d : m_allDecals) {
        if (d.id == id) {
            d.opacity = qBound(0.0f, opacity, 1.0f);
            emit decalChanged(id);
            return;
        }
    }
}

void DecalSystem::setDecalVisible(QUuid id, bool visible) {
    for (auto& d : m_allDecals) {
        if (d.id == id) {
            d.visible = visible;
            emit decalChanged(id);
            return;
        }
    }
}

QVector<Decal> DecalSystem::getDecalsInRadius(const QVector3D& center, float radius) const {
    QVector<Decal> result;
    float r2 = radius * radius;
    for (const auto& d : m_allDecals) {
        if ((d.position - center).lengthSquared() < r2) result.append(d);
    }
    return result;
}

QVector<Decal> DecalSystem::getDecalsOnObject(const QString& objectName) const {
    QVector<Decal> result;
    for (const auto& d : m_allDecals) {
        if (d.name == objectName) result.append(d);
    }
    return result;
}

void DecalSystem::addWearDecal(const QVector3D& position, const QVector3D& normal, float amount) {
    Decal d;
    d.wearDecal = true;
    d.wearAmount = amount;
    d.position = position;
    d.projectionDepth = 0.1f;
    d.size = QVector3D(0.5f, 0.5f, 0.1f);
    d.opacity = amount;
    addDecal(d);
}

void DecalSystem::addDamageDecal(const QVector3D& position, const QVector3D& normal, float amount) {
    Decal d;
    d.damageDecal = true;
    d.damageAmount = amount;
    d.position = position;
    d.projectionDepth = 0.15f;
    d.size = QVector3D(0.8f, 0.8f, 0.15f);
    d.opacity = amount;
    d.colorTint = QVector4D(0.3f, 0.2f, 0.1f, 1.0f);
    addDecal(d);
}

void DecalSystem::addDirtDecal(const QVector3D& position, const QVector3D& normal, float amount) {
    Decal d;
    d.dirtDecal = true;
    d.dirtAmount = amount;
    d.position = position;
    d.projectionDepth = 0.1f;
    d.size = QVector3D(1.0f, 1.0f, 0.1f);
    d.opacity = amount * 0.5f;
    d.colorTint = QVector4D(0.4f, 0.35f, 0.25f, 1.0f);
    addDecal(d);
}

void DecalSystem::addPaintDecal(const QVector3D& position, const QVector3D& normal, const QVector4D& color) {
    Decal d;
    d.paintDecal = true;
    d.paintColor = color;
    d.position = position;
    d.projectionDepth = 0.05f;
    d.size = QVector3D(0.3f, 0.3f, 0.05f);
    d.colorTint = color;
    addDecal(d);
}

void DecalSystem::update(float deltaTime, const QVector3D& cameraPosition) {
}

bool DecalSystem::createResources(VkDevice device, VkPhysicalDevice physDev) {
    return true;
}

void DecalSystem::destroyResources(VkDevice device) {
}

void DecalSystem::renderDecals(VkCommandBuffer cmd) {
}

void DecalSystem::updateDecalBuffer(int bucketIndex) {
}

void DecalSystem::buildDecalGeometry(const Decal& decal, QVector<QVector3D>& vertices, QVector<quint32>& indices) {
    vertices.clear();
    indices.clear();
    QVector3D corners[8] = {
        {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
        {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1}
    };
    for (int i = 0; i < 8; ++i) {
        QVector4D c = decal.transform * QVector4D(corners[i] * 0.5f, 1.0f);
        vertices.append(QVector3D(c.x() / c.w(), c.y() / c.w(), c.z() / c.w()));
    }
    int faces[6][4] = {
        {0, 1, 2, 3}, {4, 5, 6, 7}, {0, 4, 7, 3},
        {1, 5, 6, 2}, {0, 1, 5, 4}, {3, 2, 6, 7}
    };
    for (int f = 0; f < 6; ++f) {
        int base = vertices.size();
        indices.append(base);
        indices.append(base + 1);
        indices.append(base + 2);
        indices.append(base);
        indices.append(base + 2);
        indices.append(base + 3);
    }
}

void DecalSystem::updateAllBuckets() {
}

int DecalSystem::bucketIndex(const Decal& decal) const {
    return 0;
}

} // namespace ks::engine::graphics
