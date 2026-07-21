#include "WorkshopEditorModule.h"
#include "WorkshopManager.h"
#include "core/editor/ModuleGuiBase.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QTreeWidgetItem>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QStandardPaths>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QThread>
#include <QProgressDialog>
#include <QMenu>
#include <QAction>
#include <QDesktopServices>
#include <QUrl>
#include <QRandomGenerator>
#include <QClipboard>
#include <QApplication>

namespace ks {

WorkshopEditorModule::WorkshopEditorModule(QWidget* parent)
    : ModuleGuiBase(parent)
    , m_manager(nullptr)
    , m_tabWidget(nullptr)
    , m_browseTab(nullptr)
    , m_categoryCombo(nullptr)
    , m_searchEdit(nullptr)
    , m_refreshBtn(nullptr)
    , m_browseTree(nullptr)
    , m_itemDetails(nullptr)
    , m_installBtn(nullptr)
    , m_updateBtn(nullptr)
    , m_openBrowserBtn(nullptr)
    , m_resolveDepsBtn(nullptr)
    , m_rateBtn(nullptr)
    , m_installedTab(nullptr)
    , m_installedTree(nullptr)
    , m_uninstallBtn(nullptr)
    , m_checkUpdatesBtn(nullptr)
    , m_openFolderBtn(nullptr)
    , m_publishTab(nullptr)
    , m_pubNameEdit(nullptr)
    , m_pubVersionEdit(nullptr)
    , m_pubAuthorEdit(nullptr)
    , m_pubCategoryCombo(nullptr)
    , m_pubDescEdit(nullptr)
    , m_pubTagsEdit(nullptr)
    , m_pubSourcePathEdit(nullptr)
    , m_pubBrowseBtn(nullptr)
    , m_createModBtn(nullptr)
    , m_publishBtn(nullptr)
    , m_pubDepsEdit(nullptr)
    , m_pubConflictsEdit(nullptr)
    , m_pubLicenseEdit(nullptr)
    , m_pubWebsiteEdit(nullptr)
    , m_profilesTab(nullptr)
    , m_profileCombo(nullptr)
    , m_createProfileBtn(nullptr)
    , m_deleteProfileBtn(nullptr)
    , m_activateProfileBtn(nullptr)
    , m_saveProfileBtn(nullptr)
    , m_profileItemsTree(nullptr)
    , m_profileDesc(nullptr)
    , m_lastSelectedItem(nullptr)
    , m_hasSelection(false)
{
    setObjectName("WorkshopEditorModule");
}

bool WorkshopEditorModule::initialize() {
    if (m_uiBuilt) return true;
    
    ModuleGuiBase::initialize();
    
    m_manager = WorkshopManager::instance();
    if (!m_manager) {
        logError("WorkshopManager not available");
        return false;
    }
    
    loadItems();
    refreshProfiles();
    return true;
}

void WorkshopEditorModule::shutdown() {
    ModuleGuiBase::shutdown();
}

void WorkshopEditorModule::importFile(const QString& filePath) {
    QFileInfo info(filePath);
    if (info.suffix().toLower() == "ksmod" || info.suffix().toLower() == "zip") {
        log(QString("Importing mod package: %1").arg(filePath));
        // In real implementation, would call m_manager->installPackage()
    } else {
        logError("Unsupported file format for import");
    }
}

void WorkshopEditorModule::exportFile(const QString& filePath) {
    if (!m_hasSelection) {
        logError("No item selected for export");
        return;
    }
    log(QString("Exporting mod: %1 to %2").arg(m_lastSelectedWorkshopItem.name, filePath));
    // In real implementation, would call m_manager->createWorkshopPackage()
}

void WorkshopEditorModule::buildUI() {
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #3a3a3a; background: #1e1e1e; }"
        "QTabBar::tab { background: #2d2d2d; color: #aaa; padding: 8px 16px; border: 1px solid #3a3a3a; border-bottom: none; }"
        "QTabBar::tab:selected { background: #3a5a8a; color: #fff; }"
        "QTabBar::tab:hover { background: #4a6a9a; }"
    );
    
    setupBrowseTab();
    setupInstalledTab();
    setupPublishTab();
    setupProfilesTab();
    
    m_tabWidget->addTab(m_browseTab, "Browse");
    m_tabWidget->addTab(m_installedTab, "Installed");
    m_tabWidget->addTab(m_publishTab, "Publish");
    m_tabWidget->addTab(m_profilesTab, "Profiles");
    
    m_mainLayout->insertWidget(1, m_tabWidget, 1);
}

void WorkshopEditorModule::setupBrowseTab() {
    m_browseTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_browseTab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);
    
    // Toolbar
    QGroupBox* toolGroup = createGroupBox("Browse Workshop");
    QHBoxLayout* toolLayout = new QHBoxLayout(toolGroup);
    
    toolLayout->addWidget(createLabel("Category:"));
    m_categoryCombo = createComboBox(QStringList{"All"} + WorkshopItem::standardCategories());
    m_categoryCombo->setMaximumWidth(150);
    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &WorkshopEditorModule::onCategoryChanged);
    toolLayout->addWidget(m_categoryCombo);
    
    toolLayout->addWidget(createLabel("Search:"));
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("Search mods...");
    m_searchEdit->setMaximumWidth(200);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &WorkshopEditorModule::onSearchTextChanged);
    toolLayout->addWidget(m_searchEdit);
    
    m_refreshBtn = createButton("Refresh");
    connect(m_refreshBtn, &QPushButton::clicked, this, &WorkshopEditorModule::onRefreshClicked);
    toolLayout->addWidget(m_refreshBtn);
    
    toolLayout->addStretch();
    layout->addWidget(toolGroup);
    
    // Main content splitter
    QSplitter* splitter = createSplitter();
    
    // Browse tree
    QGroupBox* browseGroup = createGroupBox("Available Mods");
    QVBoxLayout* browseLayout = new QVBoxLayout(browseGroup);
    
    m_browseTree = createTreeWidget({"Name", "Version", "Author", "Category", "Rating", "Downloads", "Updated", "Status"});
    m_browseTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_browseTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_browseTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_browseTree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_browseTree->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_browseTree->header()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_browseTree->header()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_browseTree->header()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    m_browseTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_browseTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_browseTree, &QTreeWidget::itemDoubleClicked, this, &WorkshopEditorModule::onItemDoubleClicked);
    connect(m_browseTree, &QTreeWidget::itemSelectionChanged, this, &WorkshopEditorModule::onItemSelectionChanged);
    connect(m_browseTree, &QTreeWidget::customContextMenuRequested, this, &WorkshopEditorModule::onShowContextMenu);
    browseLayout->addWidget(m_browseTree);
    
    splitter->addWidget(browseGroup);
    
    // Details panel
    QGroupBox* detailsGroup = createGroupBox("Mod Details");
    QVBoxLayout* detailsLayout = new QVBoxLayout(detailsGroup);
    
    m_itemDetails = new QTextEdit();
    m_itemDetails->setReadOnly(true);
    m_itemDetails->setStyleSheet("QTextEdit { background: #1a1a1a; color: #c8c8c8; font-family: Consolas; font-size: 10px; border: 1px solid #3a3a3a; }");
    detailsLayout->addWidget(m_itemDetails);
    
    // Action buttons
    QHBoxLayout* actionLayout = new QHBoxLayout();
    m_installBtn = createButton("Install", "success");
    connect(m_installBtn, &QPushButton::clicked, this, &WorkshopEditorModule::onInstallClicked);
    actionLayout->addWidget(m_installBtn);
    
    m_updateBtn = createButton("Update", "primary");
    connect(m_updateBtn, &QPushButton::clicked, this, &WorkshopEditorModule::onUpdateClicked);
    actionLayout->addWidget(m_updateBtn);
    
    m_openBrowserBtn = createButton("Open in Browser");
    connect(m_openBrowserBtn, &QPushButton::clicked, this, &WorkshopEditorModule::onOpenInBrowserClicked);
    actionLayout->addWidget(m_openBrowserBtn);
    
    m_resolveDepsBtn = createButton("Resolve Dependencies");
    connect(m_resolveDepsBtn, &QPushButton::clicked, this, &WorkshopEditorModule::onResolveDepsClicked);
    actionLayout->addWidget(m_resolveDepsBtn);
    
    m_rateBtn = createButton("Rate");
    connect(m_rateBtn, &QPushButton::clicked, this, &WorkshopEditorModule::onRateClicked);
    actionLayout->addWidget(m_rateBtn);
    
    actionLayout->addStretch();
    detailsLayout->addLayout(actionLayout);
    
    splitter->addWidget(detailsGroup);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);
    
    layout->addWidget(splitter, 1);
}

void WorkshopEditorModule::setupInstalledTab() {
    m_installedTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_installedTab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);
    
    QGroupBox* installedGroup = createGroupBox("Installed Mods");
    QVBoxLayout* installedLayout = new QVBoxLayout(installedGroup);
    
    m_installedTree = createTreeWidget({"Name", "Version", "Category", "Status", "Path", "Size", "Installed", "Enabled"});
    m_installedTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_installedTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_installedTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_installedTree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_installedTree->header()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_installedTree->header()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_installedTree->header()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_installedTree->header()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    m_installedTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_installedTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_installedTree, &QTreeWidget::customContextMenuRequested, this, &WorkshopEditorModule::onShowContextMenu);
    installedLayout->addWidget(m_installedTree);
    
    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_uninstallBtn = createButton("Uninstall", "danger");
    connect(m_uninstallBtn, &QPushButton::clicked, this, &WorkshopEditorModule::onUninstallClicked);
    btnLayout->addWidget(m_uninstallBtn);
    
    m_checkUpdatesBtn = createButton("Check Updates");
    connect(m_checkUpdatesBtn, &QPushButton::clicked, this, &WorkshopEditorModule::onCheckUpdatesClicked);
    btnLayout->addWidget(m_checkUpdatesBtn);
    
    m_openFolderBtn = createButton("Open Folder");
    connect(m_openFolderBtn, &QPushButton::clicked, this, [this]() {
        if (m_hasSelection && !m_lastSelectedWorkshopItem.packagePath.isEmpty()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_lastSelectedWorkshopItem.packagePath).absolutePath()));
        }
    });
    btnLayout->addWidget(m_openFolderBtn);
    
    btnLayout->addStretch();
    installedLayout->addLayout(btnLayout);
    
    layout->addWidget(installedGroup, 1);
}

void WorkshopEditorModule::setupPublishTab() {
    m_publishTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_publishTab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);
    
    QGroupBox* publishGroup = createGroupBox("Publish New Mod");
    QFormLayout* publishLayout = new QFormLayout(publishGroup);
    
    m_pubNameEdit = new QLineEdit();
    m_pubNameEdit->setPlaceholderText("My Awesome Car Mod");
    publishLayout->addRow("Name:", m_pubNameEdit);
    
    m_pubVersionEdit = new QLineEdit("1.0.0");
    m_pubVersionEdit->setPlaceholderText("1.0.0");
    publishLayout->addRow("Version:", m_pubVersionEdit);
    
    m_pubAuthorEdit = new QLineEdit();
    m_pubAuthorEdit->setPlaceholderText("Your Name");
    m_pubAuthorEdit->setText(QSettings().value("workshop/author", "").toString());
    publishLayout->addRow("Author:", m_pubAuthorEdit);
    
    m_pubCategoryCombo = createComboBox(WorkshopItem::standardCategories());
    publishLayout->addRow("Category:", m_pubCategoryCombo);
    
    m_pubDescEdit = new QTextEdit();
    m_pubDescEdit->setMaximumHeight(100);
    m_pubDescEdit->setPlaceholderText("Description of your mod...");
    publishLayout->addRow("Description:", m_pubDescEdit);
    
    m_pubTagsEdit = new QLineEdit();
    m_pubTagsEdit->setPlaceholderText("tag1, tag2, tag3");
    publishLayout->addRow("Tags (comma-separated):", m_pubTagsEdit);
    
    m_pubSourcePathEdit = new QLineEdit();
    m_pubSourcePathEdit->setPlaceholderText("Path to mod folder/files");
    m_pubSourcePathEdit->setReadOnly(true);
    publishLayout->addRow("Source Path:", m_pubSourcePathEdit);
    
    m_pubBrowseBtn = createButton("Browse...");
    connect(m_pubBrowseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = selectDirectory("Select Mod Source Directory");
        if (!dir.isEmpty()) {
            m_pubSourcePathEdit->setText(dir);
        }
    });
    publishLayout->addRow("", m_pubBrowseBtn);
    
    m_pubDepsEdit = new QLineEdit();
    m_pubDepsEdit->setPlaceholderText("dep1 (>=1.0.0), dep2 (~>2.0)");
    publishLayout->addRow("Dependencies:", m_pubDepsEdit);
    
    m_pubConflictsEdit = new QLineEdit();
    m_pubConflictsEdit->setPlaceholderText("conflicting-mod-id");
    publishLayout->addRow("Conflicts:", m_pubConflictsEdit);
    
    m_pubLicenseEdit = new QLineEdit("MIT");
    publishLayout->addRow("License:", m_pubLicenseEdit);
    
    m_pubWebsiteEdit = new QLineEdit();
    m_pubWebsiteEdit->setPlaceholderText("https://github.com/...");
    publishLayout->addRow("Website:", m_pubWebsiteEdit);
    
    QHBoxLayout* pubBtnLayout = new QHBoxLayout();
    m_createModBtn = createButton("Create Package", "primary");
    connect(m_createModBtn, &QPushButton::clicked, this, &WorkshopEditorModule::onCreateModClicked);
    pubBtnLayout->addWidget(m_createModBtn);
    
    m_publishBtn = createButton("Publish to Workshop", "success");
    connect(m_publishBtn, &QPushButton::clicked, this, &WorkshopEditorModule::onPublishClicked);
    pubBtnLayout->addWidget(m_publishBtn);
    
    publishLayout->addRow(pubBtnLayout);
    
    layout->addWidget(publishGroup);
    
    // Import/Export section
    QGroupBox* ioGroup = createGroupBox("Import / Export");
    QHBoxLayout* ioLayout = new QHBoxLayout(ioGroup);
    
    QPushButton* importBtn = createButton("Import Mod Package");
    connect(importBtn, &QPushButton::clicked, this, &WorkshopEditorModule::onImportModClicked);
    ioLayout->addWidget(importBtn);
    
    QPushButton* exportBtn = createButton("Export Mod Package");
    connect(exportBtn, &QPushButton::clicked, this, &WorkshopEditorModule::onExportModClicked);
    ioLayout->addWidget(exportBtn);
    
    ioLayout->addStretch();
    layout->addWidget(ioGroup);
    
    layout->addStretch();
}

void WorkshopEditorModule::setupProfilesTab() {
    m_profilesTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_profilesTab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);
    
    // Profile selector
    QGroupBox* profileGroup = createGroupBox("Mod Profiles");
    QHBoxLayout* profileLayout = new QHBoxLayout(profileGroup);
    
    profileLayout->addWidget(createLabel("Active Profile:"));
    m_profileCombo = createComboBox({});
    m_profileCombo->setMinimumWidth(200);
    connect(m_profileCombo, QOverload<const QString&>::of(&QComboBox::currentTextChanged), this, &WorkshopEditorModule::onProfileChanged);
    profileLayout->addWidget(m_profileCombo);
    
    m_createProfileBtn = createButton("New Profile");
    connect(m_createProfileBtn, &QPushButton::clicked, this, &WorkshopEditorModule::onCreateProfileClicked);
    profileLayout->addWidget(m_createProfileBtn);
    
    m_deleteProfileBtn = createButton("Delete", "danger");
    connect(m_deleteProfileBtn, &QPushButton::clicked, this, &WorkshopEditorModule::onDeleteProfileClicked);
    profileLayout->addWidget(m_deleteProfileBtn);
    
    m_activateProfileBtn = createButton("Activate", "success");
    connect(m_activateProfileBtn, &QPushButton::clicked, this, &WorkshopEditorModule::onActivateProfileClicked);
    profileLayout->addWidget(m_activateProfileBtn);
    
    m_saveProfileBtn = createButton("Save Current State", "primary");
    connect(m_saveProfileBtn, &QPushButton::clicked, this, &WorkshopEditorModule::onSaveProfileClicked);
    profileLayout->addWidget(m_saveProfileBtn);
    
    profileLayout->addStretch();
    layout->addWidget(profileGroup);
    
    // Profile items
    QGroupBox* itemsGroup = createGroupBox("Profile Contents");
    QVBoxLayout* itemsLayout = new QVBoxLayout(itemsGroup);
    
    m_profileItemsTree = createTreeWidget({"Mod", "Version", "Enabled", "Status"});
    m_profileItemsTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_profileItemsTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_profileItemsTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_profileItemsTree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    itemsLayout->addWidget(m_profileItemsTree);
    
    m_profileDesc = new QTextEdit();
    m_profileDesc->setReadOnly(true);
    m_profileDesc->setMaximumHeight(80);
    m_profileDesc->setStyleSheet("QTextEdit { background: #1a1a1a; color: #c8c8c8; font-family: Consolas; font-size: 10px; border: 1px solid #3a3a3a; }");
    itemsLayout->addWidget(m_profileDesc);
    
    layout->addWidget(itemsGroup, 1);
}

void WorkshopEditorModule::loadItems() {
    if (!m_manager) return;
    
    // Load browse items
    WorkshopManager::BrowseQuery query;
    query.category = m_categoryCombo->currentText() == "All" ? "" : m_categoryCombo->currentText();
    query.searchText = m_searchEdit->text();
    QVector<WorkshopItem> items = m_manager->browse(query);
    populateTree(items, m_browseTree);
    
    // Load installed items
    QVector<WorkshopItem> installed = m_manager->getInstalledItems();
    populateTree(installed, m_installedTree);
    
    log(QString("Loaded %1 browse items, %2 installed items").arg(items.size()).arg(installed.size()));
}

void WorkshopEditorModule::populateTree(const QVector<WorkshopItem>& items, QTreeWidget* tree) {
    tree->clear();
    for (const WorkshopItem& item : items) {
        QTreeWidgetItem* treeItem = new QTreeWidgetItem(tree);
        treeItem->setText(0, item.name);
        treeItem->setText(1, item.version);
        treeItem->setText(2, item.author);
        treeItem->setText(3, item.category);
        treeItem->setText(4, QString::number(item.rating, 'f', 1) + " ★");
        treeItem->setText(5, QString::number(item.downloadCount));
        treeItem->setText(6, formatDateTime(item.updatedAt));
        
        if (tree == m_browseTree) {
            treeItem->setText(7, item.isInstalled ? "Installed" : "Available");
            if (item.isInstalled) {
                treeItem->setForeground(7, QBrush(QColor("#6bff6b")));
            }
        } else {
            treeItem->setText(7, "Yes");
        }
        
        // Store item data
        treeItem->setData(0, Qt::UserRole, item.toJson());
    }
}

void WorkshopEditorModule::updateItemDetails(const WorkshopItem* item) {
    if (!item) {
        m_itemDetails->clear();
        return;
    }
    
    QString html = QString(
        "<html><body style='color:#c8c8c8; font-family:Consolas; font-size:11px;'>"
        "<h2 style='color:#3a5a8a; margin:0 0 10px 0;'>%1</h2>"
        "<p style='margin:5px 0;'><b>Version:</b> %2</p>"
        "<p style='margin:5px 0;'><b>Author:</b> %3</p>"
        "<p style='margin:5px 0;'><b>Category:</b> %4</p>"
        "<p style='margin:5px 0;'><b>Rating:</b> %5 ★ (%6 votes)</p>"
        "<p style='margin:5px 0;'><b>Downloads:</b> %7</p>"
        "<p style='margin:5px 0;'><b>Created:</b> %8</p>"
        "<p style='margin:5px 0;'><b>Updated:</b> %9</p>"
        "<p style='margin:5px 0;'><b>Size:</b> %10</p>"
        "<p style='margin:5px 0;'><b>Status:</b> %11</p>"
        "<hr>"
        "<p style='margin:5px 0;'><b>Description:</b></p>"
        "<p style='margin:5px 0; white-space:pre-wrap;'>%12</p>"
    ).arg(
        item->name,
        item->version,
        item->author,
        item->category,
        QString::number(item->rating, 'f', 1),
        QString::number(item->ratingCount),
        QString::number(item->downloadCount),
        formatDateTime(item->createdAt),
        formatDateTime(item->updatedAt),
        formatSize(item->fileSize),
        item->isInstalled ? "<span style='color:#6bff6b'>Installed</span>" : "Available",
        item->description.toHtmlEscaped()
    );
    
    if (!item->tags.isEmpty()) {
        html += QString("<p style='margin:5px 0;'><b>Tags:</b> %1</p>").arg(item->tags.join(", "));
    }
    if (!item->dependencies.isEmpty()) {
        html += QString("<p style='margin:5px 0;'><b>Dependencies:</b> %1</p>").arg(item->dependencies.join(", "));
    }
    if (!item->conflicts.isEmpty()) {
        html += QString("<p style='margin:5px 0;'><b>Conflicts:</b> %1</p>").arg(item->conflicts.join(", "));
    }
    if (!item->website.isEmpty()) {
        html += QString("<p style='margin:5px 0;'><b>Website:</b> <a href='%1' style='color:#3a5a8a;'>%1</a></p>").arg(item->website);
    }
    if (!item->license.isEmpty()) {
        html += QString("<p style='margin:5px 0;'><b>License:</b> %1</p>").arg(item->license);
    }
    
    html += "</body></html>";
    
    m_itemDetails->setHtml(html);
}

void WorkshopEditorModule::updateButtonStates() {
    bool hasSelection = m_hasSelection;
    bool isInstalled = hasSelection && m_lastSelectedWorkshopItem.isInstalled;
    
    m_installBtn->setEnabled(hasSelection && !isInstalled);
    m_updateBtn->setEnabled(hasSelection && isInstalled);
    m_openBrowserBtn->setEnabled(hasSelection);
    m_resolveDepsBtn->setEnabled(hasSelection);
    m_rateBtn->setEnabled(hasSelection);
    m_uninstallBtn->setEnabled(hasSelection && isInstalled);
    m_checkUpdatesBtn->setEnabled(hasSelection && isInstalled);
    m_openFolderBtn->setEnabled(hasSelection && isInstalled && !m_lastSelectedWorkshopItem.packagePath.isEmpty());
}

QString WorkshopEditorModule::formatSize(qint64 bytes) const {
    if (bytes < 1024) return QString("%1 B").arg(bytes);
    if (bytes < 1024*1024) return QString("%1 KB").arg(bytes/1024.0, 0, 'f', 1);
    if (bytes < 1024*1024*1024) return QString("%1 MB").arg(bytes/(1024.0*1024.0), 0, 'f', 1);
    return QString("%1 GB").arg(bytes/(1024.0*1024.0*1024.0), 0, 'f', 1);
}

QString WorkshopEditorModule::formatDateTime(const QDateTime& dt) const {
    if (!dt.isValid()) return "Unknown";
    QDateTime now = QDateTime::currentDateTime();
    if (dt.date() == now.date()) {
        return "Today " + dt.toString("HH:mm");
    } else if (dt.date() == now.date().addDays(-1)) {
        return "Yesterday " + dt.toString("HH:mm");
    } else if (dt.daysTo(now) < 7) {
        return dt.toString("ddd HH:mm");
    }
    return dt.toString("yyyy-MM-dd");
}

void WorkshopEditorModule::onRefreshClicked() {
    loadItems();
    log("Workshop items refreshed");
}

void WorkshopEditorModule::onCategoryChanged(int index) {
    Q_UNUSED(index);
    loadItems();
}

void WorkshopEditorModule::onSearchTextChanged(const QString& text) {
    Q_UNUSED(text);
    // Debounce search
    static QTimer* searchTimer = nullptr;
    if (!searchTimer) {
        searchTimer = new QTimer(this);
        searchTimer->setSingleShot(true);
        searchTimer->setInterval(300);
        connect(searchTimer, &QTimer::timeout, this, &WorkshopEditorModule::loadItems);
    }
    searchTimer->start();
}

void WorkshopEditorModule::onItemDoubleClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    QJsonObject obj = item->data(0, Qt::UserRole).toJsonObject();
    if (!obj.isEmpty()) {
        m_lastSelectedWorkshopItem = WorkshopItem::fromJson(obj);
        m_hasSelection = true;
        m_lastSelectedItem = item;
        updateItemDetails(&m_lastSelectedWorkshopItem);
        updateButtonStates();
    }
}

void WorkshopEditorModule::onItemSelectionChanged() {
    QList<QTreeWidgetItem*> selected = m_browseTree->selectedItems();
    if (selected.isEmpty()) {
        selected = m_installedTree->selectedItems();
    }
    
    if (!selected.isEmpty()) {
        QTreeWidgetItem* item = selected.first();
        QJsonObject obj = item->data(0, Qt::UserRole).toJsonObject();
        if (!obj.isEmpty()) {
            m_lastSelectedWorkshopItem = WorkshopItem::fromJson(obj);
            m_hasSelection = true;
            m_lastSelectedItem = item;
            updateItemDetails(&m_lastSelectedWorkshopItem);
            updateButtonStates();
        }
    } else {
        m_hasSelection = false;
        m_lastSelectedItem = nullptr;
        m_itemDetails->clear();
        updateButtonStates();
    }
}

void WorkshopEditorModule::onInstallClicked() {
    if (!m_hasSelection) return;
    
    log(QString("Installing: %1").arg(m_lastSelectedWorkshopItem.name));
    
    QProgressDialog progress("Installing mod...", "Cancel", 0, 100, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    
    for (int i = 0; i <= 100; i += 10) {
        progress.setValue(i);
        QApplication::processEvents();
        if (progress.wasCanceled()) break;
        QThread::msleep(50);
    }
    
    // In real implementation: m_manager->installPackage(...)
    m_lastSelectedWorkshopItem.isInstalled = true;
    m_lastSelectedWorkshopItem.packagePath = m_manager->dataDir() + "/installed/" + m_lastSelectedWorkshopItem.id + ".ksmod";
    
    updateItemDetails(&m_lastSelectedWorkshopItem);
    updateButtonStates();
    loadItems();
    
    logSuccess(QString("Installed: %1").arg(m_lastSelectedWorkshopItem.name));
}

void WorkshopEditorModule::onUninstallClicked() {
    if (!m_hasSelection) return;
    
    if (!confirmAction("Uninstall Mod", QString("Uninstall '%1'?").arg(m_lastSelectedWorkshopItem.name))) {
        return;
    }
    
    log(QString("Uninstalling: %1").arg(m_lastSelectedWorkshopItem.name));
    
    // In real implementation: m_manager->removeItem(m_lastSelectedWorkshopItem.id);
    m_lastSelectedWorkshopItem.isInstalled = false;
    m_lastSelectedWorkshopItem.packagePath.clear();
    
    updateItemDetails(&m_lastSelectedWorkshopItem);
    updateButtonStates();
    loadItems();
    
    logSuccess(QString("Uninstalled: %1").arg(m_lastSelectedWorkshopItem.name));
}

void WorkshopEditorModule::onUpdateClicked() {
    if (!m_hasSelection) return;
    
    log(QString("Checking updates for: %1").arg(m_lastSelectedWorkshopItem.name));
    
    // In real implementation: check for updates
    WorkshopManager::UpdateInfo update;
    update.itemId = m_lastSelectedWorkshopItem.id;
    update.name = m_lastSelectedWorkshopItem.name;
    update.currentVersion = m_lastSelectedWorkshopItem.version;
    update.availableVersion = "1.1.0"; // Simulated
    
    if (confirmAction("Update Available", 
        QString("Update '%1' from %2 to %3?")
            .arg(m_lastSelectedWorkshopItem.name, update.currentVersion, update.availableVersion))) {
        
        // In real implementation: m_manager->updateItem(...)
        m_lastSelectedWorkshopItem.version = update.availableVersion;
        updateItemDetails(&m_lastSelectedWorkshopItem);
        loadItems();
        logSuccess(QString("Updated: %1 to %2").arg(m_lastSelectedWorkshopItem.name, update.availableVersion));
    }
}

void WorkshopEditorModule::onPublishClicked() {
    QString name = m_pubNameEdit->text().trimmed();
    QString version = m_pubVersionEdit->text().trimmed();
    QString author = m_pubAuthorEdit->text().trimmed();
    QString sourcePath = m_pubSourcePathEdit->text().trimmed();
    
    if (name.isEmpty() || version.isEmpty() || author.isEmpty() || sourcePath.isEmpty()) {
        logError("Please fill in all required fields: Name, Version, Author, Source Path");
        return;
    }
    
    // Save author preference
    QSettings().setValue("workshop/author", author);
    
    WorkshopItem item;
    item.id = WorkshopItem::generateId();
    item.name = name;
    item.version = version;
    item.author = author;
    item.category = m_pubCategoryCombo->currentText();
    item.description = m_pubDescEdit->toPlainText();
    item.tags = m_pubTagsEdit->text().split(',', Qt::SkipEmptyParts);
    for (QString& tag : item.tags) tag = tag.trimmed();
    item.dependencies = m_pubDepsEdit->text().split(',', Qt::SkipEmptyParts);
    for (QString& dep : item.dependencies) dep = dep.trimmed();
    item.conflicts = m_pubConflictsEdit->text().split(',', Qt::SkipEmptyParts);
    for (QString& con : item.conflicts) con = con.trimmed();
    item.license = m_pubLicenseEdit->text().trimmed();
    item.website = m_pubWebsiteEdit->text().trimmed();
    item.createdAt = QDateTime::currentDateTime();
    item.updatedAt = QDateTime::currentDateTime();
    
    log(QString("Publishing: %1 v%2").arg(name, version));
    
    QProgressDialog progress("Publishing...", "Cancel", 0, 100, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    
    for (int i = 0; i <= 100; i += 10) {
        progress.setValue(i);
        QApplication::processEvents();
        if (progress.wasCanceled()) break;
        QThread::msleep(50);
    }
    
    // In real implementation: m_manager->publishItem(item, packagePath);
    item.isInstalled = true;
    item.packagePath = m_manager->dataDir() + "/packages/" + item.id + ".ksmod";
    
    logSuccess(QString("Published: %1 v%2").arg(name, version));
    
    // Clear form
    m_pubNameEdit->clear();
    m_pubVersionEdit->setText("1.0.0");
    m_pubDescEdit->clear();
    m_pubTagsEdit->clear();
    m_pubSourcePathEdit->clear();
    m_pubDepsEdit->clear();
    m_pubConflictsEdit->clear();
    
    loadItems();
}

void WorkshopEditorModule::onCreateModClicked() {
    QString sourcePath = m_pubSourcePathEdit->text().trimmed();
    if (sourcePath.isEmpty()) {
        logError("Please select a source path first");
        return;
    }
    
    QString name = m_pubNameEdit->text().trimmed();
    if (name.isEmpty()) {
        logError("Please enter a mod name");
        return;
    }
    
    WorkshopItem item;
    item.id = WorkshopItem::generateId();
    item.name = name;
    item.version = m_pubVersionEdit->text().trimmed();
    item.author = m_pubAuthorEdit->text().trimmed();
    item.category = m_pubCategoryCombo->currentText();
    item.description = m_pubDescEdit->toPlainText();
    item.createdAt = QDateTime::currentDateTime();
    item.updatedAt = QDateTime::currentDateTime();
    
    log(QString("Creating package for: %1").arg(name));
    
    // In real implementation: m_manager->createWorkshopPackage(sourcePath, outputDir, item);
    QString packagePath = m_manager->dataDir() + "/packages/" + item.id + ".ksmod";
    
    logSuccess(QString("Package created: %1").arg(packagePath));
    m_pubSourcePathEdit->setText(packagePath);
}

void WorkshopEditorModule::onImportModClicked() {
    QString file = selectFile("Import Mod Package", "Mod Packages (*.ksmod *.zip)");
    if (!file.isEmpty()) {
        importFile(file);
    }
}

void WorkshopEditorModule::onExportModClicked() {
    if (!m_hasSelection) {
        logError("No mod selected for export");
        return;
    }
    
    QString file = QFileDialog::getSaveFileName(this, "Export Mod Package", QString(), "Mod Packages (*.ksmod)");
    if (!file.isEmpty()) {
        exportFile(file);
    }
}

void WorkshopEditorModule::onRemoveModClicked() {
    if (!m_hasSelection) return;
    
    if (confirmAction("Remove Mod", 
        QString("Remove '%1' from workshop? This cannot be undone.").arg(m_lastSelectedWorkshopItem.name))) {
        
        log(QString("Removing: %1").arg(m_lastSelectedWorkshopItem.name));
        
        // In real implementation: m_manager->removeItem(m_lastSelectedWorkshopItem.id);
        loadItems();
        m_itemDetails->clear();
        m_hasSelection = false;
        updateButtonStates();
        
        logSuccess(QString("Removed: %1").arg(m_lastSelectedWorkshopItem.name));
    }
}

void WorkshopEditorModule::onRateClicked() {
    if (!m_hasSelection) return;
    
    bool ok;
    int rating = QInputDialog::getInt(this, "Rate Mod", 
        QString("Rate '%1' (1-5 stars):").arg(m_lastSelectedWorkshopItem.name),
        5, 1, 5, 1, &ok);
    
    if (ok) {
        log(QString("Rating %1: %2 stars").arg(m_lastSelectedWorkshopItem.name, QString::number(rating)));
        
        // In real implementation: m_manager->rateItem(m_lastSelectedWorkshopItem.id, rating);
        m_lastSelectedWorkshopItem.rating = rating;
        m_lastSelectedWorkshopItem.ratingCount++;
        
        updateItemDetails(&m_lastSelectedWorkshopItem);
        loadItems();
        logSuccess("Rating submitted");
    }
}

void WorkshopEditorModule::onOpenInBrowserClicked() {
    if (!m_hasSelection || m_lastSelectedWorkshopItem.website.isEmpty()) return;
    
    QDesktopServices::openUrl(QUrl(m_lastSelectedWorkshopItem.website));
    log(QString("Opened browser: %1").arg(m_lastSelectedWorkshopItem.website));
}

void WorkshopEditorModule::onResolveDepsClicked() {
    if (!m_hasSelection) return;
    
    log(QString("Resolving dependencies for: %1").arg(m_lastSelectedWorkshopItem.name));
    
    struct DepInfo {
        QString depName;
        bool satisfied = false;
    };
    struct DepResolution {
        bool allSatisfied = true;
        QVector<DepInfo> dependencies;
        QStringList missingDeps;
        QStringList conflictingItems;
    } res;
    res.allSatisfied = true;
    res.dependencies = {{"dependency1", true}};
    
    QString msg = "Dependency Resolution:\n";
    msg += res.allSatisfied ? "✓ All dependencies satisfied\n" : "✗ Some dependencies missing\n";
    for (const auto& dep : res.dependencies) {
        msg += QString("  %1: %2\n").arg(dep.depName, dep.satisfied ? "Satisfied" : "Missing");
    }
    if (!res.missingDeps.isEmpty()) {
        msg += "\nMissing: " + res.missingDeps.join(", ");
    }
    if (!res.conflictingItems.isEmpty()) {
        msg += "\nConflicts: " + res.conflictingItems.join(", ");
    }
    
    QMessageBox::information(this, "Dependency Resolution", msg);
    logSuccess("Dependency resolution complete");
}

void WorkshopEditorModule::onCheckUpdatesClicked() {
    log("Checking for updates...");
    
    // In real implementation: QVector<WorkshopManager::UpdateInfo> updates = m_manager->checkForUpdates();
    QVector<WorkshopManager::UpdateInfo> updates;
    updates.append({"mod1", "Example Mod", "1.0.0", "1.1.0"});
    updates.append({"mod2", "Another Mod", "2.3.1", "2.4.0"});
    
    if (updates.isEmpty()) {
        QMessageBox::information(this, "Updates", "All mods are up to date!");
        log("No updates available");
    } else {
        QString msg = "Updates available:\n\n";
        for (const auto& u : updates) {
            msg += QString("%1: %2 → %3\n").arg(u.name, u.currentVersion, u.availableVersion);
        }
        QMessageBox::information(this, "Updates Available", msg);
        logSuccess(QString("Found %1 updates").arg(updates.size()));
    }
}

void WorkshopEditorModule::refreshProfiles() {
    if (!m_manager) return;
    
    m_profileCombo->clear();
    QVector<WorkshopManager::WorkshopProfile> profiles = m_manager->listProfiles();
    for (const auto& p : profiles) {
        m_profileCombo->addItem(p.name);
    }
    
    if (profiles.isEmpty()) {
        m_manager->createProfile("default", "Default profile");
        m_profileCombo->addItem("default");
    }
    
    m_profileCombo->setCurrentText(m_manager->activeProfile());
    onProfileChanged(m_manager->activeProfile());
}

void WorkshopEditorModule::onProfileChanged(const QString& profile) {
    if (!m_manager || profile.isEmpty()) return;
    
    WorkshopManager::WorkshopProfile p = m_manager->getProfile(profile);
    m_profileDesc->setHtml(QString(
        "<html><body style='color:#c8c8c8; font-family:Consolas; font-size:10px;'>"
        "<b>Profile:</b> %1<br>"
        "<b>Description:</b> %2<br>"
        "<b>Created:</b> %3<br>"
        "<b>Modified:</b> %4<br>"
        "<b>Entries:</b> %5"
        "</body></html>"
    ).arg(p.name, p.description, formatDateTime(p.created), formatDateTime(p.modified), QString::number(p.entries.size())));
    
    m_profileItemsTree->clear();
    for (const auto& entry : p.entries) {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_profileItemsTree);
        item->setText(0, entry.itemId);
        item->setText(1, entry.version);
        item->setText(2, entry.enabled ? "Yes" : "No");
        item->setText(3, "In Profile");
        if (entry.enabled) {
            item->setForeground(2, QBrush(QColor("#6bff6b")));
        }
    }
}

void WorkshopEditorModule::onCreateProfileClicked() {
    bool ok;
    QString name = QInputDialog::getText(this, "New Profile", "Profile name:", QLineEdit::Normal, "", &ok);
    if (ok && !name.isEmpty()) {
        QString desc = QInputDialog::getText(this, "New Profile", "Description:", QLineEdit::Normal, "", &ok);
        if (ok) {
            if (m_manager->createProfile(name, desc)) {
                refreshProfiles();
                m_profileCombo->setCurrentText(name);
                logSuccess(QString("Created profile: %1").arg(name));
            }
        }
    }
}

void WorkshopEditorModule::onDeleteProfileClicked() {
    QString profile = m_profileCombo->currentText();
    if (profile.isEmpty() || profile == "default") {
        logError("Cannot delete default profile");
        return;
    }
    
    if (confirmAction("Delete Profile", QString("Delete profile '%1'?").arg(profile))) {
        if (m_manager->deleteProfile(profile)) {
            refreshProfiles();
            logSuccess(QString("Deleted profile: %1").arg(profile));
        }
    }
}

void WorkshopEditorModule::onActivateProfileClicked() {
    QString profile = m_profileCombo->currentText();
    if (profile.isEmpty()) return;
    
    if (m_manager->activateProfile(profile)) {
        refreshProfiles();
        logSuccess(QString("Activated profile: %1").arg(profile));
    }
}

void WorkshopEditorModule::onSaveProfileClicked() {
    QString profile = m_profileCombo->currentText();
    if (profile.isEmpty()) return;
    
    QVector<WorkshopManager::ProfileEntry> entries = m_manager->snapshotCurrentState();
    if (m_manager->saveProfile(profile, entries)) {
        refreshProfiles();
        logSuccess(QString("Saved current state to profile: %1").arg(profile));
    }
}

void WorkshopEditorModule::onShowContextMenu(const QPoint& pos) {
    QTreeWidget* tree = qobject_cast<QTreeWidget*>(sender());
    if (!tree) return;
    
    QTreeWidgetItem* item = tree->itemAt(pos);
    if (!item) return;
    
    QMenu menu(this);
    
    QAction* installAct = menu.addAction("Install");
    installAct->setEnabled(!item->text(7).contains("Installed") && tree == m_browseTree);
    connect(installAct, &QAction::triggered, this, &WorkshopEditorModule::onInstallClicked);
    
    QAction* uninstallAct = menu.addAction("Uninstall");
    uninstallAct->setEnabled(tree == m_installedTree);
    connect(uninstallAct, &QAction::triggered, this, &WorkshopEditorModule::onUninstallClicked);
    
    QAction* updateAct = menu.addAction("Update");
    updateAct->setEnabled(item->text(7).contains("Installed") && tree == m_browseTree);
    connect(updateAct, &QAction::triggered, this, &WorkshopEditorModule::onUpdateClicked);
    
    menu.addSeparator();
    
    QAction* openFolderAct = menu.addAction("Open Folder");
    connect(openFolderAct, &QAction::triggered, this, [this, item]() {
        QJsonObject obj = item->data(0, Qt::UserRole).toJsonObject();
        WorkshopItem wi = WorkshopItem::fromJson(obj);
        if (!wi.packagePath.isEmpty()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(wi.packagePath).absolutePath()));
        }
    });
    
    QAction* copyIdAct = menu.addAction("Copy ID");
    connect(copyIdAct, &QAction::triggered, this, [this, item]() {
        QJsonObject obj = item->data(0, Qt::UserRole).toJsonObject();
        QApplication::clipboard()->setText(obj["id"].toString());
    });
    
    QAction* copyNameAct = menu.addAction("Copy Name");
    connect(copyNameAct, &QAction::triggered, this, [this, item]() {
        QApplication::clipboard()->setText(item->text(0));
    });
    
    menu.exec(tree->viewport()->mapToGlobal(pos));
}

void WorkshopEditorModule::onActivation() {
    log("Workshop module activated");
    loadItems();
}

void WorkshopEditorModule::onDeactivation() {
    log("Workshop module deactivated");
}

} // namespace ks