#include "HilFfbInterface.h"
#include <QDebug>
#include <QCoreApplication>
#include <QSettings>

// Simplified HIL FFB implementation using Windows API
// In a full implementation, this would use DirectInput or RawInput

namespace ks {

HilFfbInterface::HilFfbInterface(QObject* parent)
    : QObject(parent)
{
}

HilFfbInterface::~HilFfbInterface() {
    shutdown();
}

bool HilFfbInterface::initialize() {
    if (m_isInitialized) return true;

    // Attempt to enumerate available FFB devices
    bool found = enumerateDevices();

    m_isInitialized = true;
    return found;
}

void HilFfbInterface::shutdown() {
    m_isInitialized = false;
    disconnectWheel();
}

bool HilFfbInterface::connectWheel(const QString& deviceName) {
    if (!m_isInitialized) return false;

    m_connectedDevice = deviceName;
    // In a real implementation, would use DirectInput to open the device
    // and set up force feedback coefficients

    qInfo() << "HIL: Connected to FFB device:" << deviceName;
    emit wheelConnected(deviceName);
    return true;
}

void HilFfbInterface::disconnectWheel() {
    if (!m_connectedDevice.isEmpty()) {
        qInfo() << "HIL: Disconnected from FFB device:" << m_connectedDevice;
        emit wheelDisconnected();
    }
    m_connectedDevice.clear();
    m_wheelHandle = nullptr;
}

void HilFfbInterface::updateFfbForce(float lateralForce, float longitudinalForce,
                                      float aligningMoment, float slipRatio) {
    m_currentLateralForce = lateralForce;
    m_currentLongitudinalForce = longitudinalForce;
    m_currentAligningMoment = aligningMoment;
    m_currentSlipRatio = slipRatio;

    // Convert tire forces to FFB output
    // This maps tire forces to wheel force feedback magnitudes

    // Calculate overall force magnitude
    float totalForce = qSqrt(lateralForce * lateralForce +
                             longitudinalForce * longitudinalForce);

    // Normalize and clamp
    float maxFfbForce = 50.0f;  // Maximum FFB output in Newtons
    float forceMagnitude = qBound(-maxFfbForce, totalForce, maxFfbForce);

    // Apply aligning moment (self-aligning torque) to wheel rotation simulation
    float momentScale = 0.1f;  // Tuning parameter
    float effectiveMoment = aligningMoment * momentScale;

    // Send to wheel device
    sendForceFeedbackCmd(forceMagnitude, effectiveMoment, slipRatio);

    emit ffbForceUpdated(lateralForce, longitudinalForce,
                         aligningMoment, slipRatio);
}

bool HilFfbInterface::enumerateDevices() {
    // Simplified device enumeration
    // In a real implementation, would query Windows for connected
    // Direct Drive wheels (Simucube, Fanatec, Thrustmaster, etc.)

    // For now, report common supported devices
    QStringList supportedDevices = {
        "Simucube 2 Pro",
        "Simucube 1",
        "Fanatec CSL DD",
        "Fanatec DD1/DD2",
        "Thrustmaster T300 RS",
        "Thrustmaster TS-XW",
        "Logitech G Pro FFB",
        "Logitech G29/G923"
    };

    // Try to detect if any are connected (placeholder - would use DirectInput)
    bool deviceFound = false;

    // Check each potentially connected device
    for (const QString& dev : supportedDevices) {
        // Placeholder: in real code, would call IDirectInput8::Enumerate
        // or WinMM joyGetPosEx to detect presence
        m_connectedDevice = dev;  // Assume first one for demo
        deviceFound = true;
        break;
    }

    if (deviceFound) {
        emit wheelConnected(m_connectedDevice);
    } else {
        m_connectedDevice = "No Device Connected";
    }

    return deviceFound;
}

bool HilFfbInterface::sendForceFeedbackCmd(float leftMagnitude, float rightMagnitude,
                                           float moment) {
    if (!m_isInitialized || m_connectedDevice.isEmpty()) return false;

    // In a real implementation, this would send commands to the wheel via:
    // - DirectOutput plugin interface
    // - WinMM joystick commands
    // - USB vendor-specific commands
    // - Manufacturer SDK (Simucube, Fanatec, etc.)

    // For now, just log the intended FFB values
    qDebug() << "HIL FFB:" << m_connectedDevice
             << "Left:" << leftMagnitude << "Nm"
             << "Right:" << rightMagnitude << "Nm"
             << "Moment:" << moment << "Nm";

    return true;
}

} // namespace ks