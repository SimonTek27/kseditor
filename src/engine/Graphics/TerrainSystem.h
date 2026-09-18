#pragma once

#include "../EngineModule.h"
#include <QObject>
#include <QVector>
#include <QVector3D>
#include <QMatrix4x4>
#include <QImage>
#include <QJsonObject>
#include <functional>

namespace ks::engine::graphics {

struct TerrainConfig {
    int heightmapSize = 513;
    float worldSize = 2048.0f;
    float maxHeight = 512.0f;
    int chunkSize = 64;
    int maxLOD = 6;
    int splatmapLayers = 8;
    float lodDistanceMultiplier = 1.5f;
};

struct TerrainLayer {
    QString name;
    QString diffuseTexture;
    QString normalTexture;
    float tilingX = 1.0f;
    float tilingY = 1.0f;
    float minHeight = 0.0f;
    float maxHeight = 1.0f;
    float minSlope = 0.0f;
    float maxSlope = 90.0f;
    float opacity = 1.0f;
};

struct TerrainChunk {
    int lod = 0;
    int gridX = 0;
    int gridZ = 0;
    QVector<QVector3D> vertices;
    QVector<QVector3D> normals;
    QVector<QVector2D> uvs;
    QVector<quint32> indices;
    bool isHole = false;
    bool isLoaded = false;
    bool isDirty = true;
};

struct TerrainRaycastResult {
    bool hit = false;
    QVector3D position;
    QVector3D normal;
    float height = 0.0f;
    int chunkX = 0;
    int chunkZ = 0;
    int triangleIndex = -1;
};

class TerrainSystem : public QObject, public EngineModule {
    Q_OBJECT
public:
    static TerrainSystem& instance() { static TerrainSystem s; return s; }

    QString moduleName() const override { return "TerrainSystem"; }
    QString moduleId() const override { return "ks.terrain"; }
    bool initialize() override;
    void shutdown() override;

    void configure(const TerrainConfig& config);
    const TerrainConfig& config() const { return m_config; }

    bool loadHeightmap(const QString& filePath);
    bool loadHeightmapFromData(const float* data, int width, int height);
    bool saveHeightmap(const QString& filePath) const;
    bool loadSplatmap(const QString& filePath);
    bool loadSplatmapFromData(const uchar* data, int width, int height, int channels);

    void setHeight(int x, int z, float height);
    float getHeight(int x, int z) const;
    float getHeightBilinear(float x, float z) const;

    QVector3D getNormal(int x, int z) const;
    QVector3D getNormalBilinear(float x, float z) const;

    TerrainRaycastResult raycast(const QVector3D& origin, const QVector3D& direction, float maxDistance = 10000.0f) const;

    void setHole(int chunkX, int chunkZ, bool isHole);
    bool isHole(int chunkX, int chunkZ) const;

    void addLayer(const TerrainLayer& layer);
    void removeLayer(int index);
    const QVector<TerrainLayer>& layers() const { return m_layers; }

    void setSplatmapPixel(int x, int z, const QVector<float>& weights);
    QVector<float> getSplatmapPixel(int x, int z) const;

    QVector3D getTerrainSize() const;
    QVector3D getTerrainCenter() const;
    float getTerrainMinHeight() const;
    float getTerrainMaxHeight() const;

    void updateLODs(const QVector3D& cameraPosition);

    QVector<TerrainChunk>& chunks() { return m_chunks; }
    const QVector<TerrainChunk>& chunks() const { return m_chunks; }

    const QVector<float>& heightmapData() const { return m_heightmap; }
    const QImage& splatmapImage() const { return m_splatmap; }

signals:
    void terrainLoaded();
    void terrainModified();
    void chunkLODChanged(int chunkX, int chunkZ, int newLOD);
    void heightChanged(int x, int z, float newHeight);

private:
    void buildChunkMesh(int chunkX, int chunkZ, int lod);
    void buildAllChunks();
    void generateNormals();
    void generateUVs();
    int chunkIndex(int x, int z) const;
    float sampleHeight(int x, int z) const;

    TerrainConfig m_config;
    QVector<float> m_heightmap;
    QImage m_splatmap;
    QVector<TerrainLayer> m_layers;
    QVector<TerrainChunk> m_chunks;
    QVector<bool> m_holes;
    QVector<QVector3D> m_normals;
    QVector<QVector2D> m_uvs;
    float m_minHeight = 0.0f;
    float m_maxHeight = 0.0f;
    bool m_heightmapLoaded = false;
};

} // namespace ks::engine::graphics
