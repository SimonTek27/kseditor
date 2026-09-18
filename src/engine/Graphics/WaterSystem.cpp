#include "WaterSystem.h"
#include <QDebug>
#include <QtMath>
#include <algorithm>

namespace ks::engine::graphics {

bool WaterSystem::initialize() {
    if (m_initialized) return true;
    buildTileMesh(m_config.meshResolution);
    m_initialized = true;
    qInfo() << "WaterSystem: initialized";
    return true;
}

void WaterSystem::shutdown() {
    if (!m_initialized) return;
    m_tiles.clear();
    m_waveLayers.clear();
    m_tileVertices.clear();
    m_tileNormals.clear();
    m_tileUVs.clear();
    m_tileIndices.clear();
    m_meshBuilt = false;
    m_initialized = false;
    qInfo() << "WaterSystem: shutdown";
}

void WaterSystem::configure(const WaterConfig& config) {
    m_config = config;
    buildTileMesh(m_config.meshResolution);
    emit waterConfigChanged();
}

void WaterSystem::addWaveLayer(const WaveLayer& wave) {
    m_waveLayers.append(wave);
    emit waveAdded(m_waveLayers.size() - 1);
}

void WaterSystem::removeWaveLayer(int index) {
    if (index >= 0 && index < m_waveLayers.size()) {
        m_waveLayers.removeAt(index);
        emit waveRemoved(index);
    }
}

void WaterSystem::clearWaveLayers() {
    m_waveLayers.clear();
}

float WaterSystem::getWaterHeight(float x, float z, float time) const {
    float height = m_config.seaLevel;
    for (const auto& wave : m_waveLayers) {
        QVector2D dir = wave.direction.normalized();
        float k = 2.0f * M_PI / wave.wavelength;
        float omega = wave.speed * k;
        float phase = k * (dir.x() * x + dir.y() * z) - omega * time * m_config.timeSpeed;
        float sharpness = wave.steepness * wave.amplitude;
        height += sharpness * qCos(phase);
    }
    return height;
}

QVector3D WaterSystem::getWaterNormal(float x, float z, float time) const {
    float delta = 0.1f;
    float hL = getWaterHeight(x - delta, z, time);
    float hR = getWaterHeight(x + delta, z, time);
    float hD = getWaterHeight(x, z - delta, time);
    float hU = getWaterHeight(x, z + delta, time);
    QVector3D n(hL - hR, 2.0f * delta, hD - hU);
    return n.normalized();
}

QVector3D WaterSystem::getWaterVelocity(float x, float z, float time) const {
    QVector3D velocity(0, 0, 0);
    float delta = 0.01f;
    float h0 = getWaterHeight(x, z, time);
    float hX = getWaterHeight(x + delta, z, time);
    float hZ = getWaterHeight(x, z + delta, time);
    velocity.setX((hX - h0) / delta);
    velocity.setZ((hZ - h0) / delta);
    velocity.setY(0);
    return velocity;
}

bool WaterSystem::isUnderwater(const QVector3D& position) const {
    float waterH = getWaterHeight(position.x(), position.z(), m_time);
    return position.y() < waterH;
}

float WaterSystem::getDepth(const QVector3D& position) const {
    float waterH = getWaterHeight(position.x(), position.z(), m_time);
    return qMax(0.0f, waterH - position.y());
}

bool WaterSystem::isShoreline(const QVector3D& position, float time, float threshold) const {
    float waterH = getWaterHeight(position.x(), position.z(), time);
    float diff = qAbs(position.y() - waterH);
    return diff < threshold;
}

float WaterSystem::getFoamIntensity(float x, float z, float time) const {
    if (!m_config.enableFoam) return 0.0f;
    float foam = 0.0f;
    for (const auto& wave : m_waveLayers) {
        QVector2D dir = wave.direction.normalized();
        float k = 2.0f * M_PI / wave.wavelength;
        float omega = wave.speed * k;
        float phase = k * (dir.x() * x + dir.y() * z) - omega * time * m_config.timeSpeed;
        float waveCrest = qCos(phase);
        if (waveCrest > m_config.foamThreshold) {
            foam += (waveCrest - m_config.foamThreshold) / (1.0f - m_config.foamThreshold);
        }
    }
    return qBound(0.0f, foam * m_config.foamIntensity, 1.0f);
}

QVector3D WaterSystem::getFoamUV(float x, float z, float time) const {
    float u = x * m_config.texScale + time * 0.1f;
    float v = z * m_config.texScale + time * 0.05f;
    float w = time * 0.2f;
    return QVector3D(u, v, w);
}

void WaterSystem::update(float deltaTime, const QVector3D& cameraPosition) {
    m_time += deltaTime * m_config.timeSpeed;
    float halfWorld = m_config.tileSize * m_config.tileCount * 0.5f;
    for (auto& tile : m_tiles) {
        float tileCenterX = tile.tileX * m_config.tileSize - halfWorld + m_config.tileSize * 0.5f;
        float tileCenterZ = tile.tileZ * m_config.tileSize - halfWorld + m_config.tileSize * 0.5f;
        QVector3D tileCenter(tileCenterX, m_config.seaLevel, tileCenterZ);
        tile.distanceToCamera = (cameraPosition - tileCenter).length();
        float lodDist = m_config.tileSize * 2.0f;
        tile.lodLevel = qBound(0, (int)(tile.distanceToCamera / lodDist), 3);
        tile.visible = tile.distanceToCamera < m_config.tileSize * (m_config.tileCount + 2);
    }
}

void WaterSystem::getVisibleTiles(const QVector3D& cameraPos, QVector<WaterTile>& visibleTiles) const {
    visibleTiles.clear();
    for (const auto& tile : m_tiles) {
        if (tile.visible) visibleTiles.append(tile);
    }
    std::sort(visibleTiles.begin(), visibleTiles.end(),
        [](const WaterTile& a, const WaterTile& b) { return a.distanceToCamera > b.distanceToCamera; });
}

VkPipeline WaterSystem::createWaterPipeline(VkDevice device, VkRenderPass renderPass, VkPipelineLayout layout) {
    return VK_NULL_HANDLE;
}

void WaterSystem::updateWaterUniforms(VkCommandBuffer cmd, const QMatrix4x4& view, const QMatrix4x4& proj,
                                       const QVector3D& cameraPos, const QVector3D& sunDir, const QVector3D& sunColor) {
}

void WaterSystem::buildTileMesh(int resolution) {
    m_tileVertices.clear();
    m_tileNormals.clear();
    m_tileUVs.clear();
    m_tileIndices.clear();
    int vertsPerSide = resolution + 1;
    m_tileVertices.reserve(vertsPerSide * vertsPerSide);
    m_tileNormals.reserve(vertsPerSide * vertsPerSide);
    m_tileUVs.reserve(vertsPerSide * vertsPerSide);
    for (int z = 0; z < vertsPerSide; ++z) {
        for (int x = 0; x < vertsPerSide; ++x) {
            float u = (float)x / resolution;
            float v = (float)z / resolution;
            m_tileVertices.append(QVector3D(u * m_config.tileSize, 0, v * m_config.tileSize));
            m_tileNormals.append(QVector3D(0, 1, 0));
            m_tileUVs.append(QVector2D(u, v));
        }
    }
    for (int z = 0; z < resolution; ++z) {
        for (int x = 0; x < resolution; ++x) {
            int i00 = z * vertsPerSide + x;
            int i10 = i00 + 1;
            int i01 = i00 + vertsPerSide;
            int i11 = i01 + 1;
            m_tileIndices.append(i00);
            m_tileIndices.append(i01);
            m_tileIndices.append(i10);
            m_tileIndices.append(i10);
            m_tileIndices.append(i01);
            m_tileIndices.append(i11);
        }
    }
    m_meshBuilt = true;
    m_tiles.clear();
    for (int z = 0; z < m_config.tileCount; ++z) {
        for (int x = 0; x < m_config.tileCount; ++x) {
            WaterTile tile;
            tile.tileX = x;
            tile.tileZ = z;
            tile.visible = true;
            tile.lodLevel = 0;
            m_tiles.append(tile);
        }
    }
}

void WaterSystem::buildAllTiles() {
    buildTileMesh(m_config.meshResolution);
}

} // namespace ks::engine::graphics
