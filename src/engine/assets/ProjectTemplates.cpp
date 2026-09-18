#include "ProjectTemplates.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDirIterator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>
#include <QStandardPaths>

namespace ks {

ProjectTemplates* ProjectTemplates::s_instance = nullptr;

ProjectTemplates* ProjectTemplates::instance()
{
    if (!s_instance) { s_instance = new ProjectTemplates(); s_instance->buildBuiltins(); }
    return s_instance;
}

ProjectTemplates::ProjectTemplates(QObject* parent) : QObject(parent) {}
ProjectTemplates::~ProjectTemplates() { s_instance = nullptr; }

void ProjectTemplates::buildBuiltins()
{
    auto add = [this](const QString& id, const QString& name, const QString& desc,
                       const QString& cat) {
        ProjectTemplate t;
        t.id = id; t.name = name; t.description = desc; t.category = cat;
        t.isBuiltIn = true; t.gameVersion = "1.16+";
        m_templates.insert(id, t);
    };
    add("car_road",      "Road Car",             "Street/GT road car template with physics, LODs, sound",  "Car");
    add("car_formula",   "Formula Car",           "Open-wheel formula car with advanced aero setup",       "Car");
    add("car_drift",     "Drift Car",             "Rear-wheel-drive drift car with suspension tuning",     "Car");
    add("car_kart",      "Kart",                  "Single-seater kart with engine and tire setup",         "Car");
    add("car_truck",     "Truck / Van",           "Heavy vehicle template with appropriate physics",       "Car");
    add("track_circuit", "Circuit Track",         "Full racing circuit with AI spline and pit lane",       "Track");
    add("track_hillclimb","Hill Climb",           "Point-to-point hill climb track",                       "Track");
    add("track_drift",   "Drift Track",           "Drift course with banked corners and run-off",          "Track");
    add("track_drag",    "Drag Strip",            "Straight-line drag track with timing zones",            "Track");
    add("showroom",      "Showroom",              "Car showroom/presentation environment",                 "Misc");
}

void ProjectTemplates::loadTemplates()
{
    QString userDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                      + "/templates";
    QDirIterator it(userDir, {"*.json"}, QDir::Files);
    while (it.hasNext()) loadTemplate(it.next());
}

void ProjectTemplates::loadTemplate(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return;
    QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
    ProjectTemplate t;
    t.id           = obj["id"].toString();
    t.name         = obj["name"].toString();
    t.description  = obj["description"].toString();
    t.category     = obj["category"].toString();
    t.gameVersion  = obj["gameVersion"].toString("1.16+");
    t.thumbnailPath = obj["thumbnail"].toString();
    t.isBuiltIn    = false;
    if (!t.id.isEmpty()) m_templates.insert(t.id, t);
}

void ProjectTemplates::addTemplate(const ProjectTemplate& t) { m_templates.insert(t.id, t); emit templateAdded(t); }
void ProjectTemplates::removeTemplate(const QString& id) { m_templates.remove(id); emit templateRemoved(id); }

QVector<ProjectTemplate> ProjectTemplates::getTemplates(const QString& cat) const
{
    QVector<ProjectTemplate> out;
    for (const auto& t : m_templates)
        if (cat.isEmpty() || t.category == cat) out << t;
    return out;
}

QStringList ProjectTemplates::getCategories() const
{
    QStringList cats;
    for (const auto& t : m_templates)
        if (!cats.contains(t.category)) cats << t.category;
    cats.sort();
    return cats;
}

ProjectTemplate ProjectTemplates::getTemplate(const QString& id) const { return m_templates.value(id); }

QString ProjectTemplates::createFromTemplate(const QString& templateId, const QString& outputDir)
{
    return createFromTemplate(templateId, outputDir, QJsonObject());
}

QString ProjectTemplates::createFromTemplate(const QString& templateId,
                                              const QString& outputDir,
                                              const QJsonObject& overrides)
{
    if (!m_templates.contains(templateId)) { emit error("Template not found: " + templateId); return {}; }
    const auto& t = m_templates[templateId];

    QDir dir(outputDir);
    if (!dir.mkpath(".")) { emit error("Cannot create output dir: " + outputDir); return {}; }

    // Create standard subfolder structure
    QStringList subdirs;
    if (t.category == "Car") {
        subdirs << "skins/default" << "sfx" << "driver" << "animations" << "data";
    } else if (t.category == "Track") {
        subdirs << "ai" << "data" << "ui" << "skins" << "models";
    }
    for (const auto& sub : subdirs) dir.mkpath(sub);

    // Write a minimal project manifest
    QJsonObject manifest;
    manifest["templateId"]  = templateId;
    manifest["name"]        = overrides.value("name").toString(t.name);
    manifest["category"]    = t.category;
    manifest["gameVersion"] = t.gameVersion;
    manifest["createdAt"]   = QDateTime::currentDateTime().toString(Qt::ISODate);
    manifest["version"]     = "1.0";

    QFile mf(dir.filePath("kseditor_project.json"));
    if (mf.open(QIODevice::WriteOnly))
        mf.write(QJsonDocument(manifest).toJson());

    // Write ini stubs for AC content
    if (t.category == "Car") {
        QFile ui(dir.filePath("ui/ui_car.json"));
        if (ui.open(QIODevice::WriteOnly)) {
            QJsonObject uiObj;
            uiObj["name"]        = overrides.value("name").toString("My Car");
            uiObj["brand"]       = overrides.value("brand").toString("Custom");
            uiObj["class"]       = overrides.value("class").toString("Street");
            uiObj["tags"]        = QJsonArray();
            uiObj["specs"]       = QJsonObject();
            ui.write(QJsonDocument(uiObj).toJson());
        }
    } else if (t.category == "Track") {
        QFile ui(dir.filePath("ui/ui_track.json"));
        if (ui.open(QIODevice::WriteOnly)) {
            QJsonObject uiObj;
            uiObj["name"]        = overrides.value("name").toString("My Track");
            uiObj["description"] = "";
            uiObj["tags"]        = QJsonArray();
            uiObj["geotags"]     = QJsonArray();
            ui.write(QJsonDocument(uiObj).toJson());
        }
    }

    emit projectCreated(outputDir);
    return outputDir;
}

// ─── Project ──────────────────────────────────────────────────────────────────

bool Project::createNew(const QString& path, const QString& templateId)
{
    return !ProjectTemplates::instance()->createFromTemplate(templateId, path).isEmpty();
}

bool Project::open(const QString& path)
{
    QFile f(QDir(path).filePath("kseditor_project.json"));
    if (!f.open(QIODevice::ReadOnly)) return false;
    m_manifest = QJsonDocument::fromJson(f.readAll()).object();
    m_path = path;
    return true;
}

bool Project::save()
{
    if (m_path.isEmpty()) return false;
    m_manifest["savedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    QFile f(QDir(m_path).filePath("kseditor_project.json"));
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QJsonDocument(m_manifest).toJson());
    return true;
}

} // namespace ks
