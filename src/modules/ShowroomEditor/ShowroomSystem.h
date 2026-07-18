#pragma once

#include <QString>
#include <QVector>
#include <QMap>
#include <QColor>
#include <QStringList>
#include <QImage>

/**
 * @brief Showroom System for Assetto Corsa
 *
 * Manages car showrooms and preview generation.
 * Based on:
 * - gro-ove/actools Showroom & Preview Generation
 * - Content Manager Custom Showroom
 * - AC Python API documentation
 *
 * Features:
 * - Car showroom configuration
 * - Preview image generation
 * - Camera positioning
 * - Lighting setup
 * - Background configuration
 * - Studio lighting rigs (HDRI, area lights, IES profiles)
 * - Camera paths for turntables
 * - Environment reflection probes
 * - Material override for clay/normal/UV view modes
 * - Screenshot/render queue with resolution presets
 * - Comparison slider for A/B material evaluation
 */
class ShowroomSystem {
public:
    struct ShowroomConfig {
        QString name;
        QString description;
        float cameraDistance = 5.0f;
        float cameraHeight = 2.0f;
        float cameraAngle = 30.0f;
        float cameraFov = 60.0f;
        float rotateSpeed = 0.5f;
        bool autoRotate = true;
        QString backgroundPath;
        QColor ambientColor = QColor(200, 200, 200);
        QColor sunColor = QColor(255, 250, 240);
        float sunIntensity = 1.0f;
        float ambientIntensity = 0.3f;
        
        // Advanced lighting
        bool useHDRI = false;
        QString hdriPath;
        float hdriRotation = 0.0f;
        float hdriIntensity = 1.0f;
        
        // Area lights
        struct AreaLight {
            QString name;
            float position[3] = {0, 3, 0};
            float rotation[3] = {0, 0, 0};
            float size[2] = {2, 2};
            QColor color = QColor(255, 255, 255);
            float intensity = 1.0f;
            bool isActive = true;
        };
        QVector<AreaLight> areaLights;
        
        // IES profiles
        struct IESProfile {
            QString name;
            QString filePath;
            float intensity = 1.0f;
            float rotation[3] = {0, 0, 0};
            bool isActive = true;
        };
        QVector<IESProfile> iesProfiles;
        
        // Material preview modes
        enum MaterialPreviewMode {
            Normal = 0,
            Clay = 1,
            NormalMap = 2,
            UV = 3,
            Wireframe = 4,
            AO = 5,
            Curvature = 6,
            Metalness = 7,
            Roughness = 8
        };
        MaterialPreviewMode materialPreviewMode = Normal;
        
        // Extended lighting flags
        bool useAreaLights = false;
        bool useIESProfiles = false;
        QString iesProfilePath;

        // Comparison
        bool enableComparison = false;
        QString comparisonCarPath;
        float comparisonSplit = 0.5f;
    };

    struct PreviewConfig {
        int width = 1920;
        int height = 1080;
        int samples = 4;
        float fov = 60.0f;
        float cameraDistance = 5.0f;
        float cameraHeight = 2.0f;
        float cameraAngle = 30.0f;
        bool useTransparentBackground = false;
        QColor backgroundColor = Qt::black;
        QString outputPath;
        
        // Render queue presets
        enum ResolutionPreset {
            Preset_4K = 0,
            Preset_1440p = 1,
            Preset_1080p = 2,
            Preset_720p = 3,
            Preset_Thumbnail = 4,
            Preset_Custom = 5
        };
        ResolutionPreset preset = Preset_1080p;
        int customWidth = 1920;
        int customHeight = 1080;
        bool useDenoiser = true;
    };

    struct ShowroomCamera {
        QString name;
        float position[3] = {0, 2, 5};
        float target[3] = {0, 0, 0};
        float up[3] = {0, 1, 0};
        float fov = 60.0f;
        bool isActive = true;
        
        // Camera path for turntable
        struct Keyframe {
            float time = 0.0f;
            float position[3] = {0, 2, 5};
            float target[3] = {0, 0, 0};
            float fov = 60.0f;
            float roll = 0.0f;
            QString easing = "linear"; // linear, ease_in, ease_out, ease_in_out
        };
        QVector<Keyframe> path;
        bool usePath = false;
        float pathDuration = 10.0f; // seconds
        bool pathLoop = true;
    };

    struct ShowroomLight {
        QString name;
        QString type;           // "directional", "point", "spot", "area", "ies"
        float position[3] = {0, 5, 0};
        float direction[3] = {0, -1, 0};
        QColor color = Qt::white;
        float intensity = 1.0f;
        float range = 10.0f;
        float innerAngle = 30.0f;
        float outerAngle = 45.0f;
        float size[2] = {2, 2}; // for area lights
        QString iesProfile;
        bool isActive = true;
        bool castShadows = true;
    };

    struct ReflectionProbe {
        QString name;
        float position[3] = {0, 1, 0};
        float boxSize[3] = {10, 10, 10};
        int resolution = 512;
        bool realTime = false;
        float refreshRate = 30.0f;
        bool isActive = true;
    };

    struct RenderQueueItem {
        QString id;
        QString carPath;
        QString outputPath;
        PreviewConfig config;
        int priority = 0;
        QString status = "pending"; // pending, processing, completed, failed
        int progress = 0;
    };

    struct ComparisonSettings {
        QString carA;
        QString carB;
        float splitPosition = 0.5f; // 0.0 - 1.0
        bool verticalSplit = true;
        bool syncCamera = true;
    };

    // Configuration operations
    static ShowroomConfig loadConfig(const QString& configPath);
    static bool saveConfig(const ShowroomConfig& config, const QString& configPath);
    static ShowroomConfig getDefaultConfig();

    // Camera operations
    static QVector<ShowroomCamera> loadCameras(const QString& configPath);
    static bool saveCameras(const QVector<ShowroomCamera>& cameras, const QString& configPath);
    static ShowroomCamera getDefaultCamera();

    // Light operations
    static QVector<ShowroomLight> loadLights(const QString& configPath);
    static bool saveLights(const QVector<ShowroomLight>& lights, const QString& configPath);
    static QVector<ShowroomLight> getDefaultLights();

    // Reflection probe operations
    static QVector<ReflectionProbe> loadReflectionProbes(const QString& configPath);
    static bool saveReflectionProbes(const QVector<ReflectionProbe>& probes, const QString& configPath);
    static QVector<ReflectionProbe> getDefaultReflectionProbes();

    // HDRI operations
    static QVector<QString> getAvailableHDRIs(const QString& hdriDir);

    // IES profile operations
    static QVector<ShowroomConfig::IESProfile> loadIESProfiles(const QString& configPath);
    static bool saveIESProfiles(const QVector<ShowroomConfig::IESProfile>& profiles, const QString& configPath);

    // Preview generation
    static bool generatePreview(const QString& carPath, const PreviewConfig& config);
    static bool generateThumbnail(const QString& carPath, const QString& outputPath);
    static bool generateAllPreviews(const QString& carsDir, const PreviewConfig& config);

    // Render queue
    static bool processRenderQueue(const QVector<PreviewConfig>& queue, const QString& carPath);
    static QVector<PreviewConfig> createRenderQueueFromPresets();

    // Comparison
    static bool generateComparison(const QString& carA, const QString& carB, const QString& outputPath, const PreviewConfig& config);
    static QImage generateComparisonImage(const QImage& imgA, const QImage& imgB, float split, bool vertical);

    // Material preview
    static QImage generateMaterialPreview(const QString& carPath, ShowroomConfig::MaterialPreviewMode mode, const PreviewConfig& config);

    // Render queue
    static bool processRenderQueue(const QVector<RenderQueueItem>& queue);
    static void cancelRenderQueue();
    static QVector<RenderQueueItem> getRenderQueueStatus();

    // Validation
    static bool validateConfig(const ShowroomConfig& config, QString* error = nullptr);
    static bool validatePreviewConfig(const PreviewConfig& config, QString* error = nullptr);

    // Utility
    static QStringList getAvailableShowrooms(const QString& showroomsDir);
    static QString getDefaultShowroomDir();
};

class ShowroomManager {
public:
    ShowroomManager(const QString& acPath);

    bool loadShowroom(const QString& showroomName);
    bool saveShowroom(const QString& showroomName);
    bool generateCarPreview(const QString& carPath, const QString& outputPath);
    bool generateCarThumbnail(const QString& carPath);

private:
    QString m_acPath;
    ShowroomSystem::ShowroomConfig m_config;
    QVector<ShowroomSystem::ShowroomCamera> m_cameras;
    QVector<ShowroomSystem::ShowroomLight> m_lights;
};


