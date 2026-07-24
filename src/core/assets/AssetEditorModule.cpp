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
#include <QDesktopServices>
#include <QDirIterator>

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
    QStringList supported = {"kn5","fbx","gltf","glb","png","jpg","dds","obj","wav","ogg","tga","psd","mp3","flac"};
    if (supported.contains(suffix)) {
        QString assetDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/assets/" + suffix;
        QDir().mkpath(assetDir);
        QString dest = assetDir + "/" + fi.fileName();
        if (QFile::copy(filePath, dest)) {
            logSuccess(QString("Imported asset: %1").arg(fi.fileName()));
            populateAssetTree();
        } else if (QFile::exists(dest)) {
            logWarning(QString("Asset already exists: %1").arg(fi.fileName()));
        } else {
            logError("Failed to import: " + fi.fileName());
        }
    } else {
        logError(QString("Unsupported asset format: %1").arg(suffix));
    }
}

void AssetEditorModule::exportFile(const QString& filePath) {
    if (filePath.isEmpty()) return;
    auto* item = m_assetTree->currentItem();
    if (item && item->childCount() == 0) {
        QFileInfo src(item->data(0, Qt::UserRole).toString());
        if (src.exists() && QFile::copy(src.absoluteFilePath(), filePath)) {
            logSuccess(QString("Exported asset to: %1").arg(filePath));
        } else {
            logError("Failed to export asset");
        }
    }
}

void AssetEditorModule::onActivation() {
    populateAssetTree();
}
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
    QString assetRoot = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/assets";
    QStringList categoryDirs = {"kn5", "fbx", "gltf", "glb", "png", "dds", "obj", "wav", "ogg", "tga"};
    QStringList categoryNames = {"Models", "Models", "Models", "Models", "Textures", "Textures", "Models", "Audio", "Audio", "Textures"};
    QStringList typeNames = {"Mesh", "Mesh", "Mesh", "Mesh", "Texture", "Texture", "Mesh", "Audio", "Audio", "Texture"};

    QMap<QString, QTreeWidgetItem*> folders;
    for (int i = 0; i < categoryDirs.size(); ++i) {
        QString catName = categoryNames[i];
        if (!folders.contains(catName)) {
            folders[catName] = new QTreeWidgetItem(m_assetTree, {catName, "Folder", "", "", ""});
        }
        QDir dir(assetRoot + "/" + categoryDirs[i]);
        if (!dir.exists()) continue;
        for (const auto& fi : dir.entryInfoList(QDir::Files, QDir::Name)) {
            QString sizeStr;
            qint64 sz = fi.size();
            if (sz > 1048576) sizeStr = QString::number(sz / 1048576.0, 'f', 1) + " MB";
            else if (sz > 1024) sizeStr = QString::number(sz / 1024.0, 'f', 1) + " KB";
            else sizeStr = QString::number(sz) + " B";
            auto* child = new QTreeWidgetItem({fi.completeBaseName(), typeNames[i], sizeStr, fi.lastModified().toString("yyyy-MM-dd"), "Ready"});
            child->setData(0, Qt::UserRole, fi.absoluteFilePath());
            folders[catName]->addChild(child);
        }
    }
    m_assetTree->expandAll();
}

void AssetEditorModule::populateDependencies() {
    m_depTree->clear();
    QString assetRoot = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/assets";
    QDirIterator it(assetRoot, QDir::Files, QDirIterator::Subdirectories);
    QVector<QPair<QString, QString>> pairs;
    QStringList allAssets;
    while (it.hasNext()) {
        allAssets << QFileInfo(it.next()).fileName();
    }
    for (const auto& asset : allAssets) {
        if (asset.endsWith(".kn5", Qt::CaseInsensitive) || asset.endsWith(".fbx", Qt::CaseInsensitive) || asset.endsWith(".gltf", Qt::CaseInsensitive)) {
            QString base = asset.section('.', 0, -2);
            for (const auto& dep : allAssets) {
                if (dep != asset && (dep.contains(base, Qt::CaseInsensitive) || dep.contains("paint", Qt::CaseInsensitive) || dep.contains("tire", Qt::CaseInsensitive))) {
                    m_depTree->addTopLevelItem(new QTreeWidgetItem({base, dep, dep.contains(".dds") ? "Texture" : "Mesh", QFileInfo(assetRoot + "/" + dep).exists() ? "Referenced" : "Missing"}));
                }
            }
        }
    }
}

void AssetEditorModule::onAssetSelected(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    if (item && item->childCount() == 0) {
        m_assetInfoLabel->setText(QString("Asset: %1\nType: %2\nSize: %3\nPath: %4").arg(item->text(0), item->text(1), item->text(2), item->data(0, Qt::UserRole).toString()));
    }
}

void AssetEditorModule::onSearchTextChanged(const QString& text) {
    if (text.length() < 2) return;
    m_searchResults->setRowCount(0);
    QString assetRoot = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/assets";
    QDirIterator it(assetRoot, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString fp = it.next();
        QFileInfo fi(fp);
        if (fi.fileName().contains(text, Qt::CaseInsensitive)) {
            int row = m_searchResults->rowCount();
            m_searchResults->insertRow(row);
            m_searchResults->setItem(row, 0, new QTableWidgetItem(fi.completeBaseName()));
            m_searchResults->setItem(row, 1, new QTableWidgetItem(fi.suffix()));
            m_searchResults->setItem(row, 2, new QTableWidgetItem(fp));
            m_searchResults->setItem(row, 3, new QTableWidgetItem("100%"));
        }
    }
    log(QString("Search '%1': %2 results").arg(text).arg(m_searchResults->rowCount()));
}

void AssetEditorModule::onFilterChanged(int index) {
    Q_UNUSED(index);
    populateAssetTree();
}

void AssetEditorModule::onTagFilterChanged(int index) {
    Q_UNUSED(index);
}

void AssetEditorModule::onImportAsset() {
    QStringList paths = selectFiles("Import Assets", "All Supported (*.kn5 *.fbx *.gltf *.glb *.obj *.png *.dds *.wav *.ogg *.tga);;All Files (*)");
    if (!paths.isEmpty()) {
        for (const auto& path : paths) {
            importFile(path);
        }
        logSuccess(QString("Imported %1 assets").arg(paths.size()));
    }
}

void AssetEditorModule::onExportAsset() {
    auto* item = m_assetTree->currentItem();
    if (!item || item->childCount() != 0) {
        logError("No asset selected for export");
        return;
    }
    QString srcPath = item->data(0, Qt::UserRole).toString();
    QFileInfo srcFi(srcPath);
    if (!srcFi.exists()) { logError("Asset file not found"); return; }
    QString destPath = selectFile("Export Asset", srcFi.suffix().toUpper() + " Files (*." + srcFi.suffix() + ");;All Files (*)");
    if (!destPath.isEmpty()) {
        if (QFile::copy(srcPath, destPath)) {
            logSuccess(QString("Exported asset to: %1").arg(destPath));
        } else {
            logError("Failed to export asset");
        }
    }
}

void AssetEditorModule::onDeleteAsset() {
    auto* item = m_assetTree->currentItem();
    if (item && item->childCount() == 0) {
        QString path = item->data(0, Qt::UserRole).toString();
        if (confirmAction("Delete Asset", QString("Delete '%1'? This cannot be undone.").arg(item->text(0)))) {
            QFile::remove(path);
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
        QString path = item->data(0, Qt::UserRole).toString();
        log(QString("Previewing asset: %1").arg(path));
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
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
