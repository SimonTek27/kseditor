#include "SetupGarage.h"
#include <algorithm>
#include <cstdio>

namespace ks::sim {

SetupGarage::SetupGarage() {}
SetupGarage::~SetupGarage() {}

void SetupGarage::update(float dt)
{
    float target = m_visible ? 1.0f : 0.0f;
    float speed = 5.0f;
    if (m_fadeIn < target) {
        m_fadeIn = std::min(m_fadeIn + dt * speed, target);
    } else if (m_fadeIn > target) {
        m_fadeIn = std::max(m_fadeIn - dt * speed, target);
    }

    float slideTarget = m_visible ? 0.0f : -300.0f;
    float slideSpeed = 800.0f;
    if (m_slideOffset < slideTarget) {
        m_slideOffset = std::min(m_slideOffset + dt * slideSpeed, slideTarget);
    } else if (m_slideOffset > slideTarget) {
        m_slideOffset = std::max(m_slideOffset - dt * slideSpeed, slideTarget);
    }
}

void SetupGarage::render(int width, int height)
{
    (void)width; (void)height;
    if (m_fadeIn <= 0.01f) return;
    // Stubbed: No QPainter available in Win32/Vulkan path.
}

bool SetupGarage::handleKeyPress(int key)
{
    if (!m_visible) return false;

    // Use Windows virtual key codes directly
    const int VKKey_Up = 0x26;
    const int VKKey_Down = 0x28;

    if (key == VKKey_Up) {
        m_selectedRow = (m_selectedRow - 1 + m_rowCount) % m_rowCount;
        return true;
    }
    if (key == VKKey_Down) {
        m_selectedRow = (m_selectedRow + 1) % m_rowCount;
        return true;
    }

    float delta = 0.0f;
    if (key == 0x2B || key == 0x3D) { // + or =
        delta = m_shiftHeld ? 0.1f : 0.05f;
    } else if (key == 0x2D) { // -
        delta = m_shiftHeld ? -0.1f : -0.05f;
    }

    if (delta != 0.0f) {
        switch (m_selectedRow) {
        case 0: m_setup.tirePressureFL = std::clamp(m_setup.tirePressureFL + delta, 1.0f, 3.5f); break;
        case 1: m_setup.tirePressureFR = std::clamp(m_setup.tirePressureFR + delta, 1.0f, 3.5f); break;
        case 2: m_setup.tirePressureRL = std::clamp(m_setup.tirePressureRL + delta, 1.0f, 3.5f); break;
        case 3: m_setup.tirePressureRR = std::clamp(m_setup.tirePressureRR + delta, 1.0f, 3.5f); break;
        case 4: m_setup.brakeBias = std::clamp(m_setup.brakeBias + delta * 0.01f, 0.30f, 0.75f); break;
        case 5: m_setup.rideHeightFront = std::clamp(m_setup.rideHeightFront + delta * 200.0f, 10.0f, 80.0f); break;
        case 6: m_setup.rideHeightRear = std::clamp(m_setup.rideHeightRear + delta * 200.0f, 10.0f, 80.0f); break;
        case 7: m_setup.frontWingAngle = std::clamp(m_setup.frontWingAngle + delta * 100.0f, 0.0f, 30.0f); break;
        case 8: m_setup.rearWingAngle = std::clamp(m_setup.rearWingAngle + delta * 100.0f, 0.0f, 30.0f); break;
        case 9: m_setup.diffPreload = std::clamp(m_setup.diffPreload + delta * 200.0f, 10.0f, 80.0f); break;
        }
        return true;
    }

    return false;
}

bool SetupGarage::handleKeyRelease(int key)
{
    (void)key;
    return false;
}

} // namespace ks::sim
