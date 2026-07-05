#include "DatabaseManager.h"
#include "LogManager.h"

#include <QSqlError>
#include <QSqlField>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::DatabaseManager(QObject* parent)
    : QObject(parent)
{
}

DatabaseManager::~DatabaseManager()
{
    close();
}

bool DatabaseManager::initialize(const QString& databasePath)
{
    // Generate default path if not provided
    if (databasePath.isEmpty()) {
        QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(appDataPath);
        m_databasePath = appDataPath + "/assets.db";
    } else {
        m_databasePath = databasePath;
    }

    // Create database connection
    m_database = QSqlDatabase::addDatabase("QSQLITE", "kseditor_assets");
    m_database.setDatabaseName(m_databasePath);

    if (!m_database.open()) {
        QString error = m_database.lastError().text();
        LOG_ERROR("DatabaseManager", "Failed to open database: " + error);
        emit this->error(error);
        return false;
    }

    // Create tables
    if (!createTables()) {
        return false;
    }

    LOG_INFO("DatabaseManager", "Database initialized: " + m_databasePath);
    emit databaseOpened();
    return true;
}

void DatabaseManager::close()
{
    if (m_database.isOpen()) {
        m_database.close();
        LOG_INFO("DatabaseManager", "Database closed");
        emit databaseClosed();
    }
}

bool DatabaseManager::isOpen() const
{
    return m_database.isOpen();
}

bool DatabaseManager::transaction()
{
    return m_database.transaction();
}

bool DatabaseManager::commit()
{
    return m_database.commit();
}

bool DatabaseManager::rollback()
{
    return m_database.rollback();
}

bool DatabaseManager::createTables()
{
    // Assets table
    QStringList createStatements = {
        R"(
        CREATE TABLE IF NOT EXISTS assets (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            type TEXT NOT NULL,
            path TEXT NOT NULL UNIQUE,
            category TEXT,
            description TEXT,
            author TEXT,
            version TEXT,
            created TEXT NOT NULL,
            modified TEXT NOT NULL,
            size INTEGER,
            hash TEXT,
            thumbnail BLOB,
            favorite INTEGER DEFAULT 0,
            metadata TEXT
        )
        )",
        
        R"(
        CREATE TABLE IF NOT EXISTS tags (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            asset_id INTEGER NOT NULL,
            tag TEXT NOT NULL,
            FOREIGN KEY (asset_id) REFERENCES assets(id) ON DELETE CASCADE,
            UNIQUE(asset_id, tag)
        )
        )",
        
        "CREATE INDEX IF NOT EXISTS idx_assets_type ON assets(type)",
        "CREATE INDEX IF NOT EXISTS idx_assets_category ON assets(category)",
        "CREATE INDEX IF NOT EXISTS idx_assets_name ON assets(name)",
        "CREATE INDEX IF NOT EXISTS idx_tags_asset ON tags(asset_id)",
        "CREATE INDEX IF NOT EXISTS idx_tags_tag ON tags(tag)"
    };

    for (const QString& sql : createStatements) {
        QSqlQuery query(m_database);
        if (!query.exec(sql)) {
            QString error = query.lastError().text();
            LOG_ERROR("DatabaseManager", "Failed to create table: " + error);
            emit this->error(error);
            return false;
        }
    }

    return true;
}

qint64 DatabaseManager::addAsset(const QString& name, const QString& type, 
                                  const QString& path, const QVariantMap& metadata)
{
    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT INTO assets (name, type, path, created, modified, metadata)
        VALUES (:name, :type, :path, :created, :modified, :metadata)
    )");
    
    query.bindValue(":name", name);
    query.bindValue(":type", type);
    query.bindValue(":path", path);
    query.bindValue(":created", QDateTime::currentDateTime().toString(Qt::ISODate));
    query.bindValue(":modified", QDateTime::currentDateTime().toString(Qt::ISODate));
    query.bindValue(":metadata", QJsonDocument::fromVariant(metadata).toJson());
    
    if (!query.exec()) {
        QString error = query.lastError().text();
        LOG_ERROR("DatabaseManager", "Failed to add asset: " + error);
        emit this->error(error);
        return -1;
    }
    
    qint64 id = query.lastInsertId().toLongLong();
    LOG_INFO("DatabaseManager", QString("Added asset: %1 (ID: %2)").arg(name).arg(id));
    emit assetAdded(id);
    return id;
}

bool DatabaseManager::updateAsset(qint64 assetId, const QVariantMap& metadata)
{
    QSqlQuery query(m_database);
    query.prepare(R"(
        UPDATE assets 
        SET modified = :modified, metadata = :metadata
        WHERE id = :id
    )");
    
    query.bindValue(":id", assetId);
    query.bindValue(":modified", QDateTime::currentDateTime().toString(Qt::ISODate));
    query.bindValue(":metadata", QJsonDocument::fromVariant(metadata).toJson());
    
    if (!query.exec()) {
        QString error = query.lastError().text();
        LOG_ERROR("DatabaseManager", "Failed to update asset: " + error);
        emit this->error(error);
        return false;
    }
    
    emit assetUpdated(assetId);
    return true;
}

bool DatabaseManager::deleteAsset(qint64 assetId)
{
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM assets WHERE id = :id");
    query.bindValue(":id", assetId);
    
    if (!query.exec()) {
        QString error = query.lastError().text();
        LOG_ERROR("DatabaseManager", "Failed to delete asset: " + error);
        emit this->error(error);
        return false;
    }
    
    LOG_INFO("DatabaseManager", QString("Deleted asset ID: %1").arg(assetId));
    emit assetDeleted(assetId);
    return true;
}

QSqlQuery DatabaseManager::getAsset(qint64 assetId) const
{
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM assets WHERE id = :id");
    query.bindValue(":id", assetId);
    query.exec();
    return query;
}

QSqlQuery DatabaseManager::getAllAssets() const
{
    QSqlQuery query(m_database);
    query.exec("SELECT * FROM assets ORDER BY modified DESC");
    return query;
}

bool DatabaseManager::addTag(qint64 assetId, const QString& tag)
{
    QSqlQuery query(m_database);
    query.prepare("INSERT OR IGNORE INTO tags (asset_id, tag) VALUES (:asset_id, :tag)");
    query.bindValue(":asset_id", assetId);
    query.bindValue(":tag", tag.toLower());
    return query.exec();
}

bool DatabaseManager::removeTag(qint64 assetId, const QString& tag)
{
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM tags WHERE asset_id = :asset_id AND tag = :tag");
    query.bindValue(":asset_id", assetId);
    query.bindValue(":tag", tag.toLower());
    return query.exec();
}

QStringList DatabaseManager::getTags(qint64 assetId) const
{
    QStringList tags;
    QSqlQuery query(m_database);
    query.prepare("SELECT tag FROM tags WHERE asset_id = :asset_id");
    query.bindValue(":asset_id", assetId);
    
    if (query.exec()) {
        while (query.next()) {
            tags.append(query.value(0).toString());
        }
    }
    return tags;
}

QVector<qint64> DatabaseManager::getAssetsByTag(const QString& tag) const
{
    QVector<qint64> ids;
    QSqlQuery query(m_database);
    query.prepare("SELECT asset_id FROM tags WHERE tag = :tag");
    query.bindValue(":tag", tag.toLower());
    
    if (query.exec()) {
        while (query.next()) {
            ids.append(query.value(0).toLongLong());
        }
    }
    return ids;
}

QStringList DatabaseManager::getAllTags() const
{
    QStringList tags;
    QSqlQuery query(m_database);
    query.exec("SELECT DISTINCT tag FROM tags ORDER BY tag");
    
    while (query.next()) {
        tags.append(query.value(0).toString());
    }
    return tags;
}

bool DatabaseManager::setCategory(qint64 assetId, const QString& category)
{
    QSqlQuery query(m_database);
    query.prepare("UPDATE assets SET category = :category WHERE id = :id");
    query.bindValue(":id", assetId);
    query.bindValue(":category", category);
    return query.exec();
}

QString DatabaseManager::getCategory(qint64 assetId) const
{
    QSqlQuery query(m_database);
    query.prepare("SELECT category FROM assets WHERE id = :id");
    query.bindValue(":id", assetId);
    
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return QString();
}

QStringList DatabaseManager::getAllCategories() const
{
    QStringList categories;
    QSqlQuery query(m_database);
    query.exec("SELECT DISTINCT category FROM assets WHERE category IS NOT NULL ORDER BY category");
    
    while (query.next()) {
        categories.append(query.value(0).toString());
    }
    return categories;
}

QSqlQuery DatabaseManager::searchAssets(const QString& queryStr, 
                                        const QString& category, 
                                        const QString& type) const
{
    QSqlQuery query(m_database);
    QString sql = "SELECT * FROM assets WHERE 1=1";
    
    if (!queryStr.isEmpty()) {
        sql += " AND (name LIKE :query OR description LIKE :query OR author LIKE :query)";
    }
    if (!category.isEmpty()) {
        sql += " AND category = :category";
    }
    if (!type.isEmpty()) {
        sql += " AND type = :type";
    }
    
    sql += " ORDER BY modified DESC";
    
    query.prepare(sql);
    
    if (!queryStr.isEmpty()) {
        query.bindValue(":query", "%" + queryStr + "%");
    }
    if (!category.isEmpty()) {
        query.bindValue(":category", category);
    }
    if (!type.isEmpty()) {
        query.bindValue(":type", type);
    }
    
    query.exec();
    return query;
}

QSqlQuery DatabaseManager::getAssetsByType(const QString& type) const
{
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM assets WHERE type = :type ORDER BY name");
    query.bindValue(":type", type);
    query.exec();
    return query;
}

QSqlQuery DatabaseManager::getRecentAssets(int limit) const
{
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM assets ORDER BY modified DESC LIMIT :limit");
    query.bindValue(":limit", limit);
    query.exec();
    return query;
}

QSqlQuery DatabaseManager::getFavorites() const
{
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM assets WHERE favorite = 1 ORDER BY name");
    query.exec();
    return query;
}

bool DatabaseManager::setFavorite(qint64 assetId, bool favorite)
{
    QSqlQuery query(m_database);
    query.prepare("UPDATE assets SET favorite = :favorite WHERE id = :id");
    query.bindValue(":id", assetId);
    query.bindValue(":favorite", favorite ? 1 : 0);
    return query.exec();
}

bool DatabaseManager::isFavorite(qint64 assetId) const
{
    QSqlQuery query(m_database);
    query.prepare("SELECT favorite FROM assets WHERE id = :id");
    query.bindValue(":id", assetId);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt() == 1;
    }
    return false;
}

int DatabaseManager::assetCount() const
{
    QSqlQuery query(m_database);
    query.exec("SELECT COUNT(*) FROM assets");
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int DatabaseManager::assetCountByType(const QString& type) const
{
    QSqlQuery query(m_database);
    query.prepare("SELECT COUNT(*) FROM assets WHERE type = :type");
    query.bindValue(":type", type);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

QMap<QString, int> DatabaseManager::typeDistribution() const
{
    QMap<QString, int> distribution;
    QSqlQuery query(m_database);
    query.exec("SELECT type, COUNT(*) FROM assets GROUP BY type");
    
    while (query.next()) {
        distribution[query.value(0).toString()] = query.value(1).toInt();
    }
    return distribution;
}

bool DatabaseManager::exportDatabase(const QString& exportPath) const
{
    QFile file(exportPath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    // Simple export as JSON
    QJsonArray assets;
    
    QSqlQuery query(m_database);
    query.exec("SELECT * FROM assets");
    
    while (query.next()) {
        QJsonObject obj;
        QSqlRecord record = query.record();
        for (int i = 0; i < record.count(); ++i) {
            obj[record.fieldName(i)] = QJsonValue::fromVariant(record.value(i));
        }
        assets.append(obj);
    }
    
    QJsonDocument doc(assets);
    file.write(doc.toJson());
    file.close();
    
    return true;
}

bool DatabaseManager::importDatabase(const QString& importPath)
{
    QFile file(importPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        return false;
    }
    
    QJsonArray assets = doc.array();
    
    transaction();
    
    for (const QJsonValue& val : assets) {
        QJsonObject obj = val.toObject();
        addAsset(
            obj["name"].toString(),
            obj["type"].toString(),
            obj["path"].toString()
        );
    }
    
    commit();
    
    return true;
}

bool DatabaseManager::runQuery(const QString& sql, bool dryRun)
{
    if (dryRun) {
        LOG_INFO("DatabaseManager", "Would execute: " + sql);
        return true;
    }
    
    QSqlQuery query(m_database);
    if (!query.exec(sql)) {
        QString error = query.lastError().text();
        LOG_ERROR("DatabaseManager", "Query failed: " + error);
        emit this->error(error);
        return false;
    }
    return true;
}

QStringList AssetTypes::all()
{
    return {
        Car, Track, Skin, Texture, Sound, Model, Config, Font, Other
    };
}
