#pragma once

#include <QString>
#include <QVector>
#include <QVector3D>
#include <QVector4D>
#include <QQuaternion>
#include <QMatrix4x4>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QStringList>
#include <QVariant>
#include <QByteArray>
#include <QSize>
#include <functional>
#include <memory>

#include "MeshData.h"

namespace ks {
namespace fileformat {

class SceneGraph;

struct MaterialData {
    QString name;
    QVector4D diffuseColor = {1, 1, 1, 1};
    QVector4D emissiveColor = {0, 0, 0, 1};
    float metallic = 0.0f;
    float roughness = 0.5f;
    float opacity = 1.0f;
    float alphaCutoff = 0.5f;
    bool doubleSided = false;
    std::string alphaMode = "OPAQUE";
    
    QString diffuseTexture;
    QString normalTexture;
    QString metallicRoughnessTexture;
    QString occlusionTexture;
    QString emissiveTexture;
    
    float clearcoat = 0.0f;
    float clearcoatRoughness = 0.0f;
    float transmission = 0.0f;
    float ior = 1.5f;
    float specular = 1.0f;
    QVector3D specularColor = {1, 1, 1};
    QVector3D sheenColor = {0, 0, 0};
    float sheenRoughness = 0.0f;
    float anisotropy = 0.0f;
    float anisotropyRotation = 0.0f;
    float thickness = 0.0f;
    float attenuationDistance = 0.0f;
    QVector3D attenuationColor = {1, 1, 1};
    float emissiveStrength = 1.0f;
};

struct AnimationData {
    QString name;
    struct Keyframe {
        float time = 0;
        QVariant value;
    };
    struct Curve {
        int targetNode = -1;
        QString targetPath;
        std::string interpolation = "LINEAR";
        QVector<Keyframe> keyframes;
    };
    QVector<Curve> curves;
};

struct SkinData {
    QString name;
    QVector<int> joints;
    int skeletonRoot = -1;
    QVector<QMatrix4x4> inverseBindMatrices;
};

struct GLTFData {
    struct Asset {
        QString copyright;
        QString generator;
        QString version = "2.0";
        QString minVersion = "2.0";
    } asset;
    
    int scene = -1;
    
    struct Scene {
        QString name;
        QVector<int> nodes;
    };
    QVector<Scene> scenes;
    
    struct Node {
        QString name;
        int mesh = -1;
        int skin = -1;
        int camera = -1;
        QVector<int> children;
        QVector3D translation = {0, 0, 0};
        QQuaternion rotation;
        QVector3D scale = {1, 1, 1};
        QMatrix4x4 matrix;
        bool hasMatrix = false;
    };
    QVector<Node> nodes;
    
    struct Primitive {
        enum Mode { TRIANGLES = 4 };
        int mode = TRIANGLES;
        int indices = -1;
        int material = -1;
        QMap<QString, int> attributes;
        struct Target {
            int POSITION = -1;
            int NORMAL = -1;
            int TANGENT = -1;
        };
        QVector<Target> targets;
    };
    
    struct Mesh {
        QString name;
        QVector<Primitive> primitives;
        QVector<float> weights;
    };
    QVector<Mesh> meshes;
    
    struct Material {
        QString name;
        QString alphaMode = "OPAQUE";
        float alphaCutoff = 0.5f;
        bool doubleSided = false;
        QVector4D baseColorFactor = {1, 1, 1, 1};
        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;
        int baseColorTexture = -1;
        int baseColorTexCoord = 0;
        int metallicRoughnessTexture = -1;
        int metallicRoughnessTexCoord = 0;
        int normalTexture = -1;
        int normalTexCoord = 0;
        float normalScale = 1.0f;
        int occlusionTexture = -1;
        int occlusionTexCoord = 0;
        float occlusionStrength = 1.0f;
        int emissiveTexture = -1;
        int emissiveTexCoord = 0;
        QVector3D emissiveFactor = {0, 0, 0};
        float clearcoatFactor = 0.0f;
        float clearcoatRoughnessFactor = 0.0f;
        int clearcoatTexture = -1;
        int clearcoatRoughnessTexture = -1;
        int clearcoatNormalTexture = -1;
        float transmissionFactor = 0.0f;
        int transmissionTexture = -1;
        float ior = 1.5f;
        float specularFactor = 1.0f;
        QVector3D specularColorFactor = {1, 1, 1};
        int specularTexture = -1;
        int specularColorTexture = -1;
        QVector3D sheenColorFactor = {0, 0, 0};
        float sheenRoughnessFactor = 0.0f;
        int sheenColorTexture = -1;
        int sheenRoughnessTexture = -1;
        float anisotropyStrength = 0.0f;
        float anisotropyRotation = 0.0f;
        int anisotropyTexture = -1;
        float thicknessFactor = 0.0f;
        float attenuationDistance = 0.0f;
        QVector3D attenuationColor = {1, 1, 1};
        int thicknessTexture = -1;
        float emissiveStrength = 1.0f;
    };
    QVector<Material> materials;
    
    struct Accessor {
        int bufferView = -1;
        int byteOffset = 0;
        int componentType = 5126;
        int count = 0;
        QString type = "VEC3";
        bool normalized = false;
        QVector<double> min;
        QVector<double> max;
    };
    QVector<Accessor> accessors;
    
    struct BufferView {
        int buffer = 0;
        int byteOffset = 0;
        int byteLength = 0;
        int byteStride = 0;
        int target = 0;
    };
    QVector<BufferView> bufferViews;
    
    struct Buffer {
        QString uri;
        int byteLength = 0;
        QByteArray data;
    };
    QVector<Buffer> buffers;
    
    struct Texture {
        int sampler = -1;
        int source = -1;
    };
    QVector<Texture> textures;
    
    struct Image {
        QString uri;
        QString mimeType;
        int bufferView = -1;
        QString name;
    };
    QVector<Image> images;
    
    struct Sampler {
        int magFilter = 9729;
        int minFilter = 9987;
        int wrapS = 10497;
        int wrapT = 10497;
    };
    QVector<Sampler> samplers;
    
    struct Skin {
        QString name;
        int inverseBindMatrices = -1;
        int skeleton = -1;
        QVector<int> joints;
    };
    QVector<Skin> skins;
    
    struct Animation {
        QString name;
        struct Channel {
            int targetNode = -1;
            QString targetPath;
            int sampler = -1;
        };
        QVector<Channel> channels;
        struct AnimSampler {
            int input = -1;
            QString interpolation = "LINEAR";
            int output = -1;
        };
        QVector<AnimSampler> samplers;
    };
    QVector<Animation> animations;
    
    struct Camera {
        QString name;
        QString type = "perspective";
        struct Perspective {
            float aspectRatio = 1.0f;
            float yfov = 0.785f;
            float znear = 0.1f;
            float zfar = 1000.0f;
        } perspective;
        struct Orthographic {
            float xmag = 1.0f;
            float ymag = 1.0f;
            float znear = 0.1f;
            float zfar = 1000.0f;
        } orthographic;
    };
    QVector<Camera> cameras;
    
    QStringList extensionsUsed;
    QStringList extensionsRequired;
    
    void clear() {
        *this = GLTFData();
    }
};

class GLTFParser {
public:
    GLTFParser();
    
    bool loadFromFile(const QString& path);
    bool parseBinary(const QByteArray& data);
    bool parseJson(const QJsonObject& json);
    bool loadExternalBuffer(size_t bufferIndex);
    
    const GLTFData& gltf() const;
    
    bool getMeshData(int meshIndex, MeshData& outData) const;
    bool getMaterialData(int materialIndex, MaterialData& outMat) const;
    QString getTexturePath(int textureIndex) const;
    bool getAnimationData(int animIndex, AnimationData& outAnim) const;
    bool getSkinData(int skinIndex, SkinData& outSkin) const;
    
    QString lastError() const;

private:
    QString m_filePath;
    QString m_baseDir;
    bool m_isBinary = false;
    QByteArray m_binaryBuffer;
    GLTFData m_gltf;
    QString m_lastError;
};

class GLTFWriter {
public:
    bool writeToFile(const QString& path, const MeshData& mesh, const MaterialData* material = nullptr);
    bool writeToFile(const QString& path, const SceneGraph& scene);
    bool writeGLTF(const QString& path, const GLTFData& gltf);
    bool writeGLB(const QString& path, const GLTFData& gltf);
};

} // namespace fileformat
} // namespace ks
