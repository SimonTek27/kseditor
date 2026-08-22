#pragma once
// ============================================================================
// TrackExporter.h
// Exports a TrackProject to a complete Assetto Corsa track folder.
// Produces: models/  surfaces.ini  surfaces_XX.ini  ai/  data/  ui/
// The output is zip-ready and compatible with Content Manager drag-and-drop.
// ============================================================================

#include "TrackBuilderTypes.h"
#include "RoadBuilder.h"
#include "TerrainEngine.h"
#include "assettocorsa/acFiles/KN5Types.h"
#include <QObject>
#include <QString>
#include <QDir>
#include <QPair>
#include <functional>
#include <QVector3D>

namespace ks { namespace track {

// ============================================================================
class TrackExporter : public QObject
{
    Q_OBJECT
public:
    explicit TrackExporter(QObject* parent = nullptr);

    // ---- Settings ----------------------------------------------------------
    void setOutputDir(const QString& dir)     { m_outputDir = dir; }
    void setTrackId(const QString& id)        { m_trackId = id; }
    void setTerrainEngine(TerrainEngine* te)  { m_terrain = te; }
    void setRoadBuilder(RoadBuilder* rb)      { m_roadBuilder = rb; }
    void setZipOutput(bool zip)               { m_zipOutput = zip; }

    // ---- Export ------------------------------------------------------------
    bool exportTrack(const TrackProject& project,
                     std::function<void(int,QString)> progress = nullptr);

    QString lastError() const { return m_lastError; }

signals:
    void exportProgress(int percent, const QString& stage);
    void exportDone(const QString& path);
    void exportFailed(const QString& error);

private:
    bool exportTerrain    (const TrackProject& p, const QString& dir);
    bool exportRoads      (const TrackProject& p, const QString& dir,
                           QVector<RoadMesh>& outMeshes);
    bool exportWalls      (const TrackProject& p, const QString& dir,
                           const QVector<RoadMesh>& roadMeshes);
    bool exportKerbs      (const TrackProject& p, const QString& dir,
                           const QVector<RoadMesh>& roadMeshes);
    bool exportProps      (const TrackProject& p, const QString& dir);
    bool exportLights     (const TrackProject& p, const QString& dir);
    bool exportSurfacesIni  (const TrackProject& p, const QString& dir);
    bool exportAILine       (const TrackProject& p, const QString& dir,
                             const QVector<RoadMesh>& roadMeshes);
    bool exportStartPit     (const TrackProject& p, const QString& dir);
    bool exportPhysicsRoads (const TrackProject& p, const QString& dir);
    bool exportUIJson       (const TrackProject& p, const QString& dir);
    bool exportIniFiles     (const TrackProject& p, const QString& dir);
    bool exportTimingSectors(const TrackProject& p, const QString& dir);
    bool exportGroove       (const TrackProject& p, const QString& dir);
    bool exportVAOHint      (const TrackProject& p, const QString& dir);
    bool exportTrackMapSVG  (const TrackProject& p, const QString& dir);
    bool zipDirectory     (const QString& dir, const QString& zipPath);

    bool writeOBJ(const QString& path, const QVector<RoadMesh>& meshes,
                  const QString& mtlName);
    bool writeMTL(const QString& path, const TrackProject& p);
    bool writeKN5(const QString& path, const TrackProject& p,
                  const QVector<RoadMesh>& roadMeshes,
                  const QVector<RoadMesh>& wallMeshes,
                  const QVector<RoadMesh>& kerbMeshes);
    KN5Parser::Mesh roadMeshToKN5Mesh(const RoadMesh& rm,
                                      const QString& name,
                                      quint32 nodeIndex);

    QString surfaceTag(SurfaceType s) const;
    QString wallTag(WallType w) const;
    void    ensureDir(const QString& path);
    bool    writeFile(const QString& path, const QByteArray& data);

    QString        m_outputDir;
    QString        m_trackId  = "custom_track";
    TerrainEngine* m_terrain  = nullptr;
    RoadBuilder*   m_roadBuilder = nullptr;
    bool           m_zipOutput = true;
    QString        m_lastError;
};

}} // namespace ks::track
