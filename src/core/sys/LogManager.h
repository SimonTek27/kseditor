#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDateTime>
#include <QDir>
#include <QVector>
#include <QMap>
#include <QMessageLogContext>

enum class LogLevel {
    Trace = -1,
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
    Critical = 4
};

Q_DECLARE_METATYPE(LogLevel)

struct LogEntry {
    QDateTime timestamp;
    LogLevel level;
    QString category;
    QString message;
};

class LogManager : public QObject
{
    Q_OBJECT

public:
    static LogManager& instance();

    void setLogFile(const QString& path);
    void setMinLevel(LogLevel level);
    void setMaxFileSize(qint64 maxBytes);
    void setMaxBackupFiles(int maxFiles);

    void log(LogLevel level, const QString& category, const QString& message);

    void trace(const QString& category, const QString& message);
    void debug(const QString& category, const QString& message);
    void info(const QString& category, const QString& message);
    void warning(const QString& category, const QString& message);
    void error(const QString& category, const QString& message);
    void critical(const QString& category, const QString& message);

    void flush();

    void installQtMessageHandler();
    void setQtMessageHandlerEnabled(bool enabled);
    bool isQtMessageHandlerEnabled() const { return m_qtHandlerInstalled; }

    QVector<LogEntry> getEntries(LogLevel minLevel = LogLevel::Debug) const;
    void clearEntries();
    void setMaxEntries(int max) { m_maxEntries = max; }

    void setModuleLogLevel(const QString& module, LogLevel level);
    LogLevel getModuleLogLevel(const QString& module) const;
    bool isModuleEnabled(const QString& module, LogLevel level) const;

    QString getLogFilePath() const { return m_logFile.fileName(); }

signals:
    void logMessage(LogLevel level, const QString& category, const QString& message, const QDateTime& timestamp);
    void messageLogged(const LogEntry& entry);
    void entriesCleared();

private:
    explicit LogManager(QObject* parent = nullptr);
    ~LogManager();
    Q_DISABLE_COPY(LogManager)

    void writeToFile(const QString& message);
    QString levelToString(LogLevel level) const;
    void rotateLogFile();
    void appendEntry(const LogEntry& entry);

    QFile m_logFile;
    QTextStream m_stream;
    mutable QMutex m_mutex;
    LogLevel m_minLevel = LogLevel::Info;
    qint64 m_maxFileSize = 10 * 1024 * 1024;
    int m_maxBackupFiles = 5;
    bool m_initialized = false;
    bool m_qtHandlerInstalled = false;

    QVector<LogEntry> m_entries;
    int m_maxEntries = 5000;

    QMap<QString, LogLevel> m_moduleLevels;
};

#ifndef LOG_DEBUG
#define LOG_TRACE(category, message) LogManager::instance().trace(category, message)
#define LOG_DEBUG(category, message) LogManager::instance().debug(category, message)
#define LOG_INFO(category, message) LogManager::instance().info(category, message)
#define LOG_WARNING(category, message) LogManager::instance().warning(category, message)
#define LOG_ERROR(category, message) LogManager::instance().error(category, message)
#define LOG_CRITICAL(category, message) LogManager::instance().critical(category, message)
#endif
