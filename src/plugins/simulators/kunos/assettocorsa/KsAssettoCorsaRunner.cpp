#include "KsAssettoCorsaRunner.h"
#include "KsContentPaths.h"
#include <QDebug>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>

namespace ks {

KsAssettoCorsaRunner* KsAssettoCorsaRunner::s_instance = nullptr;

KsAssettoCorsaRunner::KsAssettoCorsaRunner(QObject* parent)
    : QObject(parent)
{
    QStringList installs = KsPaths::findAllInstallations();
    if (!installs.isEmpty()) {
        m_ksPath = installs.first();
    } else {
        m_ksPath = "C:/Program Files (x86)/Steam/steamapps/common/assettocorsa";
    }
    m_status = validateKsPath() ? KsAssettoCorsaRunnerStatus::Ready : KsAssettoCorsaRunnerStatus::NotFound;
}

KsAssettoCorsaRunner* KsAssettoCorsaRunner::instance() {
    if (!s_instance) {
        s_instance = new KsAssettoCorsaRunner();
    }
    return s_instance;
}

bool KsAssettoCorsaRunner::setKsPath(const QString& path) {
    m_ksPath = path;
    m_status = validateKsPath() ? KsAssettoCorsaRunnerStatus::Ready : KsAssettoCorsaRunnerStatus::NotFound;
    emit statusChanged(m_status);
    return m_status == KsAssettoCorsaRunnerStatus::Ready;
}

bool KsAssettoCorsaRunner::validateKsPath() {
    if (m_ksPath.isEmpty()) return false;
    QFileInfo exeInfo(m_ksPath + "/assetto_corsa.exe");
    if (exeInfo.exists()) return true;
    QFileInfo acsExe(m_ksPath + "/acs.exe");
    if (acsExe.exists()) return true;
    // Also check content/ directory existence
    QDir contentDir(m_ksPath + "/content");
    return contentDir.exists();
}

QString KsAssettoCorsaRunner::findKsExecutable() const {
    QStringList candidates = {
        m_ksPath + "/assetto_corsa.exe",
        m_ksPath + "/acs.exe",
        m_ksPath + "/AC.exe"
    };

    // Add Steam install paths
    QStringList installs = KsPaths::findAllInstallations();
    for (const QString& path : installs) {
        candidates << path + "/assetto_corsa.exe";
        candidates << path + "/acs.exe";
    }

    // Add hardcoded fallbacks
    candidates << "C:/Program Files (x86)/Steam/steamapps/common/assettocorsa/assetto_corsa.exe";
    candidates << "C:/Program Files/Steam/steamapps/common/assettocorsa/assetto_corsa.exe";

    for (const QString& path : candidates) {
        if (QFileInfo::exists(path)) {
            return path;
        }
    }
    return QString();
}

QString KsAssettoCorsaRunner::findCspExecutable() const {
    // CSP (Custom Shaders Patch) lives alongside the AC executable
    QString exeDir = QFileInfo(findKsExecutable()).absolutePath();
    QStringList cspCandidates = {
        exeDir + "/d3d11.dll",
        exeDir + "/dxgi.dll",
        exeDir + "/csp/csp.exe"
    };
    for (const QString& path : cspCandidates) {
        if (QFileInfo::exists(path)) {
            return path;
        }
    }
    return QString();
}

bool KsAssettoCorsaRunner::testConnection() {
    return validateKsPath();
}

void KsAssettoCorsaRunner::onProcessStarted() {
    m_status = KsAssettoCorsaRunnerStatus::Running;
    emit statusChanged(m_status);
}

void KsAssettoCorsaRunner::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    QProcess* p = qobject_cast<QProcess*>(sender());
    if (p) {
        QByteArray stdOut = p->readAllStandardOutput();
        QByteArray stdErr = p->readAllStandardError();
        if (!stdOut.isEmpty()) qDebug() << "AC stdout:" << stdOut;
        if (!stdErr.isEmpty()) qWarning() << "AC stderr:" << stdErr;
        m_lastOutput = QString::fromUtf8(stdOut) + QString::fromUtf8(stdErr);
        p->deleteLater();
    }
    if (p == m_process) {
        m_process = nullptr;
    }
    m_status = (exitStatus == QProcess::NormalExit && exitCode == 0)
               ? KsAssettoCorsaRunnerStatus::Ready
               : KsAssettoCorsaRunnerStatus::Error;
    emit statusChanged(m_status);
    emit processExited(exitCode);
}

void KsAssettoCorsaRunner::onProcessError(QProcess::ProcessError error) {
    m_lastError = QString("Process error: %1").arg(static_cast<int>(error));
    m_status = KsAssettoCorsaRunnerStatus::Error;
    emit statusChanged(m_status);
    emit launchFailed(m_lastError);
}

bool KsAssettoCorsaRunner::launchKs(const QString& track, const QString& car) {
    // Kill any existing process
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(3000);
        m_process->deleteLater();
        m_process = nullptr;
    }

    QString exePath = findKsExecutable();
    if (exePath.isEmpty()) {
        m_lastError = "Assetto Corsa executable not found. Please set correct path.";
        m_status = KsAssettoCorsaRunnerStatus::NotFound;
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
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_process, &QProcess::started, this, &KsAssettoCorsaRunner::onProcessStarted);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &KsAssettoCorsaRunner::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &KsAssettoCorsaRunner::onProcessError);

    m_process->start();

    qDebug() << "Launching Assetto Corsa:" << exePath << args;
    return true;
}

bool KsAssettoCorsaRunner::launchTimeTrial(const QString& track) {
    return launchKs(track, QString());
}

bool KsAssettoCorsaRunner::launchPractice(const QString& track) {
    QString exePath = findKsExecutable();
    if (exePath.isEmpty()) {
        m_lastError = "KS not found";
        emit launchFailed(m_lastError);
        return false;
    }

    // Kill any existing process
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(3000);
        m_process->deleteLater();
        m_process = nullptr;
    }

    QStringList args;
    args << "-practice";
    if (!track.isEmpty()) {
        args << "-track" << track;
    }

    m_process = new QProcess(this);
    m_process->setProgram(exePath);
    m_process->setArguments(args);
    m_process->setWorkingDirectory(m_ksPath);
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_process, &QProcess::started, this, &KsAssettoCorsaRunner::onProcessStarted);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &KsAssettoCorsaRunner::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &KsAssettoCorsaRunner::onProcessError);

    m_process->start();
    return true;
}

bool KsAssettoCorsaRunner::stopKs() {
    if (!m_process) return false;
    m_process->terminate();
    if (!m_process->waitForFinished(5000)) {
        m_process->kill();
        m_process->waitForFinished(3000);
    }
    m_process->deleteLater();
    m_process = nullptr;
    m_status = KsAssettoCorsaRunnerStatus::Ready;
    emit statusChanged(m_status);
    return true;
}

QString KsAssettoCorsaRunner::getContentPath() const {
    return m_ksPath + "/content";
}

QString KsAssettoCorsaRunner::getTracksPath() const {
    return m_ksPath + "/content/tracks";
}

QString KsAssettoCorsaRunner::getCarsPath() const {
    return m_ksPath + "/content/cars";
}

bool KsAssettoCorsaRunner::isRunning() const {
    return m_process && m_process->state() == QProcess::Running;
}

}