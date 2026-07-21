#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVariantList>
#include <QUrl>
#include <QDateTime>
#include <QVector>
#include "../editor/EditorModule.h"
#include "WorkshopItem.h"
#include "WorkshopManager.h"

namespace ks {

class WorkshopEditorQmlBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(int itemCount READ itemCount NOTIFY itemCountChanged)
    Q_PROPERTY(QString currentCategory READ currentCategory WRITE setCurrentCategory NOTIFY categoryChanged)
    Q_PROPERTY(QStringList categories READ categories NOTIFY categoriesChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    static WorkshopEditorQmlBridge* instance();

    bool isLoading() const { return m_isLoading; }
    int itemCount() const { return m_currentItems.size(); }
    QString currentCategory() const { return m_currentCategory; }
    void setCurrentCategory(const QString& category);
    QStringList categories() const { return WorkshopItem::standardCategories(); }
    QString statusMessage() const { return m_status; }

    Q_INVOKABLE void browseCategory(const QString& category);
    Q_INVOKABLE void browseAll();
    Q_INVOKABLE void searchItems(const QString& query);
    Q_INVOKABLE QVariantList getItems() const;
    Q_INVOKABLE QVariantMap getItem(int index) const;
    Q_INVOKABLE QVariantMap getItemById(const QString& id) const;

    Q_INVOKABLE void publishMod(const QString& sourcePath, const QString& name,
                                const QString& version, const QString& author,
                                const QString& description, const QString& category);
    Q_INVOKABLE void installItem(int index, bool autoInstallDeps = false);
    Q_INVOKABLE void installItemById(const QString& id, bool autoInstallDeps = false);
    Q_INVOKABLE void uninstallItem(int index);
    Q_INVOKABLE void removeItem(int index);
    Q_INVOKABLE void rateItem(int index, int rating);
    Q_INVOKABLE void rateItemById(const QString& id, int rating);

    Q_INVOKABLE void downloadItem(int index);
    Q_INVOKABLE void cancelDownload(int index);
    Q_INVOKABLE QVariantList getInstalledItems() const;
    Q_INVOKABLE void openInBrowser(int index);
    Q_INVOKABLE void importMod(const QString& filePath);
    Q_INVOKABLE void exportMod(const QString& filePath);

    // Dependency resolution exposed to QML
    Q_INVOKABLE QVariantMap resolveDependencies(int index);
    Q_INVOKABLE QVariantMap resolveDependenciesForId(const QString& id);
    Q_INVOKABLE bool areDependenciesSatisfied(int index);
    Q_INVOKABLE QVariantList checkForUpdates() const;
    Q_INVOKABLE bool resolveConflict(const QString& itemId, const QString& conflictingId, bool keepItem = true);
    Q_INVOKABLE QVariantList getConflicts(const QString& itemId) const;

    // Profile system
    Q_PROPERTY(QString activeProfile READ activeProfile NOTIFY activeProfileChanged)
    Q_PROPERTY(QStringList profileNames READ profileNames NOTIFY profileListChanged)
    QString activeProfile() const;
    QStringList profileNames() const;
    Q_INVOKABLE bool createProfile(const QString& name, const QString& description = QString());
    Q_INVOKABLE bool deleteProfile(const QString& name);
    Q_INVOKABLE bool renameProfile(const QString& oldName, const QString& newName);
    Q_INVOKABLE bool activateProfile(const QString& name);
    Q_INVOKABLE bool saveCurrentStateAsProfile(const QString& name);
    Q_INVOKABLE QVariantMap getProfile(const QString& name) const;
    Q_INVOKABLE QVariantList getProfileEntries(const QString& name) const;

signals:
    void loadingChanged();
    void itemCountChanged();
    void categoryChanged();
    void categoriesChanged();
    void statusMessageChanged();
    void downloadStarted(int index);
    void downloadProgress(int index, int percent);
    void downloadFinished(int index, const QString& path);
    void downloadFailed(int index, const QString& error);
    void downloadCancelled(int index);
    void itemsLoaded(const QVariantList& items);
    void publishFinished(const QString& itemId);
    void installFinished(const QString& itemName);
    void operationFailed(const QString& error);
    void conflictResolved(const QString& itemId, const QString& conflictingId, bool keptItem);
    void profileCreated(const QString& name);
    void profileDeleted(const QString& name);
    void profileActivated(const QString& name);
    void activeProfileChanged();
    void profileListChanged();

private:
    WorkshopEditorQmlBridge(QObject* parent = nullptr);
    static WorkshopEditorQmlBridge* s_instance;

    QVariantMap itemToVariant(const WorkshopItem& item) const;
    void refreshItems();
    void setStatus(const QString& msg);

    WorkshopManager* m_manager = nullptr;
    QString m_status;
    QString m_currentCategory;
    QVector<WorkshopItem> m_currentItems;
    bool m_isLoading = false;
};

}
