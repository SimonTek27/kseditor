#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QNetworkAccessManager>
#include <QTcpSocket>
#include <QHostAddress>
#include <QUrl>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QVector3D>
#include <QFileInfo>
#include <QTcpServer>
#include <QUuid>

namespace ks {

class CloudSync : public QObject
{
    Q_OBJECT
public:
    explicit CloudSync(QObject* parent = nullptr) : QObject(parent), m_nam(this) {}
    ~CloudSync() {}

    void setEndpoint(const QString& url) { m_endpoint = url; }
    QString endpoint() const { return m_endpoint.toString(); }

    void setApiKey(const QString& key) { m_apiKey = key; }
    QString apiKey() const { return m_apiKey; }

    bool login(const QString& username, const QString& password);
    void logout();

    bool isLoggedIn() const { return m_loggedIn; }
    QString currentUser() const { return m_username; }

    bool uploadProject(const QString& localPath, const QString& remoteName);
    bool downloadProject(const QString& remoteName, const QString& localPath);
    QVector<QString> listRemoteProjects();

    bool uploadAsset(const QString& localPath, const QString& remotePath);
    bool downloadAsset(const QString& remotePath, const QString& localPath);
    QVector<QString> listRemoteAssets(const QString& folder = QString());

    bool shareProject(const QString& projectId, const QString& userId, const QString& permission);
    QVector<QString> getCollaborators(const QString& projectId);

signals:
    void loginSuccess();
    void loginFailed(const QString& error);
    void uploadProgress(int percent);
    void downloadProgress(int percent);
    void syncComplete(bool success);
    void assetUploaded(const QString& path);
    void assetDownloaded(const QString& path);
    void projectShared(const QString& projectId, const QString& userId, const QString& permission);

private:
    QUrl m_endpoint;
    QString m_apiKey;
    QString m_username;
    bool m_loggedIn = false;
    QString m_token;
    QNetworkAccessManager m_nam;
    QMap<QString, QByteArray> m_remoteProjects;
    QMap<QString, QByteArray> m_remoteAssets;
    QMap<QString, QMap<QString, QString>> m_sharedProjects;
};

class MultiplayerServer : public QObject
{
    Q_OBJECT
public:
    explicit MultiplayerServer(QObject* parent = nullptr) : QObject(parent) {}
    ~MultiplayerServer() {}

    bool start(int port = 9000);
    void stop();

    bool isRunning() const { return m_running; }
    int port() const { return m_port; }

    void setMaxClients(int max) { m_maxClients = max; }
    int maxClients() const { return m_maxClients; }

    struct Client {
        QString id;
        QString name;
        QHostAddress address;
        quint16 port;
        bool connected;
        QTcpSocket* socket = nullptr;
    };

    QVector<Client> clients() const { return m_clients; }

    void broadcast(const QByteArray& data, const QString& excludeClient = QString());

signals:
    void clientConnected(const QString& clientId);
    void clientDisconnected(const QString& clientId);
    void dataReceived(const QString& clientId, const QByteArray& data);

private:
    bool m_running = false;
    int m_port = 9000;
    int m_maxClients = 16;
    QTcpServer* m_server = nullptr;
    QVector<Client> m_clients;
};

class MultiplayerClient : public QObject
{
    Q_OBJECT
public:
    explicit MultiplayerClient(QObject* parent = nullptr) : QObject(parent) {}
    ~MultiplayerClient() {}

    bool connectToServer(const QString& host, int port, const QString& username);
    void disconnect();

    bool isConnected() const { return m_connected; }

    void sendData(const QByteArray& data);
    void sendPosition(const QVector3D& pos, const QVector3D& velocity);
    void sendTelemetry(const QVariantMap& data);

signals:
    void connected();
    void disconnected();
    void dataSent(qint64 bytes);
    void serverMessage(const QByteArray& data);
    void playerJoined(const QString& name);
    void playerLeft(const QString& name);

private:
    bool m_connected = false;
    QString m_username;
    QString m_host;
    int m_port = 0;
    QTcpSocket* m_socket = nullptr;
};

class TelemetryServer : public QObject
{
    Q_OBJECT
public:
    explicit TelemetryServer(QObject* parent = nullptr) : QObject(parent) {}
    ~TelemetryServer() {}

    void startRecording(const QString& sessionName);
    void stopRecording();
    bool isRecording() const { return m_recording; }

    void recordFrame(const QVariantMap& telemetry);

    void addLap(int lapNumber, float time);
    void addSector(int sector, float time);

    QString getSessionData() const;
    void exportToCSV(const QString& path);
    void exportToJSON(const QString& path);

    QVector<float> getLapTimes() const { return m_lapTimes; }
    QVector<float> getSectorTimes() const { return m_sectorTimes; }

signals:
    void lapCompleted(int lap, float time);
    void sectorCompleted(int sector, float time);
    void recordingStarted();
    void recordingStopped();

private:
    QString m_sessionName;
    bool m_recording = false;
    QVector<QVariantMap> m_frameData;
    QVector<float> m_lapTimes;
    QVector<float> m_sectorTimes;
};

class WebDashboard : public QObject
{
    Q_OBJECT
public:
    explicit WebDashboard(QObject* parent = nullptr) : QObject(parent) {}
    ~WebDashboard() {}

    void startServer(int port = 8080);
    void stopServer();

    void setDataSource(TelemetryServer* telemetry) { m_telemetry = telemetry; }

    bool isRunning() const { return m_running; }

    void setAllowedOrigins(const QStringList& origins) { m_allowedOrigins = origins; }
    void setAuthRequired(bool required) { m_authRequired = required; }

signals:
    void serverStarted(int port);
    void serverStopped();

private:
    void handleRequest(QTcpSocket* socket, const QString& method, const QString& path);

    int m_port = 8080;
    bool m_running = false;
    QTcpServer* m_httpServer = nullptr;
    TelemetryServer* m_telemetry = nullptr;
    QStringList m_allowedOrigins;
    bool m_authRequired = false;
};

class RemoteAPI : public QObject
{
    Q_OBJECT
public:
    explicit RemoteAPI(QObject* parent = nullptr) : QObject(parent) {}
    ~RemoteAPI() {}

    QVariantMap getServerInfo();
    QVector<QVariantMap> getConnectedPlayers();

    bool startSession(const QString& track, const QString& car);
    bool stopSession();
    bool restartSession();

    void setTimeScale(float scale);
    void setWeather(const QString& condition);
    void kickPlayer(const QString& playerId);

    bool executeCommand(const QString& command);

signals:
    void serverInfoUpdated(const QVariantMap& info);
    void sessionStateChanged(const QString& state);
    void sessionStarted(const QString& track, const QString& car);
    void sessionStopped();
    void sessionRestarted();
    void timeScaleChanged(float scale);
    void weatherChanged(const QString& condition);
    void playerKicked(const QString& playerId);
    void commandExecuted(const QString& command);

private:
    QString m_serverUrl = "http://localhost:8081";
    QString m_currentTrack;
    QString m_currentCar;
    bool m_sessionActive = false;
    float m_timeScale = 1.0f;
    QString m_weatherCondition;
    QStringList m_kickedPlayers;
    QStringList m_commandHistory;
};

class CloudSyncService : public QObject
{
    Q_OBJECT

public:
    explicit CloudSyncService(QObject* parent = nullptr);
    ~CloudSyncService();

    enum SyncStatus {
        Status_Idle,
        Status_Syncing,
        Status_Error,
        Status_Offline
    };

    struct CloudFile {
        QString id;
        QString path;
        QString name;
        qint64 size = 0;
        QDateTime modifiedDate;
        QString checksum;
        bool isFolder = false;
        QStringList tags;
    };

    struct SyncConflict {
        QString localPath;
        QString remotePath;
        QDateTime localModified;
        QDateTime remoteModified;
        enum Resolution { KeepLocal, KeepRemote, KeepBoth } resolution;
        bool operator==(const SyncConflict& other) const {
            return localPath == other.localPath && remotePath == other.remotePath;
        }
    };

    void setServerUrl(const QString& url);
    QString serverUrl() const { return m_serverUrl; }

    void setAuthToken(const QString& token);
    QString authToken() const { return m_authToken; }

    bool login(const QString& username, const QString& password);
    void logout();
    bool isLoggedIn() const { return m_loggedIn; }

    void syncProject(const QString& localPath);
    void syncFile(const QString& localPath);
    void downloadFile(const QString& remoteId, const QString& localPath);
    void uploadFile(const QString& localPath);

    QVector<CloudFile> listFiles(const QString& path = QString());
    QVector<CloudFile> searchFiles(const QString& query);

    void createFolder(const QString& path);
    void deleteFile(const QString& remoteId);
    void moveFile(const QString& remoteId, const QString& newPath);

    void resolveConflict(const SyncConflict& conflict);

    SyncStatus status() const { return m_status; }
    float syncProgress() const { return m_syncProgress; }

    QStringList getSyncHistory(const QString& path);
    void clearSyncHistory();

    void setAutoSyncEnabled(bool enabled);
    bool autoSyncEnabled() const { return m_autoSync; }

    void setSyncInterval(int minutes);
    int syncInterval() const { return m_syncInterval; }

    QNetworkAccessManager* networkManager() const { return m_networkManager; }

signals:
    void loginSuccess(const QString& userId);
    void loginFailed(const QString& error);
    void logoutComplete();
    void syncStarted();
    void syncProgress(float percent);
    void syncComplete(int filesUploaded, int filesDownloaded);
    void syncError(const QString& error);
    void fileConflict(const SyncConflict& conflict);
    void fileChanged(const QString& path);

private:
    void uploadFileAsync(const QString& localPath);
    void downloadFileAsync(const QString& remoteId, const QString& localPath);
    bool calculateChecksum(const QString& path, QString& checksum);
    void processQueue();
    void refreshFileList();

    QString m_serverUrl;
    QString m_authToken;
    QString m_userId;
    bool m_loggedIn = false;

    SyncStatus m_status = Status_Idle;
    float m_syncProgress = 0.0f;

    bool m_autoSync = false;
    int m_syncInterval = 15;

    QNetworkAccessManager* m_networkManager = nullptr;
    QVector<CloudFile> m_remoteFiles;
    QVector<SyncConflict> m_conflicts;
};

class CloudPresetLibrary : public QObject
{
    Q_OBJECT

public:
    explicit CloudPresetLibrary(QObject* parent = nullptr);
    ~CloudPresetLibrary();

    struct Preset {
        QString id;
        QString name;
        QString description;
        QString category;
        QString author;
        QString authorId;
        int downloads = 0;
        float rating = 0.0f;
        QDateTime createdDate;
        QDateTime updatedDate;
        QString downloadUrl;
        QString thumbnailUrl;
        qint64 fileSize = 0;
        QStringList tags;
    };

    void setCloudService(CloudSyncService* service);
    void setAuthToken(const QString& token);

    QVector<Preset> searchPresets(const QString& query, const QString& category = QString());
    QVector<Preset> getFeaturedPresets();
    QVector<Preset> getPopularPresets(int count = 10);
    QVector<Preset> getMyPresets();

    bool downloadPreset(const QString& presetId, const QString& localPath);
    bool uploadPreset(const QString& localPath, const QString& name,
                     const QString& description, const QString& category);

    void ratePreset(const QString& presetId, int rating);
    void reportPreset(const QString& presetId, const QString& reason);

    QStringList getCategories() const;

signals:
    void searchResults(const QVector<Preset>& results);
    void downloadProgress(const QString& presetId, float percent);
    void downloadComplete(const QString& presetId);
    void uploadComplete(const QString& presetId);
    void error(const QString& error);
    void presetRated(const QString& presetId, int rating);
    void presetReported(const QString& presetId, const QString& reason);

private:
    CloudSyncService* m_cloudService = nullptr;
    QString m_authToken;
    QMap<QString, int> m_presetRatings;
    QMap<QString, QStringList> m_presetReports;
};

class CloudBackupSystem : public QObject
{
    Q_OBJECT

public:
    explicit CloudBackupSystem(QObject* parent = nullptr);
    ~CloudBackupSystem();

    struct BackupPoint {
        QString id;
        QString name;
        QDateTime createdDate;
        qint64 size = 0;
        QStringList includedPaths;
        int fileCount = 0;
    };

    void setCloudService(CloudSyncService* service);

    void createBackup(const QString& name, const QStringList& paths);
    void restoreBackup(const QString& backupId, const QString& targetPath);
    void deleteBackup(const QString& backupId);

    QVector<BackupPoint> getBackups() const;
    BackupPoint getBackup(const QString& backupId) const;

    void setAutoBackupEnabled(bool enabled);
    bool autoBackupEnabled() const { return m_autoBackup; }

    void setAutoBackupInterval(int hours);
    int autoBackupInterval() const { return m_autoBackupInterval; }

    void setMaxBackups(int max);
    int maxBackups() const { return m_maxBackups; }

signals:
    void backupCreated(const BackupPoint& backup);
    void backupProgress(float percent);
    void backupComplete();
    void restoreProgress(float percent);
    void restoreComplete();
    void error(const QString& error);

private:
    CloudSyncService* m_cloudService = nullptr;
    bool m_autoBackup = false;
    int m_autoBackupInterval = 24;
    int m_maxBackups = 10;
    QVector<BackupPoint> m_backups;
};

} // namespace ks
