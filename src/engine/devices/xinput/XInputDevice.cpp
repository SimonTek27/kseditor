#include "XInputDevice.h"

#ifdef _WIN32

namespace ks::device {

XInputDevice::XInputDevice(QObject* parent)
    : QObject(parent)
{
    // Check if XInput is available
    m_supported = true;
}

XInputDevice::~XInputDevice()
{
    shutdown();
}

bool XInputDevice::initialize()
{
    if (!m_supported) return false;

    // Try to detect connected controller
    DWORD result = XInputGetState(m_playerIndex, &m_state);
    if (result == ERROR_SUCCESS) {
        m_connected = true;
        m_hasRumble = true;
        emit connected();
        return true;
    }

    m_connected = false;
    return false;
}

void XInputDevice::shutdown()
{
    stopRumble();
    m_connected = false;
}

void XInputDevice::update()
{
    if (!m_connected) return;

    DWORD result = XInputGetState(m_playerIndex, &m_state);
    if (result != ERROR_SUCCESS) {
        if (m_connected) {
            m_connected = false;
            emit disconnected();
        }
        return;
    }

    // Thumbsticks with dead zone
    double LX = m_state.Gamepad.sThumbLX;
    double LY = m_state.Gamepad.sThumbLY;
    double RX = m_state.Gamepad.sThumbRX;
    double RY = m_state.Gamepad.sThumbRY;

    // Normalize and apply dead zone
    double magnitudeL = std::sqrt(LX * LX + LY * LY);
    if (magnitudeL > m_deadZone * 32767.0) {
        m_leftStickX = LX / 32767.0;
        m_leftStickY = LY / 32767.0;
    } else {
        m_leftStickX = 0;
        m_leftStickY = 0;
    }

    double magnitudeR = std::sqrt(RX * RX + RY * RY);
    if (magnitudeR > m_deadZone * 32767.0) {
        m_rightStickX = RX / 32767.0;
        m_rightStickY = RY / 32767.0;
    } else {
        m_rightStickX = 0;
        m_rightStickY = 0;
    }

    // Apply steering gamma
    applyStickCurve(m_leftStickX, m_steerGamma);
    m_steer = m_leftStickX;

    // Triggers (0-255 range)
    double triggerL = m_state.Gamepad.bLeftTrigger / 255.0;
    double triggerR = m_state.Gamepad.bRightTrigger / 255.0;

    // Apply threshold
    m_brake = (triggerL > m_triggerThreshold) ? triggerL : 0.0;
    m_throttle = (triggerR > m_triggerThreshold) ? triggerR : 0.0;

    // Store raw values
    m_rawThrottle = m_throttle;
    m_rawBrake = m_brake;
    m_rawSteer = m_steer;

    // Clutch (using A button as digital clutch)
    m_clutch = (m_state.Gamepad.wButtons & XINPUT_GAMEPAD_A) ? 1.0 : 0.0;

    // Buttons
    m_buttonA = (m_state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0;
    m_buttonB = (m_state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0;
    m_buttonX = (m_state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0;
    m_buttonY = (m_state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0;
    m_buttonLB = (m_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
    m_buttonRB = (m_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
    m_buttonBack = (m_state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0;
    m_buttonStart = (m_state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0;
    m_buttonGuide = (m_state.Gamepad.wButtons & 0x0400) != 0;  // Guide button

    // D-pad
    m_dpadUp = (m_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0;
    m_dpadDown = (m_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
    m_dpadLeft = (m_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
    m_dpadRight = (m_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;

    // Button edge detection
    WORD pressed = m_state.Gamepad.wButtons & ~m_prevButtons;
    if (pressed & XINPUT_GAMEPAD_A) emit buttonPressed(0);
    if (pressed & XINPUT_GAMEPAD_B) emit buttonPressed(1);
    if (pressed & XINPUT_GAMEPAD_X) emit buttonPressed(2);
    if (pressed & XINPUT_GAMEPAD_Y) emit buttonPressed(3);
    if (pressed & XINPUT_GAMEPAD_LEFT_SHOULDER) emit buttonPressed(4);
    if (pressed & XINPUT_GAMEPAD_RIGHT_SHOULDER) emit buttonPressed(5);
    if (pressed & XINPUT_GAMEPAD_BACK) emit buttonPressed(6);
    if (pressed & XINPUT_GAMEPAD_START) emit buttonPressed(7);

    WORD released = m_prevButtons & ~m_state.Gamepad.wButtons;
    if (released & XINPUT_GAMEPAD_A) emit buttonReleased(0);
    if (released & XINPUT_GAMEPAD_B) emit buttonReleased(1);
    if (released & XINPUT_GAMEPAD_X) emit buttonReleased(2);
    if (released & XINPUT_GAMEPAD_Y) emit buttonReleased(3);
    if (released & XINPUT_GAMEPAD_LEFT_SHOULDER) emit buttonReleased(4);
    if (released & XINPUT_GAMEPAD_RIGHT_SHOULDER) emit buttonReleased(5);
    if (released & XINPUT_GAMEPAD_BACK) emit buttonReleased(6);
    if (released & XINPUT_GAMEPAD_START) emit buttonReleased(7);

    m_prevButtons = m_state.Gamepad.wButtons;

    // Apply rumble
    std::lock_guard<std::mutex> lock(m_rumbleMutex);
    if (m_rumbleDirty) {
        XINPUT_VIBRATION vibration;
        vibration.wLeftMotorSpeed = static_cast<WORD>(m_rumbleLeft * 65535.0);
        vibration.wRightMotorSpeed = static_cast<WORD>(m_rumbleRight * 65535.0);
        XInputSetState(m_playerIndex, &vibration);
        m_rumbleDirty = false;
    }
}

void XInputDevice::setRumble(double leftMotor, double rightMotor)
{
    std::lock_guard<std::mutex> lock(m_rumbleMutex);
    m_rumbleLeft = std::clamp(leftMotor, 0.0, 1.0);
    m_rumbleRight = std::clamp(rightMotor, 0.0, 1.0);
    m_rumbleDirty = true;
}

void XInputDevice::stopRumble()
{
    setRumble(0.0, 0.0);
}

void XInputDevice::applyFFB(float torqueNm, float speedKph)
{
    // Convert torque to rumble pattern
    double absTorque = std::abs(torqueNm);
    double normalizedTorque = std::clamp(absTorque / 5.0, 0.0, 1.0);  // Normalize to 0-1

    // Scale by FFB strength
    normalizedTorque *= m_ffbStrength;

    // Speed factor (less FFB at low speeds)
    double speedFactor = std::clamp(speedKph / 50.0, 0.2, 1.0);

    double finalForce = normalizedTorque * speedFactor;

    // Small forces → right motor (fine detail)
    // Large forces → both motors (engine + road)
    double rightMotor = finalForce;
    double leftMotor = (finalForce > 0.3) ? finalForce * 0.6 : 0.0;

    setRumble(leftMotor, rightMotor);
}

void XInputDevice::applyDeadZone(double& value, double deadZone) const
{
    double absVal = std::abs(value);
    if (absVal < deadZone) {
        value = 0.0;
    } else {
        value = (value > 0) ? (value - deadZone) / (1.0 - deadZone)
                           : -(absVal - deadZone) / (1.0 - deadZone);
    }
}

void XInputDevice::applyStickCurve(double& value, double gamma) const
{
    double sign = (value >= 0) ? 1.0 : -1.0;
    double absVal = std::abs(value);
    value = sign * std::pow(absVal, gamma);
}

} // namespace ks::device

#endif // _WIN32