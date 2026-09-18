#pragma once

#include <vulkan/vulkan.h>
#include <QString>
#include <QVector3D>
#include <QVector2D>
#include <QSet>
#include <QMap>
#include <QDateTime>

namespace ks {

struct NavMeshParams {
    float agentRadius = 0.5f;
    float agentHeight = 2.0f;
    float agentMaxClimb = 0.5f;
    float agentMaxSlope = 45.0f;
    float regionMinSize = 20.0f;
    float regionMergeSize = 30.0f;
    float edgeMaxLen = 100.0f;
    float partitionCost = 1.0f;
    int maxTiles = 50;
    int maxPolygonsPerTile = 256;
};

struct NavMeshTile {
    QUuid id;
    QVector3D minBounds;
    QVector3D maxBounds;
    QVector<int> polygons;
    QVector<float> areas;
    QVector<int> links;
    QVector<QVector3D> verts;
    QVector<QVector2D> tris;
    bool dirty = true;
};

class NavMesh {
public:
    NavMesh();
    ~NavMesh();

    bool init(VkDevice device, VkPhysicalDevice physDev, const NavMeshParams& params);
    void cleanup();

    // Create tile from mesh data
    bool addTile(const QUuid& tileId, const QVector<QVector3D>& verts,
                 const QVector<QVector3D>& trisVerts, const QVector<int>& trisIdx);

    // Remove tile
    void removeTile(const QUuid& tileId);

    // Update tile
    void updateTile(const QUuid& tileId, const QVector<QVector3D>& verts,
                    const QVector<int>& trisIdx);

    // Pathfinding
    bool findPath(const QVector3D& start, const QVector3D& end,
                  QVector<QVector3D>& path, float& pathLength) const;

    // Sample area (find random point in navmesh)
    bool getRandomPoint(float radius, QVector3D& point) const;

    // Check if position is on navmesh
    bool isPositionValid(const QVector3D& pos, float agentRadius = -1.0f) const;

    // Get tile containing position
    QUuid getTileForPosition(const QVector3D& pos) const;

    // Tile count
    int tileCount() const { return m_tiles.size(); }

    // Rebuild whole navmesh
    void rebuild();

    // Params
    NavMeshParams params() const { return m_params; }

private:
    NavMeshParams m_params;
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physDevice = VK_NULL_HANDLE;

    struct TileData {
        QUuid id;
        QVector3D minB, maxB;
        QVector<int> polys;
        QVector<float> areas;
        QVector<int> links;
        QVector<QVector3D> verts;
        QVector<QVector2D> trisUv;
        bool dirty;
    };

    QVector<TileData> m_tiles;
    QMap<QUuid, int> m_tileIndexMap; // uid -> index in m_tiles

    // Voxel data
    static const int VOXEL_GRID_SIZE = 128;
    float m_voxels[VOXEL_GRID_SIZE][VOXEL_GRID_SIZE][VOXEL_GRID_SIZE];

    // Build pipeline
    void buildVoxelization(const QVector<QVector3D>& vertices, const QVector<int>& indices);
    void buildContours();
    void buildLinks();

    // A* search
    struct Node {
        QVector3D pos;
        int tileIdx;
        int polyIdx;
        int parentIdx;
        float g, h, f;
    };

    std::vector<Node> m_aStarNodes;
    QVector3D m_startPos, m_endPos;

    // Helpers
    float computeCost(const QVector3D& a, const QVector3D& b) const;
    bool isValidPoly(int tileIdx, int polyIdx, const QVector3D& pos, float radius) const;
};

} // namespace ks