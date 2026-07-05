#include "TrackEditor.h"
#include <QFile>
#include <QDataStream>
#include <QImage>
#include <QtMath>
#include <QDebug>

namespace ks {

TrackEditor* TrackEditor::s_instance = nullptr;

TrackEditor::TrackEditor(QObject* parent)
    : QObject(parent)
{
}

TrackEditor* TrackEditor::instance()
{
    if (!s_instance) {
        s_instance = new TrackEditor();
    }
    return s_instance;
}

void TrackEditor::newTrack(const QString& name, int width, int height)
{
    m_trackName = name;
    m_terrain.width = width;
    m_terrain.height = height;
    m_terrain.reset();
    recalculateNormals();
    emit terrainModified();
    emit trackNameChanged(name);
}

bool TrackEditor::loadTrack(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QDataStream stream(&file);
    
    int w, h;
    stream >> w >> h;
    
    m_terrain.width = w;
    m_terrain.height = h;
    m_terrain.heightmap.resize(w * h);
    m_terrain.normals.resize(w * h);
    
    for (int i = 0; i < w * h; ++i) {
        float height;
        stream >> height;
        m_terrain.heightmap[i] = height;
    }
    
    recalculateNormals();
    
    emit trackLoaded(path);
    emit terrainModified();
    return true;
}

bool TrackEditor::saveTrack(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    QDataStream stream(&file);
    
    stream << m_terrain.width << m_terrain.height;
    
    for (float h : m_terrain.heightmap) {
        stream << h;
    }
    
    emit trackSaved(path);
    return true;
}

void TrackEditor::exportToImage(const QString& path)
{
    QImage image(m_terrain.width, m_terrain.height, QImage::Format_Grayscale8);
    
    float maxH = 0.0f;
    for (float h : m_terrain.heightmap) {
        if (h > maxH) maxH = h;
    }
    
    if (maxH == 0.0f) maxH = 1.0f;
    
    for (int y = 0; y < m_terrain.height; ++y) {
        for (int x = 0; x < m_terrain.width; ++x) {
            float h = m_terrain.getHeight(x, y);
            int gray = static_cast<int>((h / maxH) * 255.0f);
            image.setPixel(x, y, qBound(0, gray, 255));
        }
    }
    
    image.save(path);
}

void TrackEditor::raiseTerrain(float wx, float wz, float radius, float amount)
{
    applyBrush(wx, wz, [this, radius, amount](float, float, float currentHeight) {
        return currentHeight + amount;
    });
}

void TrackEditor::lowerTerrain(float wx, float wz, float radius, float amount)
{
    applyBrush(wx, wz, [this, radius, amount](float, float, float currentHeight) {
        return currentHeight - amount;
    });
}

void TrackEditor::smoothTerrain(float wx, float wz, float radius, float strength)
{
    float nx = (wx / m_terrain.worldWidth + 0.5f) * m_terrain.width;
    float nz = (wz / m_terrain.worldHeight + 0.5f) * m_terrain.height;
    
    int cx = static_cast<int>(nx);
    int cz = static_cast<int>(nz);
    int r = static_cast<int>(radius);
    
    float totalWeight = 0.0f;
    float weightedSum = 0.0f;
    
    for (int dz = -r; dz <= r; ++dz) {
        for (int dx = -r; dx <= r; ++dx) {
            int x = cx + dx;
            int z = cz + dz;
            
            if (x < 0 || x >= m_terrain.width || z < 0 || z >= m_terrain.height) {
                continue;
            }
            
            float dist = qSqrt(dx * dx + dz * dz);
            if (dist > radius) continue;
            
            float weight = getFalloff(dist, radius);
            float h = m_terrain.getHeight(x, z);
            
            weightedSum += h * weight;
            totalWeight += weight;
        }
    }
    
    float avgHeight = (totalWeight > 0.0f) ? (weightedSum / totalWeight) : 0.0f;
    
    for (int dz = -r; dz <= r; ++dz) {
        for (int dx = -r; dx <= r; ++dx) {
            int x = cx + dx;
            int z = cz + dz;
            
            if (x < 0 || x >= m_terrain.width || z < 0 || z >= m_terrain.height) {
                continue;
            }
            
            float dist = qSqrt(dx * dx + dz * dz);
            if (dist > radius) continue;
            
            float falloff = getFalloff(dist, radius);
            float currentH = m_terrain.getHeight(x, z);
            float newH = currentH + (avgHeight - currentH) * strength * falloff;
            
            m_terrain.setHeight(x, z, newH);
        }
    }
    
    recalculateNormals();
    emit terrainModified();
}

void TrackEditor::flattenTerrain(float wx, float wz, float radius, float targetHeight)
{
    float nx = (wx / m_terrain.worldWidth + 0.5f) * m_terrain.width;
    float nz = (wz / m_terrain.worldHeight + 0.5f) * m_terrain.height;
    
    int cx = static_cast<int>(nx);
    int cz = static_cast<int>(nz);
    int r = static_cast<int>(radius);
    
    for (int dz = -r; dz <= r; ++dz) {
        for (int dx = -r; dx <= r; ++dx) {
            int x = cx + dx;
            int z = cz + dz;
            
            if (x < 0 || x >= m_terrain.width || z < 0 || z >= m_terrain.height) {
                continue;
            }
            
            float dist = qSqrt(dx * dx + dz * dz);
            if (dist > radius) continue;
            
            float falloff = getFalloff(dist, radius);
            float currentH = m_terrain.getHeight(x, z);
            float newH = currentH + (targetHeight - currentH) * falloff;
            
            m_terrain.setHeight(x, z, newH);
        }
    }
    
    recalculateNormals();
    emit terrainModified();
}

void TrackEditor::noiseTerrain(float wx, float wz, float radius, float scale, float amplitude)
{
    float nx = (wx / m_terrain.worldWidth + 0.5f) * m_terrain.width;
    float nz = (wz / m_terrain.worldHeight + 0.5f) * m_terrain.height;
    
    int cx = static_cast<int>(nx);
    int cz = static_cast<int>(nz);
    int r = static_cast<int>(radius);
    
    for (int dz = -r; dz <= r; ++dz) {
        for (int dx = -r; dx <= r; ++dx) {
            int x = cx + dx;
            int z = cz + dz;
            
            if (x < 0 || x >= m_terrain.width || z < 0 || z >= m_terrain.height) {
                continue;
            }
            
            float dist = qSqrt(dx * dx + dz * dz);
            if (dist > radius) continue;
            
            float falloff = getFalloff(dist, radius);
            
            float noiseX = x * scale / m_terrain.width;
            float noiseZ = z * scale / m_terrain.height;
            
            float noise = qSin(noiseX * 10.0f) * qCos(noiseZ * 10.0f) * amplitude;
            noise += qSin(noiseX * 20.0f + 1.5f) * qCos(noiseZ * 20.0f + 1.5f) * amplitude * 0.5f;
            
            float currentH = m_terrain.getHeight(x, z);
            float newH = currentH + noise * falloff;
            
            m_terrain.setHeight(x, z, newH);
        }
    }
    
    recalculateNormals();
    emit terrainModified();
}

void TrackEditor::recalculateNormals()
{
    for (int y = 0; y < m_terrain.height; ++y) {
        for (int x = 0; x < m_terrain.width; ++x) {
            m_terrain.normals[y * m_terrain.width + x] = calculateNormal(x, y);
        }
    }
}

QVector3D TrackEditor::calculateNormal(int x, int y) const
{
    float hL = m_terrain.getHeight(x - 1, y);
    float hR = m_terrain.getHeight(x + 1, y);
    float hD = m_terrain.getHeight(x, y - 1);
    float hU = m_terrain.getHeight(x, y + 1);
    
    QVector3D normal;
    normal.setX(hL - hR);
    normal.setY(2.0f);
    normal.setZ(hD - hU);
    
    normal.normalize();
    return normal;
}

void TrackEditor::applyBrush(float wx, float wz, std::function<float(float, float, float)> func)
{
    float nx = (wx / m_terrain.worldWidth + 0.5f) * m_terrain.width;
    float nz = (wz / m_terrain.worldHeight + 0.5f) * m_terrain.height;
    
    int cx = static_cast<int>(nx);
    int cz = static_cast<int>(nz);
    int r = static_cast<int>(m_brushSize);
    
    for (int dz = -r; dz <= r; ++dz) {
        for (int dx = -r; dx <= r; ++dx) {
            int x = cx + dx;
            int z = cz + dz;
            
            if (x < 0 || x >= m_terrain.width || z < 0 || z >= m_terrain.height) {
                continue;
            }
            
            float dist = qSqrt(dx * dx + dz * dz);
            if (dist > m_brushSize) continue;
            
            float falloff = getFalloff(dist, m_brushSize);
            float currentH = m_terrain.getHeight(x, z);
            float newH = func(wx, wz, currentH);
            
            float blendedH = currentH + (newH - currentH) * falloff * m_brushStrength;
            m_terrain.setHeight(x, z, blendedH);
        }
    }
    
    recalculateNormals();
    emit terrainModified();
}

float TrackEditor::getFalloff(float dist, float radius)
{
    float t = dist / radius;
    if (t >= 1.0f) return 0.0f;
    
    float falloff = 1.0f - t * t;
    return falloff * falloff;
}

void TrackEditor::addSpline(const QString& name, const QVector<QVector3D>& points)
{
    m_splines[name] = points;
}

QVector<QVector3D> TrackEditor::getSplinePoints(const QString& name) const
{
    return m_splines.value(name);
}

void TrackEditor::addMeshDecoration(const QString& name, const QVector3D& position, const QString& meshId)
{
    m_decorations[name] = qMakePair(position, meshId);
}

TrackTerrainTool::TrackTerrainTool(QObject* parent)
    : QObject(parent), m_trackEditor(TrackEditor::instance())
{
}

void TrackTerrainTool::applyAt(const QVector3D& worldPos)
{
    if (!m_trackEditor || !m_strokeActive) return;
    
    switch (m_currentTool) {
        case Raise:
            m_trackEditor->raiseTerrain(worldPos.x(), worldPos.z(), m_brushRadius, m_brushStrength);
            break;
        case Lower:
            m_trackEditor->lowerTerrain(worldPos.x(), worldPos.z(), m_brushRadius, m_brushStrength);
            break;
        case Smooth:
            m_trackEditor->smoothTerrain(worldPos.x(), worldPos.z(), m_brushRadius, m_brushStrength);
            break;
        case Flatten:
            m_trackEditor->flattenTerrain(worldPos.x(), worldPos.z(), m_brushRadius, getHeightAt(worldPos));
            break;
        case Noise:
            m_trackEditor->noiseTerrain(worldPos.x(), worldPos.z(), m_brushRadius, 0.1f, m_brushStrength);
            break;
        case Pick:
            m_lastPos = worldPos;
            break;
        case Ramp: {
            float t0 = getHeightAt(m_lastPos);
            float t1 = getHeightAt(worldPos);
            float dx = worldPos.x() - m_lastPos.x();
            float dz = worldPos.z() - m_lastPos.z();
            float dist = std::sqrt(dx*dx + dz*dz);
            if (dist > 0.01f) {
                float nx = (worldPos.x() - m_lastPos.x()) / dist;
                float nz = (worldPos.z() - m_lastPos.z()) / dist;
                for (float d = -m_brushRadius; d <= m_brushRadius; d += 1.0f) {
                    float px = worldPos.x() + nx * d;
                    float pz = worldPos.z() + nz * d;
                    float ratio = (d + m_brushRadius) / (2.0f * m_brushRadius);
                    float targetH = t0 + (t1 - t0) * ratio;
                    m_trackEditor->flattenTerrain(px, pz, 1.0f, targetH);
                }
            }
            break;
        }
    }
    m_lastPos = worldPos;
}

float TrackTerrainTool::getHeightAt(const QVector3D& pos) const
{
    if (!m_trackEditor) return 0.0f;
    return m_trackEditor->terrain()->getHeightWorld(pos.x(), pos.z());
}

QVector3D TrackTerrainTool::getNormalAt(const QVector3D& pos) const
{
    if (!m_trackEditor) return QVector3D(0, 1, 0);
    TrackTerrain* terrain = m_trackEditor->terrain();
    float nx = (pos.x() / terrain->worldWidth + 0.5f) * terrain->width;
    float nz = (pos.z() / terrain->worldHeight + 0.5f) * terrain->height;
    int x = static_cast<int>(nx);
    int z = static_cast<int>(nz);
    if (x < 0 || x >= terrain->width || z < 0 || z >= terrain->height) return QVector3D(0, 1, 0);
    
    int idx = z * terrain->width + x;
    if (idx < 0 || idx >= terrain->normals.size()) return QVector3D(0, 1, 0);
    return terrain->normals[idx];
}

void TrackTerrainTool::beginStroke()
{
    m_strokeActive = true;
}

void TrackTerrainTool::endStroke()
{
    m_strokeActive = false;
}

} // namespace ks