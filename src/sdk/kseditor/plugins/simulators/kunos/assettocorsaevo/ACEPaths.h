#pragma once

#include <QString>
#include <QStringList>
#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QObject>

namespace ks {

struct ACECarInfo {
    QString name;
    QString path;
    QStringList skinNames;
    int skinCount = 0;
    qint64 totalSize = 0;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["name"] = name;
        obj["path"] = path;
        obj["skinNames"] = QJsonArray::fromStringList(skinNames);
        obj["skinCount"] = skinCount;
        obj["totalSize"] = totalSize;
        return obj;
    }
};

class ACEPaths {
public:
    static QString getContentDirectory(const QString& aceRoot);
    static QString getCarsDirectory(const QString& aceRoot);
    static QString getTracksDirectory(const QString& aceRoot);
    static QString getSkinsDirectory(const QString& carPath);
    static QString getModsDirectory(const QString& aceRoot);
    static QString getContentKspkg(const QString& aceRoot);
    static QString getSavedGamesDirectory();

    static QStringList getCarNames(const QString& aceRoot);
    static QStringList getTrackNames(const QString& aceRoot);
    static QStringList getSkinNames(const QString& carPath);

    static bool isValidInstallation(const QString& path);
    static QStringList findAllInstallations();
    static QString findBestInstallation();
    static QString findModDirectory();

    static bool hasContentKspkg(const QString& aceRoot);
    static bool hasUnpackedContent(const QString& aceRoot);
};

class ACEContentReader {
public:
    static ACECarInfo readCarInfo(const QString& carPath);
    static QStringList scanCars(const QString& aceRoot);
    static QStringList scanTracks(const QString& aceRoot);
    static QStringList scanSkins(const QString& carPath);

private:
    static void recursiveSize(const QString& path, qint64* size);
};

class ACEContentFinder : public QObject {
    Q_OBJECT

public:
    explicit ACEContentFinder(QObject* parent = nullptr);

    QString findAceRoot() const;
    bool isAceInstalled() const;
    void setCustomPath(const QString& path);

    QStringList findCars() const;
    QStringList findTracks() const;
    QStringList findMods() const;

signals:
    void contentChanged();

private:
    QString m_customPath;

    QString searchForInstallation() const;
};

} // namespace ks
