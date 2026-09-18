#pragma once

#include <vector>
#include <string>
#include "UdpTelemetryListener.h"

namespace ks::sim {

// ============================================================================
// TelemetryOverlay - Displays and processes telemetry data
// ============================================================================

class TelemetryOverlay {
public:
    TelemetryOverlay();
    ~TelemetryOverlay();

    void update(float speed, float rpm, float throttle, float brake,
                float steering, float lateralG, float longitudinalG);

    // Update from AC telemetry data
    void updateFromAcTelemetry(const AcTelemetryData& acData);

    // Stubbed - no QPainter in pure Win32/Vulkan path
    void render(int width, int height);

    void setVisible(bool v) { m_visible = v; }
    bool isVisible() const { return m_visible; }
    void toggleVisible() { m_visible = !m_visible; }
    void setOpacity(float o) { m_opacity = o; }

    bool m_visible = false;
    float m_opacity = 0.8f;

    static constexpr int HISTORY_SIZE = 200;
    std::vector<float> m_speedHistory;
    std::vector<float> m_rpmHistory;
    std::vector<float> m_throttleHistory;
    std::vector<float> m_brakeHistory;
    std::vector<float> m_steeringHistory;
    std::vector<float> m_lateralGHistory;
    std::vector<float> m_longitudinalGHistory;

    float m_speed = 0;
    float m_rpm = 0;
    float m_throttle = 0;
    float m_brake = 0;
    float m_steering = 0;
    float m_lateralG = 0;
    float m_longitudinalG = 0;

    // AC-specific telemetry fields
    float m_fuel = 0.0f;
    float m_fuelDelta = 0.0f;
    float m_tyreWearFL = 0.0f;
    float m_tyreWearFR = 0.0f;
    float m_tyreWearRL = 0.0f;
    float m_tyreWearRR = 0.0f;
    float m_tyreTemperatureFL = 80.0f;
    float m_tyreTemperatureFR = 82.0f;
    float m_tyreTemperatureRL = 75.0f;
    float m_tyreTemperatureRR = 77.0f;
    float m_powerUnitTemperature = 0.0f;
    float m_brakeTemperature = 0.0f;
    float m_localSpeedX = 0.0f;
    float m_localSpeedY = 0.0f;
    float m_localSpeedZ = 0.0f;
    float m_angularVelocityX = 0.0f;
    float m_angularVelocityY = 0.0f;
    float m_angularVelocityZ = 0.0f;
    float m_pitch = 0.0f;
    float m_roll = 0.0f;
    float m_yaw = 0.0f;
    uint8_t m_sessionType = 0;
    uint8_t m_trackId = 0;
    uint8_t m_trackConfiguration = 0;
    float m_windSpeed = 0.0f;
    float m_windDirection = 0.0f;
    float m_airDensity = 0.0f;
    float m_airTemp = 0.0f;
    float m_grassAmount = 0.0f;
    float m_splitTime = 0.0f;
    float m_lapTime = 0.0f;
    float m_bestLapTime = 0.0f;
    int m_nLaps = 0;
    int m_racePos = 0;
    bool m_isInPit = false;
    bool m_isRacePaused = false;
    bool m_isTimeAccelerated = false;

    // Callback for when AC telemetry is received
    TelemetryCallback m_acTelemetryCallback;
};

} // namespace ks::sim
