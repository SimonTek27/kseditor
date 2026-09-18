#pragma once

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>
#include <QMap>
#include <QUuid>

namespace ks {

struct WorkshopItem {
    QString id;
    QString name;
    QString version;
    QString author;
    QString description;
    QString category;
    QStringList tags;
    QString previewUrl;
    QStringList screenshots;
    // Dependencies support version constraints: "name", "name (>= 1.0.0)", "name (~> 2.0)", "name (^1.2.3)"
    QStringList dependencies;
    QStringList conflicts;
    QString license;
    QString website;
    qint64 fileSize = 0;
    QString packagePath;
    QDateTime createdAt;
    QDateTime updatedAt;
    int downloadCount = 0;
    float rating = 0.0f;
    int ratingCount = 0;
    bool isInstalled = false;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["name"] = name;
        obj["version"] = version;
        obj["author"] = author;
        obj["description"] = description;
        obj["category"] = category;
        QJsonArray t;
        for (const auto& tag : tags) t.append(tag);
        obj["tags"] = t;
        obj["preview_url"] = previewUrl;
        QJsonArray s;
        for (const auto& shot : screenshots) s.append(shot);
        obj["screenshots"] = s;
        QJsonArray d;
        for (const auto& dep : dependencies) d.append(dep);
        obj["dependencies"] = d;
        QJsonArray c;
        for (const auto& con : conflicts) c.append(con);
        obj["conflicts"] = c;
        obj["license"] = license;
        obj["website"] = website;
        obj["file_size"] = fileSize;
        obj["package_path"] = packagePath;
        obj["created_at"] = createdAt.toString(Qt::ISODate);
        obj["updated_at"] = updatedAt.toString(Qt::ISODate);
        obj["download_count"] = downloadCount;
        obj["rating"] = static_cast<double>(rating);
        obj["rating_count"] = ratingCount;
        obj["installed"] = isInstalled;
        return obj;
    }

    static WorkshopItem fromJson(const QJsonObject& obj) {
        WorkshopItem item;
        item.id = obj["id"].toString();
        item.name = obj["name"].toString();
        item.version = obj["version"].toString();
        item.author = obj["author"].toString();
        item.description = obj["description"].toString();
        item.category = obj["category"].toString();
        for (const auto& v : obj["tags"].toArray())
            item.tags.append(v.toString());
        item.previewUrl = obj["preview_url"].toString();
        for (const auto& v : obj["screenshots"].toArray())
            item.screenshots.append(v.toString());
        for (const auto& v : obj["dependencies"].toArray())
            item.dependencies.append(v.toString());
        for (const auto& v : obj["conflicts"].toArray())
            item.conflicts.append(v.toString());
        item.license = obj["license"].toString();
        item.website = obj["website"].toString();
        item.fileSize = static_cast<qint64>(obj["file_size"].toDouble());
        item.packagePath = obj["package_path"].toString();
        item.createdAt = QDateTime::fromString(obj["created_at"].toString(), Qt::ISODate);
        item.updatedAt = QDateTime::fromString(obj["updated_at"].toString(), Qt::ISODate);
        item.downloadCount = obj["download_count"].toInt();
        item.rating = static_cast<float>(obj["rating"].toDouble());
        item.ratingCount = obj["rating_count"].toInt();
        item.isInstalled = obj["installed"].toBool();
        return item;
    }

    static QString generateId() {
        return QUuid::createUuid().toString(QUuid::Id128);
    }

    static QStringList standardCategories() {
        return {"cars", "tracks", "skins", "apps", "weather", "config", "tools", "audio", "fonts", "models"};
    }
};

using WorkshopItemList = QVector<WorkshopItem>;

}
