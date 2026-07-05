#pragma once

#include "core/editor/EditorModule.h"
#include <QString>
#include <QStringList>
#include <QMap>
#include <QUrl>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QVariantList>
#include <QVariantMap>
#include <QSet>

namespace ks {

struct WorkshopItem {
    quint64 workshopId;
    QString title;
    QString description;
    QString author;
    QString category;
    quint64 sizeBytes;
    QString previewUrl;
    QUrl workshopUrl;
    QDateTime updated;
    bool installed;
    QString localPath;

    bool operator==(const WorkshopItem& other) const {
        return workshopId == other.workshopId;
    }
};

class WorkshopDownloader : public QObject {
    Q_OBJECT
public:
    explicit WorkshopDownloader(QObject* parent = nullptr);
    ~WorkshopDownloader();

    void setOutputDirectory(const QString& dir);
    void downloadItem(const WorkshopItem& item);
    void downloadItems(const QVector<WorkshopItem>& items);
    void cancelDownload(quint64 workshopId);
    void cancelAll();

signals:
    void downloadStarted(quint64 workshopId, const QString& title);
    void downloadProgress(quint64 workshopId, qint64 received, qint64 total);
    void downloadFinished(quint64 workshopId, const QString& localPath);
    void downloadFailed(quint64 workshopId, const QString& error);
    void allDownloadsFinished();

private slots:
    void onFinished();
    void onReadyRead();

private:
    void startNextDownload();
    QString getCategoryInstallPath(const QString& category) const;

    QString m_outputDir;
    QNetworkAccessManager* m_networkManager = nullptr;
    QVector<WorkshopItem> m_queue;
    QMap<quint64, QNetworkReply*> m_activeDownloads;
    QMap<quint64, QByteArray> m_buffers;
    bool m_downloading = false;
};

// ── Update info for installed workshop items ─────────────────────────────
struct WorkshopUpdateInfo {
    quint64 workshopId;
    QString title;
    QString currentVersion;
    QString availableVersion;
    QUrl downloadUrl;
    QString changelog;
};

// ── Dependency info ─────────────────────────────────────────────────────
struct WorkshopDependency {
    quint64 workshopId;
    QString title;
    QString requiredVersion;
    bool satisfied;
    QStringList conflictsWith;
};

class WorkshopModule : public EditorModule {
    Q_OBJECT
public:
    explicit WorkshopModule(QWidget* parent = nullptr);
    ~WorkshopModule() override;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "Workshop"; }
    QString moduleId() const override { return "workshop"; }
    QString getModuleIcon() const override { return ":/icons/workshop.svg"; }
    int getModulePriority() const override { return 40; }
    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;

private:
    void setupUi(QMainWindow* mainWindow, QDockWidget* dock);

public slots:
    void refreshWorkshop();
    void browseCategory(const QString& category);
    void searchItems(const QString& query);
    void downloadItem(const WorkshopItem& item);
    void downloadSelected();
    void viewInstalled();
    void openInBrowser(const WorkshopItem& item);

    // ── Update checking ──────────────────────────────────────────────────
    void checkForUpdates();
    void updateItem(quint64 workshopId);
    void updateAll();
    int pendingUpdates() const { return m_pendingUpdates.size(); }
    QVector<WorkshopUpdateInfo> getPendingUpdates() const { return m_pendingUpdates; }

    // ── Dependency resolution ────────────────────────────────────────────
    void resolveDependencies(quint64 workshopId);
    QVector<WorkshopDependency> getDependencies(quint64 workshopId) const;
    QVector<WorkshopDependency> getConflicts(quint64 workshopId) const;
    bool canInstall(quint64 workshopId, QStringList& blockingConflicts);

signals:
    void workshopLoaded(const QVector<WorkshopItem>& items);
    void downloadProgress(quint64 workshopId, int percent);
    void downloadCompleted(quint64 workshopId);
    void statusMessage(const QString& message);
    void itemsLoaded(const QVector<WorkshopItem>& items);
    void updatesAvailable(int count);
    void updateProgress(quint64 workshopId, int percent);
    void updateCompleted(quint64 workshopId, bool success);
    void conflictDetected(const QString& item1, const QString& item2);

private:
    void loadWorkshopItems();
    void parseWorkshopPage(const QString& html);
    QString getWorkshopUrlForCategory(const QString& category);
    void checkItemVersion(quint64 workshopId, const QString& currentVersion);

    QNetworkAccessManager* m_networkManager = nullptr;
    WorkshopDownloader* m_downloader = nullptr;
    QVector<WorkshopItem> m_items;
    QVector<WorkshopItem> m_installedItems;
    QString m_currentCategory;

    // Update tracking
    QVector<WorkshopUpdateInfo> m_pendingUpdates;
    QSet<quint64> m_updatingItems;
    QMap<quint64, QString> m_installedVersions;

    // Dependency tracking
    QMap<quint64, QVector<WorkshopDependency>> m_dependencyCache;
};

class WorkshopQmlBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(int itemCount READ itemCount NOTIFY itemCountChanged)
    Q_PROPERTY(QString currentCategory READ currentCategory NOTIFY categoryChanged)

public:
    static WorkshopQmlBridge* instance();

    bool isLoading() const { return m_isLoading; }
    int itemCount() const { return m_itemCount; }
    QString currentCategory() const { return m_currentCategory; }

    Q_INVOKABLE void refreshWorkshop();
    Q_INVOKABLE void browseCategory(const QString& category);
    Q_INVOKABLE void searchItems(const QString& query);
    Q_INVOKABLE QVariantList getItems();
    Q_INVOKABLE QVariantMap getItem(int index);
    Q_INVOKABLE void downloadItem(int index);
    Q_INVOKABLE void downloadItemById(qint64 workshopId);
    Q_INVOKABLE void cancelDownload(qint64 workshopId);
    Q_INVOKABLE void cancelAllDownloads();
    Q_INVOKABLE QVariantList getInstalledItems();
    Q_INVOKABLE void openInBrowser(int index);
    Q_INVOKABLE QStringList getCategories() const { return {"cars", "tracks", "skins", "apps", "tools"}; }
    Q_INVOKABLE void setOutputDirectory(const QString& dir);

signals:
    void loadingChanged();
    void itemCountChanged();
    void categoryChanged();
    void downloadProgress(qint64 workshopId, int percent);
    void downloadFinished(qint64 workshopId, const QString& path);
    void downloadFailed(qint64 workshopId, const QString& error);

private:
    static WorkshopQmlBridge* s_instance;
    WorkshopQmlBridge(QObject* parent = nullptr) : QObject(parent) {}

    void parseWorkshopHTML(const QString& html);

    bool m_isLoading = false;
    int m_itemCount = 0;
    QString m_currentCategory;
    QString m_outputDir;
    QVector<WorkshopItem> m_items;
    QNetworkAccessManager* m_networkManager = nullptr;
    WorkshopDownloader* m_downloader = nullptr;
};

} // namespace ks
