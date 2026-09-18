#include "ContentBrowser.h"

#include <QFile>
#include <QTextStream>

namespace ks {
namespace plugin {

ContentSummary Browser::getContentSummary(const QString& acRoot) {
    ContentSummary summary;
    summary.totalCars = 0;
    summary.totalTracks = 0;
    summary.totalSkins = 0;
    summary.totalSize = 0;

    // Count cars
    QString carsDir = acRoot + "/content/cars";
    QDir carsDirObj(carsDir);
    if (carsDirObj.exists()) {
        summary.totalCars = carsDirObj.entryList(QDir::Dirs | QDir::NoDotAndDotDot).size();
    }

    // Count tracks
    QString tracksDir = acRoot + "/content/tracks";
    QDir tracksDirObj(tracksDir);
    if (tracksDirObj.exists()) {
        summary.totalTracks = tracksDirObj.entryList(QDir::Dirs | QDir::NoDotAndDotDot).size();
    }

    // Count skins
    if (carsDirObj.exists()) {
        QStringList carFolders = carsDirObj.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& car : carFolders) {
            QString skinsDir = carsDir + "/" + car + "/skins";
            QDir skinsDirObj(skinsDir);
            if (skinsDirObj.exists()) {
                summary.totalSkins += skinsDirObj.entryList(QDir::Dirs | QDir::NoDotAndDotDot).size();
            }
        }
    }

    // Calculate total size
    QDir rootDir(acRoot);
    calculateDirSize(acRoot, &summary.totalSize);

    return summary;
}

void Browser::calculateDirSize(const QString& path, qint64* totalSize) {
    QDir dir(path);
    QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QFileInfo& info : entries) {
        if (info.isFile()) {
            *totalSize += info.size();
        } else if (info.isDir()) {
            calculateDirSize(info.filePath(), totalSize);
        }
    }
}

QList<ks::CarInfo> Browser::getAllCars(const QString& acRoot) {
    QList<ks::CarInfo> cars;
    QString carsDir = acRoot + "/content/cars";

    QDir dir(carsDir);
    if (!dir.exists()) return cars;

    QStringList carFolders = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& carName : carFolders) {
        QString carPath = carsDir + "/" + carName;
        cars.append(ks::KsContentReader::readCarInfo(carPath));
    }

    return cars;
}

QList<ks::TrackInfo> Browser::getAllTracks(const QString& acRoot) {
    QList<ks::TrackInfo> tracks;
    QString tracksDir = acRoot + "/content/tracks";

    QDir dir(tracksDir);
    if (!dir.exists()) return tracks;

    QStringList trackFolders = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& trackName : trackFolders) {
        QString trackPath = tracksDir + "/" + trackName;
        tracks.append(ks::KsContentReader::readTrackInfo(trackPath));
    }

    return tracks;
}

QList<SkinInfo> Browser::getAllSkins(const QString& acRoot) {
    QList<SkinInfo> skins;
    QString carsDir = acRoot + "/content/cars";

    QDir dir(carsDir);
    if (!dir.exists()) return skins;

    QStringList carFolders = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& carName : carFolders) {
        QString carPath = carsDir + "/" + carName;
        QList<SkinInfo> carSkins = getSkinsForCar(carPath);
        skins.append(carSkins);
    }

    return skins;
}

QList<SkinInfo> Browser::getSkinsForCar(const QString& carPath) {
    QList<SkinInfo> skins;
    QString skinsDir = carPath + "/skins";

    QDir dir(skinsDir);
    if (!dir.exists()) return skins;

    QString carName = QFileInfo(carPath).fileName();
    QStringList skinFolders = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QString& skinName : skinFolders) {
        SkinInfo skin;
        skin.name = skinName;
        skin.carName = carName;
        skin.skinPath = skinsDir + "/" + skinName;
        skin.hasPreview = QFileInfo::exists(skin.skinPath + "/preview.jpg");
        skin.size = calculateFolderSize(skin.skinPath);
        skins.append(skin);
    }

    return skins;
}

qint64 Browser::calculateFolderSize(const QString& path) {
    qint64 size = 0;
    QDir dir(path);
    QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QFileInfo& info : entries) {
        if (info.isFile()) {
            size += info.size();
        } else if (info.isDir()) {
            size += calculateFolderSize(info.filePath());
        }
    }
    return size;
}

CarDetailInfo Browser::getCarDetails(const QString& carPath) {
    CarDetailInfo info;
    info.name = QFileInfo(carPath).fileName();
    info.parentPath = carPath;

    // Read car data from multiple sources
    auto carData = CarData::readCarData(carPath);

    // [BASIC] section
    info.brand = IniParser::getValue(carData, "BASIC", "brand", "Unknown");
    info.className = IniParser::getValue(carData, "BASIC", "class", "Street");
    info.type = IniParser::getValue(carData, "BASIC", "type", "car");
    info.power = IniParser::getValue(carData, "BASIC", "power", "0").toInt();
    info.weight = IniParser::getValue(carData, "BASIC", "weight", "0").toInt();
    info.drivetrain = IniParser::getValue(carData, "BASIC", "drivetrain", "RWD");
    info.year = IniParser::getValue(carData, "BASIC", "year", "0").toInt();
    info.engineType = IniParser::getValue(carData, "BASIC", "engine_type", "petrol");
    info.engineCylinders = IniParser::getValue(carData, "BASIC", "engine_cylinders", "4").toInt();
    info.displacement = IniParser::getValue(carData, "BASIC", "displacement", "0").toInt();
    info.torque = IniParser::getValue(carData, "BASIC", "torque", "0").toInt();

    // [CONTACT] section
    info.frontTyreSize = IniParser::getValue(carData, "CONTACT", "tyres_front", "");
    info.rearTyreSize = IniParser::getValue(carData, "CONTACT", "tyres_rear", "");

    // [PERFORMANCE] section
    info.topSpeed = IniParser::getValue(carData, "PERFORMANCE", "top_speed", "0").toInt();
    info.acceleration = IniParser::getValue(carData, "PERFORMANCE", "acceleration", "0").toInt();
    info.braking = IniParser::getValue(carData, "PERFORMANCE", "braking", "0").toInt();
    info.cornering = IniParser::getValue(carData, "PERFORMANCE", "cornering", "0").toInt();
    info.stability = IniParser::getValue(carData, "PERFORMANCE", "stability", "0").toInt();

    // [SPECS] section
    info.transmission = IniParser::getValue(carData, "SPECS", "transmission", "");
    info.drivetrainSpec = IniParser::getValue(carData, "SPECS", "drivetrain", "");
    info.weightDistribution = IniParser::getValue(carData, "SPECS", "weight_distribution", "");

    // [HEADER] section
    info.description = IniParser::getValue(carData, "HEADER", "description", "");
    info.tags = IniParser::getValue(carData, "HEADER", "tags", "");

    // Also read ui_car.json for additional metadata
    QString uiCarJson = carPath + "/ui/ui_car.json";
    QFile uiFile(uiCarJson);
    if (uiFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(uiFile.readAll());
        uiFile.close();
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            QJsonObject brands = root["brands"].toObject();
            if (!brands.isEmpty()) {
                info.brand = brands.value(info.name).toString();
            }
        }
    }

    // Check for sounds
    QDir sfxDir(carPath + "/sfx");
    info.hasSounds = sfxDir.exists() && !sfxDir.isEmpty();

    // Check for driver
    info.hasDriver = QFileInfo::exists(carPath + "/driver_base_pos.knh");

    // Count skins
    QDir skinsDir(carPath + "/skins");
    if (skinsDir.exists()) {
        info.skinNames = skinsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        info.skinCount = info.skinNames.size();
    }

    // Calculate size
    info.totalSize = calculateFolderSize(carPath);

    // Check if modified (has loose files)
    QDir carDir(carPath);
    info.isModified = !carDir.exists("data.acd");

    // KN5 file info
    QDir dir(carPath);
    QStringList kn5Files = dir.entryList(QStringList() << "*.kn5", QDir::Files);
    info.kn5Files = kn5Files;

    return info;
}

TrackDetailInfo Browser::getTrackDetails(const QString& trackPath) {
    TrackDetailInfo info;
    info.name = QFileInfo(trackPath).fileName();
    info.parentPath = trackPath;

    // Read track data
    auto trackData = TrackData::readTrackData(trackPath);

    info.location = IniParser::getValue(trackData, "track", "location", "Unknown");
    info.country = IniParser::getValue(trackData, "track", "country", "Unknown");
    info.city = IniParser::getValue(trackData, "track", "city", "");
    info.length = IniParser::getValue(trackData, "track", "length", "0").toFloat();
    info.pits = IniParser::getValue(trackData, "track", "pits", "0").toInt();
    info.width = IniParser::getValue(trackData, "track", "width", "0").toFloat();
    info.lapsCount = IniParser::getValue(trackData, "track", "laps", "0").toInt();
    info.description = IniParser::getValue(trackData, "HEADER", "description", "");

    // Read ui_track.json for additional metadata
    QString uiTrackJson = trackPath + "/ui/ui_track.json";
    QFile uiFile(uiTrackJson);
    if (uiFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(uiFile.readAll());
        uiFile.close();
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            info.name = root["name"].toString(info.name);
            info.description = root["description"].toString(info.description);
            info.geotags = root["geotags"].toString();
            info.city = root["city"].toString(info.city);
            info.country = root["country"].toString(info.country);
        }
    }

    // Check for AI
    QDir aiDir(trackPath + "/ai");
    info.hasAi = aiDir.exists() && !aiDir.isEmpty();

    // Check layouts (multi-layout tracks)
    QDir dataDir(trackPath + "/data");
    if (dataDir.exists()) {
        QStringList iniFiles = dataDir.entryList(QStringList() << "*.ini", QDir::Files);
        for (const QString& ini : iniFiles) {
            if (ini != "track_params.ini" && ini != "cameras.ini") {
                info.layouts.append(ini);
            }
        }
    }

    // Check map image
    info.hasMapImage = QFileInfo::exists(trackPath + "/map.png");

    // Count skins
    QDir skinsDir(trackPath + "/skins");
    if (skinsDir.exists()) {
        info.skinCount = skinsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot).size();
    }

    // Calculate size
    info.totalSize = calculateFolderSize(trackPath);

    return info;
}

QStringList Browser::findMissingAssets(const QString& acRoot) {
    QStringList missing;

    // Check main folders
    QStringList required = {
        "content/cars",
        "content/tracks",
        "content/texture",
        "apps",
        "system/cfg"
    };

    for (const QString& folder : required) {
        if (!QDir(acRoot + "/" + folder).exists()) {
            missing.append(folder);
        }
    }

    return missing;
}

QStringList Browser::findDuplicateSkins(const QString& acRoot) {
    QStringList duplicates;
    QMap<QString, QStringList> skinHashes;

    QList<SkinInfo> allSkins = getAllSkins(acRoot);
    for (const SkinInfo& skin : allSkins) {
        QString key = skin.carName + "/" + skin.name;
        if (skinHashes.contains(key)) {
            duplicates.append(key);
        }
        skinHashes[key].append(skin.skinPath);
    }

    return duplicates;
}

} // namespace plugin
} // namespace ks