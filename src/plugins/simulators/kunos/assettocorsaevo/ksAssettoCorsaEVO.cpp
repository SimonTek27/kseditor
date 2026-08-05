#include "ksAssettoCorsaEVO_export.h"
#include "ACEPaths.h"
#include "ACEPackageParser.h"
#include "ACERunner.h"
#include "ACEQmlBridge.h"
#include <QDebug>
#include <string>

namespace ks {
namespace plugins {
namespace kunos {

namespace {
    std::string s_installPath;
    ACEContentQmlBridge* s_contentBridge = nullptr;
    ACEPackageQmlBridge* s_packageBridge = nullptr;
    ACEProtobufQmlBridge* s_protobufBridge = nullptr;
}

extern "C" {

KS_ASSETTOCORSAEVO_API const char* getPluginId() {
    return "ksAssettoCorsaEVO";
}

KS_ASSETTOCORSAEVO_API const char* getPluginName() {
    return "Assetto Corsa EVO Plugin";
}

KS_ASSETTOCORSAEVO_API const char* getPluginVersion() {
    return "0.2.0";
}

KS_ASSETTOCORSAEVO_API const char* getPluginDescription() {
    return "Assetto Corsa EVO content management, kspkg parsing, and protobuf inspection plugin for ksEditor";
}

KS_ASSETTOCORSAEVO_API bool initializePlugin() {
    qDebug() << "[ksAssettoCorsaEVO] Plugin initializing...";

    QString bestPath = ACEPaths::findBestInstallation();
    if (!bestPath.isEmpty()) {
        s_installPath = bestPath.toStdString();
        qDebug() << "[ksAssettoCorsaEVO] Detected ACE installation:" << bestPath;
    } else {
        qDebug() << "[ksAssettoCorsaEVO] No ACE installation detected automatically";
    }

    s_contentBridge = new ACEContentQmlBridge();
    s_packageBridge = new ACEPackageQmlBridge();
    s_protobufBridge = new ACEProtobufQmlBridge();

    qDebug() << "[ksAssettoCorsaEVO] Plugin initialized successfully";
    return true;
}

KS_ASSETTOCORSAEVO_API void shutdownPlugin() {
    qDebug() << "[ksAssettoCorsaEVO] Plugin shutting down...";

    delete s_contentBridge;
    s_contentBridge = nullptr;
    delete s_packageBridge;
    s_packageBridge = nullptr;
    delete s_protobufBridge;
    s_protobufBridge = nullptr;

    ACERunner* runner = ACERunner::instance();
    if (runner->isRunning()) {
        runner->stopAce();
    }

    qDebug() << "[ksAssettoCorsaEVO] Plugin shut down";
}

KS_ASSETTOCORSAEVO_API bool isPluginAvailable() {
    return !ACEPaths::findAllInstallations().isEmpty();
}

KS_ASSETTOCORSAEVO_API const char* getInstallPath() {
    return s_installPath.c_str();
}

KS_ASSETTOCORSAEVO_API void setInstallPath(const char* path) {
    if (path) {
        s_installPath = std::string(path);
        qDebug() << "[ksAssettoCorsaEVO] Install path set to:" << path;
    }
}

KS_ASSETTOCORSAEVO_API int getCarCount() {
    if (s_installPath.empty()) return 0;
    return ACEPaths::getCarNames(QString::fromStdString(s_installPath)).size();
}

KS_ASSETTOCORSAEVO_API int getTrackCount() {
    if (s_installPath.empty()) return 0;
    return ACEPaths::getTrackNames(QString::fromStdString(s_installPath)).size();
}

KS_ASSETTOCORSAEVO_API int getModCount() {
    QString modsDir = ACEPaths::findModDirectory();
    if (modsDir.isEmpty()) return 0;
    QDir dir(modsDir);
    if (!dir.exists()) return 0;
    return dir.entryList(QStringList() << "*.kspkg", QDir::Files).size();
}

KS_ASSETTOCORSAEVO_API bool launchGame(const char* track, const char* car) {
    ACERunner* runner = ACERunner::instance();
    if (!s_installPath.empty()) {
        runner->setAcePath(QString::fromStdString(s_installPath));
    }
    QString t = track ? QString::fromUtf8(track) : QString();
    QString c = car ? QString::fromUtf8(car) : QString();
    return runner->launchAce(t, c);
}

KS_ASSETTOCORSAEVO_API bool stopGame() {
    return ACERunner::instance()->stopAce();
}

KS_ASSETTOCORSAEVO_API bool isGameRunning() {
    return ACERunner::instance()->isRunning();
}

KS_ASSETTOCORSAEVO_API bool hasContentKspkg() {
    if (s_installPath.empty()) return false;
    return ACEPaths::hasContentKspkg(QString::fromStdString(s_installPath));
}

KS_ASSETTOCORSAEVO_API bool hasUnpackedContent() {
    if (s_installPath.empty()) return false;
    return ACEPaths::hasUnpackedContent(QString::fromStdString(s_installPath));
}

KS_ASSETTOCORSAEVO_API void* getContentBridge() {
    return s_contentBridge;
}

KS_ASSETTOCORSAEVO_API void* getPackageBridge() {
    return s_packageBridge;
}

KS_ASSETTOCORSAEVO_API void* getProtobufBridge() {
    return s_protobufBridge;
}

KS_ASSETTOCORSAEVO_API int64_t extractXorKey(const char* packagePath) {
    if (!packagePath) return 0;
    return static_cast<int64_t>(ACEPackageParser::extractXorKey(QString::fromUtf8(packagePath)));
}

} // extern "C"

} // namespace kunos
} // namespace plugins
} // namespace ks
