#include "TemplateManager.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDirIterator>
#include <QJsonDocument>
#include <QJsonArray>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDebug>

namespace ks {

// ============================================================================
// DefaultTemplateManager
// ============================================================================

static const char* TEMPLATES_SUBDIR = "kseditor/templates";

class DefaultTemplateManager : public TemplateManager {
public:
    DefaultTemplateManager() = default;
    ~DefaultTemplateManager() override = default;

    void initialize() override
    {
        if (m_initialized) return;

        // Scan built-in templates from resources/templates/
        QStringList searchPaths;
        searchPaths << QCoreApplication::applicationDirPath() + "/../resources/templates";
        searchPaths << QCoreApplication::applicationDirPath() + "/../../resources/templates";
        searchPaths << QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/templates";

        for (const QString& dir : searchPaths) {
            QDir templateDir(dir);
            if (templateDir.exists()) {
                scanTemplates(templateDir.absolutePath());
            }
        }

        m_initialized = true;
        qDebug() << "[TemplateManager] Initialized with" << m_templates.size() << "templates";
    }

    bool isInitialized() const override { return m_initialized; }

    QJsonObject getTemplate(const QString& id) const override
    {
        if (!m_templateData.contains(id)) return {};
        return m_templateData[id];
    }

    QVector<QString> getTemplateIds() const override
    {
        return m_templateData.keys().toVector();
    }

    QVector<QString> getTemplateIdsByType(const QString& type) const override
    {
        QVector<QString> ids;
        for (auto it = m_templateData.constBegin(); it != m_templateData.constEnd(); ++it) {
            if (it.value()["type"].toString() == type) {
                ids << it.key();
            }
        }
        return ids;
    }

    QJsonObject createNewTemplate(const QString& name, const QString& description, const QString& type) override
    {
        Q_UNUSED(name)
        Q_UNUSED(description)
        Q_UNUSED(type)
        return {};
    }

    QString getTemplatePath(const QString& id) const override
    {
        if (m_templatePaths.contains(id)) return m_templatePaths[id];
        return {};
    }

    QString getTemplateDirectory(const QString& id) const override
    {
        QString path = getTemplatePath(id);
        if (path.isEmpty()) return {};
        return QFileInfo(path).absolutePath();
    }

    QString getProjectPathFromTemplate(const QString& templateId, const QString& outputDir) const override
    {
        QJsonObject tmpl = getTemplate(templateId);
        if (tmpl.isEmpty()) return {};

        QString projectName = tmpl["name"].toString();
        QString outputPath = outputDir + "/" + projectName;
        return outputPath;
    }

    bool initializeTemplateProject(const QString& templateId, const QString& projectPath) override
    {
        QJsonObject tmpl = getTemplate(templateId);
        if (tmpl.isEmpty()) return false;

        QString srcDir = getTemplatePath(templateId);
        if (srcDir.isEmpty()) return false;

        QFileInfo fi(srcDir);
        if (!fi.isDir()) {
            srcDir = fi.absolutePath();
        }

        // Create project directory
        QDir().mkpath(projectPath);

        // Copy template files
        return copyDirectory(srcDir, projectPath);
    }

    void setTemplateDirectory(const QString& directory) override
    {
        m_customTemplateDir = directory;
    }

    QString getTemplateDirectory() const override
    {
        if (!m_customTemplateDir.isEmpty()) return m_customTemplateDir;
        return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/templates";
    }

    bool hasTemplate(const QString& id) const override
    {
        return m_templateData.contains(id);
    }

    void removeTemplate(const QString& id) override
    {
        m_templateData.remove(id);
        m_templatePaths.remove(id);
        emit templateRemoved(id);
    }

    bool exportTemplate(const QString& templateId, const QString& outputPath) override
    {
        Q_UNUSED(templateId)
        Q_UNUSED(outputPath)
        return false;
    }

    bool importTemplate(const QString& inputPath) override
    {
        Q_UNUSED(inputPath)
        return false;
    }

    TemplateInfo getTemplateInfo(const QString& id) const override
    {
        TemplateInfo info;
        QJsonObject obj = getTemplate(id);
        if (obj.isEmpty()) return info;

        info.id = id;
        info.name = obj["name"].toString();
        info.description = obj["description"].toString();
        info.type = obj["type"].toString();
        info.version = obj["version"].toString();
        info.author = obj["author"].toString();
        info.difficulty = obj["difficulty"].toInt();
        info.estimatedTime = obj["estimatedTime"].toInt();

        if (obj.contains("created")) {
            info.created = QDateTime::fromString(obj["created"].toString(), Qt::ISODate);
        }
        if (obj.contains("lastModified")) {
            info.lastModified = QDateTime::fromString(obj["lastModified"].toString(), Qt::ISODate);
        }

        QJsonArray modules = obj["modules"].toArray();
        for (const auto& m : modules) {
            info.modules << m.toString();
        }

        QJsonArray files = obj["files"].toArray();
        for (const auto& f : files) {
            info.files << f.toString();
        }

        info.thumbnailPath = obj["thumbnail"].toString();
        info.metadata = obj;

        return info;
    }

    QVector<TemplateInfo> getAllTemplateInfos() const override
    {
        QVector<TemplateInfo> infos;
        for (const QString& id : m_templateData.keys()) {
            infos << getTemplateInfo(id);
        }
        return infos;
    }

private:
    void scanTemplates(const QString& dir)
    {
        QDirIterator it(dir, QDir::Dirs | QDir::NoDotAndDotDot);
        while (it.hasNext()) {
            QString subDir = it.next();
            QString projectFile = subDir + "/project.json";
            if (QFile::exists(projectFile)) {
                loadTemplate(subDir, projectFile);
            }
        }
    }

    void loadTemplate(const QString& dir, const QString& projectFile)
    {
        QFile file(projectFile);
        if (!file.open(QIODevice::ReadOnly)) return;

        QByteArray data = file.readAll();
        file.close();

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError || !doc.isObject()) return;

        QJsonObject obj = doc.object();
        QString id = obj["id"].toString();
        if (id.isEmpty()) {
            id = QFileInfo(dir).fileName();
        }

        m_templateData[id] = obj;
        m_templatePaths[id] = dir;

        emit templateAdded(id);
    }

    bool copyDirectory(const QString& src, const QString& dst) const
    {
        QDir srcDir(src);
        QDir dstDir(dst);

        if (!dstDir.exists()) {
            dstDir.mkpath(".");
        }

        QStringList entries = srcDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& entry : entries) {
            QString srcPath = src + "/" + entry;
            QString dstPath = dst + "/" + entry;

            QFileInfo fi(srcPath);
            if (fi.isDir()) {
                if (!copyDirectory(srcPath, dstPath)) return false;
            } else {
                if (entry == "project.json") continue; // Skip metadata file
                if (!QFile::copy(srcPath, dstPath)) return false;
            }
        }

        return true;
    }

    bool m_initialized = false;
    QString m_customTemplateDir;
    QMap<QString, QJsonObject> m_templateData;
    QMap<QString, QString> m_templatePaths;
};

// ============================================================================
// Static instance
// ============================================================================

TemplateManager* TemplateManager::s_instance = nullptr;

TemplateManager* TemplateManager::instance()
{
    if (!s_instance) s_instance = new DefaultTemplateManager();
    return s_instance;
}

} // namespace ks
