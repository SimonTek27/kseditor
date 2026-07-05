#include "KsContentPaths.h"
#include <QSettings>
#include <QStandardPaths>
#include <QDirIterator>
#include <QCoreApplication>

namespace ks {

// ─── KsPaths ─────────────────────────────────────────────────────────────────

QString KsPaths::getContentDirectory(const QString& ksRoot) {
    return ksRoot + "/content";
}

QString KsPaths::getCarsDirectory(const QString& ksRoot) {
    return ksRoot + "/content/cars";
}

QString KsPaths::getTracksDirectory(const QString& ksRoot) {
    return ksRoot + "/content/tracks";
}

QString KsPaths::getSkinsDirectory(const QString& carPath) {
    return carPath + "/skins";
}

QString KsPaths::getCarSkinDirectory(const QString& carPath, const QString& skinName) {
    return carPath + "/skins/" + skinName;
}

QString KsPaths::getDriverDirectory(const QString& ksRoot) {
    return ksRoot + "/content/driver";
}

QString KsPaths::getSfxDirectory(const QString& ksRoot) {
    return ksRoot + "/content/sfx";
}

QString KsPaths::getTexturesDirectory(const QString& ksRoot) {
    return ksRoot + "/content/textures";
}

QString KsPaths::getFontsDirectory(const QString& ksRoot) {
    return ksRoot + "/content/fonts";
}

QString KsPaths::getShowroomDirectory(const QString& ksRoot) {
    return ksRoot + "/content/showroom";
}

QString KsPaths::getWeatherDirectory(const QString& ksRoot) {
    return ksRoot + "/content/weather";
}

QString KsPaths::getTracksWeatherDirectory(const QString& trackPath) {
    return trackPath + "/weather";
}

QStringList KsPaths::getCarNames(const QString& ksRoot) {
    QString carsDir = getCarsDirectory(ksRoot);
    QDir dir(carsDir);
    if (!dir.exists()) return QStringList();
    return dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
}

QStringList KsPaths::getTrackNames(const QString& ksRoot) {
    QString tracksDir = getTracksDirectory(ksRoot);
    QDir dir(tracksDir);
    if (!dir.exists()) return QStringList();
    return dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
}

QStringList KsPaths::getSkinNames(const QString& carPath) {
    QString skinsDir = getSkinsDirectory(carPath);
    QDir dir(skinsDir);
    if (!dir.exists()) return QStringList();
    return dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
}

QString KsPaths::getMainCarFile(const QString& carPath) {
    QDir dir(carPath);
    QStringList kn5Files = dir.entryList(QStringList() << "*.kn5", QDir::Files);
    if (!kn5Files.isEmpty()) {
        return carPath + "/" + kn5Files.first();
    }
    return QString();
}

bool KsPaths::isValidInstallation(const QString& path) {
    if (path.isEmpty()) return false;
    QDir dir(path);
    if (!dir.exists()) return false;

    QDir contentDir(path + "/content");
    if (!contentDir.exists()) return false;

    QDir carsDir(path + "/content/cars");
    QDir tracksDir(path + "/content/tracks");

    return carsDir.exists() || tracksDir.exists();
}

QStringList KsPaths::findAllInstallations() {
    QStringList installations;

    QStringList searchPaths;
    searchPaths << "C:/Program Files (x86)/Steam/steamapps/core/assettocorsa";
    searchPaths << "C:/Program Files/Steam/steamapps/core/assettocorsa";
    searchPaths << "D:/Steam/steamapps/core/assettocorsa";
    searchPaths << "E:/Steam/steamapps/core/assettocorsa";
    searchPaths << "F:/Steam/steamapps/core/assettocorsa";

    QSettings steamSettings("HKEY_CURRENT_USER\\Software\\Valve\\Steam", QSettings::NativeFormat);
    QString steamPath = steamSettings.value("SteamPath").toString();
    if (!steamPath.isEmpty()) {
        QString libraryPath = steamPath + "/steamapps/libraryfolders.vdf";
        if (QFile::exists(libraryPath)) {
            QFile file(libraryPath);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                while (!in.atEnd()) {
                    QString line = in.readLine();
                    if (line.contains("\"path\"")) {
                        QStringList parts = line.split("\"");
                        if (parts.size() >= 4) {
                            QString libPath = parts[3];
                            libPath = libPath.replace("\\\\", "/");
                            QString acPath = libPath + "/steamapps/core/assettocorsa";
                            if (isValidInstallation(acPath) && !installations.contains(acPath)) {
                                installations.append(acPath);
                            }
                        }
                    }
                }
            }
        }
    }

    for (const QString& path : searchPaths) {
        if (isValidInstallation(path) && !installations.contains(path)) {
            installations.append(path);
        }
    }

    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString acAppData = appDataPath + "/acs";
    if (isValidInstallation(acAppData) && !installations.contains(acAppData)) {
        installations.append(acAppData);
    }

    return installations;
}

QStringList KsPaths::getCarList(const QString& simPath) {
    return getCarNames(simPath);
}

QStringList KsPaths::getTrackList(const QString& simPath) {
    return getTrackNames(simPath);
}

QString KsPaths::findCarPath(const QString& simPath, const QString& carName) {
    QString carsDir = getCarsDirectory(simPath);
    QString carPath = carsDir + "/" + carName;
    if (QDir(carPath).exists()) {
        return carPath;
    }
    return QString();
}

QString KsPaths::findTrackPath(const QString& simPath, const QString& trackName) {
    QString tracksDir = getTracksDirectory(simPath);
    QString trackPath = tracksDir + "/" + trackName;
    if (QDir(trackPath).exists()) {
        return trackPath;
    }
    return QString();
}

QString KsPaths::getUiDirectory(const QString& simPath) {
    return simPath + "/system/ui";
}

QString KsPaths::findBestInstallation() {
    QStringList installations = findAllInstallations();
    if (installations.isEmpty()) return QString();

    for (const QString& path : installations) {
        QDir carsDir(path + "/content/cars");
        if (carsDir.exists() && carsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot).size() > 5) {
            return path;
        }
    }

    return installations.first();
}

// ─── KsContentReader ─────────────────────────────────────────────────────────

CarInfo KsContentReader::readCarInfo(const QString& carPath) {
    CarInfo info;
    info.path = carPath;
    info.name = QFileInfo(carPath).fileName();

    QDir dir(carPath);
    dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

    QFileInfo mainKn5(carPath + ".kn5");
    if (mainKn5.exists()) {
        info.mainKn5File = mainKn5.filePath();
    } else {
        QStringList kn5Files = dir.entryList(QStringList() << "*.kn5", QDir::Files);
        if (!kn5Files.isEmpty()) {
            info.mainKn5File = carPath + "/" + kn5Files.first();
        }
    }

    QFileInfo collider(carPath + "/collider.kn5");
    QFileInfo driverBase(carPath + "/driver_base_pos.knh");
    QFileInfo sfxBank(carPath + "/sfx");

    info.hasSound = collider.exists();
    info.hasDriver = driverBase.exists();

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

TrackInfo KsContentReader::readTrackInfo(const QString& trackPath) {
    TrackInfo info;
    info.path = trackPath;
    info.name = QFileInfo(trackPath).fileName();

    QDir dir(trackPath);

    QFileInfo mainKn5(trackPath + ".kn5");
    if (mainKn5.exists()) {
        info.mainKn5File = mainKn5.filePath();
    } else {
        QStringList kn5Files = dir.entryList(QStringList() << "*.kn5", QDir::Files);
        if (!kn5Files.isEmpty()) {
            info.mainKn5File = trackPath + "/" + kn5Files.first();
        }
    }

    QFileInfo mapPng(trackPath + "/map.png");
    if (mapPng.exists()) {
        info.mapPreview = mapPng.filePath();
    }

    QFileInfo aiDir(trackPath + "/ai");
    info.hasAi = aiDir.exists();

    QDir skinsDir(trackPath + "/skins");
    if (skinsDir.exists()) {
        info.skinNames = skinsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        info.skinCount = info.skinNames.size();
    }

    qint64 size = 0;
    recursiveSize(trackPath, &size);
    info.totalSize = size;

    return info;
}

QStringList KsContentReader::scanCars(const QString& ksRoot) {
    QString carsDir = ksRoot + "/content/cars";
    QDir dir(carsDir);
    if (!dir.exists()) return QStringList();
    return dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
}

QStringList KsContentReader::scanTracks(const QString& ksRoot) {
    QString tracksDir = ksRoot + "/content/tracks";
    QDir dir(tracksDir);
    if (!dir.exists()) return QStringList();
    return dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
}

QStringList KsContentReader::scanSkins(const QString& carPath) {
    QString skinsDir = carPath + "/skins";
    QDir dir(skinsDir);
    if (!dir.exists()) return QStringList();
    return dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
}

void KsContentReader::recursiveSize(const QString& path, qint64* size) {
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

// ─── KsContentFinder ─────────────────────────────────────────────────────────

KsContentFinder::KsContentFinder(QObject *parent)
    : QObject(parent)
{
    m_searchPaths << QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/Assetto Corsa";
    m_searchPaths << "C:/ProgramData/Assetto Corsa";
    m_searchPaths << "D:/ProgramData/Assetto Corsa";
    m_searchPaths << QCoreApplication::applicationDirPath() + "/ks_root";
}

QString KsContentFinder::findKsRoot() const
{
    if (!m_customPath.isEmpty()) {
        return m_customPath;
    }
    return searchInPaths("");
}

QString KsContentFinder::searchInPaths(const QString &folder) const
{
    for (const QString &path : m_searchPaths) {
        QString fullPath = path + "/" + folder;
        if (QDir(fullPath).exists()) {
            return fullPath;
        }
    }
    return QString();
}

QString KsContentFinder::findContentFolder() const
{
    return searchInPaths("content");
}

QString KsContentFinder::findAppsFolder() const
{
    return searchInPaths("apps");
}

QString KsContentFinder::findSkiesFolder() const
{
    return searchInPaths("skies");
}

QStringList KsContentFinder::findCars() const
{
    QString content = findContentFolder();
    if (content.isEmpty()) return QStringList();

    QString carsPath = content + "/cars";
    return searchForExtension(carsPath, QString());
}

QStringList KsContentFinder::findTracks() const
{
    QString content = findContentFolder();
    if (content.isEmpty()) return QStringList();

    QString tracksPath = content + "/tracks";
    return searchForExtension(tracksPath, QString());
}

QStringList KsContentFinder::findSounds() const
{
    QString content = findContentFolder();
    if (content.isEmpty()) return QStringList();

    QString soundsPath = content + "/sounds";
    return searchForExtension(soundsPath, QString());
}

QStringList KsContentFinder::findSkins() const
{
    QString content = findContentFolder();
    if (content.isEmpty()) return QStringList();

    QStringList skins;
    QDir carsDir(content + "/cars");

    if (carsDir.exists()) {
        for (const QString &car : carsDir.entryList(QDir::Dirs)) {
            QString skinsPath = content + "/cars/" + car + "/skins";
            if (QDir(skinsPath).exists()) {
                skins << QDir(skinsPath).entryList(QDir::Dirs);
            }
        }
    }
    return skins;
}

QStringList KsContentFinder::searchForFolders(const QString &folder) const
{
    QStringList result;
    QDir dir(folder);

    if (!dir.exists()) return result;

    result = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    return result;
}

QStringList KsContentFinder::searchForExtension(const QString &path, const QString &extension) const
{
    QStringList result;
    QDir dir(path);

    if (!dir.exists()) return result;

    if (extension.isEmpty()) {
        result = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    } else {
        result = dir.entryList(QStringList() << "*." + extension, QDir::Files);
    }

    return result;
}

bool KsContentFinder::isKsInstalled() const
{
    return !findKsRoot().isEmpty();
}

void KsContentFinder::setCustomPath(const QString &path)
{
    m_customPath = path;
    emit contentChanged();
}

} // namespace ks
