#include "MultiresLevel.h"
#include <cmath>
#include <algorithm>
#include <QSet>

MultiresManager::MultiresManager(QObject* parent) : QObject(parent) {}

MultiresManager::~MultiresManager() = default;

void MultiresManager::createLevel(const QVector<QVector3D>& vertices, const QVector<int>& faces) {
    m_levels.clear();
    m_levels.append(SubdivisionLevel(vertices, faces));
    m_currentLevel = 0;
    emit levelsChanged(m_levels.size());
    emit levelChanged(0);
}

void MultiresManager::setCurrentLevel(int level) {
    if (level < 0) level = 0;
    if (level >= m_levels.size()) level = m_levels.size() - 1;
    if (m_currentLevel != level) {
        m_currentLevel = level;
        emit levelChanged(level);
    }
}

void MultiresManager::addLevel() {
    if (m_currentLevel < 0 || m_currentLevel >= m_levels.size()) return;
    
    // Create new level by subdividing current level
    const auto& current = m_levels[m_currentLevel];
    SubdivisionLevel newLevel;
    newLevel.vertices = catmullClarkSubdivide(current.vertices, current.faces);
    newLevel.faces = catmullClarkFaceIndices(current.faces);
    newLevel.edgeCreases.assign(newLevel.vertices.size(), 0.0f);
    newLevel.faceNormals = computeVertexNormals(newLevel.vertices, newLevel.faces);
    
    // Propagate edge creases from old to new level
    // ... (implementation would map old edges to new edges)
    
    m_levels.insert(m_currentLevel + 1, newLevel);
    setCurrentLevel(m_currentLevel + 1);
    emit levelsChanged(m_levels.size());
    emit levelChanged(m_currentLevel);
}

void MultiresManager::removeLevel() {
    if (m_levels.size() <= 1) return;
    if (m_currentLevel >= m_levels.size() - 1) m_currentLevel = m_levels.size() - 2;
    m_levels.removeAt(m_currentLevel + 1);
    emit levelsChanged(m_levels.size());
    emit levelChanged(m_currentLevel);
}

void MultiresManager::subdivideCurrentLevel() {
    if (m_levels.isEmpty() || m_currentLevel < 0 || m_currentLevel >= m_levels.size()) return;
    
    auto& current = m_levels[m_currentLevel];
    current.vertices = catmullClarkSubdivide(current.vertices, current.faces);
    current.faces = catmullClarkFaceIndices(current.faces);
    // Recompute normals and creases
    current.faceNormals = computeVertexNormals(current.vertices, current.faces);
    current.edgeCreases.assign(current.vertices.size(), 0.0f);
    
    emit levelChanged(m_currentLevel);
}

void MultiresManager::limitCurrentLevel() {
    if (m_levels.isEmpty() || m_currentLevel < 0 || m_currentLevel >= m_levels.size()) return;
    
    auto& current = m_levels[m_currentLevel];
    // Apply limit surface - preserve shape by constraining new positions
    preserveShape(current.vertices, current.vertices); // Placeholder - would need old positions
    
    emit levelChanged(m_currentLevel);
}

int MultiresManager::sculptBrush(const QVector3D& center, float radius, float strength, int mode,
                                 const QVector3D& drag, const QVector3D& previousCenter,
                                 float falloffPower, const QSet<int>* pinned) {
    if (m_levels.isEmpty()) return 0;
    
    // Sculpt on current level
    const auto& current = m_levels[m_currentLevel];
    // ... apply sculpt brush to current level vertices
    // Then propagate effects to higher levels if needed
    
    // For now, apply to current level only
    QVector<QVector3D> newVerts = current.vertices;
    int affected = 0;
    
    for (int vi = 0; vi < current.vertices.size(); ++vi) {
        // Skip pinned vertices
        if (pinned && pinned->contains(vi)) continue;
        
        float d = (current.vertices[vi] - center).length();
        if (d > radius) continue;
        
        float t = 1.0f - (d / radius);
        float falloff;
        if (qAbs(falloffPower - 2.0f) < 1e-4f)
            falloff = t * t * (3.0f - 2.0f * t);
        else
            falloff = t > 0.0f ? qPow(t, qBound(0.25f, falloffPower, 8.0f)) : 0.0f;
        if (falloff <= 0.001f) continue;
        affected++;
        
        // Apply based on mode - simplified version
        switch (mode) {
            case 0: // draw
                newVerts[vi] += current.vertices[vi].normalized() * (strength * falloff);
                break;
            case 1: // smooth
                // Would need neighbor averaging
                break;
            // ... other modes
            default:
                break;
        }
    }
    
    // Update current level
    m_levels[m_currentLevel].vertices = newVerts;
    emit sculptUpdated();
    return affected;
}

void MultiresManager::bakeCurrentLevel() {
    if (m_levels.isEmpty() || m_currentLevel < 0 || m_currentLevel >= m_levels.size()) return;
    
    // Replace base mesh with current level geometry
    // and remove higher levels
    m_levels[m_currentLevel].vertices = m_levels.last().vertices;
    m_levels[m_currentLevel].faces = m_levels.last().faces;
    m_levels.remove(m_currentLevel + 1, m_levels.size() - m_currentLevel - 1);
    
    emit baked();
    emit levelsChanged(m_levels.size());
    emit levelChanged(m_currentLevel);
}

QVector<QVector3D> MultiresManager::catmullClarkSubdivide(const QVector<QVector3D>& verts, const QVector<int>& faces) {
    // Standard Catmull-Clark subdivision
    QVector<QVector3D> newVerts;
    QVector<int> newFaces;
    
    // Count original vertices and faces
    int numVerts = verts.size();
    int numFaces = faces.size() / 3;
    
    // Compute face points
    QVector<QVector3D> facePoints(numFaces);
    for (int i = 0; i < numFaces; ++i) {
        int fi = i * 3;
        QVector3D fp(0, 0, 0);
        for (int j = 0; j < 3; ++j) {
            fp += verts[faces[fi + j]];
        }
        fp /= 3.0f;
        facePoints[i] = fp;
    }
    
    // Compute new positions for original vertices (averaging of connected face points + edge midpoints)
    QVector<QVector3D> vertPoints(numVerts);
    QVector<int> vertFaceCounts(numVerts, 0);
    
    for (int i = 0; i < numFaces; ++i) {
        int fi = i * 3;
        for (int j = 0; j < 3; ++j) {
            int v = faces[fi + j];
            vertFaceCounts[v]++;
            vertPoints[v] += facePoints[i];
        }
    }
    
    for (int i = 0; i < numVerts; ++i) {
        vertPoints[i] /= vertFaceCounts[i];
        vertPoints[i] = vertPoints[i] * (2.0f / 3.0f) + verts[i] * (1.0f / 3.0f);
    }
    
    // ... continue with edge points and face tessellation
    // This is a simplified implementation - full Catmull-Clark is more complex
    
    return newVerts;
}

QVector<int> MultiresManager::catmullClarkFaceIndices(const QVector<int>& faces) {
    // Return face indices for subdivided mesh
    return faces; // Simplified
}

QVector<QVector3D> MultiresManager::computeVertexNormals(const QVector<QVector3D>& verts, const QVector<int>& faces) {
    QVector<QVector3D> norms(verts.size(), QVector3D(0, 0, 0));
    
    for (int i = 0; i + 2 < faces.size(); i += 3) {
        int i0 = faces[i];
        int i1 = faces[i + 1];
        int i2 = faces[i + 2];
        
        if (i0 >= verts.size() || i1 >= verts.size() || i2 >= verts.size()) continue;
        
        QVector3D n = QVector3D::crossProduct(verts[i1] - verts[i0], verts[i2] - verts[i0]);
        norms[i0] += n;
        norms[i1] += n;
        norms[i2] += n;
    }
    
    for (auto& n : norms) {
        n.normalize();
    }
    
    return norms;
}

void MultiresManager::preserveShape(QVector<QVector3D>& newVerts, const QVector<QVector3D>& oldVerts) {
    // Simple shape preservation - move new vertices toward old positions
    for (int i = 0; i < qMin(newVerts.size(), oldVerts.size()); ++i) {
        newVerts[i] = newVerts[i] * 0.5f + oldVerts[i] * 0.5f;
    }
}