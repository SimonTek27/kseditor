#pragma once

#include <QObject>
#include <QVector>
#include <QVector3D>
#include <QMatrix4x4>
#include <QJsonObject>
#include <QString>

namespace ks {

// ============================================================================
// PhysicsMeshGenerator - AC-specific collision mesh generation
// ============================================================================
// Generates simplified collision meshes from visual meshes for Assetto Corsa.
// Uses Bullet Physics convex hull decomposition for accurate collision shapes.
//
// AC Collision Mesh Types:
// - Chassis: Main body collision (convex hull or compound)
// - Suspension: Suspension arm collision (simplified convex)
// - Wheels: Wheel collision cylinders
// - Interior: Driver cockpit collision
// - Extra: Additional collision volumes (mirrors, splitters, etc.)

class PhysicsMeshGenerator : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int chassisMeshCount READ chassisMeshCount NOTIFY meshGenerated)
    Q_PROPERTY(int suspensionMeshCount READ suspensionMeshCount NOTIFY meshGenerated)
    Q_PROPERTY(float simplificationRatio READ simplificationRatio WRITE setSimplificationRatio NOTIFY simplificationRatioChanged)

public:
    explicit PhysicsMeshGenerator(QObject* parent = nullptr);
    ~PhysicsMeshGenerator();

    // Collision mesh types for AC
    enum CollisionType {
        Chassis,        // Main body collision
        Suspension,     // Suspension arms
        Wheels,         // Wheel collision cylinders
        Interior,       // Driver cockpit
        Extra,          // Additional volumes (mirrors, etc.)
        GroundEffect,   // Floor/diffuser for downforce
        AeroBody        // Aerodynamic surfaces
    };
    Q_ENUM(CollisionType)

    // Mesh data structure for input/output
    struct PhysicsVertex {
        QVector3D position;
        QVector3D normal;
    };

    struct PhysicsFace {
        int v0, v1, v2;
        int materialId = 0;
    };

    struct PhysicsMesh {
        QString name;
        CollisionType type = Chassis;
        QVector<PhysicsVertex> vertices;
        QVector<PhysicsFace> faces;
        QMatrix4x4 transform;
        float mass = 0.0f;
        QVector3D centerOfMass;
        QVector3D boundingBoxMin;
        QVector3D boundingBoxMax;
    };

    struct CollisionConfig {
        float simplificationRatio = 0.1f;    // 10% of original triangles
        int maxConvexParts = 32;              // Max convex hulls per mesh
        float mergeDistance = 0.01f;          // Merge vertices within this distance
        bool preserveWeldingPoints = true;    // Keep weld points for suspension
        bool generateNormals = true;          // Auto-generate normals
        float convexDecompAccuracy = 0.001f; // VHACD accuracy
        bool useVHACD = true;                 // Use convex decomposition
    };

    // Main generation methods
    bool generateCollisionMesh(const QVector<PhysicsVertex>& inputVertices,
                               const QVector<PhysicsFace>& inputFaces,
                               CollisionType type,
                               const QString& meshName = QString());

    bool generateFromSceneMesh(const void* sceneMesh, CollisionType type);

    // Batch generation for full car
    struct CarCollisionSet {
        PhysicsMesh chassis;
        PhysicsMesh suspensionFL;
        PhysicsMesh suspensionFR;
        PhysicsMesh suspensionRL;
        PhysicsMesh suspensionRR;
        PhysicsMesh wheelFL;
        PhysicsMesh wheelFR;
        PhysicsMesh wheelRL;
        PhysicsMesh wheelRR;
        PhysicsMesh interior;
        QVector<PhysicsMesh> extras;
        QVector<PhysicsMesh> groundEffect;
        QVector<PhysicsMesh> aeroBody;
    };

    CarCollisionSet generateFullCarCollision(
        const QVector<PhysicsVertex>& chassisVertices,
        const QVector<PhysicsFace>& chassisFaces,
        const QVector<PhysicsVertex>& suspensionVertices,
        const QVector<PhysicsFace>& suspensionFaces,
        const QVector<PhysicsVertex>& wheelVertices,
        const QVector<PhysicsFace>& wheelFaces);

    // Configuration
    void setSimplificationRatio(float ratio);
    float simplificationRatio() const { return m_config.simplificationRatio; }

    void setMaxConvexParts(int maxParts) { m_config.maxConvexParts = maxParts; }
    int maxConvexParts() const { return m_config.maxConvexParts; }

    void setMergeDistance(float distance) { m_config.mergeDistance = distance; }
    float mergeDistance() const { return m_config.mergeDistance; }

    void setUseVHACD(bool use) { m_config.useVHACD = use; }
    bool useVHACD() const { return m_config.useVHACD; }

    // Mesh access
    int chassisMeshCount() const { return m_chassisMeshes.size(); }
    int suspensionMeshCount() const { return m_suspensionMeshes.size(); }

    QVector<PhysicsMesh> allMeshes() const;
    QVector<PhysicsMesh> meshesByType(CollisionType type) const;

    // Export methods
    QJsonObject toJson() const;
    bool fromJson(const QJsonObject& json);

    bool exportAcPhysicsMesh(const QString& filePath) const;
    bool exportObj(const QString& filePath, CollisionType type) const;

    // Utility methods
    static PhysicsMesh computeConvexHull(const QVector<PhysicsVertex>& vertices,
                                         const QVector<PhysicsFace>& faces);

    static PhysicsMesh simplifyMesh(const PhysicsMesh& input, float ratio);

    static PhysicsMesh mergeMeshes(const QVector<PhysicsMesh>& meshes);

    static QVector3D computeCenterOfMass(const QVector<PhysicsVertex>& vertices);

    static void computeBoundingBox(const QVector<PhysicsVertex>& vertices,
                                   QVector3D& min, QVector3D& max);

    // Validation
    struct ValidationResult {
        bool valid = true;
        QStringList warnings;
        QStringList errors;
        float totalVolume = 0.0f;
        float totalSurfaceArea = 0.0f;
        int totalTriangles = 0;
        int totalVertices = 0;
    };

    ValidationResult validateCollisionMesh(const PhysicsMesh& mesh) const;
    ValidationResult validateFullCarSet(const CarCollisionSet& carSet) const;

signals:
    void meshGenerated();
    void generationProgress(int percent);
    void simplificationRatioChanged(float ratio);
    void error(const QString& message);

private:
    CollisionConfig m_config;

    QVector<PhysicsMesh> m_chassisMeshes;
    QVector<PhysicsMesh> m_suspensionMeshes;
    QVector<PhysicsMesh> m_wheelMeshes;
    QVector<PhysicsMesh> m_interiorMeshes;
    QVector<PhysicsMesh> m_extraMeshes;
    QVector<PhysicsMesh> m_groundEffectMeshes;
    QVector<PhysicsMesh> m_aeroBodyMeshes;

    // Internal helper methods
    PhysicsMesh generateConvexDecomposition(const QVector<PhysicsVertex>& vertices,
                                            const QVector<PhysicsFace>& faces,
                                            CollisionType type);

    PhysicsMesh generateSimplifiedChassis(const QVector<PhysicsVertex>& vertices,
                                          const QVector<PhysicsFace>& faces);

    PhysicsMesh generateSuspensionArm(const QVector<PhysicsVertex>& vertices,
                                      const QVector<PhysicsFace>& faces);

    PhysicsMesh generateWheelCollider(const QVector<PhysicsVertex>& vertices,
                                      const QVector<PhysicsFace>& faces);

    void mergeCloseVertices(QVector<PhysicsVertex>& vertices, float threshold);
    void removeDegenerateFaces(QVector<PhysicsFace>& faces, int vertexCount);
    void computeFaceNormals(QVector<PhysicsVertex>& vertices, const QVector<PhysicsFace>& faces);

    // Bullet Physics helpers (conditional compilation)
#if HAS_BULLET
    void* createBulletConvexHull(const QVector<PhysicsVertex>& vertices);
    void* createBulletConvexDecomp(const QVector<PhysicsVertex>& vertices,
                                   const QVector<PhysicsFace>& faces,
                                   int maxParts);
    QVector<PhysicsMesh> extractBulletConvexParts(void* btShape);
    void releaseBulletShape(void* btShape);
#endif

    // VHACD helpers (conditional compilation)
#if HAS_VHACD
    QVector<PhysicsMesh> generateVHACDDecomposition(const QVector<PhysicsVertex>& vertices,
                                                     const QVector<PhysicsFace>& faces,
                                                     int maxParts);
#endif

    void addMeshToType(const PhysicsMesh& mesh, CollisionType type);

public:
    void clearAllMeshes();
};

} // namespace ks
