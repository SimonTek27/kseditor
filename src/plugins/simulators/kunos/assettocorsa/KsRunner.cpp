#include "KsRunner.h"
#include <QDebug>
#include <QMessageBox>

namespace ks {

KsRunner* KsRunner::s_instance = nullptr;

KsRunner::KsRunner(QObject* parent)
    : QObject(parent)
{
    m_ksPath = "C:/Program Files/Assetto Corsa";
    m_status = validateKsPath() ? KsRunnerStatus::Ready : KsRunnerStatus::NotFound;
}

KsRunner* KsRunner::instance() {
    if (!s_instance) {
        s_instance = new KsRunner();
    }
    return s_instance;
}

bool KsRunner::setKsPath(const QString& path) {
    m_ksPath = path;
    m_status = validateKsPath() ? KsRunnerStatus::Ready : KsRunnerStatus::NotFound;
    emit statusChanged(m_status);
    return m_status == KsRunnerStatus::Ready;
}

bool KsRunner::validateKsPath() {
    QFileInfo exeInfo(m_ksPath + "/assetto_corsa.exe");
    return exeInfo.exists();
}

QString KsRunner::findKsExecutable() {
    QStringList possiblePaths = {
        m_ksPath + "/assetto_corsa.exe",
        m_ksPath + "/AC.exe",
        "C:/Program Files (x86)/Steam/steamapps/core/assettocorsa/assetto_corsa.exe",
        "C:/Program Files/Steam/steamapps/core/assettocorsa/assetto_corsa.exe"
    };
    
    for (const QString& path : possiblePaths) {
        if (QFileInfo::exists(path)) {
            return path;
        }
    }
    return QString();
}

bool KsRunner::testConnection() {
    return validateKsPath();
}

bool KsRunner::launchKs(const QString& track, const QString& car) {
    QString exePath = findKsExecutable();
    if (exePath.isEmpty()) {
        m_lastError = "Assetto Corsa executable not found. Please set correct path.";
        m_status = KsRunnerStatus::NotFound;
        emit launchFailed(m_lastError);
        return false;
    }
    
    QStringList args;
    
    if (!track.isEmpty()) {
        args << "-track" << track;
    }
    if (!car.isEmpty()) {
        args << "-car" << car;
    }
    
    m_process = new QProcess(this);
    m_process->setProgram(exePath);
    m_process->setArguments(args);
    m_process->setWorkingDirectory(m_ksPath);
    
    m_status = KsRunnerStatus::Running;
    emit statusChanged(m_status);
    
    m_process->start();
    
    emit launched();
    qDebug() << "Launching Assetto Corsa:" << exePath << args;
    
    return true;
}

bool KsRunner::launchTimeTrial(const QString& track) {
    return launchKs(track, QString());
}

bool KsRunner::launchPractice(const QString& track) {
    QStringList args;
    args << "-practice";
    if (!track.isEmpty()) {
        args << "-track" << track;
    }
    
    QString exePath = findKsExecutable();
    if (exePath.isEmpty()) {
        emit launchFailed("KS not found");
        return false;
    }
    
    QProcess* proc = new QProcess(this);
    proc->setProgram(exePath);
    proc->setArguments(args);
    proc->setWorkingDirectory(m_ksPath);
    proc->start();
    
    return true;
}

QString KsRunner::getContentPath() const {
    return m_ksPath + "/content";
}

QString KsRunner::getTracksPath() const {
    return m_ksPath + "/content/tracks";
}

QString KsRunner::getCarsPath() const {
    return m_ksPath + "/content/cars";
}

}