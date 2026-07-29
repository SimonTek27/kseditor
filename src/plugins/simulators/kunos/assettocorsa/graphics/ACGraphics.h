#pragma once

// ============================================================================
// ksacGraphics — Assetto Corsa Graphics Library
// Compatible with AC's original rendering engine (KN5, ksPerPixel shaders, etc.)
// ============================================================================

#include <QObject>
#include <QMatrix4x4>
#include <QVector3D>
#include <QVector2D>
#include <QVector>
#include <QMap>
#include <QByteArray>
#include <QString>
#include <QSharedPointer>

namespace ks {
namespace ac {
namespace graphics {

// ============================================================================
// Forward declarations
// ============================================================================

class ACShaderManager;
class ACMaterial;
class ACTexture;
class ACMesh;
class ACNode;
class ACScene;
class ACModelLoader;
class ACRenderer;

// ============================================================================
// Shader constants (matching AC's ksPerPixel* shaders)
// ============================================================================

enum class ACShaderType {
    ksPerPixel,                    // Basic per-pixel
    ksPerPixelAT,                  // Alpha tested
    ksPerPixelMultiMap,            // Multi-map (detail, normal)
    ksPerPixelMultiMap_NMDetail,   // Normal map + detail
    ksPerPixelMultiMap_NMDetail_D, // Detail normal
    ksPerPixelMultiMap_NMDetail_N, // Normal + detail normal
    ksPerPixelMultiMap_AT,         // Alpha tested multi-map
    ksPerPixel_Skin,               // Skinned
    ksPerPixel_Skin_NMDetail,      // Skinned + normal detail
    ksPerPixelAT_Skin,             // Skinned alpha tested
    ksWindScreen,                  // Windshield
    ksChrome,                      // Chrome/reflective
    ksGlass,                       // Glass
    ksHeadLight,                   // Headlights
    ksRearLight,                   // Taillights
    ksEmissive,                    // Emissive
    ksEmissiveAT,                  // Emissive alpha tested
    ksDirt,                        // Dirt overlay
    ksCrowd,                       // Crowd billboards
    ksTrees,                       // Tree billboards
    ksPerPixelMultiMap_B,          // Blended multi-map
    ksPerPixelMultiMap_NMDetail_B  // Blended normal detail
};

enum class ACTextureSlot {
    Diffuse        = 0,  // Main color
    Normal         = 1,  // Normal map
    Specular       = 2,  // Specular/roughness
    Detail         = 3,  // Detail texture
    DetailNormal   = 4,  // Detail normal
    Ambient        = 5,  // AO/ambient
    LightMap       = 6,  // Lightmap
    Damage         = 7,  // Damage mask
    Reflection     = 8,  // Reflection cubemap
    Emissive       = 9   // Emissive
};

// ============================================================================
// Material properties (matching AC material structure)
// ============================================================================

struct ACMaterialProperties {
    // Shader
    ACShaderType shaderType = ACShaderType::ksPerPixelMultiMap_NMDetail;
    QString shaderName;

    // Render states
    bool alphaBlending = false;
    bool alphaTesting  = false;
    float alphaRef     = 0.5f;
    bool depthTest     = true;
    bool depthWrite    = true;
    bool backfaceCull  = true;

    // Parameters
    float specularExp  = 16.0f;
    float specularMult = 1.0f;
    float detailUVMult = 1.0f;

    // Emissive
    QVector3D emissiveColor = QVector3D(0, 0, 0);
    float emissiveMult      = 0.0f;

    // Reflection
    float reflectivity = 0.5f;
    float fresnelBias  = 0.1f;
    float fresnelScale = 1.0f;
    float fresnelPower = 2.0f;

    // Texture slots
    QMap<ACTextureSlot, QString> textureSlots;

    // Custom shader params
    QMap<QString, QVector4D> customParams;

    // Validation
    bool isValid() const;
    QString validate() const;
};

// ============================================================================
// Texture (DDS support)
// ============================================================================

struct ACTexture {
    QString name;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t format = 0;  // DXGI_FORMAT
    uint32_t mipLevels = 1;
    QByteArray data;  // Raw DDS data

    // GPU handles (filled by renderer)
    void* gpuHandle = nullptr;
    uint64_t gpuHandle64 = 0;

    bool isValid() const { return !data.isEmpty() && width > 0 && height > 0; }
    uint32_t dataSize() const { return (uint32_t)data.size(); }
};

// ============================================================================
// Mesh (KN5 compatible)
// ============================================================================

enum class ACVertexAttribute {
    Position,
    Normal,
    Tangent,
    Bitangent,
    TexCoord0,
    TexCoord1,
    BoneWeights,
    BoneIndices,
    Color
};

struct ACVertexLayout {
    struct Attribute {
        ACVertexAttribute type;
        uint32_t offset = 0;
        uint32_t format = 0;  // DXGI_FORMAT
    };
    QVector<Attribute> attributes;
    uint32_t vertexSize = 0;

    bool has(ACVertexAttribute attr) const {
        for (const auto& a : attributes) if (a.type == attr) return true;
        return false;
    }
    uint32_t offsetOf(ACVertexAttribute attr) const {
        for (const auto& a : attributes) if (a.type == attr) return a.offset;
        return UINT32_MAX;
    }
};

struct ACMesh {
    QString name;
    uint32_t meshId = 0;

    // Raw buffers (KN5 format)
    QByteArray vertexData;
    QByteArray indexData;
    ACVertexLayout vertexLayout;

    // Parsed data
    QVector<QVector3D> positions;
    QVector<QVector3D> normals;
    QVector<QVector3D> tangents;
    QVector<QVector3D> bitangents;
    QVector<QVector2D> uv0;
    QVector<QVector2D> uv1;
    QVector<QVector4D> boneWeights;
    QVector<QVector4D> boneIndices;

    // Index buffer
    QVector<uint16_t> indices16;
    QVector<uint32_t> indices32;
    bool use32BitIndices = false;

    // Submeshes (per material)
    struct SubMesh {
        uint32_t startIndex = 0;
        uint32_t indexCount = 0;
        uint32_t materialId = 0;
        uint32_t baseVertex = 0;
    };
    QVector<SubMesh> subMeshes;

    // Bounds
    QVector3D boundsMin;
    QVector3D boundsMax;
    float boundsRadius = 0.0f;

    // Skinned
    bool isSkinned = false;
    QVector<uint32_t> boneRemap;

    // GPU resources
    void* vertexBuffer = nullptr;
    void* indexBuffer = nullptr;

    // Stats
    uint32_t vertexCount() const { return vertexLayout.vertexSize ? (uint32_t)vertexData.size() / vertexLayout.vertexSize : 0; }
    uint32_t triangleCount() const { return indices16.size() / 3 + indices32.size() / 3; }

    void decodeVertices();
    void encodeVertices();
    void computeBounds();
};

// ============================================================================
// Node (Scene graph)
// ============================================================================

struct ACNode {
    QString name;
    int parentIndex = -1;
    QVector<int> children;

    // Transform
    QMatrix4x4 localMatrix;
    QMatrix4x4 worldMatrix;

    // Mesh reference
    int meshIndex = -1;
    bool visible = true;
    bool castShadow = true;
    bool receiveShadow = true;

    // LOD
    QString lodGroup;  // "LOD_A", "LOD_B", etc.
    float lodDistance = 0.0f;

    // Properties
    QMap<QString, QString> properties;

    bool isLOD() const { return name.startsWith("LOD_"); }
    QString getLODLevel() const {
        if (name.startsWith("LOD_A")) return "A";
        if (name.startsWith("LOD_B")) return "B";
        if (name.startsWith("LOD_C")) return "C";
        if (name.startsWith("LOD_D")) return "D";
        return "";
    }
};

// ============================================================================
// Scene (Complete model)
// ============================================================================

struct ACScene {
    QString filePath;
    QString modelName;

    QVector<ACTexture> textures;
    QVector<ACMaterialProperties> materials;
    QVector<ACMesh> meshes;
    QVector<ACNode> nodes;

    // LOD groups
    struct LODGroup {
        QString name;
        float distance = 0.0f;
        QVector<int> meshIndices;
    };
    QVector<LODGroup> lodGroups;

    // Bones (for skinned meshes)
    struct Bone {
        QString name;
        int parentIndex = -1;
        QMatrix4x4 bindPose;  // world space
    };
    QVector<Bone> bones;
    QMatrix4x4 rootTransform;

    // Stats
    uint32_t totalTriangles() const {
        uint32_t sum = 0;
        for (const auto& m : meshes) sum += m.triangleCount();
        return sum;
    }
    uint32_t totalVertices() const {
        uint32_t sum = 0;
        for (const auto& m : meshes) sum += m.vertexCount();
        return sum;
    }

    bool isValid() const { return !meshes.isEmpty(); }
};

// ============================================================================
// Shader Manager
// ============================================================================

class ACShaderManager : public QObject {
    Q_OBJECT
public:
    explicit ACShaderManager(QObject* parent = nullptr);
    ~ACShaderManager();

    // Load all AC shaders
    bool loadShaders(const QString& shaderPath);

    // Get shader by type
    void* getShader(ACShaderType type) const;
    QString getShaderName(ACShaderType type) const;

    // Compile custom shader variant
    void* compileVariant(ACShaderType base, const QStringList& defines);

    // Shader parameter layout
    struct ShaderLayout {
        QMap<QString, int> uniformLocations;
        QMap<QString, int> textureSlots;
    };
    ShaderLayout getLayout(ACShaderType type) const;

signals:
    void shaderLoaded(ACShaderType type);
    void shaderError(const QString& message);

private:
    class Impl;
    Impl* d = nullptr;
};

// ============================================================================
// Texture Loader (DDS)
// ============================================================================

class ACTextureLoader : public QObject {
    Q_OBJECT
public:
    explicit ACTextureLoader(QObject* parent = nullptr);
    ~ACTextureLoader();

    // Load DDS file
    bool loadDDS(const QString& path, ACTexture& outTexture);
    bool loadDDSFromMemory(const QByteArray& data, ACTexture& outTexture);

    // Load from KN5 texture data
    bool loadFromKN5(const QByteArray& ddsData, const QString& name, ACTexture& outTexture);

    // Generate mipmaps
    void generateMipmaps(ACTexture& texture);

    // Convert format
    bool convertFormat(ACTexture& texture, uint32_t targetFormat);

    // Compress (BC1/BC3/BC7)
    bool compress(ACTexture& texture, uint32_t compressionFormat);

    // Async loading
    void loadAsync(const QString& path, std::function<void(ACTexture)> callback);

signals:
    void textureLoaded(const QString& name, const ACTexture& texture);
    void textureError(const QString& message);

private:
    class Impl;
    Impl* d = nullptr;
};

// ============================================================================
// Material System
// ============================================================================

class ACMaterial : public QObject {
    Q_OBJECT
public:
    explicit ACMaterial(QObject* parent = nullptr);
    ~ACMaterial();

    // Properties
    ACMaterialProperties properties;
    QString name;

    // Texture binding
    void setTexture(ACTextureSlot slot, ACTexture* texture);
    ACTexture* getTexture(ACTextureSlot slot) const;

    // Shader binding
    void bind(ACRenderer* renderer, const QMatrix4x4& viewProj, const QMatrix4x4& model);
    void unbind();

    // Parameter updates
    void setParam(const QString& name, const QVector4D& value);
    void setParam(const QString& name, float value);
    void setParam(const QString& name, const QVector3D& value);
    void setParam(const QString& name, const QMatrix4x4& value);

    // Validation
    bool validate(QString* error = nullptr) const;

signals:
    void propertiesChanged();
    void textureChanged(ACTextureSlot slot);

private:
    class Impl;
    Impl* d = nullptr;
};

// ============================================================================
// Model Loader (KN5)
// ============================================================================

class ACModelLoader : public QObject {
    Q_OBJECT
public:
    explicit ACModelLoader(QObject* parent = nullptr);
    ~ACModelLoader();

    // Load KN5 file
    bool loadKN5(const QString& filePath, ACScene& outScene);
    bool loadKN5FromMemory(const QByteArray& data, const QString& name, ACScene& outScene);

    // Save KN5 file
    bool saveKN5(const QString& filePath, const ACScene& scene);
    bool saveKN5ToMemory(const ACScene& scene, QByteArray& outData);

    // Load individual components
    bool loadTextures(const QString& folder, QVector<ACTexture>& outTextures);
    bool loadMaterials(const QString& iniPath, QVector<ACMaterialProperties>& outMaterials);

    // Validation
    QVector<QString> validateKN5(const ACScene& scene) const;

    // Options
    void setFlipUV(bool flip) { m_flipUV = flip; }
    void setGenerateTangents(bool gen) { m_genTangents = gen; }
    void setOptimizeMeshes(bool opt) { m_optimize = opt; }

signals:
    void progress(int percent, const QString& message);
    void loaded(const ACScene& scene);
    void error(const QString& message);

private:
    class Impl;
    Impl* d = nullptr;
    bool m_flipUV = false;
    bool m_genTangents = true;
    bool m_optimize = true;
};

// ============================================================================
// Renderer Interface
// ============================================================================

class ACRenderer : public QObject {
    Q_OBJECT
public:
    enum class Backend { Vulkan, OpenGL, DirectX11, Null };

    explicit ACRenderer(Backend backend = Backend::Null, QObject* parent = nullptr);
    ~ACRenderer();

    // Initialization
    bool initialize(void* windowHandle, uint32_t width, uint32_t height);
    void resize(uint32_t width, uint32_t height);
    void shutdown();

    // Resource creation
    void* createVertexBuffer(const void* data, uint32_t size);
    void* createIndexBuffer(const void* data, uint32_t size);
    void* createTexture(const ACTexture& texture);
    void* createUniformBuffer(uint32_t size);
    void* createShader(ACShaderType type, const QStringList& defines = {});

    // Resource update
    void updateBuffer(void* buffer, const void* data, uint32_t offset, uint32_t size);
    void updateTexture(void* texture, const ACTexture& data);

    // Resource destruction
    void destroyBuffer(void* buffer);
    void destroyTexture(void* texture);
    void destroyShader(void* shader);

    // Rendering
    void beginFrame();
    void endFrame();
    void renderScene(const ACScene& scene, const QMatrix4x4& view, const QMatrix4x4& proj,
                     const QVector3D& cameraPos, const QVector<QVector3D>& lights);

    // Debug
    void setWireframe(bool enabled);
    void setShowBounds(bool enabled);
    void setShowNormals(bool enabled);

    // Stats
    struct Stats {
        uint32_t drawCalls = 0;
        uint32_t triangles = 0;
        uint32_t vertices = 0;
        float gpuTimeMs = 0.0f;
    };
    Stats getStats() const;

signals:
    void frameRendered();
    void error(const QString& message);

private:
    class Impl;
    Impl* d = nullptr;
};

// ============================================================================
// Utility functions
// ============================================================================

// Convert AC shader type to string
QString acShaderTypeToString(ACShaderType type);

// Parse shader name to type
ACShaderType acStringToShaderType(const QString& name);

// Get default shader for material properties
ACShaderType acGetDefaultShader(const ACMaterialProperties& props);

// Validate material for AC compatibility
bool acValidateMaterial(const ACMaterialProperties& props, QString* error = nullptr);

// Get texture slot name
QString acTextureSlotToString(ACTextureSlot slot);

// Parse texture slot from string
ACTextureSlot acStringToTextureSlot(const QString& str);

// DDS format constants
namespace DDSFormat {
    constexpr uint32_t DXT1  = 0x31545844; // 'DXT1'
    constexpr uint32_t DXT3  = 0x33545844; // 'DXT3'
    constexpr uint32_t DXT5  = 0x35545844; // 'DXT5'
    constexpr uint32_t BC7   = 0x37434220; // 'BC7 '
    constexpr uint32_t RGBA8 = 28;         // DXGI_FORMAT_R8G8B8A8_UNORM
    constexpr uint32_t RGB8  = 27;         // DXGI_FORMAT_R8G8B8_UNORM
    constexpr uint32_t R16G16B16A16_FLOAT = 10; // DXGI_FORMAT_R16G16B16A16_FLOAT
}

// AC standard shader parameters
namespace ACShaderParams {
    constexpr const char* g_mWorldViewProj      = "g_mWorldViewProj";
    constexpr const char* g_mWorld              = "g_mWorld";
    constexpr const char* g_mWorldInvTranspose  = "g_mWorldInvTranspose";
    constexpr const char* g_vCameraPos          = "g_vCameraPos";
    constexpr const char* g_vLightDir           = "g_vLightDir";
    constexpr const char* g_vLightColor         = "g_vLightColor";
    constexpr const char* g_vAmbientColor       = "g_vAmbientColor";
    constexpr const char* g_fSpecularExp        = "g_fSpecularExp";
    constexpr const char* g_fSpecularMult       = "g_fSpecularMult";
    constexpr const char* g_fDetailUVMult       = "g_fDetailUVMult";
    constexpr const char* g_vEmissiveColor      = "g_vEmissiveColor";
    constexpr const char* g_fEmissiveMult       = "g_fEmissiveMult";
    constexpr const char* g_fReflectivity       = "g_fReflectivity";
    constexpr const char* g_fFresnelBias        = "g_fFresnelBias";
    constexpr const char* g_fFresnelScale       = "g_fFresnelScale";
    constexpr const char* g_fFresnelPower       = "g_fFresnelPower";
    constexpr const char* g_fAlphaRef           = "g_fAlphaRef";
    constexpr const char* g_vDiffuseColor       = "g_vDiffuseColor";
    constexpr const char* g_fAlphaTest          = "g_fAlphaTest";
    constexpr const char* g_vSpecularColor      = "g_vSpecularColor";
}

} // namespace graphics
} // namespace ac
} // namespace ks

// Meta-type registration
Q_DECLARE_METATYPE(ks::ac::graphics::ACShaderType)
Q_DECLARE_METATYPE(ks::ac::graphics::ACTextureSlot)
Q_DECLARE_METATYPE(ks::ac::graphics::ACMaterialProperties)