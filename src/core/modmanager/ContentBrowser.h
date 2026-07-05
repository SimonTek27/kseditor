#pragma once

#include <QString>
#include <QVector>
#include <QMap>
#include <QJsonObject>
#include <QDir>
#include <QImage>

/**
 * @brief Content Browser for Assetto Corsa
 *
 * Browses and manages AC content (cars, tracks, weather, etc.).
 * Based on Content Manager features:
 * - Content browsing and filtering
 * - Mod management
 * - Preview generation
 * - Content validation
 */
class ContentBrowser {
public:
    struct ContentItem {
        QString name;
        QString path;
        QString type;           // "car", "track", "weather", "font", "showroom"
        QString author;
        QString version;
        QString description;
        float rating = 0.0f;
        int downloadCount = 0;
        bool isInstalled = false;
        bool isMod = false;
        QDateTime lastModified;
        qint64 size = 0;

        // Car-specific
        QString manufacturer;
        int year = 0;
        int power = 0;
        float weight = 0.0f;
        int drivetrain = 0;     // 0=FWD, 1=RWD, 2=AWD
        int transmission = 0;   // 0=manual, 1=sequential, 2=automatic

        // Track-specific
        float length = 0.0f;
        int country = 0;
        bool hasNightLighting = false;
        bool hasPitboxes = false;
        int pitboxCount = 0;

        // Preview
        QString previewPath;
        QImage previewImage;
    };

    struct ContentFilter {
        QString type;
        QString author;
        QString searchQuery;
        float minRating = 0.0f;
        bool installedOnly = false;
        bool modsOnly = false;
        bool stockOnly = false;
        int yearMin = 0;
        int yearMax = 0;
        float powerMin = 0;
        float powerMax = 0;
        float weightMin = 0;
        float weightMax = 0;
        int drivetrain = -1;    // -1 = any
    };

    struct ContentStats {
        int totalCars = 0;
        int totalTracks = 0;
        int totalWeather = 0;
        int totalMods = 0;
        int totalStock = 0;
        qint64 totalSize = 0;
    };

    // Browsing operations
    static QVector<ContentItem> browseContent(const QString& contentPath,
                                               const ContentFilter& filter = ContentFilter());
    static QVector<ContentItem> browseCars(const QString& carsPath,
                                            const ContentFilter& filter = ContentFilter());
    static QVector<ContentItem> browseTracks(const QString& tracksPath,
                                              const ContentFilter& filter = ContentFilter());
    static QVector<ContentItem> browseWeather(const QString& weatherPath);

    // Content information
    static ContentItem getItemInfo(const QString& itemPath);
    static ContentStats getContentStats(const QString& contentPath);

    // Preview generation
    static bool generatePreview(const QString& itemPath);
    static bool hasPreview(const QString& itemPath);
    static QString getPreviewPath(const QString& itemPath);

    // Content validation
    static bool validateContent(const QString& itemPath, QString* error = nullptr);
    static bool validateCar(const QString& carPath, QString* error = nullptr);
    static bool validateTrack(const QString& trackPath, QString* error = nullptr);

    // Content management
    static bool installMod(const QString& modPath, const QString& contentPath);
    static bool uninstallMod(const QString& modName, const QString& contentPath);
    static bool updateMod(const QString& modName, const QString& newPath, const QString& contentPath);

    // Content search
    static QVector<ContentItem> searchContent(const QString& contentPath,
                                               const QString& query,
                                               const QString& type = QString());

    // Content comparison
    static QMap<QString, QPair<QString, QString>> compareContent(const QString& path1,
                                                                  const QString& path2);

    // Utility
    static QString getContentTypeName(const QString& type);
    static QString getDrivetrainName(int drivetrain);
    static QString getTransmissionName(int transmission);
    static QStringList getContentTypes();

private:
    static bool parseCarInfo(const QString& carPath, ContentItem& item);
    static bool parseTrackInfo(const QString& trackPath, ContentItem& item);
    static bool parseWeatherInfo(const QString& weatherPath, ContentItem& item);
    static bool matchesFilter(const ContentItem& item, const ContentFilter& filter);
    static bool copyDirectoryRecursive(const QString& sourceDir, const QString& destDir);
};

/**
 * @brief Content Browser Widget - UI for browsing content
 */
class ContentBrowserWidget;
