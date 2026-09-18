#pragma once

#include <QObject>
#include <QString>
#include <QMap>

namespace ks::engine::sys {

// ============================================================================
// SystemDllInitializer - Initializes all system DLLs at engine startup
// ============================================================================

class SystemDllInitializer : public QObject {
    Q_OBJECT

public:
    struct InitResult {
        bool fmodCore = false;
        bool fmodStudio = false;
        bool videoModes = false;
        bool streamline = false;
        bool raceFlags = false;
        int totalLoaded = 0;
        int totalFailed = 0;
        QStringList errors;
    };

    static SystemDllInitializer* instance();

    explicit SystemDllInitializer(QObject* parent = nullptr);
    ~SystemDllInitializer();

    // --- Initialization ---
    InitResult initializeAll();
    void shutdownAll();

    // --- Individual component initialization ---
    bool initializeFmodCore();
    bool initializeFmodStudio();
    bool initializeVideoModes();
    bool initializeStreamline();
    bool initializeRaceFlags();

    // --- Status ---
    bool isInitialized() const { return m_initialized; }
    InitResult lastResult() const { return m_lastResult; }

    // --- Configuration ---
    void setFmodEnabled(bool enabled) { m_fmodEnabled = enabled; }
    void setFmodStudioEnabled(bool enabled) { m_fmodStudioEnabled = enabled; }
    void setVideoModesEnabled(bool enabled) { m_videoModesEnabled = enabled; }
    void setStreamlineEnabled(bool enabled) { m_streamlineEnabled = enabled; }
    void setRaceFlagsEnabled(bool enabled) { m_raceFlagsEnabled = enabled; }

    bool isFmodEnabled() const { return m_fmodEnabled; }
    bool isFmodStudioEnabled() const { return m_fmodStudioEnabled; }
    bool isVideoModesEnabled() const { return m_videoModesEnabled; }
    bool isStreamlineEnabled() const { return m_streamlineEnabled; }
    bool isRaceFlagsEnabled() const { return m_raceFlagsEnabled; }

signals:
    void initialized(const InitResult& result);
    void componentLoaded(const QString& component);
    void componentFailed(const QString& component, const QString& error);

private:
    bool m_initialized = false;
    bool m_fmodEnabled = true;
    bool m_fmodStudioEnabled = true;
    bool m_videoModesEnabled = true;
    bool m_streamlineEnabled = true;
    bool m_raceFlagsEnabled = true;

    InitResult m_lastResult;
};

} // namespace ks::engine::sys
