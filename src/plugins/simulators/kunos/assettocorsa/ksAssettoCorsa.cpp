#include "ksAssettoCorsa_export.h"
#include "../KsPlugin.h"
#include "WorkshopModule.h"
#include "ContentBrowser.h"
#include "QmlBridges.h"
#include "SetupComparison.h"
#include "KsAssettoCorsaRunner.h"
#include "KsAssettoCorsaContentPath.h"
#include <QDebug>

namespace ks {
namespace plugins {
namespace kunos {

extern "C" {

KS_ASSETTOCORSA_API const char* getPluginId() {
    return "ksAssettoCorsa";
}

KS_ASSETTOCORSA_API const char* getPluginName() {
    return "Assetto Corsa Plugin";
}

KS_ASSETTOCORSA_API const char* getPluginVersion() {
    return "0.9.0";
}

KS_ASSETTOCORSA_API const char* getPluginDescription() {
    return "Assetto Corsa content management and editing plugin for ksEditor";
}

KS_ASSETTOCORSA_API bool initializePlugin() {
    qDebug() << "[ksAssettoCorsa] Plugin initializing...";
    
    KsPlugin* ksPlugin = KsPlugin::instance();
    if (!ksPlugin) {
        qWarning() << "[ksAssettoCorsa] Failed to get KsPlugin instance";
        return false;
    }

    if (!ksPlugin->initialize()) {
        qWarning() << "[ksAssettoCorsa] Failed to initialize KsPlugin";
        return false;
    }

    qDebug() << "[ksAssettoCorsa] Plugin initialized successfully";
    return true;
}

KS_ASSETTOCORSA_API void shutdownPlugin() {
    qDebug() << "[ksAssettoCorsa] Plugin shutting down...";
    
    KsPlugin* ksPlugin = KsPlugin::instance();
    if (ksPlugin) {
        ksPlugin->shutdown();
    }
    
    qDebug() << "[ksAssettoCorsa] Plugin shut down";
}

KS_ASSETTOCORSA_API bool isPluginAvailable() {
    KsPlugin* ksPlugin = KsPlugin::instance();
    return ksPlugin && ksPlugin->isAvailable();
}

KS_ASSETTOCORSA_API const char* getInstallPath() {
    static std::string path;
    KsPlugin* ksPlugin = KsPlugin::instance();
    if (ksPlugin) {
        path = ksPlugin->installPath().toStdString();
    }
    return path.c_str();
}

KS_ASSETTOCORSA_API void setInstallPath(const char* path) {
    KsPlugin* ksPlugin = KsPlugin::instance();
    if (ksPlugin && path) {
        ksPlugin->setInstallPath(QString::fromUtf8(path));
    }
}

KS_ASSETTOCORSA_API int getCarCount() {
    KsPlugin* ksPlugin = KsPlugin::instance();
    if (ksPlugin) {
        return ksPlugin->getCarList().size();
    }
    return 0;
}

KS_ASSETTOCORSA_API int getTrackCount() {
    KsPlugin* ksPlugin = KsPlugin::instance();
    if (ksPlugin) {
        return ksPlugin->getTrackList().size();
    }
    return 0;
}

} // extern "C"

} // namespace kunos
} // namespace plugins
} // namespace ks
