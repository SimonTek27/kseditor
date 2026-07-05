#pragma once

#include "WorkshopItem.h"
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QProcess>
#include <functional>

namespace ks {

// Lightweight semver for dependency resolution
struct DepVersion {
    int major = 0, minor = 0, patch = 0;
    QString preRelease;

    static DepVersion fromString(const QString& str);
    int compare(const DepVersion& other) const;
    bool isValid() const { return major != 0 || minor != 0 || patch != 0 || !preRelease.isEmpty(); }
    QString toString() const;
};

struct WorkshopVersionSpec {
    enum Op { Any, Eq, Neq, Gt, Lt, Ge, Le, Tilde, Caret };
    Op op = Any;
    DepVersion version;
    QString raw;

    static WorkshopVersionSpec fromString(const QString& spec);
    bool matches(const DepVersion& v) const;
    bool isValid() const { return op == Any || version.isValid(); }
};

struct DepInfo {
    QString depName;
    WorkshopVersionSpec spec;
    bool satisfied = false;
    QString resolvedBy;
    QString resolvedVersion;
};

struct DepResolution {
    bool allSatisfied = true;
    QVector<DepInfo> dependencies;
    QStringList missingDeps;
    QStringList conflictingItems;
};

class WorkshopManager : public QObject {
    Q_OBJECT
public:
    static WorkshopManager* instance();
    static void destroyInstance();

    struct PackageResult {
        bool success = false;
        QString packagePath;
        QStringList errors;
        QStringList warnings;
    };

    struct InstallResult {
        bool success = false;
        QString modName;
        QString installPath;
        QStringList errors;
        QStringList autoInstalledDeps;
    };

    struct BrowseQuery {
        QString category;
        QString searchText;
        QString sortBy;       // "name", "date", "rating", "downloads"
        bool ascending = false;
        QStringList tags;
        int maxResults = 100;
    };

    void reset();

    PackageResult createWorkshopPackage(const QString& sourcePath, const QString& outputDir, const WorkshopItem& item);
    InstallResult installPackage(const QString& packagePath, const QString& installDir, bool autoInstallDeps = false);

    bool publishItem(const WorkshopItem& item, const QString& packagePath);
    bool removeItem(const QString& id);

    QVector<WorkshopItem> browse(const BrowseQuery& query = BrowseQuery());
    WorkshopItem getItem(const QString& id) const;
    QVector<WorkshopItem> getPublishedItems() const;
    QVector<WorkshopItem> getInstalledItems() const;
    QVector<WorkshopItem> getItemsByAuthor(const QString& author) const;
    QVector<WorkshopItem> search(const QString& text) const;

    bool rateItem(const QString& id, int rating);
    bool setInstalled(const QString& id, bool installed);

    // Dependency resolution
    DepResolution resolveDependencies(const QString& itemId);
    DepResolution resolveDependencies(const WorkshopItem& item);
    bool areDependenciesSatisfied(const QString& itemId);
    QVector<WorkshopItem> findDependencyProviders(const QString& depName) const;

    // Update checking
    struct UpdateInfo {
        QString itemId;
        QString name;
        QString currentVersion;
        QString availableVersion;
    };
    QVector<UpdateInfo> checkForUpdates();
    bool hasUpdate(const QString& itemId) const;
    bool updateItem(const QString& itemId, const QString& newVersion, const QString& packagePath);

    // ── Profile System (multi-profile support) ───────────────
    struct ProfileEntry {
        QString itemId;
        QString version;
        bool enabled = true;
    };

    struct WorkshopProfile {
        QString name;
        QString description;
        QVector<ProfileEntry> entries;
        QDateTime created;
        QDateTime modified;
    };

    bool createProfile(const QString& name, const QString& description = QString());
    bool deleteProfile(const QString& name);
    bool renameProfile(const QString& oldName, const QString& newName);
    bool saveProfile(const QString& name, const QVector<ProfileEntry>& entries);
    bool activateProfile(const QString& name);
    QString activeProfile() const { return m_activeProfile; }
    WorkshopProfile getProfile(const QString& name) const;
    QVector<WorkshopProfile> listProfiles() const;
    QVector<ProfileEntry> snapshotCurrentState() const;

    bool saveDatabase();
    bool loadDatabase();

    QString dataDir() const { return m_dataDir; }
    void setDataDir(const QString& dir);

signals:
    void databaseSaved();
    void databaseLoaded();
    void itemPublished(const QString& id);
    void itemRemoved(const QString& id);
    void itemInstalled(const QString& id);
    void itemUninstalled(const QString& id);
    void packagingProgress(const QString& message);
    void packagingFinished(const QString& packagePath);
    void packagingError(const QString& error);
    void dependenciesResolved(const QString& itemId, bool satisfied);
    void updateAvailable(const QString& itemId, const QString& newVersion);

    // Profile signals
    void profileCreated(const QString& name);
    void profileDeleted(const QString& name);
    void profileActivated(const QString& name);
    void profileSaved(const QString& name);

private:
    explicit WorkshopManager(QObject* parent = nullptr);
    static WorkshopManager* s_instance;

    QString m_dataDir;
    QString m_dbPath;
    QString m_packagesDir;
    QString m_installedDir;
    QVector<WorkshopItem> m_items;
    QMap<QString, int> m_itemIndex;

    void rebuildIndex();
    int findItem(const QString& id) const;
    bool writeJson(const QString& path, const QJsonDocument& doc);
    QJsonDocument readJson(const QString& path);
    QVector<WorkshopItem> filterItems(const BrowseQuery& query) const;

    // Profile internal
    QString m_profilesDir;
    QString m_activeProfile = "default";
    QMap<QString, WorkshopProfile> m_profiles;
    void loadProfiles();
    void saveProfiles();
    QJsonObject profileEntryToJson(const ProfileEntry& e) const;
    ProfileEntry profileEntryFromJson(const QJsonObject& obj) const;
    QJsonObject profileToJson(const WorkshopProfile& p) const;
    WorkshopProfile profileFromJson(const QJsonObject& obj) const;
};

}
