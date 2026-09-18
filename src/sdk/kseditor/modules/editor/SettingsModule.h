#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QVariant>
#include <QJsonObject>
#include <QSettings>
#include <QVector>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QDebug>

namespace ks {

class IniHandler : public QObject {
    Q_OBJECT
public:
    explicit IniHandler(QObject* parent = nullptr);

    Q_INVOKABLE QVariantMap read(const QString& path);
    Q_INVOKABLE QVariantMap readDir(const QString& dirPath);
    Q_INVOKABLE void write(const QString& path, const QVariantMap& data);
    Q_INVOKABLE void writeFromResource(const QString& dirPath, const QVariantMap& resources);

private:
    QVariantMap parseIni(const QString& path);
    void writeSection(QTextStream& out, const QString& section, const QVariantMap& data);
};

enum class SettingType {
    Bool,
    Int,
    Double,
    String,
    StringList,
    Color,
    Path,
    Enum,
    Group
};

struct SettingDefinition {
    QString id;
    QString name;
    QString category;
    SettingType type;
    QVariant defaultValue;
    QVariant min;
    QVariant max;
    QStringList enumValues;
    QString description;
    bool isAdvanced = false;
    bool isHidden = false;
};

class Settings : public QObject
{
    Q_OBJECT

public:
    explicit Settings(QObject* parent = nullptr);
    ~Settings();

    void registerSetting(const SettingDefinition& def);
    void unregisterSetting(const QString& settingId);

    void setValue(const QString& settingId, const QVariant& value);
    QVariant getValue(const QString& settingId) const;
    QVariant getDefault(const QString& settingId) const;

    bool hasSetting(const QString& settingId) const;
    bool isDefault(const QString& settingId) const;

    void resetToDefaults();
    void resetToDefault(const QString& settingId);

    void save();
    void load();

    bool saveToFile(const QString& path);
    bool loadFromFile(const QString& path);

    QStringList getCategories() const;
    QVector<SettingDefinition> getSettingsByCategory(const QString& category) const;
    QVector<SettingDefinition> getAllSettings() const;

    void setDirty(bool dirty);
    bool isDirty() const { return m_dirty; }

signals:
    void settingChanged(const QString& settingId, const QVariant& value);
    void settingsReset();
    void dirtyChanged(bool dirty);

private:
    QStringList m_categories;
    QMap<QString, SettingDefinition> m_definitions;
    QMap<QString, QVariant> m_values;
    QMap<QString, QVariant> m_defaults;
    bool m_dirty = false;
};

class SettingsDialog : public QObject
{
    Q_OBJECT

public:
    explicit SettingsDialog(QObject* parent = nullptr);
    ~SettingsDialog();

    void setSettings(Settings* settings);

    void addSearchFilter(const QString& text);
    void setCategoryFilter(const QString& category = QString());
    void setAdvancedFilter(bool showAdvanced);

    QVector<SettingDefinition> getVisibleSettings() const;

    void search(const QString& query);

signals:
    void filterChanged();
    void requested(const QString& settingId);

private:
    Settings* m_settings = nullptr;
    QString m_searchText;
    QString m_categoryFilter;
    bool m_showAdvanced = false;
};

class SettingsBackup : public QObject
{
    Q_OBJECT

public:
    explicit SettingsBackup(QObject* parent = nullptr);
    ~SettingsBackup();

    void setSettings(Settings* settings);

    void setMaxBackups(int max);
    int getMaxBackups() const { return m_maxBackups; }

    void createBackup();
    void restoreBackup(int index);
    void restoreLatest();

    QStringList getBackupList() const;
    QString getBackupPath(int index) const;

    void clearBackups();

signals:
    void backupCreated(const QString& path);
    void backupRestored(const QString& path);
    void backupCleared();

private:
    QString generateBackupName() const;

    Settings* m_settings = nullptr;
    int m_maxBackups = 10;
    QString m_backupDir;
};

class Workspace : public QObject
{
    Q_OBJECT

public:
    explicit Workspace(QObject* parent = nullptr);
    ~Workspace();

    struct WorkspaceSettings {
        QString name;
        QJsonObject windowGeometry;
        QJsonObject panels;
        QJsonObject shortcuts;
        QString lastOpenedFile;
        QStringList recentFiles;
    };

    QString getId() const { return m_id; }
    void setName(const QString& name);

    QString getPath() const { return m_path; }
    void setPath(const QString& path);

    QJsonObject getSettings() const { return m_workspaceSettings.windowGeometry; }
    void setSettings(const QJsonObject& settings);

    QString getLastFile() const { return m_workspaceSettings.lastOpenedFile; }
    void setLastFile(const QString& path);

    QStringList getRecentFiles() const { return m_workspaceSettings.recentFiles; }
    void addRecentFile(const QString& path);

    bool save();
    bool load();
    bool exportToFile(const QString& path);
    bool importFromFile(const QString& path);

signals:
    void nameChanged(const QString& name);
    void settingsChanged();
    void recentFilesChanged();

private:
    QString m_id;
    QString m_name;
    QString m_path;
    WorkspaceSettings m_workspaceSettings;
};

class WorkspaceManager : public QObject
{
    Q_OBJECT

public:
    explicit WorkspaceManager(QObject* parent = nullptr);
    ~WorkspaceManager();

    void setDefaultWorkspace(const QString& id);
    QString getDefaultWorkspaceId() const { return m_defaultId; }

    QString createWorkspace(const QString& name);
    void removeWorkspace(const QString& id);
    void renameWorkspace(const QString& id, const QString& newName);

    Workspace* getWorkspace(const QString& id) const;
    QVector<Workspace*> getWorkspaces() const { return m_workspaces.values(); }

    Workspace* getCurrentWorkspace() const { return m_currentWorkspace; }
    void setCurrentWorkspace(const QString& id);

signals:
    void workspaceAdded(const QString& id);
    void workspaceRemoved(const QString& id);
    void workspaceChanged(const QString& id);
    void currentWorkspaceChanged(const QString& id);

private:
    QString m_defaultId;
    QMap<QString, Workspace*> m_workspaces;
    Workspace* m_currentWorkspace = nullptr;
};

} // namespace ks
