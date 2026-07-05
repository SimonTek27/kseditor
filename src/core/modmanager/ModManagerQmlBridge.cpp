#include "ModManagerQmlBridge.h"
#include "ModCollection.h"
#include <QApplication>

namespace ks {

ModManagerQmlBridge* ModManagerQmlBridge::s_instance = nullptr;
ModManagerModule* ModManagerQmlBridge::s_module = nullptr;

ModManagerQmlBridge* ModManagerQmlBridge::instance() {
    if (!s_instance) {
        s_instance = new ModManagerQmlBridge(qApp);
    }
    return s_instance;
}

ModManagerQmlBridge::ModManagerQmlBridge(QObject* parent)
    : QObject(parent)
{
    connectSignals();
}

void ModManagerQmlBridge::setUseHardLinks(bool use) {
    if (m_useHardLinks != use) {
        m_useHardLinks = use;
        if (module() && module()->installEngine()) {
            module()->installEngine()->setUseHardLinks(use);
        }
        emit useHardLinksChanged();
    }
}

void ModManagerQmlBridge::installMod(const QString& zipPath) {
    if (module()) {
        module()->installMod(zipPath);
    }
}

void ModManagerQmlBridge::uninstallMod(const QString& modName) {
    if (module()) {
        module()->uninstallMod(modName);
    }
}

void ModManagerQmlBridge::refreshMods() {
    if (module()) {
        module()->refreshMods();
        m_mods = module()->mods();
        emit modCountChanged();
        emit refreshFinished();
    }
}

static QString formatSize(qint64 bytes) {
    if (bytes < 1024) return QString::number(bytes) + " B";
    if (bytes < 1024 * 1024) return QString::number(bytes / 1024) + " KB";
    if (bytes < 1024LL * 1024 * 1024) return QString::number(bytes / (1024 * 1024)) + " MB";
    return QString::number(bytes / (1024LL * 1024 * 1024)) + " GB";
}

QVariantList ModManagerQmlBridge::getMods() {
    QVariantList result;
    for (int i = 0; i < m_mods.size(); ++i) {
        result.append(getMod(i));
    }
    return result;
}

QVariantMap ModManagerQmlBridge::getMod(int index) {
    if (index < 0 || index >= m_mods.size()) return {};

    const auto& mod = m_mods[index];
    QVariantMap m;
    m["enabled"] = mod.enabled;
    m["name"] = mod.name;
    m["version"] = mod.versionStr.isEmpty() ? "1.0" : mod.versionStr;
    m["author"] = mod.author.isEmpty() ? "Unknown" : mod.author;
    m["size"] = formatSize(mod.sizeBytes);
    m["date"] = mod.installDate.toString("yyyy-MM-dd");
    m["category"] = mod.category;
    m["description"] = mod.description;
    m["isBuiltIn"] = mod.isBuiltIn;
    m["path"] = mod.path;
    return m;
}

void ModManagerQmlBridge::enableMod(const QString& modName) {
    if (module()) module()->enableMod(modName);
    refreshMods();
}

void ModManagerQmlBridge::disableMod(const QString& modName) {
    if (module()) module()->disableMod(modName);
    refreshMods();
}

void ModManagerQmlBridge::setModPriority(const QString& modName, int priority) {
    if (module()) module()->setModPriority(modName, priority);
}

QStringList ModManagerQmlBridge::getCategories() const {
    return {"cars", "tracks", "skins", "weather", "config", "models", "audio", "fonts", "other"};
}

QVariantList ModManagerQmlBridge::getDependencyInfo(const QString& modName) {
    QVariantList result;

    auto it = std::find_if(m_mods.begin(), m_mods.end(),
        [&](const ModEntry& e) { return e.name == modName; });

    if (it == m_mods.end()) return result;

    for (const QString& dep : it->dependencies) {
        QVariantMap d;
        d["name"] = dep;
        d["installed"] = std::any_of(m_mods.begin(), m_mods.end(),
            [&](const ModEntry& e) { return e.name == dep && e.enabled; });
        result.append(d);
    }

    return result;
}

QStringList ModManagerQmlBridge::getConflicts(const QString& modName) {
    auto it = std::find_if(m_mods.begin(), m_mods.end(),
        [&](const ModEntry& e) { return e.name == modName; });
    return it != m_mods.end() ? it->conflicts : QStringList();
}

void ModManagerQmlBridge::backupMod(const QString& modName) {
    if (module() && module()->installEngine()) {
        module()->installEngine()->createBackup(modName);
    }
}

void ModManagerQmlBridge::restoreMod(const QString& modName) {
    if (module() && module()->installEngine()) {
        module()->installEngine()->restoreBackup(modName);
    }
}

void ModManagerQmlBridge::connectSignals() {
    if (module()) {
        connect(module(), &ModManagerModule::modsChanged, this, [this]() {
            m_mods = module()->mods();
            emit modCountChanged();
        });

        connect(module()->installEngine(), &ModInstallEngine::installProgress,
                this, [this](int percent, const QString& message) {
                    emit installProgress(message, percent);
                });

        connect(module()->installEngine(), &ModInstallEngine::installFinished,
                this, [this](const QString& modName, bool success) {
                    emit installFinished(modName, success);
                });

        connect(module()->updateChecker(), &ModUpdateChecker::checkFinished,
                this, [this](int updatesFound) {
                    m_updatesAvailable = updatesFound;
                    emit updatesChanged();
                });

        connect(module(), &ModManagerModule::updatesAvailable,
                this, [this](int count) {
                    m_updatesAvailable = count;
                    emit updatesChanged();
                });

        connect(module(), &ModManagerModule::profileChanged,
                this, [this](const QString& name) {
                    emit profileChanged();
                });

        if (module()->collectionManager()) {
            connect(module()->collectionManager(), &CollectionManager::collectionsChanged,
                    this, [this]() { emit collectionsChanged(); });
        }
    }
}

ModManagerModule* ModManagerQmlBridge::module() const {
    return s_module;
}

QString ModManagerQmlBridge::currentProfile() const {
    return module() && module()->profileManager()
        ? module()->profileManager()->currentProfile()
        : "Default";
}

QStringList ModManagerQmlBridge::profileNames() const {
    return module() && module()->profileManager()
        ? module()->profileManager()->listProfiles()
        : QStringList{"Default"};
}

QVariantList ModManagerQmlBridge::getDependencyTree(const QString& modName)
{
    QVariantList result;
    if (!module()) return result;

    auto tree = module()->dependencyResolver()->buildDependencyTree(m_mods, modName);
    for (const auto& level : tree) {
        QVariantMap entry;
        entry["mod"] = level.first;
        QVariantList deps;
        for (const auto& d : level.second) {
            bool installed = std::any_of(m_mods.begin(), m_mods.end(),
                [&](const ModEntry& e) { return e.name == d && e.enabled; });
            QVariantMap dep;
            dep["name"] = d;
            dep["installed"] = installed;
            deps.append(dep);
        }
        entry["dependencies"] = deps;
        result.append(entry);
    }
    return result;
}

void ModManagerQmlBridge::installModWithDependencies(const QString& zipPath)
{
    if (module()) {
        module()->installModWithDependencies(zipPath);
    }
}

QVariantMap ModManagerQmlBridge::getModDetails(int index)
{
    if (index < 0 || index >= m_mods.size()) return {};

    const auto& mod = m_mods[index];
    QVariantMap m;
    m["enabled"] = mod.enabled;
    m["name"] = mod.name;
    m["version"] = mod.versionStr.isEmpty() ? "1.0" : mod.versionStr;
    m["author"] = mod.author.isEmpty() ? "Unknown" : mod.author;
    m["size"] = formatSize(mod.sizeBytes);
    m["sizeBytes"] = mod.sizeBytes;
    m["date"] = mod.installDate.toString("yyyy-MM-dd");
    m["category"] = mod.category;
    m["description"] = mod.description;
    m["isBuiltIn"] = mod.isBuiltIn;
    m["path"] = mod.path;
    m["hasUpdate"] = mod.hasUpdate;
    m["newVersion"] = mod.newVersion;
    m["updateUrl"] = mod.updateUrl;
    m["dependencyCount"] = mod.dependencies.size();
    m["conflictCount"] = mod.conflicts.size();

    // Check dependency status
    int satisfiedCount = 0;
    for (const QString& dep : mod.dependencies) {
        bool found = std::any_of(m_mods.begin(), m_mods.end(),
            [&](const ModEntry& e) { return e.name == dep && e.enabled; });
        if (found || dep.isEmpty()) satisfiedCount++;
    }
    m["dependenciesSatisfied"] = (satisfiedCount == mod.dependencies.size());
    m["satisfiedDepCount"] = satisfiedCount;

    // Check conflicts
    m["hasActiveConflicts"] = false;
    for (const QString& conflict : mod.conflicts) {
        bool active = std::any_of(m_mods.begin(), m_mods.end(),
            [&](const ModEntry& e) { return e.name == conflict && e.enabled; });
        if (active) {
            m["hasActiveConflicts"] = true;
            break;
        }
    }

    // Find reverse dependencies (mods that depend on this)
    QStringList reverseDeps;
    for (const auto& other : m_mods) {
        if (other.name != mod.name && other.dependencies.contains(mod.name)) {
            reverseDeps.append(other.name);
        }
    }
    m["reverseDependencies"] = reverseDeps;

    return m;
}

void ModManagerQmlBridge::checkForUpdates()
{
    if (module()) {
        module()->checkForUpdates();
    }
}

void ModManagerQmlBridge::switchProfile(const QString& name)
{
    if (module()) {
        module()->switchProfile(name);
        emit profileChanged();
    }
}

void ModManagerQmlBridge::createProfile(const QString& name, const QString& description)
{
    if (module() && module()->profileManager()) {
        module()->profileManager()->createProfile(name, description);
        emit profilesChanged();
    }
}

void ModManagerQmlBridge::deleteProfile(const QString& name)
{
    if (module() && module()->profileManager()) {
        module()->profileManager()->deleteProfile(name);
        emit profilesChanged();
    }
}

void ModManagerQmlBridge::duplicateProfile(const QString& name, const QString& newName)
{
    if (module() && module()->profileManager()) {
        module()->profileManager()->duplicateProfile(name, newName);
        emit profilesChanged();
    }
}

QVariantList ModManagerQmlBridge::getFileConflicts()
{
    QVariantList result;
    if (!module() || !module()->conflictDetector()) return result;

    auto report = module()->conflictDetector()->detectConflicts(m_mods, module()->modDirectory());
    for (const auto& conflict : report.criticalConflicts) {
        QVariantMap c;
        c["file"] = conflict;
        c["type"] = "critical";
        result.append(c);
    }
    return result;
}

void ModManagerQmlBridge::resolveConflict(const QString& modA, const QString& modB, int keepModIndex)
{
    if (!module()) return;
    QString keepMod = (keepModIndex == 0) ? modA : modB;
    QString disableMod = (keepModIndex == 0) ? modB : modA;
    module()->enableMod(keepMod);
    module()->disableMod(disableMod);
    refreshMods();
}

// ============================================================================
// QML Bridge — Collection Management
// ============================================================================

QVariantList ModManagerQmlBridge::getCollections()
{
    QVariantList result;
    auto* cm = module() ? module()->collectionManager() : nullptr;
    if (!cm) return result;

    QStringList ids = cm->listCollections();
    for (const QString& id : ids) {
        ModCollection col = cm->getCollection(id);
        QVariantMap m;
        m["id"] = col.id;
        m["name"] = col.name;
        m["description"] = col.description;
        m["color"] = col.color;
        m["modCount"] = col.modNames.size();
        m["isDefault"] = col.isDefault;
        result.append(m);
    }
    return result;
}

bool ModManagerQmlBridge::createCollection(const QString& name, const QString& description)
{
    auto* cm = module() ? module()->collectionManager() : nullptr;
    if (!cm) return false;
    bool ok = cm->createCollection(name, description);
    if (ok) {
        cm->saveCollections();
        emit collectionsChanged();
    }
    return ok;
}

bool ModManagerQmlBridge::deleteCollection(const QString& id)
{
    auto* cm = module() ? module()->collectionManager() : nullptr;
    if (!cm) return false;
    bool ok = cm->deleteCollection(id);
    if (ok) {
        cm->saveCollections();
        emit collectionsChanged();
    }
    return ok;
}

bool ModManagerQmlBridge::addModToCollection(const QString& collectionId, const QString& modName)
{
    auto* cm = module() ? module()->collectionManager() : nullptr;
    if (!cm) return false;
    bool ok = cm->addModToCollection(collectionId, modName);
    if (ok) {
        cm->saveCollections();
        emit collectionsChanged();
    }
    return ok;
}

bool ModManagerQmlBridge::removeModFromCollection(const QString& collectionId, const QString& modName)
{
    auto* cm = module() ? module()->collectionManager() : nullptr;
    if (!cm) return false;
    bool ok = cm->removeModFromCollection(collectionId, modName);
    if (ok) {
        cm->saveCollections();
        emit collectionsChanged();
    }
    return ok;
}

QStringList ModManagerQmlBridge::collectionMods(const QString& collectionId) const
{
    auto* cm = module() ? module()->collectionManager() : nullptr;
    return cm ? cm->getModsForCollection(collectionId) : QStringList();
}

QStringList ModManagerQmlBridge::modCollections(const QString& modName) const
{
    auto* cm = module() ? module()->collectionManager() : nullptr;
    return cm ? cm->findCollectionsForMod(modName) : QStringList();
}

// ============================================================================
// QML Bridge — Statistics
// ============================================================================

QVariantMap ModManagerQmlBridge::getStats()
{
    return module() ? module()->getStats() : QVariantMap();
}

}
