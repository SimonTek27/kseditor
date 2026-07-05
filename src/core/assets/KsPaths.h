#pragma once

#include <QString>
#include <QStringList>
#include <QDir>

namespace ks {

class KsPaths {
public:
    static QString detectKsRoot() {
        QStringList candidates = {
            "C:/Program Files (x86)/Steam/steamapps/common/assettocorsa",
            "C:/Program Files/Steam/steamapps/common/assettocorsa",
            QDir::homePath() + "/Documents/AssettoCorsa"
        };
        for (const auto& path : candidates) {
            if (isKsRoot(path)) return path;
        }
        return QString();
    }

    static bool isKsRoot(const QString& path) {
        if (path.isEmpty()) return false;
        return QDir(path + "/content").exists();
    }

    static QString getContentDirectory(const QString& root) {
        return root + "/content";
    }

    static QString getCarsDirectory(const QString& root) {
        return root + "/content/cars";
    }

    static QString getTracksDirectory(const QString& root) {
        return root + "/content/tracks";
    }

    static QString getSkinsDirectory(const QString& carPath) {
        return carPath + "/skins";
    }

    static QString getCarSkinDirectory(const QString& carPath, const QString& skinName) {
        return carPath + "/skins/" + skinName;
    }

    static QString getDriverDirectory(const QString& root) {
        return root + "/content/driver";
    }

    static QString getSfxDirectory(const QString& root) {
        return root + "/content/sfx";
    }

    static QString getTexturesDirectory(const QString& root) {
        return root + "/content/textures";
    }

    static QString getFontsDirectory(const QString& root) {
        return root + "/content/fonts";
    }

    static QString getShowroomDirectory(const QString& root) {
        return root + "/content/showroom";
    }

    static QString getWeatherDirectory(const QString& root) {
        return root + "/content/weather";
    }

    static QString getTracksWeatherDirectory(const QString& trackPath) {
        return trackPath + "/weather";
    }

    static QString getMainCarFile(const QString& carPath) {
        QString kn5 = carPath + "/car.kn5";
        if (QFile::exists(kn5)) return kn5;
        return QString();
    }
};

class SimInstallDetector {
public:
    static bool isValidInstallation(const QString& path);
    static QStringList findAllInstallations();
    static QStringList getCarList(const QString& simPath);
    static QStringList getTrackList(const QString& simPath);
    static QString findCarPath(const QString& simPath, const QString& carName);
    static QString findTrackPath(const QString& simPath, const QString& trackName);
    static QString getCarsDirectory(const QString& ksRoot);
    static QString getTracksDirectory(const QString& ksRoot);
    static QString getUiDirectory(const QString& simPath);
    static QString findBestInstallation();
};

} // namespace ks
