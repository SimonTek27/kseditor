#include "SystemEditorModule.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QTreeWidgetItem>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QStandardPaths>
#include <QMenu>
#include <QAction>
#include <QSettings>
#include <QDesktopServices>

namespace ks {
namespace sys {

SystemEditorModule::SystemEditorModule(QWidget* parent)
    : ModuleGuiBase(parent)
    , m_tabWidget(nullptr)
    , m_modulesTab(nullptr)
    , m_moduleTree(nullptr)
    , m_refreshPluginsBtn(nullptr)
    , m_pluginsTab(nullptr)
    , m_pluginTree(nullptr)
    , m_settingsTab(nullptr)
    , m_settingsTree(nullptr)
    , m_saveSettingsBtn(nullptr)
    , m_loadSettingsBtn(nullptr)
    , m_resetSettingsBtn(nullptr)
    , m_logViewerTab(nullptr)
    , m_logViewer(nullptr)
    , m_logLevelCombo(nullptr)
    , m_clearLogBtn(nullptr)
    , m_viewLogBtn(nullptr)
    , m_taskManagerTab(nullptr)
    , m_taskTree(nullptr)
    , m_taskInfoLabel(nullptr)
{
    setObjectName("SystemEditorModule");
}

bool SystemEditorModule::initialize() {
    if (m_uiBuilt) return true;
    ModuleGuiBase::initialize();
    return true;
}

void SystemEditorModule::shutdown() {
    m_uiBuilt = false;
}

void SystemEditorModule::onActivation() {}
void SystemEditorModule::onDeactivation() {}

void SystemEditorModule::buildUI() {
    m_tabWidget = new QTabWidget();

    setupModulesTab();
    setupPluginsTab();
    setupSettingsTab();
    setupLogViewerTab();
    setupTaskManagerTab();

    m_mainLayout->addWidget(m_tabWidget);
    m_mainLayout->addWidget(m_logOutput);
}

void SystemEditorModule::setupModulesTab() {
    m_modulesTab = new QWidget();
    auto* layout = new QVBoxLayout(m_modulesTab);

    auto* btnLayout = new QHBoxLayout();
    m_refreshPluginsBtn = createButton("Refresh Module List");
    btnLayout->addWidget(m_refreshPluginsBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    m_moduleTree = createTreeWidget({"Module", "ID", "Status", "Priority", "Version"});
    layout->addWidget(m_moduleTree);

    connect(m_refreshPluginsBtn, &QPushButton::clicked, this, &SystemEditorModule::onRefreshPlugins);
    connect(m_moduleTree, &QTreeWidget::itemClicked, this, &SystemEditorModule::onModuleToggled);

    populateModuleList();
    m_tabWidget->addTab(m_modulesTab, "Modules");
}

void SystemEditorModule::setupPluginsTab() {
    m_pluginsTab = new QWidget();
    auto* layout = new QVBoxLayout(m_pluginsTab);

    m_pluginTree = createTreeWidget({"Plugin", "Version", "Vendor", "Enabled"});
    layout->addWidget(m_pluginTree);

    connect(m_pluginTree, &QTreeWidget::itemClicked, this, &SystemEditorModule::onPluginToggled);

    populatePluginList();
    m_tabWidget->addTab(m_pluginsTab, "Plugins");
}

void SystemEditorModule::setupSettingsTab() {
    m_settingsTab = new QWidget();
    auto* layout = new QVBoxLayout(m_settingsTab);
    auto* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);

    auto* container = new QWidget();
    auto* containerLayout = new QVBoxLayout(container);

    auto* generalGroup = createGroupBox("General");
    auto* generalForm = new QFormLayout(generalGroup);
    generalForm->addRow("Language:", createComboBox({"English", "German", "French", "Spanish", "Chinese", "Japanese"}));
    generalForm->addRow("Auto-save interval:", createSpinBox(1, 60, 5, " min"));
    generalForm->addRow("Max recent files:", createSpinBox(5, 50, 20, ""));
    containerLayout->addWidget(generalGroup);

    auto* performanceGroup = createGroupBox("Performance");
    auto* perfForm = new QFormLayout(performanceGroup);
    perfForm->addRow("Thread pool size:", createSpinBox(1, 32, 8, " threads"));
    perfForm->addRow("Enable GPU acceleration:", new QCheckBox("Use Vulkan renderer"));
    perfForm->addRow("Background indexing:", new QCheckBox("Index assets on startup"));
    containerLayout->addWidget(performanceGroup);

    auto* btnLayout = new QHBoxLayout();
    m_saveSettingsBtn = createButton("Save Settings");
    m_loadSettingsBtn = createButton("Load Settings");
    m_resetSettingsBtn = createButton("Reset to Defaults");
    btnLayout->addWidget(m_saveSettingsBtn);
    btnLayout->addWidget(m_loadSettingsBtn);
    btnLayout->addWidget(m_resetSettingsBtn);
    btnLayout->addStretch();
    containerLayout->addLayout(btnLayout);

    containerLayout->addStretch();
    scrollArea->setWidget(container);
    layout->addWidget(scrollArea);

    connect(m_saveSettingsBtn, &QPushButton::clicked, this, &SystemEditorModule::onSaveSettings);
    connect(m_loadSettingsBtn, &QPushButton::clicked, this, &SystemEditorModule::onLoadSettings);
    connect(m_resetSettingsBtn, &QPushButton::clicked, this, &SystemEditorModule::onResetSettings);

    m_tabWidget->addTab(m_settingsTab, "Settings");
}

void SystemEditorModule::setupLogViewerTab() {
    m_logViewerTab = new QWidget();
    auto* layout = new QVBoxLayout(m_logViewerTab);

    auto* toolbar = new QHBoxLayout();
    m_logLevelCombo = createComboBox({"All", "Info", "Warning", "Error", "Debug"});
    m_clearLogBtn = createButton("Clear");
    m_viewLogBtn = createButton("View Log File");
    toolbar->addWidget(new QLabel("Level:"));
    toolbar->addWidget(m_logLevelCombo);
    toolbar->addStretch();
    toolbar->addWidget(m_clearLogBtn);
    toolbar->addWidget(m_viewLogBtn);
    layout->addLayout(toolbar);

    m_logViewer = new QTextEdit();
    m_logViewer->setReadOnly(true);
    m_logViewer->setStyleSheet("QTextEdit { background: #1a1a1a; color: #c8c8c8; font-family: Consolas, monospace; font-size: 9pt; }");
    // Load real log file if available
    QString logPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/kseditor.log";
    QFile logFile(logPath);
    if (logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_logViewer->setPlainText(logFile.readAll());
        logFile.close();
    } else {
        m_logViewer->append("[SYSTEM] Log file not found: " + logPath);
        m_logViewer->append("[SYSTEM] Application logging may not be initialized");
    }
    layout->addWidget(m_logViewer);

    connect(m_clearLogBtn, &QPushButton::clicked, this, &SystemEditorModule::onClearLog);
    connect(m_viewLogBtn, &QPushButton::clicked, this, &SystemEditorModule::onViewLog);

    m_tabWidget->addTab(m_logViewerTab, "Log Viewer");
}

void SystemEditorModule::setupTaskManagerTab() {
    m_taskManagerTab = new QWidget();
    auto* layout = new QVBoxLayout(m_taskManagerTab);

    m_taskTree = createTreeWidget({"Task", "Status", "Progress", "Priority", "Thread"});
    layout->addWidget(m_taskTree);

    m_taskInfoLabel = createLabel("Select a task to view details");
    layout->addWidget(m_taskInfoLabel);

    connect(m_taskTree, &QTreeWidget::itemClicked, this, &SystemEditorModule::onTaskSelected);

    populateTaskList();
    m_tabWidget->addTab(m_taskManagerTab, "Task Manager");
}

void SystemEditorModule::populateModuleList() {
    m_moduleTree->clear();
    m_moduleTree->addTopLevelItem(new QTreeWidgetItem({"Asset Manager", "assets", "Loaded", "10", "1.0"}));
    m_moduleTree->addTopLevelItem(new QTreeWidgetItem({"Audio Editor", "audio", "Loaded", "30", "1.0"}));
    m_moduleTree->addTopLevelItem(new QTreeWidgetItem({"Graphics Viewport", "graphics", "Loaded", "40", "1.0"}));
    m_moduleTree->addTopLevelItem(new QTreeWidgetItem({"Physics Editor", "physics", "Loaded", "35", "1.0"}));
    m_moduleTree->addTopLevelItem(new QTreeWidgetItem({"Mod Manager", "modManager", "Loaded", "15", "1.0"}));
    m_moduleTree->addTopLevelItem(new QTreeWidgetItem({"VR Editor", "vr", "Disabled", "90", "1.0"}));
    m_moduleTree->addTopLevelItem(new QTreeWidgetItem({"Help", "help", "Loaded", "99", "1.0"}));
}

void SystemEditorModule::populatePluginList() {
    m_pluginTree->clear();
    m_pluginTree->addTopLevelItem(new QTreeWidgetItem({"GLTF Importer", "1.2.0", "ksEditor Team", "Yes"}));
    m_pluginTree->addTopLevelItem(new QTreeWidgetItem({"FBX Importer", "1.0.5", "ksEditor Team", "Yes"}));
    m_pluginTree->addTopLevelItem(new QTreeWidgetItem({"Python Scripting", "3.11.0", "Python Foundation", "Yes"}));
    m_pluginTree->addTopLevelItem(new QTreeWidgetItem({"Lua Scripting", "5.4.6", "Lua.org", "Yes"}));
    m_pluginTree->addTopLevelItem(new QTreeWidgetItem({"Steam Workshop", "2.0.0", "Valve", "Yes"}));
    m_pluginTree->addTopLevelItem(new QTreeWidgetItem({"Custom Exporter", "0.9.0", "Community", "No"}));
}

void SystemEditorModule::populateTaskList() {
    m_taskTree->clear();
    m_taskTree->addTopLevelItem(new QTreeWidgetItem({"Asset Indexing", "Running", "45%", "Normal", "Worker 1"}));
    m_taskTree->addTopLevelItem(new QTreeWidgetItem({"Texture Compression", "Queued", "0%", "Low", "â€”"}));
    m_taskTree->addTopLevelItem(new QTreeWidgetItem({"GLTF Import", "Running", "60%", "High", "Worker 2"}));
    m_taskTree->addTopLevelItem(new QTreeWidgetItem({"Cloud Sync", "Idle", "100%", "Normal", "â€”"}));
}

void SystemEditorModule::onModuleToggled(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    if (item) {
        QString status = (item->text(2) == "Loaded") ? "Loaded" : "Disabled";
        log(QString("Module '%1' is %2").arg(item->text(0), status));
    }
}

void SystemEditorModule::onPluginToggled(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    if (item) {
        bool enabled = (item->text(3) == "Yes");
        item->setText(3, enabled ? "No" : "Yes");
        log(QString("Plugin '%1' %2").arg(item->text(0), enabled ? "disabled" : "enabled"));
    }
}

void SystemEditorModule::onSettingChanged() {}

void SystemEditorModule::onSaveSettings() {
    QSettings settings;
    settings.beginGroup("SystemEditor");
    if (!m_settingsTree) {
        logWarning("Settings tree not available, saving defaults");
        settings.setValue("language", 0);
        settings.setValue("autoSaveInterval", 5);
        settings.setValue("maxRecentFiles", 20);
        settings.setValue("threadPoolSize", 8);
        settings.setValue("useVulkan", true);
        settings.setValue("backgroundIndexing", true);
    } else {
        for (int i = 0; i < m_settingsTree->topLevelItemCount(); ++i) {
            auto* item = m_settingsTree->topLevelItem(i);
            for (int j = 1; j < item->columnCount(); ++j)
                settings.setValue(item->text(0) + "/col" + QString::number(j), item->text(j));
        }
    }
    settings.endGroup();
    logSuccess("Settings saved to registry");
}

void SystemEditorModule::onLoadSettings() {
    QSettings settings;
    settings.beginGroup("SystemEditor");
    if (!m_settingsTree) {
        log("Settings tree not available, skipping load");
    } else {
        for (int i = 0; i < m_settingsTree->topLevelItemCount(); ++i) {
            auto* item = m_settingsTree->topLevelItem(i);
            for (int j = 1; j < item->columnCount(); ++j) {
                QString val = settings.value(item->text(0) + "/col" + QString::number(j)).toString();
                if (!val.isEmpty()) item->setText(j, val);
            }
        }
    }
    settings.endGroup();
    logSuccess("Settings loaded from registry");
}

void SystemEditorModule::onResetSettings() {
    if (confirmAction("Reset Settings", "Reset all system settings to defaults?")) {
        QSettings settings;
        settings.remove("SystemEditor");
        // Reload default values
        for (int i = 0; i < m_settingsTree->topLevelItemCount(); ++i) {
            auto* item = m_settingsTree->topLevelItem(i);
            for (int j = 1; j < item->columnCount(); ++j)
                item->setText(j, QString());
        }
        logSuccess("Settings reset to defaults");
    }
}

void SystemEditorModule::onViewLog() {
    QString logPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/kseditor.log";
    QDesktopServices::openUrl(QUrl::fromLocalFile(logPath));
    log(QString("Opening log file: %1").arg(logPath));
}

void SystemEditorModule::onClearLog() {
    m_logViewer->clear();
    log("Log viewer cleared");
}

void SystemEditorModule::onRefreshPlugins() {
    log("Refreshing module list...");
    populateModuleList();
    logSuccess("Module list refreshed");
}

void SystemEditorModule::onTaskSelected(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    if (item) {
        m_taskInfoLabel->setText(QString("Task: %1 | Status: %2 | Progress: %3").arg(item->text(0), item->text(1), item->text(2)));
    }
}

} // namespace sys
} // namespace ks

