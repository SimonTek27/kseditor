#pragma once

#include <QString>
#include <QVector>
#include <QMap>
#include <QJsonObject>

/**
 * @brief Track Builder Tools for Assetto Corsa
 *
 * Tools for building and exporting tracks.
 * Based on:
 * - nendotools/ac-track-tools (Blender addon)
 * - nothke/blender_ac_exporter (FBX export)
 * - MeshHouse/TrackTools (GeoNodes modifiers)
 * - TreCorsa (browser-based track generator)
 * - se0lus/sam3_ac_track_mod_gen (drone imagery to track)
 *
 * Features:
 * - Track project initialization
 * - Surface mesh management
 * - Start/pit position placement
 * - AI line generation
 * - FBX export with AC settings
 * - Track validation
 */
class TrackBuilderTools {
public:
    struct TrackProject {
        QString name;
        QString path;
        QString trackName;
        QString author;
        QString description;
        float length = 0.0f;
        int pitboxCount = 20;
        bool hasNightLighting = false;
        bool hasPitboxes = true;
        QDateTime created;
        QDateTime modified;
    };

    struct TrackMesh {
        QString name;
        QString surfaceType;    // "ROAD", "GRASS", "KERB", etc.
        QString materialName;
        int vertexCount = 0;
        int triangleCount = 0;
        float boundsMin[3] = {0, 0, 0};
        float boundsMax[3] = {0, 0, 0};
    };

    struct StartPosition {
        int carIndex = 0;
        float position[3] = {0, 0, 0};
        float direction[3] = {0, 0, 1};
        float up[3] = {0, 1, 0};
    };

    struct PitPosition {
        int index = 0;
        float position[3] = {0, 0, 0};
        float direction[3] = {0, 0, 1};
        float width = 3.0f;
        float length = 6.0f;
    };

    struct CameraPosition {
        QString name;
        float position[3] = {0, 0, 0};
        float target[3] = {0, 0, 0};
        float up[3] = {0, 1, 0};
        float fov = 60.0f;
    };

    // Project management
    static TrackProject createProject(const QString& path, const QString& name);
    static TrackProject loadProject(const QString& path);
    static bool saveProject(const TrackProject& project);
    static bool validateProject(const QString& path, QString* error = nullptr);

    // Track structure
    static bool initializeTrackStructure(const QString& path);
    static bool createDefaultFiles(const QString& path);
    static QStringList getRequiredFiles();
    static QStringList getOptionalFiles();

    // Mesh management
    static QVector<TrackMesh> scanMeshes(const QString& fbxPath);
    static bool validateMeshes(const QString& fbxPath, QString* error = nullptr);
    static bool optimizeMesh(const QString& inputPath, const QString& outputPath);

    // Start/pit positions
    static QVector<StartPosition> loadStartPositions(const QString& trackPath);
    static bool saveStartPositions(const QVector<StartPosition>& positions, const QString& trackPath);
    static StartPosition createStartPosition(int carIndex, const float* position, const float* direction);

    static QVector<PitPosition> loadPitPositions(const QString& trackPath);
    static bool savePitPositions(const QVector<PitPosition>& positions, const QString& trackPath);
    static PitPosition createPitPosition(int index, const float* position, const float* direction);

    // Camera management
    static QVector<CameraPosition> loadCameras(const QString& trackPath);
    static bool saveCameras(const QVector<CameraPosition>& cameras, const QString& trackPath);

    // FBX export
    static bool exportToFbx(const QString& inputPath, const QString& outputPath,
                             const TrackProject& project);
    static bool validateFbxExport(const QString& fbxPath, QString* error = nullptr);

    // Track validation
    static bool validateTrackData(const QString& trackPath, QString* error = nullptr);
    static bool validateSurfaces(const QString& trackPath, QString* error = nullptr);
    static bool validateAiLine(const QString& trackPath, QString* error = nullptr);

    // Utility
    static float calculateTrackLength(const QString& trackPath);
    static QStringList getSurfaceTypes();
    static QString getDefaultSurfaceForMesh(const QString& meshName);

private:
    static bool createUiTrackJson(const TrackProject& project, const QString& path);
    static bool createSurfacesIni(const QString& path);
    static bool createMapPng(const QString& path);
    static bool createModelsIni(const QString& path);
};

/**
 * @brief Track Builder Manager - High-level interface
 */
class TrackBuilderManager {
public:
    explicit TrackBuilderManager(const QString& projectPath);

    // Project operations
    bool createProject(const QString& name);
    bool loadProject();
    bool saveProject();
    bool validate(QString* error = nullptr);

    // Access
    TrackBuilderTools::TrackProject& project() { return m_project; }
    const TrackBuilderTools::TrackProject& project() const { return m_project; }

    // Mesh operations
    bool scanMeshes();
    bool validateMeshes(QString* error = nullptr);
    QVector<TrackBuilderTools::TrackMesh> getMeshes() const { return m_meshes; }

    // Position operations
    bool addStartPosition(const TrackBuilderTools::StartPosition& position);
    bool addPitPosition(const TrackBuilderTools::PitPosition& position);
    bool removeStartPosition(int index);
    bool removePitPosition(int index);

    // Export
    bool exportToFbx(const QString& outputPath);

private:
    QString m_projectPath;
    TrackBuilderTools::TrackProject m_project;
    QVector<TrackBuilderTools::TrackMesh> m_meshes;
    QVector<TrackBuilderTools::StartPosition> m_startPositions;
    QVector<TrackBuilderTools::PitPosition> m_pitPositions;
    QVector<TrackBuilderTools::CameraPosition> m_cameras;
};
