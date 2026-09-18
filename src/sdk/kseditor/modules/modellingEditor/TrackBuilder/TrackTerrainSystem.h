#pragma once

#include <QString>
#include <QVector>
#include <QJsonObject>

/**
 * @brief Track Terrain System for Assetto Corsa
 *
 * Manages terrain data for tracks.
 * Based on:
 * - MeshHouse/TrackTools (GeoNodes modifiers)
 * - ac-track-tools (terrain features)
 * - TreCorsa (terrain generation)
 *
 * Features:
 * - Height map generation
 * - Terrain mesh creation
 * - Surface type assignment
 * - Terrain texturing
 * - LOD generation
 */
class TrackTerrainSystem {
public:
    struct TerrainConfig {
        int resolution = 256;           // pixels
        float size = 1000.0f;           // meters
        float heightScale = 50.0f;      // meters
        float heightOffset = 0.0f;
        QString heightmapPath;
        QString texturePath;
    };

    struct TerrainVertex {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float u = 0.0f;
        float v = 0.0f;
        float normal[3] = {0, 1, 0};
        int surfaceType = 0;
    };

    struct TerrainTriangle {
        int indices[3];
        int surfaceType = 0;
    };

    struct TerrainMesh {
        QVector<TerrainVertex> vertices;
        QVector<TerrainTriangle> triangles;
        int width = 0;
        int height = 0;
    };

    struct TerrainLayer {
        QString name;
        QString texturePath;
        float minHeight = 0.0f;
        float maxHeight = 100.0f;
        float minSlope = 0.0f;
        float maxSlope = 90.0f;
        float opacity = 1.0f;
    };

    // Height map operations
    static QVector<QVector<float>> loadHeightMap(const QString& path);
    static bool saveHeightMap(const QVector<QVector<float>>& heightMap, const QString& path);
    static QVector<QVector<float>> generateHeightMap(int width, int height, float scale);

    // Terrain mesh generation
    static TerrainMesh generateMesh(const QVector<QVector<float>>& heightMap, const TerrainConfig& config);
    static TerrainMesh smoothMesh(const TerrainMesh& mesh, int iterations);
    static TerrainMesh decimateMesh(const TerrainMesh& mesh, float targetReduction);
    static TerrainMesh subdivideMesh(const TerrainMesh& mesh, int subdivisions);

    // Surface assignment
    static QVector<int> assignSurfaces(const TerrainMesh& mesh, const QVector<TerrainLayer>& layers);
    static int getSurfaceAtHeight(float height, const QVector<TerrainLayer>& layers);
    static int getSurfaceAtSlope(float slope, const QVector<TerrainLayer>& layers);

    // Terrain analysis
    static float calculateSlope(const TerrainMesh& mesh, int triangleIndex);
    static float calculateArea(const TerrainMesh& mesh);
    static float calculateMinHeight(const QVector<QVector<float>>& heightMap);
    static float calculateMaxHeight(const QVector<QVector<float>>& heightMap);

    // Export operations
    static bool exportToObj(const TerrainMesh& mesh, const QString& path);
    static bool exportToKn5(const TerrainMesh& mesh, const QString& path);
    static bool exportToFBX(const TerrainMesh& mesh, const QString& path);

    // Presets
    static TerrainConfig getFlatConfig();
    static TerrainConfig getHillConfig();
    static TerrainConfig getMountainConfig();
    static QVector<TerrainLayer> getDefaultLayers();

    // Validation
    static bool validateConfig(const TerrainConfig& config, QString* error = nullptr);
    static bool validateMesh(const TerrainMesh& mesh, QString* error = nullptr);

private:
    static float calculateTriangleNormal(const TerrainVertex& v0, const TerrainVertex& v1,
                                          const TerrainVertex& v2, float* normal);
};

/**
 * @brief Track Terrain Manager - High-level interface
 */
class TrackTerrainManager {
public:
    explicit TrackTerrainManager(const QString& trackPath);

    // Configuration
    void setConfig(const TrackTerrainSystem::TerrainConfig& config);
    TrackTerrainSystem::TerrainConfig getConfig() const { return m_config; }

    // Operations
    bool loadHeightMap();
    bool saveHeightMap();
    bool generateMesh();
    bool assignSurfaces();
    bool exportMesh(const QString& format);

    // Access
    TrackTerrainSystem::TerrainMesh getMesh() const { return m_mesh; }
    QVector<QVector<float>> getHeightMap() const { return m_heightMap; }

    // Analysis
    float getTerrainArea() const;
    float getMinHeight() const;
    float getMaxHeight() const;

private:
    QString m_trackPath;
    TrackTerrainSystem::TerrainConfig m_config;
    TrackTerrainSystem::TerrainMesh m_mesh;
    QVector<QVector<float>> m_heightMap;
    QVector<TrackTerrainSystem::TerrainLayer> m_layers;
};
