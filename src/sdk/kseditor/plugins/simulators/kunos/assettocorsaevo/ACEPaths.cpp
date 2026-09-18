#include "ACEPaths.h"
#include <QSettings>
#include <QStandardPaths>
#include <QDirIterator>
#include <QCoreApplication>
#include <QFileInfo>

namespace ks {

// ─── ACEPaths ────────────────────────────────────────────────────────────────

QString ACEPaths::getContentDirectory(const QString& aceRoot) {
    return aceRoot + "/content";
}

QString ACEPaths::getCarsDirectory(const QString& aceRoot) {
    return aceRoot + "/content/cars";
}

QString ACEPaths::getTracksDirectory(const QString& aceRoot) {
    return aceRoot + "/content/tracks";
}

QString ACEPaths::getSkinsDirectory(const QString& carPath) {
    return carPath + "/skins";
}

QString ACEPaths::getModsDirectory(const QString& aceRoot) {
    Q_UNUSED(aceRoot);
    return getSavedGamesDirectory() + "/mods";
}

QString ACEPaths::getContentKspkg(const QString& aceRoot) {
    return aceRoot + "/content.kspkg";
}

QString ACEPaths::getSavedGamesDirectory() {
    QString savedGames = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QString acePath = savedGames + "/ACE-Modder";
    if (QDir(acePath).exists()) {
        return acePath;
    }
    return savedGames + "/ACE";
}

QStringList ACEPaths::getCarNames(const QString& aceRoot) {
    QString carsDir = getCarsDirectory(aceRoot);
    QDir dir(carsDir);
    if (!dir.exists()) return QStringList();
    return dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
}

QStringList ACEPaths::getTrackNames(const QString& aceRoot) {
    QString tracksDir = getTracksDirectory(aceRoot);
    QDir dir(tracksDir);
    if (!dir.exists()) return QStringList();
    return dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
}

QStringList ACEPaths::getSkinNames(const QString& carPath) {
    QString skinsDir = getSkinsDirectory(carPath);
    QDir dir(skinsDir);
    if (!dir.exists()) return QStringList();
    return dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
}

bool ACEPaths::isValidInstallation(const QString& path) {
    if (path.isEmpty()) return false;
    QDir dir(path);
    if (!dir.exists()) return false;

    QFileInfo exeInfo(dir.filePath("AssettoCorsaEVO.exe"));
    if (exeInfo.exists()) return true;

    if (hasContentKspkg(path)) return true;

    if (hasUnpackedContent(path)) return true;

    return false;
}

QStringList ACEPaths::findAllInstallations() {
    QStringList installations;

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
                            QString libPath = parts[3];
                            libPath = libPath.replace("\\\\", "/");
                            QString acePath = libPath + "/steamapps/common/Assetto Corsa EVO";
                            if (isValidInstallation(acePath) && !installations.contains(acePath)) {
                                installations.append(acePath);
                            }
                        }
                    }
                }
            }
        }
    }

    QStringList searchPaths = {
        "C:/Program Files (x86)/Steam/steamapps/common/Assetto Corsa EVO",
        "C:/Program Files/Steam/steamapps/common/Assetto Corsa EVO",
        "D:/Steam/steamapps/common/Assetto Corsa EVO",
        "E:/Steam/steamapps/common/Assetto Corsa EVO",
        "F:/SteamLibrary/steamapps/common/Assetto Corsa EVO",
    };

    for (const QString& path : searchPaths) {
        if (isValidInstallation(path) && !installations.contains(path)) {
            installations.append(path);
        }
    }

    return installations;
}

QString ACEPaths::findBestInstallation() {
    QStringList installations = findAllInstallations();
    if (installations.isEmpty()) return QString();

    for (const QString& path : installations) {
        if (hasUnpackedContent(path)) {
            return path;
        }
    }

    return installations.first();
}

QString ACEPaths::findModDirectory() {
    QString savedGames = getSavedGamesDirectory();
    QString modsDir = savedGames + "/mods";
    if (QDir(modsDir).exists()) {
        return modsDir;
    }
    return QString();
}

bool ACEPaths::hasContentKspkg(const QString& aceRoot) {
    return QFile::exists(getContentKspkg(aceRoot));
}

bool ACEPaths::hasUnpackedContent(const QString& aceRoot) {
    QDir contentDir(getContentDirectory(aceRoot));
    return contentDir.exists() && (contentDir.exists("cars") || contentDir.exists("tracks"));
}

// ─── ACEContentReader ────────────────────────────────────────────────────────

ACECarInfo ACEContentReader::readCarInfo(const QString& carPath) {
    ACECarInfo info;
    info.path = carPath;
    info.name = QFileInfo(carPath).fileName();

    QDir skinsDir(carPath + "/skins");
    if (skinsDir.exists()) {
        info.skinNames = skinsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        info.skinCount = info.skinNames.size();
    }

    qint64 size = 0;
    recursiveSize(carPath, &size);
    info.totalSize = size;

    return info;
}

QStringList ACEContentReader::scanCars(const QString& aceRoot) {
    return ACEPaths::getCarNames(aceRoot);
}

QStringList ACEContentReader::scanTracks(const QString& aceRoot) {
    return ACEPaths::getTrackNames(aceRoot);
}

QStringList ACEContentReader::scanSkins(const QString& carPath) {
    return ACEPaths::getSkinNames(carPath);
}

void ACEContentReader::recursiveSize(const QString& path, qint64* size) {
    QDir dir(path);
    QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo& info : entries) {
        if (info.isFile()) {
            *size += info.size();
        } else if (info.isDir()) {
            recursiveSize(info.filePath(), size);
        }
    }
}

// ─── ACEContentFinder ────────────────────────────────────────────────────────

ACEContentFinder::ACEContentFinder(QObject* parent)
    : QObject(parent) {
}

QString ACEContentFinder::findAceRoot() const {
    if (!m_customPath.isEmpty() && ACEPaths::isValidInstallation(m_customPath)) {
        return m_customPath;
    }
    return ACEPaths::findBestInstallation();
}

bool ACEContentFinder::isAceInstalled() const {
    return !findAceRoot().isEmpty();
}

void ACEContentFinder::setCustomPath(const QString& path) {
    m_customPath = path;
    emit contentChanged();
}

QStringList ACEContentFinder::findCars() const {
    QString root = findAceRoot();
    if (root.isEmpty()) return QStringList();
    return ACEPaths::getCarNames(root);
}

QStringList ACEContentFinder::findTracks() const {
    QString root = findAceRoot();
    if (root.isEmpty()) return QStringList();
    return ACEPaths::getTrackNames(root);
}

QStringList ACEContentFinder::findMods() const {
    QString modsDir = ACEPaths::findModDirectory();
    if (modsDir.isEmpty()) return QStringList();
    QDir dir(modsDir);
    if (!dir.exists()) return QStringList();
    return dir.entryList(QStringList() << "*.kspkg", QDir::Files);
}

} // namespace ks
