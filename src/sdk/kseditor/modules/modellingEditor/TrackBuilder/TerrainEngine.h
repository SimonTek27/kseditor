#pragma once
// ============================================================================
// TerrainEngine.h
// Terrain heightmap editing engine.
// Provides all brush-based operations TreCorsa exposes:
//   Raise, Lower, Smooth, Flatten, Noise, Ramp, Erosion
// Also handles: satellite heightmap import (SRTM/tile format),
// texture-paint layer management, normal recalculation.
// ============================================================================

#include "TrackBuilderTypes.h"
#include <QObject>
#include <QImage>
#include <QVector>
#include <QVector3D>
#include <QRect>
#include <QFile>
#include <QIODevice>
#include <QByteArray>
#include <functional>

namespace ks { namespace track {

// ============================================================================
// Texture paint layer
// ============================================================================
struct TerrainLayer {
    QString   name;
    QString   textureId;
    QVector<float> weights;     // per-vertex blend weight [0,1]
    float     uvScale = 10.f;

    void resize(int count) { weights.assign(count, 0.f); }
};

// ============================================================================
// TerrainEngine
// ============================================================================
class TerrainEngine : public QObject
{
    Q_OBJECT
public:
    explicit TerrainEngine(QObject* parent = nullptr);

    // ---- Initialise --------------------------------------------------------
    void   init(const TerrainConfig& config);
    void   reset();

    // ---- Heightmap access --------------------------------------------------
    float  getHeight(int x, int z) const;
    void   setHeight(int x, int z, float h);
    float  getHeightWorld(float wx, float wz) const;  // bilinear sample
    void   setHeightWorld(float wx, float wz, float h);

    QVector<float>&       heightmap()       { return m_heightmap; }
    const QVector<float>& heightmap() const { return m_heightmap; }

    int   gridW() const { return m_cfg.gridWidth; }
    int   gridH() const { return m_cfg.gridHeight; }
    float worldW() const { return m_cfg.worldWidth; }
    float worldH() const { return m_cfg.worldHeight; }

    // ---- Brush tools (TreCorsa-equivalent) ---------------------------------
    enum class BrushMode { Raise, Lower, Smooth, Flatten, Noise, Ramp, Erosion };
    Q_ENUM(BrushMode)

    void setBrushRadius(float r) { m_brushRadius = r; }
    void setBrushStrength(float s) { m_brushStrength = qBound(0.f,s,1.f); }
    void setBrushFalloff(float f) { m_brushFalloff = qBound(0.f,f,1.f); }
    void setBrushMode(BrushMode m) { m_brushMode = m; }
    void setFlattenTarget(float h) { m_flattenTarget = h; }  // for Flatten mode

    float brushRadius()   const { return m_brushRadius; }
    float brushStrength() const { return m_brushStrength; }
    BrushMode brushMode() const { return m_brushMode; }

    // Apply brush at world position
    void applyBrush(float wx, float wz);

    // One-shot operations
    void flattenRegion(float wx, float wz, float radius, float targetH);
    void addNoise(float wx, float wz, float radius, float amplitude, float frequency);
    void erode(int iterations = 5);
    void hydraulicErode(int iterations = 50);
    void normalise(float targetMin = 0.f, float targetMax = 50.f);

    // ---- Normals -----------------------------------------------------------
    void   recalcNormals();
    QVector3D normalAt(int x, int z) const;
    QVector3D normalAtWorld(float wx, float wz) const;
    const QVector<QVector3D>& normals() const { return m_normals; }

    // ---- Satellite / heightmap import --------------------------------------
    bool importFromImage(const QImage& heightmapImg, float minH, float maxH);
    bool importFromSRTM(const QString& filePath);   // .hgt file
    bool importFromRaw(const QVector<float>& data, int w, int h);

    // ---- Texture paint layers (3 layers: base, overlay, accent) -----------
    int  addLayer(const QString& name, const QString& textureId, float uvScale = 10.f);
    void removeLayer(int index);
    int  layerCount() const { return m_layers.size(); }
    TerrainLayer&       layer(int i)       { return m_layers[i]; }
    const TerrainLayer& layer(int i) const { return m_layers[i]; }

    void paintLayer(int layerIndex, float wx, float wz, float radius, float opacity);
    void autoMaskBySlope(int layerIndex, float minSlope, float maxSlope);
    void autoMaskByRoad(int layerIndex, float roadBuffer, const QVector<Road>& roads);

    // ---- Export ------------------------------------------------------------
    QImage toHeightmapImage(int resolution = 0) const;   // 0 = native grid
    QVector<float> exportHeightmap() const { return m_heightmap; }

signals:
    void modified(QRect changedRegion);  // grid coords
    void normalsRecalculated();

public:
    TerrainConfig config() const { return m_cfg; }
    void setConfig(const TerrainConfig& cfg) { m_cfg = cfg; }

private:
    float falloff(float distNorm) const;
    void  gridFromWorld(float wx, float wz, int& gx, int& gz) const;
    void  worldFromGrid(int gx, int gz, float& wx, float& wz) const;

    TerrainConfig        m_cfg;
    QVector<float>       m_heightmap;
    QVector<QVector3D>   m_normals;
    QVector<TerrainLayer> m_layers;

    BrushMode m_brushMode     = BrushMode::Raise;
    float     m_brushRadius   = 50.f;
    float     m_brushStrength = 0.5f;
    float     m_brushFalloff  = 0.5f;
    float     m_flattenTarget = 0.f;
};

}} // namespace ks::track
