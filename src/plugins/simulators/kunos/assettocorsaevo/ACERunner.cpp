#include "ACERunner.h"
#include "ACEPaths.h"
#include <QDebug>

namespace ks {

ACERunner* ACERunner::s_instance = nullptr;

ACERunner::ACERunner(QObject* parent)
    : QObject(parent) {
    QStringList installs = ACEPaths::findAllInstallations();
    if (!installs.isEmpty()) {
        m_acePath = installs.first();
    }
    m_status = validateAcePath() ? ACERunnerStatus::Ready : ACERunnerStatus::NotFound;
}

ACERunner* ACERunner::instance() {
    if (!s_instance) {
        s_instance = new ACERunner();
    }
    return s_instance;
}

bool ACERunner::setAcePath(const QString& path) {
    m_acePath = path;
    m_status = validateAcePath() ? ACERunnerStatus::Ready : ACERunnerStatus::NotFound;
    emit statusChanged(m_status);
    return m_status == ACERunnerStatus::Ready;
}

bool ACERunner::validateAcePath() {
    if (m_acePath.isEmpty()) return false;
    QFileInfo exeInfo(m_acePath + "/AssettoCorsaEVO.exe");
    return exeInfo.exists();
}

QString ACERunner::findAceExecutable() const {
    QStringList candidates = {
        m_acePath + "/AssettoCorsaEVO.exe",
        m_acePath + "/ace.exe",
    };

    QStringList installs = ACEPaths::findAllInstallations();
    for (const QString& path : installs) {
        candidates << path + "/AssettoCorsaEVO.exe";
        candidates << path + "/ace.exe";
    }

    for (const QString& path : candidates) {
        if (QFileInfo::exists(path)) {
            return path;
        }
    }
    return QString();
}

bool ACERunner::testConnection() {
    return validateAcePath();
}

void ACERunner::onProcessStarted() {
    m_status = ACERunnerStatus::Running;
    emit statusChanged(m_status);
}

void ACERunner::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    QProcess* p = qobject_cast<QProcess*>(sender());
    if (p) {
        QByteArray stdOut = p->readAllStandardOutput();
        QByteArray stdErr = p->readAllStandardError();
        if (!stdOut.isEmpty()) qDebug() << "ACE stdout:" << stdOut;
        if (!stdErr.isEmpty()) qWarning() << "ACE stderr:" << stdErr;
        m_lastOutput = QString::fromUtf8(stdOut) + QString::fromUtf8(stdErr);
        p->deleteLater();
    }
    if (p == m_process) {
        m_process = nullptr;
    }
    m_status = (exitStatus == QProcess::NormalExit && exitCode == 0)
               ? ACERunnerStatus::Ready
               : ACERunnerStatus::Error;
    emit statusChanged(m_status);
    emit processExited(exitCode);
}

void ACERunner::onProcessError(QProcess::ProcessError error) {
    m_lastError = QString("Process error: %1").arg(static_cast<int>(error));
    m_status = ACERunnerStatus::Error;
    emit statusChanged(m_status);
    emit launchFailed(m_lastError);
}

bool ACERunner::launchAce(const QString& track, const QString& car) {
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(3000);
        m_process->deleteLater();
        m_process = nullptr;
    }

    QString exePath = findAceExecutable();
    if (exePath.isEmpty()) {
        m_lastError = "Assetto Corsa EVO executable not found. Please set correct path.";
        m_status = ACERunnerStatus::NotFound;
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
    m_process->setWorkingDirectory(m_acePath);
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_process, &QProcess::started, this, &ACERunner::onProcessStarted);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ACERunner::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &ACERunner::onProcessError);

    m_process->start();

    qDebug() << "Launching Assetto Corsa EVO:" << exePath << args;
    return true;
}

bool ACERunner::launchTimeTrial(const QString& track) {
    return launchAce(track, QString());
}

bool ACERunner::launchPractice(const QString& track) {
    return launchAce(track, QString());
}

bool ACERunner::launchModded(const QString& modPath) {
    Q_UNUSED(modPath);
    return launchAce();
}

bool ACERunner::stopAce() {
    if (!m_process) return false;
    m_process->terminate();
    if (!m_process->waitForFinished(5000)) {
        m_process->kill();
        m_process->waitForFinished(3000);
    }
    m_process->deleteLater();
    m_process = nullptr;
    m_status = ACERunnerStatus::Ready;
    emit statusChanged(m_status);
    return true;
}

QString ACERunner::getContentPath() const {
    return m_acePath + "/content";
}

QString ACERunner::getTracksPath() const {
    return m_acePath + "/content/tracks";
}

QString ACERunner::getCarsPath() const {
    return m_acePath + "/content/cars";
}

QString ACERunner::getModsPath() const {
    return ACEPaths::getModsDirectory(m_acePath);
}

bool ACERunner::isRunning() const {
    return m_process && m_process->state() == QProcess::Running;
}

} // namespace ks
