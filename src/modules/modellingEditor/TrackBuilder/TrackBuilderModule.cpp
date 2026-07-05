#include "TrackBuilderModule.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QImage>
#include <QDebug>
#include <QRegularExpression>

namespace ks { namespace track {

TrackBuilderModule* TrackBuilderModule::s_instance = nullptr;

TrackBuilderModule::TrackBuilderModule(QObject* parent) : QObject(parent)
{
    s_instance = this;

    m_terrain     = new TerrainEngine(this);
    m_roadBuilder = new RoadBuilder(this);
    m_exporter    = new TrackExporter(this);

    connect(m_terrain, &TerrainEngine::modified,
            this, [this](){ emit terrainModified(); markDirty(); });

    connect(m_exporter, &TrackExporter::exportProgress, this, &TrackBuilderModule::exportProgress);
    connect(m_exporter, &TrackExporter::exportDone,     this, &TrackBuilderModule::exportDone);
    connect(m_exporter, &TrackExporter::exportFailed,   this, &TrackBuilderModule::exportFailed);

    m_exporter->setTerrainEngine(m_terrain);
    m_exporter->setRoadBuilder(m_roadBuilder);
}

TrackBuilderModule::~TrackBuilderModule() {}

// ============================================================================
// Project lifecycle
// ============================================================================
void TrackBuilderModule::newProject(const QString& name,
                                     int gridW, int gridH,
                                     float worldW, float worldH)
{
    m_project = TrackProject();
    m_project.name = name;
    m_project.terrain.gridWidth  = gridW;
    m_project.terrain.gridHeight = gridH;
    m_project.terrain.worldWidth  = worldW;
    m_project.terrain.worldHeight = worldH;

    m_terrain->init(m_project.terrain);
    m_dirty = false;
    emit projectChanged();
    emit dirtyChanged();
}

bool TrackBuilderModule::loadProject(const QString& filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        emit error("Cannot open: "+filePath);
        return false;
    }
    QJsonParseError pe;
    auto doc=QJsonDocument::fromJson(f.readAll(),&pe);
    if (pe.error!=QJsonParseError::NoError) {
        emit error("JSON parse error: "+pe.errorString());
        return false;
    }
    m_project=TrackProject::fromJson(doc.object());
    m_terrain->init(m_project.terrain);
    if (!m_project.heightmap.isEmpty())
        m_terrain->importFromRaw(m_project.heightmap,
                                  m_project.terrain.gridWidth,
                                  m_project.terrain.gridHeight);
    m_dirty=false;
    emit projectChanged();
    emit dirtyChanged();
    return true;
}

bool TrackBuilderModule::saveProject(const QString& filePath)
{
    // Snapshot current heightmap
    m_project.heightmap=m_terrain->exportHeightmap();
    m_project.terrain=m_terrain->config();   // sync config

    QJsonDocument doc(m_project.toJson());
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly)) {
        emit error("Cannot write: "+filePath);
        return false;
    }
    f.write(doc.toJson());
    m_dirty=false;
    emit dirtyChanged();
    return true;
}

bool TrackBuilderModule::exportToAC(const QString& outputDir, bool zipIt)
{
    m_project.heightmap=m_terrain->exportHeightmap();
    m_exporter->setOutputDir(outputDir);
    m_exporter->setZipOutput(zipIt);
    return m_exporter->exportTrack(m_project);
}

// ============================================================================
// Terrain tools
// ============================================================================
void TrackBuilderModule::setTerrainBrushMode(const QString& mode)
{
    using BM=TerrainEngine::BrushMode;
    if      (mode=="raise")   m_terrain->setBrushMode(BM::Raise);
    else if (mode=="lower")   m_terrain->setBrushMode(BM::Lower);
    else if (mode=="smooth")  m_terrain->setBrushMode(BM::Smooth);
    else if (mode=="flatten") m_terrain->setBrushMode(BM::Flatten);
    else if (mode=="noise")   m_terrain->setBrushMode(BM::Noise);
    else if (mode=="ramp")    m_terrain->setBrushMode(BM::Ramp);
    else if (mode=="erosion") m_terrain->setBrushMode(BM::Erosion);
}

bool TrackBuilderModule::importTerrainFromImage(const QString& path,
                                                 float minH, float maxH)
{
    QImage img(path);
    if (img.isNull()) { emit error("Cannot load image: "+path); return false; }
    return m_terrain->importFromImage(img,minH,maxH);
}

int TrackBuilderModule::addTerrainLayer(const QString& name,
                                         const QString& textureId, float uvScale)
{
    return m_terrain->addLayer(name,textureId,uvScale);
}

void TrackBuilderModule::paintTerrainLayer(int layer,float wx,float wz,float r,float opacity)
{
    m_terrain->paintLayer(layer,wx,wz,r,opacity);
    markDirty();
}

void TrackBuilderModule::autoMaskBySlope(int layer,float minDeg,float maxDeg)
{
    m_terrain->autoMaskBySlope(layer,minDeg,maxDeg);
    markDirty();
}

// ============================================================================
// Road tools
// ============================================================================
QString TrackBuilderModule::addRoad(const QString& name)
{
    Road r; r.id=newId(); r.name=name;
    m_project.roads.append(r);
    markDirty();
    return r.id;
}

bool TrackBuilderModule::removeRoad(const QString& id)
{
    for (int i=0;i<m_project.roads.size();++i)
        if (m_project.roads[i].id==id){ m_project.roads.removeAt(i); markDirty(); return true; }
    return false;
}

bool TrackBuilderModule::addRoadPoint(const QString& roadId,float x,float y,float z,
                                       float width,float camberL,float camberR)
{
    Road* r=findRoad(roadId); if(!r) return false;
    SplinePoint p;
    p.position={x,y,z}; p.width=width; p.camberL=camberL; p.camberR=camberR;
    // Snap Y to terrain if not explicitly set
    if (y==0.f && m_terrain)
        p.position.setY(m_terrain->getHeightWorld(x,z));
    r->points.append(p);
    markDirty(); return true;
}

bool TrackBuilderModule::updateRoadPoint(const QString& roadId,int idx,
                                          float x,float y,float z,
                                          float width,float camberL,float camberR)
{
    Road* r=findRoad(roadId); if(!r||idx<0||idx>=r->points.size()) return false;
    r->points[idx].position={x,y,z};
    r->points[idx].width=width;
    r->points[idx].camberL=camberL;
    r->points[idx].camberR=camberR;
    markDirty(); return true;
}

bool TrackBuilderModule::removeRoadPoint(const QString& roadId,int idx)
{
    Road* r=findRoad(roadId); if(!r||idx<0||idx>=r->points.size()) return false;
    r->points.removeAt(idx); markDirty(); return true;
}

void TrackBuilderModule::setRoadSurface(const QString& roadId,const QString& surface)
{
    Road* r=findRoad(roadId); if(!r) return;
    if      (surface=="concrete") r->surface=SurfaceType::Concrete;
    else if (surface=="gravel")   r->surface=SurfaceType::Gravel;
    else if (surface=="dirt")     r->surface=SurfaceType::Dirt;
    else if (surface=="grass")    r->surface=SurfaceType::Grass;
    else if (surface=="sand")     r->surface=SurfaceType::Sand;
    else if (surface=="ice")      r->surface=SurfaceType::Ice;
    else                          r->surface=SurfaceType::Asphalt;
    markDirty();
}

void TrackBuilderModule::setRoadBridge(const QString& roadId,bool bridge,float height)
{
    Road* r=findRoad(roadId); if(!r) return;
    r->isBridge=bridge; r->bridgeHeight=height; markDirty();
}

QStringList TrackBuilderModule::roadIds() const
{
    QStringList ids;
    for (const auto& r:m_project.roads) ids<<r.id;
    return ids;
}

// ============================================================================
// Kerb tools
// ============================================================================
QString TrackBuilderModule::addKerb(const QString& roadId,bool leftSide,const QString& style)
{
    Kerb k; k.id=newId(); k.roadId=roadId; k.leftSide=leftSide;
    if      (style=="Flat")       k.style=KerbStyle::Flat;
    else if (style=="Rumble")     k.style=KerbStyle::Rumble;
    else if (style=="Piano")      k.style=KerbStyle::Piano;
    else if (style=="WaveRed")    k.style=KerbStyle::WaveRed;
    else if (style=="WaveWhite")  k.style=KerbStyle::WaveWhite;
    else                          k.style=KerbStyle::Sausage;
    m_project.kerbs.append(k);
    markDirty(); return k.id;
}

bool TrackBuilderModule::removeKerb(const QString& id)
{
    for (int i=0;i<m_project.kerbs.size();++i)
        if (m_project.kerbs[i].id==id){ m_project.kerbs.removeAt(i); markDirty(); return true; }
    return false;
}

// ============================================================================
// Wall tools
// ============================================================================
QString TrackBuilderModule::addWall(const QString& name)
{
    Wall w; w.id=newId(); w.name=name;
    m_project.walls.append(w);
    markDirty(); return w.id;
}

bool TrackBuilderModule::addWallPoint(const QString& wallId,float x,float y,float z)
{
    Wall* w=findWall(wallId); if(!w) return false;
    float fy=(y==0.f&&m_terrain)?m_terrain->getHeightWorld(x,z):y;
    w->points.append({x,fy,z}); markDirty(); return true;
}

bool TrackBuilderModule::removeWall(const QString& id)
{
    for (int i=0;i<m_project.walls.size();++i)
        if (m_project.walls[i].id==id){ m_project.walls.removeAt(i); markDirty(); return true; }
    return false;
}

void TrackBuilderModule::setWallType(const QString& wallId,const QString& type)
{
    Wall* w=findWall(wallId); if(!w) return;
    if      (type=="TireStack")  w->type=WallType::TireStack;
    else if (type=="Armco")      w->type=WallType::Armco;
    else if (type=="Wood")       w->type=WallType::Wood;
    else if (type=="Mesh")       w->type=WallType::Mesh;
    else if (type=="Invisible")  w->type=WallType::Invisible;
    else                         w->type=WallType::Concrete;
    markDirty();
}

void TrackBuilderModule::snapWallToRoad(const QString& wallId,const QString& roadId,bool left)
{
    Wall* w=findWall(wallId); if(!w) return;
    w->snapToRoad=true; w->snapRoadId=roadId; w->snapLeft=left; markDirty();
}

// ============================================================================
// Surface tools
// ============================================================================
QString TrackBuilderModule::addSurface(const QString& name)
{
    Surface s; s.id=newId(); s.name=name;
    m_project.surfaces.append(s);
    markDirty(); return s.id;
}

bool TrackBuilderModule::addSurfacePoint(const QString& surfId,float x,float z)
{
    Surface* s=findSurface(surfId); if(!s) return false;
    s->polygon.append({x,z}); markDirty(); return true;
}

bool TrackBuilderModule::removeSurface(const QString& id)
{
    for (int i=0;i<m_project.surfaces.size();++i)
        if (m_project.surfaces[i].id==id){ m_project.surfaces.removeAt(i); markDirty(); return true; }
    return false;
}

// ============================================================================
// Props
// ============================================================================
QString TrackBuilderModule::addProp(const QString& modelId,float x,float y,float z,
                                     float yaw,const QString& name)
{
    Prop p; p.id=newId(); p.modelId=modelId;
    p.name=name.isEmpty()?modelId:name;
    p.position={x,y==0.f&&m_terrain?m_terrain->getHeightWorld(x,z):y,z};
    p.rotation={0.f,yaw,0.f};
    m_project.props.append(p);
    markDirty(); return p.id;
}

bool TrackBuilderModule::removeProp(const QString& id)
{
    for (int i=0;i<m_project.props.size();++i)
        if (m_project.props[i].id==id){ m_project.props.removeAt(i); markDirty(); return true; }
    return false;
}

bool TrackBuilderModule::moveProp(const QString& id,float x,float y,float z)
{
    Prop* p=findProp(id); if(!p) return false;
    p->position={x,y,z}; markDirty(); return true;
}

bool TrackBuilderModule::rotateProp(const QString& id,float rx,float ry,float rz)
{
    Prop* p=findProp(id); if(!p) return false;
    p->rotation={rx,ry,rz}; markDirty(); return true;
}

bool TrackBuilderModule::scaleProp(const QString& id,float sx,float sy,float sz)
{
    Prop* p=findProp(id); if(!p) return false;
    p->scale={sx,sy,sz}; markDirty(); return true;
}

// ============================================================================
// Lights
// ============================================================================
QString TrackBuilderModule::addLight(const QString& type,float x,float y,float z)
{
    Light l; l.id=newId(); l.position={x,y,z};
    if      (type=="Floodlight") l.type=LightType::Floodlight;
    else if (type=="Ambient")    l.type=LightType::Ambient;
    else if (type=="Spot")       l.type=LightType::Spot;
    else                         l.type=LightType::Street;
    m_project.lights.append(l);
    markDirty(); return l.id;
}

bool TrackBuilderModule::removeLight(const QString& id)
{
    for (int i=0;i<m_project.lights.size();++i)
        if (m_project.lights[i].id==id){ m_project.lights.removeAt(i); markDirty(); return true; }
    return false;
}

// ============================================================================
// Start / Pit
// ============================================================================
void TrackBuilderModule::addStartPosition(float x,float y,float z,float yaw)
{
    StartPosition sp;
    sp.index=m_project.startPositions.size();
    sp.position={x,y,z}; sp.yaw=yaw;
    m_project.startPositions.append(sp);
    markDirty();
}

void TrackBuilderModule::addPitPosition(float x,float y,float z,float yaw)
{
    PitPosition pp;
    pp.index=m_project.pitPositions.size();
    pp.position={x,y,z}; pp.yaw=yaw;
    pp.driveInPoint=pp.position+QVector3D(0,0,5);
    pp.driveOutPoint=pp.position-QVector3D(0,0,5);
    m_project.pitPositions.append(pp);
    markDirty();
}

void TrackBuilderModule::clearStartPositions(){ m_project.startPositions.clear(); markDirty(); }
void TrackBuilderModule::clearPitPositions()  { m_project.pitPositions.clear(); markDirty(); }

// ============================================================================
// AI line
// ============================================================================
void TrackBuilderModule::autoGenerateAILine()
{
    QVector<RoadMesh> meshes;
    for (const auto& road:m_project.roads)
        meshes.append(m_roadBuilder->buildRoad(road));
    m_project.aiLine=m_roadBuilder->autoAILine(meshes,true);
    markDirty();
}

void TrackBuilderModule::addAILinePoint(float x,float y,float z,float speed,float width)
{
    AILinePoint p; p.position={x,y,z}; p.speed=speed; p.width=width;
    m_project.aiLine.points.append(p);
    markDirty();
}

void TrackBuilderModule::clearAILine(){ m_project.aiLine.points.clear(); markDirty(); }

void TrackBuilderModule::smoothAILine(int passes)
{
    auto& pts=m_project.aiLine.points;
    for (int pass=0;pass<passes;++pass) {
        for (int i=1;i<pts.size()-1;++i) {
            QVector3D avg=(pts[i-1].position+pts[i].position+pts[i+1].position)/3.f;
            pts[i].position=pts[i].position*0.5f+avg*0.5f;
        }
    }
    markDirty();
}

// ============================================================================
// Physics roads
// ============================================================================
QString TrackBuilderModule::addPhysicsRoad(const QString& roadId)
{
    PhysicsRoad pr; pr.id=newId(); pr.roadId=roadId;
    m_project.physicsRoads.append(pr);
    markDirty(); return pr.id;
}

void TrackBuilderModule::setPhysicsRoadNoise(const QString& id,float amp,float freq)
{
    PhysicsRoad* pr=findPhysicsRoad(id); if(!pr) return;
    pr->noiseAmplitude=amp; pr->noiseFrequency=freq; markDirty();
}

void TrackBuilderModule::setPhysicsRoadGrip(const QString& id,float grip,float ffb)
{
    PhysicsRoad* pr=findPhysicsRoad(id); if(!pr) return;
    pr->gripMultiplier=grip; pr->ffbMultiplier=ffb; markDirty();
}

// ============================================================================
// Validation
// ============================================================================
QStringList TrackBuilderModule::exportValidationMessages() const
{
    QStringList msgs;
    if (m_project.roads.isEmpty())
        msgs<<"No roads defined – add at least one road.";
    for (const auto& r:m_project.roads)
        if (r.points.size()<2)
            msgs<<QString("Road \"%1\" has fewer than 2 points.").arg(r.name);
    if (m_project.startPositions.isEmpty())
        msgs<<"No start positions set – add at least one.";
    if (m_project.aiLine.points.isEmpty())
        msgs<<"No AI line – run Auto-Generate AI Line or add points manually.";
    if (msgs.isEmpty()) msgs<<"OK";
    return msgs;
}

// ============================================================================
// Lookup helpers
// ============================================================================
Road* TrackBuilderModule::findRoad(const QString& id)
{
    for (auto& r:m_project.roads) if(r.id==id) return &r;
    return nullptr;
}
Wall* TrackBuilderModule::findWall(const QString& id)
{
    for (auto& w:m_project.walls) if(w.id==id) return &w;
    return nullptr;
}
Surface* TrackBuilderModule::findSurface(const QString& id)
{
    for (auto& s:m_project.surfaces) if(s.id==id) return &s;
    return nullptr;
}
Prop* TrackBuilderModule::findProp(const QString& id)
{
    for (auto& p:m_project.props) if(p.id==id) return &p;
    return nullptr;
}
Light* TrackBuilderModule::findLight(const QString& id)
{
    for (auto& l:m_project.lights) if(l.id==id) return &l;
    return nullptr;
}
Kerb* TrackBuilderModule::findKerb(const QString& id)
{
    for (auto& k:m_project.kerbs) if(k.id==id) return &k;
    return nullptr;
}
PhysicsRoad* TrackBuilderModule::findPhysicsRoad(const QString& id)
{
    for (auto& p:m_project.physicsRoads) if(p.id==id) return &p;
    return nullptr;
}

}} // namespace ks::track
