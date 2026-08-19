#include "BrushMaskManager.h"
#include <cmath>
#include <algorithm>

BrushMaskManager::BrushMaskManager(QObject* parent) : QObject(parent) {}

BrushMaskManager::~BrushMaskManager() = default;

void BrushMaskManager::setMaskMode(BrushMaskMode mode) {
    if (m_maskMode != mode) {
        m_maskMode = mode;
        emit maskModeChanged(mode);
    }
}

void BrushMaskManager::setSelectedVertices(const QVector<int>& vertices) {
    m_selectedVertices = vertices;
    emit selectionChanged();
}

void BrushMaskManager::setSelectedFaces(const QVector<int>& faces) {
    m_selectedFaces = faces;
    emit selectionChanged();
}

void BrushMaskManager::setMaskArea(const QVector3D& center, float radius) {
    m_maskCenter = center;
    m_maskRadius = radius;
    emit maskAreaChanged(center, radius);
}

void BrushMaskManager::setSymmetryEnabled(bool enabled) {
    if (m_symmetryEnabled != enabled) {
        m_symmetryEnabled = enabled;
        emit symmetryToggled(enabled);
    }
}

QSet<int> BrushMaskManager::applyMaskToBrush(int objectId, const QVector3D& center, float radius, 
    float strength, int brushMode) const {
    QSet<int> affected;
    
    switch (m_maskMode) {
        case MaskVertex: {
            // Only affect selected vertices
            for (int vi : m_selectedVertices) {
                float d = (QVector3D(vi % 100 - 50, 0, vi / 100 - 50) - center).length(); // placeholder
                if (d <= radius) {
                    affected.insert(vi);
                }
            }
            break;
        }
        case MaskFace: {
            // Only affect faces in selection
            for (int fi : m_selectedFaces) {
                // Simple approximation - would need actual face data
                affected.insert(fi);
            }
            break;
        }
        case MaskSculpt: {
            // Would use sculpt layer weights
            // For now, affect all vertices within radius
            // In real implementation: check per-vertex weights from sculpt layers
            float d = radius; // placeholder
            if (d > 0) {
                // Affect vertices within radius
            }
            break;
        }
        case MaskPaint: {
            // Would use paint layer masks
            // For now, affect all vertices within radius
            break;
        }
        case MaskArea: {
            // Affect vertices within the mask radius area
            // Simple: all vertices within radius
            // Real impl: check distance from maskCenter
            for (int i = 0; i < 100; ++i) { // placeholder loop
                float d = (QVector3D(i % 10 - 5, 0, i / 10 - 5) - center).length();
                if (d <= m_maskRadius) {
                    affected.insert(i);
                }
            }
            break;
        }
        case MaskSymmetry: {
            // Apply mask with symmetry (mirror left/right)
            // Would check symmetry plane and mirror affected vertices
            // For now, just affect as normal
            break;
        }
        default: {
            // No masking - affect all within radius
            // In real implementation, would iterate all vertices
            break;
        }
    }
    
    return affected;
}