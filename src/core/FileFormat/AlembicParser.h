#pragma once

#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QVariant>
#include <QVector3D>
#include <QVector2D>
#include <QQuaternion>
#include "MeshData.h"

namespace ks {


// ─── Alembic Archive Structure ──────────────────────────────────────────────

struct AlembicObject {
    std::string fullName;
    std::string typeName;  // "Xform", "PolyMesh", "Camera", "Light", etc.
    std::string parent;
    QVector<std::string> children;
    QMap<std::string, QVariant> properties;
};

struct AlembicMesh {
    std::string objectPath;
    std::string subdivisionScheme;  // "catmullClark", "loop", "none"
    
    struct Sample {
        double time = 1.0;
        QVector<QVector3D> positions;
        QVector<QVector3D> velocities;
        QVector<QVector3D> normals;
        QVector<QVector2D> uvs;
        QVector<int> faceCounts;
        QVector<int> faceIndices;
        QVector<int> materialIds;
    };
    QVector<Sample> samples;
};

struct AlembicCamera {
    std::string objectPath;
    
    struct Sample {
        double time = 1.0;
        float fov = 60.0f;
        float focusDistance = 100.0f;
        float aperture = 36.0f;
        QVector3D position = {0, 0, 0};
        QQuaternion rotation;
    };
    QVector<Sample> samples;
};

struct AlembicLight {
    std::string objectPath;
    enum Type { Point, Directional, Spot, Area };
    Type type = Point;
    
    struct Sample {
        double time = 1.0;
        QVector3D color = {1, 1, 1};
        float intensity = 1.0f;
        float exposure = 0.0f;
        QVector3D position = {0, 0, 0};
        QQuaternion rotation;
        float coneAngle = 45.0f;
        float penumbraAngle = 5.0f;
    };
    QVector<Sample> samples;
};

struct AlembicArchive {
    QString filePath;
    double startTime = 1.0;
    double endTime = 100.0;
    double timeSamplingRate = 24.0;
    QMap<std::string, QVariant> metadata;
    
    QVector<AlembicObject> objects;
    QVector<AlembicMesh> meshes;
    QVector<AlembicCamera> cameras;
    QVector<AlembicLight> lights;
};

// ─── Alembic Parser ──────────────────────────────────────────────────────────

class AlembicParser {
public:
    // Read/write archive
    static bool read(const QString& filePath, AlembicArchive& archive, QString* error = nullptr);
    static bool write(const QString& filePath, const AlembicArchive& archive, QString* error = nullptr);
    
    // Mesh conversion
    static bool alembicMeshToMeshData(const AlembicMesh& mesh, int frameIndex, fileformat::MeshData& outData);
    static bool meshDataToAlembicMesh(const fileformat::MeshData& data, AlembicMesh& outMesh);
    static bool sampleMeshAtTime(const AlembicMesh& mesh, double time, fileformat::MeshData& outData);
    
    // Camera conversion
    static bool sampleCameraAtTime(const AlembicCamera& cam, double time,
                                    QVector3D& pos, QQuaternion& rot, float& fov);
    
    // Time utilities
    static QVector<double> getSampleTimes(const AlembicArchive& archive);
    static int findSampleIndex(const AlembicArchive& archive, double time);
    static double getTimeForFrame(const AlembicArchive& archive, int frame);

private:
    static bool parseJsonRepresentation(const QJsonObject& obj, AlembicArchive& archive);
};

} // namespace ks