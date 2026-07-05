#include "KsPlugin.h"
#include <QDir>
#include <QFile>
#include <QSettings>

namespace ks {
namespace plugins {
namespace kunos {

KsPlugin* KsPlugin::s_instance = nullptr;

KsPlugin::KsPlugin(QObject* parent)
    : QObject(parent) {
    s_instance = this;
}

KsPlugin::~KsPlugin() {
    shutdown();
    s_instance = nullptr;
}

KsPlugin* KsPlugin::instance() {
    if (!s_instance) {
        s_instance = new KsPlugin();
    }
    return s_instance;
}

bool KsPlugin::initialize() {
    if (m_initialized) return true;

    if (!detectInstallation()) {
        return false;
    }

    m_initialized = true;
    return true;
}

void KsPlugin::shutdown() {
    m_initialized = false;
}

bool KsPlugin::isAvailable() const {
    if (m_installPath.isEmpty()) return false;
    QDir dir(m_installPath);
    return dir.exists("ac.exe") || dir.exists("AssettoCorsa.exe");
}

void KsPlugin::setInstallPath(const QString& path) {
    m_installPath = path;
    emit installationChanged(path);
}

QStringList KsPlugin::supportedFileExtensions() const {
    return QStringList() << ".kn5" << ".ksanim" << ".dds" << ".ini" << ".json";
}

QStringList KsPlugin::supportedContentTypes() const {
    return QStringList() << "car" << "track" << "driver" << "skin" << "weather" << "shader";
}

QString KsPlugin::getContentDirectory() const {
    return m_installPath + "/content";
}

QString KsPlugin::getCarsDirectory() const {
    return m_installPath + "/content/cars";
}

QString KsPlugin::getTracksDirectory() const {
    return m_installPath + "/content/tracks";
}

QString KsPlugin::getDriversDirectory() const {
    return m_installPath + "/content/drivers";
}

QString KsPlugin::getSkinsDirectory() const {
    return m_installPath + "/content/skins";
}

QString KsPlugin::getShadersDirectory() const {
    return m_installPath + "/system/shaders";
}

QStringList KsPlugin::getCarList() const {
    QStringList cars;
    QDir dir(getCarsDirectory());
    if (dir.exists()) {
        cars = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    }
    return cars;
}

QStringList KsPlugin::getTrackList() const {
    QStringList tracks;
    QDir dir(getTracksDirectory());
    if (dir.exists()) {
        tracks = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    }
    return tracks;
}

QString KsPlugin::findCarPath(const QString& carId) const {
    return getCarsDirectory() + "/" + carId;
}

QString KsPlugin::findTrackPath(const QString& trackId) const {
    return getTracksDirectory() + "/" + trackId;
}

bool KsPlugin::detectInstallation() {
    if (!m_installPath.isEmpty() && isAvailable()) {
        return true;
    }

    for (const QString& path : getDefaultInstallationPaths()) {
        QDir dir(path);
        if (dir.exists() && (dir.exists("ac.exe") || dir.exists("AssettoCorsa.exe"))) {
            setInstallPath(path);
            emit installationDetected(path);
            return true;
        }
    }

    return false;
}

QStringList KsPlugin::getDefaultInstallationPaths() const {
    QStringList paths;

    QSettings registry("HKEY_LOCAL_MACHINE", QSettings::NativeFormat);
    QString registryPath = registry.value("SOFTWARE/WOW6432Node/Kunos Simulazioni/AssettoCorsa", "").toString();
    if (!registryPath.isEmpty()) {
        paths << registryPath;
    }

    paths << QDir::homePath() + "/AppData/Local/Steam/steamapps/core/assettocorsa";
    paths << "C:/Program Files (x86)/Steam/steamapps/core/assettocorsa";
    paths << "D:/Steam/steamapps/core/assettocorsa";
    paths << "F:/SteamLibrary/steamapps/core/assettocorsa";
    paths << "/Applications/Steam/steamapps/core/assettocorsa";
    paths << QDir::homePath() + "/.steam/steam/steamapps/core/assettocorsa";
    paths << QDir::homePath() + "/.local/share/Steam/steamapps/core/assettocorsa";

    return paths;
}

void KsPlugin::setDefaultInstallationPath(const QString& path) {
    QSettings settings("Kunos", "ksEditor");
    settings.setValue("KsInstallPath", path);
    setInstallPath(path);
}

}
}
}