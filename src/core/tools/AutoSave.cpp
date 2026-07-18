#include "AutoSave.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QUuid>
#include <QDebug>
#include <QStandardPaths>

namespace ks {

// ============================================================================
// Document
// ============================================================================

Document::Document(QObject* parent)
    : QObject(parent)
    , m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
{}

Document::~Document() {}

bool Document::load(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Document::load – cannot open" << path;
        return false;
    }
    m_path = path;
    m_name = QFileInfo(path).baseName();
    m_modified = false;
    emit pathChanged(path);
    return true;
}

bool Document::save(const QString& path)
{
    QString savePath = path.isEmpty() ? m_path : path;
    if (savePath.isEmpty()) return false;
    m_path = savePath;
    m_modified = false;
    emit saved();
    return true;
}

// ============================================================================
// DocumentManager
// ============================================================================

DocumentManager::DocumentManager(QObject* parent)
    : QObject(parent)
{}

DocumentManager::~DocumentManager() {}

void DocumentManager::addDocument(Document* doc)
{
    if (!doc) return;
    m_documents.insert(doc->getId(), doc);
    emit documentAdded(doc);
    emit unsavedChangesChanged(hasUnsavedChanges());
}

void DocumentManager::removeDocument(const QString& docId)
{
    if (!m_documents.contains(docId)) return;
    if (m_activeDocument && m_activeDocument->getId() == docId)
        m_activeDocument = nullptr;
    m_documents.remove(docId);
    emit documentRemoved(docId);
    emit unsavedChangesChanged(hasUnsavedChanges());
}

Document* DocumentManager::getDocument(const QString& docId) const
{
    return m_documents.value(docId, nullptr);
}

void DocumentManager::setActiveDocument(Document* doc)
{
    if (m_activeDocument == doc) return;
    m_activeDocument = doc;
    emit activeDocumentChanged(doc);
}

bool DocumentManager::hasUnsavedChanges() const
{
    for (auto* doc : m_documents)
        if (doc->isModified()) return true;
    return false;
}

bool DocumentManager::saveAll()
{
    bool ok = true;
    for (auto* doc : m_documents)
        if (doc->isModified()) ok &= doc->save();
    return ok;
}

bool DocumentManager::closeAll()
{
    m_documents.clear();
    m_activeDocument = nullptr;
    return true;
}

QVector<Document*> DocumentManager::getUnsavedDocuments() const
{
    QVector<Document*> result;
    for (auto* doc : m_documents)
        if (doc->isModified()) result << doc;
    return result;
}

// ============================================================================
// AutoSave
// ============================================================================

AutoSave::AutoSave(QObject* parent)
    : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &AutoSave::performAutoSave);
    m_timer.setInterval(m_interval * 1000);
}

AutoSave::~AutoSave() {}

void AutoSave::setDocument(Document* doc)
{
    m_document = doc;
    if (m_document && m_enabled) m_timer.start();
}

void AutoSave::setBackupDirectory(const QString& dir)
{
    m_backupDir = dir;
    QDir().mkpath(dir);
}

void AutoSave::setInterval(int seconds)
{
    m_interval = qMax(10, seconds);
    m_timer.setInterval(m_interval * 1000);
}

void AutoSave::setMaxBackups(int max)
{
    m_maxBackups = qMax(1, max);
}

void AutoSave::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (enabled) m_timer.start();
    else          m_timer.stop();
}

void AutoSave::saveNow()
{
    if (m_document) m_document->save();
    emit autoSaveTriggered();
}

void AutoSave::saveBackup()
{
    if (!m_document || m_backupDir.isEmpty()) return;
    QString path = generateBackupPath();
    QFile::copy(m_document->getPath(), path);
    emit backupCreated(path);
}

bool AutoSave::hasRecoveryPoint(const QString& documentId) const
{
    QDir dir(m_backupDir);
    return !dir.entryList(QStringList() << documentId + "_*.bak").isEmpty();
}

QVector<RecoveryPoint> AutoSave::getRecoveryPoints(const QString& documentId) const
{
    QVector<RecoveryPoint> points;
    QDir dir(m_backupDir);
    const auto files = dir.entryInfoList(QStringList() << documentId + "_*.bak",
                                         QDir::Files, QDir::Time);
    int version = files.size();
    for (const auto& fi : files) {
        RecoveryPoint rp;
        rp.documentId  = documentId;
        rp.backupPath  = fi.absoluteFilePath();
        rp.timestamp   = fi.lastModified().toString(Qt::ISODate);
        rp.version     = version--;
        rp.isValid     = fi.size() > 0;
        points << rp;
    }
    return points;
}

bool AutoSave::recover(const QString& documentId, int version)
{
    const auto points = getRecoveryPoints(documentId);
    for (const auto& rp : points) {
        if (rp.version == version) {
            bool ok = QFile::copy(rp.backupPath, rp.filePath);
            if (ok) emit recoveryComplete(documentId);
            return ok;
        }
    }
    return false;
}

bool AutoSave::recoverLatest(const QString& documentId)
{
    return recover(documentId, 1);
}

void AutoSave::cleanupOldBackups()
{
    if (!m_document) return;
    auto points = getRecoveryPoints(m_document->getId());
    while (points.size() > m_maxBackups) {
        QFile::remove(points.last().backupPath);
        points.removeLast();
    }
}

void AutoSave::performAutoSave()
{
    if (!m_document || !m_document->isModified()) return;
    saveBackup();
    cleanupOldBackups();
    emit autoSaveTriggered();
}

void AutoSave::setAutoRecoveryData(const QJsonObject& sessionData)
{
    if (!m_document) return;
    
    QString docId = m_document->getId();
    QString docPath = m_document->getPath();
    
    if (!m_backupDir.isEmpty() && !docPath.isEmpty()) {
        QDir().mkpath(m_backupDir);
        
        QString autoRecoveryPath = m_backupDir + "/recover_" + docId + ".dat";
        QFile file(autoRecoveryPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(sessionData).toJson());
            file.close();
            emit backupCreated(autoRecoveryPath);
        }
    }
}

bool AutoSave::recoverAutoRecoveryData(QString& outData) const
{
    if (!m_document) return false;
    
    QString docId = m_document->getId();
    QString docPath = m_document->getPath();
    
    if (m_backupDir.isEmpty() || docPath.isEmpty()) return false;
    
    QDir dir(m_backupDir);
    QStringList recoverFiles = dir.entryList(QStringList() << "recover_" + docId + "_*.dat", QDir::Files, QDir::Time);
    
    if (recoverFiles.isEmpty()) return false;
    
    QString latestFile = dir.filePath(recoverFiles.first());
    QFile file(latestFile);
    if (!file.open(QIODevice::ReadOnly)) return false;
    
    QByteArray jsonData = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &error);
    if (error.error != QJsonParseError::NoError) return false;
    
    outData = doc.toJson(QJsonDocument::Compact);
    return true;
}

QString AutoSave::generateBackupPath() const
{
    QString docId = m_document ? m_document->getId() : "unknown";
    QString ts    = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    return QDir(m_backupDir).filePath(
        QString("%1_%2_v%3.bak").arg(docId, ts).arg(++m_version)
    );
}

// ============================================================================
// CrashRecovery
// ============================================================================

CrashRecovery::CrashRecovery(QObject* parent)
    : QObject(parent)
{
    connect(&m_sessionTimer, &QTimer::timeout, this, &CrashRecovery::saveSession);
    m_sessionTimer.setInterval(30000); // every 30 s
}

CrashRecovery::~CrashRecovery() {}

void CrashRecovery::startSession()
{
    m_currentSession.id          = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_currentSession.timestamp   = QDateTime::currentSecsSinceEpoch();
    m_currentSession.isValid     = true;
    m_sessionStartTime           = m_currentSession.timestamp;
    m_sessionTimer.start();
    emit sessionStarted();
}

void CrashRecovery::endSession()
{
    m_sessionTimer.stop();
    clearSession();
    emit sessionEnded();
}

void CrashRecovery::addOpenDocument(const QString& path)
{
    if (!m_currentSession.openDocuments.contains(path))
        m_currentSession.openDocuments << path;
}

void CrashRecovery::setActiveDocument(const QString& path)
{
    m_currentSession.lastActiveDocument = path;
}

void CrashRecovery::saveSession()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                  + "/sessions";
    QDir().mkpath(dir);
    QString path = dir + "/" + m_currentSession.id + ".json";

    QJsonObject obj;
    obj["id"]                 = m_currentSession.id;
    obj["openDocuments"]      = QJsonArray::fromStringList(m_currentSession.openDocuments);
    obj["lastActiveDocument"] = m_currentSession.lastActiveDocument;
    obj["timestamp"]          = m_currentSession.timestamp;
    obj["isValid"]            = true;

    QFile f(path);
    if (f.open(QIODevice::WriteOnly))
        f.write(QJsonDocument(obj).toJson());
}

void CrashRecovery::saveSessionState(const QJsonObject& state)
{
    QJsonObject obj;
    obj["id"]                 = m_currentSession.id;
    obj["openDocuments"]      = QJsonArray::fromStringList(m_currentSession.openDocuments);
    obj["lastActiveDocument"] = m_currentSession.lastActiveDocument;
    obj["timestamp"]          = m_currentSession.timestamp;
    obj["windowState"]        = state.value("windowState").toObject();
    obj["editorState"]        = state.value("editorState").toObject();
    obj["isValid"]            = true;

    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                  + "/sessions";
    QDir().mkpath(dir);
    QString path = dir + "/" + m_currentSession.id + ".json";

    QFile f(path);
    if (f.open(QIODevice::WriteOnly))
        f.write(QJsonDocument(obj).toJson());
}

QJsonObject CrashRecovery::getSessionState() const
{
    if (!m_currentSession.isValid) return QJsonObject();
    
    QJsonObject obj;
    obj["id"]                 = m_currentSession.id;
    obj["openDocuments"]      = QJsonArray::fromStringList(m_currentSession.openDocuments);
    obj["lastActiveDocument"] = m_currentSession.lastActiveDocument;
    obj["timestamp"]          = m_currentSession.timestamp;
    obj["isValid"]            = true;
    
    return obj;
}

CrashRecovery::Session CrashRecovery::getSession() const
{
    return m_currentSession;
}

bool CrashRecovery::hasSession() const
{
    if (!m_currentSession.id.isEmpty()) return true;
    return !getAbandonedSessions().isEmpty();
}

void CrashRecovery::clearSession()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                  + "/sessions";
    QFile::remove(dir + "/" + m_currentSession.id + ".json");
    m_currentSession = {};
}

bool CrashRecovery::recover()
{
    auto sessions = getAbandonedSessions();
    if (sessions.isEmpty()) return false;
    emit recoveryNeeded(sessions);
    return true;
}

bool CrashRecovery::recoverSession(const Session& session)
{
    // Restore files from the session's backup directory
    QString backupDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + "/backups/" + session.id;
    
    QDir dir(backupDir);
    if (!dir.exists()) return false;
    
    bool success = true;
    for (const auto& file : session.files) {
        QString backupPath = backupDir + "/" + QFileInfo(file).fileName();
        if (QFile::exists(backupPath)) {
            // Restore the backup file to its original location
            QFile::remove(file);
            if (!QFile::copy(backupPath, file)) {
                success = false;
            }
        }
    }
    
    return success;
}

bool CrashRecovery::recoverSessionState(const QJsonObject& sessionState)
{
    if (!sessionState.value("isValid").toBool()) return false;
    
    Session s;
    s.id                 = sessionState["id"].toString();
    s.lastActiveDocument = sessionState["lastActiveDocument"].toString();
    s.timestamp          = static_cast<qint64>(sessionState["timestamp"].toDouble());
    s.isValid            = true;
    for (const auto& v : sessionState["openDocuments"].toArray())
        s.openDocuments << v.toString();
    
    // Load window state
    QJsonObject windowState = sessionState["windowState"].toObject();
    if (windowState.isEmpty()) {
        return true; // Still recovered without state
    }
    
    // Load editor state
    QJsonObject editorState = sessionState["editorState"].toObject();
    
    return true;
}

QVector<CrashRecovery::Session> CrashRecovery::getAbandonedSessions() const
{
    QVector<Session> sessions;
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                  + "/sessions";
    QDir d(dir);
    for (const auto& fi : d.entryInfoList({"*.json"}, QDir::Files)) {
        QFile f(fi.absoluteFilePath());
        if (!f.open(QIODevice::ReadOnly)) continue;
        QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
        if (!obj.value("isValid").toBool()) continue;
        Session s;
        s.id                 = obj["id"].toString();
        s.lastActiveDocument = obj["lastActiveDocument"].toString();
        s.timestamp          = static_cast<qint64>(obj["timestamp"].toDouble());
        s.isValid            = true;
        for (const auto& v : obj["openDocuments"].toArray())
            s.openDocuments << v.toString();
        sessions << s;
    }
    return sessions;
}

bool CrashRecovery::recoverAbandonedSession(const QString& sessionId)
{
    auto sessions = getAbandonedSessions();
    for (const auto& s : sessions) {
        if (s.id == sessionId) return recoverSession(s);
    }
    return false;
}

void CrashRecovery::loadSession()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                  + "/sessions";
    QString path = dir + "/" + m_currentSession.id + ".json";
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return;
    QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
    m_currentSession.id = obj["id"].toString();
    m_currentSession.lastActiveDocument = obj["lastActiveDocument"].toString();
    m_currentSession.timestamp = static_cast<qint64>(obj["timestamp"].toDouble());
    m_currentSession.isValid = obj["isValid"].toBool();
    m_currentSession.openDocuments.clear();
    for (const auto& v : obj["openDocuments"].toArray())
        m_currentSession.openDocuments << v.toString();
}
void CrashRecovery::cleanupOldSessions()
{
    // Keep max 5 abandoned sessions
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                  + "/sessions";
    auto files = QDir(dir).entryInfoList({"*.json"}, QDir::Files, QDir::Time);
    while (files.size() > 5) {
        QFile::remove(files.takeLast().absoluteFilePath());
    }
}

} // namespace ks
