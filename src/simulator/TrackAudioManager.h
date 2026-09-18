// Track Audio Manager - 3D positioned events from audio_config.ini
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

#include "core/engine/Physics/PhysicsCoreTypes.h"
#include "simulator/SimulatorAudio.h"

namespace ks::sim {

struct AudioEventConfig {
    std::string name;
    std::string type;
    // ... config fields
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

    std::vector<TrackAudioEvent> m_events;
    std::unordered_map<std::string, int> m_eventIndex;
    SimulatorAudio::SoundsIniData m_soundsIni;
    std::mutex m_mutex;
};

} // namespace ks::sim