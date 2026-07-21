#pragma once

#include "core/editor/ModuleGuiBase.h"
#include <QTabWidget>
#include <QTreeWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QSplitter>
#include <QGroupBox>
#include <QFormLayout>
#include <QScrollArea>
#include <QTextEdit>
#include <QSpinBox>
#include <QLineEdit>

namespace ks {
namespace sys {

class SystemEditorModule : public ModuleGuiBase {
    Q_OBJECT
public:
    explicit SystemEditorModule(QWidget* parent = nullptr);
    ~SystemEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;

    QString moduleName() const override { return "System Configuration"; }
    QString moduleId() const override { return "system"; }
    int getModulePriority() const override { return 5; }

protected:
    void buildUI() override;
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onModuleToggled(QTreeWidgetItem* item, int column);
    void onPluginToggled(QTreeWidgetItem* item, int column);
    void onSettingChanged();
    void onSaveSettings();
    void onLoadSettings();
    void onResetSettings();
    void onViewLog();
    void onClearLog();
    void onRefreshPlugins();
    void onTaskSelected(QTreeWidgetItem* item, int column);

private:
    void setupModulesTab();
    void setupPluginsTab();
    void setupSettingsTab();
    void setupLogViewerTab();
    void setupTaskManagerTab();
    void populateModuleList();
    void populatePluginList();
    void populateTaskList();

    QTabWidget* m_tabWidget = nullptr;

    QWidget* m_modulesTab = nullptr;
    QTreeWidget* m_moduleTree = nullptr;
    QPushButton* m_refreshPluginsBtn = nullptr;

    QWidget* m_pluginsTab = nullptr;
    QTreeWidget* m_pluginTree = nullptr;

    QWidget* m_settingsTab = nullptr;
    QTreeWidget* m_settingsTree = nullptr;
    QPushButton* m_saveSettingsBtn = nullptr;
    QPushButton* m_loadSettingsBtn = nullptr;
    QPushButton* m_resetSettingsBtn = nullptr;

    QWidget* m_logViewerTab = nullptr;
    QTextEdit* m_logViewer = nullptr;
    QComboBox* m_logLevelCombo = nullptr;
    QPushButton* m_clearLogBtn = nullptr;
    QPushButton* m_viewLogBtn = nullptr;

    QWidget* m_taskManagerTab = nullptr;
    QTreeWidget* m_taskTree = nullptr;
    QLabel* m_taskInfoLabel = nullptr;
};

} // namespace sys
} // namespace ks
