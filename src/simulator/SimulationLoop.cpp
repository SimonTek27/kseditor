/**
 * SimulationLoop.cpp — std-only implementation
 * Core tick never needs Qt. Optional HAS_VEHICLE_SIM / HAS_KSNET / HAS_FFB.
 */

#include "SimulationLoop.h"

#include <cstdio>
#include <cmath>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>

#ifndef HAS_VEHICLE_SIM
#define HAS_VEHICLE_SIM 0
#endif
#ifndef HAS_KSNET
#define HAS_KSNET 0
#endif
#ifndef HAS_FFB
#define HAS_FFB 0
#endif

#if HAS_VEHICLE_SIM
#include "engine/physics/VehiclePhysics.h"
#endif

namespace ks::sim {

static constexpr uint8_t SESSION_RACE = 2;
static constexpr uint8_t PHASE_COUNTDOWN = 1;
static constexpr uint8_t PHASE_GREEN_FLAG = 2;
static constexpr uint8_t PHASE_CHECKERED_FLAG = 4;

std::string SimulationLoop::readFileText(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

SimulationLoop::SimulationLoop()
    : m_vulkanMode(true)
{
#if HAS_VEHICLE_SIM
    m_vehicle = std::make_unique<ks::physics::VehicleSimulator>();
#endif
}

SimulationLoop::~SimulationLoop()
{
    stop();
#if HAS_FFB
    if (m_ffb) {
        m_ffb->shutdown();
        m_ffb.reset();
    }
#endif
}

bool SimulationLoop::initialize()
{
    return true;
}

bool SimulationLoop::loadTrack(const std::string& kn5Path)
{
    m_renderables.clear();
    m_trackData = SimTrackData{};
    m_trackData.kn5Path = kn5Path;
    m_trackData.name = std::filesystem::path(kn5Path).stem().string();

    namespace fs = std::filesystem;
    if (!fs::exists(kn5Path)) {
        std::fprintf(stderr, "SimulationLoop: track not found: %s\n", kn5Path.c_str());
        m_trackLoaded = false;
        return false;
    }

    m_trackData.valid = true;
    m_trackLoaded = true;
    std::fprintf(stderr, "SimulationLoop: track path registered %s\n", kn5Path.c_str());
    return true;
}

bool SimulationLoop::loadTrackFolder(const std::string& trackDirectory)
{
    namespace fs = std::filesystem;
    if (!fs::is_directory(trackDirectory)) {
        std::fprintf(stderr, "SimulationLoop: not a directory: %s\n", trackDirectory.c_str());
        return false;
    }
    m_trackData.directory = trackDirectory;
    m_trackData.name = fs::path(trackDirectory).filename().string();

    std::string kn5;
    for (auto& e : fs::directory_iterator(trackDirectory)) {
        if (e.path().extension() == ".kn5") {
            kn5 = e.path().string();
            break;
        }
    }
    if (kn5.empty()) {
        for (auto& e : fs::recursive_directory_iterator(trackDirectory)) {
            if (e.path().extension() == ".kn5") {
                kn5 = e.path().string();
                break;
            }
        }
    }

    if (!kn5.empty())
        return loadTrack(kn5);

    m_trackData.valid = true;
    m_trackLoaded = true;
    std::fprintf(stderr, "SimulationLoop: track folder %s (no kn5)\n", trackDirectory.c_str());
    return true;
}

bool SimulationLoop::loadCar(const std::string& carDir)
{
    namespace fs = std::filesystem;
    if (!fs::is_directory(carDir)) {
        std::fprintf(stderr, "SimulationLoop: car dir missing: %s\n", carDir.c_str());
        return false;
    }

#if HAS_VEHICLE_SIM
    if (m_vehicle) {
        m_vehicle->setMass(1200);
        m_vehicle->setEnginePower(260);
        m_vehicle->setMaxRpm(8500);
        m_vehicle->setDragCoeff(0.35);
        m_vehicle->setFrontalArea(2.2);
        m_vehicle->setWheelBase(2.6);
        m_vehicle->setTrackWidth(1.6);

        auto tryLoad = [&](const std::string& name, auto loader) {
            fs::path p1 = fs::path(carDir) / "data" / name;
            fs::path p2 = fs::path(carDir) / name;
            if (fs::exists(p1)) loader(p1.string());
            else if (fs::exists(p2)) loader(p2.string());
        };
        tryLoad("tyres.ini", [&](const std::string& p) { m_vehicle->loadTyresFromIni(p); });
        tryLoad("engine.ini", [&](const std::string& p) { m_vehicle->loadEngineFromIni(p); });
        tryLoad("drivetrain.ini", [&](const std::string& p) { m_vehicle->loadDrivetrainFromIni(p); });
        tryLoad("aero.ini", [&](const std::string& p) { m_vehicle->loadAeroFromIni(p); });
        tryLoad("suspension.ini", [&](const std::string& p) { m_vehicle->loadSuspensionFromIni(p); });
    }
#else
    (void)carDir;
#endif

    m_carLoaded = true;
    return true;
}

bool SimulationLoop::loadCarAudio(const std::string& carDirectory)
{
    (void)carDirectory;
    return true;
}

void SimulationLoop::start()
{
    if (m_running) return;
    m_running = true;
#if HAS_VEHICLE_SIM
    if (m_vehicle) m_vehicle->startSimulation();
#endif
    m_lastTime = std::chrono::steady_clock::now();
    m_sessionType = SESSION_RACE;
    m_sessionPhase = PHASE_COUNTDOWN;
    m_timeRemaining = 5.0;
    m_currentLap = 0;
    m_totalLaps = 5;
    if (onSimulationStarted) onSimulationStarted();
}

void SimulationLoop::stop()
{
    if (!m_running) return;
    m_running = false;
#if HAS_VEHICLE_SIM
    if (m_vehicle) m_vehicle->stopSimulation();
#endif
    if (onSimulationStopped) onSimulationStopped();
}

void SimulationLoop::reset()
{
#if HAS_VEHICLE_SIM
    if (m_vehicle) m_vehicle->reset();
#endif
    m_simAccumulator = 0;
    m_currentLap = 0;
    m_sessionPhase = PHASE_COUNTDOWN;
    m_timeRemaining = 5.0;
}

void SimulationLoop::applyInput()
{
    if (!m_input || !m_vehicle) return;
}

void SimulationLoop::updateWeather()
{
    (void)m_weather;
    (void)m_timeOfDay;
}

void SimulationLoop::ensureScenePipeline()
{
    if (m_pipelineInitialized || !m_vulkanRenderer) return;
    const char* candidates[] = {
        "shaders/passthrough.vert",
        "src/engine/Graphics/shaders/passthrough.vert",
        "../src/engine/Graphics/shaders/passthrough.vert",
    };
    std::string vertSrc, fragSrc;
    for (const char* v : candidates) {
        vertSrc = readFileText(v);
        if (!vertSrc.empty()) {
            std::string fragPath = v;
            auto pos = fragPath.rfind(".vert");
            if (pos != std::string::npos)
                fragPath.replace(pos, 5, ".frag");
            fragSrc = readFileText(fragPath);
            if (!fragSrc.empty()) break;
        }
    }
    if (vertSrc.empty() || fragSrc.empty()) {
        vertSrc =
            "#version 450\n"
            "layout(location=0) in vec3 inPos;\n"
            "layout(location=1) in vec3 inNormal;\n"
            "layout(location=2) in vec2 inUv;\n"
            "layout(location=0) out vec2 vUv;\n"
            "layout(push_constant) uniform PC { mat4 mvp; } pc;\n"
            "void main(){ vUv=inUv; gl_Position = pc.mvp * vec4(inPos,1.0); }\n";
        fragSrc =
            "#version 450\n"
            "layout(location=0) in vec2 vUv;\n"
            "layout(location=0) out vec4 outColor;\n"
            "void main(){ outColor = vec4(0.2,0.25,0.3,1.0); }\n";
    }
    (void)vertSrc;
    (void)fragSrc;
    m_pipelineInitialized = true;
}

void SimulationLoop::render()
{
    ensureScenePipeline();
    if (!m_vulkanRenderer) return;
    for (const auto& r : m_renderables) {
        (void)r;
    }
}

void SimulationLoop::broadcastLocalCarState() {}

void SimulationLoop::handleRemoteCarState(uint32_t carId, const net::CarStateData& state)
{
    (void)carId;
    (void)state;
}

void SimulationLoop::applyRemoteInput(int clientIndex, const net::InputData& input)
{
    (void)clientIndex;
    (void)input;
}

void SimulationLoop::tick()
{
    if (!m_running) {
        render();
        return;
    }

    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - m_lastTime).count();
    m_lastTime = now;
    if (elapsed > 0.05) elapsed = 0.05;

    m_simAccumulator += elapsed;

    while (m_simAccumulator >= m_physicsDt) {
        applyInput();
#if HAS_VEHICLE_SIM
        if (m_vehicle) m_vehicle->updatePhysics(m_physicsDt);
#endif
        m_simAccumulator -= m_physicsDt;
    }

#if HAS_FFB
    if (m_ffbEnabled && m_ffb && m_vehicle) {
        float torqueNm = 0.0f;
        m_ffb->updateFFB(torqueNm);
    }
#endif

    updateWeather();

    if (m_sessionPhase == PHASE_COUNTDOWN) {
        m_timeRemaining -= elapsed;
        if (m_timeRemaining <= 0.0) {
            m_sessionPhase = PHASE_GREEN_FLAG;
            m_timeRemaining = 0.0;
        }
        if (onSessionStateChanged)
            onSessionStateChanged(m_sessionType, m_sessionPhase, m_currentLap, m_totalLaps, m_timeRemaining);
    } else if (m_sessionPhase == PHASE_GREEN_FLAG) {
        if (onSessionStateChanged)
            onSessionStateChanged(m_sessionType, m_sessionPhase, m_currentLap, m_totalLaps, m_timeRemaining);
    }

#if HAS_VEHICLE_SIM
    if (m_vehicle && m_sessionPhase == PHASE_GREEN_FLAG) {
        static double lastZ = 0.0;
        double z = 0.0;
        (void)m_vehicle->getState();
        if ((lastZ <= 0.0 && z > 0.0) || (lastZ > 0.0 && z <= 0.0)) {
            if (m_currentLap < m_totalLaps) {
                m_currentLap++;
                if (onSessionStateChanged)
                    onSessionStateChanged(m_sessionType, m_sessionPhase, m_currentLap, m_totalLaps, m_timeRemaining);
            }
        }
        lastZ = z;

        Mat4f carBody = Mat4f::translation(0.f, 0.f, 0.f);
        for (auto& r : m_renderables) {
            if (r.meshName.find("car_") == 0)
                r.transform = carBody;
        }
    }
#endif

    if (m_sessionPhase == PHASE_GREEN_FLAG && m_currentLap >= m_totalLaps && m_totalLaps > 0) {
        m_sessionPhase = PHASE_CHECKERED_FLAG;
        if (onSessionStateChanged)
            onSessionStateChanged(m_sessionType, m_sessionPhase, m_currentLap, m_totalLaps, m_timeRemaining);
    }

    broadcastLocalCarState();
    render();
}

} // namespace ks::sim
