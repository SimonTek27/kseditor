#include "SettingsModule.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUuid>

namespace ks {

// ─── Settings ─────────────────────────────────────────────────────────────

static Settings* g_settingsInstance = nullptr;

Settings::Settings(QObject* parent)
    : QObject(parent)
{
    g_settingsInstance = this;
}

Settings::~Settings()
{
    g_settingsInstance = nullptr;
}

void Settings::registerSetting(const SettingDefinition& def)
{
    m_definitions.insert(def.id, def);
    m_values.insert(def.id, def.defaultValue);
    m_defaults.insert(def.id, def.defaultValue);
    if (!m_categories.contains(def.category))
        m_categories.append(def.category);
}

void Settings::unregisterSetting(const QString& settingId)
{
    m_definitions.remove(settingId);
    m_values.remove(settingId);
    m_defaults.remove(settingId);
}

void Settings::setValue(const QString& settingId, const QVariant& value)
{
    if (m_definitions.contains(settingId)) {
        m_values[settingId] = value;
        m_dirty = true;
        emit settingChanged(settingId, value);
    }
}

QVariant Settings::getValue(const QString& settingId) const
{
    return m_values.value(settingId);
}

QVariant Settings::getDefault(const QString& settingId) const
{
    return m_defaults.value(settingId);
}

bool Settings::hasSetting(const QString& settingId) const
{
    return m_definitions.contains(settingId);
}

bool Settings::isDefault(const QString& settingId) const
{
    return m_values.value(settingId) == m_defaults.value(settingId);
}

void Settings::resetToDefaults()
{
    m_values = m_defaults;
    m_dirty = true;
}

void Settings::resetToDefault(const QString& settingId)
{
    if (m_defaults.contains(settingId))
        m_values[settingId] = m_defaults[settingId];
}

void Settings::save()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/settings.json";
    QDir().mkpath(QFileInfo(path).absolutePath());
    saveToFile(path);
}

void Settings::load()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/settings.json";
    loadFromFile(path);
}

bool Settings::saveToFile(const QString& path)
{
    QJsonObject root;
    QJsonObject valuesObj;
    for (auto it = m_values.begin(); it != m_values.end(); ++it) {
        valuesObj[it.key()] = QJsonValue::fromVariant(it.value());
    }
    root["values"] = valuesObj;

    QJsonArray defsArray;
    for (auto it = m_definitions.begin(); it != m_definitions.end(); ++it) {
        QJsonObject defObj;
        defObj["id"] = it.key();
        defObj["category"] = it.value().category;
        defObj["name"] = it.value().name;
        defObj["description"] = it.value().description;
        defObj["defaultValue"] = QJsonValue::fromVariant(it.value().defaultValue);
        defsArray.append(defObj);
    }
    root["definitions"] = defsArray;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(QJsonDocument(root).toJson());
    file.close();
    m_dirty = false;
    return true;
}

bool Settings::loadFromFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) return false;

    QJsonObject root = doc.object();
    QJsonObject valuesObj = root["values"].toObject();
    for (auto it = valuesObj.begin(); it != valuesObj.end(); ++it) {
        m_values[it.key()] = it.value().toVariant();
    }

    m_dirty = false;
    return true;
}

QStringList Settings::getCategories() const
{
    return m_categories;
}

QVector<SettingDefinition> Settings::getSettingsByCategory(const QString& category) const
{
    QVector<SettingDefinition> result;
    for (const auto& def : m_definitions)
        if (def.category == category) result.append(def);
    return result;
}

QVector<SettingDefinition> Settings::getAllSettings() const
{
    return m_definitions.values().toVector();
}

void Settings::setDirty(bool dirty)
{
    m_dirty = dirty;
    emit dirtyChanged(dirty);
}

// ─── SettingsDialog ─────────────────────────────────────────────────────────

SettingsDialog::SettingsDialog(QObject* parent)
    : QObject(parent) {}

SettingsDialog::~SettingsDialog() {}

void SettingsDialog::setSettings(Settings* settings)
{
    m_settings = settings;
}

void SettingsDialog::addSearchFilter(const QString& text)
{
    m_searchText = text;
    emit filterChanged();
}

void SettingsDialog::setCategoryFilter(const QString& category)
{
    m_categoryFilter = category;
    emit filterChanged();
}

void SettingsDialog::setAdvancedFilter(bool showAdvanced)
{
    m_showAdvanced = showAdvanced;
    emit filterChanged();
}

QVector<SettingDefinition> SettingsDialog::getVisibleSettings() const
{
    if (!m_settings) return {};
    auto all = m_settings->getAllSettings();
    QVector<SettingDefinition> result;
    for (const auto& def : all) {
        if (def.isHidden) continue;
        if (!m_showAdvanced && def.isAdvanced) continue;
        if (!m_categoryFilter.isEmpty() && def.category != m_categoryFilter) continue;
        if (!m_searchText.isEmpty() && !def.name.contains(m_searchText, Qt::CaseInsensitive)) continue;
        result.append(def);
    }
    return result;
}

void SettingsDialog::search(const QString& query)
{
    m_searchText = query;
    emit filterChanged();
}

// ─── SettingsBackup ─────────────────────────────────────────────────────────

SettingsBackup::SettingsBackup(QObject* parent)
    : QObject(parent)
{
    m_backupDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/settings_backup";
    QDir().mkpath(m_backupDir);
}

SettingsBackup::~SettingsBackup() {}

void SettingsBackup::setSettings(Settings* settings)
{
    m_settings = settings;
}

void SettingsBackup::setMaxBackups(int max)
{
    m_maxBackups = qMax(1, max);
}

void SettingsBackup::createBackup()
{
    if (!m_settings) return;
    QString path = m_backupDir + "/" + generateBackupName() + ".json";
    m_settings->saveToFile(path);
    emit backupCreated(path);
}

void SettingsBackup::restoreBackup(int index)
{
    QString backupPath = getBackupPath(index);
    if (backupPath.isEmpty() || !m_settings) return;

    QFile file(backupPath);
    if (!file.open(QIODevice::ReadOnly)) return;

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseErr);
    if (parseErr.error != QJsonParseError::NoError) return;

    QJsonObject obj = doc.object();
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        m_settings->setValue(it.key(), it.value().toVariant());
    }

    emit backupRestored(backupPath);
}

void SettingsBackup::restoreLatest()
{
    auto list = getBackupList();
    if (!list.isEmpty()) {
        restoreBackup(0);
    }
}

QStringList SettingsBackup::getBackupList() const
{
    QDir dir(m_backupDir);
    return dir.entryList(QStringList{"*.json"}, QDir::Files, QDir::Time);
}

QString SettingsBackup::getBackupPath(int index) const
{
    auto list = getBackupList();
    if (index >= 0 && index < list.size())
        return m_backupDir + "/" + list[index];
    return {};
}

void SettingsBackup::clearBackups()
{
    QDir dir(m_backupDir);
    for (const auto& f : dir.entryList(QStringList{"*.json"}, QDir::Files))
        dir.remove(f);
    emit backupCleared();
}

QString SettingsBackup::generateBackupName() const
{
    return QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
}

// ─── Workspace ──────────────────────────────────────────────────────────────

Workspace::Workspace(QObject* parent)
    : QObject(parent) {}

Workspace::~Workspace() {}

void Workspace::setName(const QString& name)
{
    m_name = name;
    emit nameChanged(name);
}

void Workspace::setPath(const QString& path)
{
    m_path = path;
}

void Workspace::setSettings(const QJsonObject& settings)
{
    m_workspaceSettings.windowGeometry = settings;
    emit settingsChanged();
}

void Workspace::setLastFile(const QString& path)
{
    m_workspaceSettings.lastOpenedFile = path;
}

void Workspace::addRecentFile(const QString& path)
{
    if (!m_workspaceSettings.recentFiles.contains(path)) {
        m_workspaceSettings.recentFiles.prepend(path);
        if (m_workspaceSettings.recentFiles.size() > 10)
            m_workspaceSettings.recentFiles.removeLast();
    }
    emit recentFilesChanged();
}

bool Workspace::save()
{
    if (m_path.isEmpty()) return false;

    QJsonObject root;
    root["name"] = m_name;
    root["id"] = m_id;

    QJsonObject settings;
    settings["windowGeometry"] = m_workspaceSettings.windowGeometry;
    settings["panels"] = m_workspaceSettings.panels;
    settings["shortcuts"] = m_workspaceSettings.shortcuts;
    settings["lastOpenedFile"] = m_workspaceSettings.lastOpenedFile;
    settings["recentFiles"] = QJsonArray::fromStringList(m_workspaceSettings.recentFiles);
    root["settings"] = settings;

    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QFile file(m_path);
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(QJsonDocument(root).toJson());
    file.close();
    return true;
}

bool Workspace::load()
{
    if (m_path.isEmpty()) return false;

    QFile file(m_path);
    if (!file.exists()) return false;
    if (!file.open(QIODevice::ReadOnly)) return false;

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseErr);
    if (parseErr.error != QJsonParseError::NoError) return false;

    QJsonObject root = doc.object();
    m_name = root["name"].toString(m_name);

    QJsonObject settings = root["settings"].toObject();
    m_workspaceSettings.windowGeometry = settings["windowGeometry"].toObject();
    m_workspaceSettings.panels = settings["panels"].toObject();
    m_workspaceSettings.shortcuts = settings["shortcuts"].toObject();
    m_workspaceSettings.lastOpenedFile = settings["lastOpenedFile"].toString();

    QJsonArray recentArr = settings["recentFiles"].toArray();
    m_workspaceSettings.recentFiles.clear();
    for (const auto& v : recentArr)
        m_workspaceSettings.recentFiles.append(v.toString());

    return true;
}

bool Workspace::exportToFile(const QString& path)
{
    QString origPath = m_path;
    m_path = path;
    bool ok = save();
    m_path = origPath;
    return ok;
}

bool Workspace::importFromFile(const QString& path)
{
    QString origPath = m_path;
    m_path = path;
    bool ok = load();
    m_path = origPath;
    if (ok) emit settingsChanged();
    return ok;
}

// ─── WorkspaceManager ───────────────────────────────────────────────────────

WorkspaceManager::WorkspaceManager(QObject* parent)
    : QObject(parent) {}

WorkspaceManager::~WorkspaceManager()
{
    qDeleteAll(m_workspaces);
    m_workspaces.clear();
}

void WorkspaceManager::setDefaultWorkspace(const QString& id)
{
    m_defaultId = id;
}

QString WorkspaceManager::createWorkspace(const QString& name)
{
    auto* ws = new Workspace(this);
    ws->setName(name);
    QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    ws->setObjectName(id);
    m_workspaces.insert(id, ws);
    emit workspaceAdded(id);
    return id;
}

void WorkspaceManager::removeWorkspace(const QString& id)
{
    if (m_workspaces.contains(id)) {
        delete m_workspaces.take(id);
        emit workspaceRemoved(id);
    }
}

void WorkspaceManager::renameWorkspace(const QString& id, const QString& newName)
{
    if (auto* ws = m_workspaces.value(id)) {
        ws->setName(newName);
        emit workspaceChanged(id);
    }
}

Workspace* WorkspaceManager::getWorkspace(const QString& id) const
{
    return m_workspaces.value(id);
}

void WorkspaceManager::setCurrentWorkspace(const QString& id)
{
    m_currentWorkspace = m_workspaces.value(id);
    emit currentWorkspaceChanged(id);
}

} // namespace ks
