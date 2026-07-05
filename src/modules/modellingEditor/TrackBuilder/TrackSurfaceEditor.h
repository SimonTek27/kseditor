#pragma once

#include <QString>
#include <QMap>
#include <QVector>
#include <QJsonObject>
#include <QColor>

/**
 * @brief Track Surface Editor for Assetto Corsa
 *
 * Manages track surface definitions used by Assetto Corsa physics.
 * Based on community tools:
 * - ac-track-tools (github.com/nendotools/ac-track-tools)
 * - AC Tools Blender addon (extensions.blender.org/add-ons/ac-tools/)
 *
 * Surface properties affect:
 * - Grip level for different tire compounds
 * - Rolling resistance
 * - Vibration and force feedback
 * - Visual appearance (dust, particles)
 */
class TrackSurfaceEditor {
public:
    struct SurfaceDefinition {
        QString name;
        int id;
        QColor displayColor;

        // Physics properties
        float gripK = 1.0f;           // Base grip multiplier
        float gripM = 1.0f;           // Grip modifier (temperature dependent)
        float rollingResistance = 0.0f;
        float vibrationGain = 1.0f;
        float particlesGain = 1.0f;

        // Visual properties
        bool isBallast = false;
        bool isWet = false;
        bool isGrass = false;
        bool isGravel = false;
        bool isRumble = false;

        // Mesh naming convention
        QString meshPrefix; // e.g., "1ROAD", "1GRASS", "1KERB"

        // Sound effects
        QString soundType;
        float soundGain = 1.0f;
    };

    // Surface management
    static bool loadSurfaces(const QString& trackPath);
    static bool saveSurfaces(const QString& trackPath);
    static bool loadFromIni(const QString& iniPath);
    static bool saveToIni(const QString& iniPath);

    // Surface operations
    static void addSurface(const SurfaceDefinition& surface);
    static void removeSurface(const QString& name);
    static void updateSurface(const QString& name, const SurfaceDefinition& surface);
    static SurfaceDefinition getSurface(const QString& name);
    static QVector<SurfaceDefinition> getAllSurfaces();
    static bool hasSurface(const QString& name);

    // Default surfaces
    static void loadDefaults();
    static QVector<SurfaceDefinition> getDefaultSurfaces();

    // Validation
    static bool validateSurface(const SurfaceDefinition& surface, QString* error = nullptr);
    static bool validateAllSurfaces(QString* error = nullptr);

    // Import/Export
    static bool importFromBlender(const QString& csvPath);
    static bool exportToBlender(const QString& csvPath);
    static bool importFromContentManager(const QString& jsonPath);

    // Utility
    static QString getMeshPrefixForSurface(const QString& surfaceName);
    static QString getSurfaceForMesh(const QString& meshName);
    static int getNextAvailableId();

private:
    static QMap<QString, SurfaceDefinition> m_surfaces;
    static QString m_trackPath;
};

/**
 * @brief Track Surface Definition - INI format constants
 */
namespace TrackSurfaceIni {
    // Section names
    constexpr const char* SECTION_SURFACE = "SURFACE";
    constexpr const char* SECTION_MATERIAL = "MATERIAL";

    // Key names
    constexpr const char* KEY_NAME = "NAME";
    constexpr const char* KEY_GRIP_K = "GRIP_K";
    constexpr const char* KEY_GRIP_M = "GRIP_M";
    constexpr const char* KEY_ROLLING_RESISTANCE = "ROLLING_RESISTANCE";
    constexpr const char* KEY_VIBRATION_GAIN = "VIBRATION_GAIN";
    constexpr const char* KEY_PARTICLES_GAIN = "PARTICLES_GAIN";
    constexpr const char* KEY_IS_BALLAST = "IS_BALLAST";
    constexpr const char* KEY_IS_WET = "IS_WET";
    constexpr const char* KEY_IS_GRASS = "IS_GRASS";
    constexpr const char* KEY_IS_GRAVEL = "IS_GRAVEL";
    constexpr const char* KEY_IS_RUMBLE = "IS_RUMBLE";
    constexpr const char* KEY_MESH_PREFIX = "MESH_PREFIX";
    constexpr const char* KEY_SOUND_TYPE = "SOUND_TYPE";
    constexpr const char* KEY_SOUND_GAIN = "SOUND_GAIN";
    constexpr const char* KEY_DISPLAY_COLOR = "DISPLAY_COLOR";
}
