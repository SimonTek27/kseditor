#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>

/**
 * @brief Project file format types
 */
enum class ProjectFormat {
    Current = 1  // ksEditor.project.json
};

/**
 * @brief Project type
 */
enum class ProjectType {
    Car,
    Track,
    Generic
};

/**
 * @brief Project metadata
 */
struct ProjectMetadata {
    QString name;
    QString version;
    QString author;
    QString description;
    QString created;
    QString modified;
    ProjectType type;
    QStringList tags;
};

/**
 * @brief Project file structure
 */
struct ProjectFile {
    QString path;
    QString relativePath;
    QString type;  // mesh, texture, sound, config, etc.
    bool included = true;
};

/**
 * @brief Project Manager - Handles project creation, loading, and saving
 */
class ProjectManager : public QObject
{
    Q_OBJECT

public:
    static ProjectManager& instance();

    // Current project
    bool hasProject() const { return !m_projectPath.isEmpty(); }
    QString projectPath() const { return m_projectPath; }
    QString projectDir() const;
    QString projectName() const;

    // Project lifecycle
    bool newProject(const QString& name, const QString& path, ProjectType type = ProjectType::Generic);
    bool openProject(const QString& path);
    bool saveProject();
    bool saveProjectAs(const QString& path);
    void closeProject();

    // Metadata
    ProjectMetadata metadata() const { return m_metadata; }
    void setMetadata(const ProjectMetadata& metadata);

    // Files
    const QList<ProjectFile>& files() const { return m_files; }
    void addFile(const ProjectFile& file);
    void removeFile(const QString& relativePath);
    void updateFile(const QString& relativePath, const ProjectFile& file);

    // Build
    bool build(const QString& outputPath);
    QString lastBuildOutput() const { return m_lastBuildOutput; }

signals:
    void projectOpened(const QString& path);
    void projectClosed();
    void projectSaved(const QString& path);
    void metadataChanged(const ProjectMetadata& metadata);
    void filesChanged(const QList<ProjectFile>& files);
    void buildStarted();
    void buildFinished(bool success, const QString& output);

private:
    explicit ProjectManager(QObject* parent = nullptr);
    ~ProjectManager();
    Q_DISABLE_COPY(ProjectManager)

    bool loadFromJson(const QJsonObject& obj);
    QJsonObject saveToJson() const;

    QString m_projectPath;
    ProjectMetadata m_metadata;
    QList<ProjectFile> m_files;
    QString m_lastBuildOutput;
    bool m_modified = false;
};
