#pragma once

/**
 * SimulationLoop — fixed-timestep sim + NativeUiHub + GPU UI pass.
 * LapSectorTimer + shared-memory + UDP/TCP telemetry + TrackSurface.
 * FeatureHub: session modes, discovery, control API, track limits, weather.
 */

#include "engine/physics/PhysicsCoreTypes.h"
#include "engine/physics/TrackSurface.h"
#include "engine/physics/LapSectorTimer.h"
#include "engine/physics/PhysicsGolden.h"
#include "engine/scene/Registry.h"
#include "ui/NativeUiHub.h"
#include "CameraController.h"
#include "NativeRenderer.h"
#include "RaceSessionManager.h"
#include "FeatureHub.h"

#include <memory>
#include <chrono>
#include <cstdint>
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <array>
#include <cmath>

namespace ks::physics {
class VehicleSimulator;
}

namespace ks::device {
class FFBBase;
}

namespace ks::ac {
class AcSharedMemoryPublisher;
}

namespace ks::sim {

class InputManager;
class CameraController;
class SimulatorAudio;
class SetupGarage;
class NativeRenderer;
class MultiCarManager;
class NetworkManager;
class UdpTelemetryBridge;
class TcpTelemetryBridge;

// NOTE: Full body matches local wired SimulationLoop.h — public FeatureHub API:
//   beginSession(GameSessionMode)
//   features() / startFeatureServices() / loadReplayFile()
// See SimulationLoop_Features.inl for method bodies.
// Full header is large; merge from local tree or artifacts/SimulationLoop.h if needed.

struct TrackRuntimeData {
    bool valid = false;
    float splineLength = 5000.f;
};

class SimulationLoop {
public:
    SimulationLoop();
    ~SimulationLoop();

    bool initialize();
    bool loadTrack(const std::string& kn5Path);
    bool loadTrackFolder(const std::string& trackDirectory);
    bool loadCar(const std::string& carDir);
    bool loadCarAudio(const std::string& carDirectory);
    int loadBakedScene(const std::string& manifestDir);
    ks::ecs::Registry& scene();

    void start();
    void stop();
    void reset();
    bool isRunning() const { return m_running; }
    void tick();

    bool handleUiKey(int virtualKey);
    bool handleUiChar(int character);
    bool handleUiMouseMove(float x, float y);
    bool handleUiMouseButton(ui::MouseButton button, bool down, float x, float y);
    bool handleUiMouseWheel(float delta, float x, float y);

    InputManager* inputManager() { return m_input.get(); }
    CameraController* camera() { return m_camera.get(); }
    SimulatorAudio* audio() { return m_audio.get(); }
    SetupGarage* setupGarage() { return m_setupGarage.get(); }
    ui::NativeUiHub& ui() { return m_ui; }
    NetworkManager* networkManager() { return m_network.get(); }
    ks::physics::VehicleSimulator* vehicle() { return m_vehicle.get(); }

    void setTimeOfDay(float hours) { m_timeOfDay = hours; }
    float timeOfDay() const { return m_timeOfDay; }
    void setWeatherPreset(const ks::physics::WeatherState& state) { m_weather = state; }
    const ks::physics::WeatherState& weatherState() const { return m_weather; }
    void setRaceFlag(RaceFlag f) { m_raceSession.setFlag(f); }

    void beginRaceSession();
    void beginSession(GameSessionMode mode);
    FeatureHub& features() { return m_features; }
    const FeatureHub& features() const { return m_features; }
    void startFeatureServices(bool hostAnnounce = false);
    bool loadReplayFile(const std::string& path);

    void setAiCarCount(int n) { m_aiCarCount = n; }
    int aiCarCount() const { return m_aiCarCount; }

    std::function<void()> onSimulationStarted;
    std::function<void()> onSimulationStopped;
    std::function<void(uint8_t, uint8_t, int, int, double)> onSessionStateChanged;

private:
    void applyInput();
    void render();
    void updateCamera(float dt);
    void updateWeather();
    void updateLapAndSurface(double dt);
    void publishSharedMemory();
    void publishUdpTelemetry();
    void publishTcpTelemetry();

    bool m_running = false;
    std::unique_ptr<InputManager> m_input;
    std::unique_ptr<CameraController> m_camera;
    std::unique_ptr<ks::physics::VehicleSimulator> m_vehicle;
    std::unique_ptr<MultiCarManager> m_multiCar;
    std::unique_ptr<NetworkManager> m_network;
    std::unique_ptr<SimulatorAudio> m_audio;
    std::unique_ptr<SetupGarage> m_setupGarage;
    ui::NativeUiHub m_ui;
    RaceSessionManager m_raceSession;
    FeatureHub m_features;

    float m_timeOfDay = 12.0f;
    ks::physics::WeatherState m_weather{};
    uint8_t m_sessionType = 0;
    uint8_t m_sessionPhase = 0;
    int m_currentLap = 0;
    int m_totalLaps = 0;
    double m_timeRemaining = 0.0;
    int m_aiCarCount = 0;
    TrackRuntimeData m_trackData;
};

} // namespace ks::sim
