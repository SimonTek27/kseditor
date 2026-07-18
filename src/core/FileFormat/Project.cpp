#include "core/FileFormat/Project.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <algorithm>

namespace ks {

QString ProjectFile::s_projectPath;
QString ProjectFile::s_projectName;
bool ProjectFile::s_modified = false;

bool ProjectFile::createProject(const QString& path, const QString& name, const QString& type)
{
    QDir dir(path);
    if (!dir.exists() && !dir.mkpath(".")) return false;

    s_projectPath = path;
    s_projectName = name;
    s_modified = false;

    QJsonObject root;
    root["name"] = name;
    root["type"] = type;
    root["version"] = "1.0";
    root["created"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    root["modified"] = root["created"];
    root["settings"] = QJsonObject{
        {"units", "meters"},
        {"autoSave", true},
        {"autoSaveInterval", 300},
        {"colorProfile", "sRGB"},
        {"gridSize", 1},
        {"snapToGrid", 1}
    };
    root["assets"] = QJsonArray();

    QFile file(path + "/project.json");
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(QJsonDocument(root).toJson());
    file.close();
    return true;
}

bool ProjectFile::openProject(const QString& path)
{
    QFile file(path + "/project.json");
    if (!file.exists()) return false;

    QFileInfo fi(file);
    s_projectPath = fi.absolutePath();
    s_projectName = fi.dir().dirName();
    s_modified = false;
    return true;
}

bool ProjectFile::saveProject(const QString& path)
{
    QString targetPath = path.isEmpty() ? s_projectPath : path;
    QString projectFile = targetPath + "/project.json";

    QJsonObject root;
    root["name"] = s_projectName;
    root["version"] = "1.0";
    root["modified"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    // Read existing project.json to preserve fields we don't manage
    QFile existingFile(projectFile);
    if (existingFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(existingFile.readAll());
        existingFile.close();
        if (doc.isObject()) {
            root = doc.object();
        }
    }
    root["modified"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QFile file(projectFile);
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(QJsonDocument(root).toJson());
    file.close();

    s_modified = false;
    return true;
}

bool ProjectFile::exportProject(const QString& path, const QString& format)
{
    if (path.isEmpty()) return false;

    QString fmt = format.toLower();
    if (fmt == "json" || fmt.isEmpty()) {
        // Read current project.json and write it to the export path
        QString srcFile = s_projectPath + "/project.json";
        QFile src(srcFile);
        if (!src.open(QIODevice::ReadOnly)) return false;
        QByteArray data = src.readAll();
        src.close();

        QFile dst(path + "/project.json");
        if (!dst.open(QIODevice::WriteOnly)) return false;
        dst.write(data);
        dst.close();
        return true;
    }

    if (fmt == "ksp") {
        // KSP format: single-file bundle with embedded JSON header
        QString projectFile = s_projectPath + "/project.json";
        QFile pFile(projectFile);
        if (!pFile.open(QIODevice::ReadOnly)) return false;
        QByteArray projectData = pFile.readAll();
        pFile.close();

        QFile dst(path + "/" + s_projectName + ".ksp");
        if (!dst.open(QIODevice::WriteOnly)) return false;
        dst.write(projectData);
        dst.close();
        return true;
    }

    return false;
}

ProjectMetadata ProjectFile::getMetadata()
{
    ProjectMetadata meta;
    meta.name = s_projectName;
    meta.version = "1.0";
    meta.author = "";
    meta.createdDate = QDateTime::currentDateTime().toString(Qt::ISODate);
    meta.modifiedDate = meta.createdDate;
    meta.projectType = "general";
    meta.assetSource = "local";
    return meta;
}

void ProjectFile::setMetadata(const ProjectMetadata& metadata)
{
    s_projectName = metadata.name;
    s_modified = true;
}

QVector<ProjectAsset> ProjectFile::getAssets()
{
    QVector<ProjectAsset> assets;
    QString projectFile = s_projectPath + "/project.json";
    QFile file(projectFile);
    if (!file.open(QIODevice::ReadOnly)) return assets;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) return assets;

    QJsonArray assetArray = doc.object()["assets"].toArray();
    for (const auto& val : assetArray) {
        QJsonObject obj = val.toObject();
        ProjectAsset asset;
        asset.id = obj["id"].toString();
        asset.name = obj["name"].toString();
        asset.type = obj["type"].toString();
        asset.filePath = obj["path"].toString();
        asset.metadata = obj["metadata"].toObject();
        assets.append(asset);
    }
    return assets;
}

static void writeAssets(const QVector<ProjectAsset>& assets);

void ProjectFile::addAsset(const ProjectAsset& asset)
{
    QVector<ProjectAsset> assets = getAssets();
    assets.append(asset);
    writeAssets(assets);
    s_modified = true;
}

void ProjectFile::removeAsset(const QString& id)
{
    QVector<ProjectAsset> assets = getAssets();
    assets.erase(std::remove_if(assets.begin(), assets.end(),
        [&id](const ProjectAsset& a) { return a.id == id; }), assets.end());
    writeAssets(assets);
    s_modified = true;
}

static void writeAssets(const QVector<ProjectAsset>& assets)
{
    QString projectFile = ProjectFile::getProjectPath() + "/project.json";
    QFile file(projectFile);
    if (!file.open(QIODevice::ReadOnly)) return;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) return;

    QJsonObject root = doc.object();
    QJsonArray assetArray;
    for (const auto& asset : assets) {
        QJsonObject obj;
        obj["id"] = asset.id;
        obj["name"] = asset.name;
        obj["type"] = asset.type;
        obj["path"] = asset.filePath;
        obj["metadata"] = asset.metadata;
        assetArray.append(obj);
    }
    root["assets"] = assetArray;
    root["modified"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    if (!file.open(QIODevice::WriteOnly)) return;
    file.write(QJsonDocument(root).toJson());
    file.close();
}

ProjectSettings ProjectFile::getSettings()
{
    ProjectSettings s;
    s.units = "meters";
    s.autoSave = true;
    s.autoSaveInterval = 300;
    s.colorProfile = "sRGB";
    s.gridSize = 1;
    s.snapToGrid = 1;
    return s;
}

void ProjectFile::setSettings(const ProjectSettings& settings)
{
    Q_UNUSED(settings);
    s_modified = true;
}

QString ProjectFile::getProjectPath() { return s_projectPath; }
QString ProjectFile::getProjectName() { return s_projectName; }
bool ProjectFile::isProjectModified() { return s_modified; }
void ProjectFile::markAsModified() { s_modified = true; }

} // namespace ks
