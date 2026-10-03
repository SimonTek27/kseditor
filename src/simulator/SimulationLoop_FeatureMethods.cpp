#include "SimulationLoop.h"
#include "ApplySetup.h"
#include "SetupFile.h"
#include "GarageExit.h"
#include <cctype>

namespace ks::sim {

void SimulationLoop::beginSession(GameSessionMode mode) {
    m_features.setSessionMode(mode);
    const auto& p = m_features.sessionParams;
    m_sessionType = toNetSessionType(mode);
    m_currentLap = 0;
    m_totalLaps = p.totalLaps > 0 ? p.totalLaps : (p.sessionTimeSeconds > 0 ? 0 : 5);
    m_timeRemaining = p.sessionTimeSeconds > 0 ? (double)p.sessionTimeSeconds : 0.0;
    m_sessionPhase = 1; // PHASE_COUNTDOWN

    // Practice / Qualifying: start in garage (rF2-style)
    if (p.startInGarage) {
        m_garageExit.forceEnterGarage();
        std::fprintf(stderr, "SimulationLoop: session %s starts IN GARAGE\n",
                     sessionModeName(mode));
    } else {
        m_garageExit.forceOnTrack();
    }
}

void SimulationLoop::startFeatureServices(bool hostAnnounce) {
    m_features.startServices(hostAnnounce);
    m_features.onBeginSession = [this](GameSessionMode mode, const SessionStartParams&) {
        beginSession(mode);
    };
    m_features.onSetFlag = [this](uint8_t f) {
        RaceFlag rf = RaceFlag::Green;
        if (f == 2) rf = RaceFlag::Yellow;
        else if (f >= 4) rf = RaceFlag::Checkered;
        setRaceFlag(rf);
    };
    m_features.onPenalty = [this](int car, int kind, float value, const std::string& reason) {
        using PT = Penalty::Type;
        PT ty = PT::TimeAdded;
        if (kind == 1) ty = PT::DriveThrough;
        else if (kind == 2) ty = PT::StopGo;
        m_raceSession.addPenalty(car, ty, value, reason);
    };
    m_features.onSetTimeOfDay = [this](float h) { setTimeOfDay(h); };
    m_features.onSetWeather = [this](const std::string& name) {
        auto wp = presetByName(name);
        ks::physics::WeatherState ws;
        ws.ambientTemp = wp.ambientC;
        ws.trackTemp = wp.trackC;
        ws.trackWetness = wp.wetness;
        ws.rainIntensity = wp.rain;
        setWeatherPreset(ws);
        m_features.weatherCtrl.applyPreset(name);
    };
    m_features.onSetupLoad = [this](const std::string& path) {
        if (!m_setupGarage) return;
        SetupData s = m_setupGarage->setup();
        loadSetupFromFile(s, path);
        m_setupGarage->setSetup(s);
    };
    m_features.onSetupSave = [this](const std::string& path) {
        if (!m_setupGarage) return;
        saveSetupToFile(m_setupGarage->setup(), path);
    };
}

bool SimulationLoop::loadReplayFile(const std::string& path) {
    return m_features.loadReplay(path);
}

void SimulationLoop::updateGarageExit(float dt) {
#if HAS_VEHICLE_SIM
    if (!m_vehicle) return;
    const auto st = m_vehicle->getState();
    GarageExitInput in;
    in.engineRunning = st.rpm > 200.0;
    in.ignitionOn = true;
    in.speedMs = static_cast<float>(st.speed);
    in.throttle = static_cast<float>(st.throttle);
    in.brake = static_cast<float>(st.brake);
    in.steer = static_cast<float>(st.steering);
    in.posX = static_cast<float>(st.position.x);
    in.posY = static_cast<float>(st.position.y);
    in.posZ = static_cast<float>(st.position.z);
    in.heading = static_cast<float>(st.heading);
    in.pitLaneOpen = true;
    in.pathBlocked = false;
    if (m_garageExit.phase() == GarageExitPhase::InGarage && st.throttle > 0.2)
        in.requestLeave = true;

    GarageExitOutput out = m_garageExit.update(dt, in);
    if (out.holdControls) {
        m_vehicle->setThrottle(0);
        m_vehicle->setBrake(out.snapToBox ? 1.0 : st.brake);
        if (out.snapToBox)
            m_vehicle->setSteering(0);
    }
    if (out.pitLimiterActive && st.speed > out.pitLimiterMaxMs) {
        m_vehicle->setThrottle(0);
        m_vehicle->setBrake(0.4);
    }
#else
    (void)dt;
#endif
}

} // namespace ks::sim
