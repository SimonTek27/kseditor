/**
 * SimulationLoop.cpp — std-only
 * HAS_VEHICLE_SIM=1 → VehicleSimulator; HAS_FFB=1 → FFBBridge + FFBSDKFactory
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
#define HAS_VEHICLE_SIM 1
#endif
#ifndef HAS_KSNET
#define HAS_KSNET 0
#endif
#ifndef HAS_FFB
#define HAS_FFB 1
#endif

#if HAS_VEHICLE_SIM
#include "engine/physics/VehicleSimulator.h"
#endif
#if HAS_FFB
#include "engine/devices/FFBBridge.h"
#include "engine/devices/simracing/FFBSDKFactory.h"
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
#if HAS_FFB
    if (m_ffbEnabled && !m_ffb) {
        m_ffb = ks::device::FFBSDKFactory::createFFB();
        if (m_ffb)
            std::fprintf(stderr, "SimulationLoop: FFB device ready\n");
        else
            std::fprintf(stderr, "SimulationLoop: no FFB wheel (software torque only)\n");
    }
#endif
    return true;
}

bool SimulationLoop::loadTrack(const std::string& kn5Path)
{
    m_renderables.clear();
    m_trackData = SimTrackData{};
    m_trackData.kn5Path = kn5Path;
    m_trackData.name = std::filesystem::path(kn5Path).stem().string();
    if (!std::filesystem::exists(kn5Path)) {
        std::fprintf(stderr, "SimulationLoop: track not found: %s\n", kn5Path.c_str());
        m_trackLoaded = false;
        return false;
    }
    m_trackData.valid = true;
    m_trackLoaded = true;
    return true;
}

bool SimulationLoop::loadTrackFolder(const std::string& trackDirectory)
{
    namespace fs = std::filesystem;
    if (!fs::is_directory(trackDirectory)) return false;
    m_trackData.directory = trackDirectory;
    m_trackData.name = fs::path(trackDirectory).filename().string();
    std::string kn5;
    for (auto& e : fs::directory_iterator(trackDirectory)) {
        if (e.path().extension() == ".kn5") { kn5 = e.path().string(); break; }
    }
    if (kn5.empty()) {
        try {
            for (auto& e : fs::recursive_directory_iterator(trackDirectory)) {
                if (e.path().extension() == ".kn5") { kn5 = e.path().string(); break; }
            }
        } catch (...) {}
    }
    if (!kn5.empty()) return loadTrack(kn5);
    m_trackData.valid = true;
    m_trackLoaded = true;
    return true;
}

bool SimulationLoop::loadCar(const std::string& carDir)
{
    namespace fs = std::filesystem;
    if (!fs::is_directory(carDir)) return false;
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
    m_pipelineInitialized = true;
}

void SimulationLoop::render()
{
    ensureScenePipeline();
}

void SimulationLoop::broadcastLocalCarState() {}
void SimulationLoop::handleRemoteCarState(uint32_t, const net::CarStateData&) {}
void SimulationLoop::applyRemoteInput(int, const net::InputData&) {}

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
    if (m_ffbEnabled && m_vehicle) {
        const auto& samp = m_vehicle->ffbSample();
        ks::device::FFBInputs in;
        in.slipAngleFL = samp.slipAngleFL;
        in.slipAngleFR = samp.slipAngleFR;
        in.loadFL = samp.loadFL;
        in.loadFR = samp.loadFR;
        in.camberFL = samp.camberFL;
        in.camberFR = samp.camberFR;
        in.speedMs = samp.speedMs;
        in.steerAngle = samp.steerAngle;
        float torqueNm = ks::device::FFBBridge::computeSteeringTorque(
            in, &m_vehicle->tires(), &m_vehicle->tires());
        if (m_ffb)
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
        auto st = m_vehicle->getState();
        static double lastZ = 0.0;
        double z = st.position.z;
        if ((lastZ <= 0.0 && z > 0.0) || (lastZ > 0.0 && z <= 0.0)) {
            if (m_currentLap < m_totalLaps) {
                m_currentLap++;
                if (onSessionStateChanged)
                    onSessionStateChanged(m_sessionType, m_sessionPhase, m_currentLap, m_totalLaps, m_timeRemaining);
            }
        }
        lastZ = z;
        Mat4f carBody = Mat4f::translation(st.position.x, st.position.y, st.position.z);
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
