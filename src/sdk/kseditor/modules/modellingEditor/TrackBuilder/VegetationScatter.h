#pragma once
// ============================================================================
// VegetationScatter.h
// Instanced vegetation placement system for terrain.
// Provides density-map-based scattering of trees, grass, bushes, rocks,
// and other props on terrain surfaces with slope/height masking.
// ============================================================================

#include <QObject>
#include <QVector>
#include <QVector3D>
#include <QImage>
#include <QUuid>
#include <QJsonObject>
#include <QJsonArray>
#include <random>

namespace ks {

// ============================================================================
// Vegetation Types
// ============================================================================
enum class VegetationType {
    Tree,           // Large trees (billboard or mesh)
    Bush,           // Medium shrubs
    Grass,          // Ground cover (instanced quads)
    Rock,           // Scattered rocks
    Flower,         // Decorative flowers
    Custom          // User-defined mesh
};

// ============================================================================
// Vegetation Instance
// ============================================================================
struct VegetationInstance {
    QUuid id;
    QVector3D position;
    float rotationY = 0.0f;             // Random Y rotation [0, 360]
    float scale = 1.0f;                 // Uniform scale
    float scaleVariance = 0.0f;         // Random scale variation
    float slopeAngle = 0.0f;            // Terrain slope at placement (degrees)
    float heightAboveSea = 0.0f;        // Height at placement
};

// ============================================================================
// Vegetation Preset
// ============================================================================
struct VegetationPreset {
    QString name;
    VegetationType type = VegetationType::Tree;
    QString meshPath;                   // Path to 3D model (or empty for billboard)
    QString texturePath;                // Diffuse texture
    QString normalPath;                 // Normal map
    float baseScale = 1.0f;
    float scaleVariance = 0.2f;        // ±20% random scale
    float density = 0.5f;              // Instances per square meter
    float minHeight = 0.0f;            // Minimum terrain height
    float maxHeight = 1000.0f;         // Maximum terrain height
    float minSlope = 0.0f;             // Minimum slope angle (degrees)
    float maxSlope = 45.0f;            // Maximum slope angle (degrees)
    float minSlopeAngle = 0.0f;        // Alias for clarity
    float maxSlopeAngle = 90.0f;
    bool alignToNormal = false;        // Align to terrain normal
    bool castShadows = true;
    bool receiveShadows = true;
    float windResponse = 0.5f;         // How much wind affects this vegetation

    // Billboard settings (for trees at distance)
    bool useBillboard = true;
    float billboardDistance = 100.0f;  // Distance to switch to billboard
    float billboardFadeStart = 80.0f;

    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);
};

// ============================================================================
// Vegetation Layer (a scatter layer on the terrain)
// ============================================================================
struct VegetationLayer {
    QUuid id;
    QString name;
    VegetationPreset preset;
    bool enabled = true;
    bool locked = false;
    float densityMultiplier = 1.0f;

    // Density map (painted by user)
    QImage densityMap;                  // Grayscale: 0=none, 255=full density
    int densityMapResolution = 512;     // Resolution of the density map

    // Random seed for reproducibility
    int randomSeed = 42;

    // Cached instances
    QVector<VegetationInstance> instances;

    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);
};

// ============================================================================
// VegetationScatter - Main scatter system
// ============================================================================
class VegetationScatter : public QObject
{
    Q_OBJECT
public:
    explicit VegetationScatter(QObject* parent = nullptr);
    ~VegetationScatter() override;

    // ---- Layer management ---------------------------------------------------
    int addLayer(const QString& name, const VegetationPreset& preset);
    void removeLayer(int index);
    void moveLayer(int fromIndex, int toIndex);
    int layerCount() const { return m_layers.size(); }
    VegetationLayer& layer(int i) { return m_layers[i]; }
    const VegetationLayer& layer(int i) const { return m_layers[i]; }

    // ---- Density painting ---------------------------------------------------
    void paintDensity(int layerIndex, float worldX, float worldZ,
                      float radius, float opacity);
    void clearDensity(int layerIndex);
    void autoMaskBySlope(int layerIndex, float minSlope, float maxSlope);
    void autoMaskByHeight(int layerIndex, float minHeight, float maxHeight);

    // ---- Scatter generation -------------------------------------------------
    // Generate instances for a layer based on density map and terrain
    void generateInstances(int layerIndex);

    // Generate instances for all layers
    void generateAllInstances();

    // Clear instances for a layer
    void clearInstances(int layerIndex);

    // Get all instances (for rendering)
    QVector<VegetationInstance> allInstances() const;

    // Get instances for a specific layer
    const QVector<VegetationInstance>& instances(int layerIndex) const;

    // ---- Terrain data (must be set before scatter) --------------------------
    void setTerrainData(const QVector<float>& heightmap, int gridW, int gridH,
                        float worldW, float worldH);
    void setTerrainNormals(const QVector<QVector3D>& normals);

    // ---- Presets ------------------------------------------------------------
    static VegetationPreset defaultPreset(VegetationType type);
    static QVector<VegetationPreset> builtInPresets();

    // ---- Serialization ------------------------------------------------------
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);

signals:
    void layerAdded(int index);
    void layerRemoved(int index);
    void instancesGenerated(int layerIndex, int count);
    void allInstancesGenerated(int totalCount);
    void densityPainted(int layerIndex);

private:
    float sampleDensity(int layerIndex, float worldX, float worldZ) const;
    float sampleHeight(float worldX, float worldZ) const;
    QVector3D sampleNormal(float worldX, float worldZ) const;
    bool passesMask(const VegetationPreset& preset, float height, float slopeAngle) const;

    QVector<VegetationLayer> m_layers;

    // Terrain data
    QVector<float> m_heightmap;
    QVector<QVector3D> m_normals;
    int m_gridW = 0;
    int m_gridH = 0;
    float m_worldW = 0.0f;
    float m_worldH = 0.0f;
};

} // namespace ks
