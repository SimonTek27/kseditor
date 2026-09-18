#include "VegetationSystem.h"
#include <QDebug>
#include <QtMath>
#include <algorithm>
#include <random>

namespace ks::engine::graphics {

static std::mt19937 s_vegRng(std::random_device{}());

bool VegetationSystem::initialize() {
    if (m_initialized) return true;
    m_lodLevels = {
        {50.0f, 1.0f, false},
        {100.0f, 0.75f, false},
        {200.0f, 0.5f, true},
        {500.0f, 0.25f, true}
    };
    m_initialized = true;
    qInfo() << "VegetationSystem: initialized";
    return true;
}

void VegetationSystem::shutdown() {
    if (!m_initialized) return;
    m_types.clear();
    m_buckets.clear();
    m_allInstances.clear();
    m_initialized = false;
    qInfo() << "VegetationSystem: shutdown";
}

void VegetationSystem::configure(const VegetationConfig& config) {
    m_config = config;
}

int VegetationSystem::addVegetationType(const VegetationType& type) {
    int idx = m_types.size();
    m_types.append(type);
    VegetationBucket bucket;
    bucket.typeIndex = idx;
    m_buckets.append(bucket);
    return idx;
}

void VegetationSystem::removeVegetationType(int index) {
    if (index >= 0 && index < m_types.size()) {
        m_types.removeAt(index);
        if (index < m_buckets.size()) m_buckets.removeAt(index);
        for (auto& inst : m_allInstances) {
            int tid = inst.typeId.data1;
            if (tid == index) inst.visible = false;
            else if (tid > index) inst.typeId = QUuid(tid - 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        }
    }
}

QUuid VegetationSystem::addInstance(int typeIndex, const QVector3D& position,
                                     const QVector3D& rotation, const QVector3D& scale) {
    if (typeIndex < 0 || typeIndex >= m_types.size()) return QUuid();
    if (m_allInstances.size() >= m_config.maxTotalInstances) return QUuid();
    VegetationInstance inst;
    inst.typeId = QUuid::createUuid();
    inst.position = position;
    inst.rotation = rotation;
    inst.scale = scale;
    inst.randomSeed = std::uniform_real_distribution<float>(0.0f, 1.0f)(s_vegRng);
    inst.visible = true;
    m_allInstances.append(inst);
    if (typeIndex < m_buckets.size()) {
        m_buckets[typeIndex].instances.append(inst);
        m_buckets[typeIndex].dirty = true;
    }
    emit instanceAdded(inst.typeId, typeIndex);
    return inst.typeId;
}

void VegetationSystem::removeInstance(QUuid instanceId) {
    for (int i = 0; i < m_allInstances.size(); ++i) {
        if (m_allInstances[i].typeId == instanceId) {
            int typeIdx = m_allInstances[i].typeId.data1;
            m_allInstances.removeAt(i);
            if (typeIdx >= 0 && typeIdx < m_buckets.size()) {
                auto& bucket = m_buckets[typeIdx].instances;
                for (int j = 0; j < bucket.size(); ++j) {
                    if (bucket[j].typeId == instanceId) {
                        bucket.removeAt(j);
                        m_buckets[typeIdx].dirty = true;
                        break;
                    }
                }
            }
            emit instanceRemoved(instanceId);
            return;
        }
    }
}

void VegetationSystem::clearInstances() {
    m_allInstances.clear();
    for (auto& bucket : m_buckets) {
        bucket.instances.clear();
        bucket.dirty = true;
    }
    emit instancesChanged();
}

void VegetationSystem::clearInstancesOfType(int typeIndex) {
    if (typeIndex < 0 || typeIndex >= m_buckets.size()) return;
    QUuid id = m_buckets[typeIndex].instances.isEmpty() ? QUuid() : m_buckets[typeIndex].instances[0].typeId;
    m_buckets[typeIndex].instances.clear();
    m_buckets[typeIndex].dirty = true;
    for (int i = m_allInstances.size() - 1; i >= 0; --i) {
        if (m_allInstances[i].typeId.data1 == typeIndex) {
            m_allInstances.removeAt(i);
        }
    }
    emit instancesChanged();
}

void VegetationSystem::addInstancesBatch(int typeIndex, const QVector<VegetationInstance>& instances) {
    if (typeIndex < 0 || typeIndex >= m_buckets.size()) return;
    for (const auto& inst : instances) {
        if (m_allInstances.size() >= m_config.maxTotalInstances) break;
        m_allInstances.append(inst);
        m_buckets[typeIndex].instances.append(inst);
    }
    m_buckets[typeIndex].dirty = true;
    emit instancesChanged();
}

int VegetationSystem::instanceCount(int typeIndex) const {
    if (typeIndex >= 0 && typeIndex < m_buckets.size())
        return m_buckets[typeIndex].instances.size();
    return 0;
}

int VegetationSystem::totalInstanceCount() const {
    return m_allInstances.size();
}

void VegetationSystem::setInstancePosition(QUuid instanceId, const QVector3D& position) {
    for (auto& inst : m_allInstances) {
        if (inst.typeId == instanceId) {
            inst.position = position;
            int typeIdx = inst.typeId.data1;
            if (typeIdx >= 0 && typeIdx < m_buckets.size()) {
                for (auto& bi : m_buckets[typeIdx].instances) {
                    if (bi.typeId == instanceId) {
                        bi.position = position;
                        m_buckets[typeIdx].dirty = true;
                        break;
                    }
                }
            }
            return;
        }
    }
}

void VegetationSystem::setInstanceVisible(QUuid instanceId, bool visible) {
    for (auto& inst : m_allInstances) {
        if (inst.typeId == instanceId) {
            inst.visible = visible;
            return;
        }
    }
}

void VegetationSystem::update(float deltaTime, const QVector3D& cameraPosition, const QVector3D& windDir) {
    m_windPhase += deltaTime * m_config.globalWindStrength;
    if (m_windPhase > 2.0f * M_PI) m_windPhase -= 2.0f * M_PI;
    m_windDirection = windDir.normalized();
    updateLODs(cameraPosition);
    emit windChanged();
}

void VegetationSystem::fillArea(int typeIndex, const QVector3D& center, const QVector3D& size,
                                  float density, const std::function<float(const QVector3D&)>& heightFunc) {
    if (typeIndex < 0 || typeIndex >= m_types.size()) return;
    float area = size.x() * size.z();
    int count = (int)(area * density);
    count = qMin(count, m_config.maxInstancesPerType - instanceCount(typeIndex));
    std::uniform_real_distribution<float> distX(-size.x() * 0.5f, size.x() * 0.5f);
    std::uniform_real_distribution<float> distZ(-size.z() * 0.5f, size.z() * 0.5f);
    std::uniform_real_distribution<float> distRot(0.0f, 360.0f);
    std::uniform_real_distribution<float> distScale(0.8f, 1.2f);
    for (int i = 0; i < count; ++i) {
        float x = center.x() + distX(s_vegRng);
        float z = center.z() + distZ(s_vegRng);
        float y = heightFunc ? heightFunc(QVector3D(x, 0, z)) : center.y();
        QVector3D pos(x, y, z);
        QVector3D rot(0, distRot(s_vegRng), 0);
        float s = distScale(s_vegRng);
        QVector3D scale(s, s, s);
        addInstance(typeIndex, pos, rot, scale);
    }
}

void VegetationSystem::removeInArea(const QVector3D& center, const QVector3D& size) {
    QVector3D halfSize = size * 0.5f;
    for (int i = m_allInstances.size() - 1; i >= 0; --i) {
        const auto& inst = m_allInstances[i];
        if (qAbs(inst.position.x() - center.x()) < halfSize.x() &&
            qAbs(inst.position.z() - center.z()) < halfSize.z()) {
            removeInstance(inst.typeId);
        }
    }
}

QVector<VegetationInstance> VegetationSystem::getVisibleInstances() const {
    QVector<VegetationInstance> result;
    for (const auto& inst : m_allInstances) {
        if (inst.visible) result.append(inst);
    }
    return result;
}

QVector<VegetationInstance> VegetationSystem::getInstancesInRadius(const QVector3D& center, float radius) const {
    QVector<VegetationInstance> result;
    float r2 = radius * radius;
    for (const auto& inst : m_allInstances) {
        float d2 = (inst.position - center).lengthSquared();
        if (d2 < r2) result.append(inst);
    }
    return result;
}

void VegetationSystem::updateLODs(const QVector3D& cameraPosition) {
}

void VegetationSystem::buildRenderBuckets() {
    for (auto& bucket : m_buckets) {
        if (!bucket.dirty) continue;
        bucket.dirty = false;
    }
}

int VegetationSystem::findNearestType(const QVector3D& position) const {
    return -1;
}

} // namespace ks::engine::graphics
