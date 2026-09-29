#pragma once

#include <QObject>
#include <QString>
#include <QLibrary>

namespace ks {

/**
 * Runtime host for the Assetto Corsa plugin DLL (plugins/ksassettocorsa.dll).
 * The editor never hard-links the plugin: load() failures only log a warning.
 */
class KsAcPluginHost : public QObject {
    Q_OBJECT

public:
    static KsAcPluginHost& instance();

    bool load();
    bool isLoaded() const { return m_loaded; }

    bool initialize();
    void shutdown();

    QString pluginId() const;
    QString pluginName() const;
    QString pluginVersion() const;
    QString pluginDescription() const;
    bool isPluginAvailable() const;

    QString installPath() const;
    void setInstallPath(const QString& path);

    int carCount() const;
    int trackCount() const;
    QString carListJson() const;
    QString trackListJson() const;
    QString contentSummaryJson() const;

    bool launchGame(const QString& car, const QString& track);
    bool launchPractice(const QString& track);
    bool launchTimeTrial(const QString& track);
    bool stopGame();
    bool isGameRunning() const;

    bool telemetryAttach();
    void telemetryDetach();
    bool telemetryIsAttached() const;
    QString telemetryGetJson() const;

    bool kn5Validate(const QString& path) const;
    QString kn5GetInfoJson(const QString& path) const;

    QString workshopGetCategoriesJson() const;
    void workshopRefresh();
    QString workshopGetItemsJson() const;

private:
    explicit KsAcPluginHost(QObject* parent = nullptr);

    bool resolveExports();

    using Fn_cstr = const char* (*)();
    using Fn_bool = bool (*)();
    using Fn_void = void (*)();
    using Fn_int = int (*)();
    using Fn_setPath = void (*)(const char*);
    using Fn_launch = bool (*)(const char*, const char*);
    using Fn_launchTrack = bool (*)(const char*);
    using Fn_str_path = const char* (*)(const char*);
    using Fn_bool_path = bool (*)(const char*);

    QLibrary m_lib;
    bool m_loaded = false;

    Fn_cstr m_getPluginId = nullptr;
    Fn_cstr m_getPluginName = nullptr;
    Fn_cstr m_getPluginVersion = nullptr;
    Fn_cstr m_getPluginDescription = nullptr;
    Fn_bool m_initializePlugin = nullptr;
    Fn_void m_shutdownPlugin = nullptr;
    Fn_bool m_isPluginAvailable = nullptr;
    Fn_cstr m_getInstallPath = nullptr;
    Fn_setPath m_setInstallPath = nullptr;
    Fn_int m_getCarCount = nullptr;
    Fn_int m_getTrackCount = nullptr;
    Fn_cstr m_getCarListJson = nullptr;
    Fn_cstr m_getTrackListJson = nullptr;
    Fn_cstr m_getContentSummaryJson = nullptr;
    Fn_launch m_launchGame = nullptr;
    Fn_launchTrack m_launchPractice = nullptr;
    Fn_launchTrack m_launchTimeTrial = nullptr;
    Fn_bool m_stopGame = nullptr;
    Fn_bool m_isGameRunning = nullptr;
    Fn_bool m_telemetryAttach = nullptr;
    Fn_void m_telemetryDetach = nullptr;
    Fn_bool m_telemetryIsAttached = nullptr;
    Fn_cstr m_telemetryGetJson = nullptr;
    Fn_bool_path m_kn5Validate = nullptr;
    Fn_str_path m_kn5GetInfoJson = nullptr;
    Fn_cstr m_workshopGetCategoriesJson = nullptr;
    Fn_void m_workshopRefresh = nullptr;
    Fn_cstr m_workshopGetItemsJson = nullptr;
};

} // namespace ks
