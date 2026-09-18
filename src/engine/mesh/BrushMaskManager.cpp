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
            // Only affect selected vertices that are within brush radius
            for (int vi : m_selectedVertices) {
                float d;
                if (!m_vertexPositions.isEmpty() && vi < m_vertexPositions.size()) {
                    d = (m_vertexPositions[vi] - center).length();
                } else {
                    d = (QVector3D(float(vi % 100) - 50.0f, 0.0f, float(vi / 100) - 50.0f) - center).length();
                }
                if (d <= radius) {
                    affected.insert(vi);
                }
            }
            break;
        }
        case MaskFace: {
            // Only affect faces in selection - insert all vertex indices of selected faces
            for (int fi : m_selectedFaces) {
                // Face index to vertex indices would require face data
                // For now, treat face index as vertex index (legacy behavior)
                affected.insert(fi);
            }
            break;
        }
        case MaskSculpt: {
            // Use sculpt layer weights to determine affected vertices
            for (int vi = 0; vi < m_sculptLayerWeights.size(); ++vi) {
                float weight = m_sculptLayerWeights[vi];
                if (weight <= 0.0f) continue;
                float d;
                if (!m_vertexPositions.isEmpty() && vi < m_vertexPositions.size()) {
                    d = (m_vertexPositions[vi] - center).length();
                } else {
                    d = (QVector3D(float(vi % 100) - 50.0f, 0.0f, float(vi / 100) - 50.0f) - center).length();
                }
                if (d <= radius && weight > 0.0f) {
                    affected.insert(vi);
                }
            }
            break;
        }
        case MaskPaint: {
            // Use paint layer masks to determine affected vertices
            for (int vi = 0; vi < m_paintLayerMasks.size(); ++vi) {
                float mask = m_paintLayerMasks[vi];
                if (mask <= 0.0f) continue;
                float d;
                if (!m_vertexPositions.isEmpty() && vi < m_vertexPositions.size()) {
                    d = (m_vertexPositions[vi] - center).length();
                } else {
                    d = (QVector3D(float(vi % 100) - 50.0f, 0.0f, float(vi / 100) - 50.0f) - center).length();
                }
                if (d <= radius && mask > 0.0f) {
                    affected.insert(vi);
                }
            }
            break;
        }
        case MaskArea: {
            // Affect vertices within the mask radius area
            int vertCount = m_vertexPositions.isEmpty() ? 100 : m_vertexPositions.size();
            for (int vi = 0; vi < vertCount; ++vi) {
                float d;
                if (!m_vertexPositions.isEmpty() && vi < m_vertexPositions.size()) {
                    d = (m_vertexPositions[vi] - m_maskCenter).length();
                } else {
                    d = (QVector3D(float(vi % 10) - 5.0f, 0.0f, float(vi / 10) - 5.0f) - m_maskCenter).length();
                }
                if (d <= m_maskRadius) {
                    affected.insert(vi);
                }
            }
            break;
        }
        case MaskSymmetry: {
            // Apply mask with symmetry (mirror left/right across symmetry plane)
            int vertCount = m_vertexPositions.isEmpty() ? 0 : m_vertexPositions.size();
            for (int vi = 0; vi < vertCount; ++vi) {
                float d;
                if (!m_vertexPositions.isEmpty() && vi < m_vertexPositions.size()) {
                    d = (m_vertexPositions[vi] - center).length();
                } else {
                    continue;
                }
                if (d <= radius) {
                    affected.insert(vi);
                    // Mirror across symmetry plane
                    if (m_symmetryEnabled && !m_symmetryPlaneNormal.isNull()) {
                        QVector3D pos = m_vertexPositions[vi];
                        float distFromPlane = QVector3D::dotProduct(pos, m_symmetryPlaneNormal) - m_symmetryPlaneOffset;
                        QVector3D mirrored = pos - 2.0f * distFromPlane * m_symmetryPlaneNormal;
                        // Find closest vertex to mirrored position
                        float bestDist = 1e9f;
                        int bestIdx = -1;
                        for (int mi = 0; mi < vertCount; ++mi) {
                            float md = (m_vertexPositions[mi] - mirrored).length();
                            if (md < bestDist) {
                                bestDist = md;
                                bestIdx = mi;
                            }
                        }
                        if (bestIdx >= 0 && bestDist < radius * 0.5f) {
                            affected.insert(bestIdx);
                        }
                    }
                }
            }
            break;
        }
        default: {
            // No masking - affect all vertices within radius
            int vertCount = m_vertexPositions.isEmpty() ? 0 : m_vertexPositions.size();
            for (int vi = 0; vi < vertCount; ++vi) {
                float d = (m_vertexPositions[vi] - center).length();
                if (d <= radius) {
                    affected.insert(vi);
                }
            }
            break;
        }
    }
    
    return affected;
}

void BrushMaskManager::setVertexPositions(const QVector<QVector3D>& positions) {
    m_vertexPositions = positions;
}

void BrushMaskManager::setVertexNormals(const QVector<QVector3D>& normals) {
    m_vertexNormals = normals;
}

void BrushMaskManager::setSculptLayerWeights(const QVector<float>& weights) {
    m_sculptLayerWeights = weights;
}

void BrushMaskManager::setPaintLayerMasks(const QVector<float>& masks) {
    m_paintLayerMasks = masks;
}

void BrushMaskManager::setSymmetryPlane(const QVector3D& normal, float offset) {
    m_symmetryPlaneNormal = normal;
    m_symmetryPlaneOffset = offset;
}