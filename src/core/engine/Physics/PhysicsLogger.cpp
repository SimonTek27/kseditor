#include "PhysicsLogger.h"
#include <iostream>
#include <QDebug>
#include <QDir>

namespace ks {
namespace physics {

// ============================================================================
// PhysicsLogger Implementation
// ============================================================================

PhysicsLogger& PhysicsLogger::instance() {
    static PhysicsLogger instance;
    return instance;
}

PhysicsLogger::PhysicsLogger() 
    : QObject(nullptr)
{
    // Set default log file
    QString logPath = QDir::current().filePath("physics_log.txt");
    setLogFile(logPath);
    
    qDebug() << "PhysicsLogger initialized, log file:" << logPath;
}

PhysicsLogger::~PhysicsLogger() {
    flush();
    if (m_fileOpen) {
        m_logFile.close();
    }
}

void PhysicsLogger::setLogFile(const QString& path) {
    QMutexLocker locker(&m_mutex);
    
    if (m_fileOpen) {
        m_logFile.close();
        m_fileOpen = false;
    }
    
    m_logFile.setFileName(path);
    if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        m_stream.setDevice(&m_logFile);
        m_fileOpen = true;
        log(Level::Info, "Logger", "Log file opened: %1", path);
    } else {
        qWarning() << "Failed to open log file:" << path;
    }
}

void PhysicsLogger::setLevel(Level level) {
    QMutexLocker locker(&m_mutex);
    m_level = level;
}

void PhysicsLogger::log(Level level, const QString& message) {
    log(level, "General", message);
}

void PhysicsLogger::log(Level level, const QString& category, const QString& message) {
    if (!m_enabled || level < m_level) return;
    
    QMutexLocker locker(&m_mutex);
    
    QString timestamp = currentTimestamp();
    QString levelStr = levelToString(level);
    QString color = levelColor(level);
    
    // Format: [timestamp] [LEVEL] [category] message
    QString formatted = QString("[%1] [%2] [%3] %4")
                        .arg(timestamp)
                        .arg(levelStr)
                        .arg(category)
                        .arg(message);
    
    // Output to console with color
    QString colored = QString("%1%2%3")
                      .arg(color)
                      .arg(formatted)
                      .arg("\033[0m");
    
    // Use qDebug for proper output
    switch (level) {
        case Level::Trace:
        case Level::Debug:
            qDebug().noquote() << colored;
            break;
        case Level::Info:
            qInfo().noquote() << colored;
            break;
        case Level::Warning:
            qWarning().noquote() << colored;
            break;
        case Level::Error:
            qCritical().noquote() << colored;
            break;
        case Level::Critical:
            qFatal("%s", qPrintable(formatted));
            break;
    }
    
    // Write to file
    writeToFile(formatted);
    
    // Emit signal
    emit logMessage(level, message, timestamp);
}

void PhysicsLogger::flush() {
    QMutexLocker locker(&m_mutex);
    if (m_fileOpen) {
        m_stream.flush();
    }
}

QString PhysicsLogger::levelToString(Level level) const {
    switch (level) {
        case Level::Trace:   return "TRACE";
        case Level::Debug:   return "DEBUG";
        case Level::Info:    return "INFO";
        case Level::Warning: return "WARN";
        case Level::Error:   return "ERROR";
        case Level::Critical:return "CRITICAL";
        default:             return "UNKNOWN";
    }
}

QString PhysicsLogger::levelColor(Level level) const {
    switch (level) {
        case Level::Trace:
        case Level::Debug:   return "\033[36m"; // Cyan
        case Level::Info:    return "\033[32m"; // Green
        case Level::Warning: return "\033[33m"; // Yellow
        case Level::Error:   return "\033[31m"; // Red
        case Level::Critical:return "\033[41m\033[37m"; // Red background, white text
        default:             return "\033[0m";
    }
}

QString PhysicsLogger::currentTimestamp() const {
    return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
}

void PhysicsLogger::writeToFile(const QString& formatted) {
    if (m_fileOpen) {
        m_stream << formatted << "\n";
        m_stream.flush();
    }
}

} // namespace physics
} // namespace ks