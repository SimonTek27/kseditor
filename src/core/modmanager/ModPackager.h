#pragma once

#include <QString>
#include <QVector>
#include <QMap>

/**
 * @brief Mod Packager for Assetto Corsa
 *
 * Packages and exports AC mods for distribution.
 * Based on Content Manager features:
 * - Mod installation
 * - ZIP packaging
 * - Content validation
 * - Preview generation
 *
 * Features:
 * - Car mod packaging
 * - Track mod packaging
 * - ZIP creation with proper structure
 * - Content validation before export
 * - Preview image inclusion
 * - README generation
 */
class ModPackager {
public:
    struct PackageConfig {
        QString name;
        QString version;
        QString author;
        QString description;
        QString category;       // "car", "track", "weather", "app"
        QString license;
        QString website;
        QString previewPath;
        bool includePreview = true;
        bool validateBeforeExport = true;
        bool createReadme = true;
    };

    struct PackageInfo {
        QString name;
        QString version;
        QString author;
        QString sourcePath;
        QString outputPath;
        qint64 totalSize = 0;
        int fileCount = 0;
        bool isValid = false;
        QStringList warnings;
        QStringList errors;
    };

    // Package operations
    static PackageInfo packageCar(const QString& carPath, const QString& outputPath,
                                   const PackageConfig& config);
    static PackageInfo packageTrack(const QString& trackPath, const QString& outputPath,
                                     const PackageConfig& config);
    static PackageInfo packageContent(const QString& contentPath, const QString& outputPath,
                                       const PackageConfig& config);

    // ZIP operations
    static bool createZip(const QString& sourceDir, const QString& zipPath);
    static bool extractZip(const QString& zipPath, const QString& outputDir);
    static bool addToZip(const QString& zipPath, const QString& filePath, const QString& entryName);

    // Validation
    static PackageInfo validatePackage(const QString& contentPath);
    static bool validateCarPackage(const QString& carPath, QStringList& errors, QStringList& warnings);
    static bool validateTrackPackage(const QString& trackPath, QStringList& errors, QStringList& warnings);

    // Preview generation
    static bool generatePreview(const QString& contentPath, const QString& outputPath);
    static bool hasPreview(const QString& contentPath);

    // README generation
    static bool generateReadme(const PackageConfig& config, const QString& outputPath);
    static bool generateChangelog(const QString& version, const QString& changes, const QString& outputPath);

    // Utility
    static QString getDefaultOutputPath(const QString& contentPath);
    static QString getPackageName(const QString& contentPath);
    static qint64 calculateContentSize(const QString& contentPath);
    static QStringList getContentFiles(const QString& contentPath);

private:
    static bool copyDirectoryRecursive(const QString& source, const QString& destination);
    static bool createZipFromDirectory(const QString& sourceDir, const QString& zipPath);
};

/**
 * @brief Mod Packager Manager - High-level interface
 */
class ModPackagerManager {
public:
    explicit ModPackagerManager(const QString& acPath);

    // Configuration
    void setConfig(const ModPackager::PackageConfig& config) { m_config = config; }
    ModPackager::PackageConfig getConfig() const { return m_config; }

    // Operations
    bool packageContent(const QString& contentPath, const QString& outputPath);
    bool validateContent(const QString& contentPath);

    // Access
    ModPackager::PackageInfo getLastPackageInfo() const { return m_lastInfo; }

private:
    QString m_acPath;
    ModPackager::PackageConfig m_config;
    ModPackager::PackageInfo m_lastInfo;
};
