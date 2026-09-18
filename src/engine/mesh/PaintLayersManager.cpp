#include "PaintLayersManager.h"
#include <cmath>
#include <algorithm>

PaintLayersManager::PaintLayersManager(QObject* parent) : QObject(parent) {}

PaintLayersManager::~PaintLayersManager() = default;

int PaintLayersManager::addLayer(const QString& name) {
    PaintLayer layer(name);
    m_layers.append(layer);
    if (m_layers.size() == 1) {
        setCurrentLayer(0);
    }
    emit layerAdded(m_layers.size() - 1);
    emit layersChanged();
    return m_layers.size() - 1;
}

bool PaintLayersManager::removeLayer(int index) {
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

bool PaintLayersManager::renameLayer(int index, const QString& newName) {
    if (index < 0 || index >= m_layers.size()) return false;
    m_layers[index].name = newName;
    emit layerRenamed(index, newName);
    return true;
}

void PaintLayersManager::setLayerVisible(int index, bool visible) {
    if (index < 0 || index >= m_layers.size()) return;
    m_layers[index].visible = visible;
    emit layerVisibilityChanged(index, visible);
}

void PaintLayersManager::setLayerLocked(int index, bool locked) {
    if (index < 0 || index >= m_layers.size()) return;
    m_layers[index].locked = locked;
    emit layerLockedChanged(index, locked);
}

void PaintLayersManager::setLayerBlendMode(int index, float mode) {
    if (index < 0 || index >= m_layers.size()) return;
    m_layers[index].blendMode = mode;
    emit layerBlendModeChanged(index, mode);
}

void PaintLayersManager::setLayerOpacity(int index, float opacity) {
    if (index < 0 || index >= m_layers.size()) return;
    m_layers[index].opacity = opacity;
    emit layerOpacityChanged(index, opacity);
}

void PaintLayersManager::setCurrentLayer(int index) {
    if (index < 0 || index >= m_layers.size()) return;
    if (m_currentLayer != index) {
        m_currentLayer = index;
        emit currentLayerChanged(index);
    }
}

void PaintLayersManager::setVertexMask(int layerIndex, int vertexIndex, float weight) {
    if (layerIndex < 0 || layerIndex >= m_layers.size()) return;
    if (vertexIndex < 0 || vertexIndex >= m_layers[layerIndex].mask.size()) {
        // Extend mask if needed
        int needed = vertexIndex + 1 - m_layers[layerIndex].mask.size();
        m_layers[layerIndex].mask.resize(m_layers[layerIndex].mask.size() + needed, 0.0f);
    }
    m_layers[layerIndex].mask[vertexIndex] = weight;
    // Clamp to 0-1
    m_layers[layerIndex].mask[vertexIndex] = qBound(0.0f, m_layers[layerIndex].mask[vertexIndex], 1.0f);
}

float PaintLayersManager::vertexMask(int layerIndex, int vertexIndex) const {
    if (layerIndex < 0 || layerIndex >= m_layers.size()) return 0.0f;
    if (vertexIndex < 0 || vertexIndex >= m_layers[layerIndex].mask.size()) return 0.0f;
    return m_layers[layerIndex].mask[vertexIndex];
}

QVector<float> PaintLayersManager::vertexMasks(int layerIndex) const {
    if (layerIndex < 0 || layerIndex >= m_layers.size()) return QVector<float>();
    return m_layers[layerIndex].mask;
}