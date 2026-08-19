#include "SculptLayer.h"
#include <cmath>
#include <algorithm>

SculptLayersManager::SculptLayersManager(QObject* parent) : QObject(parent) {}

SculptLayersManager::~SculptLayersManager() = default;

int SculptLayersManager::addLayer(const QString& name) {
    SculptLayer layer(name);
    m_layers.append(layer);
    if (m_layers.size() == 1) {
        setCurrentLayer(0);
    }
    emit layerAdded(m_layers.size() - 1);
    emit layersChanged();
    return m_layers.size() - 1;
}

bool SculptLayersManager::removeLayer(int index) {
    if (index < 0 || index >= m_layers.size()) return false;
    if (m_layers.size() <= 1) return false; // Keep at least one layer
    
    m_layers.removeAt(index);
    
    // Adjust current layer index if needed
    if (m_currentLayer >= m_layers.size()) {
        m_currentLayer = m_layers.size() - 1;
    }
    if (m_currentLayer < 0) m_currentLayer = 0;
    
    emit layerRemoved(index);
    emit layersChanged();
    emit currentLayerChanged(m_currentLayer);
    return true;
}

bool SculptLayersManager::renameLayer(int index, const QString& newName) {
    if (index < 0 || index >= m_layers.size()) return false;
    m_layers[index].name = newName;
    emit layerRenamed(index, newName);
    return true;
}

void SculptLayersManager::setLayerVisible(int index, bool visible) {
    if (index < 0 || index >= m_layers.size()) return;
    m_layers[index].visible = visible;
    emit layerVisibilityChanged(index, visible);
}

void SculptLayersManager::setLayerLocked(int index, bool locked) {
    if (index < 0 || index >= m_layers.size()) return;
    m_layers[index].locked = locked;
    emit layerLockedChanged(index, locked);
}

void SculptLayersManager::setLayerBlendMode(int index, float mode) {
    if (index < 0 || index >= m_layers.size()) return;
    m_layers[index].blendMode = mode;
    emit layerBlendModeChanged(index, mode);
}

void SculptLayersManager::setLayerOpacity(int index, float opacity) {
    if (index < 0 || index >= m_layers.size()) return;
    m_layers[index].opacity = opacity;
    emit layerOpacityChanged(index, opacity);
}

void SculptLayersManager::setCurrentLayer(int index) {
    if (index < 0 || index >= m_layers.size()) return;
    if (m_currentLayer != index) {
        m_currentLayer = index;
        emit currentLayerChanged(index);
    }
}

void SculptLayersManager::setVertexWeight(int layerIndex, int vertexIndex, float weight) {
    if (layerIndex < 0 || layerIndex >= m_layers.size()) return;
    if (vertexIndex < 0 || vertexIndex >= m_layers[layerIndex].vertexWeights.size()) {
        // Extend weights vector if needed
        int needed = vertexIndex + 1 - m_layers[layerIndex].vertexWeights.size();
        m_layers[layerIndex].vertexWeights.resize(m_layers[layerIndex].vertexWeights.size() + needed, 0.0f);
    }
    m_layers[layerIndex].vertexWeights[vertexIndex] = weight;
    // Clamp to 0-1
    m_layers[layerIndex].vertexWeights[vertexIndex] = qBound(0.0f, m_layers[layerIndex].vertexWeights[vertexIndex], 1.0f);
}

float SculptLayersManager::vertexWeight(int layerIndex, int vertexIndex) const {
    if (layerIndex < 0 || layerIndex >= m_layers.size()) return 0.0f;
    if (vertexIndex < 0 || vertexIndex >= m_layers[layerIndex].vertexWeights.size()) return 0.0f;
    return m_layers[layerIndex].vertexWeights[vertexIndex];
}

QVector<float> SculptLayersManager::vertexWeights(int layerIndex) const {
    if (layerIndex < 0 || layerIndex >= m_layers.size()) return QVector<float>();
    return m_layers[layerIndex].vertexWeights;
}

int SculptLayersManager::sculptBrush(int layerIndex, const QVector3D& center, float radius, float strength, int mode,
                                     const QVector3D& drag, const QVector3D& previousCenter,
                                     float falloffPower, const QSet<int>* pinned) {
    if (layerIndex < 0 || layerIndex >= m_layers.size()) return 0;
    if (m_layers[layerIndex].locked) return 0;
    
    // Apply brush to this layer's weights
    SculptLayer& layer = m_layers[layerIndex];
    int affected = 0;
    
    // For simplicity, apply to all vertices (in a real implementation would be limited to brush radius)
    for (int vi = 0; vi < layer.vertexWeights.size(); ++vi) {
        float d = qSqrt(QVector3D::dotProduct(QVector3D(vi % 10 - 5, 0, vi / 10 - 5), QVector3D(vi % 10 - 5, 0, vi / 10 - 5))); // placeholder distance
        if (d > radius) continue;
        
        float t = 1.0f - (d / radius);
        float falloff;
        if (qAbs(falloffPower - 2.0f) < 1e-4f)
            falloff = t * t * (3.0f - 2.0f * t);
        else
            falloff = t > 0.0f ? qPow(t, qBound(0.25f, falloffPower, 8.0f)) : 0.0f;
        if (falloff <= 0.001f) continue;
        affected++;
        
        // Update weight based on brush mode
        switch (mode) {
            case 0: // draw - add weight
                layer.vertexWeights[vi] = qMin(1.0f, layer.vertexWeights[vi] + strength * falloff);
                break;
            case 1: // smooth - average with neighbors
                // Would need neighbor averaging
                break;
            case 2: // grab - pull toward center
                // Would need center position logic
                break;
            default:
                break;
        }
    }
    
    emit sculptUpdated();
    return affected;
}

void SculptLayersManager::bakeCurrentLayer() {
    if (m_currentLayer < 0 || m_currentLayer >= m_layers.size()) return;
    if (m_layers[m_currentLayer].vertexWeights.isEmpty()) return;
    
    // Apply layer weights to base mesh geometry
    // ... would modify the underlying mesh vertices based on layer weights
    
    emit baked();
}