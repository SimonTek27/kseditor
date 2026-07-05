#include "LogManager.h"
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDebug>
#include <iostream>

static QtMessageHandler s_prevQtHandler = nullptr;

static void ksMessageHandler(QtMsgType type,
                               const QMessageLogContext& ctx,
                               const QString& msg)
{
    LogManager& lm = LogManager::instance();
    if (lm.isQtMessageHandlerEnabled())
        lm.log(LogLevel::Debug, "Qt", msg);

    if (s_prevQtHandler)
        s_prevQtHandler(type, ctx, msg);
}

LogManager& LogManager::instance()
{
    static LogManager instance;
    return instance;
}

LogManager::LogManager(QObject* parent)
    : QObject(parent)
    , m_stream(&m_logFile)
{
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(logDir);

    QString logPath = logDir + "/kseditor.log";
    setLogFile(logPath);

    m_initialized = true;
}

LogManager::~LogManager()
{
    flush();
}

void LogManager::setLogFile(const QString& path)
{
    QMutexLocker locker(&m_mutex);

    if (m_logFile.isOpen()) {
        m_stream.flush();
        m_logFile.close();
    }

    m_logFile.setFileName(path);

    if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Failed to open log file:" << path;
        return;
    }

    m_stream.setDevice(&m_logFile);
}

void LogManager::setMinLevel(LogLevel level)
{
    m_minLevel = level;
}

void LogManager::setMaxFileSize(qint64 maxBytes)
{
    m_maxFileSize = maxBytes;
}

void LogManager::setMaxBackupFiles(int maxFiles)
{
    m_maxBackupFiles = maxFiles;
}

void LogManager::log(LogLevel level, const QString& category, const QString& message)
{
    if (level < m_minLevel || !isModuleEnabled(category, level)) {
        return;
    }

    QMutexLocker locker(&m_mutex);

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString levelStr = levelToString(level);
    QString formatted = QString("[%1] [%2] [%3] %4")
                            .arg(timestamp)
                            .arg(levelStr)
                            .arg(category)
                            .arg(message);

    writeToFile(formatted);

    LogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.level = level;
    entry.category = category;
    entry.message = message;
    appendEntry(entry);

    emit logMessage(level, category, message, QDateTime::currentDateTime());
    emit messageLogged(entry);
}

void LogManager::trace(const QString& category, const QString& message)
{
    log(LogLevel::Trace, category, message);
}

void LogManager::debug(const QString& category, const QString& message)
{
    log(LogLevel::Debug, category, message);
}

void LogManager::info(const QString& category, const QString& message)
{
    log(LogLevel::Info, category, message);
}

void LogManager::warning(const QString& category, const QString& message)
{
    log(LogLevel::Warning, category, message);
}

void LogManager::error(const QString& category, const QString& message)
{
    log(LogLevel::Error, category, message);
}

void LogManager::critical(const QString& category, const QString& message)
{
    log(LogLevel::Critical, category, message);
}

void LogManager::flush()
{
    QMutexLocker locker(&m_mutex);
    m_stream.flush();
}

void LogManager::installQtMessageHandler()
{
    s_prevQtHandler = qInstallMessageHandler(ksMessageHandler);
    m_qtHandlerInstalled = true;
}

void LogManager::setQtMessageHandlerEnabled(bool enabled)
{
    m_qtHandlerInstalled = enabled;
}

QVector<LogEntry> LogManager::getEntries(LogLevel minLevel) const
{
    QMutexLocker lock(&m_mutex);
    if (minLevel == LogLevel::Debug || minLevel == LogLevel::Trace)
        return m_entries;

    QVector<LogEntry> out;
    for (const auto& e : m_entries)
        if (static_cast<int>(e.level) >= static_cast<int>(minLevel))
            out << e;
    return out;
}

void LogManager::clearEntries()
{
    QMutexLocker lock(&m_mutex);
    m_entries.clear();
    emit entriesCleared();
}

void LogManager::setModuleLogLevel(const QString& module, LogLevel level)
{
    m_moduleLevels[module] = level;
}

LogLevel LogManager::getModuleLogLevel(const QString& module) const
{
    return m_moduleLevels.value(module, LogLevel::Debug);
}

bool LogManager::isModuleEnabled(const QString& module, LogLevel level) const
{
    if (!m_moduleLevels.contains(module))
        return true;
    return static_cast<int>(level) >= static_cast<int>(m_moduleLevels.value(module));
}

void LogManager::appendEntry(const LogEntry& entry)
{
    if (m_entries.size() >= m_maxEntries)
        m_entries.removeFirst();
    m_entries.append(entry);
}

void LogManager::writeToFile(const QString& message)
{
    if (m_logFile.size() >= m_maxFileSize) {
        m_stream.flush();
        rotateLogFile();
    }

    m_stream << message << "\n";
    m_stream.flush();
}

QString LogManager::levelToString(LogLevel level) const
{
    switch (level) {
        case LogLevel::Trace:    return "TRACE";
        case LogLevel::Debug:    return "DEBUG";
        case LogLevel::Info:     return "INFO ";
        case LogLevel::Warning:  return "WARN ";
        case LogLevel::Error:    return "ERROR";
        case LogLevel::Critical: return "FATAL";
        default:                 return "???? ";
    }
}

void LogManager::rotateLogFile()
{
    if (!m_logFile.isOpen()) return;

    m_logFile.close();

    QString basePath = m_logFile.fileName();

    QString oldest = QString("%1.%2").arg(basePath).arg(m_maxBackupFiles);
    QFile::remove(oldest);

    for (int i = m_maxBackupFiles - 1; i >= 1; --i) {
        QString oldName = QString("%1.%2").arg(basePath).arg(i);
        QString newName = QString("%1.%3").arg(basePath).arg(i + 1);
        if (!QFile::rename(oldName, newName)) {
            qWarning("LogManager: failed to rename %s to %s", qPrintable(oldName), qPrintable(newName));
        }
    }

    if (!QFile::rename(basePath, basePath + ".1")) {
        qWarning("LogManager: failed to rename %s to %s.1", qPrintable(basePath), qPrintable(basePath));
    }

    m_logFile.setFileName(basePath);
    if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning("LogManager: failed to reopen log file after rotation: %s", qPrintable(basePath));
        return;
    }
    m_stream.setDevice(&m_logFile);
}
