#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QMap>
#include <QColor>
#include <QQmlListProperty>
#include <QVector3D>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QTimer>
#include <QtMath>

#include "core/editor/BaseEditor.h"
#include "core/Graphics/SceneObject.h"
#include "core/Graphics/SceneGraph.h"
#include "core/Graphics/SceneMesh.h"
#include "core/FileFormat/MeshData.h"
#include "core/FileFormat/FBXParser.h"
#include "core/FileFormat/GLBParser.h"
#include "core/FileFormat/CADOBJParser.h"
#include "core/Math/MathCore.h"
#include "3DModeling_utils.h"
#include "core/mesh/ShapeKeyData.h"
#include "core/animation/ShapeKeyAnimDriver.h"
#include "core/tools/LODSystem.h"
#include "core/mesh/PhysicsCollisionSystem.h"
#include "SceneObjectListModel.h"
#include "CommandHistory.h"
#include "ShortcutManager.h"

namespace ks {

class SceneObjectQml : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(int id READ id)
    Q_PROPERTY(QVector3D position READ position WRITE setPosition)
    Q_PROPERTY(QVector3D rotation READ rotation WRITE setRotation)
    Q_PROPERTY(QVector3D scale READ scale WRITE setScale)
    Q_PROPERTY(bool selected READ isSelected WRITE setSelected)
    Q_PROPERTY(QString type READ type)

public:
    explicit SceneObjectQml(QObject* parent = nullptr) : QObject(parent), m_object(nullptr) {}
    explicit SceneObjectQml(SceneObject* obj, QObject* parent = nullptr) : QObject(parent), m_object(obj) {}

    QString name() const { return m_object ? m_object->name() : QString(); }
    void setName(const QString& n) { if (m_object) m_object->setName(n); emit changed(); }

    int id() const { return m_object ? m_object->id() : -1; }

    QVector3D position() const { return m_object ? QVector3D(m_object->transform().translation().x, m_object->transform().translation().y, m_object->transform().translation().z) : QVector3D(); }
    void setPosition(const QVector3D& p) { if (m_object) { Matrix4 t = m_object->transform(); t.setTranslation(Vec3(p.x(), p.y(), p.z())); m_object->setTransform(t); emit changed(); } }

    QVector3D rotation() const {
        if (!m_object) return QVector3D();
        Vec3 r = m_object->transform().rotation();
        return QVector3D(r.x, r.y, r.z);
    }
    void setRotation(const QVector3D& r) {
        if (m_object) {
            Matrix4 t = m_object->transform();
            t.setRotation(Vec3(r.x(), r.y(), r.z()));
            m_object->setTransform(t);
            emit changed();
        }
    }

    QVector3D scale() const { if (!m_object) return QVector3D(1,1,1); Vec3 s = m_object->transform().scale(); return QVector3D(s.x, s.y, s.z); }
    void setScale(const QVector3D& s) { if (m_object) { Matrix4 t = m_object->transform(); t.setScale(Vec3(s.x(), s.y(), s.z())); m_object->setTransform(t); emit changed(); } }

    bool isSelected() const { return m_object ? m_object->isSelected() : false; }
    void setSelected(bool s) { if (m_object) m_object->setSelected(s); emit changed(); }

    QString type() const {
        if (!m_object) return "None";
        switch (m_object->type()) {
            case SceneObject::Type::Node: return "Node";
            case SceneObject::Type::Mesh: return "Mesh";
            case SceneObject::Type::Light: return "Light";
            case SceneObject::Type::Camera: return "Camera";
            case SceneObject::Type::Bone: return "Bone";
            default: return "Unknown";
        }
    }

    SceneObject* object() const { return m_object; }

signals:
    void changed();

private:
    SceneObject* m_object;
};

class MeshDataQml : public QObject {
    Q_OBJECT
    Q_PROPERTY(int vertexCount READ vertexCount)
    Q_PROPERTY(int faceCount READ faceCount)
    Q_PROPERTY(int triangleCount READ triangleCount)
    Q_PROPERTY(int materialCount READ materialCount)
    Q_PROPERTY(bool hasNormals READ hasNormals)
    Q_PROPERTY(bool hasUVs READ hasUVs)

public:
    explicit MeshDataQml(QObject* parent = nullptr) : QObject(parent), m_mesh(nullptr) {}
    explicit MeshDataQml(Mesh* mesh, QObject* parent = nullptr) : QObject(parent), m_mesh(mesh) {}

    int vertexCount() const { return m_mesh ? (int)m_mesh->vertices.size() : 0; }
    int faceCount() const { return m_mesh ? (int)(m_mesh->indices.size() / 3) : 0; }
    int triangleCount() const { return faceCount(); }
    int materialCount() const { return m_mesh ? (int)m_mesh->materialIds.size() : 0; }
    bool hasNormals() const { if (!m_mesh) return false; for (const auto &v : m_mesh->vertices) if (v.nx!=0 || v.ny!=0 || v.nz!=0) return true; return false; }
    bool hasUVs() const { if (!m_mesh) return false; for (const auto &v : m_mesh->vertices) if (v.u!=0 || v.v!=0) return true; return false; }

private:
    Mesh* m_mesh;
};

class KSModelerQml : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString currentEditorType READ currentEditorType WRITE setCurrentEditorType NOTIFY editorTypeChanged)
    Q_PROPERTY(int objectCount READ objectCount NOTIFY sceneChanged)
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY selectionChanged)
    Q_PROPERTY(SceneObjectQml* selectedObject READ selectedObject NOTIFY selectionChanged)
    Q_PROPERTY(QString currentFile READ currentFile NOTIFY fileChanged)

    Q_PROPERTY(qreal camTheta READ camTheta WRITE setCamTheta NOTIFY cameraChanged)
    Q_PROPERTY(qreal camPhi READ camPhi WRITE setCamPhi NOTIFY cameraChanged)
    Q_PROPERTY(qreal camDistance READ camDistance WRITE setCamDistance NOTIFY cameraChanged)
    Q_PROPERTY(qreal camTargetX READ camTargetX WRITE setCamTargetX NOTIFY cameraChanged)
    Q_PROPERTY(qreal camTargetY READ camTargetY WRITE setCamTargetY NOTIFY cameraChanged)
    Q_PROPERTY(qreal camTargetZ READ camTargetZ WRITE setCamTargetZ NOTIFY cameraChanged)
    Q_PROPERTY(bool gridVisible READ gridVisible WRITE setGridVisible NOTIFY gridVisibleChanged)
    Q_PROPERTY(int viewMode READ viewMode WRITE setViewMode NOTIFY viewModeChanged)
    Q_PROPERTY(QString currentViewName READ currentViewName NOTIFY cameraChanged)
    Q_PROPERTY(QVector3D gizmoPosition READ gizmoPosition NOTIFY gizmoTransformChanged)
    Q_PROPERTY(QVector3D gizmoRotation READ gizmoRotation NOTIFY gizmoTransformChanged)
    Q_PROPERTY(QVector3D gizmoScale READ gizmoScale NOTIFY gizmoTransformChanged)
    Q_PROPERTY(int gizmoMode READ gizmoMode NOTIFY gizmoModeChanged)
    Q_PROPERTY(int boneVersion READ boneVersion NOTIFY skeletonChanged)
    Q_PROPERTY(SceneObjectListModel* sceneModel READ sceneModel CONSTANT)

    // Animation state for QML timeline
    Q_PROPERTY(qreal animationTime READ animationTime NOTIFY animationTimeChanged)
    Q_PROPERTY(qreal animationDuration READ animationDuration NOTIFY animationTimeChanged)
    Q_PROPERTY(bool isAnimating READ isAnimating NOTIFY playbackStateChanged)
    Q_PROPERTY(QString animationName READ animationName NOTIFY animationNameChanged)
    Q_PROPERTY(int animationFps READ animationFps CONSTANT)

public:
    explicit KSModelerQml(QObject* parent = nullptr);
    ~KSModelerQml();

    static KSModelerQml& instance() {
        static KSModelerQml inst;
        return inst;
    }

    QString currentEditorType() const { return m_currentEditorType; }
    void setCurrentEditorType(const QString& type) {
        if (m_currentEditorType != type) {
            m_currentEditorType = type;
            emit editorTypeChanged(type);
        }
    }

    QString currentFile() const { return m_currentFile; }

    int objectCount() const { return m_scene ? m_scene->allObjects().size() : 0; }
    bool hasSelection() const { return m_selectedObject != nullptr; }
    SceneObjectQml* selectedObject() const { return m_selectedObject; }

    Q_INVOKABLE void newProject();
    Q_INVOKABLE bool importFile(const QString& path);
    Q_INVOKABLE bool exportFile(const QString& path);
    Q_INVOKABLE bool importKN5(const QString& path);
    Q_INVOKABLE bool importFBX(const QString& path);
    Q_INVOKABLE bool importGLB(const QString& path);
    Q_INVOKABLE bool importOBJ(const QString& path);
    Q_INVOKABLE bool exportKN5(const QString& path);
    Q_INVOKABLE bool exportFBX(const QString& path);
    Q_INVOKABLE bool exportGLB(const QString& path);

    Q_INVOKABLE void selectObject(int id);
    Q_INVOKABLE void deselectAll();
    Q_INVOKABLE void deleteSelected();
    Q_INVOKABLE void duplicateSelected();

    Q_INVOKABLE void setGizmoMode(int mode);
    Q_INVOKABLE void translateSelected(float x, float y, float z);
    Q_INVOKABLE void rotateSelected(float x, float y, float z);
    Q_INVOKABLE void scaleSelected(float x, float y, float z);
    Q_INVOKABLE void setSelectedPosition(float x, float y, float z);
    Q_INVOKABLE void setSelectedRotation(float x, float y, float z);
    Q_INVOKABLE void setSelectedScale(float x, float y, float z);

    // Proportional editing
    Q_INVOKABLE void setProportionalEditing(bool enabled);
    Q_INVOKABLE bool isProportionalEditing() const;
    Q_INVOKABLE void setProportionalRadius(float radius);
    Q_INVOKABLE float proportionalRadius() const;
    Q_INVOKABLE void setProportionalFalloffType(int type);
    Q_INVOKABLE int proportionalFalloffType() const;
    Q_INVOKABLE bool pickProportionalCenter(float pickX, float pickY, float pickZ);
    Q_INVOKABLE void clearProportionalCenter();
    Q_INVOKABLE bool hasProportionalCenter() const;
    Q_INVOKABLE QVector3D proportionalCenter() const;
    Q_INVOKABLE void translateProportional(float x, float y, float z);
    Q_INVOKABLE void rotateProportional(float x, float y, float z);
    Q_INVOKABLE void scaleProportional(float x, float y, float z);

    Q_INVOKABLE void extrudeFaces(const QList<int>& faceIndices, float distance);
    Q_INVOKABLE void bevelEdges(const QList<int>& edgeIndices, float amount, int segments);
    Q_INVOKABLE void subdivideFaces(const QList<int>& faceIndices, int cuts);
    Q_INVOKABLE void triangulateMesh();
    Q_INVOKABLE void flipNormals();
    Q_INVOKABLE void recalculateNormals();
    Q_INVOKABLE void weldVertices(float threshold);
    Q_INVOKABLE void removeDoubles(float threshold);
    Q_INVOKABLE void mirrorMesh(int axis);
    Q_INVOKABLE void booleanOperation(int operation, int targetObjectId = -1);
    Q_INVOKABLE QVariantList getMeshObjects() const;
    Q_INVOKABLE void insetFaces(const QList<int>& faceIndices, float amount);
    Q_INVOKABLE void knifeCut(float startX, float startY, float startZ, float endX, float endY, float endZ);
    Q_INVOKABLE void dissolveEdges(const QList<int>& edgeIndices);
    Q_INVOKABLE void dissolveVertices(const QList<int>& vertexIndices);
    Q_INVOKABLE void splitMeshes();

    // Shape key / morph target operations
    Q_INVOKABLE int addShapeKey(const QString& name);
    Q_INVOKABLE bool removeShapeKey(int index);
    Q_INVOKABLE bool renameShapeKey(int index, const QString& newName);
    Q_INVOKABLE int captureShapeKey(const QString& name);
    Q_INVOKABLE void setShapeKeyWeight(int index, float weight);
    Q_INVOKABLE float getShapeKeyWeight(int index);
    Q_INVOKABLE QStringList getShapeKeyNames();
    Q_INVOKABLE int getShapeKeyCount();
    Q_INVOKABLE void resetShapeKeys();
    Q_INVOKABLE void muteShapeKey(int index, bool mute);
    Q_INVOKABLE void applyShapeKeys();
    Q_INVOKABLE void clearAllShapeKeys();

    // LOD generation
    Q_INVOKABLE bool generateLODs(int levelCount = 3);
    Q_INVOKABLE bool exportLODs(const QString& basePath);
    Q_INVOKABLE void setLODDistance(int level, float distance);
    Q_INVOKABLE float getLODDistance(int level) const;

    // Deformation tools
    Q_INVOKABLE void applyCageDeform(const QString& cageObjectName, float strength, bool preserveVolume);
    Q_INVOKABLE void cageDeformBuildCage(int subdivisions);
    Q_INVOKABLE void applyLattice(int uDivs, int vDivs, int wDivs, float strength);
    Q_INVOKABLE void applySimpleDeform(int method, int axis, float angle, float factor);
    Q_INVOKABLE QVector3D getLatticeControlPoint(int index);
    Q_INVOKABLE void setLatticeControlPoint(int index, float x, float y, float z);
    Q_INVOKABLE int getLatticeControlPointCount();
    Q_INVOKABLE void resetLattice();

    // Collision mesh generation
    Q_INVOKABLE bool generateCollisionMesh(int shapeType);
    Q_INVOKABLE bool generateCollisionConvexDecomp(int maxHulls);
    Q_INVOKABLE void setCollisionSimplifyRatio(float ratio);
    Q_INVOKABLE float getCollisionSimplifyRatio() const;

    // Shape key animation
    Q_INVOKABLE void animateShapeKey(const QString& keyName, int startFrame, int endFrame, float startWeight, float endWeight);
    Q_INVOKABLE void clearShapeKeyAnimation(const QString& keyName);
    Q_INVOKABLE void evaluateShapeKeyAnimation(int frame);

    Q_INVOKABLE void addPrimitiveCube(float size = 1.0f);
    Q_INVOKABLE void addPrimitiveSphere(float radius = 1.0f, int segments = 32, int rings = 16);
    Q_INVOKABLE void addPrimitiveCylinder(float radius = 1.0f, float height = 2.0f, int segments = 32);
    Q_INVOKABLE void addPrimitiveCone(float radius = 1.0f, float height = 2.0f, int segments = 32);
    Q_INVOKABLE void addPrimitiveTorus(float majorRadius = 1.0f, float minorRadius = 0.25f, int majorSegments = 32, int minorSegments = 16);
    Q_INVOKABLE void addPrimitivePlane(float width = 2.0f, float height = 2.0f, int widthSegments = 1, int heightSegments = 1);

    Q_INVOKABLE void projectUVPlanar(int axis);
    Q_INVOKABLE void projectUVCylindrical(int axis);
    Q_INVOKABLE void projectUVSpherical();
    Q_INVOKABLE void projectUVBox(float size);
    Q_INVOKABLE void unwrapUVs(const QString& method);
    Q_INVOKABLE void packUVs(float margin, int resolution);
    Q_INVOKABLE void translateUVs(float u, float v);
    Q_INVOKABLE void rotateUVs(float angle);
    Q_INVOKABLE void scaleUVs(float u, float v);
    Q_INVOKABLE void generateAutoUVs(int resolution);

    Q_INVOKABLE void createSkeleton(const QString& type);
    Q_INVOKABLE void addBone(const QString& name, int parentId, float x, float y, float z);
    Q_INVOKABLE void bindToSkeleton(float maxDistance);
    Q_INVOKABLE void smoothSkinning(int iterations);
    Q_INVOKABLE void normalizeWeights();
    Q_INVOKABLE void pruneWeights(float threshold, int maxInfluences);
    Q_INVOKABLE void paintWeights(int boneId, float x, float y, float z, float radius, float strength);
    Q_INVOKABLE void mirrorWeights(int axis);
    Q_INVOKABLE void solveTwoBoneIK(float targetX, float targetY, float targetZ);
    Q_INVOKABLE void solveCCDIK(float targetX, float targetY, float targetZ, int iterations);

    // Bone access and IK/FK control
    Q_INVOKABLE int boneCount() const;
    Q_INVOKABLE QString getBoneName(int boneIdx) const;
    Q_INVOKABLE int getBoneParentId(int boneIdx) const;
    Q_INVOKABLE QVector3D getBonePosition(int boneIdx) const;
    Q_INVOKABLE QVector3D getBoneWorldPosition(int boneIdx) const;
    Q_INVOKABLE QVector3D getBoneRotation(int boneIdx) const;
    Q_INVOKABLE QVector3D getBoneLength(int boneIdx) const;
    Q_INVOKABLE int selectedBoneIndex() const;
    Q_INVOKABLE void selectBone(int boneIdx);
    Q_INVOKABLE void setBoneIKWeight(int boneIdx, float weight);
    Q_INVOKABLE float getBoneIKWeight(int boneIdx) const;
    Q_INVOKABLE void updateBoneHierarchy();
    Q_INVOKABLE void setBonePosition(int boneIdx, float x, float y, float z);
    Q_INVOKABLE void setBoneRotation(int boneIdx, float x, float y, float z);
    Q_INVOKABLE void applyFKPose(int boneIdx, float x, float y, float z, float rx, float ry, float rz);

    Q_INVOKABLE void createMaterial(const QString& name);
    Q_INVOKABLE void setMaterialAlbedo(float r, float g, float b, float a);
    Q_INVOKABLE void setMaterialMetallic(float value);
    Q_INVOKABLE void setMaterialRoughness(float value);

    Q_INVOKABLE void setSelectedBaseColor(float r, float g, float b);
    Q_INVOKABLE void setSelectedMetallic(float value);
    Q_INVOKABLE void setSelectedRoughness(float value);
    Q_INVOKABLE void setSelectedOpacity(float value);

    Q_INVOKABLE int addTransformGroup(const QString& name = "TransformGroup");
    Q_INVOKABLE int addCamera(const QString& name = "Camera");
    Q_INVOKABLE int addLight(const QString& name = "Light");
    Q_INVOKABLE void setMaterialNormalStrength(float value);
    Q_INVOKABLE void setMaterialEmissive(float r, float g, float b);
    Q_INVOKABLE void setMaterialOpacity(float value);
    Q_INVOKABLE void createShader(const QString& vertexShader, const QString& fragmentShader);
    Q_INVOKABLE void compileShader();

    // Animation state accessors
    qreal animationTime() const { return m_animationTime; }
    qreal animationDuration() const {
        return (m_currentAnimation >= 0 && m_currentAnimation < m_animations.size())
            ? m_animations[m_currentAnimation].duration : 0.0;
    }
    QString animationName() const {
        return (m_currentAnimation >= 0 && m_currentAnimation < m_animations.size())
            ? m_animations[m_currentAnimation].name : QString();
    }
    int animationFps() const { return m_animFps; }

    Q_INVOKABLE void addAnimation(const QString& name, float duration);
    Q_INVOKABLE QStringList animationNames() const;
    Q_INVOKABLE void setCurrentAnimationByName(const QString& name);
    Q_INVOKABLE void deleteAnimation(const QString& name);
    Q_INVOKABLE void addKeyframe(const QString& animName, float time, int boneId, float x, float y, float z, float rotX, float rotY, float rotZ);
    Q_INVOKABLE void addKeyframeForSelectedObject(const QString& animName);
    Q_INVOKABLE void playAnimation(const QString& name);
    Q_INVOKABLE void stopAnimation();
    Q_INVOKABLE void togglePlayPause();
    Q_INVOKABLE void setAnimationTime(float time);
    Q_INVOKABLE float getAnimationTime() const;
    Q_INVOKABLE bool isAnimating() const;
    Q_INVOKABLE void setAnimationLoop(bool loop);

    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE bool canUndo() const;
    Q_INVOKABLE bool canRedo() const;

    // Keyboard shortcuts
    Q_INVOKABLE bool loadShortcuts(const QString& filePath);
    Q_INVOKABLE bool saveShortcuts(const QString& filePath);
    Q_INVOKABLE QString getShortcutKey(const QString& id) const;
    Q_INVOKABLE QString getShortcutDescription(const QString& id) const;
    Q_INVOKABLE void remapShortcut(const QString& id, const QString& newKey);
    Q_INVOKABLE void resetShortcutsToDefaults();
    Q_INVOKABLE QStringList getAllShortcutIds() const;
    Q_INVOKABLE QVariantMap getShortcutsData() const;

    Q_INVOKABLE void showMaterialEditor();
    Q_INVOKABLE void showPropertiesPanel();
    Q_INVOKABLE void showSceneGraph();

    // Camera controls
    qreal camTheta() const { return m_camTheta; }
    void setCamTheta(qreal v);
    qreal camPhi() const { return m_camPhi; }
    void setCamPhi(qreal v);
    qreal camDistance() const { return m_camDistance; }
    void setCamDistance(qreal v);
    qreal camTargetX() const { return m_camTargetX; }
    void setCamTargetX(qreal v);
    qreal camTargetY() const { return m_camTargetY; }
    void setCamTargetY(qreal v);
    qreal camTargetZ() const { return m_camTargetZ; }
    void setCamTargetZ(qreal v);

    // Gizmo state for QML viewport
    QVector3D gizmoPosition() const;
    QVector3D gizmoRotation() const;
    QVector3D gizmoScale() const;
    int gizmoMode() const { return m_gizmoMode; }

    Q_INVOKABLE void setCameraView(const QString& view);
    Q_INVOKABLE void focusOnSelected();

    bool gridVisible() const { return m_gridVisible; }
    void setGridVisible(bool v);

    int viewMode() const { return m_viewMode; }
    void setViewMode(int mode);

    QString currentViewName() const { return m_currentViewName; }
    int boneVersion() const { return m_boneVersion; }

    SceneObjectListModel* sceneModel() const { return m_sceneModel; }

    // Track methods
    Q_INVOKABLE void newTrack(const QString& name);
    Q_INVOKABLE bool loadTrack(const QString& path);
    Q_INVOKABLE bool saveTrack(const QString& path);
    Q_INVOKABLE bool exportTrackKN5(const QString& path);
    Q_INVOKABLE bool importGPX(const QString& path);
    Q_INVOKABLE bool importKML(const QString& path);
    Q_INVOKABLE bool importFromMap(const QString& service, double lat, double lon, int zoom);

    Q_INVOKABLE void addTrackPoint(float x, float y, float z);
    Q_INVOKABLE void insertTrackPoint(int index, float x, float y, float z);
    Q_INVOKABLE void removeTrackPoint(int index);
    Q_INVOKABLE void smoothTrackPoints(int iterations);
    Q_INVOKABLE void closeTrackLoop();
    Q_INVOKABLE void setTrackWidth(float width);

    Q_INVOKABLE void addTrackSection(int type);
    Q_INVOKABLE void removeTrackSection(int index);
    Q_INVOKABLE void setSectionSurface(int index, const QString& surface);
    Q_INVOKABLE void addSectionKerb(int index, float height, float width);

    Q_INVOKABLE void generateTrackMesh();
    Q_INVOKABLE void generateTrackEdges();
    Q_INVOKABLE void generateTerrain(float size, float maxHeight);
    Q_INVOKABLE void generateAILine();

    Q_INVOKABLE int trackPointCount() const;
    Q_INVOKABLE int trackSectionCount() const;
    Q_INVOKABLE float trackLength() const;
    Q_INVOKABLE int cornerCount() const;

    Q_INVOKABLE void addTrackCamera(const QString& name, float x, float y, float z, float targetX, float targetY, float targetZ);
    Q_INVOKABLE void removeTrackCamera(int index);
    Q_INVOKABLE void setCameraPosition(int index, float x, float y, float z);
    Q_INVOKABLE void setCameraTarget(int index, float x, float y, float z);

    Q_INVOKABLE QStringList getSupportedImportFormats() const;
    Q_INVOKABLE QStringList getSupportedExportFormats() const;

    void setScene(SceneGraph* scene);
    SceneGraph* sceneGraph() const { return m_scene; }
    void setEditor(core_BaseEditor* editor);

signals:
    void editorTypeChanged(const QString& type);
    void sceneChanged();
    void selectionChanged();
    void statusMessage(const QString& message);
    void errorMessage(const QString& error);
    void fileChanged(const QString& path);
    void error(const QString& message);
    void cameraChanged();
    void gridVisibleChanged();
    void viewModeChanged();
    void gizmoModeChanged();
    void gizmoTransformChanged();
    void animationTimeChanged();
    void playbackStateChanged();
    void animationNameChanged();
    void shapeKeysChanged();
    void lodsGenerated(int count);
    void collisionGenerated(int hullCount);
    void proportionalEditingChanged();
    void proportionalCenterChanged();
    void boneSelectionChanged();
    void skeletonChanged();

private:
    QString m_currentEditorType = "car";
    SceneGraph* m_scene = nullptr;
    core_BaseEditor* m_editor = nullptr;
    SceneObjectQml* m_selectedObject = nullptr;
    SceneObjectListModel* m_sceneModel = nullptr;
    QString m_currentFile;
    int m_gizmoMode = 0;

    qreal m_camTheta = 45.0;
    qreal m_camPhi = 30.0;
    qreal m_camDistance = 5.0;
    qreal m_camTargetX = 0.0;
    qreal m_camTargetY = 0.0;
    qreal m_camTargetZ = 0.0;
    bool m_gridVisible = true;
    int m_viewMode = 0;
    QString m_currentViewName = "Perspective";

    // Track data
    QVector<QVector3D> m_trackPoints;
    QVector<int> m_trackSections;
    float m_trackWidth = 12.0f;
    struct TrackSurface {
        QString type = "asphalt";
        float grip = 1.0f;
        float roughness = 0.3f;
    };
    struct TrackKerb {
        float height = 0.1f;
        float width = 0.3f;
        QColor color = Qt::red;
    };
    QMap<int, TrackSurface> m_surfaces;
    QMap<int, TrackKerb> m_kerbs;

    // Undo/Redo
    QVector<QVariantMap> m_undoStack;
    QVector<QVariantMap> m_redoStack;
    QVariantMap m_currentState;
    CommandHistory* m_commandHistory = nullptr;
    ShortcutManager* m_shortcutManager = nullptr;

    // Skeleton data
    struct Bone {
        QString name;
        int parentId;
        QVector3D position;
        QVector3D rotation;
        QVector<int> children;
        float ikWeight = 1.0f; // 0=FK, 1=full IK
        QVector3D length;      // bone offset from parent (computed)
    };
    QVector<Bone> m_bones;
    int m_selectedBone = -1;

    // Animation data
    struct Keyframe {
        float time;
        int boneId;
        QVector3D position;
        QVector3D rotation;
    };
    struct Animation {
        QString name;
        float duration;
        QVector<Keyframe> keyframes;
    };
    QVector<Animation> m_animations;
    int m_currentAnimation = -1;
    float m_animationTime = 0.0f;
    bool m_isAnimating = false;
    QTimer* m_animTimer = nullptr;
    int m_animFps = 30;
    bool m_animLoop = true;
    int m_animEasing = 0;

    QVector3D interpolatePosition(const QVector<Keyframe>& kfs, float time, int boneId);
    QVector3D interpolateRotation(const QVector<Keyframe>& kfs, float time, int boneId);
    float applyEasing(float t, int type) const;
    void applyPoseToBones(const Animation& anim, float time);
    int findKeyframeIndex(const QVector<Keyframe>& kfs, float time, int boneId) const;

private slots:
    void advanceAnimation();

private:
    // Shape key data
    MeshData m_shapeKeyMesh;
    bool m_shapeKeysDirty = false;

    // Shape key animation driver
    ShapeKeyAnimDriver* m_shapeKeyAnimDriver = nullptr;

    // Proportional editing state
    struct ProportionalEditState {
        bool enabled = false;
        float radius = 2.0f;
        int falloffType = 1; // 0=Smooth, 1=Linear, 2=Sharp, 3=Root, 4=Sphere, 5=Constant
        QVector3D center;
        bool hasCenter = false;
    };
    ProportionalEditState m_propEdit;

    // LOD data
    LODResult m_lodResult;
    CollisionResult m_collisionResult;
    CollisionConfig m_collisionConfig;
    float m_collisionSimplifyRatio = 0.3f;
    int m_boneVersion = 0;

    // Lattice state
    struct LatticeState {
        bool active = false;
        int uDivs = 2, vDivs = 2, wDivs = 2;
        float strength = 1.0f;
        QVector<QVector3D> controlPoints;
        QVector<QVector3D> restControlPoints;
        int selectedObject=-1;
    };
    LatticeState m_lattice;

    // Vertex weights
    struct VertexWeight {
        int boneId;
        float weight;
    };
    QVector<QVector<VertexWeight>> m_vertexWeights;

    // Material state
    struct MaterialState {
        QString name;
        QColor albedo = QColor(200, 200, 200, 255);
        float metallic = 0.0f;
        float roughness = 0.5f;
        float normalStrength = 1.0f;
        QColor emissive = QColor(0, 0, 0);
        float opacity = 1.0f;
        QString vertexShader;
        QString fragmentShader;
    };
    QVector<MaterialState> m_materials;
    int m_currentMaterial = -1;

    void syncShapeKeyMesh();
    QString detectFormat(const QString& path) const;
    Mesh* getSelectedMesh();
};

} // namespace ks