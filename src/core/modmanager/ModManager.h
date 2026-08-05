#pragma once

#include "../editor/EditorModule.h"
#include <QString>
#include <QStringList>
#include <QMap>
#include <QJsonObject>
#include <QVariantList>
#include <QVariantMap>
#include <QDateTime>
#include <QProcess>
#include <QQueue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QElapsedTimer>
#include <QMutex>
#include <QCache>
#include <QRegularExpression>
#include <QDialog>
#include <QTreeWidget>
#include <QTreeWidgetItem>

class QListWidget;
class QLabel;
class QPushButton;
class QComboBox;
class QTabWidget;
class QMenu;
class QAction;
class QFileSystemWatcher;
class QGraphicsScene;
class QGraphicsView;

namespace ks {

class CollectionManager;
class WorkshopManager;
class WorkshopItem;
}

namespace ks {

// ============================================================================
// VersionSpec — semver (v2.0.0) parsing & constraint matching
// ============================================================================
struct Version {
    int major = 0;
    int minor = 0;
    int patch = 0;
    QString preRelease;

    static Version fromString(const QString& str);
    int compare(const Version& other) const;
    bool operator==(const Version& other) const;
    bool operator!=(const Version& other) const { return !(*this == other); }
    bool operator<(const Version& other) const { return compare(other) < 0; }
    bool operator<=(const Version& other) const { return compare(other) <= 0; }
    bool operator>(const Version& other) const { return compare(other) > 0; }
    bool operator>=(const Version& other) const { return compare(other) >= 0; }
    QString toString() const;
    bool isValid() const { return major >= 0 && minor >= 0 && patch >= 0; }
    bool isEmpty() const { return major == 0 && minor == 0 && patch == 0 && preRelease.isEmpty(); }
};

struct VersionSpec {
    enum Op { Any, Eq, Neq, Gt, Lt, Ge, Le, Tilde, Caret };
    Version version;
    Op op = Any;
    QString raw;

    bool matches(const Version& v) const;
    bool isValid() const;
    static VersionSpec fromString(const QString& spec);
    static QString opToString(Op o);
};

struct ModEntry {
    QString name;
    Version version;
    QString versionStr;
    QString author;
    QString category;
    qint64 sizeBytes;
    QDateTime installDate;
    QDateTime lastUpdateCheck;
    QString path;
    QString description;
    bool enabled = true;
    bool isBuiltIn = false;
    bool hasUpdate = false;
    QString newVersion;
    QString updateUrl;
    QStringList dependencies;
    QMap<QString, VersionSpec> dependencySpecs; // dep name -> version constraint
    QStringList conflicts;
    QMap<QString, QString> metadata;
};

class ModScanner : public QObject {
    Q_OBJECT
public:
    explicit ModScanner(QObject* parent = nullptr);

    void scanDirectory(const QString& path);
    void scanACInstallation(const QString& acRoot);
    void cancelScan();

signals:
    void modFound(const QString& name, const QString& path, const QString& category);
    void scanProgress(int current, int total);
    void scanFinished();

private:
    void scanForMods(const QString& dir, const QString& category);
    void scanSingleFile(const QString& filePath);

public:
    QJsonObject readModManifest(const QString& manifestPath);
    QString detectModCategory(const QString& path);

    bool m_cancelled = false;
    QSet<QString> m_scannedPaths;
};

class DependencyResolver : public QObject {
    Q_OBJECT
public:
    explicit DependencyResolver(QObject* parent = nullptr);

    struct ResolvedDep {
        QString depName;
        VersionSpec spec;
        bool satisfied = false;
        QString installedVersion;
        QString resolvedBy;    // which mod provides this dep
    };

    struct Resolution {
        bool satisfied = true;
        QStringList missingDeps;
        QStringList conflictingMods;
        QStringList resolvedOrder;
        QStringList circularDeps;
        QMap<QString, QVector<ResolvedDep>> depDetails; // mod -> deps with status
    };

    Resolution resolve(const QVector<ModEntry>& allMods, const QStringList& targetMods);
    QStringList topologicalSort(const QVector<ModEntry>& mods);
    QStringList findMissing(const QVector<ModEntry>& allMods, const QStringList& targetMods);
    QStringList findConflicts(const QVector<ModEntry>& allMods, const QStringList& targetMods);

    // Version-aware resolution
    bool checkConstraint(const VersionSpec& spec, const Version& installed) const;
    QStringList findVersionConflicts(const QVector<ModEntry>& allMods,
                                      const QStringList& targetMods) const;
    QVector<ResolvedDep> resolveDependencyDetails(const QVector<ModEntry>& allMods,
                                                    const QString& modName) const;
    QVector<QPair<QString, QStringList>> buildDependencyTree(
        const QVector<ModEntry>& allMods, const QString& rootMod,
        int maxDepth = -1) const;

    // Reverse dependency lookup
    QStringList reverseDependencies(const QVector<ModEntry>& allMods,
                                     const QString& modName) const;

signals:
    void resolutionChanged(const QStringList& unsatisfied);
};

// ============================================================================
// ModConflictDetector — detects file overlaps between mods
// ============================================================================
class ModConflictDetector : public QObject {
    Q_OBJECT
public:
    explicit ModConflictDetector(QObject* parent = nullptr);

    struct FileConflict {
        QString filePath;
        QStringList modNames;
        QString conflictType; // "overwrite", "merge", "duplicate"
    };

    struct ConflictReport {
        QVector<FileConflict> conflicts;
        QStringList criticalConflicts; // files that cannot be merged
        bool hasCritical = false;
    };

    // Scan mods for file overlaps
    ConflictReport detectConflicts(const QVector<ModEntry>& mods, const QString& gameRoot) const;

    // Get list of files a mod would install (for preview)
    QStringList getModFiles(const ModEntry& mod) const;

    // Check if a specific mod conflicts with already installed mods
    ConflictReport checkModConflicts(const ModEntry& newMod, const QVector<ModEntry>& installedMods, const QString& gameRoot) const;

signals:
    void scanProgress(int current, int total);
    void scanFinished(const ConflictReport& report);
};

class ModInstallEngine : public QObject {
    Q_OBJECT
public:
    explicit ModInstallEngine(QObject* parent = nullptr);

    void setModDirectory(const QString& dir);
    void setUseHardLinks(bool useHardLinks);

    bool installMod(const QString& zipPath);
    bool uninstallMod(const QString& modName);
    bool enableMod(const QString& modName);
    bool disableMod(const QString& modName);
    bool createBackup(const QString& modName);
    bool restoreBackup(const QString& modName);

    // Batch operations
    struct BatchResult {
        QStringList succeeded;
        QStringList failed;
        QStringList skipped;
    };
    bool installMods(const QStringList& zipPaths);
    bool uninstallMods(const QStringList& modNames);
    BatchResult getLastBatchResult() const { return m_lastBatchResult; }

    // File integrity
    struct IntegrityResult {
        QString modName;
        bool intact = true;
        int totalFiles = 0;
        int checkedFiles = 0;
        int corruptedFiles = 0;
        QStringList corruptedPaths;
        QMap<QString, QString> fileHashes; // path -> hash
    };
    IntegrityResult verifyModIntegrity(const QString& modName);
    QVector<IntegrityResult> verifyAllModsIntegrity();
    bool repairMod(const QString& modName);
    void setAutoVerifyIntegrity(bool enabled) { m_autoVerify = enabled; }
    bool autoVerifyIntegrity() const { return m_autoVerify; }

    QStringList getInstalledMods() const { return m_installedMods.keys(); }
    QString getModPath(const QString& modName) const;

signals:
    void installProgress(int percent, const QString& message);
    void installFinished(const QString& modName, bool success);
    void uninstallFinished(const QString& modName, bool success);
    void batchInstallProgress(int current, int total, const QString& modName);
    void batchInstallFinished(const BatchResult& result);
    void integrityCheckProgress(int current, int total, const QString& modName);
    void integrityCheckFinished(const IntegrityResult& result);

private:
    bool extractZip(const QString& zipPath, const QString& outputDir);
    bool createHardLink(const QString& target, const QString& link);
    QString getModInstallPath(const QString& modName) const;
    QString getBackupPath(const QString& modName) const;

    // Integrity helpers
    QString calculateFileHash(const QString& filePath) const;
    bool verifySingleFile(const QString& filePath, const QString& expectedHash) const;
    void saveIntegrityManifest(const QString& modName, const QMap<QString, QString>& hashes);
    QMap<QString, QString> loadIntegrityManifest(const QString& modName) const;

    QString m_modDir;
    bool m_useHardLinks = true;
    bool m_autoVerify = true;
    QMap<QString, QString> m_installedMods;
    QMap<QString, QString> m_backupPaths;
    QMap<QString, QJsonObject> m_modMetadata;
    BatchResult m_lastBatchResult;
    int m_batchTotal = 0;
    int m_batchCurrent = 0;
};

// ============================================================================
// ModUpdateChecker — checks for mod updates from remote sources
// ============================================================================
class ModUpdateChecker : public QObject {
    Q_OBJECT
public:
    explicit ModUpdateChecker(QObject* parent = nullptr);

    void checkForUpdates(const QVector<ModEntry>& mods);
    void checkSingleMod(const ModEntry& mod);
    void cancelAll();

    // Download updates
    void downloadUpdate(const QString& modName, const QString& downloadUrl);
    void downloadAllUpdates();
    void setDownloadDirectory(const QString& dir) { m_downloadDir = dir; }
    QString downloadDirectory() const { return m_downloadDir; }

    bool isChecking() const { return m_checking; }
    bool isDownloading() const { return m_downloading; }
    int updatesAvailable() const { return m_updates.size(); }

signals:
    void updateFound(const QString& modName, const QString& currentVersion, const QString& newVersion);
    void checkProgress(int current, int total);
    void checkFinished(int updatesFound);
    void checkError(const QString& modName, const QString& error);
    void downloadProgress(const QString& modName, qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(const QString& modName, bool success, const QString& filePath);
    void downloadError(const QString& modName, const QString& error);
    void allDownloadsFinished(int successCount, int failCount);

private:
    void processNext();
    void processNextDownload();
    void parseUpdateResponse(QNetworkReply* reply, const QString& modName);
    void handleDownloadReply(QNetworkReply* reply, const QString& modName);

    QNetworkAccessManager* m_networkManager = nullptr;
    QQueue<QString> m_checkQueue;
    QMap<QString, QString> m_modVersionMap;
    QVector<QPair<QString, QString>> m_updates;
    bool m_checking = false;
    int m_total = 0;
    int m_current = 0;

    // Download state
    struct DownloadItem {
        QString modName;
        QString url;
        QString savePath;
    };
    QQueue<DownloadItem> m_downloadQueue;
    QMap<QString, QNetworkReply*> m_activeDownloads;
    bool m_downloading = false;
    int m_downloadSuccessCount = 0;
    int m_downloadFailCount = 0;
    QString m_downloadDir;

    static const QString s_updateEndpoint;
};

// ============================================================================
// ProfileManager — manages multiple mod profiles
// ============================================================================
class ProfileManager : public QObject {
    Q_OBJECT
public:
    struct Profile {
        QString name;
        QString description;
        QMap<QString, bool> modStates; // mod name -> enabled
        QMap<QString, int> priorities;
        QDateTime created;
        QDateTime modified;
    };

    explicit ProfileManager(QObject* parent = nullptr);

    QStringList listProfiles() const;
    bool createProfile(const QString& name, const QString& description = QString());
    bool deleteProfile(const QString& name);
    bool renameProfile(const QString& oldName, const QString& newName);
    bool duplicateProfile(const QString& name, const QString& newName);

    bool saveProfile(const QString& name, const QVector<ModEntry>& mods, const QMap<QString, int>& priorities);
    bool loadProfile(const QString& name, QVector<ModEntry>& mods, QMap<QString, int>& priorities);
    Profile getProfile(const QString& name) const;

    QString currentProfile() const { return m_currentProfile; }
    void setCurrentProfile(const QString& name);

    bool exportProfile(const QString& name, const QString& filePath);
    bool importProfile(const QString& filePath);

signals:
    void profileCreated(const QString& name);
    void profileDeleted(const QString& name);
    void profileRenamed(const QString& oldName, const QString& newName);
    void profileLoaded(const QString& name);
    void profilesChanged();

private:
    QString profilesDir() const;
    QString profileFilePath(const QString& name) const;
    void loadProfileList();
    void saveProfileList();

    QString m_currentProfile = "Default";
    QMap<QString, Profile> m_profiles;
};

// ============================================================================
// ModRepository — queries WorkshopManager and remote sources for mod metadata
// ============================================================================
class ModRepository : public QObject {
    Q_OBJECT
public:
    struct ModInfo {
        QString name;
        QString version;
        QString id;           // Workshop UUID
        QString author;
        QString category;
        QString description;
        QStringList tags;
        QMap<QString, VersionSpec> dependencies;  // dep name -> version constraint
        QStringList conflicts;
        QString downloadUrl;
        qint64 fileSize = 0;
        float rating = 0;
        int downloadCount = 0;
    };

    explicit ModRepository(QObject* parent = nullptr);
    ~ModRepository();

    void setWorkshopManager(WorkshopManager* workshop);
    void addRemoteSource(const QString& name, const QString& baseUrl);

    // Search for mods matching criteria
    QVector<ModInfo> search(const QString& query = QString(),
                            const QString& category = QString(),
                            const QStringList& tags = QStringList()) const;

    // Get specific mod by name (returns all versions available)
    QVector<ModInfo> getModVersions(const QString& name) const;

    // Get latest version satisfying constraint
    ModInfo* resolveDependency(const QString& depName, const VersionSpec& spec);

    // Fetch and install a mod (with its dependencies) into ModManager
    bool fetchAndInstall(const QString& modName, const VersionSpec& spec = VersionSpec());

    // Refresh from WorkshopManager
    void refresh();

    // Download a mod package (returns true on success)
    bool downloadMod(const ModInfo& info, QString& outputPath);

signals:
    void repositoryUpdated();
    void fetchProgress(const QString& modName, int percent);
    void fetchFinished(const QString& modName, bool success, const QString& error = QString());

private:
    WorkshopManager* m_workshop = nullptr;
    QMap<QString, QVector<ModInfo>> m_modIndex;  // name -> versions
    QMap<QString, QString> m_remoteSources;      // name -> baseUrl
    QNetworkAccessManager* m_networkManager = nullptr;

    void buildIndexFromWorkshop();
    QVector<ModInfo> convertWorkshopItem(const WorkshopItem& item) const;
    bool downloadRemoteFile(const QString& url, const QString& outputPath);
};

// ============================================================================
// DependencyTreeDialog — visual tree of mod dependencies
// ============================================================================
class DependencyTreeDialog : public QDialog {
    Q_OBJECT
public:
    explicit DependencyTreeDialog(const QVector<ModEntry>& allMods,
                                   const QString& rootMod,
                                   QWidget* parent = nullptr);

    void setMaxDepth(int depth) { m_maxDepth = depth; }

private:
    void setupUI();
    void populateTree();
    void addDepChildren(QTreeWidgetItem* parent, const QString& modName,
                        QSet<QString>& visited, int depth);

    const QVector<ModEntry>& m_mods;
    QString m_rootMod;
    int m_maxDepth = -1;
    QTreeWidget* m_tree = nullptr;
};

class ModManagerModule : public EditorModule {
    Q_OBJECT
public:
    explicit ModManagerModule(QWidget* parent = nullptr);
    static ModManagerModule* instance();
    ~ModManagerModule() override;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "Mod Manager"; }
    QString moduleId() const override { return "modmanager"; }
    QString getModuleIcon() const override { return ":/icons/modmanager.svg"; }
    int getModulePriority() const override { return 35; }
    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;

    ModInstallEngine* installEngine() const { return m_installEngine; }
    DependencyResolver* dependencyResolver() const { return m_dependencyResolver; }
    ModUpdateChecker* updateChecker() const { return m_updateChecker; }
    ProfileManager* profileManager() const { return m_profileManager; }
    ModRepository* repository() const { return m_repository; }
    ModConflictDetector* conflictDetector() const { return m_conflictDetector; }
    CollectionManager* collectionManager() const { return m_collectionManager; }
    const QVector<ModEntry>& mods() const { return m_mods; }
    const QString& modDirectory() const { return m_modDirectory; }

public slots:
    void refreshMods();
    void installMod(const QString& zipPath);
    void uninstallMod(const QString& modName);
    void enableMod(const QString& modName);
    void disableMod(const QString& modName);
    void setModPriority(const QString& modName, int priority);
    void checkForUpdates();
    void switchProfile(const QString& name);
    void createProfile(const QString& name);
    void deleteProfile(const QString& name);
    void saveCurrentProfile();
    void showUpdateDetails();

    // Batch operations
    void installModsBatch(const QStringList& zipPaths);
    void uninstallModsBatch(const QStringList& modNames);
    void selectAllMods();
    void deselectAllMods();
    QStringList getSelectedMods() const;

    // Integrity operations
    void verifyAllIntegrity();
    void verifyModIntegrity(const QString& modName);
    void repairAllMods();
    void repairMod(const QString& modName);

    // Update operations
    void downloadAllUpdates();
    void downloadModUpdate(const QString& modName);

    // Load order management
    void applyLoadOrder();
    void recalculateLoadOrder();
    QStringList getCurrentLoadOrder() const;
    void saveLoadOrder();
    void loadLoadOrder();

    // Dependency management
    void installModWithDependencies(const QString& zipPath);
    QStringList resolveDependencies(const QStringList& targetMods);
    void showDependencyDialog(const QString& modName);
    void showDependencyTree(const QString& modName);
    void resolveVersionConflicts();

    // Conflict management
    void checkForConflicts();
    void showConflictDialog(const QStringList& conflicts);
    bool hasConflicts(const QString& modName) const;

    // AC Integration
    void syncWithACPriorityIni();
    void writeACPriorityIni();
    bool validateACPriorityIni();

    // Repository
    void refreshRepository();
    void installModFromRepository(const QString& modName);
    void installMissingDependencies(const QStringList& targetMods);

    // Conflict Detection
    void scanForFileConflicts();
    void showConflictReport(const ModConflictDetector::ConflictReport& report);

    // Performance
    void enableLazyLoading(bool enabled);
    void preloadModDetails();

    // Collection management
    void addModToCollection(const QString& collectionId, const QString& modName);
    void removeModFromCollection(const QString& collectionId, const QString& modName);
    void createCollection(const QString& name, const QString& description = QString());
    void deleteCollection(const QString& id);
    QStringList modCollections(const QString& modName) const;
    QStringList collectionMods(const QString& collectionId) const;

    // Statistics
public:
    struct ModStats {
        int totalMods = 0;
        int enabledMods = 0;
        int disabledMods = 0;
        int builtInMods = 0;
        int hasUpdates = 0;
        int withDependencies = 0;
        int withConflicts = 0;
        int integrityFailed = 0;
        qint64 totalSizeBytes = 0;
        QMap<QString, int> categoryCounts;
        QMap<QString, qint64> categorySizes;
        double avgModSizeMB = 0;
    };
    ModStats calculateStats() const;
    Q_INVOKABLE QVariantMap getStats() const;

public slots:

    // Dependency graph
    void showDependencyGraph(const QString& rootMod = QString());

    // Load order drag-and-drop
    void moveModInLoadOrder(const QString& modName, int newIndex);
    void setModListDragDropEnabled(bool enabled);

signals:
    void modsChanged();
    void modInstalled(const QString& name);
    void modUninstalled(const QString& name);
    void updatesAvailable(int count);
    void profileChanged(const QString& name);
    void batchInstallProgress(int current, int total, const QString& modName);
    void batchInstallFinished(int successCount, int failCount);
    void integrityCheckProgress(int current, int total, const QString& modName);
    void integrityCheckFinished(int totalChecked, int totalCorrupted);
    void downloadProgress(const QString& modName, int percent);

private:
    void loadModsFromDisk();
    void saveModState();
    void loadModState();
    void syncModsWithState();
    void scanACContentMods();
    void populateModList();
    void populateProfileCombo();
    void handleUpdateResults();

    // Batch helpers
    void processBatchInstall();
    void processBatchUninstall();

    // Integrity helpers
    void handleIntegrityResult(const ModInstallEngine::IntegrityResult& result);

    // Performance: lazy load mod details
    void loadModDetailsAsync(int index);
    bool m_detailsPreloaded = false;

    ModInstallEngine* m_installEngine = nullptr;
    DependencyResolver* m_dependencyResolver = nullptr;
    ModUpdateChecker* m_updateChecker = nullptr;
    ProfileManager* m_profileManager = nullptr;
    ModRepository* m_repository = nullptr;
    ModConflictDetector* m_conflictDetector = nullptr;
    CollectionManager* m_collectionManager = nullptr;
    QVector<ModEntry> m_mods;
    QMap<QString, int> m_priorities;
    QString m_modDirectory;
    int m_pendingUpdates = 0;

    // Batch state
    QStringList m_pendingBatchInstalls;
    QStringList m_pendingBatchUninstalls;
    int m_batchTotal = 0;
    int m_batchCurrent = 0;

    // Dock widget UI
    QListWidget* m_modList = nullptr;
    QLabel* m_summaryLabel = nullptr;
    QPushButton* m_toggleBtn = nullptr;
    QPushButton* m_updateBtn = nullptr;
    QPushButton* m_scanConflictsBtn = nullptr;
    QComboBox* m_profileCombo = nullptr;
    QLabel* m_conflictLabel = nullptr;
    QFileSystemWatcher* m_fsWatcher = nullptr;
    QGraphicsScene* m_depGraphScene = nullptr;
    QGraphicsView* m_depGraphView = nullptr;
    bool m_preloadScheduled = false;
    bool m_suppressConflictDialog = false;
    bool m_dragDropEnabled = true;
};

} // namespace ks
