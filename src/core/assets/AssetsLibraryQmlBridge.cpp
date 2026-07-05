#include "AssetsLibraryQmlBridge.h"
#include "FileFormat/FormatConverter.h"
#include "modules/modellingEditor/3DModelingQmlBridge.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QDirIterator>
#include <QStorageInfo>

namespace ks {

// ----------------------------------------------------------------------------
// Helper: convert AssetType to human-readable string for QML
// ----------------------------------------------------------------------------
static QString assetTypeString(AssetType type)
{
    switch (type) {
        case AssetType::Model: return "Model";
        case AssetType::Texture: return "Texture";
        case AssetType::Audio: return "Audio";
        case AssetType::Material: return "Material";
        case AssetType::Physics: return "Physics";
        case AssetType::Animation: return "Animation";
        case AssetType::Scene: return "Scene";
        case AssetType::Bundle: return "Bundle";
        case AssetType::Config: return "Config";
        case AssetType::Script: return "Script";
        case AssetType::Font: return "Font";
        case AssetType::Mesh: return "Mesh";
        default: return "Unknown";
    }
}

AssetsLibraryQmlBridge* AssetsLibraryQmlBridge::s_instance = nullptr;

AssetsLibraryQmlBridge* AssetsLibraryQmlBridge::instance()
{
    if (!s_instance) {
        s_instance = new AssetsLibraryQmlBridge();
    }
    return s_instance;
}

AssetsLibraryQmlBridge::AssetsLibraryQmlBridge(QObject* parent)
    : QObject(parent)
{
    AssetManager* mgr = AssetManager::instance();
    connect(mgr, &AssetManager::scanComplete, this, [this](int count) {
        m_scanning = false;
        m_assets = AssetManager::instance()->getAssets();
        emit scanningChanged();
        emit assetsChanged();
        emit scanComplete(count);
    });
    connect(mgr, &AssetManager::assetAdded, this, [this](const Asset& asset) {
        m_assets.append(asset);
        emit assetsChanged();
    });
    connect(mgr, &AssetManager::assetRemoved, this, [this](const QString&) {
        m_assets = AssetManager::instance()->getAssets();
        emit assetsChanged();
    });
    connect(mgr, &AssetManager::assetImported, this, [this](const Asset& asset) {
        emit importComplete(asset.id, true);
    });
    connect(mgr, &AssetManager::importFailed, this, [this](const QString& path, const QString& reason) {
        Q_UNUSED(path)
        emit error(reason);
    });
    connect(mgr, &AssetManager::batchImportStarted, this, [this](int fileCount) {
        m_importing = true;
        emit importingChanged();
        emit batchImportStarted(fileCount);
    });
    connect(mgr, &AssetManager::batchImportProgress, this, [this](int current, int total, const QString& currentFile) {
        emit batchImportProgress(current, total, currentFile);
    });
    connect(mgr, &AssetManager::batchImportFileProcessed, this, [this](int current, int total, bool success, const QString& file) {
        emit batchImportFileProcessed(current, total, success, file);
    });
    connect(mgr, &AssetManager::batchImportCompleted, this, [this](const QVariantMap& summary) {
        m_importing = false;
        m_assets = AssetManager::instance()->getAssets();
        emit importingChanged();
        emit assetsChanged();
        emit batchImportCompleted(summary);
    });
}

AssetManager* AssetsLibraryQmlBridge::assetManager() const
{
    return AssetManager::instance();
}

void AssetsLibraryQmlBridge::scan(const QString& directory)
{
    if (m_scanning) return;
    m_scanning = true;
    emit scanningChanged();

    AssetManager* mgr = assetManager();
    mgr->setRootDirectory(directory);
    mgr->scan();
}

QVariantList AssetsLibraryQmlBridge::getAssets(const QString& category,
                                                const QString& searchQuery,
                                                const QString& typeFilter)
{
    QVector<Asset> assets = m_assets;
    QVariantList result;

    // Apply filters
    if (!category.isEmpty() && category != "All") {
        QVector<Asset> filtered;
        for (const auto& a : assets) {
            QString cat = a.metadata.value("category").toString();
            if (cat.compare(category, Qt::CaseInsensitive) == 0)
                filtered.append(a);
        }
        assets = filtered;
    }

    if (!typeFilter.isEmpty()) {
        QVector<Asset> filtered;
        for (const auto& a : assets) {
            QString t = assetTypeString(a.type);
            if (t.compare(typeFilter, Qt::CaseInsensitive) == 0)
                filtered.append(a);
        }
        assets = filtered;
    }

    if (!searchQuery.isEmpty()) {
        QString q = searchQuery.toLower();
        QVector<Asset> filtered;
        for (const auto& a : assets) {
            if (a.name.toLower().contains(q) || a.tags.toLower().contains(q))
                filtered.append(a);
        }
        assets = filtered;
    }

    for (const auto& a : assets) {
        QVariantMap m;
        m["id"] = a.id;
        m["name"] = a.name;
        m["type"] = assetTypeString(a.type);
        m["category"] = a.metadata.value("category").toString();
        m["size"] = a.fileSize;
        m["tags"] = a.tags;
        m["modified"] = a.modifiedDate;
        m["thumbnail"] = getThumbnail(a.id);
        m["displayName"] = a.metadata.value("displayName").toString();
        m["author"] = a.metadata.value("author").toString();
        m["brand"] = a.metadata.value("brand").toString();
        result.append(m);
    }

    return result;
}

QVariantMap AssetsLibraryQmlBridge::getAsset(const QString& assetId)
{
    const Asset* a = assetManager()->getAsset(assetId);
    if (!a) return QVariantMap();

    QVariantMap m;
    m["id"] = a->id;
    m["name"] = a->name;
    m["path"] = a->path;
    m["extension"] = a->extension;
    m["type"] = assetTypeString(a->type);
    m["category"] = a->metadata.value("category").toString();
    m["size"] = a->fileSize;
    m["tags"] = a->tags;
    m["modified"] = a->modifiedDate;
    m["isCore"] = a->isCore;
    m["thumbnail"] = getThumbnail(assetId);
    m["displayName"] = a->metadata.value("displayName").toString();
    m["author"] = a->metadata.value("author").toString();
    m["brand"] = a->metadata.value("brand").toString();
    m["year"] = a->metadata.value("year").toString();
    m["country"] = a->metadata.value("country").toString();

    QVariantList deps;
    for (const auto& d : a->dependencies) {
        QVariantMap dm;
        dm["assetId"] = d.assetId;
        dm["path"] = d.path;
        dm["type"] = static_cast<int>(d.type);
        deps.append(dm);
    }
    m["dependencies"] = deps;
    return m;
}

QStringList AssetsLibraryQmlBridge::getCategories()
{
    QSet<QString> cats;
    cats.insert("All");
    for (const auto& a : m_assets) {
        QString cat = a.metadata.value("category").toString();
        if (!cat.isEmpty()) cats.insert(cat);
    }
    return cats.values();
}

QStringList AssetsLibraryQmlBridge::getTags()
{
    return assetManager()->getTags();
}

QVariantList AssetsLibraryQmlBridge::search(const QString& query)
{
    QVector<Asset> results = assetManager()->search(query);
    QVariantList list;
    for (const auto& a : results) {
        QVariantMap m;
        m["id"] = a.id;
        m["name"] = a.name;
        m["type"] = assetTypeString(a.type);
        m["path"] = a.path;
        list.append(m);
    }
    return list;
}

bool AssetsLibraryQmlBridge::importAsset(const QString& sourcePath, const QString& destDir)
{
    bool ok = assetManager()->importAsset(sourcePath, destDir);
    if (ok) {
        AssetSearchEngine searchEngine;
        searchEngine.loadIndex(assetManager()->getRootDirectory() + "/.asset_index.json");
        QFileInfo fi(sourcePath);
        searchEngine.indexFile(fi.filePath());
        searchEngine.saveIndex(assetManager()->getRootDirectory() + "/.asset_index.json");
    }
    return ok;
}

bool AssetsLibraryQmlBridge::exportAsset(const QString& assetId, const QString& destinationPath)
{
    QString src = assetManager()->getAssetPath(assetId);
    if (src.isEmpty()) return false;
    return QFile::copy(src, destinationPath);
}

bool AssetsLibraryQmlBridge::removeAsset(const QString& assetId)
{
    return assetManager()->removeAsset(assetId);
}

QString AssetsLibraryQmlBridge::getAssetPath(const QString& assetId) const
{
    return assetManager()->getAssetPath(assetId);
}

QVariantMap AssetsLibraryQmlBridge::getAssetDetails(const QString& assetId)
{
    return getAsset(assetId);
}

void AssetsLibraryQmlBridge::addTag(const QString& assetId, const QString& tag)
{
    assetManager()->addTag(assetId, tag);
}

void AssetsLibraryQmlBridge::removeTag(const QString& assetId, const QString& tag)
{
    assetManager()->removeTag(assetId, tag);
}

QVariantList AssetsLibraryQmlBridge::getFavorites()
{
    QStringList favs = assetManager()->getFavorites();
    QVariantList result;
    for (const auto& id : favs) {
        QVariantMap m;
        m["id"] = id;
        const Asset* a = assetManager()->getAsset(id);
        if (a) {
            m["name"] = a->name;
            m["type"] = assetTypeString(a->type);
        }
        result.append(m);
    }
    return result;
}

void AssetsLibraryQmlBridge::toggleFavorite(const QString& assetId)
{
    QStringList favs = assetManager()->getFavorites();
    if (favs.contains(assetId)) {
        favs.removeAll(assetId);
    } else {
        favs.append(assetId);
    }
    assetManager()->setFavorites(favs);
}

bool AssetsLibraryQmlBridge::isFavorite(const QString& assetId) const
{
    return assetManager()->getFavorites().contains(assetId);
}

QVariantList AssetsLibraryQmlBridge::getRecentAssets()
{
    QStringList recent = assetManager()->getRecent();
    QVariantList result;
    for (const auto& id : recent) {
        const Asset* a = assetManager()->getAsset(id);
        if (a) {
            QVariantMap m;
            m["id"] = a->id;
            m["name"] = a->name;
            result.append(m);
        }
    }
    return result;
}

static QString formatFileSize(qint64 bytes)
{
    if (bytes < 1024) return QString::number(bytes) + " B";
    if (bytes < 1024 * 1024) return QString::number(bytes / 1024.0, 'f', 1) + " KB";
    if (bytes < 1024LL * 1024 * 1024) return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
    return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
}

QString AssetsLibraryQmlBridge::getThumbnail(const QString& assetId)
{
    return assetManager()->getThumbnail(assetId);
}

QVariantMap AssetsLibraryQmlBridge::getStorageStats()
{
    QVariantMap stats;
    qint64 totalSize = 0;
    int count = m_assets.size();

    for (const auto& a : m_assets)
        totalSize += a.fileSize;

    stats["assetCount"] = count;
    stats["totalSize"] = totalSize;
    stats["formattedSize"] = formatFileSize(totalSize);

    QString root = assetManager()->getRootDirectory();
    if (!root.isEmpty() && QDir(root).exists()) {
        QStorageInfo storage(root);
        if (storage.isValid()) {
            stats["totalSpace"] = storage.bytesTotal();
            stats["freeSpace"] = storage.bytesAvailable();
            stats["usedSpace"] = storage.bytesTotal() - storage.bytesAvailable();
            double pct = storage.bytesTotal() > 0
                ? 100.0 * (storage.bytesTotal() - storage.bytesAvailable()) / storage.bytesTotal()
                : 0.0;
            stats["usedPercent"] = pct;
            stats["totalFormatted"] = formatFileSize(storage.bytesTotal());
            stats["freeFormatted"] = formatFileSize(storage.bytesAvailable());
        }
    }

    return stats;
}

bool AssetsLibraryQmlBridge::openInModeler(const QString& assetId)
{
    const Asset* a = assetManager()->getAsset(assetId);
    if (!a || a->path.isEmpty()) {
        emit error("Asset not found or has no path");
        return false;
    }
    bool ok = KSModelerQml::instance().importFile(a->path);
    if (ok)
        emit assetOpenedInModeler(a->path);
    else
        emit error("Failed to import " + a->path + " into modeler");
    return ok;
}

// --- File watcher ---

bool AssetsLibraryQmlBridge::isWatching() const
{
    return assetManager()->isWatching();
}

void AssetsLibraryQmlBridge::startWatching(const QString& directory)
{
    assetManager()->startWatching(directory);
    emit watchingChanged();
}

void AssetsLibraryQmlBridge::stopWatching()
{
    assetManager()->stopWatching();
    emit watchingChanged();
}

bool AssetsLibraryQmlBridge::isWatchActive() const
{
    return assetManager()->isWatching();
}

// --- Deduplication ---

QVariantList AssetsLibraryQmlBridge::findDuplicates()
{
    QVector<Asset> dups = assetManager()->findDuplicates();
    QVariantList result;
    for (const auto& d : dups) {
        QVariantMap m;
        m["id"] = d.id;
        m["name"] = d.name;
        m["path"] = d.path;
        m["size"] = d.fileSize;
        m["hash"] = d.contentHash;
        m["isDuplicate"] = d.isDuplicate;
        m["originalAssetId"] = d.originalAssetId;
        if (!d.originalAssetId.isEmpty()) {
            const Asset* orig = assetManager()->getAsset(d.originalAssetId);
            m["originalName"] = orig ? orig->name : "Unknown";
        }
        result.append(m);
    }
    m_duplicateCount = result.size();
    emit duplicatesFound(m_duplicateCount);
    return result;
}

QVariantList AssetsLibraryQmlBridge::getDuplicateGroups()
{
    auto groups = assetManager()->findDuplicateGroups();
    QVariantList result;
    for (const auto& group : groups) {
        QVariantList groupList;
        for (const auto& a : group) {
            QVariantMap m;
            m["id"] = a.id;
            m["name"] = a.name;
            m["path"] = a.path;
            m["size"] = a.fileSize;
            m["hash"] = a.contentHash;
            m["isDuplicate"] = a.isDuplicate;
            groupList.append(m);
        }
        result.append(groupList);
    }
    return result;
}

void AssetsLibraryQmlBridge::removeDuplicates(bool keepFirst)
{
    assetManager()->removeDuplicates(keepFirst);
    m_assets = AssetManager::instance()->getAssets();
    emit assetsChanged();
}

void AssetsLibraryQmlBridge::scanWithDedup()
{
    assetManager()->scanAndDeduplicate();
    m_assets = AssetManager::instance()->getAssets();
    m_duplicateCount = assetManager()->findDuplicates().size();
    emit assetsChanged();
    emit duplicatesFound(m_duplicateCount);
}

bool AssetsLibraryQmlBridge::isAssetDuplicate(const QString& assetId) const
{
    const Asset* a = assetManager()->getAsset(assetId);
    return a ? a->isDuplicate : false;
}

QString AssetsLibraryQmlBridge::getOriginalAssetId(const QString& duplicateId) const
{
    const Asset* a = assetManager()->getAsset(duplicateId);
    return a ? a->originalAssetId : QString();
}

// --- Batch import ---

void AssetsLibraryQmlBridge::importDirectoryAsync(const QString& directory)
{
    if (m_importing) {
        emit error("Import already in progress");
        return;
    }
    AssetManager::ImportOptions opts;
    opts.recursive = true;
    opts.autoConvert = true;
    opts.generateThumbnails = true;
    assetManager()->importDirectory(directory, opts);
}

void AssetsLibraryQmlBridge::importFilesAsync(const QStringList& filePaths)
{
    if (m_importing) {
        emit error("Import already in progress");
        return;
    }
    AssetManager::ImportOptions opts;
    opts.autoConvert = true;
    opts.generateThumbnails = true;
    assetManager()->importFiles(filePaths, opts);
}

void AssetsLibraryQmlBridge::importWithConversionAsync(const QString& sourcePath, const QString& targetFormat)
{
    if (m_importing) {
        emit error("Import already in progress");
        return;
    }
    AssetManager::ImportOptions opts;
    opts.autoConvert = true;
    opts.generateThumbnails = true;
    assetManager()->importWithConversion(sourcePath, targetFormat, opts);
}

QStringList AssetsLibraryQmlBridge::getSupportedConversions()
{
    return FormatConverter::supportedConversions();
}

} // namespace ks
