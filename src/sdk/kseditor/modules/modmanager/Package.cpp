#include "Package.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QtCore/private/qzipwriter_p.h>
#include <QtCore/private/qzipreader_p.h>
#include <QStandardPaths>

namespace ks {

QJsonObject ksPackage::packageInfoToJson(const PackageInfo& info)
{
    QJsonObject obj;
    obj["name"] = info.name;
    obj["version"] = info.version;
    obj["author"] = info.author;
    obj["description"] = info.description;
    obj["type"] = info.type;
    obj["gameVersion"] = info.gameVersion;
    obj["website"] = info.website;

    QJsonArray tagsArray;
    for (const QString& tag : info.tags) {
        tagsArray.append(tag);
    }
    obj["tags"] = tagsArray;

    QJsonArray includesArray;
    for (const QString& inc : info.includes) {
        includesArray.append(inc);
    }
    obj["includes"] = includesArray;

    return obj;
}

PackageInfo ksPackage::jsonToPackageInfo(const QJsonObject& obj)
{
    PackageInfo info;
    info.name = obj["name"].toString();
    info.version = obj["version"].toString();
    info.author = obj["author"].toString();
    info.description = obj["description"].toString();
    info.type = obj["type"].toString();
    info.gameVersion = obj["gameVersion"].toString();
    info.website = obj["website"].toString();

    QJsonArray tagsArray = obj["tags"].toArray();
    for (const QJsonValue& val : tagsArray) {
        info.tags.append(val.toString());
    }

    QJsonArray includesArray = obj["includes"].toArray();
    for (const QJsonValue& val : includesArray) {
        info.includes.append(val.toString());
    }

    return info;
}

bool ksPackage::createPackage(const QString& sourceFolder, const QString& outputPath, const PackageInfo& info)
{
    QDir sourceDir(sourceFolder);
    if (!sourceDir.exists()) {
        return false;
    }

    QFile outputFile(outputPath);
    if (!outputFile.open(QIODevice::WriteOnly)) {
        return false;
    }

    QZipWriter zipWriter(&outputFile);
    zipWriter.setCompressionPolicy(QZipWriter::AlwaysCompress);

    QJsonObject manifest = packageInfoToJson(info);
    QByteArray manifestData = QJsonDocument(manifest).toJson(QJsonDocument::Indented);
    zipWriter.addFile("manifest.json", manifestData);

    addDirectoryToZip(&zipWriter, sourceFolder, "");

    zipWriter.close();
    outputFile.close();

    return true;
}

void ksPackage::addDirectoryToZip(QZipWriter* zip, const QString& sourceDir, const QString& relativePath)
{
    QDir dir(sourceDir);
    QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);

    for (const QFileInfo& entry : entries) {
        QString entryPath = relativePath.isEmpty() ? entry.fileName() : relativePath + "/" + entry.fileName();

        if (entry.isDir()) {
            addDirectoryToZip(zip, entry.absoluteFilePath(), entryPath);
        } else {
            QFile file(entry.absoluteFilePath());
            if (file.open(QIODevice::ReadOnly)) {
                zip->addFile(entryPath, file.readAll());
                file.close();
            }
        }
    }
}

bool ksPackage::extractPackage(const QString& packagePath, const QString& destFolder)
{
    QFile file(packagePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QZipReader zipReader(&file);

    if (!zipReader.exists()) {
        file.close();
        return false;
    }

    QDir destDir(destFolder);
    destDir.mkpath(".");

    for (const QZipReader::FileInfo& fileInfo : zipReader.fileInfoList()) {
        if (fileInfo.isDir) {
            destDir.mkpath(fileInfo.filePath);
        } else {
            QFile destFile(destFolder + "/" + fileInfo.filePath);
            QDir().mkpath(QFileInfo(destFile).absolutePath());
            if (destFile.open(QIODevice::WriteOnly)) {
                destFile.write(zipReader.fileData(fileInfo.filePath));
                destFile.close();
            }
        }
    }

    zipReader.close();
    file.close();

    return true;
}

PackageInfo ksPackage::readManifest(const QString& packagePath)
{
    QFile file(packagePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return PackageInfo();
    }

    QZipReader zipReader(&file);
    if (!zipReader.exists()) {
        file.close();
        return PackageInfo();
    }

    QByteArray manifestData = zipReader.fileData("manifest.json");
    zipReader.close();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(manifestData);
    if (doc.isNull() || !doc.isObject()) {
        return PackageInfo();
    }

    return jsonToPackageInfo(doc.object());
}

bool ksPackage::validatePackage(const QString& packagePath)
{
    QFile file(packagePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QZipReader zipReader(&file);
    bool exists = zipReader.exists() && zipReader.fileInfoList().size() > 0;
    bool hasManifest = false;

    for (const QZipReader::FileInfo& info : zipReader.fileInfoList()) {
        if (info.filePath == "manifest.json") {
            hasManifest = true;
            break;
        }
    }

    zipReader.close();
    file.close();

    return exists && hasManifest;
}

} // namespace ks