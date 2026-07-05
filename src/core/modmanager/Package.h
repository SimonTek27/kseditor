#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QJsonObject>

class QZipWriter;

namespace ks {

struct PackageInfo {
    QString name;
    QString version;
    QString author;
    QString description;
    QString type;  // car, track, driver, object3d, showroom, skin, font
    QStringList tags;
    QString iconPath;
    QString gameVersion;
    QString website;
    QStringList includes;
};

class ksPackage
{
public:
    static bool createPackage(const QString& sourceFolder, const QString& outputPath, const PackageInfo& info);
    static bool extractPackage(const QString& packagePath, const QString& destFolder);
    static PackageInfo readManifest(const QString& packagePath);
    static bool validatePackage(const QString& packagePath);

    static QStringList getPackageTypes() {
        return {"car", "track", "driver", "object3d", "showroom", "skin", "fonts", "weather", "sound"};
    }

private:
    static QJsonObject packageInfoToJson(const PackageInfo& info);
    static PackageInfo jsonToPackageInfo(const QJsonObject& obj);
    static void addDirectoryToZip(QZipWriter* zip, const QString& sourceDir, const QString& relativePath);
};

} // namespace ks