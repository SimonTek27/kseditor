#pragma once

#include <QString>
#include <QStringList>
#include <QObject>
#include <QVector>
#include <QMap>
#include <QVariant>
#include <QJsonObject>
#include <QQuaternion>
#include <memory>
#include <string>

#include "MeshData.h"

namespace ks {

struct MaterialData {
    QString name;
    QVector4D baseColorFactor = {1, 1, 1, 1};
    float metallic = 0.0f;
    float roughness = 0.5f;
    QString baseColorTexture;
    QString metallicRoughnessTexture;
    QString normalTexture;
    QString emissiveTexture;
    QVector3D emissiveFactor = {0, 0, 0};
    float alphaCutoff = 0.5f;
    bool doubleSided = false;
};

// ─── USD (Universal Scene Description) Support ──────────────────────────────

struct USDPrim {
    std::string path;
    std::string typeName;          // "Mesh", "Xform", "Camera", "Light", "Material"
    QMap<std::string, QVariant> attributes;
    QVector<std::string> children;
    std::string parent;
};

struct USDMesh {
    std::string primPath;
    QVector<QVector3D> points;
    QVector<QVector3D> normals;
    QVector<QVector2D> uvs;
    QVector<QVector4D> colors;
    QVector<int> faceVertexCounts;
    QVector<int> faceVertexIndices;
    QVector<int> faceMaterialIndices;
    std::string subdivisionScheme;  // "catmullClark", "loop", "none"
    float subdivisionLevel = 0;
};

struct USDMaterial {
    std::string primPath;
    QMap<std::string, QVariant> inputs;
    std::string shaderId;
    // PBR metallic/roughness
    QVector3D diffuseColor = {1,1,1};
    float metallic = 0;
    float roughness = 0.5f;
    QVector3D specularColor = {1,1,1};
    float clearcoat = 0;
    float clearcoatRoughness = 0;
    float opacity = 1;
    float ior = 1.5f;
    // Textures
    std::string diffuseTexture;
    std::string normalTexture;
    std::string metallicRoughnessTexture;
    std::string emissiveTexture;
    std::string occlusionTexture;
};

struct USDAnimation {
    std::string primPath;
    std::string attributeName;
    QVector<double> timeSamples;
    QVector<QVariant> values;
    std::string interpolation;  // "linear", "step", "cubic"
};

struct USDStage {
    std::string rootLayer;
    QVector<USDPrim> prims;
    QVector<USDMesh> meshes;
    QVector<USDMaterial> materials;
    QVector<USDAnimation> animations;
    double startTimeCode = 1.0;
    double endTimeCode = 100.0;
    double timeCodesPerSecond = 24.0;
    QMap<std::string, std::string> layerStack;
};

class USDParser {
public:
    static bool read(const QString& filePath, USDStage& stage, QString* error = nullptr);
    static bool write(const QString& filePath, const USDStage& stage, QString* error = nullptr);
    
    // Convert to/from internal MeshData
    static bool usdMeshToMeshData(const USDMesh& mesh, fileformat::MeshData& outData);
    static bool meshDataToUsdMesh(const fileformat::MeshData& data, USDMesh& outMesh);
    
    // Material conversion
    static bool usdMaterialToPBR(const USDMaterial& mat, fileformat::MaterialData& outMat);
    static bool pbrToUsdMaterial(const fileformat::MaterialData& mat, USDMaterial& outMat);
    
    // Animation sampling
    static QVariant sampleAnimation(const USDAnimation& anim, double time);
    
    // Scene graph traversal
    static QVector<std::string> findPrimsByType(const USDStage& stage, const std::string& typeName);
    static const USDPrim* findPrim(const USDStage& stage, const std::string& path);
    static QVector<std::string> getPrimChildren(const USDStage& stage, const std::string& path);
    
private:
    static bool parseUsda(const QString& filePath, USDStage& stage, QString* error);
    static bool parseUsdc(const QString& filePath, USDStage& stage, QString* error);
    static bool writeUsda(const QString& filePath, const USDStage& stage, QString* error);
    static bool writeUsdc(const QString& filePath, const USDStage& stage, QString* error);
};

// ─── Alembic Support ────────────────────────────────────────────────────────

struct AlembicObject {
    std::string fullName;
    std::string typeName;  // "polymesh", "curves", "points", "camera", "xform"
    QMap<std::string, QVariant> properties;
    QVector<std::string> children;
    std::string parent;
};

struct AlembicMesh {
    std::string objectPath;
    // Per-frame data
    struct Sample {
        double time;
        QVector<QVector3D> positions;
        QVector<QVector3D> velocities;
        QVector<QVector3D> normals;
        QVector<QVector2D> uvs;
        QVector<int> faceCounts;
        QVector<int> faceIndices;
        QVector<int> materialIds;
    };
    QVector<Sample> samples;
    std::string subdivisionScheme;
};

struct AlembicCamera {
    std::string objectPath;
    struct Sample {
        double time;
        float fov;
        float focusDistance;
        float aperture;
        QVector3D position;
        QQuaternion rotation;
    };
    QVector<Sample> samples;
};

struct AlembicArchive {
    std::string filePath;
    double startTime = 1.0;
    double endTime = 100.0;
    double timeSamplingRate = 24.0;
    QVector<AlembicObject> objects;
    QVector<AlembicMesh> meshes;
    QVector<AlembicCamera> cameras;
    QMap<std::string, QVariant> metadata;
};

class AlembicParser {
public:
    static bool read(const QString& filePath, AlembicArchive& archive, QString* error = nullptr);
    static bool write(const QString& filePath, const AlembicArchive& archive, QString* error = nullptr);
    
    // Convert to/from internal formats
    static bool alembicMeshToMeshData(const AlembicMesh& mesh, int frameIndex, fileformat::MeshData& outData);
    static bool meshDataToAlembicMesh(const fileformat::MeshData& data, AlembicMesh& outMesh);
    
    // Sample at arbitrary time
    static bool sampleMeshAtTime(const AlembicMesh& mesh, double time, fileformat::MeshData& outData);
    static bool sampleCameraAtTime(const AlembicCamera& cam, double time, 
                                   QVector3D& pos, QQuaternion& rot, float& fov);
    
    // Time sampling
    static QVector<double> getSampleTimes(const AlembicArchive& archive);
    static int findSampleIndex(const AlembicArchive& archive, double time);
    static double getTimeForFrame(const AlembicArchive& archive, int frame);
    
private:
    static bool parseAbc(const QString& filePath, AlembicArchive& archive, QString* error);
    static bool writeAbc(const QString& filePath, const AlembicArchive& archive, QString* error);
};

// ─── Enhanced glTF 2.0 Support ──────────────────────────────────────────────

struct GLTFTextureInfo {
    int index = -1;
    int texCoord = 0;
    std::map<std::string, QVariant> extensions;
};

struct GLTFNormalTextureInfo : GLTFTextureInfo {
    float scale = 1.0f;
};

struct GLTFOcclusionTextureInfo : GLTFTextureInfo {
    float strength = 1.0f;
};

struct GLTFPBRMetallicRoughness {
    QVector4D baseColorFactor = {1,1,1,1};
    GLTFTextureInfo baseColorTexture;
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    GLTFTextureInfo metallicRoughnessTexture;
};

struct GLTFMaterial {
    std::string name;
    std::string alphaMode = "OPAQUE";  // OPAQUE, MASK, BLEND
    float alphaCutoff = 0.5f;
    bool doubleSided = false;
    
    // PBR Metallic/Roughness
    GLTFPBRMetallicRoughness pbrMetallicRoughness;
    
    // Normal
    GLTFNormalTextureInfo normalTexture;
    
    // Occlusion
    GLTFOcclusionTextureInfo occlusionTexture;
    
    // Emissive
    QVector3D emissiveFactor = {0,0,0};
    GLTFTextureInfo emissiveTexture;
    
    // Extensions
    std::map<std::string, QVariant> extensions;
    QJsonObject extras;
};

struct GLTFMesh {
    std::string name;
    struct Primitive {
        std::map<std::string, int> attributes;  // "POSITION" -> accessor index
        int indices = -1;
        int material = -1;
        int mode = 4;  // TRIANGLES
        std::vector<std::map<std::string, int>> targets;  // morph targets
        std::map<std::string, QVariant> extensions;
        QJsonObject extras;
    };
    QVector<Primitive> primitives;
    QVector<float> weights;  // morph target weights
    std::map<std::string, QVariant> extensions;
    QJsonObject extras;
};

struct GLTFSkin {
    std::string name;
    std::vector<int> joints;  // node indices
    std::string inverseBindMatrices;  // accessor index
    std::map<std::string, QVariant> extensions;
    QJsonObject extras;
};

struct GLTFNode {
    std::string name;
    int skin = -1;
    int mesh = -1;
    int camera = -1;
    QVector<int> children;
    QVector3D translation = {0,0,0};
    QQuaternion rotation = QQuaternion();
    QVector3D scale = {1,1,1};
    QVector<float> weights;  // morph target weights
    QMatrix4x4 matrix;  // TRS matrix (if provided, overrides TRS)
    std::map<std::string, QVariant> extensions;
    QJsonObject extras;
};

struct GLTFCamera {
    std::string name;
    std::string type = "perspective";  // perspective, orthographic
    struct Perspective {
        float aspectRatio = 1.0f;
        float yfov = 45.0f;
        float znear = 0.1f;
        float zfar = 100.0f;
    } perspective;
    struct Orthographic {
        float xmag = 1.0f;
        float ymag = 1.0f;
        float znear = 0.1f;
        float zfar = 100.0f;
    } orthographic;
    std::map<std::string, QVariant> extensions;
    QJsonObject extras;
};

struct GLTFAnimation {
    std::string name;
    struct Channel {
        int sampler;
        struct Target {
            int node;
            std::string path;  // translation, rotation, scale, weights
        } target;
    };
    struct Sampler {
        int input;  // accessor index for times
        std::string interpolation = "LINEAR";  // LINEAR, STEP, CUBICSPLINE
        int output;  // accessor index for values
    };
    QVector<Channel> channels;
    QVector<Sampler> samplers;
    std::map<std::string, QVariant> extensions;
    QJsonObject extras;
};

struct GLTFScene {
    std::string name;
    QVector<int> nodes;
    std::map<std::string, QVariant> extensions;
    QJsonObject extras;
};

struct GLTFAsset {
    std::string copyright;
    std::string generator;
    std::string version = "2.0";
    std::string minVersion = "2.0";
    std::map<std::string, QVariant> extensions;
    QJsonObject extras;
};

struct GLTFDocument {
    GLTFAsset asset;
    QVector<GLTFScene> scenes;
    int scene = 0;
    QVector<GLTFNode> nodes;
    QVector<GLTFMesh> meshes;
    QVector<GLTFMaterial> materials;
    QVector<GLTFSkin> skins;
    QVector<GLTFCamera> cameras;
    QVector<GLTFAnimation> animations;
    
    // Buffers/Views/Accessors
    struct Buffer {
        std::string uri;
        size_t byteLength = 0;
        QVector<uint8_t> data;
    };
    struct BufferView {
        int buffer = 0;
        size_t byteOffset = 0;
        size_t byteLength = 0;
        size_t byteStride = 0;
        int target = 0;  // ARRAY_BUFFER, ELEMENT_ARRAY_BUFFER
    };
    struct Accessor {
        int bufferView = -1;
        size_t byteOffset = 0;
        std::string type;  // SCALAR, VEC2, VEC3, VEC4, MAT2, MAT3, MAT4
        int componentType = 5126;  // 5120=BYTE, 5121=UBYTE, 5122=SHORT, 5123=USHORT, 5125=UINT, 5126=FLOAT
        bool normalized = false;
        size_t count = 0;
        QVector<float> min;  // for accessors
        QVector<float> max;
        std::string sparse;
    };
    
    QVector<Buffer> buffers;
    QVector<BufferView> bufferViews;
    QVector<Accessor> accessors;
    
    // Images/Textures/Samplers
    struct Image {
        std::string uri;
        std::string mimeType;
        std::string bufferView;
        std::string name;
    };
    struct Texture {
        int sampler = -1;
        int source = -1;
        std::string name;
    };
    struct Sampler {
        int magFilter = 9729;  // LINEAR
        int minFilter = 9987;  // LINEAR_MIPMAP_LINEAR
        int wrapS = 10497;  // REPEAT
        int wrapT = 10497;  // REPEAT
        std::string name;
    };
    
    QVector<Image> images;
    QVector<Texture> textures;
    QVector<Sampler> samplers;
    
    // Extensions used/required
    QVector<std::string> extensionsUsed;
    QVector<std::string> extensionsRequired;
    
    std::map<std::string, QVariant> extensions;
    QJsonObject extras;
};

// ─── GLTF 2.0 Parser/Writer ─────────────────────────────────────────────────

class GLTF2Parser {
public:
    static bool read(const QString& filePath, GLTFDocument& doc, QString* error = nullptr);
    static bool write(const QString& filePath, const GLTFDocument& doc, bool binary = true, QString* error = nullptr);
    
    // Convert to/from internal formats
    static bool gltfToMeshData(const GLTFDocument& doc, int meshIndex, fileformat::MeshData& outData);
    static bool meshDataToGltf(const fileformat::MeshData& data, GLTFDocument& doc, int& meshIndex);
    
    // Material conversion
    static bool gltfMaterialToPBR(const GLTFMaterial& mat, fileformat::MaterialData& outMat);
    static bool pbrToGltfMaterial(const fileformat::MaterialData& mat, GLTFMaterial& outMat);
    
    // Animation
    static bool sampleAnimation(const GLTFDocument& doc, int animIndex, double time, 
                                QMap<int, QMatrix4x4>& nodeTransforms);
    
    // Scene hierarchy
    static QVector<int> getRootNodes(const GLTFDocument& doc, int sceneIndex = 0);
    static QMatrix4x4 computeNodeMatrix(const GLTFDocument& doc, int nodeIndex);
    static QVector<QMatrix4x4> computeGlobalTransforms(const GLTFDocument& doc, int sceneIndex = 0);
    
    // Skinning
    static bool extractSkinData(const GLTFDocument& doc, int skinIndex,
                                QVector<QMatrix4x4>& inverseBindMatrices,
                                QVector<int>& jointIndices);
    
    // Validation
    static bool validate(const GLTFDocument& doc, QStringList* errors = nullptr);
    static QVector<std::string> getUsedExtensions(const GLTFDocument& doc);
    
private:
    static bool parseJson(const QByteArray& json, GLTFDocument& doc, QString* error);
    static bool parseBinary(const QByteArray& bin, GLTFDocument& doc);
    static QByteArray serializeJson(const GLTFDocument& doc);
    static QByteArray serializeBinary(const GLTFDocument& doc);
    
    static bool loadBuffers(const QString& baseDir, GLTFDocument& doc, QString* error);
    static bool saveBuffers(const QString& baseDir, const GLTFDocument& doc, bool binary, QString* error);
    
    // Accessor data extraction
    static QVector<float> getAccessorDataFloat(const GLTFDocument& doc, int accessorIndex);
    static QVector<uint32_t> getAccessorDataUInt(const GLTFDocument& doc, int accessorIndex);
    static QVector<uint16_t> getAccessorDataUShort(const GLTFDocument& doc, int accessorIndex);
    static QVector<uint8_t> getAccessorDataUByte(const GLTFDocument& doc, int accessorIndex);
    
    // Interpolation
    static float lerp(float a, float b, float t);
    static QVector3D lerp(const QVector3D& a, const QVector3D& b, float t);
    static QQuaternion slerp(const QQuaternion& a, const QQuaternion& b, float t);
    
    // Cubic spline interpolation
    static float cubicSpline(const QVector<float>& times, const QVector<float>& values, float t);
    static QVector3D cubicSpline(const QVector<float>& times, const QVector<QVector3D>& values, float t);
    static QQuaternion cubicSpline(const QVector<float>& times, const QVector<QQuaternion>& values, float t);
};

// ─── Format Validation & Schema ─────────────────────────────────────────────

struct FormatValidationResult {
    bool valid = true;
    QStringList errors;
    QStringList warnings;
    QStringList infos;
    QMap<QString, QVariant> metadata;
};

class FormatValidator {
public:
    // Validate file against format specification
    static FormatValidationResult validateKN5(const QString& filePath);
    static FormatValidationResult validateFBX(const QString& filePath);
    static FormatValidationResult validateGLB(const QString& filePath);
    static FormatValidationResult validateUSD(const QString& filePath);
    static FormatValidationResult validateAlembic(const QString& filePath);
    static FormatValidationResult validateOBJ(const QString& filePath);
    static FormatValidationResult validateSTL(const QString& filePath);
    static FormatValidationResult validateCOLLADA(const QString& filePath);
    
    // Generic validation
    static FormatValidationResult validate(const QString& filePath, const QString& format);
    
    // Schema-based validation
    struct SchemaRule {
        QString field;
        QString type;  // "string", "int", "float", "bool", "array", "object"
        bool required = false;
        QVariant minValue;
        QVariant maxValue;
        QStringList enumValues;
        QString pattern;  // regex
    };
    
    static FormatValidationResult validateSchema(const QJsonObject& data, 
                                                  const QVector<SchemaRule>& rules);
    
    // Round-trip validation
    static FormatValidationResult validateRoundTrip(const QString& inputPath, 
                                                     const QString& format);
};

} // namespace ks