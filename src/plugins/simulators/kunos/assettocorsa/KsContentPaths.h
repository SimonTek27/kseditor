#pragma once

#include <QString>
#include <QStringList>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QJsonArray>
#include <QObject>
#include <QTextStream>

namespace ks {

class KsPaths {
public:
    static QString getContentDirectory(const QString& ksRoot);
    static QString getCarsDirectory(const QString& ksRoot);
    static QString getTracksDirectory(const QString& ksRoot);
    static QString getSkinsDirectory(const QString& carPath);
    static QString getCarSkinDirectory(const QString& carPath, const QString& skinName);
    static QString getDriverDirectory(const QString& ksRoot);
    static QString getSfxDirectory(const QString& ksRoot);
    static QString getTexturesDirectory(const QString& ksRoot);
    static QString getFontsDirectory(const QString& ksRoot);
    static QString getShowroomDirectory(const QString& ksRoot);
    static QString getWeatherDirectory(const QString& ksRoot);
    static QString getTracksWeatherDirectory(const QString& trackPath);

    static QStringList getCarNames(const QString& ksRoot);
    static QStringList getTrackNames(const QString& ksRoot);
    static QStringList getSkinNames(const QString& carPath);

    static QString getMainCarFile(const QString& carPath);

    static bool isValidInstallation(const QString& path);
    static QStringList findAllInstallations();
    static QStringList getCarList(const QString& simPath);
    static QStringList getTrackList(const QString& simPath);
    static QString findCarPath(const QString& simPath, const QString& carName);
    static QString findTrackPath(const QString& simPath, const QString& trackName);
    static QString getUiDirectory(const QString& simPath);
    static QString findBestInstallation();
};

struct CarInfo {
    QString name;
    QString path;
    QString mainKn5File;
    QStringList skinNames;
    bool hasSound;
    bool hasDriver;
    int skinCount;
    qint64 totalSize;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["name"] = name;
        obj["path"] = path;
        obj["mainKn5File"] = mainKn5File;
        obj["skinNames"] = QJsonArray::fromStringList(skinNames);
        obj["hasSound"] = hasSound;
        obj["hasDriver"] = hasDriver;
        obj["skinCount"] = skinCount;
        obj["totalSize"] = totalSize;
        return obj;
    }
};

struct TrackInfo {
    QString name;
    QString path;
    QString mainKn5File;
    QString mapPreview;
    QStringList skinNames;
    bool hasAi;
    int skinCount;
    qint64 totalSize;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["name"] = name;
        obj["path"] = path;
        obj["mainKn5File"] = mainKn5File;
        obj["mapPreview"] = mapPreview;
        obj["skinNames"] = QJsonArray::fromStringList(skinNames);
        obj["hasAi"] = hasAi;
        obj["skinCount"] = skinCount;
        obj["totalSize"] = totalSize;
        return obj;
    }
};

class KsContentReader {
public:
    static CarInfo readCarInfo(const QString& carPath);
    static TrackInfo readTrackInfo(const QString& trackPath);
    static QStringList scanCars(const QString& ksRoot);
    static QStringList scanTracks(const QString& ksRoot);
    static QStringList scanSkins(const QString& carPath);

private:
    static void recursiveSize(const QString& path, qint64* size);
};

class KsContentFinder : public QObject
{
    Q_OBJECT

public:
    explicit KsContentFinder(QObject *parent = nullptr);

    QString findKsRoot() const;
    QString findContentFolder() const;
    QString findAppsFolder() const;
    QString findSkiesFolder() const;
    QStringList findCars() const;
    QStringList findTracks() const;
    QStringList findSounds() const;
    QStringList findSkins() const;

    bool isKsInstalled() const;
    void setCustomPath(const QString &path);

signals:
    void contentChanged();

private:
    QString m_customPath;
    QStringList m_searchPaths;

    QString searchInPaths(const QString &folder) const;
    QStringList searchForFolders(const QString &folder) const;
    QStringList searchForExtension(const QString &path, const QString &extension) const;
};

} // namespace ks

// ---------------------------------------------------------------------------
// Low-level AC data file readers (moved from ksdata.h)
// ---------------------------------------------------------------------------
namespace KsD {

struct CarData {
    static QString readCarData(const QString& carPath) {
        QStringList paths = {
            carPath + "/data.acd",
            carPath + "/data/car.ini",
            carPath + "/data.ini"
        };
        for (const QString& p : paths) {
            QFile f(p);
            if (f.open(QIODevice::ReadOnly | QIODevice::Text))
                return QString::fromUtf8(f.readAll());
        }
        return {};
    }
};

struct TrackData {
    static QString readTrackData(const QString& trackPath) {
        QStringList paths = {
            trackPath + "/scene.acd",
            trackPath + "/data/track.ini",
            trackPath + "/ui_track.json"
        };
        for (const QString& p : paths) {
            QFile f(p);
            if (f.open(QIODevice::ReadOnly | QIODevice::Text))
                return QString::fromUtf8(f.readAll());
        }
        return {};
    }
};

struct IniParser {
    static QString getValue(const QString& data, const QString& section,
                            const QString& key, const QString& defaultValue = {}) {
        QString currentSection;
        QTextStream in(const_cast<QString*>(&data));
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith(';') || line.startsWith('#'))
                continue;
            if (line.startsWith('[') && line.endsWith(']')) {
                currentSection = line.mid(1, line.size() - 2).trimmed().toLower();
                continue;
            }
            int eq = line.indexOf('=');
            if (eq > 0 && currentSection == section.toLower()) {
                QString k = line.left(eq).trimmed();
                if (k.compare(key, Qt::CaseInsensitive) == 0)
                    return line.mid(eq + 1).trimmed();
            }
        }
        return defaultValue;
    }
};

} // namespace KsD
