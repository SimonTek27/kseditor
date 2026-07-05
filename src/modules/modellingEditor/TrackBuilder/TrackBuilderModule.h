#pragma once
// ============================================================================
// TrackBuilderModule.h
// Top-level controller for the ksTrackBuilder module.
// Integrates: TerrainEngine, RoadBuilder, TrackExporter.
// Exposes a clean API to the ksEditor UI (QML / Qt Widgets).
// ============================================================================

#include "TrackBuilderTypes.h"
#include "TerrainEngine.h"
#include "RoadBuilder.h"
#include "TrackExporter.h"
#include <QObject>
#include <QUuid>
#include <QJsonDocument>
#include <QFileInfo>

namespace ks { namespace track {

class TrackBuilderModule : public QObject
{
    Q_OBJECT

    // QML-exposed properties
    Q_PROPERTY(QString trackName   READ trackName   WRITE setTrackName   NOTIFY projectChanged)
    Q_PROPERTY(bool    isDirty     READ isDirty                          NOTIFY dirtyChanged)
    Q_PROPERTY(int     roadCount   READ roadCount                        NOTIFY projectChanged)
    Q_PROPERTY(int     propCount   READ propCount                        NOTIFY projectChanged)
    Q_PROPERTY(QString activeTool  READ activeTool  WRITE setActiveTool  NOTIFY activeToolChanged)

public:
    explicit TrackBuilderModule(QObject* parent = nullptr);
    ~TrackBuilderModule() override;

    static TrackBuilderModule* instance() { return s_instance; }

    // ---- Project lifecycle ------------------------------------------------
    Q_INVOKABLE void     newProject(const QString& name = "New Track",
                                    int gridW = 512, int gridH = 512,
                                    float worldW = 2000.f, float worldH = 2000.f);
    Q_INVOKABLE bool     loadProject(const QString& filePath);
    Q_INVOKABLE bool     saveProject(const QString& filePath);
    Q_INVOKABLE bool     exportToAC(const QString& outputDir, bool zipIt = true);

    // ---- Properties -------------------------------------------------------
    QString  trackName() const  { return m_project.name; }
    void     setTrackName(const QString& n){ m_project.name=n; markDirty(); }
    bool     isDirty()   const  { return m_dirty; }
    int      roadCount() const  { return m_project.roads.size(); }
    int      propCount() const  { return m_project.props.size(); }
    QString  activeTool()const  { return m_activeTool; }
    void     setActiveTool(const QString& t){ m_activeTool=t; emit activeToolChanged(); }

    // ---- Terrain tools (mirrors TreCorsa toolbar) -------------------------
    Q_INVOKABLE void setTerrainBrushMode(const QString& mode);
    Q_INVOKABLE void setTerrainBrushRadius(float r)   { m_terrain->setBrushRadius(r); }
    Q_INVOKABLE void setTerrainBrushStrength(float s) { m_terrain->setBrushStrength(s); }
    Q_INVOKABLE void applyTerrainBrush(float wx, float wz){ m_terrain->applyBrush(wx,wz); markDirty(); }
    Q_INVOKABLE void flattenTerrain(float wx,float wz,float r,float h){ m_terrain->flattenRegion(wx,wz,r,h); markDirty(); }
    Q_INVOKABLE void addTerrainNoise(float wx,float wz,float r,float amp,float freq){ m_terrain->addNoise(wx,wz,r,amp,freq); markDirty(); }
    Q_INVOKABLE void erodeTerrain(int iters=5){ m_terrain->erode(iters); markDirty(); }
    Q_INVOKABLE void hydraulicErosion(int iters=50){ m_terrain->hydraulicErode(iters); markDirty(); }
    Q_INVOKABLE bool importTerrainFromImage(const QString& path, float minH, float maxH);
    Q_INVOKABLE bool importTerrainFromSRTM(const QString& path){ return m_terrain->importFromSRTM(path); }
    Q_INVOKABLE float terrainHeightAt(float wx,float wz){ return m_terrain->getHeightWorld(wx,wz); }

    // ---- Texture paint ----------------------------------------------------
    Q_INVOKABLE int  addTerrainLayer(const QString& name, const QString& textureId, float uvScale=10.f);
    Q_INVOKABLE void paintTerrainLayer(int layer,float wx,float wz,float r,float opacity);
    Q_INVOKABLE void autoMaskBySlope(int layer,float minDeg,float maxDeg);

    // ---- Road tools -------------------------------------------------------
    Q_INVOKABLE QString addRoad(const QString& name = "Road");
    Q_INVOKABLE bool    removeRoad(const QString& id);
    Q_INVOKABLE bool    addRoadPoint(const QString& roadId, float x,float y,float z,
                                     float width=10.f, float camberL=0.f, float camberR=0.f);
    Q_INVOKABLE bool    updateRoadPoint(const QString& roadId, int pointIndex,
                                        float x,float y,float z,float width,
                                        float camberL,float camberR);
    Q_INVOKABLE bool    removeRoadPoint(const QString& roadId, int pointIndex);
    Q_INVOKABLE void    setRoadSurface(const QString& roadId, const QString& surface);
    Q_INVOKABLE void    setRoadBridge(const QString& roadId, bool bridge, float height=3.f);
    Q_INVOKABLE QStringList roadIds() const;

    // ---- Kerb tools -------------------------------------------------------
    Q_INVOKABLE QString addKerb(const QString& roadId, bool leftSide,
                                 const QString& style = "Sausage");
    Q_INVOKABLE bool    removeKerb(const QString& id);

    // ---- Wall tools -------------------------------------------------------
    Q_INVOKABLE QString addWall(const QString& name="Wall");
    Q_INVOKABLE bool    addWallPoint(const QString& wallId, float x,float y,float z);
    Q_INVOKABLE bool    removeWall(const QString& id);
    Q_INVOKABLE void    setWallType(const QString& wallId, const QString& type);
    Q_INVOKABLE void    snapWallToRoad(const QString& wallId, const QString& roadId, bool left);

    // ---- Surface tools ----------------------------------------------------
    Q_INVOKABLE QString addSurface(const QString& name="Surface");
    Q_INVOKABLE bool    addSurfacePoint(const QString& surfId, float x, float z);
    Q_INVOKABLE bool    removeSurface(const QString& id);

    // ---- Props ------------------------------------------------------------
    Q_INVOKABLE QString addProp(const QString& modelId, float x,float y,float z,
                                 float yaw=0.f, const QString& name=QString());
    Q_INVOKABLE bool    removeProp(const QString& id);
    Q_INVOKABLE bool    moveProp(const QString& id, float x,float y,float z);
    Q_INVOKABLE bool    rotateProp(const QString& id, float rx,float ry,float rz);
    Q_INVOKABLE bool    scaleProp(const QString& id, float sx,float sy,float sz);

    // ---- Lights -----------------------------------------------------------
    Q_INVOKABLE QString addLight(const QString& type="Street",
                                  float x=0.f,float y=5.f,float z=0.f);
    Q_INVOKABLE bool    removeLight(const QString& id);

    // ---- Start / Pit ------------------------------------------------------
    Q_INVOKABLE void    addStartPosition(float x,float y,float z,float yaw);
    Q_INVOKABLE void    addPitPosition(float x,float y,float z,float yaw);
    Q_INVOKABLE void    clearStartPositions();
    Q_INVOKABLE void    clearPitPositions();

    // ---- AI line ----------------------------------------------------------
    Q_INVOKABLE void    autoGenerateAILine();
    Q_INVOKABLE void    addAILinePoint(float x,float y,float z,float speed=100.f,float width=3.f);
    Q_INVOKABLE void    clearAILine();
    Q_INVOKABLE void    smoothAILine(int passes=3);

    // ---- Physics roads (Noise Labs) ---------------------------------------
    Q_INVOKABLE QString addPhysicsRoad(const QString& roadId);
    Q_INVOKABLE void    setPhysicsRoadNoise(const QString& id, float amp, float freq);
    Q_INVOKABLE void    setPhysicsRoadGrip(const QString& id, float grip, float ffb);

    // ---- Sub-system access ------------------------------------------------
    TerrainEngine* terrain()   { return m_terrain; }
    RoadBuilder*   roadBuilder(){ return m_roadBuilder; }
    TrackProject&  project()   { return m_project; }

    // ---- Export helpers ---------------------------------------------------
    Q_INVOKABLE QStringList exportValidationMessages() const;

signals:
    void projectChanged();
    void dirtyChanged();
    void activeToolChanged();
    void terrainModified();
    void exportProgress(int percent, const QString& stage);
    void exportDone(const QString& path);
    void exportFailed(const QString& error);
    void error(const QString& msg);

private:
    void markDirty(){ if(!m_dirty){m_dirty=true;emit dirtyChanged();} emit projectChanged(); }
    QString newId(){ return QUuid::createUuid().toString(QUuid::WithoutBraces).left(8); }
    Road*    findRoad(const QString& id);
    Wall*    findWall(const QString& id);
    Surface* findSurface(const QString& id);
    Prop*    findProp(const QString& id);
    Light*   findLight(const QString& id);
    Kerb*    findKerb(const QString& id);
    PhysicsRoad* findPhysicsRoad(const QString& id);

    TrackProject   m_project;
    TerrainEngine* m_terrain     = nullptr;
    RoadBuilder*   m_roadBuilder = nullptr;
    TrackExporter* m_exporter    = nullptr;
    bool           m_dirty       = false;
    QString        m_activeTool  = "navigate";

    static TrackBuilderModule* s_instance;
};

}} // namespace ks::track
