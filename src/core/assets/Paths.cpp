#include "Paths.h"
#include <QCoreApplication>
#include <QStandardPaths>
#include <QSettings>
#include <QFile>
#include <QDir>
#include <QFileInfo>

#include "core/editor/EditorConfig.h"

namespace ks {

bool SimInstallDetector::isValidInstallation(const QString& path) {
    if (path.isEmpty()) return false;

    QString exePath = path + "/" + EditorConfig::instance().simExeName();
    QString dlssPath = path + "/plugins/dd/ddfile_x64.dll";

    return QFile::exists(exePath) || QFile::exists(dlssPath);
}

QStringList SimInstallDetector::findAllInstallations() {
    QStringList result;

    QStringList searchPaths = EditorConfig::instance().defaultSearchPaths();
    for (const QString& path : searchPaths) {
        if (isValidInstallation(path)) {
            result.append(path);
        }
    }

#ifdef _WIN32
    QSettings registry("HKEY_LOCAL_MACHINE\\SOFTWARE\\Valve\\Steam", QSettings::NativeFormat);
    QString steamPath = registry.value("InstallPath").toString();
    if (!steamPath.isEmpty()) {
        QString simPath = steamPath + "/" + EditorConfig::instance().steamRelativePath();
        if (isValidInstallation(simPath) && !result.contains(simPath)) {
            result.append(simPath);
        }
    }
#endif

    return result;
}

QStringList SimInstallDetector::getCarList(const QString& simPath) {
    QStringList result;
    QString carsDir = getCarsDirectory(simPath);

    if (!QDir(carsDir).exists()) {
        return result;
    }

    QDir dir(carsDir);
    for (const QFileInfo& info : dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QString carDir = info.filePath() + "/ui";
        if (QDir(carDir).exists() || QFile::exists(info.filePath() + "/data.acd")) {
            result.append(info.fileName());
        }
    }

    result.sort();
    return result;
}

QStringList SimInstallDetector::getTrackList(const QString& simPath) {
    QStringList result;
    QString tracksDir = getTracksDirectory(simPath);

    if (!QDir(tracksDir).exists()) {
        return result;
    }

    QDir dir(tracksDir);
    for (const QFileInfo& info : dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        result.append(info.fileName());
    }

    result.sort();
    return result;
}

QString SimInstallDetector::findCarPath(const QString& simPath, const QString& carName) {
    QString carsDir = getCarsDirectory(simPath);
    QString carPath = carsDir + "/" + carName;

    if (QDir(carPath).exists()) {
        return carPath;
    }

    return QString();
}

QString SimInstallDetector::findTrackPath(const QString& simPath, const QString& trackName) {
    QString tracksDir = getTracksDirectory(simPath);
    QString trackPath = tracksDir + "/" + trackName;

    if (QDir(trackPath).exists()) {
        return trackPath;
    }

    return QString();
}

QString SimInstallDetector::getUiDirectory(const QString& simPath) {
    return simPath + "/content/ui";
}

QString SimInstallDetector::findBestInstallation() {
    QStringList installations = findAllInstallations();

    if (!installations.isEmpty()) {
        return installations.first();
    }

    return QString();
}

// ─── KsPathDetector ──────────────────────────────────────────────────────────

KsPathDetector::KsPathDetector(QObject* parent)
    : QObject(parent)
{
}

KsPathDetector::~KsPathDetector() = default;

QString KsPathDetector::detect()
{
    QStringList candidates = SimInstallDetector::findAllInstallations();
    if (candidates.isEmpty()) {
        emit error("Simulator installation not found");
        return {};
    }
    m_simPath = candidates.first();
    m_contentPath = m_simPath + "/content";
    m_cars = SimInstallDetector::getCarList(m_simPath);
    m_tracks = SimInstallDetector::getTrackList(m_simPath);
    emit detected(m_simPath);
    return m_simPath;
}

QString SimInstallDetector::getCarsDirectory(const QString& ksRoot) {
    if (ksRoot.isEmpty()) return QString();
    return ksRoot + "/content/cars";
}

QString SimInstallDetector::getTracksDirectory(const QString& ksRoot) {
    if (ksRoot.isEmpty()) return QString();
    return ksRoot + "/content/tracks";
}

} // namespace ks