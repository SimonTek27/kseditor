#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QDir>
#include <QFileInfo>

#include "KsAssettoCorsaContentPath.h"

/**
 * @brief Content Browser - Lists and manages AC content
 * 
 * Provides functions to scan, list, and get detailed info about
 * cars, tracks, skins, and other AC content.
 */

namespace ks {
namespace plugin {

struct ContentSummary {
    int totalCars;
    int totalTracks;
    int totalSkins;
    qint64 totalSize;

    QString toHtml() const {
        return QString(
            "<table>"
            "<tr><td>Cars:</td><td>%1</td></tr>"
            "<tr><td>Tracks:</td><td>%2</td></tr>"
            "<tr><td>Skins:</td><td>%3</td></tr>"
            "<tr><td>Total Size:</td><td>%4</td></tr>"
            "</table>"
        ).arg(totalCars).arg(totalTracks).arg(totalSkins).arg(formatSize(totalSize));
    }

    static QString formatSize(qint64 bytes) {
        if (bytes < 1024) return QString::number(bytes) + " B";
        if (bytes < 1024 * 1024) return QString::number(bytes / 1024.0, 'f', 1) + " KB";
        if (bytes < 1024 * 1024 * 1024) return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
        return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
    }
};

struct SkinInfo {
    QString name;
    QString carName;
    QString skinPath;
    bool hasPreview;
    qint64 size;

    QString getPreviewPath() const {
        return skinPath + "/preview.jpg";
    }

    QString getLiveryPath() const {
        return skinPath + "/livery.png";
    }
};

struct CarDetailInfo {
    QString name;
    QString brand;
    QString className;
    int power = 0;
    int weight = 0;
    QString drivetrain;
    QString type;
    int skinCount = 0;
    QStringList skinNames;
    bool hasSounds = false;
    bool hasDriver = false;
    bool isModified = false;
    qint64 totalSize = 0;
    QString parentPath;

    // Extended fields
    int year = 0;
    QString engineType;
    int engineCylinders = 0;
    int displacement = 0;
    int torque = 0;
    QString frontTyreSize;
    QString rearTyreSize;
    int topSpeed = 0;
    int acceleration = 0;
    int braking = 0;
    int cornering = 0;
    int stability = 0;
    QString transmission;
    QString drivetrainSpec;
    QString weightDistribution;
    QString description;
    QString tags;
    QStringList kn5Files;
};

struct TrackDetailInfo {
    QString name;
    QString location;
    QString country;
    float length = 0.0f;
    int pits = 0;
    bool hasAi = false;
    QStringList layouts;
    int skinCount = 0;
    qint64 totalSize = 0;
    QString parentPath;

    // Extended fields
    QString city;
    float width = 0.0f;
    int lapsCount = 0;
    QString description;
    QString geotags;
    bool hasMapImage = false;
};

class Browser {
public:
    static ContentSummary getContentSummary(const QString& acRoot);
    static QList<ks::CarInfo> getAllCars(const QString& acRoot);
    static QList<ks::TrackInfo> getAllTracks(const QString& acRoot);
    static QList<SkinInfo> getAllSkins(const QString& acRoot);
    static QList<SkinInfo> getSkinsForCar(const QString& carPath);

    static CarDetailInfo getCarDetails(const QString& carPath);
    static TrackDetailInfo getTrackDetails(const QString& trackPath);

    static QStringList findMissingAssets(const QString& acRoot);
    static QStringList findDuplicateSkins(const QString& acRoot);
    static QList<qint64> calculateContentSize(const QString& acRoot);

private:
    static void calculateDirSize(const QString& path, qint64* totalSize);
    static qint64 calculateFolderSize(const QString& path);
};

} // namespace plugin
} // namespace ks