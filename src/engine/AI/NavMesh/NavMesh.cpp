#include "NavMesh.h"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace ks::

NavMesh::NavMesh() = default;

NavMesh::~NavMesh() {
    cleanup();
}

bool NavMesh::init(VkDevice device, VkPhysicalDevice physDev, const NavMeshParams& params) {
    m_device = device;
    m_physDevice = physDev;
    m_params = params;

    qInfo() << "NavMesh initialized:" << m_tiles.size() << "tiles max";

    // Initialize voxel grid
    clearVoxels();

    return true;
}

void NavMesh::cleanup() {
    m_tiles.clear();
    m_tileIndexMap.clear();
    clearVoxels();

    if (m_device) {
        // Cleanup any GPU buffers if created
    }
}

void NavMesh::clearVoxels() {
    for (int x = 0; x < VOXEL_GRID_SIZE; x++)
        for (int y = 0; y < VOXEL_GRID_SIZE; y++)
            for (int z = 0; z < VOXEL_GRID_SIZE; z++)
                m_voxels[x][y][z] = 0.0f;
}

bool NavMesh::addTile(const QUuid& tileId, const QVector<QVector3D>& verts,
                      const QVector<QVector3D>& trisVerts, const QVector<int>& trisIdx) {
    if (m_tileIndexMap.contains(tileId)) {
        qWarning() << "NavMesh: Tile already exists:" << tileId;
        return false;
    }

    TileData tile;
    tile.id = tileId;
    tile.dirty = true;

    // Calculate bounds
    if (!verts.isEmpty()) {
        tile.minB = verts[0];
        tile.maxB = verts[0];
        for (const auto& v : verts) {
            tile.minB.setX(qMin(tile.minB.x(), v.x()));
            tile.minB.setY(qMin(tile.minB.y(), v.y()));
            tile.minB.setZ(qMin(tile.minB.z(), v.z()));
            tile.maxB.setX(qMax(tile.maxB.x(), v.x()));
            tile.maxB.setY(qMax(tile.maxB.y(), v.y()));
            tile.maxB.setZ(qMax(tile.maxB.z(), v.z()));
        }
    }

    m_tiles.append(tile);
    m_tileIndexMap[tileId] = m_tiles.size() - 1;

    qInfo() << "NavMesh: Added tile" << tileId << "bounds" << tile.minB << "-" << tile.maxB;
    return true;
}

void NavMesh::removeTile(const QUuid& tileId) {
    if (!m_tileIndexMap.contains(tileId)) {
        qWarning() << "NavMesh: Tile not found:" << tileId;
        return;
    }

    int idx = m_tileIndexMap[tileId];
    m_tiles.removeAt(idx);
    m_tileIndexMap.remove(tileId);

    qInfo() << "NavMesh: Removed tile" << tileId;
}

void NavMesh::updateTile(const QUuid& tileId, const QVector<QVector3D>& verts,
                         const QVector<int>& trisIdx) {
    if (!m_tileIndexMap.contains(tileId)) {
        qWarning() << "NavMesh: Tile not found for update:" << tileId;
        return;
    }

    int idx = m_tileIndexMap[tileId];
    m_tiles[idx].dirty = true;
    m_tiles[idx].verts = verts;

    // Rebuild this tile's navigation data
    if (m_tiles[idx].dirty) {
        rebuildTile(idx);
    }
}

bool NavMesh::findPath(const QVector3D& start, const QVector3D& end,
                       QVector<QVector3D>& path, float& pathLength) const {
// Simplified: returns direct path if navmesh has not been built
// In a complete implementation, this would use A* on the generated navmesh

// For now, return direct vector path with basic checks
    QVector3D dir = end - start;
    float dist = dir.length();
    dir.normalize();

    // Campiona punti lungo il raggio
    int numSteps = qMax(1, int(dist / m_params.agentRadius));
    path.resize(numSteps + 1);
    pathLength = dist;

    for (int i = 0; i <= numSteps; i++) {
        path[i] = start + dir * (dist * i / numSteps);
    }

    return true;
}

bool NavMesh::getRandomPoint(float radius, QVector3D& point) const {
    // Simplified: returns a random point within the first tile's bounds
    if (m_tiles.isEmpty()) return false;

    const TileData& tile = m_tiles.first();
    if (tile.verts.isEmpty()) return false;

    // Punto casuale nella bounding box del tile
    float minX = tile.minB.x(), minY = tile.minB.y(), minZ = tile.minB.z();
    float maxX = tile.maxB.x(), maxY = tile.maxB.y(), maxZ = tile.maxB.z();

    // Aggiungi un offset basato sul radius
    float offset = radius > 0 ? radius : 1.0f;

    point.setX(qMin(maxX - offset, qMax(minX + offset, QRandomGenerator::global()->bounded(minX, maxX))));
    point.setY(qMin(maxY - offset, qMax(minY + offset, QRandomGenerator::global()->bounded(minY, maxY))));
    point.setZ(qMin(maxZ - offset, qMax(minZ + offset, QRandomGenerator::global()->bounded(minZ, maxZ))));

    return true;
}

bool NavMesh::isPositionValid(const QVector3D& pos, float agentRadius) const {
    if (m_tiles.isEmpty()) return true; // Se nessun navmesh, assumi che sia valido

    // Controlla tutti i tile
    for (const auto& tile : m_tiles) {
        // Controlla se la posizione è dentro i bounds del tile
        if (pos.x() >= tile.minB.x() && pos.x() <= tile.maxB.x() &&
            pos.y() >= tile.minB.y() && pos.y() <= tile.maxB.y() &&
            pos.z() >= tile.minB.z() && pos.z() <= tile.maxB.z()) {

            // Controlla distance to nearest polygon (semplificato)
            for (int v = 0; v < tile.verts.size() - 2; v += 3) {
                // Triangolo
                QVector3D v1 = tile.verts[v];
                QVector3D v2 = tile.verts[v + 1];
                QVector3D v3 = tile.verts[v + 2];

                // Calcola distance da punto al triangolo (Moller-Trumbore)
                QVector3D edge1 = v2 - v1;
                QVector3D edge2 = v3 - v1;
                QVector3D h = QVector3D::crossProduct(pos - v1, edge2);
                float a = QVector3D::dotProduct(edge1, h);

                if (fabs(a) < 0.0001) return false;

                float f = 1.0f / a;
                QVector3D s = pos - v1;
                float u = f * QVector3D::dotProduct(s, h);
                if (u < 0.0f || u > 1.0f) return false;

                QVector3D q = QVector3D::crossProduct(s, edge1);
                float v = f * QVector3D::dotProduct(edge2, q);
                if (v < 0.0f || u + v > 1.0f) return false;

                // Controlla altezza (y coordinate)
                float w = f * QVector3D::dotProduct(edge1, QVector3D::crossProduct(s, edge2));
                if (w < 0.0f) return false;
            }

            return true;
        }
    }

    return false;
}

QUuid NavMesh::getTileForPosition(const QVector3D& pos) const {
    for (int i = 0; i < m_tiles.size(); i++) {
        const auto& tile = m_tiles[i];
        if (pos.x() >= tile.minB.x() && pos.x() <= tile.maxB.x() &&
            pos.y() >= tile.minB.y() && pos.y() <= tile.maxB.y() &&
            pos.z() >= tile.minB.z() && pos.z() <= tile.maxB.z()) {
            return tile.id;
        }
    }
    return QUuid(); // Invalid uuid
}

void NavMesh::rebuild() {
    qInfo() << "NavMesh: Rebuilding entire navmesh";

    // Clear existing tiles
    m_tiles.clear();
    m_tileIndexMap.clear();

    // Clear voxel grid
    clearVoxels();

    // TODO: In un'implementazione completa, qui verrebbero:
    // 1. Voxelization of all scene geometry
    // 2. Contour tracing to generate polygons
    // 3. Mesh generation per tile
    // 4. Link generation between adjacent polygons

    qInfo() << "NavMesh: Rebuild complete -" << m_tiles.size() << "tiles";
}

float NavMesh::computeCost(const QVector3D& start, const QVector3D& end) const {
    float dx = end.x() - start.x();
    float dy = end.y() - start.y();
    float dz = end.z() - start.z();
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

bool NavMesh::isValidPoly(int tileIdx, int polyIdx, const QVector3D& pos, float radius) const {
    if (tileIdx < 0 || tileIdx >= m_tiles.size()) return false;
    if (polyIdx < 0 || polyIdx >= (int)m_tiles[tileIdx].verts.size()) return false;

    const auto& tile = m_tiles[tileIdx];
    // Simplified: checks if point is inside the triangle
    const QVector3D& v1 = tile.verts[polyIdx];
    const QVector3D& v2 = tile.verts[polyIdx + 1];
    const QVector3D& v3 = tile.verts[polyIdx + 2];

    // Check using barycentric coordinates
    QVector3D v0v1 = v2 - v1;
    QVector3D v0v2 = v3 - v1;
    QVector3D v0vp = pos - v1;

    float dot00 = QVector3D::dotProduct(v0v1, v0v1);
    float dot01 = QVector3D::dotProduct(v0v1, v0v2);
    float dot02 = QVector3D::dotProduct(v0v1, v0vp);
    float dot11 = QVector3D::dotProduct(v0v2, v0vp);
    float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
    float u = (dot11 * dot02 - dot01 * dot02) * invDenom;
    float v = (dot00 * dot02 - dot01 * dot02) * invDenom;

    return (u >= 0.0f) && (v >= 0.0f) && (u + v < 1.0f);
}

void NavMesh::rebuildTile(int tileIdx) {
    if (tileIdx < 0 || tileIdx >= m_tiles.size()) return;

    auto& tile = m_tiles[tileIdx];
    if (tile.verts.size() < 3) {
        tile.dirty = false;
        return;
    }

    qInfo() << "NavMesh: Rebuilding tile" << tile.id << "with" << tile.verts.size() << "vertices";

// Simplified: mark as dirty and let buildContours process it
// In a complete implementation, here would be:
// 1. Voxelization of the tile's geometry
// 2. Contour tracing to generate polygons
// 3. Polygon area calculation
// 4. Link generation

    tile.dirty = false;
}