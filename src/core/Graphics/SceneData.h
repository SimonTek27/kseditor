#pragma once

#include <QString>
#include <QVector>
#include <QMap>
#include <QVariant>
#include <QVector3D>
#include <QVector4D>
#include <QMatrix4x4>
#include <QSharedPointer>
#include <QColor>

namespace Ks {

struct ShaderParam {
    QString name;
    QString description;
    QVariant defaultValue;
    float   rangeMin = 0.f;
    float   rangeMax = 1.f;
};

struct ShaderTexture {
    QString name;
    QString notes;
};

struct ShaderDef {
    QString      name;
    bool         isAlphaTested   = false;
    bool         isSkinned       = false;
    bool         isParticle      = false;
    bool         hasGeomShadow   = false;
    QString      psModel;
    QString      vsModel;
    QVector<ShaderParam>   params;
    QVector<ShaderTexture> textures;

    static QMap<QString, ShaderDef> loadAllFromDirectory(const QString& shaderHtmlDir);
};

struct Material {
    QString  name;
    QString  shaderName;
    QMap<QString, float>     floatParams;
    QMap<QString, QVector4D> vec4Params;
    QMap<QString, QString>   texturePaths;

    float ambient()   const { return floatParams.value("ksAmbient",  0.5f); }
    float diffuse()   const { return floatParams.value("ksDiffuse",  0.6f); }
    float specular()  const { return floatParams.value("ksSpecular", 0.3f); }
    float specularEx()const { return floatParams.value("ksSpecularEXP", 80.f); }
    float emissive()  const { return floatParams.value("ksEmissive", 0.f); }
    float alphaRef()  const { return floatParams.value("ksAlphaRef", 0.5f); }
};

struct Vertex {
    QVector3D position;
    QVector3D normal;
    QVector3D tangent;
    QVector2D uv0;
    QVector2D uv1;
    QVector4D boneWeights;
    QVector<int> boneIndices;
};

struct SubMesh {
    QString        materialName;
    QVector<Vertex> vertices;
    QVector<quint32> indices;
    QVector3D      boundsMin;
    QVector3D      boundsMax;
};

struct Mesh {
    QString          name;
    QVector<SubMesh> subMeshes;
    bool             castShadow   = true;
    bool             receiveShadow = true;
    bool             isVisible     = true;
    QString          layer;
};

struct SceneNode {
    using Ptr = QSharedPointer<SceneNode>;

    QString         name;
    QMatrix4x4      localTransform;
    QVector<Ptr>    children;
    SceneNode*      parent = nullptr;
    QSharedPointer<Mesh> mesh;

    QVector3D translation() const;
    QVector3D rotation()    const;
    QVector3D scale()       const;
    void setTranslation(const QVector3D&);
    void setRotation(const QVector3D&);
    void setScale(const QVector3D&);

    QMatrix4x4 worldTransform() const;
};

enum class ColliderType {
    Box, Sphere, Capsule, Mesh, ConvexHull
};

struct PhysicsCollider {
    ColliderType type     = ColliderType::Box;
    QVector3D    center;
    QVector3D    halfExtents = {0.5f, 0.5f, 0.5f};
    float        radius     = 0.5f;
    float        height     = 1.0f;
    float        friction   = 0.7f;
    float        restitution = 0.3f;
    bool         isTrigger  = false;
    QString      surfaceType;
};

struct PhysicsBody {
    bool               isStatic     = true;
    float              mass         = 0.f;
    float              linearDamp   = 0.05f;
    float              angularDamp  = 0.05f;
    QVector<PhysicsCollider> colliders;
};

enum class SoundTrigger {
    Always, Speed, RPM, Gear, Surface, Collision, Custom
};

struct SoundEmitter {
    QString      name;
    QString      bankFile;
    QString      eventPath;
    QVector3D    position;
    float        minDistance = 1.f;
    float        maxDistance = 100.f;
    float        volume      = 1.f;
    float        pitch       = 1.f;
    bool         loop        = false;
    bool         is3D        = true;
    SoundTrigger trigger     = SoundTrigger::Always;
    float        triggerMin  = 0.f;
    float        triggerMax  = 1.f;

    QMap<QString, QString> surfaceSounds;
};

struct Scene {
    QString                         name;
    QString                         filePath;
    SceneNode::Ptr                  root;
    QMap<QString, Material>         materials;
    QMap<QString, ShaderDef>        shaderDefs;
    QMap<QString, PhysicsBody>      physicsBodies;
    QVector<SoundEmitter>           soundEmitters;

    bool isDirty = false;

    void clear();
};

} // namespace Ks