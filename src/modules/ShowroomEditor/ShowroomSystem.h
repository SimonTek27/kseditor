#pragma once

#include <QString>
#include <QVector>
#include <QMap>
#include <QColor>
#include <QStringList>

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
    };

    struct ShowroomCamera {
        QString name;
        float position[3] = {0, 2, 5};
        float target[3] = {0, 0, 0};
        float up[3] = {0, 1, 0};
        float fov = 60.0f;
        bool isActive = true;
    };

    struct ShowroomLight {
        QString name;
        QString type;           // "directional", "point", "spot"
        float position[3] = {0, 5, 0};
        float direction[3] = {0, -1, 0};
        QColor color = Qt::white;
        float intensity = 1.0f;
        float range = 10.0f;
        bool isActive = true;
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

    // Preview generation
    static bool generatePreview(const QString& carPath, const PreviewConfig& config);
    static bool generateThumbnail(const QString& carPath, const QString& outputPath);
    static bool generateAllPreviews(const QString& carsDir, const PreviewConfig& config);

    // Validation
    static bool validateConfig(const ShowroomConfig& config, QString* error = nullptr);
    static bool validatePreviewConfig(const PreviewConfig& config, QString* error = nullptr);

    // Utility
    static QStringList getAvailableShowrooms(const QString& showroomsDir);
    static QString getDefaultShowroomDir();
};

/**
 * @brief Showroom Manager - High-level interface
 */
class ShowroomManager {
public:
    explicit ShowroomManager(const QString& acPath);

    // Configuration
    bool loadShowroom(const QString& showroomName);
    bool saveShowroom(const QString& showroomName);

    // Preview
    bool generateCarPreview(const QString& carPath, const QString& outputPath);
    bool generateCarThumbnail(const QString& carPath);

    // Access
    ShowroomSystem::ShowroomConfig& config() { return m_config; }
    QVector<ShowroomSystem::ShowroomCamera>& cameras() { return m_cameras; }
    QVector<ShowroomSystem::ShowroomLight>& lights() { return m_lights; }

private:
    QString m_acPath;
    ShowroomSystem::ShowroomConfig m_config;
    QVector<ShowroomSystem::ShowroomCamera> m_cameras;
    QVector<ShowroomSystem::ShowroomLight> m_lights;
};
