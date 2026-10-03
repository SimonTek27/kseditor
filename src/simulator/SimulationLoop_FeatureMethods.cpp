#include "SimulationLoop.h"
#include "ApplySetup.h"
#include "SetupFile.h"
#include "GarageExit.h"
#include "GarageSpawn.h"
#include "PitLaneQueue.h"
#include "PitLaneCollision.h"
#include "PitLaneRepair.h"
#include "MultiCarManager.h"
#include <cctype>
#include <cstdio>
#include <algorithm>

namespace ks::sim {

void SimulationLoop::setupDefaultGarageLayout(int boxCount) {
    WorldPose first;
    first.x = 0.f; first.y = 0.f; first.z = 0.f; first.heading = 0.f;
    m_garageLayout = GarageSpawnPolicy::makeLinearRow(boxCount, first, 6.f, 0.f);
    configurePitAxis(first.x, first.z, m_garageLayout.pitLaneHeading, 120.f);
    if (!m_garageLayout.boxes.empty()) {
        m_garageLayout.boxes[0].occupied = true;
        m_garageExit.bindBox(0, m_garageLayout.boxes[0].pose, m_garageLayout.pitLaneHeading);
    }
    std::fprintf(stderr, "SimulationLoop: garage layout %d boxes\n", boxCount);
}

void SimulationLoop::beginSession(GameSessionMode mode) {
    m_features.setSessionMode(mode);
    const auto& p = m_features.sessionParams;
    m_sessionType = toNetSessionType(mode);
    m_currentLap = 0;
    m_totalLaps = p.totalLaps > 0 ? p.totalLaps : (p.sessionTimeSeconds > 0 ? 0 : 5);
    m_timeRemaining = p.sessionTimeSeconds > 0 ? (double)p.sessionTimeSeconds : 0.0;
    m_sessionPhase = 1;

    m_pitQueue = PitLaneQueue{};
    m_pitCollision = PitLaneCollision{};
    m_pitRepair = PitLaneRepair{};
    m_pitSystemsReady = true;
    m_requestPitService = false;

    setupDefaultGarageLayout(std::max(8, p.aiCars + 1));
    if (p.aiCars > 0)
        spawnAiGrid(p.aiCars);

    if (p.startInGarage) {
        m_garageExit.forceEnterGarage();
        std::fprintf(stderr, "SimulationLoop: session %s starts IN GARAGE\n", sessionModeName(mode));
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
        if (kind == 2) m_requestPitService = true;
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
}

void SimulationLoop::updatePitLane(float dt) {
    if (!m_pitSystemsReady) m_pitSystemsReady = true;
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
    body.x = x; body.z = z; body.heading = heading;
    body.vx = static_cast<float>(st.velocity.x);
    body.vz = static_cast<float>(st.velocity.z);
    body.active = true;
    body.invulnerable = (m_garageExit.phase() == GarageExitPhase::InGarage ||
                         m_garageExit.phase() == GarageExitPhase::Preparing ||
                         m_garageExit.phase() == GarageExitPhase::EngineStart);
    m_pitCollision.upsert(body);

    if (m_multiCar) {
        for (const auto& ce : m_multiCar->cars()) {
            if (!ce || !ce->isActive || ce->isPlayer || !ce->vehicle) continue;
            const auto ost = ce->vehicle->getState();
            PitCarBody ob;
            ob.carId = ce->id;
            ob.x = static_cast<float>(ost.position.x);
            ob.z = static_cast<float>(ost.position.z);
            ob.heading = static_cast<float>(ost.heading);
            ob.vx = static_cast<float>(ost.velocity.x);
            ob.vz = static_cast<float>(ost.velocity.z);
            ob.active = true;
            m_pitCollision.upsert(ob);
            m_pitQueue.updateCar(ce->id, ob.x, ob.z, static_cast<float>(ost.speed));
        }
    }

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
            m_vehicle->setBrake(maxMs < 0.5f ? 0.6 : 0.25);
        }
    }
    if (m_pitCollision.damageImpulseFor(kPlayerCarId) > 0.f)
        m_raceSession.addPenalty(kPlayerCarId, Penalty::Type::TimeAdded, 0.f, "pit contact");
#else
    (void)dt;
    m_pitQueue.update(dt);
#endif
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

    const bool queueBlock = m_pitQueue.shouldBlockGarageExit(kPlayerCarId);
    const bool contactBlock = m_pitCollision.isInContact(kPlayerCarId) &&
        (m_garageExit.phase() == GarageExitPhase::BoxClear ||
         m_garageExit.phase() == GarageExitPhase::RollingOut);
    const bool serviceBlock = m_pitRepair.isBusy();
    in.pathBlocked = queueBlock || contactBlock || serviceBlock;

    if (m_garageExit.phase() == GarageExitPhase::InGarage && st.throttle > 0.2 && !serviceBlock)
        in.requestLeave = true;

    GarageExitOutput out = m_garageExit.update(dt, in);
    if (out.holdControls || serviceBlock) {
        m_vehicle->setThrottle(0);
        m_vehicle->setBrake(out.snapToBox || serviceBlock ? 1.0 : st.brake);
        if (out.snapToBox || serviceBlock) m_vehicle->setSteering(0);
    }
    if (out.pitLimiterActive && st.speed > out.pitLimiterMaxMs) {
        m_vehicle->setThrottle(0);
        m_vehicle->setBrake(0.4);
    }
#else
    (void)dt;
#endif
}

void SimulationLoop::updatePitRepair(float dt) {
#if HAS_VEHICLE_SIM
    if (!m_vehicle) return;
    applyDamageEffects();
    const auto st = m_vehicle->getState();
    const auto phase = m_garageExit.phase();

    PitRepairInput in;
    in.inGarageBox = (phase == GarageExitPhase::InGarage || phase == GarageExitPhase::Returning);
    in.speedMs = static_cast<float>(st.speed);
    in.fuelL = static_cast<float>(st.fuel);
    in.fuelCapacityL = 100.f;
    in.targetFuelL = 100.f;
    in.wantTyres = true;
    in.wantBody = true;
    in.wantSuspension = true;
    in.wantAero = true;
    in.requestService = m_requestPitService && in.inGarageBox;

    PitRepairOutput out = m_pitRepair.update(dt, in, m_damage, phase);
    if (out.holdCar) {
        m_vehicle->setThrottle(0);
        m_vehicle->setBrake(1.0);
        m_vehicle->setSteering(0);
    }
    if (out.completedThisFrame) {
        m_requestPitService = false;
        m_vehicle->damage().repairPartial(1.f);
        std::fprintf(stderr, "SimulationLoop: pit service COMPLETE (%.0f%%)\n",
                     out.overallProgress * 100.f);
    }
#else
    (void)dt;
#endif
}

void SimulationLoop::spawnAiGrid(int count) {
    if (count <= 0) return;
    if (!m_multiCar)
        m_multiCar = std::make_unique<MultiCarManager>();

    std::vector<int> toRemove;
    for (const auto& ce : m_multiCar->cars()) {
        if (ce && !ce->isPlayer) toRemove.push_back(ce->id);
    }
    for (int id : toRemove)
        m_multiCar->removeCar(id);

    const int boxes = static_cast<int>(m_garageLayout.boxes.size());
    for (int i = 0; i < count; ++i) {
        const int boxIdx = (i + 1) % std::max(1, boxes);
        WorldPose pose;
        if (boxes > 0) {
            pose = m_garageLayout.boxes[boxIdx].pose;
            m_garageLayout.boxes[boxIdx].occupied = true;
        } else {
            pose.x = 6.f * float(i + 1);
            pose.z = 0.f;
            pose.heading = m_garageLayout.pitLaneHeading;
        }
        char name[32];
        std::snprintf(name, sizeof(name), "AI_%02d", i + 1);
        const int id = m_multiCar->addCar("ai_car", name,
            vec3{pose.x, pose.y, pose.z}, false);
        if (auto* ce = m_multiCar->getCar(id)) {
            ce->transform = mat4();
            ce->transform(0, 3) = pose.x;
            ce->transform(1, 3) = pose.y;
            ce->transform(2, 3) = pose.z;
        }
        std::fprintf(stderr, "SimulationLoop: AI %s -> garage box %d (%.1f,%.1f)\n",
                     name, boxIdx, pose.x, pose.z);
    }
    m_aiCarCount = count;
}

void SimulationLoop::applyDamageEffects() {
#if HAS_VEHICLE_SIM
    if (!m_vehicle) return;
    auto& dmg = m_vehicle->damage();
    const float pm = dmg.powerMultiplier();
    m_vehicle->setEnginePower(260.0 * std::max(0.15, (double)pm));
#endif
}

} // namespace ks::sim
