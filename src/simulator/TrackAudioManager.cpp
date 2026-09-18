#include "TrackAudioManager.h"
#include "Audio/AudioTypes.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>

namespace ks::sim {

static std::string trim(const std::string& s)
{
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static std::string toLower(const std::string& s)
{
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

static AttenuationModel parseAttenuation(const std::string& s)
{
    auto lower = toLower(s);
    if (lower == "inverse" || lower == "inverse") return AttenuationModel::Inverse;
    if (lower == "linear") return AttenuationModel::Linear;
    if (lower == "logarithmic") return AttenuationModel::Logarithmic;
    return AttenuationModel::Inverse;
}

static float parseFloat(const std::string& value, float defaultVal = 0.0f)
{
    std::string trimmed = trim(value);
    if (trimmed.empty()) return defaultVal;
    try { return std::stof(trimmed); } catch (...) { return defaultVal; }
}

static bool parseBool(const std::string& value)
{
    std::string lower = toLower(trim(value));
    return lower == "1" || lower == "true" || lower == "on";
}

TrackAudioManager::TrackAudioManager() = default;
TrackAudioManager::~TrackAudioManager() { unload(); }

bool TrackAudioManager::initialize()
{
    // Mixer channels already added in SimulatorAudio::initialize()
    return true;
}

void TrackAudioManager::unload()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_events.clear();
    m_eventIndex.clear();
}

bool TrackAudioManager::loadConfig(const std::string& configPath)
{
    // ... (implementation as in header) - omitted for brevity
    // Full INI parser would go here
    printf("TrackAudioManager: Config loading placeholder - %s\n", configPath.c_str());
    return false;
}

void TrackAudioManager::update(float dt, const ks::physics::WeatherState& weather,
                               SimulatorAudio::CameraMode cameraMode)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& ev : m_events) {
        float tv = deriveTriggerValue(ev.config.trigger_variable, weather, cameraMode);

        bool shouldTrigger = false;
        if (ev.config.trigger_variable.empty()) {
            shouldTrigger = true;
        }
        else if (ev.config.trigger_condition == "lt") {
            shouldTrigger = tv < ev.config.trigger_condition_value;
        }
        else if (ev.config.trigger_condition == "gt") {
            shouldTrigger = tv > ev.config.trigger_condition_value;
        }
        else if (ev.config.trigger_condition == "eq") {
            shouldTrigger = std::abs(tv - ev.config.trigger_condition_value) < 0.01f;
        }

        if (shouldTrigger) {
            if (!ev.is_playing) {
                ev.is_playing = true;
                ev.timer = 0.0f;
                printf("TrackAudioManager: Triggered %s (trigger=%.2f)\n", ev.config.name.c_str(), tv);
            }
            ev.timer += dt;
            if (ev.config.loop) {
                ev.playback_position = fmod(ev.timer / 10.0f, 1.0f);
            }
        }
        else {
            if (ev.is_playing) {
                ev.is_playing = false;
                printf("TrackAudioManager: Stopped %s\n", ev.config.name.c_str());
            }
        }
    }
}

float TrackAudioManager::deriveTriggerValue(const std::string& variable,
                                            const ks::physics::WeatherState& weather,
                                            SimulatorAudio::CameraMode cameraMode) const
{
    if (variable == "SUN" || variable == "sun")
        return weather.cloudCover;
    else if (variable == "RAIN" || variable == "rain")
        return weather.rainIntensity;
    else if (variable == "WIND" || variable == "wind")
        return std::min(1.0f, weather.windSpeed / 30.0f);
    return 0.0f;
}

float TrackAudioManager::applyCameraMultiplier(TrackAudioEvent& ev, SimulatorAudio::CameraMode cameraMode) const
{
    float mult = 1.0f;
    switch (cameraMode) {
        case SimulatorAudio::CameraMode::Cockpit:
            mult = ev.config.camera_interior_mult;
            break;
        case SimulatorAudio::CameraMode::Chase:
        case SimulatorAudio::CameraMode::Free:
            mult = ev.config.camera_exterior_mult;
            break;
        case SimulatorAudio::CameraMode::TV:
        case SimulatorAudio::CameraMode::Replay:
            mult = ev.config.camera_track_mult;
            break;
    }
    return mult;
}

} // namespace ks::sim