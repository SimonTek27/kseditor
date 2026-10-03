#pragma once

/**
 * SimulationLoop — fixed-timestep sim + pit stack + telemetry + damage HUD + snap hold.
 */

#include "engine/physics/PhysicsCoreTypes.h"
#include "engine/physics/TrackSurface.h"
#include "engine/physics/LapSectorTimer.h"
#include "engine/physics/PhysicsGolden.h"
#include "engine/physics/DamageSystem.h"
#include "engine/scene/Registry.h"
#include "ui/NativeUiHub.h"
#include "CameraController.h"
#include "NativeRenderer.h"
#include "RaceSessionManager.h"
#include "FeatureHub.h"
#include "GarageExit.h"
#include "GarageSpawn.h"
#include "PitLaneQueue.h"
#include "PitLaneCollision.h"
#include "PitLaneRepair.h"

#include <memory>
#include <chrono>
#include <cstdint>
#include <string>
#include <functional>

namespace ks::physics { class VehicleSimulator; }
namespace ks::device { class FFBBase; }
namespace ks::ac { class AcSharedMemoryPublisher; }

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

struct TrackRuntimeData {
    bool valid = false;
    float splineLength = 5000.f;
    std::string name;
    std::string kn5Path;
    std::string directory;
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
    MultiCarManager* multiCar() { return m_multiCar.get(); }

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

    void updateGarageExit(float dt);
    void updatePitLane(float dt);
    void updatePitRepair(float dt);
    void configurePitAxis(float originX, float originZ, float headingRad, float lengthM = 120.f);
    void setupDefaultGarageLayout(int boxCount = 8);
    void spawnAiGrid(int count);
    void applyDamageEffects();
    bool loadGarageFromTrack(const std::string& trackDir);
    void snapVehicleToPose(ks::physics::VehicleSimulator* veh, const WorldPose& pose);
    float snapHoldRemaining() const { return m_snapHoldSec; }
    void requestPitService(bool on = true) { m_requestPitService = on; }

    GarageExitController& garageExit() { return m_garageExit; }
    PitLaneQueue& pitQueue() { return m_pitQueue; }
    PitLaneCollision& pitCollision() { return m_pitCollision; }
    PitLaneRepair& pitRepair() { return m_pitRepair; }
    GarageLayout& garageLayout() { return m_garageLayout; }

    void setAiCarCount(int n) { m_aiCarCount = n; }
    int aiCarCount() const { return m_aiCarCount; }
    void setSharedMemoryEnabled(bool e) { m_shmEnabled = e; }
    void setUdpTelemetry(bool e, const std::string& host = "127.0.0.1", uint16_t port = 9996) {
        m_udpEnabled = e; m_udpHost = host; m_udpPort = port;
    }
    void setTcpTelemetry(bool e, uint16_t port = 9997) { m_tcpEnabled = e; m_tcpPort = port; }

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
    ks::physics::LapSectorTimer m_lapTimer;
    FeatureHub m_features;
    GarageExitController m_garageExit;
    PitLaneQueue m_pitQueue;
    PitLaneCollision m_pitCollision;
    PitLaneRepair m_pitRepair;
    GarageLayout m_garageLayout;
    ks::physics::DamageSystem m_damage;
    bool m_pitSystemsReady = false;
    bool m_requestPitService = false;
    float m_snapHoldSec = 0.f;
    static constexpr float kSnapHoldDuration = 0.35f;
    static constexpr int kPlayerCarId = 0;

    float m_timeOfDay = 12.0f;
    ks::physics::WeatherState m_weather{};
    uint8_t m_sessionType = 0;
    uint8_t m_sessionPhase = 0;
    int m_currentLap = 0;
    int m_totalLaps = 0;
    double m_timeRemaining = 0.0;
    int m_aiCarCount = 0;
    TrackRuntimeData m_trackData;
    std::string m_carName;
    double m_physicsDt = 0.001;
    double m_simAccumulator = 0;
    double m_simTime = 0;
    double m_lapDistance = 0;
    float m_normalizedSpline = 0;
    bool m_trackLoaded = false;
    bool m_carLoaded = false;
    std::chrono::steady_clock::time_point m_lastTime{};

    std::unique_ptr<ks::ac::AcSharedMemoryPublisher> m_shm;
    std::unique_ptr<UdpTelemetryBridge> m_udp;
    std::unique_ptr<TcpTelemetryBridge> m_tcp;
    bool m_shmEnabled = true;
    bool m_udpEnabled = false;
    bool m_tcpEnabled = false;
    std::string m_udpHost = "127.0.0.1";
    uint16_t m_udpPort = 9996;
    uint16_t m_tcpPort = 9997;
};

} // namespace ks::sim
