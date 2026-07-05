#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>

namespace ks {

struct ModEntry;

struct ModCollection {
    QString id;
    QString name;
    QString description;
    QDateTime created;
    QDateTime modified;
    QStringList modNames;   // mod names in this collection
    QString color;          // display color hex
    int priority = 0;
    bool isDefault = false;
};

class CollectionManager : public QObject {
    Q_OBJECT
public:
    explicit CollectionManager(QObject* parent = nullptr);

    // CRUD
    QStringList listCollections() const;
    bool createCollection(const QString& name, const QString& description = QString(),
                          const QString& color = "#3b82f6");
    bool deleteCollection(const QString& id);
    bool renameCollection(const QString& id, const QString& newName);
    bool duplicateCollection(const QString& id, const QString& newName);
    ModCollection getCollection(const QString& id) const;

    // Mod membership
    bool addModToCollection(const QString& collectionId, const QString& modName);
    bool removeModFromCollection(const QString& collectionId, const QString& modName);
    bool setCollectionMods(const QString& collectionId, const QStringList& modNames);
    QStringList getModsForCollection(const QString& collectionId) const;

    // Query
    QStringList findCollectionsForMod(const QString& modName) const;
    QVector<ModCollection> collectionsForMods(const QStringList& modNames) const;

    // Persistence
    bool saveCollections(const QString& filePath = QString());
    bool loadCollections(const QString& filePath = QString());

    // Default
    void ensureDefaultCollection();

signals:
    void collectionCreated(const QString& id);
    void collectionDeleted(const QString& id);
    void collectionRenamed(const QString& id, const QString& newName);
    void collectionModified(const QString& id);
    void collectionsChanged();

private:
    QString generateId() const;
    QString collectionsFilePath() const;

    QMap<QString, ModCollection> m_collections;
};

} // namespace ks
