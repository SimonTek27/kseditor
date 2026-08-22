#include "SimInstallDetector.h"
#include <QFile>
#include <QDirIterator>
#include <QSettings>
#include <QStandardPaths>

namespace ks {

bool SimInstallDetector::isValidInstallation(const QString& path) {
    if (path.isEmpty()) return false;
    QDir dir(path);
    if (!dir.exists()) return false;
    if (QFile::exists(path + "/content")) {
        QDir contentDir(path + "/content");
        if (contentDir.exists("cars") || contentDir.exists("tracks"))
            return true;
    }
    return false;
}

QStringList SimInstallDetector::findAllInstallations() {
    QStringList installations;

    // Check Steam library folders
    QSettings steamSettings("HKEY_CURRENT_USER\\Software\\Valve\\Steam", QSettings::NativeFormat);
    QString steamPath = steamSettings.value("SteamPath").toString();
    if (!steamPath.isEmpty()) {
        QString libraryFile = steamPath + "/steamapps/libraryfolders.vdf";
        if (QFile::exists(libraryFile)) {
            QFile file(libraryFile);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                while (!in.atEnd()) {
                    QString line = in.readLine();
                    if (line.contains("\"path\"")) {
                        QStringList parts = line.split("\"");
                        if (parts.size() >= 4) {
                            QString libPath = parts[3].replace("\\\\", "/");
                            QString acPath = libPath + "/steamapps/common/assettocorsa";
                            if (isValidInstallation(acPath) && !installations.contains(acPath))
                                installations.append(acPath);
                        }
                    }
                }
            }
        }
    }

    // Common fallback paths
    QStringList fallbacks = {
        "C:/Program Files (x86)/Steam/steamapps/common/assettocorsa",
        "C:/Program Files/Steam/steamapps/common/assettocorsa",
        "D:/Steam/steamapps/common/assettocorsa",
        "E:/Steam/steamapps/common/assettocorsa",
        "F:/SteamLibrary/steamapps/common/assettocorsa",
        QDir::homePath() + "/Documents/AssettoCorsa"
    };
    for (const QString& p : fallbacks) {
        if (isValidInstallation(p) && !installations.contains(p))
            installations.append(p);
    }

    return installations;
}

QStringList SimInstallDetector::getCarList(const QString& simPath) {
    QString carsDir = getCarsDirectory(simPath);
    QDir dir(carsDir);
    if (!dir.exists()) return {};
    return dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
}

QStringList SimInstallDetector::getTrackList(const QString& simPath) {
    QString tracksDir = getTracksDirectory(simPath);
    QDir dir(tracksDir);
    if (!dir.exists()) return {};
    return dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
}

QString SimInstallDetector::findCarPath(const QString& simPath, const QString& carName) {
    QString path = getCarsDirectory(simPath) + "/" + carName;
    return QDir(path).exists() ? path : QString();
}

QString SimInstallDetector::findTrackPath(const QString& simPath, const QString& trackName) {
    QString path = getTracksDirectory(simPath) + "/" + trackName;
    return QDir(path).exists() ? path : QString();
}

QString SimInstallDetector::getCarsDirectory(const QString& ksRoot) {
    return ksRoot + "/content/cars";
}

QString SimInstallDetector::getTracksDirectory(const QString& ksRoot) {
    return ksRoot + "/content/tracks";
}

QString SimInstallDetector::getUiDirectory(const QString& simPath) {
    return simPath + "/content/UI";
}

QString SimInstallDetector::findBestInstallation() {
    QStringList installs = findAllInstallations();
    return installs.isEmpty() ? QString() : installs.first();
}

} // namespace ks
