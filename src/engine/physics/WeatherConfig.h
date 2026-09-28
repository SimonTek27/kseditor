#pragma once
/** Weather configuration — Qt-free. */
#include "PhysicsCoreTypes.h"
#include <string>

namespace ks {
namespace physics {

struct WeatherConfig {
    float ambientTempC = 25.0f;
    float trackTempC = 30.0f;
    float humidity = 0.5f;
    float windSpeedMs = 0.0f;
    float windDirDeg = 0.0f;
    float rainIntensity = 0.0f;
    float fogDensity = 0.0f;
    std::string presetName = "clear";
};

inline WeatherState weatherStateFromConfig(const WeatherConfig& c) {
    WeatherState s;
    s.ambientTempC = c.ambientTempC;
    s.trackTempC = c.trackTempC;
    s.humidity = c.humidity;
    s.windSpeedMs = c.windSpeedMs;
    s.windDirDeg = c.windDirDeg;
    s.rainIntensity = c.rainIntensity;
    return s;
}

} // namespace physics
} // namespace ks
