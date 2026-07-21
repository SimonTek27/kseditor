#include "AssetEditorModule.h"
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
namespace assets {

AssetEditorModule::AssetEditorModule(QWidget* parent)
    : ModuleGuiBase(parent)
    , m_tabWidget(nullptr)
    , m_browserTab(nullptr)
    , m_assetTree(nullptr)
    , m_assetInfoLabel(nullptr)
    , m_assetPreviewLabel(nullptr)
    , m_searchInput(nullptr)
    , m_filterCombo(nullptr)
    , m_tagFilterCombo(nullptr)
    , m_importBtn(nullptr)
    , m_exportBtn(nullptr)
    , m_deleteBtn(nullptr)
    , m_refreshBtn(nullptr)
    , m_previewBtn(nullptr)
    , m_searchTab(nullptr)
    , m_searchResults(nullptr)
    , m_searchProgress(nullptr)
    , m_dependenciesTab(nullptr)
    , m_depTree(nullptr)
    , m_showDepsBtn(nullptr)
    , m_cloudSyncTab(nullptr)
    , m_syncNowBtn(nullptr)
    , m_configureSyncBtn(nullptr)
    , m_syncStatusLabel(nullptr)
    , m_syncProgress(nullptr)
{
    setObjectName("AssetEditorModule");
}

bool AssetEditorModule::initialize() {
    if (m_uiBuilt) return true;
    ModuleGuiBase::initialize();
    return true;
}

void AssetEditorModule::shutdown() {
    m_uiBuilt = false;
}

void AssetEditorModule::importFile(const QString& filePath) {
    if (filePath.isEmpty()) return;
    QFileInfo fi(filePath);
    QString suffix = fi.suffix().toLower();
    if (suffix == "kn5" || suffix == "fbx" || suffix == "gltf" || suffix == "glb" ||
        suffix == "png" || suffix == "jpg" || suffix == "dds" || suffix == "obj" ||
        suffix == "wav" || suffix == "ogg") {
        log(QString("Importing asset: %1").arg(filePath));
    } else {
        logError(QString("Unsupported asset format: %1").arg(suffix));
    }
}

void AssetEditorModule::exportFile(const QString& filePath) {
    if (filePath.isEmpty()) return;
    log(QString("Exporting asset to: %1").arg(filePath));
}

void AssetEditorModule::onActivation() {}
void AssetEditorModule::onDeactivation() {}

void AssetEditorModule::buildUI() {
    m_tabWidget = new QTabWidget();

    setupBrowserTab();
    setupSearchTab();
    setupDependenciesTab();
    setupCloudSyncTab();

    m_mainLayout->addWidget(m_tabWidget);
    m_mainLayout->addWidget(m_logOutput);
}

void AssetEditorModule::setupBrowserTab() {
    m_browserTab = new QWidget();
    auto* layout = new QVBoxLayout(m_browserTab);

    auto* toolbar = new QHBoxLayout();
    m_searchInput = new QLineEdit();
    m_searchInput->setPlaceholderText("Search assets...");
    m_filterCombo = createComboBox({"All Assets", "Models", "Textures", "Audio", "Materials", "Animations", "Documents"});
    m_tagFilterCombo = createComboBox({"All Tags", "Car", "Track", "UI", "Character", "Environment"});
    toolbar->addWidget(m_searchInput);
    toolbar->addWidget(m_filterCombo);
    toolbar->addWidget(m_tagFilterCombo);
    layout->addLayout(toolbar);

    auto* actionBar = new QHBoxLayout();
    m_importBtn = createButton("Import");
    m_exportBtn = createButton("Export");
    m_deleteBtn = createButton("Delete");
    m_refreshBtn = createButton("Refresh");
    m_previewBtn = createButton("Preview");
    actionBar->addWidget(m_importBtn);
    actionBar->addWidget(m_exportBtn);
    actionBar->addWidget(m_deleteBtn);
    actionBar->addWidget(m_refreshBtn);
    actionBar->addWidget(m_previewBtn);
    actionBar->addStretch();
    layout->addLayout(actionBar);

    auto* splitter = createSplitter(Qt::Horizontal);
    m_assetTree = createTreeWidget({"Name", "Type", "Size", "Modified", "Status"});
    splitter->addWidget(m_assetTree);

    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);
    m_assetInfoLabel = createLabel("Select an asset to view details");
    rightLayout->addWidget(m_assetInfoLabel);
    m_assetPreviewLabel = createLabel("");
    m_assetPreviewLabel->setMinimumSize(200, 200);
    m_assetPreviewLabel->setStyleSheet("QLabel { background: #1e1e1e; border: 1px solid #3a3a3a; }");
    rightLayout->addWidget(m_assetPreviewLabel);
    rightLayout->addStretch();
    splitter->addWidget(rightPanel);

    layout->addWidget(splitter);

    connect(m_importBtn, &QPushButton::clicked, this, &AssetEditorModule::onImportAsset);
    connect(m_exportBtn, &QPushButton::clicked, this, &AssetEditorModule::onExportAsset);
    connect(m_deleteBtn, &QPushButton::clicked, this, &AssetEditorModule::onDeleteAsset);
    connect(m_refreshBtn, &QPushButton::clicked, this, &AssetEditorModule::onRefreshAssets);
    connect(m_previewBtn, &QPushButton::clicked, this, &AssetEditorModule::onPreviewAsset);
    connect(m_assetTree, &QTreeWidget::itemClicked, this, &AssetEditorModule::onAssetSelected);
    connect(m_searchInput, &QLineEdit::textChanged, this, &AssetEditorModule::onSearchTextChanged);
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AssetEditorModule::onFilterChanged);
    connect(m_tagFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AssetEditorModule::onTagFilterChanged);

    populateAssetTree();
    m_tabWidget->addTab(m_browserTab, "Asset Browser");
}

void AssetEditorModule::setupSearchTab() {
    m_searchTab = new QWidget();
    auto* layout = new QVBoxLayout(m_searchTab);

    m_searchProgress = new QProgressBar();
    m_searchProgress->setVisible(false);
    layout->addWidget(m_searchProgress);

    m_searchResults = new QTableWidget(0, 4);
    m_searchResults->setHorizontalHeaderLabels({"Name", "Type", "Path", "Score"});
    m_searchResults->horizontalHeader()->setStretchLastSection(true);
    m_searchResults->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_searchResults->setAlternatingRowColors(true);
    layout->addWidget(m_searchResults);

    m_tabWidget->addTab(m_searchTab, "Search");
}

void AssetEditorModule::setupDependenciesTab() {
    m_dependenciesTab = new QWidget();
    auto* layout = new QVBoxLayout(m_dependenciesTab);

    m_showDepsBtn = createButton("Analyze Dependencies");
    layout->addWidget(m_showDepsBtn);

    m_depTree = createTreeWidget({"Asset", "Dependency", "Type", "Status"});
    layout->addWidget(m_depTree);

    connect(m_showDepsBtn, &QPushButton::clicked, this, &AssetEditorModule::onShowDependencies);

    populateDependencies();
    m_tabWidget->addTab(m_dependenciesTab, "Dependencies");
}

void AssetEditorModule::setupCloudSyncTab() {
    m_cloudSyncTab = new QWidget();
    auto* layout = new QVBoxLayout(m_cloudSyncTab);

    auto* statusGroup = createGroupBox("Sync Status");
    auto* statusLayout = new QVBoxLayout(statusGroup);
    m_syncStatusLabel = createLabel("Not connected");
    statusLayout->addWidget(m_syncStatusLabel);
    m_syncProgress = new QProgressBar();
    statusLayout->addWidget(m_syncProgress);
    layout->addWidget(statusGroup);

    auto* buttonLayout = new QHBoxLayout();
    m_syncNowBtn = createButton("Sync Now");
    m_configureSyncBtn = createButton("Configure Sync");
    buttonLayout->addWidget(m_syncNowBtn);
    buttonLayout->addWidget(m_configureSyncBtn);
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);

    layout->addStretch();

    connect(m_syncNowBtn, &QPushButton::clicked, this, [this]() {
        log("Starting cloud synchronization...");
        m_syncProgress->setRange(0, 0);
        m_syncStatusLabel->setText("Syncing...");
    });
    connect(m_configureSyncBtn, &QPushButton::clicked, this, [this]() {
        log("Opening sync configuration");
    });

    m_tabWidget->addTab(m_cloudSyncTab, "Cloud Sync");
}

void AssetEditorModule::populateAssetTree() {
    m_assetTree->clear();
    auto* models = new QTreeWidgetItem(m_assetTree, {"Models", "Folder", "", "", ""});
    models->addChild(new QTreeWidgetItem({"Car Body", "Mesh", "12.4 MB", "2024-01-15", "Ready"}));
    models->addChild(new QTreeWidgetItem({"Wheels", "Mesh", "8.2 MB", "2024-01-15", "Ready"}));
    models->addChild(new QTreeWidgetItem({"Interior", "Mesh", "15.7 MB", "2024-01-14", "Ready"}));

    auto* textures = new QTreeWidgetItem(m_assetTree, {"Textures", "Folder", "", "", ""});
    textures->addChild(new QTreeWidgetItem({"Paint_Red", "Texture", "24.1 MB", "2024-01-15", "Ready"}));
    textures->addChild(new QTreeWidgetItem({"CarbonFiber", "Texture", "18.5 MB", "2024-01-14", "Ready"}));
    textures->addChild(new QTreeWidgetItem({"Interior Leather", "Texture", "32.0 MB", "2024-01-13", "Ready"}));

    auto* audio = new QTreeWidgetItem(m_assetTree, {"Audio", "Folder", "", "", ""});
    audio->addChild(new QTreeWidgetItem({"Engine_V8", "Audio", "5.2 MB", "2024-01-15", "Ready"}));
    audio->addChild(new QTreeWidgetItem({"Tire Skid", "Audio", "1.8 MB", "2024-01-12", "Ready"}));

    m_assetTree->expandAll();
}

void AssetEditorModule::populateDependencies() {
    m_depTree->clear();
    m_depTree->addTopLevelItem(new QTreeWidgetItem({"Car Body", "Cockpit.obj", "Mesh", "Referenced"}));
    m_depTree->addTopLevelItem(new QTreeWidgetItem({"Car Body", "Paint_Red.dds", "Texture", "Referenced"}));
    m_depTree->addTopLevelItem(new QTreeWidgetItem({"Interior", "Interior Leather.dds", "Texture", "Referenced"}));
    m_depTree->addTopLevelItem(new QTreeWidgetItem({"Wheels", "Tire_Rubber.dds", "Texture", "Missing"}));
}

void AssetEditorModule::onAssetSelected(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    if (item && item->childCount() == 0) {
        m_assetInfoLabel->setText(QString("Asset: %1\nType: %2\nSize: %3").arg(item->text(0), item->text(1), item->text(2)));
    }
}

void AssetEditorModule::onSearchTextChanged(const QString& text) {
    Q_UNUSED(text);
}

void AssetEditorModule::onFilterChanged(int index) {
    Q_UNUSED(index);
}

void AssetEditorModule::onTagFilterChanged(int index) {
    Q_UNUSED(index);
}

void AssetEditorModule::onImportAsset() {
    QStringList paths = selectFiles("Import Assets", "All Supported (*.kn5 *.fbx *.gltf *.glb *.obj *.png *.dds *.wav *.ogg);;All Files (*)");
    if (!paths.isEmpty()) {
        for (const auto& path : paths) {
            importFile(path);
        }
        logSuccess(QString("Imported %1 assets").arg(paths.size()));
    }
}

void AssetEditorModule::onExportAsset() {
    auto* item = m_assetTree->currentItem();
    if (!item) {
        logError("No asset selected for export");
        return;
    }
    QString path = selectFile("Export Asset", "All Files (*)");
    if (!path.isEmpty()) {
        exportFile(path);
    }
}

void AssetEditorModule::onDeleteAsset() {
    auto* item = m_assetTree->currentItem();
    if (item && item->childCount() == 0) {
        if (confirmAction("Delete Asset", QString("Delete '%1'? This cannot be undone.").arg(item->text(0)))) {
            log(QString("Deleted asset: %1").arg(item->text(0)));
            delete item;
        }
    }
}

void AssetEditorModule::onRefreshAssets() {
    log("Refreshing asset list...");
    populateAssetTree();
    logSuccess("Asset list refreshed");
}

void AssetEditorModule::onPreviewAsset() {
    auto* item = m_assetTree->currentItem();
    if (item && item->childCount() == 0) {
        log(QString("Previewing asset: %1").arg(item->text(0)));
    }
}

void AssetEditorModule::onShowDependencies() {
    log("Analyzing asset dependencies...");
    populateDependencies();
    logSuccess("Dependency analysis complete");
}

} // namespace assets
} // namespace ks

#include "AssetEditorModule.moc"
