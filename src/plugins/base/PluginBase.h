#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QObject>
#include <QVariant>

namespace ks {

class SimulatorPlugin;

class PluginBase {
public:
    virtual ~PluginBase() = default;

    virtual QString id() const = 0;
    virtual QString name() const = 0;
    virtual QString version() const = 0;
    virtual QString description() const = 0;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;

    virtual bool isInitialized() const = 0;
    virtual bool isAvailable() const = 0;

    virtual QString installPath() const = 0;
    virtual void setInstallPath(const QString& path) = 0;

    virtual QStringList supportedFileExtensions() const = 0;
    virtual QStringList supportedContentTypes() const = 0;

protected:
    bool m_initialized = false;
    QString m_installPath;
};

class PluginManagerBase {
public:
    static PluginManagerBase* instance();

    virtual ~PluginManagerBase() = default;

    virtual void registerPlugin(PluginBase* plugin) = 0;
    virtual void unregisterPlugin(const QString& pluginId) = 0;
    virtual PluginBase* getPlugin(const QString& pluginId) = 0;
    virtual QList<PluginBase*> allPlugins() const = 0;
    virtual QStringList availablePlugins() const = 0;

    virtual bool loadPlugin(const QString& pluginId) = 0;
    virtual bool unloadPlugin(const QString& pluginId) = 0;

    virtual QString getContentDirectory(const QString& pluginId) const = 0;
    virtual QString getCarsDirectory(const QString& pluginId) const = 0;
    virtual QString getTracksDirectory(const QString& pluginId) const = 0;
    virtual QStringList getCarList(const QString& pluginId) const = 0;
    virtual QStringList getTrackList(const QString& pluginId) const = 0;

protected:
    static PluginManagerBase* s_instance;
};

}