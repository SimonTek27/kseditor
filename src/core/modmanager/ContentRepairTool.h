#pragma once

#include <QString>
#include <QVector>
#include <QMap>
#include <QVariant>

/**
 * @brief Content Repair Tool for Assetto Corsa
 *
 * Validates and repairs AC content files.
 * Based on Content Manager's repair tool features:
 * - Repair tool for obsolete car mods
 * - Fix common errors
 * - Validate file integrity
 * - Auto-fix missing files
 *
 * Features:
 * - Car validation and repair
 * - Track validation and repair
 * - Physics file validation
 * - Texture validation
 * - Model validation
 * - Missing file detection
 */
class ContentRepairTool {
public:
    struct RepairIssue {
        QString id;
        QString title;
        QString description;
        QString severity;       // "error", "warning", "info"
        QString category;       // "physics", "texture", "model", "config", "structure"
        QString filePath;
        QString suggestedFix;
        bool autoFixable = false;
        QVariantMap fixParams;
    };

    struct RepairReport {
        QString contentPath;
        QString contentType;    // "car", "track"
        QVector<RepairIssue> issues;
        int errorCount = 0;
        int warningCount = 0;
        int infoCount = 0;
        int autoFixableCount = 0;
    };

    // Validation operations
    static RepairReport validateCar(const QString& carPath);
    static RepairReport validateTrack(const QString& trackPath);
    static RepairReport validateContent(const QString& contentPath);

    // Repair operations
    static bool fixIssue(const RepairIssue& issue);
    static bool fixAllIssues(RepairReport& report);
    static int autoFix(RepairReport& report);

    // Specific validations
    static QVector<RepairIssue> validateCarStructure(const QString& carPath);
    static QVector<RepairIssue> validatePhysicsFiles(const QString& carPath);
    static QVector<RepairIssue> validateTextures(const QString& carPath);
    static QVector<RepairIssue> validateModels(const QString& carPath);
    static QVector<RepairIssue> validateTrackStructure(const QString& trackPath);
    static QVector<RepairIssue> validateTrackSurfaces(const QString& trackPath);
    static QVector<RepairIssue> validateTrackAI(const QString& trackPath);

    // Advanced validations
    static QVector<RepairIssue> validateMeshFiles(const QString& carPath);
    static QVector<RepairIssue> validatePhysicsValues(const QString& carPath);
    static QVector<RepairIssue> validateSuspensionGeometry(const QString& carPath);
    static QVector<RepairIssue> validateAeroBalance(const QString& carPath);
    static QVector<RepairIssue> validateTyreData(const QString& carPath);
    static QVector<RepairIssue> validateEngineData(const QString& carPath);

    // Auto-fix operations
    static bool fixMissingFile(const QString& filePath, const QString& templatePath);
    static bool fixInvalidIni(const QString& iniPath);
    static bool fixMissingTexture(const QString& texturePath);
    static bool fixMissingPreview(const QString& contentPath);
    static bool fixMissingMap(const QString& trackPath);
    static bool fixMissingAeroIni(const QString& carPath);
    static bool fixMissingDifferentialIni(const QString& carPath);

    // File integrity
    static bool verifyFileIntegrity(const QString& filePath);
    static QString calculateFileHash(const QString& filePath);
    static bool backupFile(const QString& filePath);
    static bool restoreFile(const QString& filePath);

    // Utility
    static QStringList getRequiredCarFiles();
    static QStringList getRequiredTrackFiles();
    static QStringList getOptionalCarFiles();
    static QStringList getOptionalTrackFiles();

private:
    static RepairIssue createIssue(const QString& id, const QString& title,
                                    const QString& description, const QString& severity,
                                    const QString& category, const QString& filePath);
};

/**
 * @brief Content Repair Manager - High-level interface
 */
class ContentRepairManager {
public:
    ContentRepairManager();

    // Validation
    ContentRepairTool::RepairReport scanContent(const QString& contentPath);
    ContentRepairTool::RepairReport scanCar(const QString& carPath);
    ContentRepairTool::RepairReport scanTrack(const QString& trackPath);

    // Repair
    bool fixSelected(const QVector<ContentRepairTool::RepairIssue>& issues);
    bool fixAll(ContentRepairTool::RepairReport& report);
    bool autoFix(ContentRepairTool::RepairReport& report);

    // Reporting
    QString generateReport(const ContentRepairTool::RepairReport& report);
    bool exportReport(const ContentRepairTool::RepairReport& report, const QString& filePath);

    // Statistics
    int getTotalIssues() const;
    int getFixedIssues() const;

private:
    QVector<ContentRepairTool::RepairReport> m_reports;
    int m_fixedCount = 0;
};
