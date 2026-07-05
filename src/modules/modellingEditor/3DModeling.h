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
#include <QPair>
#include <QMatrix4x4>
#include <QJsonObject>
#include <QJsonDocument>
#include <QImage>
#include <QQuaternion>

#include "3DModeling_io.h"
#include "3DModeling_utils.h"

#include "core/Math/MathCore.h"

namespace ks {

// ============================================================================
// ModelerContext - Unified context for Car/Track/Character editors
// ============================================================================
class ModelerContext : public QObject
{
    Q_OBJECT
public:
    static ModelerContext* instance();

    enum EditorType { TypeCar, TypeTrack, TypeCharacter };
    enum EditMode { ModeSelect, ModeEdit, ModePaint, ModeAnimate };

    void setEditorType(EditorType type) { m_editorType = type; emit editorTypeChanged(type); }
    EditorType editorType() const { return m_editorType; }

    void setEditMode(EditMode mode) { m_editMode = mode; emit editModeChanged(mode); }
    EditMode editMode() const { return m_editMode; }

    void setCurrentTool(const QString& tool) { m_currentTool = tool; emit toolChanged(tool); }
    QString currentTool() const { return m_currentTool; }

    void setActiveObject(const QString& id) { m_activeObject = id; emit activeObjectChanged(id); }
    QString activeObject() const { return m_activeObject; }

    QStringList getToolsForType(EditorType type) const;
    QStringList getToolsForMode(EditMode mode) const;
    bool isToolValid(const QString& tool) const;

signals:
    void editorTypeChanged(EditorType type);
    void editModeChanged(EditMode mode);
    void toolChanged(const QString& tool);
    void activeObjectChanged(const QString& id);

private:
    explicit ModelerContext(QObject* parent = nullptr);
    static ModelerContext* s_instance;

    EditorType m_editorType = TypeCar;
    EditMode m_editMode = ModeSelect;
    QString m_currentTool;
    QString m_activeObject;

    QMap<EditorType, QStringList> m_toolsByType;
    QMap<EditMode, QStringList> m_toolsByMode;
};

// ============================================================================
// Geometry3D - Core 3D geometry classes
// ============================================================================
namespace geometry {

class Mesh3D : public QObject
{
    Q_OBJECT
public:
    explicit Mesh3D(QObject* parent = nullptr) : QObject(parent) {}
    ~Mesh3D() {}

    void setVertices(const QVector<QVector3D>& verts) { m_vertices = verts; }
    QVector<QVector3D> vertices() const { return m_vertices; }

    void setNormals(const QVector<QVector3D>& norms) { m_normals = norms; }
    QVector<QVector3D> normals() const { return m_normals; }

    void setIndices(const QVector<quint32>& idx) { m_indices = idx; }
    QVector<quint32> indices() const { return m_indices; }

    void setUVs(const QVector<QVector2D>& uvs) { m_uvs = uvs; }
    QVector<QVector2D> uvs() const { return m_uvs; }

    void setMaterialId(const QString& id) { m_materialId = id; }
    QString materialId() const { return m_materialId; }

    void computeNormals();
    void subdivide(int levels);
    void triangulate();

    QVector<float> toFloatArray() const;

private:
    QVector<QVector3D> m_vertices;
    QVector<QVector3D> m_normals;
    QVector<quint32> m_indices;
    QVector<QVector2D> m_uvs;
    QString m_materialId;
};

class Material3D : public QObject
{
    Q_OBJECT
public:
    explicit Material3D(QObject* parent = nullptr) : QObject(parent) {}
    ~Material3D() {}

    void setName(const QString& name) { m_name = name; }
    QString name() const { return m_name; }

    void setDiffuse(const QVector3D& color) { m_diffuse = color; }
    QVector3D diffuse() const { return m_diffuse; }

    void setSpecular(const QVector3D& color) { m_specular = color; }
    QVector3D specular() const { return m_specular; }

    void setAmbient(const QVector3D& color) { m_ambient = color; }
    QVector3D ambient() const { return m_ambient; }

    void setEmissive(const QVector3D& color) { m_emissive = color; }
    QVector3D emissive() const { return m_emissive; }

    void setOpacity(float opacity) { m_opacity = qBound(0.0f, opacity, 1.0f); }
    float opacity() const { return m_opacity; }

    void setRoughness(float roughness) { m_roughness = qBound(0.0f, roughness, 1.0f); }
    float roughness() const { return m_roughness; }

    void setMetallic(float metallic) { m_metallic = qBound(0.0f, metallic, 1.0f); }
    float metallic() const { return m_metallic; }

    void setTexture(const QString& map, const QString& path) { m_textures[map] = path; }
    QString texture(const QString& map) const { return m_textures.value(map); }

    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);

 signals:
    void changed();

private:
    QString m_name;
    QVector3D m_diffuse = QVector3D(0.8f, 0.8f, 0.8f);
    QVector3D m_specular = QVector3D(1.0f, 1.0f, 1.0f);
    QVector3D m_ambient = QVector3D(0.2f, 0.2f, 0.2f);
    QVector3D m_emissive = QVector3D(0, 0, 0);
    float m_opacity = 1.0f;
    float m_roughness = 0.5f;
    float m_metallic = 0.0f;
    QMap<QString, QString> m_textures;
};

class Scene3D : public QObject
{
    Q_OBJECT
public:
    explicit Scene3D(QObject* parent = nullptr) : QObject(parent) {}
    ~Scene3D() {}

    struct Object3D {
        QString id;
        QString name;
        Mesh3D* mesh = nullptr;
        Material3D* material = nullptr;
        QMatrix4x4 transform;
        bool visible = true;
        bool selected = false;
    };

    QString addObject(const QString& name, Mesh3D* mesh);
    void removeObject(const QString& objId);
    Object3D* getObject(const QString& objId) const;
    QVector<Object3D*> allObjects() const { return m_objects.values(); }

    void setObjectTransform(const QString& objId, const QMatrix4x4& matrix);
    void setObjectPosition(const QString& objId, const QVector3D& pos);
    void setObjectRotation(const QString& objId, const QVector3D& rot);
    void setObjectScale(const QString& objId, const QVector3D& scale);

    void selectObject(const QString& objId, bool select);

    void setBackgroundColor(const QVector3D& color) { m_background = color; }
    QVector3D backgroundColor() const { return m_background; }

 signals:
    void objectAdded(const QString& objId);
    void objectRemoved(const QString& objId);
    void objectModified(const QString& objId);
    void selectionChanged();

private:
    QMap<QString, Object3D*> m_objects;
    QVector3D m_background = QVector3D(0.1f, 0.1f, 0.1f);
};

} // namespace geometry

// ============================================================================
// Modeling3D - 3D modeling operations
// ============================================================================
namespace geometry {

class Modeling3D : public QObject
{
    Q_OBJECT
public:
    explicit Modeling3D(QObject* parent = nullptr) : QObject(parent) {}
    ~Modeling3D() {}

    enum PrimitiveType { Cube, Sphere, Cylinder, Cone, Torus, Plane, Circle };

    Mesh3D* createPrimitive(PrimitiveType type);

    Mesh3D* createCube(float width = 1.0f, float height = 1.0f, float depth = 1.0f);
    Mesh3D* createSphere(float radius = 1.0f, int segments = 32, int rings = 16);
    Mesh3D* createCylinder(float radius = 1.0f, float height = 2.0f, int segments = 32);
    Mesh3D* createCone(float radius = 1.0f, float height = 2.0f, int segments = 32);
    Mesh3D* createTorus(float majorRadius = 1.0f, float minorRadius = 0.3f, int majorSegs = 32, int minorSegs = 16);
    Mesh3D* createPlane(float width = 2.0f, float height = 2.0f, int subdivisions = 1);
    Mesh3D* createCircle(float radius = 1.0f, int segments = 32);

    Mesh3D* createFromHeightmap(const QImage& image, float heightScale = 1.0f);

    void extrude(Mesh3D* mesh, const QVector3D& direction, float distance);
    void bevel(Mesh3D* mesh, float distance, int segments = 1);
    void inset(Mesh3D* mesh, float distance);

    void subdivide(Mesh3D* mesh, int levels);
    void decimate(Mesh3D* mesh, float ratio);
    void triangulate(Mesh3D* mesh);

    void mirror(Mesh3D* mesh, const QVector3D& axis, float pivot = 0.0f);
    void array(Mesh3D* mesh, int count, const QVector3D& offset);
    void screw(Mesh3D* mesh, int steps, float angle, float height);

    enum UVProjection { Planar, Cylindrical, Spherical, Cubic };
    void generateUVs(Mesh3D* mesh, UVProjection projection);

 signals:
    void meshCreated(Mesh3D* mesh);
    void meshModified(Mesh3D* mesh);
};

class Skeleton3D : public QObject
{
    Q_OBJECT
public:
    explicit Skeleton3D(QObject* parent = nullptr) : QObject(parent) {}
    ~Skeleton3D() {}

    struct Bone {
        QString id;
        QString name;
        QString parentId;
        QVector3D head;
        QVector3D tail;
        QQuaternion rotation;
        float length;
        QMatrix4x4 worldMatrix;
    };

    QMatrix4x4 getBoneWorldMatrix(const QString& boneId) const;

    QString addBone(const QString& name, const QString& parentId = QString());
    void removeBone(const QString& boneId);
    Bone getBone(const QString& boneId) const;

    void setBonePosition(const QString& boneId, const QVector3D& pos);
    void setBoneRotation(const QString& boneId, const QQuaternion& rot);

    void calculateFK();
    void calculateIK(const QString& targetBoneId, const QVector3D& targetPos);

    QVector<Bone> allBones() const { return m_bones.values(); }

 signals:
    void boneAdded(const QString& boneId);
    void boneRemoved(const QString& boneId);
    void fkUpdated();

private:
    QMap<QString, Bone> m_bones;
};

} // namespace geometry

// ============================================================================
// Rendering3D - 3D rendering classes
// ============================================================================
namespace rendering {

class Shader3D : public QObject
{
    Q_OBJECT
public:
    explicit Shader3D(QObject* parent = nullptr) : QObject(parent) {}
    ~Shader3D() {}

    enum ShaderType { PBR, Emission, Transparent, Custom };

    void setType(ShaderType type) { m_type = type; }
    ShaderType type() const { return m_type; }

    struct BSDF {
        float baseColor[3] = {0.8f, 0.8f, 0.8f};
        float metallic = 0.0f;
        float roughness = 0.5f;
        float specular = 0.5f;
        float alpha = 1.0f;
        float ior = 1.45f;
    };

    void setBSDF(const BSDF& bsdf) { m_bsdf = bsdf; }
    BSDF bsdf() const { return m_bsdf; }

    void addNode(const QString& nodeId, const QString& nodeType);
    void removeNode(const QString& nodeId);
    void connectNodes(const QString& fromNode, const QString& toNode, const QString& fromSocket, const QString& toSocket);
    void disconnectNodes(const QString& fromNode, const QString& toNode, const QString& fromSocket, const QString& toSocket);

    QString compile() const;

 signals:
    void shaderModified();

private:
    struct Connection {
        QString fromNode;
        QString toNode;
        QString fromSocket;
        QString toSocket;
    };

    ShaderType m_type = PBR;
    BSDF m_bsdf;
    QMap<QString, QString> m_nodes;
    QVector<Connection> m_connections;
};

class Light3D : public QObject
{
    Q_OBJECT
public:
    explicit Light3D(QObject* parent = nullptr) : QObject(parent) {}
    ~Light3D() {}

    enum LightType { Point, Spot, Sun, Area };

    void setType(LightType type) { m_type = type; }
    LightType type() const { return m_type; }

    void setPosition(const QVector3D& pos) { m_position = pos; }
    QVector3D position() const { return m_position; }

    void setDirection(const QVector3D& dir) { m_direction = dir.normalized(); }
    QVector3D direction() const { return m_direction; }

    void setColor(const QVector3D& color) { m_color = color; }
    QVector3D color() const { return m_color; }

    void setIntensity(float intensity) { m_intensity = intensity; }
    float intensity() const { return m_intensity; }

    void setAngle(float angle) { m_angle = qBound(0.0f, angle, 180.0f); }
    float angle() const { return m_angle; }

    void setFalloff(float falloff) { m_falloff = falloff; }
    float falloff() const { return m_falloff; }

 signals:
    void lightModified();

private:
    LightType m_type = Point;
    QVector3D m_position;
    QVector3D m_direction = QVector3D(0, -1, 0);
    QVector3D m_color = QVector3D(1, 1, 1);
    float m_intensity = 100.0f;
    float m_angle = 45.0f;
    float m_falloff = 1.0f;
};

class Camera3D : public QObject
{
    Q_OBJECT
public:
    explicit Camera3D(QObject* parent = nullptr) : QObject(parent) {}
    ~Camera3D() {}

    void setPosition(const QVector3D& pos) { m_position = pos; }
    QVector3D position() const { return m_position; }

    void setTarget(const QVector3D& target) { m_target = target; }
    QVector3D target() const { return m_target; }

    void setUp(const QVector3D& up) { m_up = up.normalized(); }
    QVector3D up() const { return m_up; }

    void setFOV(float fov) { m_fov = qBound(10.0f, fov, 180.0f); }
    float fov() const { return m_fov; }

    void setNear(float value) { m_near = value; }
    float getNear() const { return m_near; }

    void setFar(float value) { m_far = value; }
    float getFar() const { return m_far; }

    QMatrix4x4 viewMatrix() const;
    QMatrix4x4 projectionMatrix() const;

    void orbit(const QVector3D& center, float azimuth, float elevation);
    void pan(float dx, float dy);
    void zoom(float delta);

 signals:
    void cameraModified();

private:
    QVector3D m_position = QVector3D(0, 5, 10);
    QVector3D m_target = QVector3D(0, 0, 0);
    QVector3D m_up = QVector3D(0, 1, 0);
    float m_fov = 50.0f;
    float m_near = 0.01f;
    float m_far = 1000.0f;
};

class RenderEngine : public QObject
{
    Q_OBJECT
public:
    explicit RenderEngine(QObject* parent = nullptr) : QObject(parent) {}
    ~RenderEngine() {}

    enum RenderEngineType { Eevee, Cycles, OpenGL };

    void setEngine(RenderEngineType type) { m_engine = type; }
    RenderEngineType engine() const { return m_engine; }

    void setScene(geometry::Scene3D* scene) { m_scene = scene; }
    geometry::Scene3D* scene() const { return m_scene; }

    void setCamera(Camera3D* camera) { m_camera = camera; }
    Camera3D* camera() const { return m_camera; }

    void render(int width, int height);
    QImage result() const { return m_result; }

    void setSamples(int samples) { m_samples = samples; }
    int samples() const { return m_samples; }

    void setResolution(int width, int height) { m_width = width; m_height = height; }
    void setBackgroundColor(const QVector3D& color) { m_background = color; }

    void enableShadows(bool enable) { m_shadows = enable; }
    void enableAO(bool enable) { m_ao = enable; }
    void enableGI(bool enable) { m_gi = enable; }

 signals:
    void renderStarted(int width, int height);
    void renderProgress(int percent);
    void renderComplete();

private:
    RenderEngineType m_engine = Eevee;
    geometry::Scene3D* m_scene = nullptr;
    Camera3D* m_camera = nullptr;
    QImage m_result;
    int m_samples = 128;
    int m_width = 1920;
    int m_height = 1080;
    QVector3D m_background = QVector3D(0.05f, 0.05f, 0.05f);
    bool m_shadows = true;
    bool m_ao = true;
    bool m_gi = false;
};

} // namespace rendering

// ============================================================================
// MeshModifier - Mesh manipulation
// ============================================================================

struct MeshVertex {
    QVector3D position;
    QVector3D normal;
    QVector2D uv;
    QVector4D color;
    QVector<int> boneIndices;
    QVector<float> boneWeights;
};

struct MeshFace {
    int v1, v2, v3;
    int materialId;
};

struct VertexGroup {
    QString name;
    QVector<int> vertexIndices;
};

struct ModifierData {
    QString type;
    QJsonObject params;
};

class MeshObject {
public:
    QString id;
    QString name;
    QString meshType;
    QVector<MeshVertex> vertices;
    QVector<MeshFace> faces;
    QVector<VertexGroup> vertexGroups;
    QVector<ModifierData> modifiers;
    QMatrix4x4 transform;
    bool visible = true;

    QVector3D getCenter() const;
    void applyTransform();
};

class MeshModifier : public QObject {
    Q_OBJECT
public:
    explicit MeshModifier(QObject* parent = nullptr);
    ~MeshModifier();

    static MeshModifier* instance();

    QVector<int> findMeshesByName(const QString& infix);
    int findMeshByName(const QString& name);

    MeshObject* getMesh(int meshId);
    void setMesh(int meshId, MeshObject* mesh);

    bool createVertexGroup(int meshId, const QString& groupName, const QVector<int>& vertexIndices);
    bool removeVertexGroup(int meshId, const QString& groupName);
    QVector<int> getVerticesInGroup(int meshId, const QString& groupName);
    QVector<int> getVerticesInRadius(int meshId, const QVector3D& center, float radius);

    bool createBones(int meshId, int boneCount, float radius);
    bool addSkinModifier(int meshId);

    void translateVertices(int meshId, const QVector<int>& vertices, const QVector3D& delta);
    void rotateVertices(int meshId, const QVector<int>& vertices, const QVector3D& center, const QVector3D& rotation);
    void scaleVertices(int meshId, const QVector<int>& vertices, const QVector3D& center, const QVector3D& scale);

    void mirrorAlongAxis(int meshId, int axis, float threshold = 0.001f);

    void addModifier(int meshId, const QString& modifierType, const QJsonObject& params);
    bool removeModifier(int meshId, const QString& modifierType);
    bool applyModifiers(int meshId);

    QVector3D calculateMeshCenter(int meshId);
    QVector3D calculateMeshBounds(int meshId);

    int addMesh(const QString& name, const QString& type);
    void removeMesh(int meshId);

 signals:
    void meshModified(int meshId);
    void meshAdded(int meshId);
    void meshRemoved(int meshId);

private:
    static MeshModifier* s_instance;

    QMap<int, MeshObject*> m_meshes;
    int m_nextMeshId = 0;

    int findMeshIndex(int meshId);
};

// ============================================================================
// ProceduralGenerator - Procedural content generation
// ============================================================================

class ProceduralTextureGenerator : public QObject
{
    Q_OBJECT

public:
    explicit ProceduralTextureGenerator(QObject* parent = nullptr);
    ~ProceduralTextureGenerator();

    enum TextureType {
        Type_Marble,
        Type_Wood,
        Type_Concrete,
        Type_Asphalt,
        Type_Grass,
        Type_Metal,
        Type_Carbon,
        Type_Plastic,
        Type_Rust,
        Type_Grunge
    };

    struct TextureParams {
        TextureType type = Type_Marble;
        int width = 1024;
        int height = 1024;
        float scale = 1.0f;
        float contrast = 1.0f;
        float brightness = 0.5f;
        QVector3D color1 = QVector3D(1, 1, 1);
        QVector3D color2 = QVector3D(0.5, 0.5, 0.5);
        int seed = 0;
    };

    QImage generateTexture(const TextureParams& params);
    QImage generateNormalMap(const QImage& diffuse);

    static QString textureTypeToString(TextureType type);
    static TextureType stringToTextureType(const QString& str);

 signals:
    void generationComplete(const QImage& texture);

private:
    float fbm(float x, float y, int octaves, int seed);
    float turbulence(float x, float y, int octaves, int seed);
    QVector3D marblePattern(float x, float y, int seed, const TextureParams& params);
    QVector3D woodPattern(float x, float y, int seed);
    QVector3D concretePattern(float x, float y, int seed);
    QVector3D asphaltPattern(float x, float y, int seed);
};

class ProceduralMeshGenerator : public QObject
{
    Q_OBJECT

public:
    explicit ProceduralMeshGenerator(QObject* parent = nullptr);
    ~ProceduralMeshGenerator();

    struct MeshData {
        QVector<QVector3D> vertices;
        QVector<QVector3D> normals;
        QVector<QVector2D> texCoords;
        QVector<QVector3D> indices;
    };

    struct MeshParams {
        enum Primitive { Box, Sphere, Cylinder, Cone, Torus, Plane, Grid } primitive = Box;
        float width = 1.0f;
        float height = 1.0f;
        float depth = 1.0f;
        int segments = 32;
        int rings = 16;
    };

    MeshData generateMesh(const MeshParams& params);
    MeshData generateTerrain(int width, int height, float scale, int seed);

 signals:
    void generationComplete(const MeshData& mesh);

private:
    void generateBox(MeshData& mesh, const MeshParams& params);
    void generateSphere(MeshData& mesh, const MeshParams& params);
    void generateCylinder(MeshData& mesh, const MeshParams& params);
    void generateTorus(MeshData& mesh, const MeshParams& params);
    float heightMap(float x, float z, int seed);
};

class ProceduralTrackGenerator : public QObject
{
    Q_OBJECT

public:
    explicit ProceduralTrackGenerator(QObject* parent = nullptr);
    ~ProceduralTrackGenerator();

    struct TrackParams {
        int numPoints = 20;
        float minRadius = 30.0f;
        float maxRadius = 150.0f;
        float totalLength = 3000.0f;
        float width = 12.0f;
        int seed = 0;
        bool closed = true;
        bool includePitLane = true;
    };

    struct TrackPoint {
        QVector3D position;
        QVector3D tangent;
        QVector3D normal;
        float curvature;
        float width;
    };

    struct TrackData {
        QVector<TrackPoint> points;
        QVector<QVector3D> centerLine;
        QVector<QVector3D> leftEdge;
        QVector<QVector3D> rightEdge;
        float totalLength;
    };

    TrackData generateTrack(const TrackParams& params);
    TrackData addChicanes(const TrackData& baseTrack, int count, float intensity);
    TrackData addCrest(const TrackData& baseTrack, float position, float height);

 signals:
    void generationComplete(const TrackData& track);

private:
    TrackPoint calculateTrackPoint(const QVector<QVector2D>& spline, float t);
    float m_trackWidth = 12.0f;
};

class ProceduralCarGenerator : public QObject
{
    Q_OBJECT

public:
    explicit ProceduralCarGenerator(QObject* parent = nullptr);
    ~ProceduralCarGenerator();

    struct CarParams {
        enum BodyStyle { Sedan, Coupe, SUV, Formula, GT } bodyStyle = Coupe;
        float length = 4.5f;
        float width = 1.8f;
        float height = 1.2f;
        float wheelbase = 2.7f;
        float trackWidth = 1.6f;
        float wheelRadius = 0.33f;
        int detailLevel = 2;
    };

    struct CarModel {
        ProceduralMeshGenerator::MeshData body;
        QVector<ProceduralMeshGenerator::MeshData> wheels;
        QVector<ProceduralMeshGenerator::MeshData> aerodynamics;
    };

    CarModel generateCar(const CarParams& params);

 signals:
    void generationComplete(const CarModel& car);
};

class DecalGenerator : public QObject
{
    Q_OBJECT

public:
    explicit DecalGenerator(QObject* parent = nullptr);
    ~DecalGenerator();

    struct DecalParams {
        enum Type { Number, Logo, Stripe, Circle, Rectangle, Text } type = Number;
        float width = 0.5f;
        float height = 0.5f;
        QString text;
        QString fontFamily = "Arial";
        int fontSize = 48;
        QColor color = Qt::white;
        QColor outlineColor = Qt::black;
        float outlineWidth = 2.0f;
    };

    QImage generateDecal(const DecalParams& params);
    QImage generateNumberPlate(const QString& number, const QString& fontFamily);

 signals:
    void generationComplete(const QImage& decal);
};

} // namespace ks