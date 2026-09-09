#pragma once

#include <QObject>
#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVector>
#include <QVariant>
#include <QDateTime>

/**
 * @brief Database Manager - SQLite database for Assets Library
 * 
 * Manages the local asset database with support for:
 * - Asset metadata storage
 * - Tagging and categorization
 * - Search and filtering
 * - Import/export
 */
class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    static DatabaseManager& instance();

    // Database lifecycle
    bool initialize(const QString& databasePath = QString());
    void close();
    bool isOpen() const;

    // Transaction support
    bool transaction();
    bool commit();
    bool rollback();

    // Asset operations
    qint64 addAsset(const QString& name, const QString& type, const QString& path,
                    const QVariantMap& metadata = QVariantMap());
    bool updateAsset(qint64 assetId, const QVariantMap& metadata);
    bool deleteAsset(qint64 assetId);
    QSqlQuery getAsset(qint64 assetId) const;
    QSqlQuery getAllAssets() const;

    // Tag operations
    bool addTag(qint64 assetId, const QString& tag);
    bool removeTag(qint64 assetId, const QString& tag);
    QStringList getTags(qint64 assetId) const;
    QVector<qint64> getAssetsByTag(const QString& tag) const;
    QStringList getAllTags() const;

    // Category operations
    bool setCategory(qint64 assetId, const QString& category);
    QString getCategory(qint64 assetId) const;
    QStringList getAllCategories() const;

    // Search operations
    QSqlQuery searchAssets(const QString& query, const QString& category = QString(),
                          const QString& type = QString()) const;
    QSqlQuery getAssetsByType(const QString& type) const;
    QSqlQuery getRecentAssets(int limit = 10) const;
    QSqlQuery getFavorites() const;

    // Favorite operations
    bool setFavorite(qint64 assetId, bool favorite);
    bool isFavorite(qint64 assetId) const;

    // Statistics
    int assetCount() const;
    int assetCountByType(const QString& type) const;
    QMap<QString, int> typeDistribution() const;

    // Export/Import
    bool exportDatabase(const QString& exportPath) const;
    bool importDatabase(const QString& importPath);

signals:
    void assetAdded(qint64 assetId);
    void assetUpdated(qint64 assetId);
    void assetDeleted(qint64 assetId);
    void databaseOpened();
    void databaseClosed();
    void error(const QString& message);

private:
    explicit DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager();
    Q_DISABLE_COPY(DatabaseManager)

    bool createTables();
    bool runQuery(const QString& sql, bool dryRun = false);
    
    QSqlDatabase m_database;
    QString m_databasePath;
};

// Asset metadata keys
namespace AssetFields {
    const QString Name = "name";
    const QString Type = "type";
    const QString Path = "path";
    const QString Category = "category";
    const QString Description = "description";
    const QString Author = "author";
    const QString Version = "version";
    const QString Created = "created";
    const QString Modified = "modified";
    const QString Size = "size";
    const QString Hash = "hash";
    const QString Thumbnail = "thumbnail";
    const QString Favorite = "favorite";
    const QString Tags = "tags";
    const QString Metadata = "metadata";
}

// Asset types
namespace AssetTypes {
    const QString Car = "car";
    const QString Track = "track";
    const QString Skin = "skin";
    const QString Texture = "texture";
    const QString Sound = "sound";
    const QString Model = "model";
    const QString Config = "config";
    const QString Font = "font";
    const QString Other = "other";
    
    QStringList all();
}
