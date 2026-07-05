#pragma once

#include <QString>
#include <QJsonObject>
#include <QVector>
#include <QMap>

namespace ks {

struct ProjectMetadata {
    QString name;
    QString version;
    QString author;
    QString createdDate;
    QString modifiedDate;
    QString projectType;
    QString assetSource;
};

struct ProjectAsset {
    QString id;
    QString type;
    QString name;
    QString filePath;
    QJsonObject metadata;
};

struct ProjectSettings {
    QString units;
    bool autoSave;
    int autoSaveInterval;
    QString colorProfile;
    int gridSize;
    int snapToGrid;
};

class ProjectFile {
public:
    static bool createProject(const QString& path, const QString& name, const QString& type);
    static bool openProject(const QString& path);
    static bool saveProject(const QString& path);
    static bool exportProject(const QString& path, const QString& format);

    static ProjectMetadata getMetadata();
    static void setMetadata(const ProjectMetadata& metadata);

    static QVector<ProjectAsset> getAssets();
    static void addAsset(const ProjectAsset& asset);
    static void removeAsset(const QString& id);

    static ProjectSettings getSettings();
    static void setSettings(const ProjectSettings& settings);

    static QString getProjectPath();
    static QString getProjectName();

    static bool isProjectModified();
    static void markAsModified();

private:
    static QString s_projectPath;
    static QString s_projectName;
    static bool s_modified;
};

} // namespace ks