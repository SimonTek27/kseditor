#pragma once
// SimulationLoop - the other orchestrator class that was forward-declared in
// Network/NetworkManager.h (`class SimulationLoop;`) and actively called
// (`m_simLoop->inputManager()->throttle()`, `m_simLoop->applyRemoteInput(...)`)
// but, like MultiCarManager, never defined anywhere in the codebase.
//
// Owns the player's VehicleSimulator, the input/FFB device manager, the
// replay recorder, and the multi-car manager (AI + remote). Ticking this
// class once per fixed physics step is what actually connects:
//   physics -> ForceFeedback (via steeringTorqueNm() / updateFFBFromPhysics)
//   physics -> ReplayRecorder
//   physics -> TrackSurface::evolve() (rain/rubber evolution over time)
//   AI cars  -> MultiCarManager::update()

#include <QObject>
#include <map>
#include "../Physics/VehiclePhysics.h"
#include "../Physics/ReplaySystem.h"
#include "MultiCarManager.h"
#include "../Physics/TrackSurface.h"
#include "../Devices/simracing/SimRacingDevices.h"
#include "../Network/NetworkConfig.h"

namespace ks::sim {

class SimulationLoop : public QObject {
    Q_OBJECT
public:
    explicit SimulationLoop(QObject* parent = nullptr) : QObject(parent) {
        m_playerCar = std::make_unique<ks::physics::VehicleSimulator>();
        m_input = std::make_unique<ks::device::RacingInputManager>(this);
        m_multiCar = std::make_unique<MultiCarManager>(this);
    }

    void start(double rainIntensity = 0.0) {
        m_input->initialize();
        m_playerCar->startSimulation();
        m_replay.start(60.0);
        m_rainIntensity = rainIntensity;
    }

    void stop() {
        m_playerCar->stopSimulation();
        m_replay.stop();
    }

    // Advances the whole simulated world by one fixed physics step: reads
    // local input into the player car, steps physics, feeds the result back
    // out to force feedback and the replay recorder, steps AI cars, and
    // evolves the shared track surface (rubber build-up / rain wetting).
    void tick(double dt) {
        m_input->update(dt);
        m_playerCar->setThrottle(m_input->getAxis(ks::device::AxisType::Throttle));
        m_playerCar->setBrake(m_input->getAxis(ks::device::AxisType::Brake));
        m_playerCar->setSteering(m_input->getAxis(ks::device::AxisType::SteeringWheel));
        m_playerCar->updatePhysics(dt);

        m_input->updateFFBFromPhysics(0.0f);

        auto state = m_playerCar->getState();
        ks::physics::ReplayFrame frame;
        frame.time = m_elapsed;
        frame.pos = state.position;
        frame.rot = state.rotation;
        frame.vel = state.velocity;
        frame.speed = state.speed;
        frame.rpm = state.rpm;
        frame.gear = state.gear;
        frame.throttle = state.throttle;
        frame.brake = state.brake;
        frame.steer = state.steering;
        frame.carId = 0; // player
        m_replay.push(frame, dt);

        m_multiCar->update(dt);

        ks::engine::physics::TrackSurface::instance().evolve(dt, m_rainIntensity);

        m_elapsed += dt;
    }

    // Applies input received over the network for a client-controlled car
    // (called from NetworkServer::processMessages() -> MSG_PLAYER_INPUT).
    // clientIndex doubles as carId here since spawnCarForClient() assigns the
    // id returned by MultiCarManager::addCar() as the client's carId.
    void applyRemoteInput(int clientIndex, const net::InputData& input) {
        m_multiCar->applyRemoteInput(static_cast<uint32_t>(clientIndex),
                                      input.throttle, input.brake, input.steering);
    }

    ks::device::RacingInputManager* inputManager() { return m_input.get(); }
    ks::physics::VehicleSimulator* playerCar() { return m_playerCar.get(); }
    MultiCarManager* multiCarManager() { return m_multiCar.get(); }
    ks::physics::ReplayRecorder& replay() { return m_replay; }
    void setRainIntensity(double v) { m_rainIntensity = v; }

private:
    std::unique_ptr<ks::physics::VehicleSimulator> m_playerCar;
    std::unique_ptr<ks::device::RacingInputManager> m_input;
    std::unique_ptr<MultiCarManager> m_multiCar;
    ks::physics::ReplayRecorder m_replay;
    double m_elapsed = 0.0;
    double m_rainIntensity = 0.0;
};

} // namespace ks::sim
