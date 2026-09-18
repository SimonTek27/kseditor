#pragma once

#include "ModManager.h"
#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

namespace ks {

class ModManagerQmlBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(int modCount READ modCount NOTIFY modCountChanged)
    Q_PROPERTY(bool useHardLinks READ useHardLinks WRITE setUseHardLinks NOTIFY useHardLinksChanged)
    Q_PROPERTY(int updatesAvailable READ updatesAvailable NOTIFY updatesChanged)
    Q_PROPERTY(QString currentProfile READ currentProfile NOTIFY profileChanged)
    Q_PROPERTY(QStringList profileNames READ profileNames NOTIFY profilesChanged)

public:
    static ModManagerQmlBridge* instance();

    int modCount() const { return m_mods.size(); }
    bool useHardLinks() const { return m_useHardLinks; }
    void setUseHardLinks(bool use);
    int updatesAvailable() const { return m_updatesAvailable; }
    QString currentProfile() const;
    QStringList profileNames() const;

    Q_INVOKABLE void installMod(const QString& zipPath);
    Q_INVOKABLE void uninstallMod(const QString& modName);
    Q_INVOKABLE void refreshMods();
    Q_INVOKABLE QVariantList getMods();
    Q_INVOKABLE QVariantMap getMod(int index);
    Q_INVOKABLE void enableMod(const QString& modName);
    Q_INVOKABLE void disableMod(const QString& modName);
    Q_INVOKABLE void setModPriority(const QString& modName, int priority);
    Q_INVOKABLE QStringList getCategories() const;
    Q_INVOKABLE QVariantList getDependencyInfo(const QString& modName);
    Q_INVOKABLE QStringList getConflicts(const QString& modName);
    Q_INVOKABLE void backupMod(const QString& modName);
    Q_INVOKABLE void restoreMod(const QString& modName);

    // Enhanced dependency management
    Q_INVOKABLE QVariantList getDependencyTree(const QString& modName);
    Q_INVOKABLE void installModWithDependencies(const QString& zipPath);
    Q_INVOKABLE QVariantMap getModDetails(int index);
    Q_INVOKABLE void checkForUpdates();

    // Profile management
    Q_INVOKABLE void switchProfile(const QString& name);
    Q_INVOKABLE void createProfile(const QString& name, const QString& description = "");
    Q_INVOKABLE void deleteProfile(const QString& name);
    Q_INVOKABLE void duplicateProfile(const QString& name, const QString& newName);

    // Conflict resolution
    Q_INVOKABLE QVariantList getFileConflicts();
    Q_INVOKABLE void resolveConflict(const QString& modA, const QString& modB, int keepModIndex);

    // Collection management
    Q_INVOKABLE QVariantList getCollections();
    Q_INVOKABLE bool createCollection(const QString& name, const QString& description = "");
    Q_INVOKABLE bool deleteCollection(const QString& id);
    Q_INVOKABLE bool addModToCollection(const QString& collectionId, const QString& modName);
    Q_INVOKABLE bool removeModFromCollection(const QString& collectionId, const QString& modName);
    Q_INVOKABLE QStringList collectionMods(const QString& collectionId) const;
    Q_INVOKABLE QStringList modCollections(const QString& modName) const;

    // Statistics
    Q_INVOKABLE QVariantMap getStats();

signals:
    void modCountChanged();
    void useHardLinksChanged();
    void installProgress(const QString& modName, int percent);
    void installFinished(const QString& modName, bool success);
    void refreshFinished();
    void updatesChanged();
    void profileChanged();
    void profilesChanged();
    void dependencyResolved(const QString& modName, bool allSatisfied);
    void collectionsChanged();

public:
    static void setModule(ModManagerModule* mod) { s_module = mod; }

private:
    ModManagerQmlBridge(QObject* parent = nullptr);
    static ModManagerQmlBridge* s_instance;
    static ModManagerModule* s_module;

    void connectSignals();
    ModManagerModule* module() const;

    QVector<ModEntry> m_mods;
    bool m_useHardLinks = true;
    int m_updatesAvailable = 0;
};

} // namespace ks
