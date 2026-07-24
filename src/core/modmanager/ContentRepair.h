#pragma once

#include "../editor/EditorModule.h"
#include <QString>
#include <QStringList>
#include <QMap>
#include <QVector>
#include <QDir>
#include <QJsonObject>
#include <QImage>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidget>
#include <QProgressBar>
#include <QTextBrowser>
#include <QSplitter>

namespace ks {

struct ContentIssue {
    enum Severity { Info, Warning, Error, Critical };
    enum Category { Mesh, Texture, Sound, Config, Skin, Track, Physics, Animation, Audio, Model };

    QString id;
    QString title;
    QString description;
    Severity severity;
    Category category;
    QString filePath;
    QString suggestedFix;
    bool autoFixable = false;
    QVariantMap fixParams;

    static QString categoryName(Category c) {
        switch (c) {
            case Mesh: return "Mesh";
            case Texture: return "Texture";
            case Sound: return "Sound";
            case Config: return "Config";
            case Skin: return "Skin";
            case Track: return "Track";
            case Physics: return "Physics";
            case Animation: return "Animation";
            case Audio: return "Audio";
            case Model: return "Model";
        }
        return "Unknown";
    }
};

struct ContentReport {
    QString contentPath;
    QString contentType;
    QString name;
    QVector<ContentIssue> issues;
    bool hasCriticalIssues = false;
    int autoFixableCount = 0;
};

class ContentValidator {
public:
    // Texture validation
    static QVector<ContentIssue> validateTexture(const QString& path, const QString& contentRoot);
    static QVector<ContentIssue> validateTexturesInDir(const QString& dir, const QString& contentRoot);

    // Mesh validation
    static QVector<ContentIssue> validateMeshFile(const QString& path);
    static QVector<ContentIssue> validateMeshesInDir(const QString& dir);

    // Physics/INI validation
    static QVector<ContentIssue> validatePhysicsFile(const QString& path);
    static QVector<ContentIssue> validateIniFile(const QString& path, const QStringList& requiredSections,
                                                   const QMap<QString, QStringList>& requiredKeys);
    static QVector<ContentIssue> validatePhysicsDir(const QString& dir);

    // Sound validation
    static QVector<ContentIssue> validateSoundFile(const QString& path);
    static QVector<ContentIssue> validateSoundsInDir(const QString& dir);

    // General file validation
    static QVector<ContentIssue> validateFileExists(const QString& path, const QString& description,
                                                     ContentIssue::Severity severity);
    static QVector<ContentIssue> validateFileSize(const QString& path, qint64 minBytes);

    // Batch validation
    static QVector<ContentIssue> validateCarStructure(const QString& carPath);
    static QVector<ContentIssue> validateTrackStructure(const QString& trackPath);
};

class ContentRepairEngine {
public:
    static QVector<ContentReport> scanAllContent(const QString& contentDir);
    static ContentReport scanCar(const QString& carPath);
    static ContentReport scanTrack(const QString& trackPath);

    static bool fixIssue(const ContentIssue& issue);
    static bool fixAllIssues(ContentReport& report);
    static int autoFix(ContentReport& report);

    // Missing file detection
    static QVector<ContentIssue> detectMissingFiles(const QString& contentDir);
    static QVector<ContentIssue> detectMissingCarFiles(const QString& carPath);
    static QVector<ContentIssue> detectMissingTrackFiles(const QString& trackPath);

    // Corrupt file recovery
    static bool recoverCorruptedIni(const QString& filePath);
    static bool recoverCorruptedDds(const QString& filePath);
    static bool recoverCorruptedWav(const QString& filePath);
    static bool recoverCorruptedKn5(const QString& filePath);

    // File hash verification
    static bool verifyFileIntegrity(const QString& filePath, const QString& expectedHash);
    static QString calculateFileHash(const QString& filePath);
    static bool backupBeforeFix(const QString& filePath);
    static bool restoreFromBackup(const QString& filePath);

    // Batch recovery
    static int recoverAllCorrupted(const QVector<ContentIssue>& issues);
    static QVector<ContentIssue> scanForCorruptedFiles(const QString& dir);

private:
    // Repair implementations
    static bool fixMissingPreview(const ContentIssue& issue);
    static bool fixIniSection(const ContentIssue& issue);
    static bool fixIniKey(const ContentIssue& issue);
    static bool fixTextureSize(const ContentIssue& issue);
    static bool fixTextureFormat(const ContentIssue& issue);
    static bool fixMissingCmgEntry(const ContentIssue& issue);
    static bool fixCorruptedDataFile(const ContentIssue& issue);
    static bool generateDefaultConfig(const ContentIssue& issue);
    static bool generateManifest(const ContentIssue& issue);
    static bool fixMissingFile(const ContentIssue& issue);
    static bool restoreCorruptedFile(const ContentIssue& issue);

    // Backup directory
    static QString backupDir();
};

class ContentRepairModule : public EditorModule {
    Q_OBJECT
public:
    explicit ContentRepairModule(QWidget* parent = nullptr);
    ~ContentRepairModule() override;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "Content Repair"; }
    QString moduleId() const override { return "contentrepair"; }
    QString getModuleIcon() const override { return ":/icons/repair.svg"; }
    int getModulePriority() const override { return 50; }
    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;

public slots:
    void scanContent();
    void scanContentDir(const QString& path);
    void scanCar(const QString& carPath);
    void scanTrack(const QString& trackPath);
    void fixSelected();
    void fixAll();
    void autoFix();
    void clearReports();
    void exportReport(const QString& filePath);

signals:
    void scanStarted();
    void scanProgress(int current, int total, const QString& currentItem);
    void scanFinished(const QVector<ContentReport>& reports);
    void issuesFound(const QString& contentName, int issueCount);
    void fixCompleted(bool success, const QString& message);

private:
    void populateIssuesTree();
    void addIssueToTree(QTreeWidget* tree, const ContentIssue& issue, const QString& contentName);
    QVector<ContentIssue> getSelectedIssues() const;
    void updateDetailPanel();

    QVector<ContentReport> m_reports;
    QString m_lastContentDir;
    QTreeWidget* m_issueTree = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QTextBrowser* m_detailPanel = nullptr;
    QLabel* m_summaryLabel = nullptr;
    QPushButton* m_scanBtn = nullptr;
    QPushButton* m_fixBtn = nullptr;
    QPushButton* m_fixAllBtn = nullptr;
    QPushButton* m_autoFixBtn = nullptr;
    QPushButton* m_exportBtn = nullptr;
};

} // namespace ks
