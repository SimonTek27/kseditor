#pragma once

#include <QObject>
#ifdef _WIN32
#include <windows.h>
#include <XInput.h>
#endif
#include <atomic>
#include <thread>
#include <mutex>

namespace ks::device {

// ============================================================================
// XInputDevice — Xbox controller via XInput API with rumble/FFB
// ============================================================================
// Supports Xbox 360, Xbox One, Xbox Series X|S controllers via XInput.
// Provides normalized throttle/brake/steer/clutch + left/right trigger.
// Rumble: independent left (heavy) and right (fine) motors.
//
// Motor mapping:
//   Left motor  (low-frequency) → engine vibration, road texture, impact
//   Right motor (high-frequency) → surface detail, gear shift, ABS pulse
//
// FFB force mapping (torque Nm → motor speed):
//   Small forces → right motor only (fine detail)
//   Large forces → both motors (engine + road)
// ============================================================================
class XInputDevice : public QObject {
    Q_OBJECT
public:
    explicit XInputDevice(QObject* parent = nullptr);
    ~XInputDevice() override;

    // Lifecycle
    bool initialize();
    void shutdown();
    bool isSupported() const { return m_supported; }

    // Update — polls controller state, call once per frame
    void update();

    // Normalized outputs (range depends on axis)
    double throttle() const { return m_throttle; }    // 0.0 – 1.0 (right trigger)
    double brake()    const { return m_brake; }       // 0.0 – 1.0 (left trigger)
    double steer()    const { return m_steer; }       // -1.0 – 1.0 (left stick X)
    double clutch()   const { return m_clutch; }      // 0.0 – 1.0 (A button analog or 0/1)

    // Raw values (for HUD display)
    double rawThrottle() const { return m_rawThrottle; }
    double rawBrake()    const { return m_rawBrake; }
    double rawSteer()    const { return m_rawSteer; }

    // Buttons (digital)
    bool buttonA() const { return m_buttonA; }
    bool buttonB() const { return m_buttonB; }
    bool buttonX() const { return m_buttonX; }
    bool buttonY() const { return m_buttonY; }
    bool buttonLB() const { return m_buttonLB; }
    bool buttonRB() const { return m_buttonRB; }
    bool buttonBack() const { return m_buttonBack; }
    bool buttonStart() const { return m_buttonStart; }
    bool buttonGuide() const { return m_buttonGuide; }  // Xbox button
    bool dpadUp() const { return m_dpadUp; }
    bool dpadDown() const { return m_dpadDown; }
    bool dpadLeft() const { return m_dpadLeft; }
    bool dpadRight() const { return m_dpadRight; }

    // Thumbstick raw values
    double leftStickX() const { return m_leftStickX; }  // -1.0 – 1.0
    double leftStickY() const { return m_leftStickY; }  // -1.0 – 1.0
    double rightStickX() const { return m_rightStickX; }
    double rightStickY() const { return m_rightStickY; }

    // Rumble / FFB
    bool hasRumble() const { return m_hasRumble; }

    // Set vibration motors (0.0 – 1.0)
    void setRumble(double leftMotor, double rightMotor);
    void stopRumble();

    // FFB: apply torque from simulation (Nm) → vibration pattern
    // Positive torque = turn right, negative = turn left
    void applyFFB(float torqueNm, float speedKph);

    // Configuration
    void setDeadZone(double dz) { m_deadZone = dz; }
    void setSteerGamma(double g) { m_steerGamma = g; }
    void setTriggerThreshold(double t) { m_triggerThreshold = t; }
    void setFfbStrength(double s) { m_ffbStrength = s; }

    // Controller info
    int playerIndex() const { return m_playerIndex; }
    bool isConnected() const { return m_connected; }

signals:
    void connected();
    void disconnected();
    void buttonPressed(int button);
    void buttonReleased(int button);

private:
    void applyDeadZone(double& value, double deadZone) const;
    void applyStickCurve(double& value, double gamma) const;

    // XInput state
#ifdef _WIN32
    XINPUT_STATE m_state = {};
#endif
    int m_playerIndex = 0;  // 0 = first controller
    bool m_supported = false;
    bool m_connected = false;
    bool m_hasRumble = false;

    // Previous state for edge detection
#ifdef _WIN32
    WORD m_prevButtons = 0;
#else
    unsigned short m_prevButtons = 0;
#endif

    // Normalized outputs
    double m_throttle = 0, m_brake = 0, m_steer = 0, m_clutch = 0;
    double m_rawThrottle = 0, m_rawBrake = 0, m_rawSteer = 0;

    // Buttons
    bool m_buttonA = false, m_buttonB = false, m_buttonX = false, m_buttonY = false;
    bool m_buttonLB = false, m_buttonRB = false;
    bool m_buttonBack = false, m_buttonStart = false, m_buttonGuide = false;
    bool m_dpadUp = false, m_dpadDown = false, m_dpadLeft = false, m_dpadRight = false;

    // Thumbsticks
    double m_leftStickX = 0, m_leftStickY = 0;
    double m_rightStickX = 0, m_rightStickY = 0;

    // Configuration
    double m_deadZone = 0.15;       // 15% stick dead zone (XInput standard)
    double m_steerGamma = 1.8;      // Steering curve (1.0 = linear)
    double m_triggerThreshold = 0.05; // 5% trigger threshold
    double m_ffbStrength = 1.0;     // FFB strength multiplier

    // Rumble state
    std::mutex m_rumbleMutex;
    double m_rumbleLeft = 0;
    double m_rumbleRight = 0;
    bool m_rumbleDirty = false;
};

} // namespace ks::device
