#include "AssetsLibraryModule.h"

#include <QMenuBar>
#include <QMenu>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QSplitter>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileSystemModel>
#include <QHeaderView>
#include <QPainter>
#include <QCryptographicHash>
#include <QDir>
#include <QDateTime>
#include <QDesktopServices>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QStandardPaths>
#include <QTableView>
#include <QtCore/private/qzipreader_p.h>
#include <QSqlRecord>

#include "core/sys/LogManager.h"
#include "core/sys/SettingsManager.h"
#include <QSqlError>

namespace ks {

// ============================================================================
// Helper Functions
// ============================================================================
QString assetTypeToString(ACAssetType type)
{
    switch (type)
    {
        case ACAssetType::Car: return "Car";
        case ACAssetType::Track: return "Track";
        case ACAssetType::Skin: return "Skin";
        case ACAssetType::Texture: return "Texture";
        case ACAssetType::Sound: return "Sound";
        case ACAssetType::Font: return "Font";
        case ACAssetType::Physics: return "Physics";
        case ACAssetType::AI: return "AI";
        default: return "Generic";
    }
}

ACAssetType stringToAssetType(const QString& str)
{
    if (str == "Car") return ACAssetType::Car;
    if (str == "Track") return ACAssetType::Track;
    if (str == "Skin") return ACAssetType::Skin;
    if (str == "Texture") return ACAssetType::Texture;
    if (str == "Sound") return ACAssetType::Sound;
    if (str == "Font") return ACAssetType::Font;
    if (str == "Physics") return ACAssetType::Physics;
    if (str == "AI") return ACAssetType::AI;
    return ACAssetType::Generic;
}

// ============================================================================
// AssetsLibraryModule Implementation
// ============================================================================
AssetsLibraryModule::AssetsLibraryModule(QWidget* parent)
    : EditorModule(parent)
    , m_centralWidget(nullptr)
    , m_dockWidget(nullptr)
    , m_categoryTree(nullptr)
    , m_assetTable(nullptr)
    , m_proxyModel(nullptr)
    , m_assetModel(nullptr)
    , m_searchEdit(nullptr)
    , m_typeFilterCombo(nullptr)
    , m_sortCombo(nullptr)
    , m_detailsScrollArea(nullptr)
    , m_detailsWidget(nullptr)
    , m_previewLabel(nullptr)
    , m_nameEdit(nullptr)
    , m_authorEdit(nullptr)
    , m_descriptionEdit(nullptr)
    , m_tagsEdit(nullptr)
    , m_typeLabel(nullptr)
    , m_sizeLabel(nullptr)
    , m_ratingLabel(nullptr)
    , m_downloadsLabel(nullptr)
    , m_createdDateLabel(nullptr)
    , m_modifiedDateLabel(nullptr)
    , m_cloudStatusLabel(nullptr)
    , m_favoriteBtn(nullptr)
    , m_previewBtn(nullptr)
    , m_openFileBtn(nullptr)
    , m_cloudUploadBtn(nullptr)
    , m_cloudDownloadBtn(nullptr)
    , m_tagList(nullptr)
    , m_syncQueueList(nullptr)
    , m_importBtn(nullptr)
    , m_syncAllBtn(nullptr)
    , m_syncProgress(nullptr)
    , m_clearFilterBtn(nullptr)
    , m_pathLabel(nullptr)
    , m_addTagBtn(nullptr)
    , m_removeTagBtn(nullptr)
    , m_cancelSyncBtn(nullptr)
    , m_networkManager(nullptr)
    , m_syncInProgress(false)
    , m_currentAssetId(-1)
{
    LOG_INFO("AssetsLibraryModule", "Initializing Assets Library module");
    
    // Get settings
    SettingsManager* settings = globalSettings();
    m_databasePath = settings->value("Assets/databasePath", 
        QDir::homePath() + "/kseditor/assets.db").toString();
    m_localAssetsPath = settings->value("Assets/localPath", 
        QDir::homePath() + "/Documents/kseditor/assets").toString();
    m_thumbnailsPath = settings->value("Assets/thumbnailsPath", 
        QDir::homePath() + "/kseditor/thumbnails").toString();
    m_cloudEndpoint = settings->value("Cloud/endpoint", "").toString();
    
    setupUi();
    setupConnections();
    setupDatabase();
    setupCloudManager();
    updateAssetList();
}

AssetsLibraryModule::~AssetsLibraryModule()
{
    LOG_INFO("AssetsLibraryModule", "Shutting down Assets Library module");
    
    if (m_database.isOpen())
    {
        m_database.close();
    }
    
    if (m_networkManager)
    {
        delete m_networkManager;
    }
}

QDockWidget* AssetsLibraryModule::getOrCreateDockWidget(QMainWindow* mainWindow)
{
    if (!m_dockWidget)
    {
        m_dockWidget = new QDockWidget("Assets Library", mainWindow);
        m_dockWidget->setWidget(m_centralWidget);
        m_dockWidget->setAllowedAreas(Qt::AllDockWidgetAreas);
    }
    return m_dockWidget;
}

void AssetsLibraryModule::setupUi()
{
    m_centralWidget = new QWidget();
    QHBoxLayout* mainLayout = new QHBoxLayout(m_centralWidget);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Main splitter
    QSplitter* mainSplitter = new QSplitter(Qt::Horizontal, m_centralWidget);
    mainLayout->addWidget(mainSplitter);
    
    // ========== Left Panel - Categories ==========
    QWidget* leftPanel = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    
    // Toolbar
    QHBoxLayout* toolbarLayout = new QHBoxLayout();
    m_importBtn = new QPushButton("Import");
    m_syncAllBtn = new QPushButton("Sync All");
    toolbarLayout->addWidget(m_importBtn);
    toolbarLayout->addWidget(m_syncAllBtn);
    leftLayout->addLayout(toolbarLayout);
    
    // Category tree
    m_categoryTree = new QTreeWidget();
    m_categoryTree->setHeaderLabel("Categories");
    
    // Add category items
    QTreeWidgetItem* allItem = new QTreeWidgetItem(m_categoryTree);
    allItem->setText(0, "All Assets");
    allItem->setData(0, Qt::UserRole, -1);
    
    QTreeWidgetItem* carsItem = new QTreeWidgetItem(m_categoryTree);
    carsItem->setText(0, "Cars");
    carsItem->setData(0, Qt::UserRole, (int)ACAssetType::Car);
    
    QTreeWidgetItem* tracksItem = new QTreeWidgetItem(m_categoryTree);
    tracksItem->setText(0, "Tracks");
    tracksItem->setData(0, Qt::UserRole, (int)ACAssetType::Track);
    
    QTreeWidgetItem* skinsItem = new QTreeWidgetItem(m_categoryTree);
    skinsItem->setText(0, "Skins");
    skinsItem->setData(0, Qt::UserRole, (int)ACAssetType::Skin);
    
    QTreeWidgetItem* texturesItem = new QTreeWidgetItem(m_categoryTree);
    texturesItem->setText(0, "Textures");
    texturesItem->setData(0, Qt::UserRole, (int)ACAssetType::Texture);
    
    QTreeWidgetItem* soundsItem = new QTreeWidgetItem(m_categoryTree);
    soundsItem->setText(0, "Sounds");
    soundsItem->setData(0, Qt::UserRole, (int)ACAssetType::Sound);
    
    QTreeWidgetItem* fontsItem = new QTreeWidgetItem(m_categoryTree);
    fontsItem->setText(0, "Fonts");
    fontsItem->setData(0, Qt::UserRole, (int)ACAssetType::Font);
    
    QTreeWidgetItem* physicsItem = new QTreeWidgetItem(m_categoryTree);
    physicsItem->setText(0, "Physics");
    physicsItem->setData(0, Qt::UserRole, (int)ACAssetType::Physics);
    
    QTreeWidgetItem* aiItem = new QTreeWidgetItem(m_categoryTree);
    aiItem->setText(0, "AI");
    aiItem->setData(0, Qt::UserRole, (int)ACAssetType::AI);
    
    QTreeWidgetItem* favItem = new QTreeWidgetItem(m_categoryTree);
    favItem->setText(0, "Favorites");
    favItem->setData(0, Qt::UserRole, -2);
    
    m_categoryTree->expandAll();
    leftLayout->addWidget(m_categoryTree);
    
    // Sync status
    QGroupBox* syncGroup = new QGroupBox("Cloud Sync");
    QVBoxLayout* syncLayout = new QVBoxLayout();
    
    m_syncStatusLabel = new QLabel("Ready");
    m_syncProgress = new QProgressBar();
    m_syncQueueList = new QListWidget();
    m_cancelSyncBtn = new QPushButton("Cancel");
    syncLayout->addWidget(m_syncStatusLabel);
    syncLayout->addWidget(m_syncProgress);
    syncLayout->addWidget(m_syncQueueList);
    syncLayout->addWidget(m_cancelSyncBtn);
    syncGroup->setLayout(syncLayout);
    leftLayout->addWidget(syncGroup);
    
    mainSplitter->addWidget(leftPanel);
    
    // ========== Center Panel - Asset List ==========
    QWidget* centerPanel = new QWidget();
    QVBoxLayout* centerLayout = new QVBoxLayout(centerPanel);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    
    // Search and filter bar
    QHBoxLayout* filterLayout = new QHBoxLayout();
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("Search assets...");
    m_typeFilterCombo = new QComboBox();
    m_typeFilterCombo->addItems({"All Types", "Cars", "Tracks", "Skins", "Textures", "Sounds", "Fonts", "Physics", "AI"});
    m_sortCombo = new QComboBox();
    m_sortCombo->addItems({"Name (A-Z)", "Name (Z-A)", "Date (Newest)", "Date (Oldest)", "Rating", "Downloads"});
    m_clearFilterBtn = new QPushButton("Clear");
    
    filterLayout->addWidget(m_searchEdit);
    filterLayout->addWidget(m_typeFilterCombo);
    filterLayout->addWidget(m_sortCombo);
    filterLayout->addWidget(m_clearFilterBtn);
    centerLayout->addLayout(filterLayout);
    
    // Asset table
    m_assetModel = new QSqlTableModel(this, m_database);
    m_assetModel->setTable("assets");
    m_assetModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    m_assetModel->select();

    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_assetModel);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setFilterKeyColumn(-1);

    m_assetTable = new QTableView();
    m_assetTable->setModel(m_proxyModel);
    m_assetTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_assetTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_assetTable->setAlternatingRowColors(true);
    m_assetTable->horizontalHeader()->setStretchLastSection(true);
    centerLayout->addWidget(m_assetTable);
    
    mainSplitter->addWidget(centerPanel);
    
    // ========== Right Panel - Details ==========
    QWidget* rightPanel = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    
    // Scroll area for details
    m_detailsScrollArea = new QScrollArea();
    m_detailsScrollArea->setWidgetResizable(true);
    m_detailsWidget = new QWidget();
    QVBoxLayout* detailsLayout = new QVBoxLayout(m_detailsWidget);
    
    // Preview
    QGroupBox* previewGroup = new QGroupBox("Preview");
    QVBoxLayout* previewLayout = new QVBoxLayout();
    m_previewLabel = new QLabel();
    m_previewLabel->setMinimumSize(200, 150);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setStyleSheet("background-color: #2a2a2a; border: 1px solid #3a3a3a;");
    m_previewLabel->setText("No preview");
    m_previewBtn = new QPushButton("Preview");
    previewLayout->addWidget(m_previewLabel);
    previewLayout->addWidget(m_previewBtn);
    previewGroup->setLayout(previewLayout);
    detailsLayout->addWidget(previewGroup);
    
    // Asset info
    QGroupBox* infoGroup = new QGroupBox("Information");
    QFormLayout* infoForm = new QFormLayout();
    
    m_nameEdit = new QLineEdit();
    m_authorEdit = new QLineEdit();
    m_descriptionEdit = new QTextEdit();
    m_descriptionEdit->setMaximumHeight(80);
    m_tagsEdit = new QLineEdit();
    
    infoForm->addRow("Name:", m_nameEdit);
    infoForm->addRow("Author:", m_authorEdit);
    infoForm->addRow("Description:", m_descriptionEdit);
    infoForm->addRow("Tags:", m_tagsEdit);
    infoForm->addRow("Type:", m_typeLabel = new QLabel("-"));
    infoForm->addRow("Size:", m_sizeLabel = new QLabel("-"));
    infoForm->addRow("Rating:", m_ratingLabel = new QLabel("-"));
    infoForm->addRow("Downloads:", m_downloadsLabel = new QLabel("-"));
    infoForm->addRow("Created:", m_createdDateLabel = new QLabel("-"));
    infoForm->addRow("Modified:", m_modifiedDateLabel = new QLabel("-"));
    infoForm->addRow("Cloud:", m_cloudStatusLabel = new QLabel("-"));
    m_pathLabel = new QLabel("-");
    m_pathLabel->setWordWrap(true);
    infoForm->addRow("Path:", m_pathLabel);
    
    infoGroup->setLayout(infoForm);
    detailsLayout->addWidget(infoGroup);
    
    // Actions
    QGroupBox* actionsGroup = new QGroupBox("Actions");
    QVBoxLayout* actionsLayout = new QVBoxLayout();
    
    m_favoriteBtn = new QPushButton("Add to Favorites");
    m_openFileBtn = new QPushButton("Open File Location");
    m_cloudUploadBtn = new QPushButton("Upload to Cloud");
    m_cloudDownloadBtn = new QPushButton("Download from Cloud");
    
    QPushButton* deleteBtn = new QPushButton("Delete");
    deleteBtn->setStyleSheet("color: #ff6666;");
    
    actionsLayout->addWidget(m_favoriteBtn);
    actionsLayout->addWidget(m_openFileBtn);
    actionsLayout->addWidget(m_cloudUploadBtn);
    actionsLayout->addWidget(m_cloudDownloadBtn);
    actionsLayout->addWidget(deleteBtn);
    
    actionsGroup->setLayout(actionsLayout);
    detailsLayout->addWidget(actionsGroup);
    
    // Tags
    QGroupBox* tagsGroup = new QGroupBox("Tags");
    QVBoxLayout* tagsLayout = new QVBoxLayout();
    
    m_tagList = new QListWidget();
    m_tagList->setMaximumHeight(100);
    QHBoxLayout* tagBtnLayout = new QHBoxLayout();
    m_addTagBtn = new QPushButton("+");
    m_removeTagBtn = new QPushButton("-");
    tagBtnLayout->addWidget(m_addTagBtn);
    tagBtnLayout->addWidget(m_removeTagBtn);
    tagBtnLayout->addStretch();
    tagsLayout->addWidget(m_tagList);
    tagsLayout->addLayout(tagBtnLayout);
    
    tagsGroup->setLayout(tagsLayout);
    detailsLayout->addWidget(tagsGroup);
    
    detailsLayout->addStretch();
    m_detailsScrollArea->setWidget(m_detailsWidget);
    rightLayout->addWidget(m_detailsScrollArea);
    
    mainSplitter->addWidget(rightPanel);
    
    // Set splitter proportions
    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 2);
    mainSplitter->setStretchFactor(2, 1);
    
    mainSplitter->setSizes({200, 400, 250});
}

void AssetsLibraryModule::setupConnections()
{
    connect(m_importBtn, &QPushButton::clicked, this, &AssetsLibraryModule::onImportAsset);
    connect(m_syncAllBtn, &QPushButton::clicked, this, &AssetsLibraryModule::onSyncAll);
    connect(m_cancelSyncBtn, &QPushButton::clicked, this, &AssetsLibraryModule::onCancelSync);
    
    connect(m_categoryTree, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* item, int column) {
        int typeIndex = item->data(0, Qt::UserRole).toInt();
        if (typeIndex == -1)
        {
            m_typeFilterCombo->setCurrentIndex(0);
        }
        else if (typeIndex == -2)
        {
            m_searchFilter.clear();
            m_typeFilter = -2;
        }
        else
        {
            m_typeFilterCombo->setCurrentIndex(typeIndex + 1);
        }
        updateAssetList();
    });
    
    connect(m_assetTable, &QTableView::clicked, this, &AssetsLibraryModule::onAssetSelected);
    connect(m_assetTable, &QTableView::doubleClicked, this, &AssetsLibraryModule::onAssetDoubleClicked);
    
    connect(m_searchEdit, &QLineEdit::textChanged, this, &AssetsLibraryModule::onSearchTextChanged);
    connect(m_typeFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AssetsLibraryModule::onTypeFilterChanged);
    connect(m_sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AssetsLibraryModule::onSortChanged);
    connect(m_clearFilterBtn, &QPushButton::clicked, this, [this]() {
        m_searchEdit->clear();
        m_typeFilterCombo->setCurrentIndex(0);
        updateAssetList();
    });
    
    connect(m_favoriteBtn, &QPushButton::clicked, this, &AssetsLibraryModule::onToggleFavorite);
    connect(m_previewBtn, &QPushButton::clicked, this, &AssetsLibraryModule::onPreviewAsset);
    connect(m_openFileBtn, &QPushButton::clicked, this, &AssetsLibraryModule::onOpenFileLocation);
    connect(m_cloudUploadBtn, &QPushButton::clicked, this, &AssetsLibraryModule::onUploadToCloud);
    connect(m_cloudDownloadBtn, &QPushButton::clicked, this, &AssetsLibraryModule::onDownloadFromCloud);
    
    connect(m_addTagBtn, &QPushButton::clicked, this, &AssetsLibraryModule::onAddTag);
    connect(m_removeTagBtn, &QPushButton::clicked, this, &AssetsLibraryModule::onRemoveTag);

    // Sync connections
    connect(m_networkManager, &QNetworkAccessManager::finished, this, [this](QNetworkReply* reply) {
        if (reply->error() == QNetworkReply::NoError) {
            onCloudSyncFinished();
        } else {
            onCloudSyncError(reply->errorString());
        }
    });
    
    // Connect details changes
    connect(m_nameEdit, &QLineEdit::textChanged, this, [this]() { 
        m_currentAsset.name = m_nameEdit->text();
        saveCurrentAsset();
    });
    connect(m_authorEdit, &QLineEdit::textChanged, this, [this]() {
        m_currentAsset.author = m_authorEdit->text();
        saveCurrentAsset();
    });
    connect(m_descriptionEdit, &QTextEdit::textChanged, this, [this]() {
        m_currentAsset.description = m_descriptionEdit->toPlainText();
        saveCurrentAsset();
    });
    connect(m_tagsEdit, &QLineEdit::textChanged, this, [this]() {
        m_currentAsset.tags = m_tagsEdit->text();
        saveCurrentAsset();
    });
}

void AssetsLibraryModule::setupDatabase()
{
    // Ensure directory exists
    QFileInfo fileInfo(m_databasePath);
    QDir dir = fileInfo.dir();
    if (!dir.exists())
    {
        dir.mkpath(".");
    }
    
    m_database = QSqlDatabase::addDatabase("QSQLITE");
    m_database.setDatabaseName(m_databasePath);
    
    if (!m_database.open())
    {
        LOG_ERROR("AssetsLibraryModule", "Failed to open database: " + m_database.lastError().text());
        return;
    }
    
    // Create tables
    QSqlQuery query(m_database);
    
    query.exec("CREATE TABLE IF NOT EXISTS assets ("
              "id INTEGER PRIMARY KEY AUTOINCREMENT,"
              "name TEXT NOT NULL,"
              "type TEXT NOT NULL,"
              "author TEXT,"
              "description TEXT,"
              "tags TEXT,"
              "filePath TEXT NOT NULL,"
              "thumbnailPath TEXT,"
              "originalPath TEXT,"
              "rating REAL DEFAULT 0,"
              "downloads INTEGER DEFAULT 0,"
              "likes INTEGER DEFAULT 0,"
              "createdDate TEXT,"
              "modifiedDate TEXT,"
              "uploadedDate TEXT,"
              "fileSize INTEGER DEFAULT 0,"
              "hash TEXT,"
              "version TEXT,"
              "gameVersion TEXT,"
              "isFavorite INTEGER DEFAULT 0,"
              "isLocal INTEGER DEFAULT 1,"
              "isCloud INTEGER DEFAULT 0,"
              "cloudUrl TEXT"
              ")");
    
    query.exec("CREATE TABLE IF NOT EXISTS tags ("
              "id INTEGER PRIMARY KEY AUTOINCREMENT,"
              "name TEXT UNIQUE NOT NULL"
              ")");
    
    query.exec("CREATE TABLE IF NOT EXISTS asset_tags ("
              "assetId INTEGER,"
              "tagId INTEGER,"
              "PRIMARY KEY (assetId, tagId)"
              ")");
    
    query.exec("CREATE TABLE IF NOT EXISTS collections ("
              "id INTEGER PRIMARY KEY AUTOINCREMENT,"
              "name TEXT NOT NULL,"
              "description TEXT,"
              "createdDate TEXT,"
              "isShared INTEGER DEFAULT 0"
              ")");
    
    query.exec("CREATE TABLE IF NOT EXISTS collection_assets ("
              "collectionId INTEGER,"
              "assetId INTEGER,"
              "PRIMARY KEY (collectionId, assetId)"
              ")");
    
    LOG_INFO("AssetsLibraryModule", "Database initialized: " + m_databasePath);
}

void AssetsLibraryModule::setupCloudManager()
{
    m_networkManager = new QNetworkAccessManager(this);
}

void AssetsLibraryModule::onActivation()
{
    LOG_INFO("AssetsLibraryModule", "Module activated");
    updateAssetList();
}

void AssetsLibraryModule::onDeactivation()
{
    LOG_INFO("AssetsLibraryModule", "Module deactivated");
}

void AssetsLibraryModule::updateAssetList()
{
    AssetFilter filter;
    if (m_typeFilterCombo) {
        if (m_typeFilterCombo->currentIndex() > 0) {
            filter.type = (ACAssetType)(m_typeFilterCombo->currentIndex() - 1);
        }
    }
    filter.searchText = m_searchFilter;
    if (m_sortCombo) {
        switch (m_sortCombo->currentIndex()) {
            case 0: filter.sortBy = "name"; filter.sortAscending = true; break;
            case 1: filter.sortBy = "name"; filter.sortAscending = false; break;
            case 2: filter.sortBy = "createdDate"; filter.sortAscending = false; break;
            case 3: filter.sortBy = "createdDate"; filter.sortAscending = true; break;
            case 4: filter.sortBy = "rating"; filter.sortAscending = false; break;
            case 5: filter.sortBy = "downloads"; filter.sortAscending = false; break;
            default: filter.sortBy = "name"; filter.sortAscending = true; break;
        }
    }

    QList<AssetMetadata> assets = DatabaseManager::instance().queryAssets(filter);
    if (m_assetModel) {
        m_assetModel->clear();
        for (const auto& asset : assets) {
            QSqlRecord record = m_assetModel->record();
            record.setValue("id", asset.id);
            record.setValue("name", asset.name);
            record.setValue("type", assetTypeToString(asset.type));
            record.setValue("author", asset.author);
            record.setValue("rating", asset.rating);
            m_assetModel->insertRecord(-1, record);
        }
    }
}

void AssetsLibraryModule::saveCurrentAsset()
{
    if (m_currentAssetId < 0) return;
    m_currentAsset.modifiedDate = QDateTime::currentDateTime();
    DatabaseManager::instance().updateAsset(m_currentAsset);
}

void AssetsLibraryModule::queueCloudUpload(const AssetMetadata& asset)
{
    m_syncQueue.append({asset.filePath, SyncStatus::NotSynced, "", "", 0.0f});
    if (m_syncQueueList) {
        m_syncQueueList->addItem("Upload: " + asset.name);
    }
    syncWithCloud();
}

void AssetsLibraryModule::queueCloudDownload(const AssetMetadata& asset)
{
    m_syncQueue.append({asset.filePath, SyncStatus::NotSynced, "", "", 0.0f});
    if (m_syncQueueList) {
        m_syncQueueList->addItem("Download: " + asset.name);
    }
    syncWithCloud();
}

void AssetsLibraryModule::syncWithCloud()
{
    if (m_syncQueue.isEmpty() || !m_networkManager) {
        m_syncInProgress = false;
        m_syncStatusLabel->setText("Sync complete");
        return;
    }

    SyncState& state = m_syncQueue.first();
    if (state.status == SyncStatus::NotSynced) {
        state.status = SyncStatus::Syncing;
        QNetworkRequest request(QUrl(m_cloudEndpoint + "/sync"));
        request.setRawHeader("Authorization", "Bearer " + m_apiKey.toUtf8());
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QJsonObject payload;
        payload["path"] = state.assetPath;
        QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(payload).toJson());
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            reply->deleteLater();
            if (!m_syncQueue.isEmpty()) {
                m_syncQueue.removeFirst();
                if (m_syncQueueList) {
                    delete m_syncQueueList->takeItem(0);
                }
            }
            syncWithCloud();
        });
    }
}

void AssetsLibraryModule::generateThumbnail(const QString& filePath)
{
    ACThumbnailGenerator::generate(filePath, m_thumbnailsPath);
}

QString AssetsLibraryModule::calculateFileHash(const QString& filePath)
{
    return FileHashCalculator::calculateMD5(filePath);
}

QString AssetsLibraryModule::formatFileSize(qint64 bytes)
{
    if (bytes < 1024) return QString::number(bytes) + " B";
    if (bytes < 1024 * 1024) return QString::number(bytes / 1024) + " KB";
    if (bytes < 1024LL * 1024 * 1024) return QString::number(bytes / (1024 * 1024)) + " MB";
    return QString::number(bytes / (1024LL * 1024 * 1024)) + " GB";
}

void AssetsLibraryModule::onImportAsset()
{
    QStringList filePaths = QFileDialog::getOpenFileNames(this, "Import Assets",
        m_localAssetsPath,
        "All Files (*.*);;Cars (*.kn5 *.fbx);;Tracks (*.kn5);;Textures (*.png *.jpg);;Sounds (*.wav *.ogg);;Fonts (*.acf)");
    
    if (filePaths.isEmpty()) return;
    
    for (const QString& filePath : filePaths)
    {
        AssetMetadata asset;
        QFileInfo fileInfo(filePath);
        
        asset.name = fileInfo.baseName();
        asset.filePath = filePath;
        asset.author = "Unknown";
        asset.createdDate = QDateTime::currentDateTime();
        asset.modifiedDate = QDateTime::currentDateTime();
        asset.fileSize = fileInfo.size();
        asset.hash = FileHashCalculator::calculateMD5(filePath);
        asset.isLocal = true;
        
        // Determine type from extension
        QString ext = fileInfo.suffix().toLower();
        if (ext == "kn5" || ext == "fbx")
            asset.type = ACAssetType::Car;
        else if (ext == "png" || ext == "jpg")
            asset.type = ACAssetType::Texture;
        else if (ext == "wav" || ext == "ogg")
            asset.type = ACAssetType::Sound;
        else if (ext == "acf")
            asset.type = ACAssetType::Font;
        else
            asset.type = ACAssetType::Generic;
        
        // Generate thumbnail
        QString thumbPath = ACThumbnailGenerator::generate(filePath, m_thumbnailsPath);
        if (!thumbPath.isEmpty())
        {
            asset.thumbnailPath = thumbPath;
        }
        
        // Save to database
        DatabaseManager::instance().insertAsset(asset);
        
        LOG_INFO("AssetsLibraryModule", "Imported asset: " + asset.name);
    }
    
    updateAssetList();
    QMessageBox::information(this, "Import Complete", 
        QString("Imported %1 asset(s).").arg(filePaths.size()));
}

void AssetsLibraryModule::onExportAsset()
{
    if (m_currentAssetId < 0) return;
    
    QString destPath = QFileDialog::getSaveFileName(this, "Export Asset",
        globalSettings()->value("lastDirectory", "").toString(),
        "All Files (*.*)");
    
    if (destPath.isEmpty()) return;
    
    if (QFile::copy(m_currentAsset.filePath, destPath))
    {
        LOG_INFO("AssetsLibraryModule", "Exported asset to: " + destPath);
        QMessageBox::information(this, "Export Complete", "Asset exported successfully.");
    }
    else
    {
        QMessageBox::critical(this, "Export Failed", "Failed to export asset.");
    }
}

void AssetsLibraryModule::onDeleteAsset()
{
    if (m_currentAssetId < 0) return;
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Delete Asset",
        "Are you sure you want to delete this asset from the library?\nThe original file will not be deleted.",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes)
    {
        DatabaseManager::instance().deleteAsset(m_currentAssetId);
        updateAssetList();
        loadAssetDetails(AssetMetadata()); // Clear details
    }
}

void AssetsLibraryModule::onDuplicateAsset()
{
    if (m_currentAssetId <= 0) return;

    AssetMetadata original = DatabaseManager::instance().getAsset(m_currentAssetId);
    if (original.filePath.isEmpty()) return;

    QFileInfo fi(original.filePath);
    QString newPath = fi.absolutePath() + "/" + fi.completeBaseName() + "_copy." + fi.suffix();
    if (QFile::exists(newPath)) {
        newPath = fi.absolutePath() + "/" + fi.completeBaseName() + "_copy_1." + fi.suffix();
    }

    if (QFile::copy(original.filePath, newPath)) {
        AssetMetadata copy = original;
        copy.name = original.name + " (Copy)";
        copy.filePath = newPath;
        copy.id = 0;
        DatabaseManager::instance().insertAsset(copy);
        updateAssetList();
    }
}

void AssetsLibraryModule::onAssetSelected(const QModelIndex& index)
{
    int row = index.row();
    int assetId = m_proxyModel->data(m_proxyModel->index(row, 0)).toInt();
    
    AssetMetadata asset = DatabaseManager::instance().getAsset(assetId);
    m_currentAssetId = assetId;
    m_currentAsset = asset;
    
    loadAssetDetails(asset);
}

void AssetsLibraryModule::loadAssetDetails(const AssetMetadata& asset)
{
    if (asset.filePath.isEmpty()) {
        m_nameEdit->clear();
        m_typeLabel->clear();
        m_sizeLabel->clear();
        m_pathLabel->clear();
        m_tagList->clear();
        return;
    }
    m_nameEdit->setText(asset.name);
    m_typeLabel->setText(assetTypeToString(asset.type));
    m_sizeLabel->setText(formatFileSize(asset.fileSize));
    m_pathLabel->setText(asset.filePath);
    m_tagList->clear();
    m_tagList->addItems(asset.tags.split(','));
}

void AssetsLibraryModule::onAssetDoubleClicked(const QModelIndex& index)
{
    int row = index.row();
    int assetId = m_proxyModel->data(m_proxyModel->index(row, 0)).toInt();
    AssetMetadata asset = DatabaseManager::instance().getAsset(assetId);
    QDesktopServices::openUrl(QUrl::fromLocalFile(asset.filePath));
}

void AssetsLibraryModule::onSearchTextChanged(const QString& text)
{
    m_searchFilter = text;
    updateAssetList();
}

void AssetsLibraryModule::onTypeFilterChanged(int index)
{
    m_typeFilter = index;
    updateAssetList();
}

void AssetsLibraryModule::onAuthorFilterChanged(const QString& author)
{
    m_authorFilter = author;
    updateAssetList();
}

void AssetsLibraryModule::onSortChanged(int index)
{
    m_sortOrder = index;
    updateAssetList();
}

void AssetsLibraryModule::onUploadToCloud()
{
    if (m_currentAssetId < 0) return;
    
    if (m_cloudEndpoint.isEmpty())
    {
        QMessageBox::warning(this, "Cloud Not Configured",
            "Please configure cloud settings first.");
        return;
    }
    
    queueCloudUpload(m_currentAsset);
    m_syncStatusLabel->setText("Uploading...");
    m_syncInProgress = true;
    
    LOG_INFO("AssetsLibraryModule", "Queued asset for cloud upload: " + m_currentAsset.name);
}

void AssetsLibraryModule::onDownloadFromCloud()
{
    if (m_currentAssetId < 0) return;
    
    if (m_currentAsset.cloudUrl.isEmpty())
    {
        QMessageBox::warning(this, "Not Uploaded",
            "This asset has not been uploaded to the cloud.");
        return;
    }
    
    queueCloudDownload(m_currentAsset);
    m_syncStatusLabel->setText("Downloading...");
    m_syncInProgress = true;
}

void AssetsLibraryModule::onSyncAsset()
{
    if (m_currentAssetId < 0) return;
    syncWithCloud();
}

void AssetsLibraryModule::onSyncAll()
{
    AssetFilter filter;
    filter.localOnly = true;
    QList<AssetMetadata> localAssets = DatabaseManager::instance().queryAssets(filter);
    
    for (const auto& asset : localAssets)
    {
        if (!asset.cloudUrl.isEmpty())
        {
            queueCloudUpload(asset);
        }
    }
    
    m_syncStatusLabel->setText(QString("Syncing %1 assets...").arg(m_syncQueue.size()));
    m_syncInProgress = true;
}

void AssetsLibraryModule::onCancelSync()
{
    m_syncQueue.clear();
    m_syncQueueList->clear();
    m_syncInProgress = false;
    m_syncStatusLabel->setText("Cancelled");
    m_syncProgress->setValue(0);
}

void AssetsLibraryModule::onOpenFileLocation()
{
    if (m_currentAsset.filePath.isEmpty()) return;
    
    QString path = m_currentAsset.filePath;
    QFileInfo fileInfo(path);
    QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.dir().absolutePath()));
}

void AssetsLibraryModule::onRevealInExplorer()
{
    onOpenFileLocation();
}

void AssetsLibraryModule::onToggleFavorite()
{
    if (m_currentAssetId < 0) return;
    
    m_currentAsset.isFavorite = !m_currentAsset.isFavorite;
    m_favoriteBtn->setText(m_currentAsset.isFavorite ? "Remove from Favorites" : "Add to Favorites");
    
    saveCurrentAsset();
    updateAssetList();
}

void AssetsLibraryModule::onAddTag()
{
    if (m_currentAssetId < 0) return;
    
    QString tag = QInputDialog::getText(this, "Add Tag", "Enter tag name:");
    if (tag.isEmpty()) return;
    
    DatabaseManager::instance().addTagToAsset(m_currentAssetId, tag);
    
    m_tagList->addItem(tag);
    m_currentAsset.tags += "," + tag;
}

void AssetsLibraryModule::onRemoveTag()
{
    if (m_currentAssetId < 0) return;
    
    QListWidgetItem* item = m_tagList->currentItem();
    if (!item) return;
    
    QString tag = item->text();
    DatabaseManager::instance().removeTagFromAsset(m_currentAssetId, tag);
    
    delete item;
}

void AssetsLibraryModule::onManageTags()
{
    TagManagerDialog dialog(this);
    dialog.exec();
}

void AssetsLibraryModule::onSettings()
{
    CloudSettingsDialog dialog(this);
    dialog.exec();
}

void AssetsLibraryModule::onPreviewAsset()
{
    if (m_currentAsset.filePath.isEmpty()) return;
    
    QString ext = QFileInfo(m_currentAsset.filePath).suffix().toLower();
    
    if (ext == "kn5" || ext == "fbx")
    {
        // Open in 3D Editor
        LOG_INFO("AssetsLibraryModule", "Opening 3D asset in editor");
    }
    else if (ext == "png" || ext == "jpg")
    {
        // Open image viewer
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_currentAsset.filePath));
    }
    else if (ext == "wav" || ext == "ogg")
    {
        // Open audio player
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_currentAsset.filePath));
    }
    else
    {
        // Open default application
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_currentAsset.filePath));
    }
}

QString ACThumbnailGenerator::generateFromArchive(const QString& archivePath, const QString& outputDir)
{
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/asset_thumb_temp";
    QDir().mkpath(tempDir);

    if (archivePath.endsWith(".zip", Qt::CaseInsensitive)) {
        QZipReader reader(archivePath);
        if (reader.exists()) {
            const auto entries = reader.fileInfoList();
            for (const auto& info : entries) {
                if (!(info.isDir) &&
                    (info.filePath.endsWith(".png") || info.filePath.endsWith(".jpg") ||
                     info.filePath.endsWith(".jpeg") || info.filePath.endsWith(".bmp"))) {
                    QString extractedPath = tempDir + "/" + info.filePath;
                    reader.extractAll(tempDir);
                    QString thumbPath = outputDir + "/archive_thumb.png";
                    QImage img(extractedPath);
                    if (!img.isNull()) {
                        img.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation)
                                .save(thumbPath);
                        return thumbPath;
                    }
                }
            }
        }
    }

    return QString();
}

QString ACThumbnailGenerator::generateFromVideo(const QString& videoPath, const QString& outputDir)
{
    QImage image(256, 144, QImage::Format_ARGB32);
    image.fill(QColor(30, 30, 30));

    QPainter painter(&image);
    painter.setPen(QColor(200, 200, 200));
    painter.setFont(QFont("Arial", 24));
    painter.drawText(image.rect(), Qt::AlignCenter, "▶");

    QString outputPath = outputDir + "/" + QFileInfo(videoPath).baseName() + "_thumb.png";
    if (image.save(outputPath)) {
        return outputPath;
    }
    return QString();
}

// ============================================================================
// AssetImportDialog Implementation
// ============================================================================
AssetImportDialog::AssetImportDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Import Asset");
    setMinimumSize(500, 400);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QFormLayout* formLayout = new QFormLayout();
    
    m_filePathEdit = new QLineEdit();
    QPushButton* browseFileBtn = new QPushButton("...");
    QHBoxLayout* fileLayout = new QHBoxLayout();
    fileLayout->addWidget(m_filePathEdit);
    fileLayout->addWidget(browseFileBtn);
    
    m_nameEdit = new QLineEdit();
    m_typeCombo = new QComboBox();
    m_typeCombo->addItems({"Car", "Track", "Skin", "Texture", "Sound", "Font", "Physics", "AI", "Generic"});
    m_authorEdit = new QLineEdit();
    m_descriptionEdit = new QTextEdit();
    m_tagsEdit = new QLineEdit();
    m_thumbnailEdit = new QLineEdit();
    QPushButton* browseThumbBtn = new QPushButton("...");
    QHBoxLayout* thumbLayout = new QHBoxLayout();
    thumbLayout->addWidget(m_thumbnailEdit);
    thumbLayout->addWidget(browseThumbBtn);
    m_versionEdit = new QLineEdit("1.0");
    
    formLayout->addRow("File:", fileLayout);
    formLayout->addRow("Name:", m_nameEdit);
    formLayout->addRow("Type:", m_typeCombo);
    formLayout->addRow("Author:", m_authorEdit);
    formLayout->addRow("Description:", m_descriptionEdit);
    formLayout->addRow("Tags:", m_tagsEdit);
    formLayout->addRow("Thumbnail:", thumbLayout);
    formLayout->addRow("Version:", m_versionEdit);
    
    mainLayout->addLayout(formLayout);
    
    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* importBtn = new QPushButton("Import");
    QPushButton* cancelBtn = new QPushButton("Cancel");
    btnLayout->addStretch();
    btnLayout->addWidget(importBtn);
    btnLayout->addWidget(cancelBtn);
    
    mainLayout->addLayout(btnLayout);
    
    connect(browseFileBtn, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Select File");
        if (!path.isEmpty()) m_filePathEdit->setText(path);
    });
    connect(browseThumbBtn, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Select Thumbnail", QString(), "Images (*.png *.jpg)");
        if (!path.isEmpty()) m_thumbnailEdit->setText(path);
    });
    connect(importBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void AssetImportDialog::setAssetType(ACAssetType type)
{
    m_typeCombo->setCurrentIndex((int)type);
}

AssetMetadata AssetImportDialog::getAssetMetadata() const
{
    AssetMetadata asset;
    asset.name = m_nameEdit->text();
    asset.filePath = m_filePathEdit->text();
    asset.type = (ACAssetType)m_typeCombo->currentIndex();
    asset.author = m_authorEdit->text();
    asset.description = m_descriptionEdit->toPlainText();
    asset.tags = m_tagsEdit->text();
    asset.thumbnailPath = m_thumbnailEdit->text();
    asset.version = m_versionEdit->text();
    return asset;
}

// ============================================================================
// CloudSettingsDialog Implementation
// ============================================================================
CloudSettingsDialog::CloudSettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Cloud Settings");
    setMinimumSize(400, 250);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QFormLayout* formLayout = new QFormLayout();
    
    m_endpointEdit = new QLineEdit();
    m_apiKeyEdit = new QLineEdit();
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    m_usernameEdit = new QLineEdit();
    
    formLayout->addRow("API Endpoint:", m_endpointEdit);
    formLayout->addRow("API Key:", m_apiKeyEdit);
    formLayout->addRow("Username:", m_usernameEdit);
    
    mainLayout->addLayout(formLayout);
    
    m_statusLabel = new QLabel();
    mainLayout->addWidget(m_statusLabel);
    
    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_testBtn = new QPushButton("Test Connection");
    QPushButton* saveBtn = new QPushButton("Save");
    QPushButton* cancelBtn = new QPushButton("Cancel");
    btnLayout->addWidget(m_testBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(cancelBtn);
    
    mainLayout->addLayout(btnLayout);
    
    // Load current settings
    SettingsManager* settings = globalSettings();
    m_endpointEdit->setText(settings->value("Cloud/endpoint", "").toString());
    m_apiKeyEdit->setText(settings->value("Cloud/apiKey", "").toString());
    m_usernameEdit->setText(settings->value("Cloud/username", "").toString());
    
    connect(m_testBtn, &QPushButton::clicked, this, &CloudSettingsDialog::onTestConnection);
    connect(saveBtn, &QPushButton::clicked, this, &CloudSettingsDialog::onSave);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void CloudSettingsDialog::onTestConnection()
{
    m_statusLabel->setText("Testing connection...");
    m_testBtn->setEnabled(false);

    QNetworkRequest request(QUrl(m_endpointEdit->text() + "/api/ping"));
    request.setRawHeader("Authorization", ("Bearer " + m_apiKeyEdit->text()).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            m_statusLabel->setText("Connection successful");
            m_statusLabel->setStyleSheet("color: #4CAF50;");
        } else {
            m_statusLabel->setText("Connection failed: " + reply->errorString());
            m_statusLabel->setStyleSheet("color: #f44336;");
        }
        m_testBtn->setEnabled(true);
        reply->deleteLater();
    });
}

void CloudSettingsDialog::onSave()
{
    SettingsManager* settings = globalSettings();
    settings->setValue("Cloud/endpoint", m_endpointEdit->text());
    settings->setValue("Cloud/apiKey", m_apiKeyEdit->text());
    settings->setValue("Cloud/username", m_usernameEdit->text());
    
    accept();
}

// ============================================================================
// TagManagerDialog Implementation
// ============================================================================
TagManagerDialog::TagManagerDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Manage Tags");
    setMinimumSize(400, 300);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    m_tagList = new QListWidget();
    mainLayout->addWidget(m_tagList);
    
    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* addBtn = new QPushButton("Add");
    QPushButton* removeBtn = new QPushButton("Remove");
    QPushButton* createBtn = new QPushButton("Create");
    QPushButton* applyBtn = new QPushButton("Apply");
    QPushButton* cancelBtn = new QPushButton("Cancel");
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(removeBtn);
    btnLayout->addWidget(createBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(applyBtn);
    btnLayout->addWidget(cancelBtn);
    
    mainLayout->addLayout(btnLayout);
    
    // Load existing tags
    QStringList tags = DatabaseManager::instance().getAllTags();
    m_allTags = tags;
    for (const QString& tag : tags)
    {
        m_tagList->addItem(tag);
    }
    
    connect(addBtn, &QPushButton::clicked, this, &TagManagerDialog::onAddTag);
    connect(removeBtn, &QPushButton::clicked, this, &TagManagerDialog::onRemoveTag);
    connect(createBtn, &QPushButton::clicked, this, &TagManagerDialog::onCreateTag);
    connect(applyBtn, &QPushButton::clicked, this, &TagManagerDialog::onApply);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void TagManagerDialog::onAddTag()
{
    bool ok;
    QString tag = QInputDialog::getText(this, "Add Tag", "Enter tag name:", QLineEdit::Normal, "", &ok);
    if (ok && !tag.isEmpty())
    {
        if (!m_allTags.contains(tag))
        {
            m_allTags.append(tag);
            m_tagList->addItem(tag);
        }
    }
}

void TagManagerDialog::onRemoveTag()
{
    QListWidgetItem* item = m_tagList->currentItem();
    if (item)
    {
        QString tag = item->text();
        m_allTags.removeOne(tag);
        delete item;
    }
}

void TagManagerDialog::onCreateTag()
{
    onAddTag();
}

void TagManagerDialog::onApply()
{
    m_selectedTags = m_allTags;
    accept();
}

// ============================================================================
// SyncManager Implementation
// ============================================================================
SyncManager::SyncManager(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_syncing(false)
    , m_progress(0.0f)
    , m_currentIndex(0)
{
}

void SyncManager::setEndpoint(const QString& endpoint)
{
    m_endpoint = endpoint;
}

void SyncManager::setApiKey(const QString& apiKey)
{
    m_apiKey = apiKey;
}

void SyncManager::queueUpload(const AssetMetadata& asset)
{
    SyncItem item;
    item.asset = asset;
    item.isUpload = true;
    m_queue.append(item);
}

void SyncManager::queueDownload(const AssetMetadata& asset)
{
    SyncItem item;
    item.asset = asset;
    item.isUpload = false;
    m_queue.append(item);
}

void SyncManager::cancelAll()
{
    m_queue.clear();
    m_syncing = false;
}

void SyncManager::processNextInQueue()
{
    if (m_currentIndex >= m_queue.size())
    {
        m_syncing = false;
        emit syncFinished();
        return;
    }

    const SyncItem& item = m_queue[m_currentIndex];

    if (item.isUpload)
    {
        QNetworkRequest request(QUrl(m_endpoint + "/upload"));
        request.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());

        QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
        QHttpPart filePart;
        filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
            QString("form-data; name=\"file\"; filename=\"%1\"").arg(QFileInfo(item.asset.filePath).fileName()));
        QFile* file = new QFile(item.asset.filePath);
        if (file->open(QIODevice::ReadOnly)) {
            filePart.setBodyDevice(file);
            file->setParent(multiPart);
            multiPart->append(filePart);
        }

        QNetworkReply* reply = m_networkManager->post(request, multiPart);
        multiPart->setParent(reply);

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            m_currentIndex++;
            if (reply->error() == QNetworkReply::NoError) {
                float pct = m_queue.isEmpty() ? 1.0f : static_cast<float>(m_currentIndex) / m_queue.size();
                emit syncProgress(pct);
            }
            reply->deleteLater();
            processNextInQueue();
        });
    }
    else
    {
        QNetworkRequest request(QUrl(item.asset.cloudUrl));
        request.setRawHeader("Authorization", QByteArray("Bearer ") + m_apiKey.toUtf8());

    QNetworkReply* reply = m_networkManager->get(request);

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            m_currentIndex++;
            if (reply->error() == QNetworkReply::NoError) {
                QString localPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
                                    + "/cloud/" + QFileInfo(reply->url().path()).fileName();
                QDir().mkpath(QFileInfo(localPath).absolutePath());
                QFile file(localPath);
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(reply->readAll());
                    file.close();
                }
                float pct = m_queue.isEmpty() ? 1.0f : static_cast<float>(m_currentIndex) / m_queue.size();
                emit syncProgress(pct);
            }
            reply->deleteLater();
            processNextInQueue();
        });
    }
}

// ============================================================================
// DatabaseManager Implementation
// ============================================================================
DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager instance;
    return instance;
}

bool DatabaseManager::initialize(const QString& databasePath)
{
    m_databasePath = databasePath;
    
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(databasePath);
    
    if (!db.open())
    {
        LOG_ERROR("DatabaseManager", "Failed to open database");
        return false;
    }
    
    return createTables();
}

void DatabaseManager::close()
{
    if (m_database.isOpen())
    {
        m_database.close();
    }
}

int DatabaseManager::insertAsset(const AssetMetadata& asset)
{
    QSqlQuery query(m_database);
    
    query.prepare("INSERT INTO assets (name, type, author, description, tags, filePath, thumbnailPath, "
                  "rating, downloads, likes, createdDate, modifiedDate, fileSize, hash, version, "
                  "isFavorite, isLocal, isCloud, cloudUrl) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    
    query.addBindValue(asset.name);
    query.addBindValue(assetTypeToString(asset.type));
    query.addBindValue(asset.author);
    query.addBindValue(asset.description);
    query.addBindValue(asset.tags);
    query.addBindValue(asset.filePath);
    query.addBindValue(asset.thumbnailPath);
    query.addBindValue(asset.rating);
    query.addBindValue(asset.downloads);
    query.addBindValue(asset.likes);
    query.addBindValue(asset.createdDate.toString(Qt::ISODate));
    query.addBindValue(asset.modifiedDate.toString(Qt::ISODate));
    query.addBindValue(asset.fileSize);
    query.addBindValue(asset.hash);
    query.addBindValue(asset.version);
    query.addBindValue(asset.isFavorite ? 1 : 0);
    query.addBindValue(asset.isLocal ? 1 : 0);
    query.addBindValue(asset.isCloud ? 1 : 0);
    query.addBindValue(asset.cloudUrl);
    
    if (query.exec())
    {
        return query.lastInsertId().toInt();
    }
    
    LOG_ERROR("DatabaseManager", "Failed to insert asset: " + query.lastError().text());
    return -1;
}

bool DatabaseManager::updateAsset(const AssetMetadata& asset)
{
    QSqlQuery query(m_database);
    
    query.prepare("UPDATE assets SET name = ?, type = ?, author = ?, description = ?, tags = ?, "
                  "thumbnailPath = ?, rating = ?, downloads = ?, likes = ?, modifiedDate = ?, "
                  "version = ?, isFavorite = ?, isCloud = ?, cloudUrl = ? WHERE id = ?");
    
    query.addBindValue(asset.name);
    query.addBindValue(assetTypeToString(asset.type));
    query.addBindValue(asset.author);
    query.addBindValue(asset.description);
    query.addBindValue(asset.tags);
    query.addBindValue(asset.thumbnailPath);
    query.addBindValue(asset.rating);
    query.addBindValue(asset.downloads);
    query.addBindValue(asset.likes);
    query.addBindValue(asset.modifiedDate.toString(Qt::ISODate));
    query.addBindValue(asset.version);
    query.addBindValue(asset.isFavorite ? 1 : 0);
    query.addBindValue(asset.isCloud ? 1 : 0);
    query.addBindValue(asset.cloudUrl);
    query.addBindValue(asset.id);
    
    if (!query.exec())
    {
        LOG_ERROR("DatabaseManager", "Failed to update asset: " + query.lastError().text());
        return false;
    }
    
    return true;
}

bool DatabaseManager::deleteAsset(int assetId)
{
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM assets WHERE id = ?");
    query.addBindValue(assetId);
    
    if (!query.exec())
    {
        LOG_ERROR("DatabaseManager", "Failed to delete asset");
        return false;
    }
    
    return true;
}

AssetMetadata DatabaseManager::getAsset(int assetId)
{
    AssetMetadata asset;
    asset.id = -1;
    
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM assets WHERE id = ?");
    query.addBindValue(assetId);
    
    if (query.exec() && query.next())
    {
        asset.id = query.value("id").toInt();
        asset.name = query.value("name").toString();
        asset.type = stringToAssetType(query.value("type").toString());
        asset.author = query.value("author").toString();
        asset.description = query.value("description").toString();
        asset.tags = query.value("tags").toString();
        asset.filePath = query.value("filePath").toString();
        asset.thumbnailPath = query.value("thumbnailPath").toString();
        asset.rating = query.value("rating").toFloat();
        asset.downloads = query.value("downloads").toInt();
        asset.likes = query.value("likes").toInt();
        asset.createdDate = QDateTime::fromString(query.value("createdDate").toString(), Qt::ISODate);
        asset.modifiedDate = QDateTime::fromString(query.value("modifiedDate").toString(), Qt::ISODate);
        asset.fileSize = query.value("fileSize").toLongLong();
        asset.hash = query.value("hash").toString();
        asset.version = query.value("version").toString();
        asset.isFavorite = query.value("isFavorite").toInt() == 1;
        asset.isLocal = query.value("isLocal").toInt() == 1;
        asset.isCloud = query.value("isCloud").toInt() == 1;
        asset.cloudUrl = query.value("cloudUrl").toString();
    }
    
    return asset;
}

QList<AssetMetadata> DatabaseManager::getAllAssets()
{
    AssetFilter filter;
    return queryAssets(filter);
}

QList<AssetMetadata> DatabaseManager::queryAssets(const AssetFilter& filter)
{
    QList<AssetMetadata> assets;
    
    QSqlQuery query(m_database);
    QString sql = "SELECT * FROM assets WHERE 1=1";
    QVariantMap bindings;
    
    if (filter.type != ACAssetType::Generic)
    {
        sql += " AND type = :type";
        bindings[":type"] = assetTypeToString(filter.type);
    }
    
    if (!filter.searchText.isEmpty())
    {
        sql += " AND (name LIKE :searchText OR author LIKE :searchText2)";
        bindings[":searchText"] = "%" + filter.searchText + "%";
        bindings[":searchText2"] = "%" + filter.searchText + "%";
    }
    
    if (filter.favoritesOnly)
    {
        sql += " AND isFavorite = 1";
    }
    
    if (filter.localOnly)
    {
        sql += " AND isLocal = 1";
    }
    
    if (filter.minRating > 0)
    {
        sql += " AND rating >= :minRating";
        bindings[":minRating"] = filter.minRating;
    }
    
    if (!filter.sortBy.isEmpty())
    {
        const QStringList allowedColumns = {"name", "type", "author", "rating", "downloads", "fileSize", "createdDate", "modifiedDate", "version"};
        QString safeCol = filter.sortBy.trimmed().toLower();
        QString suffix;
        if (safeCol.endsWith(" desc")) { safeCol.chop(5); suffix = " DESC"; }
        else if (safeCol.endsWith(" asc")) { safeCol.chop(4); suffix = " ASC"; }
        if (allowedColumns.contains(safeCol))
            sql += " ORDER BY " + safeCol + (suffix.isEmpty() ? (filter.sortAscending ? " ASC" : " DESC") : suffix);
    }
    
    query.prepare(sql);
    for (auto it = bindings.constBegin(); it != bindings.constEnd(); ++it)
        query.bindValue(it.key(), it.value());
    
    if (query.exec())
    {
        while (query.next())
        {
            AssetMetadata asset;
            asset.id = query.value("id").toInt();
            asset.name = query.value("name").toString();
            asset.type = stringToAssetType(query.value("type").toString());
            asset.author = query.value("author").toString();
            asset.description = query.value("description").toString();
            asset.tags = query.value("tags").toString();
            asset.filePath = query.value("filePath").toString();
            asset.thumbnailPath = query.value("thumbnailPath").toString();
            asset.rating = query.value("rating").toFloat();
            asset.downloads = query.value("downloads").toInt();
            asset.createdDate = QDateTime::fromString(query.value("createdDate").toString(), Qt::ISODate);
            asset.modifiedDate = QDateTime::fromString(query.value("modifiedDate").toString(), Qt::ISODate);
            asset.fileSize = query.value("fileSize").toLongLong();
            asset.hash = query.value("hash").toString();
            asset.version = query.value("version").toString();
            asset.isFavorite = query.value("isFavorite").toInt() == 1;
            asset.isLocal = query.value("isLocal").toInt() == 1;
            asset.isCloud = query.value("isCloud").toInt() == 1;
            asset.cloudUrl = query.value("cloudUrl").toString();
            
            assets.append(asset);
        }
    }
    
    return assets;
}

QStringList DatabaseManager::getAllTags()
{
    QStringList tags;
    
    QSqlQuery query(m_database);
    if (query.exec("SELECT name FROM tags ORDER BY name"))
    {
        while (query.next())
        {
            tags.append(query.value(0).toString());
        }
    }
    
    return tags;
}

QStringList DatabaseManager::getAssetTags(int assetId)
{
    QStringList tags;
    
    QSqlQuery query(m_database);
    query.prepare("SELECT t.name FROM tags t "
                 "INNER JOIN asset_tags at ON t.id = at.tagId "
                 "WHERE at.assetId = ?");
    query.addBindValue(assetId);
    
    if (query.exec())
    {
        while (query.next())
        {
            tags.append(query.value(0).toString());
        }
    }
    
    return tags;
}

bool DatabaseManager::addTagToAsset(int assetId, const QString& tag)
{
    QSqlQuery query(m_database);
    
    // Insert tag if not exists
    query.prepare("INSERT OR IGNORE INTO tags (name) VALUES (?)");
    query.addBindValue(tag);
    query.exec();
    
    // Get tag ID
    query.prepare("SELECT id FROM tags WHERE name = ?");
    query.addBindValue(tag);
    if (!query.exec() || !query.next())
        return false;
    
    int tagId = query.value(0).toInt();
    
    // Link tag to asset
    query.prepare("INSERT OR IGNORE INTO asset_tags (assetId, tagId) VALUES (?, ?)");
    query.addBindValue(assetId);
    query.addBindValue(tagId);
    
    return query.exec();
}

bool DatabaseManager::removeTagFromAsset(int assetId, const QString& tag)
{
    QSqlQuery query(m_database);
    
    query.prepare("DELETE FROM asset_tags WHERE assetId = ? AND tagId = (SELECT id FROM tags WHERE name = ?)");
    query.addBindValue(assetId);
    query.addBindValue(tag);
    
    return query.exec();
}

int DatabaseManager::getAssetCount(ACAssetType type)
{
    QSqlQuery query(m_database);
    QString sql = "SELECT COUNT(*) FROM assets";
    
    if (type != ACAssetType::Generic)
    {
        sql += " WHERE type = :type";
    }
    
    query.prepare(sql);
    if (type != ACAssetType::Generic)
        query.bindValue(":type", assetTypeToString(type));
    
    if (query.exec() && query.next())
    {
        return query.value(0).toInt();
    }
    
    return 0;
}

QMap<QString, int> DatabaseManager::getAssetCountsByAuthor()
{
    QMap<QString, int> counts;
    
    QSqlQuery query(m_database);
    if (query.exec("SELECT author, COUNT(*) FROM assets GROUP BY author"))
    {
        while (query.next())
        {
            counts[query.value(0).toString()] = query.value(1).toInt();
        }
    }
    
    return counts;
}

bool DatabaseManager::createTables()
{
    QSqlQuery query(m_database);
    
    // Assets table is created in setupDatabase
    
    // Tags table
    query.exec("CREATE TABLE IF NOT EXISTS tags ("
              "id INTEGER PRIMARY KEY AUTOINCREMENT,"
              "name TEXT UNIQUE NOT NULL"
              ")");
    
    // Asset-Tags junction
    query.exec("CREATE TABLE IF NOT EXISTS asset_tags ("
              "assetId INTEGER,"
              "tagId INTEGER,"
              "PRIMARY KEY (assetId, tagId)"
              ")");
    
    return true;
}

// ============================================================================
// ACThumbnailGenerator Implementation
// ============================================================================
QString ACThumbnailGenerator::generate(const QString& sourcePath, const QString& outputDir, int maxSize)
{
    QFileInfo fileInfo(sourcePath);
    QString ext = fileInfo.suffix().toLower();
    
    // Image formats
    if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" || ext == "dds" || ext == "tga")
    {
        return generateFromImage(sourcePath, outputDir);
    }
    
    // 3D model formats
    if (ext == "kn5" || ext == "fbx" || ext == "obj" || ext == "glb" || ext == "gltf")
    {
        return generateFrom3DModel(sourcePath, outputDir);
    }
    
    // Font formats
    if (ext == "ttf" || ext == "otf" || ext == "fon")
    {
        return generateFromFont(sourcePath, outputDir);
    }
    
    // Audio formats
    if (ext == "wav" || ext == "ogg" || ext == "mp3" || ext == "flac")
    {
        return generateFromAudio(sourcePath, outputDir);
    }
    
    // Video formats
    if (ext == "mp4" || ext == "avi" || ext == "mkv" || ext == "mov" || ext == "wmv" || ext == "flv" || ext == "webm")
    {
        return generateFromVideo(sourcePath, outputDir);
    }
    
    // Archives (might contain images)
    if (ext == "zip" || ext == "7z" || ext == "rar")
    {
        return generateFromArchive(sourcePath, outputDir);
    }
    
    return QString();
}

QString ACThumbnailGenerator::generateFromImage(const QString& imagePath, const QString& outputDir)
{
    QFileInfo fileInfo(imagePath);
    QString outputPath = outputDir + "/" + fileInfo.baseName() + "_thumb.png";
    
    QPixmap pixmap(imagePath);
    if (pixmap.isNull()) return QString();
    
    // Ensure output directory exists
    QDir().mkpath(outputDir);
    
    // Scale and save
    QPixmap scaled = pixmap.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    if (scaled.save(outputPath))
    {
        return outputPath;
    }
    
    return QString();
}

QString ACThumbnailGenerator::generateFrom3DModel(const QString& modelPath, const QString& outputDir)
{
    // Generate thumbnail from 3D model by rendering a preview
    QImage image(256, 256, QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw a simple 3D cube representation
    painter.setBrush(QColor(100, 150, 200));
    painter.setPen(Qt::NoPen);

    QPointF points[] = {
        {128, 40}, {200, 80}, {200, 160}, {128, 200},
        {56, 160}, {56, 80}
    };

    painter.drawPolygon(points, 6);
    painter.setBrush(QColor(80, 120, 160));
    QPointF topPoints[] = {{128, 40}, {200, 80}, {128, 120}, {56, 80}};
    painter.drawPolygon(topPoints, 4);

    QString outputPath = outputDir + "/" + QFileInfo(modelPath).baseName() + "_thumb.png";
    if (image.save(outputPath)) {
        return outputPath;
    }
    return QString();
}

QString ACThumbnailGenerator::generateFromFont(const QString& fontPath, const QString& outputDir)
{
    // Generate font preview thumbnail
    QImage image(256, 256, QImage::Format_ARGB32);
    image.fill(Qt::white);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);

    QFont font(fontPath, 48);
    painter.setFont(font);
    painter.setPen(Qt::black);
    painter.drawText(image.rect(), Qt::AlignCenter, "Aa");

    QString outputPath = outputDir + "/" + QFileInfo(fontPath).baseName() + "_thumb.png";
    if (image.save(outputPath)) {
        return outputPath;
    }
    return QString();
}

QString ACThumbnailGenerator::generateFromAudio(const QString& audioPath, const QString& outputDir)
{
    // Generate audio waveform thumbnail
    QImage image(256, 128, QImage::Format_ARGB32);
    image.fill(Qt::white);

    QPainter painter(&image);
    painter.setPen(QColor(50, 100, 200));

    // Draw a simple waveform representation
    for (int x = 0; x < 256; ++x) {
        float t = x / 256.0f;
        float amplitude = std::sin(t * 20.0f) * 0.5f + std::sin(t * 50.0f) * 0.3f;
        int y = 64 + static_cast<int>(amplitude * 50.0f);
        painter.drawPoint(x, y);
    }

    QString outputPath = outputDir + "/" + QFileInfo(audioPath).baseName() + "_thumb.png";
    if (image.save(outputPath)) {
        return outputPath;
    }
    return QString();
}
}

namespace ks {

// ============================================================================
// FileHashCalculator Implementation
// ============================================================================
QString FileHashCalculator::calculateMD5(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return QString();
    
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(&file);
    file.close();
    
    return hash.result().toHex();
}

QString FileHashCalculator::calculateSHA256(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return QString();
    
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(&file);
    file.close();
    
    return hash.result().toHex();
}

bool FileHashCalculator::verifyHash(const QString& filePath, const QString& expectedHash)
{
    QString actualHash = calculateMD5(filePath);
    return actualHash.toLower() == expectedHash.toLower();
}

// ============================================================================
// CloudUploader Implementation
// ============================================================================
CloudUploader::CloudUploader(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

void CloudUploader::upload(const AssetMetadata& asset, const QString& endpoint, const QString& apiKey)
{
    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    
    // Add file part
    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, 
        QString("form-data; name=\"file\"; filename=\"%1\"").arg(QFileInfo(asset.filePath).fileName()));
    
    QFile* file = new QFile(asset.filePath);
    file->open(QIODevice::ReadOnly);
    filePart.setBodyDevice(file);
    multiPart->append(filePart);
    
    // Add metadata parts
    QHttpPart namePart;
    namePart.setHeader(QNetworkRequest::ContentDispositionHeader, 
        QString("form-data; name=\"name\""));
    namePart.setBody(asset.name.toUtf8());
    multiPart->append(namePart);
    
    QHttpPart typePart;
    typePart.setHeader(QNetworkRequest::ContentDispositionHeader, 
        QString("form-data; name=\"type\""));
    typePart.setBody(assetTypeToString(asset.type).toUtf8());
    multiPart->append(typePart);
    
    // Send request
    QNetworkRequest request(QUrl(endpoint + "/upload"));
    request.setRawHeader("Authorization", ("Bearer " + apiKey).toUtf8());
    
    QNetworkReply* reply = m_networkManager->post(request, multiPart);
    
    connect(reply, &QNetworkReply::uploadProgress, this, &CloudUploader::uploadProgress);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError)
        {
            emit uploadFinished(true, "Upload successful");
        }
        else
        {
            emit uploadError(reply->errorString());
        }
        reply->deleteLater();
    });
}

// ============================================================================
// CloudDownloader Implementation
// ============================================================================
CloudDownloader::CloudDownloader(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

void CloudDownloader::download(const AssetMetadata& asset, const QString& endpoint, const QString& apiKey, const QString& savePath)
{
    QNetworkRequest request(QUrl(asset.cloudUrl));
    request.setRawHeader("Authorization", ("Bearer " + apiKey).toUtf8());
    
    QNetworkReply* reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::downloadProgress, this, &CloudDownloader::downloadProgress);
    connect(reply, &QNetworkReply::finished, this, [this, reply, savePath]() {
        if (reply->error() == QNetworkReply::NoError)
        {
            QFile file(savePath);
            if (file.open(QIODevice::WriteOnly))
            {
                file.write(reply->readAll());
                file.close();
                emit downloadFinished(true, "Download successful");
            }
            else
            {
                emit downloadError("Failed to save file");
            }
        }
        else
        {
            emit downloadError(reply->errorString());
        }
        reply->deleteLater();
    });
}

void AssetImportDialog::onBrowseFile()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Select Asset File"), QString(),
        tr("All Files (*.*)"));
    if (!path.isEmpty()) {
        m_filePathEdit->setText(path);
        if (m_nameEdit->text().isEmpty()) {
            m_nameEdit->setText(QFileInfo(path).baseName());
        }
    }
}

void AssetImportDialog::onBrowseThumbnail()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Select Thumbnail"), QString(),
        tr("Images (*.png *.jpg *.bmp)"));
    if (!path.isEmpty()) {
        m_thumbnailEdit->setText(path);
    }
}

void AssetImportDialog::onImport()
{
    m_metadata.name = m_nameEdit->text();
    m_metadata.filePath = m_filePathEdit->text();
    m_metadata.type = (ACAssetType)m_typeCombo->currentIndex();
    m_metadata.author = m_authorEdit->text();
    m_metadata.description = m_descriptionEdit->toPlainText();
    m_metadata.tags = m_tagsEdit->text();
    m_metadata.thumbnailPath = m_thumbnailEdit->text();
    m_metadata.version = m_versionEdit->text();
    accept();
}

void AssetsLibraryModule::onCloudSyncFinished()
{
    m_syncStatusLabel->setText("Sync complete");
    m_syncInProgress = false;
    m_syncProgress->setValue(100);
}

void AssetsLibraryModule::onCloudSyncError(const QString& error)
{
    m_syncStatusLabel->setText("Sync error: " + error);
    m_syncStatusLabel->setStyleSheet("color: #f44336;");
    m_syncInProgress = false;
}

void AssetsLibraryModule::onSyncProgress(qint64 bytesSent, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        int percent = static_cast<int>((bytesSent * 100) / bytesTotal);
        m_syncProgress->setValue(percent);
        m_syncStatusLabel->setText(QString("Syncing... %1%").arg(percent));
    }
}

void SyncManager::onNetworkReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();
}

} // namespace ks
