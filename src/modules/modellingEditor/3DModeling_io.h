#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QVariant>
#include <QVector2D>
#include <QVector3D>

#include <QVector4D>
#include <QMatrix4x4>
#include <QJsonObject>
#include <QJsonDocument>
#include <QImage>
#include <QTransform>
#include <QQuaternion>
#include <vector>

namespace ks { namespace geometry { class Mesh3D; class Scene3D; class Material3D; } }

namespace ks {

// ============================================================================
// ImportExport3D - File I/O
// ============================================================================

namespace io {

class ImportExport3D : public QObject
{
    Q_OBJECT
public:
    explicit ImportExport3D(QObject* parent = nullptr) : QObject(parent) {}
    ~ImportExport3D() {}

    enum Format {
        OBJ, FBX, GLTF, GLB_FILE, STL, THREE_DS, BLEND,
        DAE, DXF, STEP, IGES, SAT, DWG, KS3D
    };

    static QStringList supportedImportFormats() {
        return {"obj", "fbx", "gltf", "glb", "stl", "3ds", "dae", "dxf", "step", "iges", "sat", "ks3d"};
    }

    static QStringList supportedExportFormats() {
        return {"obj", "fbx", "gltf", "glb", "stl", "dae", "dxf", "ks3d"};
    }

    geometry::Mesh3D* importMesh(const QString& path);
    bool exportMesh(geometry::Mesh3D* mesh, const QString& path, Format format);

    geometry::Scene3D* importScene(const QString& path);
    bool exportScene(geometry::Scene3D* scene, const QString& path, Format format);

    void setImportOptions(const QMap<QString, bool>& options) { m_importOptions = options; }
    void setExportOptions(const QMap<QString, bool>& options) { m_exportOptions = options; }

    struct ImportResult {
        bool success;
        QString error;
        geometry::Scene3D* scene = nullptr;
    };

    ImportResult import(const QString& path);

 signals:
    void importProgress(int percent);
    void exportProgress(int percent);
    void importComplete(bool success);
    void exportComplete(bool success);

private:
    bool importOBJ(const QString& path, geometry::Scene3D* scene);
    bool importGLTF(const QString& path, geometry::Scene3D* scene);
    bool importSTL(const QString& path, geometry::Scene3D* scene);
    bool importFBX(const QString& path, geometry::Scene3D* scene);
    bool importKS3D(const QString& path, geometry::Scene3D* scene);

    bool exportOBJ(geometry::Scene3D* scene, const QString& path);
    bool exportGLTF(geometry::Scene3D* scene, const QString& path);
    bool exportSTL(geometry::Scene3D* scene, const QString& path);
    bool exportKS3D(geometry::Scene3D* scene, const QString& path);

    QMap<QString, bool> m_importOptions;
    QMap<QString, bool> m_exportOptions;
};

class TextureBaker : public QObject
{
    Q_OBJECT
public:
    explicit TextureBaker(QObject* parent = nullptr) : QObject(parent) {}
    ~TextureBaker() {}

    enum BakeType { Diffuse, Normal, Roughness, Metallic, AO, Height, Emission };

    void setSourceMesh(geometry::Mesh3D* mesh) { m_source = mesh; }
    void setTargetResolution(int width, int height) { m_width = width; m_height = height; }

    void addBakeTarget(BakeType type, const QString& outputPath);
    static QString textureTypeName(BakeType type);

    void bake(BakeType type);
    QImage getBakedTexture(BakeType type) const { return m_bakedTextures.value(type); }

 signals:
    void bakeComplete(BakeType type);
    void bakeProgress(BakeType type, int percent);

private:
    geometry::Mesh3D* m_source = nullptr;
    int m_width = 2048;
    int m_height = 2048;
    QMap<BakeType, QString> m_targets;
    QMap<BakeType, QImage> m_bakedTextures;
};

} // namespace io

// ============================================================================
// TexturingSystem - Texture management
// ============================================================================

class TexturingSystem : public QObject
{
    Q_OBJECT
public:
    static TexturingSystem* instance();

    enum TextureType { Diffuse, Normal, Specular, Emissive, Ambient, Height, Opacity, Metallic, Roughness };

    struct TextureLayer {
        QString id;
        QString name;
        TextureType type;
        QString imagePath;
        float opacity = 1.0f;
        int blendMode = 0;
        bool enabled = true;
        QTransform transform;
        QImage texture;
    };

    struct Material {
        QString id;
        QString name;
        QVector3D color;
        float metallic = 0.0f;
        float roughness = 0.5f;
        float emission = 0.0f;
        QVector<TextureLayer> layers;
    };

    void createMaterial(const QString& id, const QString& name);
    void deleteMaterial(const QString& id);
    Material* getMaterial(const QString& id) { auto it = m_materials.find(id); return it != m_materials.end() ? &it.value() : nullptr; }

    QStringList materials() const { return m_materials.keys(); }

    void addLayer(const QString& matId, const TextureLayer& layer);
    void removeLayer(const QString& matId, const QString& layerId);
    void updateLayer(const QString& matId, const QString& layerId, const TextureLayer& layer);

    void setCurrentMaterial(const QString& id) { m_currentMaterial = id; emit currentMaterialChanged(id); }
    QString currentMaterial() const { return m_currentMaterial; }

    void paint(const QString& matId, const QString& layerId, const QImage& image, const QPoint& pos);
    void applyDecal(const QString& matId, const QString& layerId, const QImage& decal, const QPoint& pos, float angle);

    bool exportTextures(const QString& outputDir);
    bool importTextures(const QString& inputDir);

    static QString textureTypeName(TextureType type);
    static TextureType stringToTextureType(const QString& name);

 signals:
    void materialCreated(const QString& id);
    void materialDeleted(const QString& id);
    void currentMaterialChanged(const QString& id);
    void layerAdded(const QString& matId, const QString& layerId);
    void layerRemoved(const QString& matId, const QString& layerId);
    void textureModified(const QString& matId, const QString& layerId);

private:
    explicit TexturingSystem(QObject* parent = nullptr);
    static TexturingSystem* s_instance;

    QMap<QString, Material> m_materials;
    QString m_currentMaterial;
};

} // namespace ks