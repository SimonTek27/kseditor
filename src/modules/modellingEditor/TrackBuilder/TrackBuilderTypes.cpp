#include "TrackBuilderTypes.h"
#include <QJsonDocument>

namespace ks { namespace track {

// ---- helpers ---------------------------------------------------------------
static QString surfaceStr(SurfaceType s) {
    switch(s) {
    case SurfaceType::Asphalt:  return "asphalt";
    case SurfaceType::Concrete: return "concrete";
    case SurfaceType::Gravel:   return "gravel";
    case SurfaceType::Dirt:     return "dirt";
    case SurfaceType::Grass:    return "grass";
    case SurfaceType::Sand:     return "sand";
    case SurfaceType::Ice:      return "ice";
    }
    return "asphalt";
}
static SurfaceType surfaceFromStr(const QString& s) {
    if (s=="concrete") return SurfaceType::Concrete;
    if (s=="gravel")   return SurfaceType::Gravel;
    if (s=="dirt")     return SurfaceType::Dirt;
    if (s=="grass")    return SurfaceType::Grass;
    if (s=="sand")     return SurfaceType::Sand;
    if (s=="ice")      return SurfaceType::Ice;
    return SurfaceType::Asphalt;
}
static QJsonObject vec3(QVector3D v){ return {{"x",v.x()},{"y",v.y()},{"z",v.z()}}; }
static QVector3D   vec3(const QJsonObject& o){ return {float(o["x"].toDouble()),float(o["y"].toDouble()),float(o["z"].toDouble())}; }

// ============================================================================
// Road
// ============================================================================
QJsonObject Road::toJson() const {
    QJsonObject o;
    o["id"]=id; o["name"]=name; o["surface"]=surfaceStr(surface);
    o["profile"]=(int)profile; o["crownHeight"]=crownHeight;
    o["textureScale"]=textureScale; o["bridge"]=isBridge;
    o["bridgeHeight"]=bridgeHeight; o["material"]=materialId;
    o["tessRes"]=tessResolution;
    QJsonArray pts;
    for (auto& p : points) pts.append(p.toJson());
    o["points"]=pts;
    return o;
}
Road Road::fromJson(const QJsonObject& o) {
    Road r;
    r.id=o["id"].toString(); r.name=o["name"].toString("Road");
    r.surface=surfaceFromStr(o["surface"].toString());
    r.profile=(RoadProfile)o["profile"].toInt();
    r.crownHeight=float(o["crownHeight"].toDouble(0.05));
    r.textureScale=float(o["textureScale"].toDouble(1));
    r.isBridge=o["bridge"].toBool();
    r.bridgeHeight=float(o["bridgeHeight"].toDouble());
    r.materialId=o["material"].toString();
    r.tessResolution=float(o["tessRes"].toDouble(2));
    for (auto v : o["points"].toArray())
        r.points.append(SplinePoint::fromJson(v.toObject()));
    return r;
}

// ============================================================================
// Kerb
// ============================================================================
QJsonObject Kerb::toJson() const {
    QJsonObject o;
    o["id"]=id; o["roadId"]=roadId; o["left"]=leftSide;
    o["style"]=(int)style; o["width"]=width; o["height"]=height;
    o["colorA"]=colorA; o["colorB"]=colorB;
    QJsonArray idx;
    for (int i : pointIndices) idx.append(i);
    o["indices"]=idx;
    return o;
}
Kerb Kerb::fromJson(const QJsonObject& o) {
    Kerb k;
    k.id=o["id"].toString(); k.roadId=o["roadId"].toString();
    k.leftSide=o["left"].toBool(true);
    k.style=(KerbStyle)o["style"].toInt();
    k.width=float(o["width"].toDouble(0.3));
    k.height=float(o["height"].toDouble(0.05));
    k.colorA=o["colorA"].toString("#FF0000");
    k.colorB=o["colorB"].toString("#FFFFFF");
    for (auto v : o["indices"].toArray()) k.pointIndices.append(v.toInt());
    return k;
}

// ============================================================================
// Wall
// ============================================================================
QJsonObject Wall::toJson() const {
    QJsonObject o;
    o["id"]=id; o["name"]=name; o["type"]=(int)type;
    o["height"]=height; o["thickness"]=thickness; o["texture"]=textureId;
    o["snapToRoad"]=snapToRoad; o["snapRoadId"]=snapRoadId; o["snapLeft"]=snapLeft;
    QJsonArray pts;
    for (auto& p : points) pts.append(::ks::track::vec3(p));
    o["points"]=pts;
    return o;
}
Wall Wall::fromJson(const QJsonObject& o) {
    Wall w;
    w.id=o["id"].toString(); w.name=o["name"].toString("Wall");
    w.type=(WallType)o["type"].toInt();
    w.height=float(o["height"].toDouble(1.2));
    w.thickness=float(o["thickness"].toDouble(0.3));
    w.textureId=o["texture"].toString();
    w.snapToRoad=o["snapToRoad"].toBool();
    w.snapRoadId=o["snapRoadId"].toString();
    w.snapLeft=o["snapLeft"].toBool(true);
    for (auto v : o["points"].toArray())
        w.points.append(::ks::track::vec3(v.toObject()));
    return w;
}

// ============================================================================
// Surface
// ============================================================================
QJsonObject Surface::toJson() const {
    QJsonObject o;
    o["id"]=id; o["name"]=name; o["surface"]=surfaceStr(surface);
    o["elevation"]=elevation; o["material"]=materialId;
    QJsonArray poly;
    for (auto& p : polygon) poly.append(QJsonObject{{"x",p.x()},{"z",p.y()}});
    o["polygon"]=poly;
    return o;
}
Surface Surface::fromJson(const QJsonObject& o) {
    Surface s;
    s.id=o["id"].toString(); s.name=o["name"].toString("Surface");
    s.surface=surfaceFromStr(o["surface"].toString());
    s.elevation=float(o["elevation"].toDouble());
    s.materialId=o["material"].toString();
    for (auto v : o["polygon"].toArray()) {
        auto p=v.toObject();
        s.polygon.append({float(p["x"].toDouble()),float(p["z"].toDouble())});
    }
    return s;
}

// ============================================================================
// Prop
// ============================================================================
QJsonObject Prop::toJson() const {
    QJsonObject o;
    o["id"]=id; o["name"]=name; o["modelId"]=modelId;
    o["category"]=(int)category;
    o["pos"]=::ks::track::vec3(position); o["rot"]=::ks::track::vec3(rotation);
    o["scale"]=::ks::track::vec3(scale);
    o["shadow"]=castsShadow; o["collide"]=collidable;
    return o;
}
Prop Prop::fromJson(const QJsonObject& o) {
    Prop p;
    p.id=o["id"].toString(); p.name=o["name"].toString();
    p.modelId=o["modelId"].toString();
    p.category=(PropCategory)o["category"].toInt();
    p.position=::ks::track::vec3(o["pos"].toObject());
    p.rotation=::ks::track::vec3(o["rot"].toObject());
    p.scale=::ks::track::vec3(o["scale"].toObject());
    if (p.scale.isNull()) p.scale={1,1,1};
    p.castsShadow=o["shadow"].toBool(true);
    p.collidable=o["collide"].toBool(true);
    return p;
}

// ============================================================================
// Light
// ============================================================================
QJsonObject Light::toJson() const {
    QJsonObject o;
    o["id"]=id; o["type"]=(int)type;
    o["pos"]=::ks::track::vec3(position); o["dir"]=::ks::track::vec3(direction);
    o["color"]=color.name(); o["intensity"]=intensity;
    o["range"]=range; o["spotAngle"]=spotAngle; o["enabled"]=enabled;
    return o;
}
Light Light::fromJson(const QJsonObject& o) {
    Light l;
    l.id=o["id"].toString(); l.type=(LightType)o["type"].toInt();
    l.position=::ks::track::vec3(o["pos"].toObject());
    l.direction=::ks::track::vec3(o["dir"].toObject());
    l.color=QColor(o["color"].toString("#ffffff"));
    l.intensity=float(o["intensity"].toDouble(1));
    l.range=float(o["range"].toDouble(20));
    l.spotAngle=float(o["spotAngle"].toDouble(45));
    l.enabled=o["enabled"].toBool(true);
    return l;
}

// ============================================================================
// StartPosition / PitPosition
// ============================================================================
QJsonObject StartPosition::toJson() const {
    return {{"index",index},{"pos",::ks::track::vec3(position)},{"yaw",yaw}};
}
StartPosition StartPosition::fromJson(const QJsonObject& o) {
    StartPosition s;
    s.index=o["index"].toInt();
    s.position=::ks::track::vec3(o["pos"].toObject());
    s.yaw=float(o["yaw"].toDouble());
    return s;
}
QJsonObject PitPosition::toJson() const {
    return {{"index",index},{"pos",::ks::track::vec3(position)},{"yaw",yaw},
            {"in",::ks::track::vec3(driveInPoint)},{"out",::ks::track::vec3(driveOutPoint)}};
}
PitPosition PitPosition::fromJson(const QJsonObject& o) {
    PitPosition p;
    p.index=o["index"].toInt();
    p.position=::ks::track::vec3(o["pos"].toObject());
    p.yaw=float(o["yaw"].toDouble());
    p.driveInPoint=::ks::track::vec3(o["in"].toObject());
    p.driveOutPoint=::ks::track::vec3(o["out"].toObject());
    return p;
}

// ============================================================================
// AILine
// ============================================================================
QJsonObject AILinePoint::toJson() const {
    return {{"pos",::ks::track::vec3(position)},{"speed",speed},
            {"width",width},{"accel",accel},{"brake",brake}};
}
AILinePoint AILinePoint::fromJson(const QJsonObject& o) {
    AILinePoint p;
    p.position=::ks::track::vec3(o["pos"].toObject());
    p.speed=float(o["speed"].toDouble(100));
    p.width=float(o["width"].toDouble(3));
    p.accel=float(o["accel"].toDouble(1));
    p.brake=float(o["brake"].toDouble());
    return p;
}
QJsonObject AILine::toJson() const {
    QJsonArray pts;
    for (auto& p : points) pts.append(p.toJson());
    return {{"id",id},{"points",pts},{"loop",isLoop}};
}
AILine AILine::fromJson(const QJsonObject& o) {
    AILine l;
    l.id=o["id"].toString(); l.isLoop=o["loop"].toBool(true);
    for (auto v : o["points"].toArray()) l.points.append(AILinePoint::fromJson(v.toObject()));
    return l;
}

// ============================================================================
// PhysicsRoad
// ============================================================================
QJsonObject PhysicsRoad::toJson() const {
    return {{"id",id},{"roadId",roadId},
            {"noiseAmp",noiseAmplitude},{"noiseFreq",noiseFrequency},
            {"bump",bumpiness},{"grip",gripMultiplier},
            {"ffb",ffbMultiplier},{"pit",hasPitLane}};
}
PhysicsRoad PhysicsRoad::fromJson(const QJsonObject& o) {
    PhysicsRoad p;
    p.id=o["id"].toString(); p.roadId=o["roadId"].toString();
    p.noiseAmplitude=float(o["noiseAmp"].toDouble(0.002));
    p.noiseFrequency=float(o["noiseFreq"].toDouble(10));
    p.bumpiness=float(o["bump"].toDouble(0.5));
    p.gripMultiplier=float(o["grip"].toDouble(1));
    p.ffbMultiplier=float(o["ffb"].toDouble(1));
    p.hasPitLane=o["pit"].toBool();
    return p;
}

// ============================================================================
// TerrainConfig
// ============================================================================
QJsonObject TerrainConfig::toJson() const {
    return {{"gw",gridWidth},{"gh",gridHeight},
            {"ww",worldWidth},{"wh",worldHeight},
            {"minEl",minElevation},{"maxEl",maxElevation},
            {"lat",geoLat},{"lon",geoLon},{"zoom",geoZoom},{"sat",hasSatellite}};
}
TerrainConfig TerrainConfig::fromJson(const QJsonObject& o) {
    TerrainConfig t;
    t.gridWidth=o["gw"].toInt(512); t.gridHeight=o["gh"].toInt(512);
    t.worldWidth=float(o["ww"].toDouble(2000)); t.worldHeight=float(o["wh"].toDouble(2000));
    t.minElevation=float(o["minEl"].toDouble(-50)); t.maxElevation=float(o["maxEl"].toDouble(200));
    t.geoLat=o["lat"].toDouble(); t.geoLon=o["lon"].toDouble(); t.geoZoom=o["zoom"].toDouble(16);
    t.hasSatellite=o["sat"].toBool();
    return t;
}

// ============================================================================
// TrackProject
// ============================================================================
QJsonObject TrackProject::toJson() const {
    QJsonObject o;
    o["name"]=name; o["author"]=author; o["description"]=description;
    o["location"]=location; o["terrain"]=terrain.toJson();
    o["aiLine"]=aiLine.toJson();

    auto toArr = [](auto& vec){ QJsonArray a; for(auto& v:vec) a.append(v.toJson()); return a; };
    o["roads"]=toArr(roads); o["kerbs"]=toArr(kerbs); o["walls"]=toArr(walls);
    o["surfaces"]=toArr(surfaces); o["props"]=toArr(props); o["lights"]=toArr(lights);
    o["startPos"]=toArr(startPositions); o["pitPos"]=toArr(pitPositions);
    o["physicsRoads"]=toArr(physicsRoads);

    QJsonArray hm;
    for (float f : heightmap) hm.append(f);
    o["heightmap"]=hm;
    return o;
}

TrackProject TrackProject::fromJson(const QJsonObject& o) {
    TrackProject p;
    p.name=o["name"].toString("New Track");
    p.author=o["author"].toString();
    p.description=o["description"].toString();
    p.location=o["location"].toString();
    p.terrain=TerrainConfig::fromJson(o["terrain"].toObject());
    p.aiLine=AILine::fromJson(o["aiLine"].toObject());

    for(auto v:o["roads"].toArray())      p.roads.append(Road::fromJson(v.toObject()));
    for(auto v:o["kerbs"].toArray())      p.kerbs.append(Kerb::fromJson(v.toObject()));
    for(auto v:o["walls"].toArray())      p.walls.append(Wall::fromJson(v.toObject()));
    for(auto v:o["surfaces"].toArray())   p.surfaces.append(Surface::fromJson(v.toObject()));
    for(auto v:o["props"].toArray())      p.props.append(Prop::fromJson(v.toObject()));
    for(auto v:o["lights"].toArray())     p.lights.append(Light::fromJson(v.toObject()));
    for(auto v:o["startPos"].toArray())   p.startPositions.append(StartPosition::fromJson(v.toObject()));
    for(auto v:o["pitPos"].toArray())     p.pitPositions.append(PitPosition::fromJson(v.toObject()));
    for(auto v:o["physicsRoads"].toArray()) p.physicsRoads.append(PhysicsRoad::fromJson(v.toObject()));
    for(auto v:o["heightmap"].toArray())  p.heightmap.append(float(v.toDouble()));
    return p;
}

}} // namespace ks::track
