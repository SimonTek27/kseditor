#pragma once

/**
 * SimulationLoop — fixed-timestep sim + session + optional Vulkan mesh list.
 * Std-only: std::string / std::vector / Mat4f / Vec3f (no Qt types).
 */

#include "engine/physics/PhysicsCoreTypes.h"
#include "engine/physics/TrackSurface.h"

#include <memory>
#include <chrono>
#include <cstdint>
#include <string>
#include <functional>
#include <vector>
#include <array>
#include <cmath>

class InputManager;
class CameraController;
class SimulatorAudio;
class DashboardOverlay;
class SetupGarage;
class TelemetryOverlay;

namespace ks {
class VulkanRenderer;
class VulkanRenderPass;
}

namespace ks::physics {
class VehicleSimulator;
}

namespace ks::device {
class FFBBase;
}

namespace ks::sim {

class MultiCarManager;
class NetworkManager;

namespace net {
struct InputData;
struct CarStateData;
}

struct Vec3f {
    float x = 0, y = 0, z = 0;
    Vec3f() = default;
    Vec3f(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    Vec3f operator+(Vec3f o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3f operator*(float s) const { return {x * s, y * s, z * s}; }
    float length() const { return std::sqrt(x * x + y * y + z * z); }
};

struct Mat4f {
    std::array<float, 16> m{
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    static Mat4f identity() { return Mat4f{}; }
    static Mat4f translation(float tx, float ty, float tz) {
        Mat4f r; r.m[12] = tx; r.m[13] = ty; r.m[14] = tz; return r;
    }
    static Mat4f translation(Vec3f t) { return translation(t.x, t.y, t.z); }
    Mat4f operator*(const Mat4f& o) const {
        Mat4f r;
        for (int c = 0; c < 4; ++c)
            for (int row = 0; row < 4; ++row)
                r.m[c * 4 + row] =
                    m[0 * 4 + row] * o.m[c * 4 + 0] +
                    m[1 * 4 + row] * o.m[c * 4 + 1] +
                    m[2 * 4 + row] * o.m[c * 4 + 2] +
                    m[3 * 4 + row] * o.m[c * 4 + 3];
        return r;
    }
    void translate(float tx, float ty, float tz) { *this = *this * translation(tx, ty, tz); }
};

struct RenderableMesh {
    std::string meshName;
    Mat4f transform = Mat4f::identity();
    int indexCount = 0;
    int firstIndex = 0;
};

struct SimTrackData {
    std::string name;
    std::string kn5Path;
    std::string directory;
    bool valid = false;
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
    SimulatorAudio* audio() { return m_audio.get(); }
    DashboardOverlay* dashboard() { return m_dashboard.get(); }
    SetupGarage* setupGarage() { return m_setupGarage.get(); }
    TelemetryOverlay* telemetry() { return m_telemetry.get(); }
    const SimTrackData& trackData() const { return m_trackData; }
    ks::physics::VehicleSimulator* vehicle() { return m_vehicle.get(); }
    MultiCarManager* multiCarManager() { return m_multiCar.get(); }
    NetworkManager* networkManager() { return m_network.get(); }

    bool isVulkanMode() const { return m_vulkanMode; }
    void setVulkanRenderer(ks::VulkanRenderer* r) { m_vulkanRenderer = r; }
    ks::VulkanRenderer* vulkanRenderer() { return m_vulkanRenderer; }
    ks::VulkanRenderer* renderer() { return m_vulkanRenderer; }

    void setTimeOfDay(float hours) { m_timeOfDay = hours; }
    float timeOfDay() const { return m_timeOfDay; }
    void setWeatherPreset(const ks::physics::WeatherState& state) { m_weather = state; }
    const ks::physics::WeatherState& weatherState() const { return m_weather; }

    void setFfbEnabled(bool e) { m_ffbEnabled = e; }
    bool ffbEnabled() const { return m_ffbEnabled; }
    const std::vector<RenderableMesh>& renderables() const { return m_renderables; }

    void applyRemoteInput(int clientIndex, const net::InputData& input);

    std::function<void()> onSimulationStarted;
    std::function<void()> onSimulationStopped;
    std::function<void(uint8_t, uint8_t, int, int, double)> onSessionStateChanged;

private:
    void applyInput();
    void render();
    void updateWeather();
    void broadcastLocalCarState();
    void handleRemoteCarState(uint32_t carId, const net::CarStateData& state);
    void ensureScenePipeline();
    static std::string readFileText(const std::string& path);

    bool m_vulkanMode = true;
    ks::VulkanRenderer* m_vulkanRenderer = nullptr;
    std::unique_ptr<InputManager> m_input;
    std::unique_ptr<CameraController> m_camera;
    std::unique_ptr<ks::physics::VehicleSimulator> m_vehicle;
    std::unique_ptr<MultiCarManager> m_multiCar;
    std::unique_ptr<NetworkManager> m_network;
    std::unique_ptr<SimulatorAudio> m_audio;
    std::unique_ptr<DashboardOverlay> m_dashboard;
    std::unique_ptr<SetupGarage> m_setupGarage;
    std::unique_ptr<TelemetryOverlay> m_telemetry;

    SimTrackData m_trackData;
    std::chrono::steady_clock::time_point m_lastTime{};
    double m_simAccumulator = 0;
    static constexpr double m_physicsDt = 0.001;

    std::unique_ptr<ks::device::FFBBase> m_ffb;
    bool m_ffbEnabled = true;
    bool m_running = false;
    bool m_trackLoaded = false;
    bool m_carLoaded = false;

    float m_timeOfDay = 12.0f;
    ks::physics::WeatherState m_weather{};

    uint8_t m_sessionType = 0;
    uint8_t m_sessionPhase = 0;
    int m_currentLap = 0;
    int m_totalLaps = 0;
    double m_timeRemaining = 0.0;

    std::vector<RenderableMesh> m_renderables;
    std::unique_ptr<ks::VulkanRenderPass> m_scenePass;
    bool m_pipelineInitialized = false;
    uint32_t m_streamlineFrameIndex = 0;
};

} // namespace ks::sim
