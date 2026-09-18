#pragma once

#include <QObject>
#include <QScopedPointer>
#include <QString>

// Forward declarations for Windows API types
struct HWND__;
typedef struct HWND__* HWND;
typedef void* HDEVCTX;
typedef void* HDIJOYCONFIG;
typedef void* HJOYINFO;

// Windows DirectInput includes would go here, but we'll use a simplified interface
// for now to avoid requiring the full DirectX SDK

namespace ks {

// HIL Interface for Hardware-in-the-Loop force feedback
class HilFfbInterface : public QObject {
    Q_OBJECT
public:
    explicit HilFfbInterface(QObject* parent = nullptr);
    ~HilFfbInterface();

    // Initialize HIL subsystem
    bool initialize();

    // Shutdown HIL subsystem
    void shutdown();

    // Connect a Direct Drive wheel
    bool connectWheel(const QString& deviceName);

    // Disconnect wheel
    void disconnectWheel();

    // Update force feedback output based on tire model
    void updateFfbForce(float lateralForce, float longitudinalForce,
                         float aligningMoment, float slipRatio);

    // Get connected device info
    QString connectedDevice() const { return m_connectedDevice; }

signals:
    void wheelConnected(const QString& deviceName);
    void wheelDisconnected();
    void ffbForceUpdated(float lateralForce, float longitudinalForce,
                         float aligningMoment, float slipRatio);
    void error(const QString& message);

private:
    // Platform-specific wheel handle
    void* m_wheelHandle = nullptr;
    QString m_connectedDevice;
    bool m_isInitialized = false;

    // FFB parameters derived from tire model
    float m_currentLateralForce = 0.0f;
    float m_currentLongitudinalForce = 0.0f;
    float m_currentAligningMoment = 0.0f;
    float m_currentSlipRatio = 0.0f;

    // Windows HIL helpers
    bool enumerateDevices();
    bool sendForceFeedbackCmd(float left, float right, float magnitude);
};

} // namespace ks