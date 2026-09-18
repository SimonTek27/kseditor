#include "InputManager.h"
#include <cstdio>
#include <cmath>
#include <algorithm>

#ifdef _WIN32
#include "devices/xinput/XInputDevice.h"
#endif

namespace ks::sim {

InputManager::InputManager() = default;
InputManager::~InputManager() = default;

bool InputManager::initialize()
{
#ifdef _WIN32
    m_xinput = std::make_unique<ks::device::XInputDevice>();
    if (m_xinput->initialize()) {
        printf("InputManager: Xbox controller ready\n");
    } else {
        printf("InputManager: No Xbox controller found, using keyboard only\n");
    }
#endif
    printf("InputManager: Initialized\n");
    return true;
}

void InputManager::update()
{
    m_shiftUp = false;
    m_shiftDown = false;

#ifdef _WIN32
    if (m_xinput && m_xinput->isConnected()) {
        processXInput();
        return;
    }
#endif
    processKeyboard();
}

void InputManager::processXInput()
{
#ifdef _WIN32
    m_xinput->update();

    m_throttle = m_xinput->throttle();
    m_brake = m_xinput->brake();
    m_steer = m_xinput->steer();
    m_clutch = m_xinput->clutch();

    static bool prevRB = false, prevLB = false;
    bool curRB = m_xinput->buttonRB();
    bool curLB = m_xinput->buttonLB();
    if (curRB && !prevRB) m_shiftUp = true;
    if (curLB && !prevLB) m_shiftDown = true;
    prevRB = curRB;
    prevLB = curLB;

    if (m_xinput->dpadUp()) m_shiftUp = true;
    if (m_xinput->dpadDown()) m_shiftDown = true;

    applyDeadZone(m_steer, m_deadZone);
    if (m_steer != 0.0) {
        double sign = (m_steer > 0) ? 1.0 : -1.0;
        m_steer = sign * std::pow(std::abs(m_steer), m_steeringGamma);
    }

    if (m_invertSteer) m_steer = -m_steer;

    m_throttle = std::clamp(m_throttle, 0.0, 1.0);
    m_brake = std::clamp(m_brake, 0.0, 1.0);
    m_steer = std::clamp(m_steer, -1.0, 1.0);

    m_rawThrottle = m_throttle;
    m_rawBrake = m_brake;
    m_rawSteer = m_steer;
#endif
}

bool InputManager::isXInputConnected() const
{
#ifdef _WIN32
    return m_xinput && m_xinput->isConnected();
#else
    return false;
#endif
}

void InputManager::processKeyboard()
{
    m_throttle = 0;
    if (isKeyDown('W') || isKeyDown(VK_UP))
        m_throttle = 1.0;

    m_brake = 0;
    if (isKeyDown('S') || isKeyDown(VK_DOWN))
        m_brake = 1.0;

    m_steer = 0;
    if (isKeyDown('A') || isKeyDown(VK_LEFT))
        m_steer -= 1.0;
    if (isKeyDown('D') || isKeyDown(VK_RIGHT))
        m_steer += 1.0;

    if (m_invertSteer) m_steer = -m_steer;

    bool curE = isKeyDown('E');
    bool curQ = isKeyDown('Q');
    if (curE && !m_prevE) m_shiftUp = true;
    if (curQ && !m_prevQ) m_shiftDown = true;
    m_prevE = curE;
    m_prevQ = curQ;

    applyDeadZone(m_steer, m_deadZone);
    if (m_steer != 0.0) {
        double sign = (m_steer > 0) ? 1.0 : -1.0;
        m_steer = sign * std::pow(std::abs(m_steer), m_steeringGamma);
    }

    m_throttle = std::clamp(m_throttle, 0.0, 1.0);
    m_brake = std::clamp(m_brake, 0.0, 1.0);
    m_steer = std::clamp(m_steer, -1.0, 1.0);

    m_rawThrottle = m_throttle;
    m_rawBrake = m_brake;
    m_rawSteer = m_steer;
}

void InputManager::applyDeadZone(double& value, double deadZone) const
{
    if (std::abs(value) < deadZone) {
        value = 0.0;
    } else {
        double sign = (value > 0) ? 1.0 : -1.0;
        value = sign * (std::abs(value) - deadZone) / (1.0 - deadZone);
    }
}

void InputManager::reset()
{
    m_throttle = m_brake = m_steer = m_clutch = 0;
    m_rawThrottle = m_rawBrake = m_rawSteer = 0;
    m_shiftUp = m_shiftDown = false;
    m_keys.clear();
}

} // namespace ks::sim
