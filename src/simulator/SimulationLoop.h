#pragma once

#include "InputManager.h"
#include "CameraController.h"
#include "SimulatorAudio.h"
#include "DashboardOverlay.h"
#include "SetupGarage.h"
#include "TelemetryOverlay.h"
#include "TrackLoader.h"
#include "engine/physics/VehiclePhysics.h"
#include "engine/physics/PhysicsCoreTypes.h"
#include "engine/physics/TrackSurface.h"
#include "Graphics/RenderSystem.h"
#include "engine/devices/simracing/FFBSDKFactory.h"
#include <memory>
#include <chrono>
#include <cstdint>
#include <string>
#include <functional>

namespace ks::sim {
class MultiCarManager;
class NetworkManager;
struct CarEntry;
}

namespace ks::sim::net {
struct InputData;
struct CarStateData;
}

namespace ks { class VulkanRenderer; class VulkanRenderPass; }
namespace ks::engine::graphics { class RenderSystem; class StreamlineIntegration; }

namespace ks::sim {

struct RenderableMesh {
    QString meshName;
    QMatrix4x4 transform;
    int indexCount = 0;
    int firstIndex = 0;
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

    void start();
    void stop();
    void reset();
    bool isRunning() const { return m_running; }

    void tick();

    InputManager* inputManager() { return m_input.get(); }
    CameraController* camera() { return m_camera.get(); }
    ::ks::VulkanRenderer* renderer() { return m_vulkanRenderer; }
    SimulatorAudio* audio() { return m_audio.get(); }
    DashboardOverlay* dashboard() { return &m_dashboard; }
    SetupGarage* setupGarage() { return &m_setupGarage; }
    TelemetryOverlay* telemetry() { return &m_telemetry; }
    const TrackData& trackData() const { return m_trackData; }
    ks::physics::VehicleSimulator* vehicle() { return m_vehicle.get(); }
    MultiCarManager* multiCarManager() { return m_multiCar.get(); }

    bool isVulkanMode() const { return m_vulkanMode; }
    void setVulkanRenderer(::ks::VulkanRenderer* r) { m_vulkanRenderer = r; }
    ::ks::VulkanRenderer* vulkanRenderer() { return m_vulkanRenderer; }

    void setCameraMode(CameraController::Mode m) { m_camera->setMode(m); }

    void setTimeOfDay(float hours) { m_timeOfDay = hours; }
    float timeOfDay() const { return m_timeOfDay; }
    void setWeatherPreset(const ks::physics::WeatherState& state) { m_weather = state; }
    const ks::physics::WeatherState& weatherState() const { return m_weather; }

    std::function<void()> onSimulationStarted;
    std::function<void()> onSimulationStopped;
    std::function<void(uint8_t, uint8_t, int, int, double)> onSessionStateChanged;

public:
    NetworkManager* networkManager() { return m_network.get(); }
    void applyRemoteInput(int clientIndex, const net::InputData& input);

private:
    void applyInput();
    void render();
    void updateWeather();
    void broadcastLocalCarState();
    void handleRemoteCarState(uint32_t carId, const net::CarStateData& state);
    std::unique_ptr<NetworkManager> m_network;

    bool m_vulkanMode = true;
    ::ks::VulkanRenderer* m_vulkanRenderer = nullptr;
    std::unique_ptr<InputManager> m_input;
    std::unique_ptr<CameraController> m_camera;
    std::unique_ptr<ks::physics::VehicleSimulator> m_vehicle;
    std::unique_ptr<MultiCarManager> m_multiCar;
    std::unique_ptr<SimulatorAudio> m_audio;
    DashboardOverlay m_dashboard;
    SetupGarage m_setupGarage;
    TelemetryOverlay m_telemetry;
    TrackData m_trackData;
    TrackLoader m_trackLoader;

    std::chrono::steady_clock::time_point m_lastTime;
    double m_simAccumulator = 0;
    static constexpr double m_physicsDt = 0.001;

    double m_fps = 0;
    int m_physicsFrameCount = 0;
    double m_frameTime = 0;

    std::unique_ptr<ks::device::FFBBase> m_ffb;
    bool m_ffbEnabled = true;

    bool m_running = false;
    bool m_trackLoaded = false;
    bool m_carLoaded = false;

    float m_timeOfDay = 12.0f;
    ks::physics::WeatherState m_weather;
    SimulatorAudio::SurfaceType m_lastSurfaceType = SimulatorAudio::SurfaceType::Asphalt;

    uint8_t m_sessionType = 0;
    uint8_t m_sessionPhase = 0;
    int m_currentLap = 0;
    int m_totalLaps = 0;
    double m_timeRemaining = 0.0;

    QVector<RenderableMesh> m_renderables;
    std::unique_ptr<::ks::VulkanRenderPass> m_scenePass;
    bool m_pipelineInitialized = false;
    uint32_t m_streamlineFrameIndex = 0;
};

} // namespace ks::sim
