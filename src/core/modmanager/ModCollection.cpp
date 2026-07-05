#include "ModCollection.h"
#include "ModManager.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QUuid>

namespace ks {

CollectionManager::CollectionManager(QObject* parent)
    : QObject(parent)
{
    ensureDefaultCollection();
}

QString CollectionManager::generateId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString CollectionManager::collectionsFilePath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
        + "/mods/collections.json";
}

QStringList CollectionManager::listCollections() const
{
    return m_collections.keys();
}

bool CollectionManager::createCollection(const QString& name, const QString& description,
                                           const QString& color)
{
    if (name.trimmed().isEmpty()) return false;

    ModCollection col;
    col.id = generateId();
    col.name = name.trimmed();
    col.description = description;
    col.color = color;
    col.created = QDateTime::currentDateTime();
    col.modified = col.created;
    m_collections[col.id] = col;

    emit collectionCreated(col.id);
    emit collectionsChanged();
    return true;
}

bool CollectionManager::deleteCollection(const QString& id)
{
    auto it = m_collections.find(id);
    if (it == m_collections.end() || it->isDefault) return false;

    m_collections.erase(it);
    emit collectionDeleted(id);
    emit collectionsChanged();
    return true;
}

bool CollectionManager::renameCollection(const QString& id, const QString& newName)
{
    if (newName.trimmed().isEmpty()) return false;
    auto it = m_collections.find(id);
    if (it == m_collections.end()) return false;

    it->name = newName.trimmed();
    it->modified = QDateTime::currentDateTime();
    emit collectionRenamed(id, newName);
    emit collectionsChanged();
    return true;
}

bool CollectionManager::duplicateCollection(const QString& id, const QString& newName)
{
    auto it = m_collections.find(id);
    if (it == m_collections.end()) return false;

    QString name = newName.trimmed();
    if (name.isEmpty()) {
        name = it->name + " (Copy)";
        int counter = 2;
        while (std::any_of(m_collections.begin(), m_collections.end(),
               [&](const auto& p) { return p.name == name; })) {
            name = it->name + " (Copy " + QString::number(counter++) + ")";
        }
    }

    ModCollection copy = *it;
    copy.id = generateId();
    copy.name = name;
    copy.created = QDateTime::currentDateTime();
    copy.modified = copy.created;
    copy.isDefault = false;
    m_collections[copy.id] = copy;

    emit collectionCreated(copy.id);
    emit collectionsChanged();
    return true;
}

ModCollection CollectionManager::getCollection(const QString& id) const
{
    return m_collections.value(id);
}

bool CollectionManager::addModToCollection(const QString& collectionId, const QString& modName)
{
    auto it = m_collections.find(collectionId);
    if (it == m_collections.end()) return false;
    if (it->modNames.contains(modName)) return true;

    it->modNames.append(modName);
    it->modified = QDateTime::currentDateTime();
    emit collectionModified(collectionId);
    emit collectionsChanged();
    return true;
}

bool CollectionManager::removeModFromCollection(const QString& collectionId, const QString& modName)
{
    auto it = m_collections.find(collectionId);
    if (it == m_collections.end()) return false;

    int removed = it->modNames.removeAll(modName);
    if (removed > 0) {
        it->modified = QDateTime::currentDateTime();
        emit collectionModified(collectionId);
        emit collectionsChanged();
    }
    return removed > 0;
}

bool CollectionManager::setCollectionMods(const QString& collectionId, const QStringList& modNames)
{
    auto it = m_collections.find(collectionId);
    if (it == m_collections.end()) return false;

    it->modNames = modNames;
    it->modified = QDateTime::currentDateTime();
    emit collectionModified(collectionId);
    emit collectionsChanged();
    return true;
}

QStringList CollectionManager::getModsForCollection(const QString& collectionId) const
{
    auto it = m_collections.find(collectionId);
    return it != m_collections.end() ? it->modNames : QStringList();
}

QStringList CollectionManager::findCollectionsForMod(const QString& modName) const
{
    QStringList result;
    for (auto it = m_collections.begin(); it != m_collections.end(); ++it) {
        if (it->modNames.contains(modName)) {
            result.append(it->id);
        }
    }
    return result;
}

QVector<ModCollection> CollectionManager::collectionsForMods(const QStringList& modNames) const
{
    QVector<ModCollection> result;
    QSet<QString> modSet(modNames.begin(), modNames.end());
    for (auto it = m_collections.begin(); it != m_collections.end(); ++it) {
        for (const QString& modName : it->modNames) {
            if (modSet.contains(modName)) {
                result.append(it.value());
                break;
            }
        }
    }
    return result;
}

bool CollectionManager::saveCollections(const QString& filePath)
{
    QString path = filePath.isEmpty() ? collectionsFilePath() : filePath;
    QDir().mkpath(QFileInfo(path).absolutePath());

    QJsonArray arr;
    for (auto it = m_collections.begin(); it != m_collections.end(); ++it) {
        QJsonObject obj;
        obj["id"] = it->id;
        obj["name"] = it->name;
        obj["description"] = it->description;
        obj["color"] = it->color;
        obj["priority"] = it->priority;
        obj["isDefault"] = it->isDefault;
        obj["created"] = it->created.toString(Qt::ISODate);
        obj["modified"] = it->modified.toString(Qt::ISODate);

        QJsonArray modsArr;
        for (const QString& m : it->modNames) {
            modsArr.append(m);
        }
        obj["mods"] = modsArr;
        arr.append(obj);
    }

    QJsonObject root;
    root["collections"] = arr;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(QJsonDocument(root).toJson());
    file.close();
    return true;
}

bool CollectionManager::loadCollections(const QString& filePath)
{
    QString path = filePath.isEmpty() ? collectionsFilePath() : filePath;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        ensureDefaultCollection();
        return true;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        ensureDefaultCollection();
        return false;
    }

    m_collections.clear();
    QJsonArray arr = doc.object()["collections"].toArray();
    for (const auto& val : arr) {
        QJsonObject obj = val.toObject();
        ModCollection col;
        col.id = obj["id"].toString();
        col.name = obj["name"].toString();
        col.description = obj["description"].toString();
        col.color = obj["color"].toString("#3b82f6");
        col.priority = obj["priority"].toInt();
        col.isDefault = obj["isDefault"].toBool();
        col.created = QDateTime::fromString(obj["created"].toString(), Qt::ISODate);
        col.modified = QDateTime::fromString(obj["modified"].toString(), Qt::ISODate);

        QJsonArray modsArr = obj["mods"].toArray();
        for (const auto& m : modsArr) {
            col.modNames.append(m.toString());
        }

        m_collections[col.id] = col;
    }

    ensureDefaultCollection();
    return true;
}

void CollectionManager::ensureDefaultCollection()
{
    bool hasDefault = false;
    for (auto it = m_collections.begin(); it != m_collections.end(); ++it) {
        if (it->isDefault) { hasDefault = true; break; }
    }
    if (!hasDefault) {
        ModCollection col;
        col.id = "default";
        col.name = "All Mods";
        col.description = "Default collection containing all mods";
        col.color = "#6b7280";
        col.isDefault = true;
        col.created = QDateTime::currentDateTime();
        col.modified = col.created;
        m_collections["default"] = col;
    }
}

} // namespace ks
