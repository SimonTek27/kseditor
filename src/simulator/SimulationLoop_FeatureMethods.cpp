#include "SimulationLoop.h"
#include "ApplySetup.h"
#include "SetupFile.h"
#include "GarageExit.h"
#include "PitLaneQueue.h"
#include "PitLaneCollision.h"
#include <cctype>
#include <cstdio>

namespace ks::sim {

void SimulationLoop::beginSession(GameSessionMode mode) {
    m_features.setSessionMode(mode);
    const auto& p = m_features.sessionParams;
    m_sessionType = toNetSessionType(mode);
    m_currentLap = 0;
    m_totalLaps = p.totalLaps > 0 ? p.totalLaps : (p.sessionTimeSeconds > 0 ? 0 : 5);
    m_timeRemaining = p.sessionTimeSeconds > 0 ? (double)p.sessionTimeSeconds : 0.0;
    m_sessionPhase = 1; // PHASE_COUNTDOWN

    m_pitQueue = PitLaneQueue{};
    m_pitCollision = PitLaneCollision{};
    m_pitSystemsReady = true;

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

void SimulationLoop::configurePitAxis(float originX, float originZ, float headingRad, float lengthM) {
    PitAxis axis;
    axis.originX = originX;
    axis.originZ = originZ;
    axis.heading = headingRad;
    m_pitQueue.setAxis(axis);
    m_pitCollision.setAxis(axis);
    PitLaneQueueConfig qc = m_pitQueue.config();
    qc.pitEndAlong = lengthM > 1.f ? lengthM : 120.f;
    m_pitQueue.setConfig(qc);
    std::fprintf(stderr, "SimulationLoop: pit axis origin=(%.1f,%.1f) hdg=%.2f len=%.0f\n",
                 originX, originZ, headingRad, qc.pitEndAlong);
}

void SimulationLoop::updatePitLane(float dt) {
    if (!m_pitSystemsReady)
        m_pitSystemsReady = true;
    m_pitQueue.setSimTime(static_cast<float>(m_simTime));

#if HAS_VEHICLE_SIM
    if (!m_vehicle) {
        m_pitQueue.update(dt);
        return;
    }
    const auto st = m_vehicle->getState();
    const float x = static_cast<float>(st.position.x);
    const float z = static_cast<float>(st.position.z);
    const float speed = static_cast<float>(st.speed);
    const float heading = static_cast<float>(st.heading);

    PitCarBody body;
    body.carId = kPlayerCarId;
    body.x = x;
    body.z = z;
    body.heading = heading;
    body.vx = static_cast<float>(st.velocity.x);
    body.vz = static_cast<float>(st.velocity.z);
    body.active = true;
    body.invulnerable = (m_garageExit.phase() == GarageExitPhase::InGarage ||
                         m_garageExit.phase() == GarageExitPhase::Preparing ||
                         m_garageExit.phase() == GarageExitPhase::EngineStart);
    m_pitCollision.upsert(body);

    const auto phase = m_garageExit.phase();
    if (phase == GarageExitPhase::Preparing || phase == GarageExitPhase::BoxClear ||
        phase == GarageExitPhase::RollingOut || phase == GarageExitPhase::PitLane) {
        m_pitQueue.requestLeave(kPlayerCarId, 1, m_garageExit.garageIndex(), true, x, z);
        m_pitQueue.updateCar(kPlayerCarId, x, z, speed);
        if (m_pitQueue.isCleared(kPlayerCarId) && phase == GarageExitPhase::RollingOut)
            m_pitQueue.markMoving(kPlayerCarId);
    }
    if (phase == GarageExitPhase::OnTrack)
        m_pitQueue.leaveQueue(kPlayerCarId);
    if (phase == GarageExitPhase::Returning) {
        m_pitQueue.requestEnter(kPlayerCarId, 1, m_garageExit.garageIndex(), true, x, z);
        m_pitQueue.updateCar(kPlayerCarId, x, z, speed);
    }

    m_pitQueue.update(dt);
    m_pitCollision.step(dt);
    m_pitCollision.applyToQueue(m_pitQueue);

    const float maxMs = m_pitQueue.suggestedMaxSpeedMs(kPlayerCarId);
    if (phase == GarageExitPhase::PitLane || phase == GarageExitPhase::RollingOut) {
        if (speed > maxMs && maxMs >= 0.f) {
            m_vehicle->setThrottle(0);
            if (maxMs < 0.5f)
                m_vehicle->setBrake(0.6);
            else
                m_vehicle->setBrake(0.25);
        }
    }

    const float dmg = m_pitCollision.damageImpulseFor(kPlayerCarId);
    if (dmg > 0.f) {
        m_raceSession.addPenalty(kPlayerCarId, Penalty::Type::TimeAdded, 0.f, "pit contact");
    }
#else
    (void)dt;
    m_pitQueue.update(dt);
#endif
}

void SimulationLoop::updateGarageExit(float dt) {
#if HAS_VEHICLE_SIM
    if (!m_vehicle) return;
    const auto st = m_vehicle->getState();
    const float x = static_cast<float>(st.position.x);
    const float z = static_cast<float>(st.position.z);

    GarageExitInput in;
    in.engineRunning = st.rpm > 200.0;
    in.ignitionOn = true;
    in.speedMs = static_cast<float>(st.speed);
    in.throttle = static_cast<float>(st.throttle);
    in.brake = static_cast<float>(st.brake);
    in.steer = static_cast<float>(st.steering);
    in.posX = x;
    in.posY = static_cast<float>(st.position.y);
    in.posZ = z;
    in.heading = static_cast<float>(st.heading);
    in.pitLaneOpen = true;

    const bool queueBlock = m_pitQueue.shouldBlockGarageExit(kPlayerCarId);
    const bool contactBlock = m_pitCollision.isInContact(kPlayerCarId) &&
        (m_garageExit.phase() == GarageExitPhase::BoxClear ||
         m_garageExit.phase() == GarageExitPhase::RollingOut);
    in.pathBlocked = queueBlock || contactBlock;

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
