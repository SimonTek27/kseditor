#pragma once
// ============================================================================
// TrackBuilderTypes.h
// All shared data structures for the ksTrackBuilder module.
// Mirrors TreCorsa's domain model: Road, Kerb, Wall, Surface, Prop, Light,
// Terrain, AILine, StartPit, PhysicsRoad.
// ============================================================================

#include <QVector>
#include <QVector2D>
#include <QVector3D>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QColor>

namespace ks {
namespace track {

// ============================================================================
// Spline point with optional camber and width override
// ============================================================================
struct SplinePoint {
    QVector3D position;   // world XYZ (metres)
    float     width      = 10.f;  // road half-width in metres
    float     camberL    = 0.f;   // left camber angle degrees (positive = banked in)
    float     camberR    = 0.f;
    float     grade      = 0.f;   // longitudinal slope override (degrees)
    bool      smoothTan  = true;  // use catmull-rom tangent

    QJsonObject toJson() const {
        QJsonObject o;
        o["x"] = position.x(); o["y"] = position.y(); o["z"] = position.z();
        o["w"] = width; o["cL"] = camberL; o["cR"] = camberR;
        o["grade"] = grade; o["smooth"] = smoothTan;
        return o;
    }
    static SplinePoint fromJson(const QJsonObject& o) {
        SplinePoint p;
        p.position  = { float(o["x"].toDouble()), float(o["y"].toDouble()), float(o["z"].toDouble()) };
        p.width     = float(o["w"].toDouble(10.f));
        p.camberL   = float(o["cL"].toDouble());
        p.camberR   = float(o["cR"].toDouble());
        p.grade     = float(o["grade"].toDouble());
        p.smoothTan = o["smooth"].toBool(true);
        return p;
    }
};

// ============================================================================
// Road segment
// ============================================================================
enum class SurfaceType { Asphalt, Concrete, Gravel, Dirt, Grass, Sand, Ice };
enum class RoadProfile  { Flat, Crowned, Custom };

struct Road {
    QString             id;
    QString             name         = "Road";
    QVector<SplinePoint> points;
    SurfaceType         surface      = SurfaceType::Asphalt;
    RoadProfile         profile      = RoadProfile::Crowned;
    float               crownHeight  = 0.05f;  // crown apex height (m)
    float               textureScale = 1.f;
    bool                isBridge     = false;
    float               bridgeHeight = 0.f;
    QString             materialId;

    // Tessellation resolution (metres per segment)
    float tessResolution = 2.f;

    QJsonObject toJson() const;
    static Road fromJson(const QJsonObject& o);
};

// ============================================================================
// Kerb
// ============================================================================
enum class KerbStyle { Sausage, Flat, Rumble, Piano, WaveRed, WaveWhite };

struct Kerb {
    QString            id;
    QString            roadId;       // parent road
    bool               leftSide     = true;
    KerbStyle          style        = KerbStyle::Sausage;
    float              width        = 0.3f;
    float              height       = 0.05f;
    QString            colorA       = "#FF0000";
    QString            colorB       = "#FFFFFF";
    QVector<int>       pointIndices; // which road points carry this kerb

    QJsonObject toJson() const;
    static Kerb fromJson(const QJsonObject& o);
};

// ============================================================================
// Wall / barrier
// ============================================================================
enum class WallType { Concrete, TireStack, Armco, Wood, Mesh, Invisible };

struct Wall {
    QString           id;
    QString           name       = "Wall";
    QVector<QVector3D> points;
    WallType          type       = WallType::Concrete;
    float             height     = 1.2f;
    float             thickness  = 0.3f;
    QString           textureId;
    bool              snapToRoad = false;
    QString           snapRoadId;
    bool              snapLeft   = true;

    QJsonObject toJson() const;
    static Wall fromJson(const QJsonObject& o);
};

// ============================================================================
// Surface (parking lots, run-off areas)
// ============================================================================
struct Surface {
    QString           id;
    QString           name = "Surface";
    QVector<QVector2D> polygon;   // XZ polygon (Y from terrain)
    SurfaceType       surface   = SurfaceType::Asphalt;
    float             elevation = 0.f;
    QString           materialId;

    QJsonObject toJson() const;
    static Surface fromJson(const QJsonObject& o);
};

// ============================================================================
// Prop (sign, barrier, cone, tree…)
// ============================================================================
enum class PropCategory { Sign, Barrier, Cone, Tree2D, Tree3D, Rock, Generic };

struct Prop {
    QString     id;
    QString     name;
    QString     modelId;        // asset bank model reference
    PropCategory category      = PropCategory::Generic;
    QVector3D   position;
    QVector3D   rotation;       // Euler degrees
    QVector3D   scale           = {1.f, 1.f, 1.f};
    bool        castsShadow     = true;
    bool        collidable      = true;

    QJsonObject toJson() const;
    static Prop fromJson(const QJsonObject& o);
};

// ============================================================================
// Light
// ============================================================================
enum class LightType { Street, Floodlight, Ambient, Spot };

struct Light {
    QString   id;
    LightType type       = LightType::Street;
    QVector3D position;
    QVector3D direction  = {0.f, -1.f, 0.f};
    QColor    color      = Qt::white;
    float     intensity  = 1.f;
    float     range      = 20.f;
    float     spotAngle  = 45.f;    // for Spot only
    bool      enabled    = true;

    QJsonObject toJson() const;
    static Light fromJson(const QJsonObject& o);
};

// ============================================================================
// Start / Pit positions
// ============================================================================
struct StartPosition {
    int       index = 0;
    QVector3D position;
    float     yaw   = 0.f;

    QJsonObject toJson() const;
    static StartPosition fromJson(const QJsonObject& o);
};

struct PitPosition {
    int       index = 0;
    QVector3D position;
    float     yaw   = 0.f;
    QVector3D driveInPoint;
    QVector3D driveOutPoint;

    QJsonObject toJson() const;
    static PitPosition fromJson(const QJsonObject& o);
};

// ============================================================================
// AI Line
// ============================================================================
struct AILinePoint {
    QVector3D position;
    float     speed        = 100.f;  // km/h hint
    float     width        = 3.f;    // corridor half-width
    float     accel        = 1.f;    // 0–1 acceleration zone
    float     brake        = 0.f;    // 0–1 braking zone

    QJsonObject toJson() const;
    static AILinePoint fromJson(const QJsonObject& o);
};

struct AILine {
    QString             id;
    QVector<AILinePoint> points;
    bool                isLoop = true;

    QJsonObject toJson() const;
    static AILine fromJson(const QJsonObject& o);
};

// ============================================================================
// Physics road (Noise Labs)
// ============================================================================
struct PhysicsRoad {
    QString   id;
    QString   roadId;          // link to Road
    float     noiseAmplitude  = 0.002f;
    float     noiseFrequency  = 10.f;
    float     bumpiness       = 0.5f;
    float     gripMultiplier  = 1.f;
    float     ffbMultiplier   = 1.f;
    bool      hasPitLane      = false;

    QJsonObject toJson() const;
    static PhysicsRoad fromJson(const QJsonObject& o);
};

// ============================================================================
// Terrain
// ============================================================================
struct TerrainConfig {
    int   gridWidth    = 512;
    int   gridHeight   = 512;
    float worldWidth   = 2000.f;  // metres
    float worldHeight  = 2000.f;
    float minElevation = -50.f;
    float maxElevation = 200.f;

    // Satellite import metadata
    double  geoLat     = 0.0;
    double  geoLon     = 0.0;
    double  geoZoom    = 16.0;
    bool    hasSatellite = false;

    QJsonObject toJson() const;
    static TerrainConfig fromJson(const QJsonObject& o);
};


// ============================================================================
// Timing sector (for extension.ini / time_attack.ini)
// AC supports up to 3 splits (sectors 1, 2, 3 = finish)
// ============================================================================
struct TimingSector {
    int       index = 0;        // 0 = sector 1 (first split), 1 = sector 2, 2 = finish
    QVector3D position;
    float     yaw   = 0.f;      // heading degrees
    float     width = 20.f;     // trigger box width (metres)
    bool      isFinish = false;

    QJsonObject toJson() const {
        QJsonObject o;
        o["idx"] = index; o["x"] = position.x(); o["y"] = position.y(); o["z"] = position.z();
        o["yaw"] = yaw; o["width"] = width; o["finish"] = isFinish;
        return o;
    }
    static TimingSector fromJson(const QJsonObject& o) {
        TimingSector s;
        s.index    = o["idx"].toInt();
        s.position = {float(o["x"].toDouble()), float(o["y"].toDouble()), float(o["z"].toDouble())};
        s.yaw      = float(o["yaw"].toDouble());
        s.width    = float(o["width"].toDouble(20.f));
        s.isFinish = o["finish"].toBool();
        return s;
    }
};

// ============================================================================
// Groove data (rubber buildup per road segment)
// Controls the slippery/grippy groove progression AC simulates
// ============================================================================
struct GrooveData {
    QString roadId;
    float   maxGrip   = 1.05f;  // max grip relative to base surface
    float   minGrip   = 0.98f;  // wet / off-line grip
    float   buildRate = 0.1f;   // how quickly groove builds (0=slow, 1=instant)
    float   width     = 3.0f;   // groove width in metres (centred on ideal line)

    QJsonObject toJson() const {
        QJsonObject o; o["road"]=roadId; o["maxGrip"]=maxGrip;
        o["minGrip"]=minGrip; o["rate"]=buildRate; o["width"]=width;
        return o;
    }
    static GrooveData fromJson(const QJsonObject& o) {
        GrooveData g; g.roadId=o["road"].toString();
        g.maxGrip=float(o["maxGrip"].toDouble(1.05));
        g.minGrip=float(o["minGrip"].toDouble(0.98));
        g.buildRate=float(o["rate"].toDouble(0.1));
        g.width=float(o["width"].toDouble(3.0));
        return g;
    }
};

// ============================================================================
// VAO bake settings (Ambient Occlusion patch)
// ============================================================================
struct VAOSettings {
    bool    enabled       = false;
    int     rayCount      = 256;
    float   maxDistance   = 5.0f;   // metres
    float   bias          = 0.001f;
    int     textureSize   = 1024;
    QString outputPatch   = "vao_patch.vao"; // relative to track folder
};

// ============================================================================
// Track project (top-level document)
// ============================================================================
struct TrackProject {
    QString                name        = "New Track";
    QString                author;
    QString                description;
    QString                location;       // real-world location name
    TerrainConfig          terrain;
    QVector<Road>          roads;
    QVector<Kerb>          kerbs;
    QVector<Wall>          walls;
    QVector<Surface>       surfaces;
    QVector<Prop>          props;
    QVector<Light>         lights;
    QVector<StartPosition> startPositions;
    QVector<PitPosition>   pitPositions;
    AILine                 aiLine;
    QVector<PhysicsRoad>   physicsRoads;
    QVector<TimingSector>  timingSectors;
    QVector<GrooveData>    grooves;
    VAOSettings            vao;
    QVector<float>         heightmap;     // flat grid, gridWidth*gridHeight floats

    QJsonObject toJson() const;
    static TrackProject fromJson(const QJsonObject& o);

    // Helpers
    bool isEmpty() const { return roads.isEmpty() && heightmap.isEmpty(); }
};

} // namespace track
} // namespace ks
