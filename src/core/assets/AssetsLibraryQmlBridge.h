#pragma once

#include "AssetManager.h"
#include "AssetSearchEngine.h"
#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

namespace ks {

class AssetsLibraryQmlBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
    Q_PROPERTY(int assetCount READ assetCount NOTIFY assetsChanged)
    Q_PROPERTY(bool watching READ isWatching NOTIFY watchingChanged)
    Q_PROPERTY(int duplicateCount READ duplicateCount NOTIFY duplicatesFound)
    Q_PROPERTY(bool importing READ importing NOTIFY importingChanged)

public:
    static AssetsLibraryQmlBridge* instance();

    bool scanning() const { return m_scanning; }
    int assetCount() const { return m_assets.size(); }
    bool isWatching() const;
    int duplicateCount() const { return m_duplicateCount; }
    bool importing() const { return m_importing; }

    Q_INVOKABLE void scan(const QString& directory);
    Q_INVOKABLE QVariantList getAssets(const QString& category = "",
                                       const QString& searchQuery = "",
                                       const QString& typeFilter = "");
    Q_INVOKABLE QVariantMap getAsset(const QString& assetId);
    Q_INVOKABLE QStringList getCategories();
    Q_INVOKABLE QStringList getTags();
    Q_INVOKABLE QVariantList search(const QString& query);
    Q_INVOKABLE bool importAsset(const QString& sourcePath, const QString& destDir);
    Q_INVOKABLE bool exportAsset(const QString& assetId, const QString& destinationPath);
    Q_INVOKABLE bool removeAsset(const QString& assetId);
    Q_INVOKABLE QString getAssetPath(const QString& assetId) const;
    Q_INVOKABLE QVariantMap getAssetDetails(const QString& assetId);

    Q_INVOKABLE void addTag(const QString& assetId, const QString& tag);
    Q_INVOKABLE void removeTag(const QString& assetId, const QString& tag);

    Q_INVOKABLE QVariantList getFavorites();
    Q_INVOKABLE void toggleFavorite(const QString& assetId);
    Q_INVOKABLE bool isFavorite(const QString& assetId) const;

    Q_INVOKABLE QVariantList getRecentAssets();
    Q_INVOKABLE QString getThumbnail(const QString& assetId);
    Q_INVOKABLE QVariantMap getStorageStats();

    Q_INVOKABLE bool openInModeler(const QString& assetId);

    // File watcher
    Q_INVOKABLE void startWatching(const QString& directory);
    Q_INVOKABLE void stopWatching();
    Q_INVOKABLE bool isWatchActive() const;

    // Deduplication
    Q_INVOKABLE QVariantList findDuplicates();
    Q_INVOKABLE QVariantList getDuplicateGroups();
    Q_INVOKABLE void removeDuplicates(bool keepFirst = true);
    Q_INVOKABLE void scanWithDedup();
    Q_INVOKABLE bool isAssetDuplicate(const QString& assetId) const;
    Q_INVOKABLE QString getOriginalAssetId(const QString& duplicateId) const;

    // Batch import
    Q_INVOKABLE void importDirectoryAsync(const QString& directory);
    Q_INVOKABLE void importFilesAsync(const QStringList& filePaths);
    Q_INVOKABLE void importWithConversionAsync(const QString& sourcePath, const QString& targetFormat);
    Q_INVOKABLE QStringList getSupportedConversions();

signals:
    void scanningChanged();
    void assetsChanged();
    void scanProgress(int current, int total);
    void scanComplete(int assetCount);
    void importComplete(const QString& assetId, bool success);
    void error(const QString& message);
    void assetOpenedInModeler(const QString& path);
    void watchingChanged();
    void duplicatesFound(int count);
    void fileSystemChanged(const QString& path);

    // Batch import signals
    void importingChanged();
    void batchImportStarted(int fileCount);
    void batchImportProgress(int current, int total, const QString& currentFile);
    void batchImportCompleted(const QVariantMap& summary);
    void batchImportFileProcessed(int current, int total, bool success, const QString& file);

private:
    AssetsLibraryQmlBridge(QObject* parent = nullptr);
    static AssetsLibraryQmlBridge* s_instance;

    AssetManager* assetManager() const;

    QVector<Asset> m_assets;
    bool m_scanning = false;
    bool m_importing = false;
    int m_duplicateCount = 0;
};

} // namespace ks
