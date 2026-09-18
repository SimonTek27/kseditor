#pragma once
// ============================================================================
// RoadBuilder.h
// Converts Road spline data → tessellated 3D mesh geometry.
// Handles: Catmull-Rom interpolation, camber, crown, bridge elevation,
// kerb attachment, physics road overlay, surface normals & UV.
// ============================================================================

#include "TrackBuilderTypes.h"
#include <QObject>
#include <QVector>
#include <QVector2D>
#include <QVector3D>

namespace ks { namespace track {

// ============================================================================
// Tessellated road vertex
// ============================================================================
struct RoadVertex {
    QVector3D position;
    QVector3D normal;
    QVector2D uv;
    float     laneT = 0.f;   // 0=left edge, 0.5=centre, 1=right edge
    float     distAlong = 0.f;
};

// ============================================================================
// Tessellated road strip (ready for GPU / OBJ export)
// ============================================================================
struct RoadMesh {
    QString            roadId;
    QVector<RoadVertex> vertices;
    QVector<int>        indices;   // triangle list

    // Centre-line positions (used for wall/kerb snapping, AI line seeding)
    QVector<QVector3D>  centreLine;
    QVector<QVector3D>  centreNormals;
    QVector<float>      cumulativeDist;   // arc length per centre point
    float               totalLength = 0.f;

    bool isEmpty() const { return vertices.isEmpty(); }
};

// ============================================================================
// Kerb mesh (attached to a RoadMesh)
// ============================================================================
struct KerbMesh {
    QString           kerbId;
    QVector<RoadVertex> vertices;
    QVector<int>        indices;
};

// ============================================================================
// RoadBuilder
// ============================================================================
class RoadBuilder : public QObject
{
    Q_OBJECT
public:
    explicit RoadBuilder(QObject* parent = nullptr);

    // ---- Settings ----------------------------------------------------------
    void setTerrainSampleFn(std::function<float(float,float)> fn) { m_terrainSample = fn; }
    void setSnapToTerrain(bool snap) { m_snapToTerrain = snap; }

    // ---- Build road mesh from Road data ------------------------------------
    RoadMesh buildRoad(const Road& road) const;

    // ---- Build kerb mesh (requires parent RoadMesh) ------------------------
    KerbMesh buildKerb(const Kerb& kerb, const RoadMesh& parentRoad) const;

    // ---- Build wall mesh ---------------------------------------------------
    RoadMesh buildWall(const Wall& wall,
                       const RoadMesh* snapRoad = nullptr) const;

    // ---- Build surface polygon mesh ----------------------------------------
    RoadMesh buildSurface(const Surface& surface,
                          std::function<float(float,float)> heightFn) const;

    // ---- AI line auto-generation from road centre lines --------------------
    AILine autoAILine(const QVector<RoadMesh>& roads,
                      bool closeLoop = true) const;

    // ---- Utilities ---------------------------------------------------------
    // Evaluate Catmull-Rom at t in [0,1] over the full spline
    QVector3D evalSpline(const QVector<SplinePoint>& pts, float t) const;
    float     splineWidth(const QVector<SplinePoint>& pts, float t) const;
    float     splineCamberL(const QVector<SplinePoint>& pts, float t) const;
    float     splineCamberR(const QVector<SplinePoint>& pts, float t) const;

    // Returns tessellated centre points at ~tessResolution spacing
    QVector<QVector3D> tessellateCentreLine(const Road& road) const;

signals:
    void buildProgress(int percent);

private:
    // Catmull-Rom basis
    QVector3D catmullRom(QVector3D p0, QVector3D p1,
                          QVector3D p2, QVector3D p3, float t) const;
    float     lerpAtT(const QVector<SplinePoint>& pts, float t,
                       std::function<float(const SplinePoint&)> get) const;
    void      buildTriStrip(QVector<RoadVertex>& verts, QVector<int>& idx,
                             int row, int cols) const;

    std::function<float(float,float)> m_terrainSample;
    bool m_snapToTerrain = true;
};

}} // namespace ks::track
