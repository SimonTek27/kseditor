#include "ProjectManager.h"
#include "sys/LogManager.h"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QDateTime>
#include <QMessageBox>

ProjectManager& ProjectManager::instance()
{
    static ProjectManager instance;
    return instance;
}

ProjectManager::ProjectManager(QObject* parent)
    : QObject(parent)
{
}

ProjectManager::~ProjectManager()
{
    if (m_modified && hasProject()) {
        LOG_WARNING("ProjectManager", "Closing modified project without saving");
    }
}

QString ProjectManager::projectDir() const
{
    QFileInfo info(m_projectPath);
    return info.absolutePath();
}

QString ProjectManager::projectName() const
{
    QFileInfo info(m_projectPath);
    return info.baseName();
}

bool ProjectManager::newProject(const QString& name, const QString& path, ProjectType type)
{
    QString projectPath = path;
    if (!projectPath.endsWith(".ksep")) {
        projectPath += "/" + name + ".ksep";
    }

    // Create directory structure
    QDir dir(QFileInfo(projectPath).absolutePath());
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString projectDirPath = QFileInfo(projectPath).absolutePath() + "/" + name;
    QDir().mkpath(projectDirPath);
    QDir().mkpath(projectDirPath + "/meshes");
    QDir().mkpath(projectDirPath + "/textures");
    QDir().mkpath(projectDirPath + "/configs");
    QDir().mkpath(projectDirPath + "/sounds");
    QDir().mkpath(projectDirPath + "/data");

    // Set metadata
    m_metadata.name = name;
    m_metadata.version = "1.0";
    m_metadata.created = QDateTime::currentDateTime().toString(Qt::ISODate);
    m_metadata.modified = m_metadata.created;
    m_metadata.type = type;

    m_projectPath = projectPath;
    m_files.clear();
    m_modified = true;

    // Save initial project
    if (!saveProject()) {
        LOG_ERROR("ProjectManager", "Failed to save new project");
        return false;
    }

    LOG_INFO("ProjectManager", "Created new project: " + name);
    emit projectOpened(m_projectPath);
    return true;
}

bool ProjectManager::openProject(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR("ProjectManager", "Failed to open project file: " + path);
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        LOG_ERROR("ProjectManager", "JSON parse error: " + error.errorString());
        return false;
    }

    if (!doc.isObject()) {
        LOG_ERROR("ProjectManager", "Invalid project format: root must be object");
        return false;
    }

    if (!loadFromJson(doc.object())) {
        return false;
    }

    m_projectPath = path;
    m_modified = false;

    LOG_INFO("ProjectManager", "Opened project: " + projectName());
    emit projectOpened(m_projectPath);
    return true;
}

bool ProjectManager::loadFromJson(const QJsonObject& obj)
{
    // Load metadata
    m_metadata.name = obj["name"].toString();
    m_metadata.version = obj["version"].toString("1.0");
    m_metadata.author = obj["author"].toString();
    m_metadata.description = obj["description"].toString();
    m_metadata.created = obj["created"].toString();
    m_metadata.modified = obj["modified"].toString();

    QString typeStr = obj["type"].toString().toLower();
    if (typeStr == "car") m_metadata.type = ProjectType::Car;
    else if (typeStr == "track") m_metadata.type = ProjectType::Track;
    else m_metadata.type = ProjectType::Generic;

    QJsonArray tagsArray = obj["tags"].toArray();
    m_metadata.tags.clear();
    for (const QJsonValue& v : tagsArray) {
        m_metadata.tags.append(v.toString());
    }

    // Load files
    m_files.clear();
    QJsonArray filesArray = obj["files"].toArray();
    for (const QJsonValue& v : filesArray) {
        QJsonObject fileObj = v.toObject();
        ProjectFile file;
        file.relativePath = fileObj["path"].toString();
        file.type = fileObj["type"].toString();
        file.included = fileObj["included"].toBool(true);
        m_files.append(file);
    }

    emit metadataChanged(m_metadata);
    emit filesChanged(m_files);
    return true;
}

bool ProjectManager::saveProject()
{
    if (m_projectPath.isEmpty()) {
        LOG_ERROR("ProjectManager", "No project path set");
        return false;
    }

    QJsonObject obj = saveToJson();
    QJsonDocument doc(obj);

    QFile file(m_projectPath);
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_ERROR("ProjectManager", "Failed to open file for writing: " + m_projectPath);
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    m_metadata.modified = QDateTime::currentDateTime().toString(Qt::ISODate);
    m_modified = false;

    LOG_INFO("ProjectManager", "Saved project: " + projectName());
    emit projectSaved(m_projectPath);
    return true;
}

bool ProjectManager::saveProjectAs(const QString& path)
{
    m_projectPath = path;
    return saveProject();
}

void ProjectManager::closeProject()
{
    if (m_modified && hasProject()) {
        // Prompt user to save changes
        QString msg = QStringLiteral("Project '%1' has unsaved changes.\nSave before closing?")
                          .arg(m_projectPath);
        auto result = QMessageBox::question(nullptr, tr("Unsaved Changes"), msg,
                                            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (result == QMessageBox::Save) {
            saveProject();
        } else if (result == QMessageBox::Cancel) {
            return;
        }
    }

    m_projectPath.clear();
    m_metadata = ProjectMetadata();
    m_files.clear();
    m_modified = false;

    emit projectClosed();
}

void ProjectManager::setMetadata(const ProjectMetadata& metadata)
{
    m_metadata = metadata;
    m_modified = true;
    emit metadataChanged(m_metadata);
}

void ProjectManager::addFile(const ProjectFile& file)
{
    m_files.append(file);
    m_modified = true;
    emit filesChanged(m_files);
}

void ProjectManager::removeFile(const QString& relativePath)
{
    for (int i = 0; i < m_files.size(); ++i) {
        if (m_files[i].relativePath == relativePath) {
            m_files.removeAt(i);
            m_modified = true;
            emit filesChanged(m_files);
            return;
        }
    }
}

void ProjectManager::updateFile(const QString& relativePath, const ProjectFile& file)
{
    for (int i = 0; i < m_files.size(); ++i) {
        if (m_files[i].relativePath == relativePath) {
            m_files[i] = file;
            m_modified = true;
            emit filesChanged(m_files);
            return;
        }
    }
}

QJsonObject ProjectManager::saveToJson() const
{
    QJsonObject obj;

    // Metadata
    obj["name"] = m_metadata.name;
    obj["version"] = m_metadata.version;
    obj["author"] = m_metadata.author;
    obj["description"] = m_metadata.description;
    obj["created"] = m_metadata.created;
    obj["modified"] = m_metadata.modified;

    QString typeStr;
    switch (m_metadata.type) {
        case ProjectType::Car:    typeStr = "car"; break;
        case ProjectType::Track:  typeStr = "track"; break;
        default:                  typeStr = "generic"; break;
    }
    obj["type"] = typeStr;

    QJsonArray tagsArray;
    for (const QString& tag : m_metadata.tags) {
        tagsArray.append(tag);
    }
    obj["tags"] = tagsArray;

    // Files
    QJsonArray filesArray;
    for (const ProjectFile& file : m_files) {
        QJsonObject fileObj;
        fileObj["path"] = file.relativePath;
        fileObj["type"] = file.type;
        fileObj["included"] = file.included;
        filesArray.append(fileObj);
    }
    obj["files"] = filesArray;

    return obj;
}

bool ProjectManager::build(const QString& outputPath)
{
    if (!hasProject()) {
        LOG_ERROR("ProjectManager", "No project open");
        return false;
    }

    LOG_INFO("ProjectManager", "Starting build: " + projectName());
    emit buildStarted();

    m_lastBuildOutput = outputPath.isEmpty()
        ? projectDir() + "/output"
        : outputPath;

    QDir outDir(m_lastBuildOutput);
    if (!outDir.exists()) {
        if (!QDir().mkpath(m_lastBuildOutput)) {
            LOG_ERROR("ProjectManager", "Failed to create output directory");
            emit buildFinished(false, m_lastBuildOutput);
            return false;
        }
    }

    // Copy project files to output
    int copied = 0;
    for (const auto& file : m_files) {
        QString src = projectDir() + "/" + file.relativePath;
        QString dst = m_lastBuildOutput + "/" + file.relativePath;
        QFileInfo dstInfo(dst);
        if (!dstInfo.dir().exists()) {
            dstInfo.dir().mkpath(".");
        }
        if (QFile::copy(src, dst)) {
            copied++;
        } else if (QFile::exists(src)) {
            LOG_WARNING("ProjectManager", "Could not copy: " + file.relativePath);
        }
    }

    // Create build manifest
    QJsonObject manifest;
    manifest["project"] = projectName();
    manifest["version"] = m_metadata.version;
    manifest["buildTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    QStringList paths;
    for (const auto& f : m_files) paths.append(f.relativePath);
    manifest["files"] = QJsonArray::fromStringList(paths);

    QFile mf(m_lastBuildOutput + "/build_manifest.json");
    if (mf.open(QIODevice::WriteOnly))
        mf.write(QJsonDocument(manifest).toJson());

    LOG_INFO("ProjectManager", QString("Build complete: %1 files copied to %2")
             .arg(copied).arg(m_lastBuildOutput));
    emit buildFinished(true, m_lastBuildOutput);
    return true;
}
