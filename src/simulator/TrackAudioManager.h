// Track Audio Manager - 3D positioned events from audio_config.ini
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

#include "engine/physics/PhysicsCoreTypes.h"
#include "simulator/SimulatorAudio.h"

namespace ks::sim {

enum class AttenuationModel { Inverse, Linear, Logarithmic };

struct AudioEventConfig {
    std::string name;
    std::string type;

    // Trigger: fires while `trigger_variable` compares against the value.
    // Empty variable = always triggered.
    std::string trigger_variable;
    std::string trigger_condition;        // "lt" | "gt" | "eq"
    float trigger_condition_value = 0.0f;
    bool loop = false;

    // Camera gain multipliers (interior / chase-free / tv-replay).
    float camera_interior_mult = 1.0f;
    float camera_exterior_mult = 1.0f;
    float camera_track_mult = 1.0f;

    // Positional playback
    AttenuationModel attenuation = AttenuationModel::Inverse;
    float minDistance = 1.0f;
    float maxDistance = 120.0f;
};

struct TrackAudioEvent {
    AudioEventConfig config;
    float last_trigger_value = 0.0f;
    bool is_playing = false;
    float playback_position = 0.0f;
    float timer = 0.0f;
};

class TrackAudioManager {
public:
    TrackAudioManager();
    ~TrackAudioManager();

    bool initialize();
    void unload();

    bool loadConfig(const std::string& configPath);
    void update(float dt, const physics::WeatherState& weather,
                SimulatorAudio::CameraMode cameraMode);

    const std::vector<TrackAudioEvent>& events() const { return m_events; }

private:
    float deriveTriggerValue(const std::string& variable,
                             const physics::WeatherState& weather,
                             SimulatorAudio::CameraMode cameraMode) const;
    float applyCameraMultiplier(TrackAudioEvent& ev,
                                SimulatorAudio::CameraMode cameraMode) const;

    std::vector<TrackAudioEvent> m_events;
    std::unordered_map<std::string, int> m_eventIndex;
    SoundsIniData m_soundsIni;
    std::mutex m_mutex;
};

} // namespace ks::sim
