#pragma once

#include <QWidget>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QGroupBox>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QScrollArea>
#include <QDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QSplitter>
#include <QProgressBar>
#include <QSortFilterProxyModel>
#include <QFileSystemModel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlTableModel>

#include "core/sys/ModuleManager.h"

namespace ks {

// ============================================================================
// Asset Types
// ============================================================================
enum class ACAssetType
{
    Car,
    Track,
    Skin,
    Texture,
    Sound,
    Font,
    Physics,
    AI,
    Generic
};

QString assetTypeToString(ACAssetType type);
ACAssetType stringToAssetType(const QString& str);

// ============================================================================
// Asset Metadata
// ============================================================================
struct AssetMetadata
{
    int id;                    // Database ID
    QString name;              // Asset name
    ACAssetType type;            // Asset type
    QString author;            // Author/creator
    QString description;       // Description
    QString tags;              // Tags (comma separated)
    QString filePath;          // Local file path
    QString thumbnailPath;      // Thumbnail image path
    QString originalPath;      // Original source path
    float rating;              // Community rating (0-5)
    int downloads;             // Download count
    int likes;                 // Like count
    QDateTime createdDate;     // Creation date
    QDateTime modifiedDate;    // Last modified date
    QDateTime uploadedDate;    // Upload to cloud date
    qint64 fileSize;           // File size in bytes
    QString hash;              // File hash (MD5/SHA256)
    QString version;           // Asset version
    QString gameVersion;       // Compatible game version
    bool isFavorite;           // User's favorite
    bool isLocal;             // Is locally available
    bool isCloud;             // Is synced to cloud
    QString cloudUrl;          // Cloud storage URL
    QString previewImages;     // Comma-separated preview image paths
};

// ============================================================================
// Asset Filters
// ============================================================================
struct AssetFilter
{
    ACAssetType type;            // Filter by type (-1 = all)
    QString searchText;        // Search text
    QString author;           // Filter by author
    QString tags;             // Required tags (comma separated)
    float minRating;          // Minimum rating
    int minDownloads;         // Minimum downloads
    bool favoritesOnly;       // Show favorites only
    bool localOnly;           // Show local only
    bool cloudOnly;           // Show cloud only
    QString sortBy;           // Sort field
    bool sortAscending;       // Sort direction
};

// ============================================================================
// Cloud Sync Status
// ============================================================================
enum class SyncStatus
{
    NotSynced,
    Syncing,
    Synced,
    Conflict,
    Error
};

struct SyncState
{
    QString assetPath;
    SyncStatus status;
    QString lastSyncTime;
    QString errorMessage;
    float progress;
};

// ============================================================================
// AssetsLibraryModule - Asset management with cloud sync
// ============================================================================
class AssetsLibraryModule : public EditorModule
{
    Q_OBJECT

public:
    explicit AssetsLibraryModule(QWidget* parent = nullptr);
    ~AssetsLibraryModule();

    // EditorModule overrides
    QString getModuleName() const override { return "Assets Library"; }
    QString moduleId() const override { return "assetsLibraryPlugin"; }
    QString getModuleIcon() const override { return ":/icons/library.png"; }
    int getModulePriority() const override { return 10; }
    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;

protected:
    void onActivation() override;
    void onDeactivation() override;

private slots:
    // File operations
    void onImportAsset();
    void onExportAsset();
    void onDeleteAsset();
    void onDuplicateAsset();
    
    // Asset selection
    void onAssetSelected(const QModelIndex& index);
    void onAssetDoubleClicked(const QModelIndex& index);
    
    // Filtering and search
    void onSearchTextChanged(const QString& text);
    void onTypeFilterChanged(int index);
    void onAuthorFilterChanged(const QString& author);
    void onSortChanged(int index);
    
    // Cloud operations
    void onUploadToCloud();
    void onDownloadFromCloud();
    void onSyncAsset();
    void onSyncAll();
    void onCancelSync();
    
    // Local file operations
    void onOpenFileLocation();
    void onRevealInExplorer();
    
    // Favorite operations
    void onToggleFavorite();
    
    // Tag operations
    void onAddTag();
    void onRemoveTag();
    void onManageTags();
    
    // Settings
    void onSettings();
    
    // Preview
    void onPreviewAsset();
    
    // Cloud status updates
    void onCloudSyncFinished();
    void onCloudSyncError(const QString& error);
    void onSyncProgress(qint64 bytesSent, qint64 bytesTotal);

private:
    void setupUi();
    void setupConnections();
    void setupDatabase();
    void setupCloudManager();
    void updateAssetList();
    void loadAssetDetails(const AssetMetadata& asset);
    void syncWithCloud();
    void generateThumbnail(const QString& filePath);
    QString calculateFileHash(const QString& filePath);
    void queueCloudUpload(const AssetMetadata& asset);
    void queueCloudDownload(const AssetMetadata& asset);
    void saveCurrentAsset();
    QString formatFileSize(qint64 bytes);

    // UI Components
    QWidget* m_centralWidget;
    QDockWidget* m_dockWidget;
    
    // Left panel - Categories tree
    QTreeWidget* m_categoryTree;
    QPushButton* m_importBtn;
    QPushButton* m_syncAllBtn;
    QLabel* m_syncStatusLabel;
    QProgressBar* m_syncProgress;
    
    // Center panel - Asset list
    QTableView* m_assetTable;
    QSortFilterProxyModel* m_proxyModel;
    QSqlTableModel* m_assetModel;
    QLineEdit* m_searchEdit;
    QComboBox* m_typeFilterCombo;
    QComboBox* m_sortCombo;
    QPushButton* m_clearFilterBtn;
    
    // Right panel - Asset details
    QScrollArea* m_detailsScrollArea;
    QWidget* m_detailsWidget;
    QLabel* m_previewLabel;
    QLineEdit* m_nameEdit;
    QLineEdit* m_authorEdit;
    QTextEdit* m_descriptionEdit;
    QLineEdit* m_tagsEdit;
     QLabel* m_typeLabel;
     QLabel* m_sizeLabel;
     QLabel* m_pathLabel;
     QLabel* m_ratingLabel;
    QLabel* m_downloadsLabel;
    QLabel* m_createdDateLabel;
    QLabel* m_modifiedDateLabel;
    QLabel* m_cloudStatusLabel;
    QPushButton* m_favoriteBtn;
    QPushButton* m_previewBtn;
    QPushButton* m_openFileBtn;
    QPushButton* m_cloudUploadBtn;
    QPushButton* m_cloudDownloadBtn;
    
    // Tag management
    QListWidget* m_tagList;
    QPushButton* m_addTagBtn;
    QPushButton* m_removeTagBtn;
    
    // Cloud sync queue
    QListWidget* m_syncQueueList;
    QPushButton* m_cancelSyncBtn;
    
    // Database
    QSqlDatabase m_database;
    QString m_databasePath;
    
    // Cloud sync
    QNetworkAccessManager* m_networkManager;
    QVector<SyncState> m_syncQueue;
    bool m_syncInProgress;
    
    // Current selection
    int m_currentAssetId;
    AssetMetadata m_currentAsset;
    
    // Settings
    QString m_cloudEndpoint;
    QString m_apiKey;
    QString m_localAssetsPath;
    QString m_thumbnailsPath;

    // Filter state
    QString m_searchFilter;
    int m_typeFilter = -1;
    QString m_authorFilter;
    int m_sortOrder = 0;
};

// ============================================================================
// AssetImportDialog - Import new asset with metadata
// ============================================================================
class AssetImportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AssetImportDialog(QWidget* parent = nullptr);
    void setAssetType(ACAssetType type);
    AssetMetadata getAssetMetadata() const;

private slots:
    void onBrowseFile();
    void onBrowseThumbnail();
    void onImport();

private:
    AssetMetadata m_metadata;
    QLineEdit* m_filePathEdit;
    QLineEdit* m_nameEdit;
    QComboBox* m_typeCombo;
    QLineEdit* m_authorEdit;
    QTextEdit* m_descriptionEdit;
    QLineEdit* m_tagsEdit;
    QLineEdit* m_thumbnailEdit;
    QLineEdit* m_versionEdit;
};

// ============================================================================
// CloudSettingsDialog - Configure cloud sync settings
// ============================================================================
class CloudSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CloudSettingsDialog(QWidget* parent = nullptr);

private slots:
    void onTestConnection();
    void onSave();

private:
    QLineEdit* m_endpointEdit;
    QLineEdit* m_apiKeyEdit;
    QLineEdit* m_usernameEdit;
    QPushButton* m_testBtn;
    QLabel* m_statusLabel;
    QNetworkAccessManager* m_networkManager = nullptr;
};

// ============================================================================
// TagManagerDialog - Manage asset tags
// ============================================================================
class TagManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TagManagerDialog(QWidget* parent = nullptr);
    QStringList getSelectedTags() const { return m_selectedTags; }

private slots:
    void onAddTag();
    void onRemoveTag();
    void onCreateTag();
    void onApply();

private:
    QStringList m_selectedTags;
    QStringList m_allTags;
    QListWidget* m_tagList;
};

// ============================================================================
// SyncManager - Handle cloud sync operations
// ============================================================================
class SyncManager : public QObject
{
    Q_OBJECT

public:
    explicit SyncManager(QObject* parent = nullptr);
    void setEndpoint(const QString& endpoint);
    void setApiKey(const QString& apiKey);
    
    void queueUpload(const AssetMetadata& asset);
    void queueDownload(const AssetMetadata& asset);
    void cancelAll();
    
    bool isSyncing() const { return m_syncing; }
    float progress() const { return m_progress; }

signals:
    void syncStarted();
    void syncProgress(float progress);
    void syncFinished();
    void syncError(const QString& error);
    void assetUploaded(const AssetMetadata& asset);
    void assetDownloaded(const AssetMetadata& asset);

private slots:
    void onNetworkReplyFinished();
    void processNextInQueue();

private:
    struct SyncItem
    {
        AssetMetadata asset;
        bool isUpload;
    };
    
    QVector<SyncItem> m_queue;
    QNetworkAccessManager* m_networkManager;
    QString m_endpoint;
    QString m_apiKey;
    bool m_syncing;
    float m_progress;
    int m_currentIndex;
};

// ============================================================================
// DatabaseManager - Handle SQLite database operations
// ============================================================================
class DatabaseManager
{
public:
    static DatabaseManager& instance();
    
    bool initialize(const QString& databasePath);
    void close();
    
    // Asset CRUD operations
    int insertAsset(const AssetMetadata& asset);
    bool updateAsset(const AssetMetadata& asset);
    bool deleteAsset(int assetId);
    AssetMetadata getAsset(int assetId);
    QList<AssetMetadata> getAllAssets();
    QList<AssetMetadata> queryAssets(const AssetFilter& filter);
    
    // Tag operations
    QStringList getAllTags();
    QStringList getAssetTags(int assetId);
    bool addTagToAsset(int assetId, const QString& tag);
    bool removeTagFromAsset(int assetId, const QString& tag);
    
    // Statistics
    int getAssetCount(ACAssetType type = ACAssetType::Generic);
    QMap<QString, int> getAssetCountsByAuthor();
    
private:
    DatabaseManager() = default;
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;
    
    bool createTables();
    
    QSqlDatabase m_database;
    QString m_databasePath;
};

// ============================================================================
// ThumbnailGenerator - Generate asset thumbnails
// ============================================================================
class ACThumbnailGenerator
{
public:
    static QString generate(const QString& sourcePath, const QString& outputDir, int maxSize = 256);
    static QString generateFromImage(const QString& imagePath, const QString& outputDir);
    static QString generateFrom3DModel(const QString& modelPath, const QString& outputDir);
    static QString generateFromFont(const QString& fontPath, const QString& outputDir);
    static QString generateFromAudio(const QString& audioPath, const QString& outputDir);
    static QString generateFromArchive(const QString& archivePath, const QString& outputDir);
    static QString generateFromVideo(const QString& videoPath, const QString& outputDir);
};

// ============================================================================
// FileHashCalculator - Calculate file hashes
// ============================================================================
class FileHashCalculator
{
public:
    static QString calculateMD5(const QString& filePath);
    static QString calculateSHA256(const QString& filePath);
    static bool verifyHash(const QString& filePath, const QString& expectedHash);
};

// ============================================================================
// CloudUploader - Upload assets to cloud storage
// ============================================================================
class CloudUploader : public QObject
{
    Q_OBJECT

public:
    explicit CloudUploader(QObject* parent = nullptr);
    void upload(const AssetMetadata& asset, const QString& endpoint, const QString& apiKey);

signals:
    void uploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void uploadFinished(bool success, const QString& message);
    void uploadError(const QString& error);

private:
    QNetworkAccessManager* m_networkManager;
};

// ============================================================================
// CloudDownloader - Download assets from cloud storage
// ============================================================================
class CloudDownloader : public QObject
{
    Q_OBJECT

public:
    explicit CloudDownloader(QObject* parent = nullptr);
    void download(const AssetMetadata& asset, const QString& endpoint, const QString& apiKey, const QString& savePath);

signals:
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(bool success, const QString& message);
    void downloadError(const QString& error);

private:
    QNetworkAccessManager* m_networkManager;
};

// ============================================================================
// AssetCollection - Group assets into collections
// ============================================================================
class ACAssetCollection
{
public:
    int id;
    QString name;
    QString description;
    QVector<int> assetIds;
    QDateTime createdDate;
    bool isShared;
};

} // namespace ks
