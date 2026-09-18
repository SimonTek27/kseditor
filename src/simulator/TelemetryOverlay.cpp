#include "TelemetryOverlay.h"
#include "UdpTelemetryListener.h"
#include <algorithm>
#include <cmath>

namespace ks::sim {

TelemetryOverlay::TelemetryOverlay()
    : m_speedHistory(HISTORY_SIZE, 0.0f)
    , m_rpmHistory(HISTORY_SIZE, 0.0f)
    , m_throttleHistory(HISTORY_SIZE, 0.0f)
    , m_brakeHistory(HISTORY_SIZE, 0.0f)
    , m_steeringHistory(HISTORY_SIZE, 0.0f)
    , m_lateralGHistory(HISTORY_SIZE, 0.0f)
    , m_longitudinalGHistory(HISTORY_SIZE, 0.0f)
{
}

TelemetryOverlay::~TelemetryOverlay() {}

void TelemetryOverlay::update(float speed, float rpm, float throttle, float brake,
                              float steering, float lateralG, float longitudinalG)
{
    m_speed = speed;
    m_rpm = rpm;
    m_throttle = throttle;
    m_brake = brake;
    m_steering = steering;
    m_lateralG = lateralG;
    m_longitudinalG = longitudinalG;

    m_speedHistory.push_back(speed);
    m_rpmHistory.push_back(rpm);
    m_throttleHistory.push_back(throttle);
    m_brakeHistory.push_back(brake);
    m_steeringHistory.push_back(steering);
    m_lateralGHistory.push_back(lateralG);
    m_longitudinalGHistory.push_back(longitudinalG);

    if ((int)m_speedHistory.size() > HISTORY_SIZE) m_speedHistory.erase(m_speedHistory.begin());
    if ((int)m_rpmHistory.size() > HISTORY_SIZE) m_rpmHistory.erase(m_rpmHistory.begin());
    if ((int)m_throttleHistory.size() > HISTORY_SIZE) m_throttleHistory.erase(m_throttleHistory.begin());
    if ((int)m_brakeHistory.size() > HISTORY_SIZE) m_brakeHistory.erase(m_brakeHistory.begin());
    if ((int)m_steeringHistory.size() > HISTORY_SIZE) m_steeringHistory.erase(m_steeringHistory.begin());
    if ((int)m_lateralGHistory.size() > HISTORY_SIZE) m_lateralGHistory.erase(m_lateralGHistory.begin());
    if ((int)m_longitudinalGHistory.size() > HISTORY_SIZE) m_longitudinalGHistory.erase(m_longitudinalGHistory.begin());

    float loadFactor = std::abs(lateralG) * 0.5f + throttle * 0.3f;
    m_tireTempFL = std::clamp(80.0f + loadFactor * 30.0f, 40.0f, 120.0f);
    m_tireTempFR = std::clamp(82.0f + loadFactor * 30.0f, 40.0f, 120.0f);
    m_tireTempRL = std::clamp(75.0f + loadFactor * 20.0f, 40.0f, 120.0f);
    m_tireTempRR = std::clamp(77.0f + loadFactor * 20.0f, 40.0f, 120.0f);
}

void TelemetryOverlay::updateFromAcTelemetry(const AcTelemetryData& acData)
{
    // Update core telemetry values
    m_speed = acData.speed;
    m_rpm = acData.rpm;
    m_throttle = acData.throttle;
    m_brake = acData.brake;
    m_steering = acData.steering;
    m_lateralG = acData.lateralG;
    m_longitudinalG = acData.longitudinalG;

    // Update AC-specific fields
    m_fuel = acData.fuel;
    m_fuelDelta = acData.fuelDelta;
    m_tyreWearFL = acData.tyreWearFL;
    m_tyreWearFR = acData.tyreWearFR;
    m_tyreWearRL = acData.tyreWearRL;
    m_tyreWearRR = acData.tyreWearRR;
    m_tyreTemperatureFL = acData.tyreTemperatureFL;
    m_tyreTemperatureFR = acData.tyreTemperatureFR;
    m_tyreTemperatureRL = acData.tyreTemperatureRL;
    m_tyreTemperatureRR = acData.tyreTemperatureRR;
    m_powerUnitTemperature = acData.powerUnitTemperature;
    m_brakeTemperature = acData.brakeTemperature;
    m_localSpeedX = acData.localSpeedX;
    m_localSpeedY = acData.localSpeedY;
    m_localSpeedZ = acData.localSpeedZ;
    m_angularVelocityX = acData.angularVelocityX;
    m_angularVelocityY = acData.angularVelocityY;
    m_angularVelocityZ = acData.angularVelocityZ;
    m_pitch = acData.pitch;
    m_roll = acData.roll;
    m_yaw = acData.yaw;
    m_sessionType = acData.sessionType;
    m_trackId = acData.trackId;
    m_trackConfiguration = acData.trackConfiguration;
    m_windSpeed = acData.windSpeed;
    m_windDirection = acData.windDirection;
    m_airDensity = acData.airDensity;
    m_airTemp = acData.airTemp;
    m_grassAmount = acData.grassAmount;
    m_splitTime = acData.splitTime;
    m_lapTime = acData.lapTime;
    m_bestLapTime = acData.bestLapTime;
    m_nLaps = acData.nLaps;
    m_racePos = acData.racePos;
    m_isInPit = acData.isInPit;
    m_isRacePaused = acData.isRacePaused;
    m_isTimeAccelerated = acData.isTimeAccelerated;

    // Update history buffers
    m_speedHistory.push_back(m_speed);
    m_rpmHistory.push_back(m_rpm);
    m_throttleHistory.push_back(m_throttle);
    m_brakeHistory.push_back(m_brake);
    m_steeringHistory.push_back(m_steering);
    m_lateralGHistory.push_back(m_lateralG);
    m_longitudinalGHistory.push_back(m_longitudinalG);

    // Trim history
    if ((int)m_speedHistory.size() > HISTORY_SIZE) m_speedHistory.erase(m_speedHistory.begin());
    if ((int)m_rpmHistory.size() > HISTORY_SIZE) m_rpmHistory.erase(m_rpmHistory.begin());
    if ((int)m_throttleHistory.size() > HISTORY_SIZE) m_throttleHistory.erase(m_throttleHistory.begin());
    if ((int)m_brakeHistory.size() > HISTORY_SIZE) m_brakeHistory.erase(m_brakeHistory.begin());
    if ((int)m_steeringHistory.size() > HISTORY_SIZE) m_steeringHistory.erase(m_steeringHistory.begin());
    if ((int)m_lateralGHistory.size() > HISTORY_SIZE) m_lateralGHistory.erase(m_lateralGHistory.begin());
    if ((int)m_longitudinalGHistory.size() > HISTORY_SIZE) m_longitudinalGHistory.erase(m_longitudinalGHistory.begin());
}

void TelemetryOverlay::render(int width, int height)
{
    (void)width; (void)height;
    if (!m_visible) return;
    // Stubbed: No QPainter available in Win32/Vulkan path.
}

} // namespace ks::sim
