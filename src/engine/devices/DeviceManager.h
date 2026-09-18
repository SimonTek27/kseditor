#pragma once

#include <QObject>
#include <QTimer>
#include <QSettings>
#include "simracing/SimRacingDevices.h"
#include "vr/XrManager.h"

namespace ks::device {

// ============================================================================
// DeviceManager
// ============================================================================
// Central hub for all input/display devices in the racing simulator.
// Coordinates triple monitor rendering, VR headset, steering wheels,
// pedals, shifters, and force feedback.
//
// Usage:
//   auto* dm = DeviceManager::instance();
//   dm->initialize();
//   dm->tripleMonitor()->setConfig(config);
//   dm->racingInput()->selectDevice(wheelIndex);
// ============================================================================

class DeviceManager : public QObject {
    Q_OBJECT
public:
    static DeviceManager* instance();

    explicit DeviceManager(QObject* parent = nullptr);
    ~DeviceManager() override;

    // Lifecycle
    bool initialize();
    void shutdown();
    bool isInitialized() const { return m_initialized; }

    // Sub-managers
    TripleMonitorManager* tripleMonitor() const { return m_tripleMonitor.get(); }
    RacingInputManager* racingInput() const { return m_racingInput.get(); }
    XrManager* vr() const { return m_vr.get(); }

    // Rendering mode
    enum class RenderMode : uint8_t {
        SingleMonitor = 0,
        TripleMonitor,
        VR
    };

    void setRenderMode(RenderMode mode);
    RenderMode renderMode() const { return m_renderMode; }
    bool isVRActive() const;
    bool isTripleActive() const;

    // Unified device status
    struct DeviceStatus {
        bool tripleMonitorConnected = false;
        int monitorCount = 0;
        bool vrConnected = false;
        bool vrSessionRunning = false;
        bool steeringWheelConnected = false;
        QString activeDeviceName;
    };

    DeviceStatus status() const;

    // Profiles
    void loadProfiles(const QString& basePath);
    void saveProfiles(const QString& basePath) const;

signals:
    void renderModeChanged(RenderMode mode);
    void deviceStatusChanged(const DeviceStatus& status);
    void error(const QString& message);

private:
    void onMonitorChanged();
    void onVRSessionChanged(bool running);
    void onInputDeviceChanged(int index);

    std::unique_ptr<TripleMonitorManager> m_tripleMonitor;
    std::unique_ptr<RacingInputManager> m_racingInput;
    std::unique_ptr<XrManager> m_vr;

    RenderMode m_renderMode = RenderMode::SingleMonitor;
    bool m_initialized = false;
    QTimer m_statusTimer;

    static DeviceManager* s_instance;
};

} // namespace ks::device