#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QJsonObject>

namespace ks {

struct ProjectTemplate {
    QString id;
    QString name;
    QString description;
    QString category;
    QString thumbnailPath;
    QVector<QString> requiredFiles;
    QVector<QString> optionalFiles;
    QJsonObject defaultSettings;
    QString gameVersion;
    bool isBuiltIn = false;
};

class ProjectTemplates : public QObject {
    Q_OBJECT

public:
    static ProjectTemplates* instance();

    explicit ProjectTemplates(QObject* parent = nullptr);
    ~ProjectTemplates();

    void loadTemplates();
    void loadTemplate(const QString& path);

    QVector<ProjectTemplate> getTemplates(const QString& category = QString()) const;
    ProjectTemplate getTemplate(const QString& templateId) const;

    QString createFromTemplate(const QString& templateId, const QString& outputDir);
    QString createFromTemplate(const QString& templateId, const QString& outputDir, const QJsonObject& overrides);

    void addTemplate(const ProjectTemplate& tmpl);
    void removeTemplate(const QString& templateId);

    QStringList getCategories() const;

signals:
    void templateAdded(const ProjectTemplate& tmpl);
    void templateRemoved(const QString& templateId);
    void projectCreated(const QString& projectPath);
    void error(const QString& message);

private:
    void buildBuiltins();
    void copyTemplateFiles(const ProjectTemplate& tmpl, const QString& outputDir);
    void applySettings(QJsonObject& target, const QJsonObject& overrides);

    static ProjectTemplates* s_instance;

    QMap<QString, ProjectTemplate> m_templates;
    QStringList m_categories;
};

class Project {
public:
    Project();
    explicit Project(const QString& path);
    ~Project();

    bool createNew(const QString& path, const QString& templateId = QString());
    bool open(const QString& path);
    bool save();
    bool saveAs(const QString& path);
    void close();

    QString getPath() const { return m_path; }
    QString getName() const { return m_name; }
    void setName(const QString& name) { m_name = name; }

    bool isModified() const { return m_modified; }
    void setModified(bool modified) { m_modified = modified; }

    QString getLastError() const { return m_lastError; }

private:
    QString m_path;
    QString m_name;
    bool m_modified = false;
    QString m_lastError;
    QJsonObject m_manifest;
};

} // namespace ks
