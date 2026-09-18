#include "TerrainSystem.h"
#include <QDebug>
#include <QtMath>
#include <QFile>
#include <QDataStream>
#include <limits>

namespace ks::engine::graphics {

bool TerrainSystem::initialize() {
    if (m_initialized) return true;
    m_initialized = true;
    qInfo() << "TerrainSystem: initialized";
    return true;
}

void TerrainSystem::shutdown() {
    if (!m_initialized) return;
    m_heightmap.clear();
    m_chunks.clear();
    m_holes.clear();
    m_normals.clear();
    m_uvs.clear();
    m_layers.clear();
    m_splatmap = QImage();
    m_heightmapLoaded = false;
    m_initialized = false;
    qInfo() << "TerrainSystem: shutdown";
}

void TerrainSystem::configure(const TerrainConfig& config) {
    m_config = config;
    int totalChunks = (m_config.heightmapSize - 1) / m_config.chunkSize;
    if (totalChunks <= 0) totalChunks = 1;
    int n = m_config.heightmapSize * m_config.heightmapSize;
    m_heightmap.fill(0.0f, n);
    m_normals.fill(QVector3D(0, 1, 0), n);
    m_uvs.fill(QVector2D(0, 0), n);
    int chunksPerSide = totalChunks;
    m_chunks.resize(chunksPerSide * chunksPerSide);
    m_holes.fill(false, chunksPerSide * chunksPerSide);
    for (int z = 0; z < chunksPerSide; ++z) {
        for (int x = 0; x < chunksPerSide; ++x) {
            int idx = z * chunksPerSide + x;
            m_chunks[idx].gridX = x;
            m_chunks[idx].gridZ = z;
            m_chunks[idx].lod = 0;
            m_chunks[idx].isDirty = true;
        }
    }
    generateUVs();
}

bool TerrainSystem::loadHeightmap(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "TerrainSystem: cannot open heightmap file" << filePath;
        return false;
    }
    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    int width = 0, height = 0;
    stream >> width >> height;
    if (width <= 0 || height <= 0 || width > 8192 || height > 8192) {
        qWarning() << "TerrainSystem: invalid heightmap dimensions" << width << height;
        return false;
    }
    m_config.heightmapSize = qMax(width, height);
    int n = width * height;
    m_heightmap.resize(n);
    for (int i = 0; i < n; ++i) {
        float h;
        stream >> h;
        m_heightmap[i] = h * m_config.maxHeight;
    }
    m_heightmapLoaded = true;
    buildAllChunks();
    generateNormals();
    emit terrainLoaded();
    qInfo() << "TerrainSystem: loaded heightmap" << width << "x" << height;
    return true;
}

bool TerrainSystem::loadHeightmapFromData(const float* data, int width, int height) {
    if (!data || width <= 0 || height <= 0) return false;
    m_config.heightmapSize = qMax(width, height);
    int n = width * height;
    m_heightmap.resize(n);
    for (int i = 0; i < n; ++i) {
        m_heightmap[i] = data[i] * m_config.maxHeight;
    }
    m_heightmapLoaded = true;
    buildAllChunks();
    generateNormals();
    emit terrainLoaded();
    return true;
}

bool TerrainSystem::saveHeightmap(const QString& filePath) const {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) return false;
    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    int size = m_config.heightmapSize;
    stream << size << size;
    for (float h : m_heightmap) {
        stream << (h / m_config.maxHeight);
    }
    return true;
}

bool TerrainSystem::loadSplatmap(const QString& filePath) {
    QImage img(filePath);
    if (img.isNull()) {
        qWarning() << "TerrainSystem: cannot load splatmap" << filePath;
        return false;
    }
    m_splatmap = img.convertToFormat(QImage::Format_RGBA8888);
    emit terrainModified();
    return true;
}

bool TerrainSystem::loadSplatmapFromData(const uchar* data, int width, int height, int channels) {
    if (!data || width <= 0 || height <= 0) return false;
    QImage::Format fmt = (channels == 4) ? QImage::Format_RGBA8888 : QImage::Format_RGB888;
    m_splatmap = QImage(data, width, height, width * channels, fmt).copy();
    emit terrainModified();
    return true;
}

void TerrainSystem::setHeight(int x, int z, float height) {
    if (x < 0 || x >= m_config.heightmapSize || z < 0 || z >= m_config.heightmapSize) return;
    int idx = z * m_config.heightmapSize + x;
    m_heightmap[idx] = qBound(0.0f, height, m_config.maxHeight);
    int chunkSize = m_config.chunkSize;
    int cx = x / chunkSize;
    int cz = z / chunkSize;
    int chunksPerSide = (m_config.heightmapSize - 1) / chunkSize;
    if (cx >= 0 && cx < chunksPerSide && cz >= 0 && cz < chunksPerSide) {
        int ci = cz * chunksPerSide + cx;
        if (ci < m_chunks.size()) m_chunks[ci].isDirty = true;
    }
    generateNormals();
    emit heightChanged(x, z, m_heightmap[idx]);
    emit terrainModified();
}

float TerrainSystem::getHeight(int x, int z) const {
    if (x < 0 || x >= m_config.heightmapSize || z < 0 || z >= m_config.heightmapSize) return 0.0f;
    return m_heightmap[z * m_config.heightmapSize + x];
}

float TerrainSystem::getHeightBilinear(float x, float z) const {
    float fx = x * (m_config.heightmapSize - 1) / m_config.worldSize + m_config.heightmapSize * 0.5f;
    float fz = z * (m_config.heightmapSize - 1) / m_config.worldSize + m_config.heightmapSize * 0.5f;
    int ix = qBound(0, (int)fx, m_config.heightmapSize - 2);
    int iz = qBound(0, (int)fz, m_config.heightmapSize - 2);
    float fracX = fx - ix;
    float fracZ = fz - iz;
    float h00 = getHeight(ix, iz);
    float h10 = getHeight(ix + 1, iz);
    float h01 = getHeight(ix, iz + 1);
    float h11 = getHeight(ix + 1, iz + 1);
    float h0 = h00 * (1.0f - fracX) + h10 * fracX;
    float h1 = h01 * (1.0f - fracX) + h11 * fracX;
    return h0 * (1.0f - fracZ) + h1 * fracZ;
}

QVector3D TerrainSystem::getNormal(int x, int z) const {
    if (x < 0 || x >= m_config.heightmapSize || z < 0 || z >= m_config.heightmapSize)
        return QVector3D(0, 1, 0);
    return m_normals[z * m_config.heightmapSize + x];
}

QVector3D TerrainSystem::getNormalBilinear(float x, float z) const {
    float fx = x * (m_config.heightmapSize - 1) / m_config.worldSize + m_config.heightmapSize * 0.5f;
    float fz = z * (m_config.heightmapSize - 1) / m_config.worldSize + m_config.heightmapSize * 0.5f;
    int ix = qBound(0, (int)fx, m_config.heightmapSize - 2);
    int iz = qBound(0, (int)fz, m_config.heightmapSize - 2);
    float fracX = fx - ix;
    float fracZ = fz - iz;
    QVector3D n00 = getNormal(ix, iz);
    QVector3D n10 = getNormal(ix + 1, iz);
    QVector3D n01 = getNormal(ix, iz + 1);
    QVector3D n11 = getNormal(ix + 1, iz + 1);
    QVector3D n0 = n00 * (1.0f - fracX) + n10 * fracX;
    QVector3D n1 = n01 * (1.0f - fracX) + n11 * fracX;
    QVector3D result = n0 * (1.0f - fracZ) + n1 * fracZ;
    return result.normalized();
}

TerrainRaycastResult TerrainSystem::raycast(const QVector3D& origin, const QVector3D& direction, float maxDistance) const {
    TerrainRaycastResult result;
    if (!m_heightmapLoaded) return result;
    QVector3D dir = direction.normalized();
    float step = m_config.worldSize / m_config.heightmapSize;
    for (float t = 0; t < maxDistance; t += step) {
        QVector3D p = origin + dir * t;
        float terrainH = getHeightBilinear(p.x(), p.z());
        if (p.y() <= terrainH) {
            result.hit = true;
            result.position = p;
            result.height = terrainH;
            result.normal = getNormalBilinear(p.x(), p.z());
            float fx = p.x() * (m_config.heightmapSize - 1) / m_config.worldSize + m_config.heightmapSize * 0.5f;
            float fz = p.z() * (m_config.heightmapSize - 1) / m_config.worldSize + m_config.heightmapSize * 0.5f;
            result.chunkX = (int)fx / m_config.chunkSize;
            result.chunkZ = (int)fz / m_config.chunkSize;
            return result;
        }
    }
    return result;
}

void TerrainSystem::setHole(int chunkX, int chunkZ, bool isHole) {
    int idx = chunkIndex(chunkX, chunkZ);
    if (idx >= 0 && idx < m_holes.size()) {
        m_holes[idx] = isHole;
        if (idx < m_chunks.size()) m_chunks[idx].isHole = isHole;
        emit terrainModified();
    }
}

bool TerrainSystem::isHole(int chunkX, int chunkZ) const {
    int idx = chunkIndex(chunkX, chunkZ);
    return idx >= 0 && idx < m_holes.size() ? m_holes[idx] : false;
}

void TerrainSystem::addLayer(const TerrainLayer& layer) {
    if (m_layers.size() < m_config.splatmapLayers) {
        m_layers.append(layer);
        emit terrainModified();
    }
}

void TerrainSystem::removeLayer(int index) {
    if (index >= 0 && index < m_layers.size()) {
        m_layers.removeAt(index);
        emit terrainModified();
    }
}

void TerrainSystem::setSplatmapPixel(int x, int z, const QVector<float>& weights) {
    if (m_splatmap.isNull()) {
        int size = m_config.heightmapSize;
        m_splatmap = QImage(size, size, QImage::Format_RGBA8888);
        m_splatmap.fill(Qt::black);
    }
    if (x < 0 || x >= m_splatmap.width() || z < 0 || z >= m_splatmap.height()) return;
    int r = 0, g = 0, b = 0, a = 255;
    if (weights.size() > 0) r = qBound(0, (int)(weights[0] * 255), 255);
    if (weights.size() > 1) g = qBound(0, (int)(weights[1] * 255), 255);
    if (weights.size() > 2) b = qBound(0, (int)(weights[2] * 255), 255);
    m_splatmap.setPixelColor(x, z, QColor(r, g, b, a));
    emit terrainModified();
}

QVector<float> TerrainSystem::getSplatmapPixel(int x, int z) const {
    if (m_splatmap.isNull() || x < 0 || x >= m_splatmap.width() || z < 0 || z >= m_splatmap.height())
        return {1.0f, 0, 0, 0, 0, 0, 0, 0};
    QColor c = m_splatmap.pixelColor(x, z);
    return {c.redF(), c.greenF(), c.blueF(), 0, 0, 0, 0, 0};
}

QVector3D TerrainSystem::getTerrainSize() const {
    return QVector3D(m_config.worldSize, m_config.maxHeight, m_config.worldSize);
}

QVector3D TerrainSystem::getTerrainCenter() const {
    return QVector3D(0, m_config.maxHeight * 0.5f, 0);
}

float TerrainSystem::getTerrainMinHeight() const { return m_minHeight; }
float TerrainSystem::getTerrainMaxHeight() const { return m_maxHeight; }

void TerrainSystem::updateLODs(const QVector3D& cameraPosition) {
    float halfWorld = m_config.worldSize * 0.5f;
    float chunkWorldSize = m_config.worldSize / ((m_config.heightmapSize - 1) / m_config.chunkSize);
    for (auto& chunk : m_chunks) {
        float chunkCenterX = chunk.gridX * chunkWorldSize + chunkWorldSize * 0.5f - halfWorld;
        float chunkCenterZ = chunk.gridZ * chunkWorldSize + chunkWorldSize * 0.5f - halfWorld;
        QVector3D chunkCenter(chunkCenterX, 0, chunkCenterZ);
        float dist = (cameraPosition - chunkCenter).length();
        int newLOD = qBound(0, (int)(dist / (chunkWorldSize * m_config.lodDistanceMultiplier)), m_config.maxLOD - 1);
        if (newLOD != chunk.lod) {
            int oldLOD = chunk.lod;
            chunk.lod = newLOD;
            chunk.isDirty = true;
            emit chunkLODChanged(chunk.gridX, chunk.gridZ, newLOD);
        }
    }
}

void TerrainSystem::buildChunkMesh(int chunkX, int chunkZ, int lod) {
    int idx = chunkIndex(chunkX, chunkZ);
    if (idx < 0 || idx >= m_chunks.size()) return;
    TerrainChunk& chunk = m_chunks[idx];
    if (chunk.isHole) { chunk.vertices.clear(); chunk.indices.clear(); chunk.isLoaded = true; return; }
    int vertsPerSide = m_config.chunkSize / (1 << lod) + 1;
    float cellSize = m_config.worldSize / (m_config.heightmapSize - 1);
    float halfWorld = m_config.worldSize * 0.5f;
    float startX = chunkX * m_config.chunkSize * cellSize - halfWorld;
    float startZ = chunkZ * m_config.chunkSize * cellSize - halfWorld;
    chunk.vertices.clear();
    chunk.normals.clear();
    chunk.uvs.clear();
    chunk.indices.clear();
    chunk.vertices.reserve(vertsPerSide * vertsPerSide);
    chunk.normals.reserve(vertsPerSide * vertsPerSide);
    chunk.uvs.reserve(vertsPerSide * vertsPerSide);
    int step = 1 << lod;
    for (int z = 0; z < vertsPerSide; ++z) {
        for (int x = 0; x < vertsPerSide; ++x) {
            int hx = chunkX * m_config.chunkSize + x * step;
            int hz = chunkZ * m_config.chunkSize + z * step;
            hx = qBound(0, hx, m_config.heightmapSize - 1);
            hz = qBound(0, hz, m_config.heightmapSize - 1);
            float h = getHeight(hx, hz);
            QVector3D pos(startX + x * step * cellSize, h, startZ + z * step * cellSize);
            chunk.vertices.append(pos);
            chunk.normals.append(getNormal(hx, hz));
            float u = (float)x / (vertsPerSide - 1);
            float v = (float)z / (vertsPerSide - 1);
            chunk.uvs.append(QVector2D(u, v));
        }
    }
    for (int z = 0; z < vertsPerSide - 1; ++z) {
        for (int x = 0; x < vertsPerSide - 1; ++x) {
            int i00 = z * vertsPerSide + x;
            int i10 = i00 + 1;
            int i01 = i00 + vertsPerSide;
            int i11 = i01 + 1;
            chunk.indices.append(i00);
            chunk.indices.append(i01);
            chunk.indices.append(i10);
            chunk.indices.append(i10);
            chunk.indices.append(i01);
            chunk.indices.append(i11);
        }
    }
    chunk.isLoaded = true;
    chunk.isDirty = false;
}

void TerrainSystem::buildAllChunks() {
    int chunksPerSide = (m_config.heightmapSize - 1) / m_config.chunkSize;
    if (chunksPerSide <= 0) chunksPerSide = 1;
    m_chunks.resize(chunksPerSide * chunksPerSide);
    for (int z = 0; z < chunksPerSide; ++z) {
        for (int x = 0; x < chunksPerSide; ++x) {
            int idx = z * chunksPerSide + x;
            m_chunks[idx].gridX = x;
            m_chunks[idx].gridZ = z;
            buildChunkMesh(x, z, 0);
        }
    }
}

void TerrainSystem::generateNormals() {
    int size = m_config.heightmapSize;
    if (m_normals.size() != size * size) m_normals.resize(size * size);
    float cellSize = m_config.worldSize / (size - 1);
    for (int z = 0; z < size; ++z) {
        for (int x = 0; x < size; ++x) {
            float hL = getHeight(qMax(0, x - 1), z);
            float hR = getHeight(qMin(size - 1, x + 1), z);
            float hD = getHeight(x, qMax(0, z - 1));
            float hU = getHeight(x, qMin(size - 1, z + 1));
            QVector3D n(hL - hR, 2.0f * cellSize, hD - hU);
            m_normals[z * size + x] = n.normalized();
        }
    }
    m_minHeight = std::numeric_limits<float>::max();
    m_maxHeight = std::numeric_limits<float>::lowest();
    for (float h : m_heightmap) {
        if (h < m_minHeight) m_minHeight = h;
        if (h > m_maxHeight) m_maxHeight = h;
    }
}

void TerrainSystem::generateUVs() {
    int size = m_config.heightmapSize;
    if (m_uvs.size() != size * size) m_uvs.resize(size * size);
    for (int z = 0; z < size; ++z) {
        for (int x = 0; x < size; ++x) {
            m_uvs[z * size + x] = QVector2D((float)x / (size - 1), (float)z / (size - 1));
        }
    }
}

int TerrainSystem::chunkIndex(int x, int z) const {
    int chunksPerSide = (m_config.heightmapSize - 1) / m_config.chunkSize;
    if (x < 0 || x >= chunksPerSide || z < 0 || z >= chunksPerSide) return -1;
    return z * chunksPerSide + x;
}

float TerrainSystem::sampleHeight(int x, int z) const {
    return getHeight(qBound(0, x, m_config.heightmapSize - 1), qBound(0, z, m_config.heightmapSize - 1));
}

} // namespace ks::engine::graphics
