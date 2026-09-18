#include "ModManagerEditorModule.h"
#include "ModManager.h"
#include "ContentRepairTool.h"
#include "ContentBrowser.h"
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
#include <QSplitter>
#include <QScrollArea>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QApplication>
#include <QClipboard>
#include <QFileSystemWatcher>
#include <QDirIterator>
#include <QCryptographicHash>

namespace ks {
namespace modmanager {

using ks::ModManagerModule;

ModManagerEditorModule::ModManagerEditorModule(QWidget* parent)
    : ModuleGuiBase(parent)
    , m_tabWidget(nullptr)
    , m_modListTab(nullptr)
    , m_modTree(nullptr)
    , m_modInfoLabel(nullptr)
    , m_modVersionLabel(nullptr)
    , m_modStatusLabel(nullptr)
    , m_installBtn(nullptr)
    , m_uninstallBtn(nullptr)
    , m_enableBtn(nullptr)
    , m_disableBtn(nullptr)
    , m_refreshBtn(nullptr)
    , m_conflictBtn(nullptr)
    , m_modSearchInput(nullptr)
    , m_profilesTab(nullptr)
    , m_profileCombo(nullptr)
    , m_createProfileBtn(nullptr)
    , m_deleteProfileBtn(nullptr)
    , m_profileModsTable(nullptr)
    , m_contentRepairTab(nullptr)
    , m_verifyBtn(nullptr)
    , m_repairBtn(nullptr)
    , m_repairProgress(nullptr)
    , m_repairStatusLabel(nullptr)
    , m_issueTree(nullptr)
    , m_manager(nullptr)
    , m_refreshTimer(nullptr)
{
    setObjectName("ModManagerEditorModule");
}

bool ModManagerEditorModule::initialize() {
    if (m_uiBuilt) return true;
    
    ModuleGuiBase::initialize();
    
    // Get ModManager instance
    m_manager = ModManagerModule::instance();
    if (!m_manager) {
        logError("ModManager not available");
        return false;
    }
    
    // Connect to ModManager signals
    connect(m_manager, &ModManagerModule::modsChanged, this, &ModManagerEditorModule::populateModTree);
    connect(m_manager, &ModManagerModule::modInstalled, this, [this](const QString& name) {
        logSuccess(QString("Mod installed: %1").arg(name));
        populateModTree();
    });
    connect(m_manager, &ModManagerModule::modUninstalled, this, [this](const QString& name) {
        log(QString("Mod uninstalled: %1").arg(name));
        populateModTree();
    });
    connect(m_manager, &ModManagerModule::updatesAvailable, this, [this](int count) {
        logSuccess(QString("%1 updates available").arg(count));
    });
    connect(m_manager, &ModManagerModule::profileChanged, this, [this](const QString& name) {
        logSuccess(QString("Profile switched to: %1").arg(name));
        populateProfiles();
    });
    connect(m_manager, &ModManagerModule::batchInstallProgress, this, [this](int current, int total, const QString& modName) {
        m_repairProgress->setVisible(true);
        m_repairProgress->setMaximum(total);
        m_repairProgress->setValue(current);
        m_repairStatusLabel->setText(QString("Installing %1 (%2/%3)").arg(modName).arg(current).arg(total));
    });
    connect(m_manager, &ModManagerModule::batchInstallFinished, this, [this](int successCount, int failCount) {
        m_repairProgress->setVisible(false);
        logSuccess(QString("Batch install: %1 success, %2 failed").arg(successCount).arg(failCount));
    });
    connect(m_manager, &ModManagerModule::integrityCheckProgress, this, [this](int current, int total, const QString& modName) {
        m_repairProgress->setVisible(true);
        m_repairProgress->setMaximum(total);
        m_repairProgress->setValue(current);
        m_repairStatusLabel->setText(QString("Verifying %1 (%2/%3)").arg(modName).arg(current).arg(total));
    });
    connect(m_manager, &ModManagerModule::integrityCheckFinished, this, [this](int totalChecked, int totalCorrupted) {
        m_repairProgress->setVisible(false);
        logSuccess(QString("Integrity check: %1 checked, %2 corrupted").arg(totalChecked).arg(totalCorrupted));
    });
    connect(m_manager, &ModManagerModule::downloadProgress, this, [this](const QString& modName, int percent) {
        m_repairProgress->setVisible(true);
        m_repairProgress->setMaximum(100);
        m_repairProgress->setValue(percent);
        m_repairStatusLabel->setText(QString("Downloading %1: %2%").arg(modName).arg(percent));
    });
    
    // Auto-refresh timer
    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, &QTimer::timeout, this, &ModManagerEditorModule::onRefreshMods);
    m_refreshTimer->start(30000); // Refresh every 30 seconds
    
    populateModTree();
    populateProfiles();
    return true;
}

void ModManagerEditorModule::shutdown() {
    if (m_refreshTimer) {
        m_refreshTimer->stop();
    }
    m_uiBuilt = false;
}

void ModManagerEditorModule::buildUI() {
    m_tabWidget = new QTabWidget();

    setupModListTab();
    setupProfilesTab();
    setupContentRepairTab();

    m_mainLayout->addWidget(m_tabWidget);
    m_mainLayout->addWidget(m_logOutput);
}

void ModManagerEditorModule::setupModListTab() {
    m_modListTab = new QWidget();
    auto* layout = new QVBoxLayout(m_modListTab);

    auto* searchLayout = new QHBoxLayout();
    m_modSearchInput = new QLineEdit();
    m_modSearchInput->setPlaceholderText("Search mods...");
    searchLayout->addWidget(m_modSearchInput);
    layout->addLayout(searchLayout);

    auto* actionBar = new QHBoxLayout();
    m_installBtn = createButton("Install Mod");
    m_uninstallBtn = createButton("Uninstall");
    m_enableBtn = createButton("Enable");
    m_disableBtn = createButton("Disable");
    m_refreshBtn = createButton("Refresh");
    m_conflictBtn = createButton("Check Conflicts");
    actionBar->addWidget(m_installBtn);
    actionBar->addWidget(m_uninstallBtn);
    actionBar->addWidget(m_enableBtn);
    actionBar->addWidget(m_disableBtn);
    actionBar->addWidget(m_refreshBtn);
    actionBar->addWidget(m_conflictBtn);
    actionBar->addStretch();
    layout->addLayout(actionBar);

    auto* splitter = createSplitter(Qt::Horizontal);
    m_modTree = createTreeWidget({"Mod Name", "Version", "Status", "Size", "Source"});
    splitter->addWidget(m_modTree);

    auto* infoPanel = new QWidget();
    auto* infoLayout = new QVBoxLayout(infoPanel);
    m_modInfoLabel = createLabel("Select a mod to view details");
    m_modVersionLabel = createLabel("");
    m_modStatusLabel = createLabel("");
    infoLayout->addWidget(m_modInfoLabel);
    infoLayout->addWidget(m_modVersionLabel);
    infoLayout->addWidget(m_modStatusLabel);
    infoLayout->addStretch();
    splitter->addWidget(infoPanel);

    layout->addWidget(splitter);

    connect(m_installBtn, &QPushButton::clicked, this, &ModManagerEditorModule::onInstallMod);
    connect(m_uninstallBtn, &QPushButton::clicked, this, &ModManagerEditorModule::onUninstallMod);
    connect(m_enableBtn, &QPushButton::clicked, this, &ModManagerEditorModule::onEnableMod);
    connect(m_disableBtn, &QPushButton::clicked, this, &ModManagerEditorModule::onDisableMod);
    connect(m_refreshBtn, &QPushButton::clicked, this, &ModManagerEditorModule::onRefreshMods);
    connect(m_conflictBtn, &QPushButton::clicked, this, &ModManagerEditorModule::onCheckConflicts);
    connect(m_modTree, &QTreeWidget::itemClicked, this, &ModManagerEditorModule::onModSelected);

    populateModTree();
    m_tabWidget->addTab(m_modListTab, "Mod List");
}

void ModManagerEditorModule::setupProfilesTab() {
    m_profilesTab = new QWidget();
    auto* layout = new QVBoxLayout(m_profilesTab);

    auto* profileBar = new QHBoxLayout();
    m_profileCombo = createComboBox({"Default Profile", "Performance", "Graphics Quality", "VR Setup", "Streaming Setup"});
    m_createProfileBtn = createButton("New Profile");
    m_deleteProfileBtn = createButton("Delete Profile");
    profileBar->addWidget(new QLabel("Profile:"));
    profileBar->addWidget(m_profileCombo);
    profileBar->addWidget(m_createProfileBtn);
    profileBar->addWidget(m_deleteProfileBtn);
    profileBar->addStretch();
    layout->addLayout(profileBar);

    m_profileModsTable = new QTableWidget(0, 4);
    m_profileModsTable->setHorizontalHeaderLabels({"Mod", "Version", "Enabled", "Priority"});
    m_profileModsTable->horizontalHeader()->setStretchLastSection(true);
    m_profileModsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_profileModsTable->setAlternatingRowColors(true);
    layout->addWidget(m_profileModsTable);

    connect(m_profileCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ModManagerEditorModule::onProfileSelected);
    connect(m_createProfileBtn, &QPushButton::clicked, this, &ModManagerEditorModule::onCreateProfile);
    connect(m_deleteProfileBtn, &QPushButton::clicked, this, &ModManagerEditorModule::onDeleteProfile);

    populateProfiles();
    m_tabWidget->addTab(m_profilesTab, "Profiles");
}

void ModManagerEditorModule::setupContentRepairTab() {
    m_contentRepairTab = new QWidget();
    auto* layout = new QVBoxLayout(m_contentRepairTab);

    auto* btnLayout = new QHBoxLayout();
    m_verifyBtn = createButton("Verify Content");
    m_repairBtn = createButton("Repair Content");
    btnLayout->addWidget(m_verifyBtn);
    btnLayout->addWidget(m_repairBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    m_repairProgress = new QProgressBar();
    m_repairProgress->setVisible(false);
    layout->addWidget(m_repairProgress);

    m_repairStatusLabel = createLabel("");
    layout->addWidget(m_repairStatusLabel);

    m_issueTree = createTreeWidget({"Issue", "Mod", "Type", "Severity"});
    layout->addWidget(m_issueTree);

    connect(m_verifyBtn, &QPushButton::clicked, this, &ModManagerEditorModule::onVerifyContent);
    connect(m_repairBtn, &QPushButton::clicked, this, &ModManagerEditorModule::onRepairContent);

    m_tabWidget->addTab(m_contentRepairTab, "Content Repair");
}

void ModManagerEditorModule::populateModTree() {
    m_modTree->clear();
    if (!m_manager) return;

    const auto& mods = m_manager->mods();
    auto* installed = new QTreeWidgetItem(m_modTree, {"Installed Mods (" + QString::number(mods.size()) + ")", "Version", "Status", "Size", "Source"});
    QStringList updateCandidates;
    QStringList conflictNames;
    for (const auto& mod : mods) {
        auto* child = new QTreeWidgetItem();
        child->setText(0, mod.name);
        child->setText(1, mod.version.toString());
        child->setText(2, mod.enabled ? "Enabled" : "Disabled");
        child->setText(3, mod.sizeBytes > 0 ? QString::number(mod.sizeBytes / 1048576.0, 'f', 1) + " MB" : "");
        child->setText(4, mod.isBuiltIn ? "Built-in" : "Manual");
        child->setData(0, Qt::UserRole, mod.name);
        installed->addChild(child);
        if (mod.hasUpdate) updateCandidates << mod.name;
    }

    if (!updateCandidates.isEmpty()) {
        auto* available = new QTreeWidgetItem(m_modTree, {"Available Updates", "", "", "", ""});
        for (const auto& name : updateCandidates) {
            available->addChild(new QTreeWidgetItem({name, "", "Update Available", "", ""}));
        }
    }

    // Conflicts section will be populated when scanForFileConflicts emits scanFinished

    m_modTree->expandAll();
}

void ModManagerEditorModule::populateProfiles() {
    if (!m_manager) return;
    auto* pm = m_manager->profileManager();
    if (!pm) return;

    m_profileCombo->blockSignals(true);
    QString current = m_profileCombo->currentText();
    m_profileCombo->clear();

    QStringList profiles = pm->listProfiles();
    if (profiles.isEmpty()) profiles = {"Default Profile"};
    for (const auto& name : profiles) m_profileCombo->addItem(name);

    int idx = m_profileCombo->findText(current);
    if (idx >= 0) m_profileCombo->setCurrentIndex(idx);
    m_profileCombo->blockSignals(false);

    // Populate mod table from active profile's ModEntry list via manager
    const auto& allMods = m_manager->mods();
    m_profileModsTable->setRowCount(allMods.size());
    for (int row = 0; row < allMods.size(); ++row) {
        const auto& mod = allMods[row];
        m_profileModsTable->setItem(row, 0, new QTableWidgetItem(mod.name));
        m_profileModsTable->setItem(row, 1, new QTableWidgetItem(mod.version.toString()));
        m_profileModsTable->setItem(row, 2, new QTableWidgetItem(mod.enabled ? "Yes" : "No"));
        m_profileModsTable->setItem(row, 3, new QTableWidgetItem(QString::number(row + 1)));
    }
}

void ModManagerEditorModule::onModSelected(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    if (item && item->childCount() == 0 && item->data(0, Qt::UserRole).isValid()) {
        QString modName = item->data(0, Qt::UserRole).toString();
        m_modInfoLabel->setText(QString("Mod: %1").arg(modName));
        m_modVersionLabel->setText(QString("Version: %1").arg(item->text(1)));
        m_modStatusLabel->setText(QString("Status: %1").arg(item->text(2)));
    }
}

void ModManagerEditorModule::onInstallMod() {
    QString path = selectFile("Install Mod Package", "Mod Packages (*.zip *.7z *.rar);;All Files (*)");
    if (!path.isEmpty() && m_manager) {
        importFile(path);
        log(QString("Installing mod from: %1").arg(path));
    }
}

void ModManagerEditorModule::onUninstallMod() {
    auto* item = m_modTree->currentItem();
    if (item && item->childCount() == 0 && item->data(0, Qt::UserRole).isValid() && m_manager) {
        QString modName = item->data(0, Qt::UserRole).toString();
        if (confirmAction("Uninstall Mod", QString("Uninstall '%1'?").arg(modName))) {
            m_manager->uninstallMod(modName);
            logSuccess(QString("Uninstalled mod: %1").arg(modName));
        }
    }
}

void ModManagerEditorModule::onEnableMod() {
    auto* item = m_modTree->currentItem();
    if (item && item->childCount() == 0 && item->data(0, Qt::UserRole).isValid() && m_manager) {
        QString modName = item->data(0, Qt::UserRole).toString();
        m_manager->enableMod(modName);
        item->setText(2, "Enabled");
        log(QString("Enabled mod: %1").arg(modName));
    }
}

void ModManagerEditorModule::onDisableMod() {
    auto* item = m_modTree->currentItem();
    if (item && item->childCount() == 0 && item->data(0, Qt::UserRole).isValid() && m_manager) {
        QString modName = item->data(0, Qt::UserRole).toString();
        m_manager->disableMod(modName);
        item->setText(2, "Disabled");
        log(QString("Disabled mod: %1").arg(modName));
    }
}

void ModManagerEditorModule::onCheckConflicts() {
    if (!m_manager) return;
    log("Scanning for mod conflicts...");
    m_manager->scanForFileConflicts();
    logSuccess("Conflict scan completed (see log for details)");
}

void ModManagerEditorModule::onCreateProfile() {
    if (!m_manager) return;
    bool ok;
    QString name = QInputDialog::getText(this, "New Profile", "Profile name:", QLineEdit::Normal, "", &ok);
    if (ok && !name.isEmpty()) {
        m_manager->createProfile(name);
        m_profileCombo->addItem(name);
        m_profileCombo->setCurrentIndex(m_profileCombo->count() - 1);
        logSuccess(QString("Created profile: %1").arg(name));
    }
}

void ModManagerEditorModule::onDeleteProfile() {
    if (!m_manager) return;
    if (m_profileCombo->count() <= 1) {
        logError("Cannot delete the last profile");
        return;
    }
    QString profile = m_profileCombo->currentText();
    if (confirmAction("Delete Profile", QString("Delete profile '%1'?").arg(profile))) {
        m_manager->deleteProfile(profile);
        m_profileCombo->removeItem(m_profileCombo->currentIndex());
        log(QString("Deleted profile: %1").arg(profile));
    }
}

void ModManagerEditorModule::onProfileSelected(int index) {
    if (!m_manager || index < 0) return;
    QString profile = m_profileCombo->itemText(index);
    m_manager->switchProfile(profile);
    populateProfiles();
    log(QString("Selected profile: %1").arg(profile));
}

void ModManagerEditorModule::onVerifyContent() {
    if (!m_manager) return;
    log("Verifying content integrity...");
    m_repairProgress->setVisible(true);
    m_repairProgress->setRange(0, 100);
    m_repairProgress->setValue(0);
    m_repairStatusLabel->setText("Verifying...");
    m_manager->verifyAllIntegrity();
}

void ModManagerEditorModule::onRepairContent() {
    if (!m_manager) return;
    if (confirmAction("Repair Content", "This will attempt to repair damaged mod files. Continue?")) {
        log("Starting content repair...");
        m_repairProgress->setVisible(true);
        m_repairProgress->setRange(0, 100);
        m_repairProgress->setValue(0);
        m_repairStatusLabel->setText("Repairing...");
        m_manager->repairAllMods();
    }
}

void ModManagerEditorModule::onRefreshMods() {
    if (!m_manager) return;
    log("Refreshing mod list...");
    m_manager->refreshMods();
    populateModTree();
    logSuccess("Mod list refreshed");
}

void ModManagerEditorModule::importFile(const QString& filePath) {
    if (!filePath.isEmpty() && m_manager) {
        m_manager->installMod(filePath);
    } else {
        ModuleGuiBase::importFile(filePath);
    }
}

void ModManagerEditorModule::exportFile(const QString& filePath) {
    ModuleGuiBase::exportFile(filePath);
}

void ModManagerEditorModule::onActivation() {
    ModuleGuiBase::onActivation();
}

void ModManagerEditorModule::onDeactivation() {
    ModuleGuiBase::onDeactivation();
}

} // namespace modmanager
} // namespace ks

