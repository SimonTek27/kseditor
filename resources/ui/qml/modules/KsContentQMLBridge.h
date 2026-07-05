#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

class KsContentQMLBridge : public QObject {
    Q_OBJECT
public:
    explicit KsContentQMLBridge(QObject* parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE QVariantList listCars(const QString& acRoot) {
        return listContentDir(acRoot + "/content/cars", "car");
    }

    Q_INVOKABLE QVariantList listTracks(const QString& acRoot) {
        return listContentDir(acRoot + "/content/tracks", "track");
    }

    Q_INVOKABLE QVariantList listSkins(const QString& carPath) {
        QVariantList result;
        QDir skinsDir(carPath + "/skins");
        if (skinsDir.exists()) {
            for (const QString& s : skinsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                QVariantMap m;
                m["name"] = s;
                m["path"] = skinsDir.absoluteFilePath(s);
                m["preview"] = QFile::exists(skinsDir.absoluteFilePath(s) + "/preview.png")
                    ? skinsDir.absoluteFilePath(s) + "/preview.png" : "";
                result.append(m);
            }
        }
        return result;
    }

    Q_INVOKABLE QVariantMap getContentInfo(const QString& path) {
        QVariantMap info;
        info["name"] = QFileInfo(path).fileName();
        info["path"] = path;
        info["exists"] = QFileInfo::exists(path);
        info["size"] = 0;
        QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) { it.next(); info["size"] = info["size"].toLongLong() + it.fileInfo().size(); }
        info["hasPreview"] = QFile::exists(path + "/preview.png") || QFile::exists(path + "/preview.jpg");
        return info;
    }

    Q_INVOKABLE QStringList findContent(const QString& acRoot, const QString& query) {
        QStringList results;
        QString lower = query.toLower();
        for (const QString& dir : QStringList() << acRoot + "/content/cars" << acRoot + "/content/tracks") {
            QDir d(dir);
            if (!d.exists()) continue;
            for (const QString& entry : d.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                if (entry.toLower().contains(lower))
                    results.append(d.absoluteFilePath(entry));
            }
        }
        return results;
    }

private:
    static QVariantList listContentDir(const QString& dirPath, const QString& type) {
        QVariantList result;
        QDir dir(dirPath);
        if (!dir.exists()) return result;
        for (const QFileInfo& fi : dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            QVariantMap m;
            m["name"] = fi.fileName();
            m["path"] = fi.absoluteFilePath();
            m["type"] = type;
            m["preview"] = QFile::exists(fi.absoluteFilePath() + "/preview.png")
                ? fi.absoluteFilePath() + "/preview.png" : "";
            result.append(m);
        }
        return result;
    }
};

