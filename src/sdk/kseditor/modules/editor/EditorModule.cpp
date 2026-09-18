#include "EditorModule.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QDateTime>
#include <QDebug>

namespace ks {

void EditorModule::newProject(const QString& name, const QString& path)
{
    QDir dir(path);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qWarning() << "newProject: cannot create directory" << path;
            return;
        }
    }
    m_projectPath = path + "/" + name + ".ksproj";

    QJsonObject root;
    root["moduleId"] = moduleId();
    root["moduleName"] = moduleName();
    root["version"] = 1;

    QJsonObject project;
    project["name"] = name;
    project["created"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    root["project"] = project;

    saveProject(m_projectPath);
}

void EditorModule::openProject(const QString& projectPath)
{
    QFile f(projectPath);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "openProject: cannot open" << projectPath;
        return;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    f.close();

    if (err.error != QJsonParseError::NoError) {
        qWarning() << "openProject: JSON parse error" << err.errorString();
        return;
    }

    QJsonObject root = doc.object();
    if (root["moduleId"].toString() != moduleId()) {
        qWarning() << "openProject: module mismatch" << root["moduleId"].toString()
                    << "!=" << moduleId();
    }

    m_projectPath = projectPath;
    deserializeProject(root["project"].toObject());
}

void EditorModule::saveProject(const QString& path)
{
    QString savePath = path.isEmpty() ? m_projectPath : path;
    if (savePath.isEmpty()) return;

    QJsonObject root;
    root["moduleId"] = moduleId();
    root["moduleName"] = moduleName();
    root["version"] = 1;
    root["project"] = serializeProject();

    QFile f(savePath);
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning() << "saveProject: cannot write" << savePath;
        return;
    }
    f.write(QJsonDocument(root).toJson());
    f.close();

    m_projectPath = savePath;
}

void EditorModule::saveProjectAs(const QString& path)
{
    saveProject(path);
}

} // namespace ks