#pragma once

#include <string>

namespace ks::sim {

class DashboardOverlay {
public:
    DashboardOverlay();
    ~DashboardOverlay();

    void update(float speed, float rpm, int gear, float throttle, float brake,
                int currentLap, float lapTime, float bestLapTime,
                float lastLapTime, int position, int totalCars);

    // Stubbed - no QPainter in pure Win32/Vulkan path
    void render(int width, int height);

    void setVisible(bool v) { m_visible = v; }
    bool isVisible() const { return m_visible; }
    void setOpacity(float o) { m_opacity = o; }

private:
    static std::string formatTime(float seconds);

    bool m_visible = true;
    float m_opacity = 0.9f;

    float m_speed = 0;
    float m_rpm = 0;
    int m_gear = 0;
    float m_throttle = 0;
    float m_brake = 0;
    int m_currentLap = 0;
    float m_lapTime = 0;
    float m_bestLapTime = 1e9f;
    float m_lastLapTime = 0;
    int m_position = 1;
    int m_totalCars = 1;
};

} // namespace ks::sim
