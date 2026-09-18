#include "DashboardOverlay.h"
#include <cstdio>
#include <cmath>

namespace ks::sim {

DashboardOverlay::DashboardOverlay() = default;
DashboardOverlay::~DashboardOverlay() = default;

void DashboardOverlay::update(float speed, float rpm, int gear, float throttle, float brake,
                               int currentLap, float lapTime, float bestLapTime,
                               float lastLapTime, int position, int totalCars)
{
    m_speed = speed;
    m_rpm = rpm;
    m_gear = gear;
    m_throttle = throttle;
    m_brake = brake;
    m_currentLap = currentLap;
    m_lapTime = lapTime;
    m_bestLapTime = bestLapTime;
    m_lastLapTime = lastLapTime;
    m_position = position;
    m_totalCars = totalCars;
}

void DashboardOverlay::render(int width, int height)
{
    if (!m_visible) return;
    // Stubbed: No QPainter available in Win32/Vulkan path.
    // Dashboard will be rendered via Vulkan overlay or ImGui in the future.
}

std::string DashboardOverlay::formatTime(float seconds)
{
    if (seconds <= 0 || seconds > 1e8) return "--:--.---";
    int mins = static_cast<int>(seconds) / 60;
    float secs = seconds - mins * 60;
    char buf[32];
    snprintf(buf, sizeof(buf), "%d:%02d.%03d",
             mins,
             static_cast<int>(secs),
             static_cast<int>((secs - static_cast<int>(secs)) * 1000));
    return buf;
}

} // namespace ks::sim
