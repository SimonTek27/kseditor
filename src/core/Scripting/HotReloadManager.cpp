#include "ScriptDebugger.h"
#include <QFileSystemWatcher>
#include <QTimer>
#include <QCoreApplication>
#include <QFileInfo>

namespace ks {
namespace scripting {

HotReloadManager::HotReloadManager(QObject* parent) : QObject(parent)
{
    m_watcher = new QFileSystemWatcher(this);
    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(m_config.debounceMs);
    
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, &HotReloadManager::onFileChanged);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &HotReloadManager::onFileChanged);
    connect(m_debounceTimer, &QTimer::timeout, this, &HotReloadManager::debouncedReload);
}

HotReloadManager::~HotReloadManager() = default;

void HotReloadManager::addWatchPath(const QString& path)
{
    if (!m_config.watchPaths.contains(path)) {
        m_config.watchPaths.append(path);
        if (QFileInfo(path).isFile()) {
            m_watcher->addPath(path);
        } else if (QFileInfo(path).isDir()) {
            m_watcher->addPath(path);
        }
    }
}

void HotReloadManager::removeWatchPath(const QString& path)
{
    m_config.watchPaths.removeAll(path);
    m_watcher->removePath(path);
}

void HotReloadManager::setConfig(const ReloadConfig& config)
{
    m_config = config;
    m_debounceTimer->setInterval(m_config.debounceMs);
}

HotReloadManager::ReloadConfig HotReloadManager::config() const
{
    return m_config;
}

void HotReloadManager::triggerReload(const QString& filePath)
{
    if (m_config.reloadOnSave) {
        m_lastChangedFile = filePath;
        m_debounceTimer->start();
    }
}

void HotReloadManager::cancelPendingReload()
{
    m_debounceTimer->stop();
}

void HotReloadManager::registerModule(const QString& name, QObject* module)
{
    m_modules[name] = module;
}

void HotReloadManager::unregisterModule(const QString& name)
{
    m_modules.remove(name);
}

void HotReloadManager::reloadModule(const QString& name)
{
    if (m_modules.contains(name)) {
        QObject* module = m_modules[name];
        QVariantMap state;
        saveModuleState(name, state);
        m_moduleStates[name] = state;
        
        // Emit signal for module to handle reload
        emit moduleReloaded(name);
        
        QVariantMap restoredState = restoreModuleState(name);
        emit reloadFinished(true, "Module " + name + " reloaded");
    }
}

void HotReloadManager::saveModuleState(const QString& name, const QVariantMap& state)
{
    m_moduleStates[name] = state;
}

QVariantMap HotReloadManager::restoreModuleState(const QString& name)
{
    return m_moduleStates.value(name, QVariantMap());
}

void HotReloadManager::setupFileWatchers()
{
    for (const QString& path : m_config.watchPaths) {
        if (QFileInfo(path).exists()) {
            m_watcher->addPath(path);
        }
    }
}

void HotReloadManager::onFileChanged(const QString& filePath)
{
    if (!m_config.ignorePatterns.isEmpty()) {
        for (const QString& pattern : m_config.ignorePatterns) {
            if (QFileInfo(filePath).fileName().contains(pattern)) {
                return;
            }
        }
    }
    
    m_lastChangedFile = filePath;
    m_debounceTimer->start();
}

void HotReloadManager::debouncedReload()
{
    if (!m_lastChangedFile.isEmpty()) {
        emit reloadStarted(m_lastChangedFile);
        triggerReload(m_lastChangedFile);
    }
}

} // namespace scripting
} // namespace ks