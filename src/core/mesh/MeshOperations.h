#pragma once

#include <QVector3D>
#include <QVector4D>
#include <QVector2D>
#include <QMatrix4x4>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QtMath>
#include "GeometryTypes.h"

namespace ks {

struct Vertex {
    QVector3D position;
    QVector3D normal;
    QVector2D uv;
    QVector4D color;
    QVector3D tangent;
    float weight = 1.0f;
    float mask = 0.0f;
    int boneIndex = -1;

    Vertex() : color(1, 1, 1, 1) {}
};

struct Edge {
    int v1, v2;
    Edge(int a = -1, int b = -1) : v1(a), v2(b) {}
};

struct Face {
    QVector<int> indices;
    QVector<int> uvIndices;    // separate UV indices (if different from position indices)
    QVector<int> uv2Indices;   // UV2 channel indices
    QVector3D normal;
    int materialId = 0;

    Face() = default;
    Face(std::initializer_list<int> il) : indices(il) {}
    Face(const QVector<int>& v) : indices(v) {}

    int vertexCount() const { return indices.size(); }
    int& operator[](int i) { return indices[i]; }
    int operator[](int i) const { return indices[i]; }
    bool operator==(const Face& other) const { return indices == other.indices; }
    bool operator!=(const Face& other) const { return !(*this == other); }
};

struct MeshData {
    QString   name;
    QString   materialName;
    QVector4D diffuseColor  = {0.8f,0.8f,0.8f,1.f};
    float     metallic      = 0.f;
    float     roughness     = 0.5f;

    QVector<Vertex>     vertices;
    QVector<Face>       faces;
    QVector<Edge>       edges;
    QVector<QVector3D>  normals;
    QVector<QVector2D>  uvs;
    QVector<QVector2D>  uv2s;       // UV2 channel (damage maps, lightmaps)
    QVector<QVector3D>  tangents;
    QVector<QVector3D>  bitangents;
    QVector3D boundingBoxMin;
    QVector3D boundingBoxMax;
    float boundingRadius = 0.0f;
    QStringList materials;
    QMap<QString, QVector<float>> vertexGroups;

    // Shape key / morph target data
    QStringList shapeKeyNames;
    QVector<QVector<QVector3D>> shapeKeyDeltas;
    QVector<float> shapeKeyWeights;
    QVector<bool> shapeKeyMute;
    QVector<float> shapeKeyMin;
    QVector<float> shapeKeyMax;

    void clear();
    void computeBoundingBox();
    void computeNormals();
    void computeTangents();
    void flipFaces();
    void triangulate();
    
    int getTriangleCount() const;
    int getVertexCount() const;
    
    geometry::GeoMeshData toGeoMesh() const;
    static MeshData fromGeoMesh(const geometry::GeoMeshData& geo);
};

struct MeshUVIsland {
    QVector<int> faceIndices;
    QVector2D minUV, maxUV;
};

class MeshOperations {
public:
    static MeshData createBox(float width = 1.0f, float height = 1.0f, float depth = 1.0f);
    static MeshData createSphere(float radius = 0.5f, int segments = 32, int rings = 16);
    static MeshData createCylinder(float radius = 0.5f, float height = 1.0f, int segments = 32);
    static MeshData createCone(float radius = 0.5f, float height = 1.0f, int segments = 32);
    static MeshData createPlane(float width = 1.0f, float height = 1.0f, int uSubdiv = 1, int vSubdiv = 1);
    static MeshData createTorus(float majorRadius = 0.5f, float minorRadius = 0.2f, int majorSeg = 32, int minorSeg = 16);
    static MeshData createGrid(float width = 1.0f, float height = 1.0f, int uSubdiv = 10, int vSubdiv = 10);
    static MeshData createIcosphere(float radius = 0.5f, int subdivisions = 2);

    static MeshData booleanUnion(const MeshData& a, const MeshData& b);
    static MeshData booleanDifference(const MeshData& a, const MeshData& b);
    static MeshData booleanIntersection(const MeshData& a, const MeshData& b);
    static MeshData booleanXor(const MeshData& a, const MeshData& b);

    static MeshData extrude(const MeshData& mesh, const QVector3D& direction, float distance, bool individualFaces = false);
    static MeshData extrudeFaces(const MeshData& mesh, const QVector<QVector3D>& directions);
    static MeshData bevelEdges(const MeshData& mesh, float distance, int segments = 1, float angleLimit = qDegreesToRadians(30.0f));
    static MeshData bevelVertices(const MeshData& mesh, float distance);

    static MeshData insetFaces(const MeshData& mesh, float distance, float depth = 0.0f);
    static MeshData weldVertices(const MeshData& mesh, float threshold = 0.001f);
    static MeshData dissolveEdges(const MeshData& mesh, const QVector<int>& edgeIndices);
    static MeshData dissolveFaces(const MeshData& mesh, const QVector<int>& faceIndices);
    static MeshData dissolveVertices(const MeshData& mesh, const QVector<int>& vertexIndices);

    static MeshData loft(const QVector<MeshData>& profiles, bool close = false);
    static MeshData sweep(const MeshData& profile, const QVector<QMatrix4x4>& transforms, bool close = false);
    static MeshData spin(const MeshData& profile, const QVector3D& axis, float angle, int steps);

    static MeshData subdivide(const MeshData& mesh, int levels = 1);
    static MeshData unsubdivide(const MeshData& mesh, float detail = 0.0f);
    static MeshData triangulate(const MeshData& mesh);
    static MeshData quadrangulate(const MeshData& mesh);

    static MeshData mirror(const MeshData& mesh, const QVector3D& axis, float offset = 0.0f);
    static MeshData array(const MeshData& mesh, int count, const QVector3D& offset);
    static MeshData radialArray(const MeshData& mesh, int count, const QVector3D& axis, float angle);

    static MeshData knifeCut(const MeshData& mesh, const QVector3D& cutStart, const QVector3D& cutEnd);
    static MeshData shrinkwrap(const MeshData& mesh, const MeshData& target, const QVector3D& direction);
    static MeshData displace(const MeshData& mesh, const QImage& heightmap, float strength);

    static void mergeMeshes(MeshData& target, const MeshData& source);
    static void splitMeshes(const MeshData& mesh, QVector<MeshData>& result);

    static QVector<MeshUVIsland> findUVIslands(const MeshData& mesh);

    static geometry::GeoMeshData toGeoMesh(const MeshData& mesh);
    static MeshData fromGeoMesh(const geometry::GeoMeshData& geo);

private:
    static QVector<int> findEdge(const MeshData& mesh, int v1, int v2);
    static bool isEdge(const MeshData& mesh, int v1, int v2);
    static QVector3D computeFaceNormal(const MeshData& mesh, int faceIndex);
    static void ensureEdgeList(MeshData& mesh);
};

class ExtrudeOptions {
public:
    QVector3D direction;
    float distance = 1.0f;
    bool individualFaces = false;
    bool createCaps = true;
    bool createFrontCaps = true;
    bool createBackCaps = true;
    float offset = 0.0f;
};

class BevelOptions {
public:
    float distance = 0.01f;
    int segments = 1;
    float angleLimit = qDegreesToRadians(30.0f);
    bool bevelVertices = false;
    bool bevelEdges = true;
    bool useClampOverlap = true;
    float clampOverlap = 0.05f;
};

class ArrayOptions {
public:
    int count = 4;
    QVector3D constantOffset;
    QVector3D relativeOffset;
    QVector3D pivotPoint;
    bool useCount = true;
    bool useConstantOffset = true;
    bool useRelativeOffset = false;
    float mergeThreshold = 0.0001f;
};

}