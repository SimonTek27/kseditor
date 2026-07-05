#include "CloudSyncManager.h"
#include "../sys/LogManager.h"
#include <QJsonDocument>
#include <QFile>
#include <QFileInfo>
#include <QDirIterator>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <algorithm>

namespace ks {

CloudSyncManager::CloudSyncManager(QObject* parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
    m_autoSyncTimer = new QTimer(this);
    m_autoSyncTimer->setSingleShot(false);
    connect(m_autoSyncTimer, &QTimer::timeout, this, &CloudSyncManager::onAutoSyncTimer);
}

bool CloudSyncManager::configure(const CloudSyncConfig& config)
{
    m_config = config;
    m_authenticated = !config.accessToken.isEmpty();
    emit authStatusChanged(m_authenticated);

    if (config.autoSync && m_authenticated && config.syncIntervalMinutes > 0) {
        m_autoSyncTimer->start(config.syncIntervalMinutes * 60 * 1000);
    } else {
        m_autoSyncTimer->stop();
    }

    LOG_INFO("CloudSyncManager", QString("Configured provider: %1, autoSync: %2")
        .arg(static_cast<int>(config.provider)).arg(config.autoSync));
    return true;
}

void CloudSyncManager::syncNow()
{
    if (m_syncing || m_config.provider == CloudProviderType::None) return;
    m_syncing = true;
    emit syncStarted();

    // Run sync asynchronously via timer to avoid blocking
    QTimer::singleShot(0, this, &CloudSyncManager::performSync);
}

void CloudSyncManager::stopSync()
{
    if (m_syncing) {
        m_syncing = false;
        m_autoSyncTimer->stop();
        LOG_INFO("CloudSyncManager", "Sync stopped by user");
        emit syncCompleted(false, "Sync cancelled by user");
    }
}

void CloudSyncManager::performSync()
{
    if (m_config.localCachePath.isEmpty()) {
        m_syncing = false;
        emit syncCompleted(false, "No local cache path configured");
        return;
    }

    QDir localDir(m_config.localCachePath);
    if (!localDir.exists()) {
        localDir.mkpath(".");
    }

    int totalSteps = 3; // scan local, scan remote, resolve
    int currentStep = 0;

    // Step 1: Scan local changes
    updateProgress(0, "Scanning local files...");
    scanLocalChanges();
    currentStep++;
    updateProgress(currentStep * 100 / totalSteps, "Local scan complete");

    // Step 2: Scan remote changes
    scanRemoteChanges();
    currentStep++;
    updateProgress(currentStep * 100 / totalSteps, "Remote scan complete");

    // Step 3: Resolve differences
    resolveDifferences();
    currentStep++;
    updateProgress(100, "Sync complete");

    m_syncing = false;
    int uploaded = 0, downloaded = 0, conflicts = 0;
    for (const auto& f : m_files) {
        if (f.status == CloudSyncFile::NewLocal) uploaded++;
        else if (f.status == CloudSyncFile::NewRemote) downloaded++;
        else if (f.status == CloudSyncFile::Conflict) conflicts++;
    }

    QString msg = QString("Synced: %1 uploaded, %2 downloaded, %3 conflicts")
        .arg(uploaded).arg(downloaded).arg(conflicts);
    emit syncCompleted(true, msg);
    LOG_INFO("CloudSyncManager", msg);
}

void CloudSyncManager::scanLocalChanges()
{
    m_localChecksums.clear();
    m_localTimestamps.clear();

    QDir localDir(m_config.localCachePath);
    QDirIterator it(localDir, QDirIterator::Subdirectories);

    while (it.hasNext()) {
        QString filePath = it.next();
        QFileInfo fi(filePath);
        if (!fi.isFile()) continue;

        QString relativePath = localDir.relativeFilePath(filePath);
        if (shouldExclude(relativePath)) continue;

        m_localChecksums[relativePath] = computeChecksum(filePath);
        m_localTimestamps[relativePath] = fi.lastModified();
    }
}

void CloudSyncManager::scanRemoteChanges()
{
    m_remoteChecksums.clear();
    m_remoteTimestamps.clear();
    m_files.clear();

    // For local sync, remote is the remotePath directory
    if (m_config.provider == CloudProviderType::Local && !m_config.remotePath.isEmpty()) {
        QDir remoteDir(m_config.remotePath);
        if (remoteDir.exists()) {
            QDirIterator it(remoteDir, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                QString filePath = it.next();
                QFileInfo fi(filePath);
                if (!fi.isFile()) continue;

                QString relativePath = remoteDir.relativeFilePath(filePath);
                if (shouldExclude(relativePath)) continue;

                m_remoteChecksums[relativePath] = computeChecksum(filePath);
                m_remoteTimestamps[relativePath] = fi.lastModified();
            }
        }
    }

    // Compare local vs remote
    QSet<QString> allPaths;
    for (auto it = m_localChecksums.begin(); it != m_localChecksums.end(); ++it)
        allPaths.insert(it.key());
    for (auto it = m_remoteChecksums.begin(); it != m_remoteChecksums.end(); ++it)
        allPaths.insert(it.key());

    for (const QString& path : allPaths) {
        CloudSyncFile file;
        file.remotePath = path;
        file.localPath = QDir(m_config.localCachePath).filePath(path);

        bool hasLocal = m_localChecksums.contains(path);
        bool hasRemote = m_remoteChecksums.contains(path);

        if (hasLocal && !hasRemote) {
            file.status = CloudSyncFile::NewLocal;
            file.checksum = m_localChecksums[path];
            file.lastModified = m_localTimestamps[path];
        } else if (!hasLocal && hasRemote) {
            file.status = CloudSyncFile::NewRemote;
            file.checksum = m_remoteChecksums[path];
            file.lastModified = m_remoteTimestamps[path];
        } else if (m_localChecksums[path] != m_remoteChecksums[path]) {
            file.status = CloudSyncFile::Conflict;
            file.checksum = m_localChecksums[path];
            file.lastModified = m_localTimestamps[path];
            if (m_remoteTimestamps[path] > m_localTimestamps[path]) {
                file.checksum = m_remoteChecksums[path];
                file.lastModified = m_remoteTimestamps[path];
            }
        } else {
            file.status = CloudSyncFile::Synced;
            file.checksum = m_localChecksums[path];
            file.lastModified = m_localTimestamps[path];
        }

        m_files.append(file);
    }
}

void CloudSyncManager::resolveDifferences()
{
    int total = m_files.size();
    int processed = 0;

    for (auto& file : m_files) {
        if (!m_syncing) break;

        processed++;
        updateProgress(processed * 100 / total, file.remotePath);

        if (file.status == CloudSyncFile::NewLocal) {
            if (uploadFile(file.localPath, file.remotePath)) {
                file.status = CloudSyncFile::Synced;
                logSync(file.remotePath, "upload", true);
                emit fileSynced(file.remotePath, true);
            } else {
                logSync(file.remotePath, "upload", false, "Upload failed");
                emit syncError(file.remotePath, "Upload failed");
            }
        } else if (file.status == CloudSyncFile::NewRemote) {
            if (downloadFile(file.remotePath, file.localPath)) {
                file.status = CloudSyncFile::Synced;
                logSync(file.remotePath, "download", true);
                emit fileSynced(file.remotePath, false);
            } else {
                logSync(file.remotePath, "download", false, "Download failed");
                emit syncError(file.remotePath, "Download failed");
            }
        } else if (file.status == CloudSyncFile::Conflict) {
            emit conflictDetected(file.remotePath);
            logSync(file.remotePath, "conflict", false, "Conflict detected");
        }
    }
}

bool CloudSyncManager::uploadFile(const QString& localPath, const QString& remotePath)
{
    QFile file(localPath);
    if (!file.exists()) return false;

    if (m_config.provider == CloudProviderType::Local && !m_config.remotePath.isEmpty()) {
        QString destPath = QDir(m_config.remotePath).filePath(remotePath);
        QFileInfo fi(destPath);
        QDir().mkpath(fi.absolutePath());
        return QFile::copy(localPath, destPath);
    }

    // Google Drive / Dropbox / OneDrive would use QNetworkAccessManager here
    // with their respective REST APIs
    logSync(remotePath, "upload", false, "Cloud provider API not yet implemented");
    return false;
}

bool CloudSyncManager::downloadFile(const QString& remotePath, const QString& localPath)
{
    QFileInfo fi(localPath);
    QDir().mkpath(fi.absolutePath());

    if (m_config.provider == CloudProviderType::Local && !m_config.remotePath.isEmpty()) {
        QString sourcePath = QDir(m_config.remotePath).filePath(remotePath);
        if (!QFile::exists(sourcePath)) return false;
        // Remove local target if it exists so copy doesn't fail
        if (QFile::exists(localPath))
            QFile::remove(localPath);
        return QFile::copy(sourcePath, localPath);
    }

    // Google Drive / Dropbox / OneDrive would use QNetworkAccessManager here
    // with their respective REST APIs
    logSync(remotePath, "download", false, "Cloud provider API not yet implemented");
    return false;
}

bool CloudSyncManager::deleteRemote(const QString& remotePath)
{
    if (m_config.provider == CloudProviderType::Local && !m_config.remotePath.isEmpty()) {
        QString targetPath = QDir(m_config.remotePath).filePath(remotePath);
        return QFile::remove(targetPath);
    }
    logSync(remotePath, "delete", false, "Cloud provider API not yet implemented");
    return false;
}

QString CloudSyncManager::computeChecksum(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return QString();

    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file)) return QString();
    return hash.result().toHex();
}

bool CloudSyncManager::shouldExclude(const QString& path) const
{
    for (const auto& excluded : m_excludedPaths) {
        if (path.startsWith(excluded)) return true;
    }
    return false;
}

QList<CloudSyncFile> CloudSyncManager::getSyncStatus() const
{
    return m_files;
}

QList<CloudSyncFile> CloudSyncManager::getConflicts() const
{
    QList<CloudSyncFile> conflicts;
    for (const auto& f : m_files) {
        if (f.status == CloudSyncFile::Conflict) {
            conflicts.append(f);
        }
    }
    return conflicts;
}

int CloudSyncManager::getPendingCount() const
{
    int count = 0;
    for (const auto& f : m_files) {
        if (f.status != CloudSyncFile::Synced && f.status != CloudSyncFile::Unknown) {
            count++;
        }
    }
    return count;
}

bool CloudSyncManager::resolveConflict(const QString& remotePath, ConflictResolution resolution)
{
    for (auto& file : m_files) {
        if (file.remotePath == remotePath && file.status == CloudSyncFile::Conflict) {
            switch (resolution) {
                case KeepLocal:
                    if (uploadFile(file.localPath, file.remotePath)) {
                        file.status = CloudSyncFile::Synced;
                        logSync(remotePath, "upload", true, "Kept local version");
                        return true;
                    }
                    break;
                case KeepRemote:
                    if (downloadFile(file.remotePath, file.localPath)) {
                        file.status = CloudSyncFile::Synced;
                        logSync(remotePath, "download", true, "Kept remote version");
                        return true;
                    }
                    break;
                case Skip:
                    file.status = CloudSyncFile::Synced;
                    logSync(remotePath, "skip", true, "Skipped conflict");
                    return true;
                case UseNewest: {
                    QFileInfo localFi(file.localPath);
                    if (localFi.lastModified() > file.lastModified) {
                        if (uploadFile(file.localPath, file.remotePath)) {
                            file.status = CloudSyncFile::Synced;
                            logSync(remotePath, "upload", true, "Used newer local version");
                            return true;
                        }
                    } else {
                        if (downloadFile(file.remotePath, file.localPath)) {
                            file.status = CloudSyncFile::Synced;
                            logSync(remotePath, "download", true, "Used newer remote version");
                            return true;
                        }
                    }
                    break;
                }
            }
        }
    }
    return false;
}

void CloudSyncManager::setExcludedPaths(const QStringList& paths)
{
    m_excludedPaths = paths;
}

void CloudSyncManager::onAutoSyncTimer()
{
    if (m_authenticated && !m_syncing) {
        syncNow();
    }
}

void CloudSyncManager::updateProgress(int percent, const QString& file)
{
    emit syncProgress(qBound(0, percent, 100), file);
}

void CloudSyncManager::logSync(const QString& path, const QString& action, bool success, const QString& msg)
{
    SyncEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.path = path;
    entry.action = action;
    entry.success = success;
    entry.message = msg;
    m_syncHistory.append(entry);

    while (m_syncHistory.size() > MAX_HISTORY) {
        m_syncHistory.removeFirst();
    }
}

QJsonObject CloudSyncManager::serialize() const
{
    QJsonObject data;
    data["provider"] = static_cast<int>(m_config.provider);
    data["remotePath"] = m_config.remotePath;
    data["localCachePath"] = m_config.localCachePath;
    data["accessToken"] = m_config.accessToken;
    data["refreshToken"] = m_config.refreshToken;
    data["autoSync"] = m_config.autoSync;
    data["syncIntervalMinutes"] = m_config.syncIntervalMinutes;
    data["syncOnStartup"] = m_config.syncOnStartup;
    data["excludedPaths"] = QJsonArray::fromStringList(m_excludedPaths);

    QJsonArray historyArr;
    for (const auto& entry : m_syncHistory) {
        QJsonObject hObj;
        hObj["timestamp"] = entry.timestamp.toString(Qt::ISODate);
        hObj["path"] = entry.path;
        hObj["action"] = entry.action;
        hObj["success"] = entry.success;
        hObj["message"] = entry.message;
        historyArr.append(hObj);
    }
    data["syncHistory"] = historyArr;
    return data;
}

void CloudSyncManager::deserialize(const QJsonObject& data)
{
    m_config.provider = static_cast<CloudProviderType>(data["provider"].toInt(0));
    m_config.remotePath = data["remotePath"].toString();
    m_config.localCachePath = data["localCachePath"].toString();
    m_config.accessToken = data["accessToken"].toString();
    m_config.refreshToken = data["refreshToken"].toString();
    m_config.autoSync = data["autoSync"].toBool(false);
    m_config.syncIntervalMinutes = data["syncIntervalMinutes"].toInt(30);
    m_config.syncOnStartup = data["syncOnStartup"].toBool(false);
    m_authenticated = !m_config.accessToken.isEmpty();

    m_excludedPaths.clear();
    for (const auto& v : data["excludedPaths"].toArray()) {
        m_excludedPaths.append(v.toString());
    }

    m_syncHistory.clear();
    for (const auto& v : data["syncHistory"].toArray()) {
        QJsonObject hObj = v.toObject();
        SyncEntry entry;
        entry.timestamp = QDateTime::fromString(hObj["timestamp"].toString(), Qt::ISODate);
        entry.path = hObj["path"].toString();
        entry.action = hObj["action"].toString();
        entry.success = hObj["success"].toBool();
        entry.message = hObj["message"].toString();
        m_syncHistory.append(entry);
    }
}

} // namespace ks
