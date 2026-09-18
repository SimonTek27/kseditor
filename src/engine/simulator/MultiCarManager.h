#pragma once
// MultiCarManager - owns every car the local process is responsible for
// simulating: AI-driven cars (path-following via AIDriver) and, on a server,
// player cars driven by remote input (MSG_PLAYER_INPUT -> applyRemoteInput).
//
// This is one of the two orchestrator classes that were forward-declared in
// Network/NetworkManager.h (`class MultiCarManager;`) and actively called
// (`m_multiCar->addCar(...)`) but never defined anywhere in the codebase.
//
// Cars belonging to OTHER network peers that this process does NOT simulate
// (i.e. what a client renders for every other player) are not tracked here:
// that is exactly what Network/NetRace.h's SnapshotInterp is for, fed
// directly from NetworkClient::carStateReceived.

#include <QObject>
#include <QVector>
#include <QVector3D>
#include <QString>
#include <memory>
#include <map>
#include "../Physics/VehiclePhysics.h"
#include "../Physics/AIDriver.h"

namespace ks::sim {

struct CarEntry {
    std::unique_ptr<ks::physics::VehicleSimulator> sim;
    ks::ai::AIDriver driver; // only used when isAI is true
    QString carName;
    QString driverName;
    bool isAI = false;
};

class MultiCarManager : public QObject {
    Q_OBJECT
public:
    explicit MultiCarManager(QObject* parent = nullptr) : QObject(parent) {}

    // Entry point used by NetworkServer::spawnCarForClient(): every car this
    // process is responsible for (AI or a connected player) gets a real,
    // locally-stepped VehicleSimulator so its state can be broadcast.
    uint32_t addCar(const QString& carName, const QString& driverName,
                     const QVector3D& spawnPos, bool isAI) {
        uint32_t id = m_nextCarId++;
        CarEntry entry;
        entry.sim = std::make_unique<ks::physics::VehicleSimulator>();
        entry.carName = carName;
        entry.driverName = driverName;
        entry.isAI = isAI;
        entry.sim->startSimulation();
        Q_UNUSED(spawnPos); // starting position is applied via the simulator's own state once available
        m_cars.erase(id);
        m_cars.emplace(id, std::move(entry));
        emit carSpawned(id);
        return id;
    }

    // Spawns a fully AI-driven car following the given path (from AiSpline data).
    uint32_t spawnAICar(const QString& carName, const QString& driverName,
                         const QVector<ks::ai::AITarget>& path,
                         int aggression = 2, float skill = 0.85f) {
        uint32_t id = addCar(carName, driverName, QVector3D(), true);
        auto it = m_cars.find(id);
        it->second.driver.aggression = aggression;
        it->second.driver.skill = skill;
        it->second.driver.setPath(path);
        return id;
    }

    void removeCar(uint32_t carId) {
        m_cars.erase(carId);
        emit carRemoved(carId);
    }

    // Applies input received over the network for a client-controlled car
    // (called from NetworkServer::processMessages() -> MSG_PLAYER_INPUT).
    void applyRemoteInput(uint32_t carId, float throttle, float brake, float steering) {
        auto it = m_cars.find(carId);
        if (it == m_cars.end()) return;
        it->second.sim->setThrottle(throttle);
        it->second.sim->setBrake(brake);
        it->second.sim->setSteering(steering);
    }

    // Steps every locally-owned car by dt. AI cars get their control inputs
    // from AIDriver each tick; player-input cars already had setThrottle/
    // setBrake/setSteering applied via applyRemoteInput() this frame.
    void update(double dt) {
        QVector<QVector3D> positions = allCarPositions();
        for (auto& kv : m_cars) {
            CarEntry& entry = kv.second;
            if (entry.isAI && entry.driver.hasPath()) {
                auto state = entry.sim->getState();
                ks::ai::AIInputs in;
                in.pos = state.position;
                in.vel = state.velocity;
                in.heading = state.heading;
                in.speedMs = state.speed;
                in.wetness = static_cast<float>(entry.sim->getTrackGripReduction());
                in.damaged = entry.sim->damageState().bodyDamage > 0.3f ||
                             entry.sim->damageState().engineDamage > 0.3f;

                entry.driver.setObstacles(positions);
                auto out = entry.driver.update(in, static_cast<float>(dt));

                entry.sim->setThrottle(out.throttle);
                entry.sim->setBrake(out.brake);
                entry.sim->setSteering(out.steer);
            }
            entry.sim->updatePhysics(dt);
        }
    }

    // Every locally-owned car's world position - used for draft calculations
    // and AI obstacle avoidance.
    QVector<QVector3D> allCarPositions() const {
        QVector<QVector3D> out;
        for (const auto& kv : m_cars) out.push_back(kv.second.sim->getState().position);
        return out;
    }

    int carCount() const { return static_cast<int>(m_cars.size()); }
    ks::physics::VehicleSimulator* car(uint32_t id) {
        auto it = m_cars.find(id);
        return it != m_cars.end() ? it->second.sim.get() : nullptr;
    }
    // Kept for API symmetry with earlier drafts of this class.
    ks::physics::VehicleSimulator* aiCar(uint32_t id) { return car(id); }

signals:
    void carSpawned(uint32_t carId);
    void carRemoved(uint32_t carId);

private:
    std::map<uint32_t, CarEntry> m_cars;
    uint32_t m_nextCarId = 1;
};

} // namespace ks::sim
