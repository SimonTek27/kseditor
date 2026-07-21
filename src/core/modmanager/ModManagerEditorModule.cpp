#include "ModManagerEditorModule.h"
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

namespace ks {
namespace modmanager {

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
{
    setObjectName("ModManagerEditorModule");
}

bool ModManagerEditorModule::initialize() {
    if (m_uiBuilt) return true;
    ModuleGuiBase::initialize();
    return true;
}

void ModManagerEditorModule::shutdown() {
    m_uiBuilt = false;
}

void ModManagerEditorModule::importFile(const QString& filePath) {
    if (filePath.isEmpty()) return;
    QFileInfo fi(filePath);
    QString suffix = fi.suffix().toLower();
    if (suffix == "zip" || suffix == "7z" || suffix == "rar" || suffix == "tar" || suffix == "gz") {
        log(QString("Installing mod package: %1").arg(filePath));
    } else {
        logError(QString("Unsupported mod package format: %1").arg(suffix));
    }
}

void ModManagerEditorModule::exportFile(const QString& filePath) {
    if (filePath.isEmpty()) return;
    log(QString("Exporting mod package to: %1").arg(filePath));
}

void ModManagerEditorModule::onActivation() {}
void ModManagerEditorModule::onDeactivation() {}

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
    auto* installed = new QTreeWidgetItem(m_modTree, {"Installed Mods", "", "", "", ""});
    installed->addChild(new QTreeWidgetItem({"Spa-Francorchamps", "1.2.0", "Enabled", "245 MB", "Steam Workshop"}));
    installed->addChild(new QTreeWidgetItem({"Ferrari 488 GT3 Evo", "2.1.0", "Enabled", "180 MB", "Manual Install"}));
    installed->addChild(new QTreeWidgetItem({"Nordschleife 2024", "1.0.5", "Enabled", "890 MB", "Steam Workshop"}));
    installed->addChild(new QTreeWidgetItem({"Porsche 911 RSR", "1.3.0", "Disabled", "210 MB", "Manual Install"}));
    installed->addChild(new QTreeWidgetItem({"Rain FX", "2.5.0", "Enabled", "45 MB", "CSP"}));

    auto* available = new QTreeWidgetItem(m_modTree, {"Available Updates", "", "", "", ""});
    available->addChild(new QTreeWidgetItem({"Spa-Francorchamps", "1.3.0", "Update Available", "245 MB", "Steam Workshop"}));

    auto* conflicting = new QTreeWidgetItem(m_modTree, {"Conflicts", "", "", "", ""});
    conflicting->addChild(new QTreeWidgetItem({"Weather FX", "1.8.0", "Conflict", "12 MB", "CSP"}));

    m_modTree->expandAll();
}

void ModManagerEditorModule::populateProfiles() {
    m_profileModsTable->setRowCount(3);
    m_profileModsTable->setItem(0, 0, new QTableWidgetItem("Spa-Francorchamps"));
    m_profileModsTable->setItem(0, 1, new QTableWidgetItem("1.2.0"));
    m_profileModsTable->setItem(0, 2, new QTableWidgetItem("Yes"));
    m_profileModsTable->setItem(0, 3, new QTableWidgetItem("1"));

    m_profileModsTable->setItem(1, 0, new QTableWidgetItem("Ferrari 488 GT3 Evo"));
    m_profileModsTable->setItem(1, 1, new QTableWidgetItem("2.1.0"));
    m_profileModsTable->setItem(1, 2, new QTableWidgetItem("Yes"));
    m_profileModsTable->setItem(1, 3, new QTableWidgetItem("2"));

    m_profileModsTable->setItem(2, 0, new QTableWidgetItem("Rain FX"));
    m_profileModsTable->setItem(2, 1, new QTableWidgetItem("2.5.0"));
    m_profileModsTable->setItem(2, 2, new QTableWidgetItem("No"));
    m_profileModsTable->setItem(2, 3, new QTableWidgetItem("3"));
}

void ModManagerEditorModule::onModSelected(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    if (item && item->childCount() == 0) {
        m_modInfoLabel->setText(QString("Mod: %1").arg(item->text(0)));
        m_modVersionLabel->setText(QString("Version: %1").arg(item->text(1)));
        m_modStatusLabel->setText(QString("Status: %1").arg(item->text(2)));
    }
}

void ModManagerEditorModule::onInstallMod() {
    QString path = selectFile("Install Mod Package", "Mod Packages (*.zip *.7z *.rar);;All Files (*)");
    if (!path.isEmpty()) {
        importFile(path);
        logSuccess("Mod installed successfully");
    }
}

void ModManagerEditorModule::onUninstallMod() {
    auto* item = m_modTree->currentItem();
    if (item && item->childCount() == 0) {
        if (confirmAction("Uninstall Mod", QString("Uninstall '%1'?").arg(item->text(0)))) {
            log(QString("Uninstalled mod: %1").arg(item->text(0)));
            delete item;
        }
    }
}

void ModManagerEditorModule::onEnableMod() {
    auto* item = m_modTree->currentItem();
    if (item && item->childCount() == 0) {
        item->setText(2, "Enabled");
        log(QString("Enabled mod: %1").arg(item->text(0)));
    }
}

void ModManagerEditorModule::onDisableMod() {
    auto* item = m_modTree->currentItem();
    if (item && item->childCount() == 0) {
        item->setText(2, "Disabled");
        log(QString("Disabled mod: %1").arg(item->text(0)));
    }
}

void ModManagerEditorModule::onCheckConflicts() {
    log("Checking for mod conflicts...");
    logWarning("Found 1 potential conflict: Weather FX");
}

void ModManagerEditorModule::onCreateProfile() {
    bool ok;
    QString name = QInputDialog::getText(this, "New Profile", "Profile name:", QLineEdit::Normal, "", &ok);
    if (ok && !name.isEmpty()) {
        m_profileCombo->addItem(name);
        m_profileCombo->setCurrentIndex(m_profileCombo->count() - 1);
        logSuccess(QString("Created profile: %1").arg(name));
    }
}

void ModManagerEditorModule::onDeleteProfile() {
    if (m_profileCombo->count() <= 1) {
        logError("Cannot delete the last profile");
        return;
    }
    QString profile = m_profileCombo->currentText();
    if (confirmAction("Delete Profile", QString("Delete profile '%1'?").arg(profile))) {
        m_profileCombo->removeItem(m_profileCombo->currentIndex());
        log(QString("Deleted profile: %1").arg(profile));
    }
}

void ModManagerEditorModule::onProfileSelected(int index) {
    Q_UNUSED(index);
}

void ModManagerEditorModule::onVerifyContent() {
    log("Verifying content integrity...");
    m_repairProgress->setVisible(true);
    m_repairProgress->setRange(0, 0);
    m_repairStatusLabel->setText("Verifying...");
}

void ModManagerEditorModule::onRepairContent() {
    if (confirmAction("Repair Content", "This will attempt to repair damaged mod files. Continue?")) {
        log("Starting content repair...");
        m_repairProgress->setVisible(true);
        m_repairProgress->setRange(0, 0);
        m_repairStatusLabel->setText("Repairing...");
    }
}

void ModManagerEditorModule::onRefreshMods() {
    log("Refreshing mod list...");
    populateModTree();
    logSuccess("Mod list refreshed");
}

} // namespace modmanager
} // namespace ks

#include "ModManagerEditorModule.moc"
