#include "MultiCarManager.h"
#include <algorithm>
#include <cmath>

namespace ks::sim {

MultiCarManager::MultiCarManager() = default;
MultiCarManager::~MultiCarManager() = default;

int MultiCarManager::addCar(const std::string& carName, const std::string& driverName,
                             const vec3& startPosition, bool isPlayer)
{
    auto entry = std::make_unique<CarEntry>();
    entry->id = m_nextCarId++;
    entry->carName = carName;
    entry->driverName = driverName;
    entry->isPlayer = isPlayer;
    entry->vehicle = std::make_unique<ks::physics::VehicleSimulator>();
    entry->vehicle->setMass(1200);
    entry->vehicle->setEnginePower(260);
    entry->vehicle->setMaxRpm(8500);
    entry->vehicle->setDragCoeff(0.35);
    entry->vehicle->setFrontalArea(2.2);
    entry->vehicle->setWheelBase(2.6);
    entry->vehicle->setTrackWidth(1.6);

    if (!isPlayer) {
        entry->ai = std::make_unique<AIController>();
        if (!m_aiSplinePath.empty()) {
            entry->ai->loadSpline(m_aiSplinePath);
        }
    }

    entry->transform = mat4();

    int id = entry->id;
    if (isPlayer) {
        m_playerCarId = id;
    }

    m_cars.push_back(std::move(entry));
    if (onCarAdded) onCarAdded(id, carName);
    return id;
}

void MultiCarManager::removeCar(int carId)
{
    for (auto it = m_cars.begin(); it != m_cars.end(); ++it) {
        if ((*it)->id == carId) {
            m_cars.erase(it);
            if (onCarRemoved) onCarRemoved(carId);
            return;
        }
    }
}

void MultiCarManager::clearAllCars()
{
    m_cars.clear();
    m_playerCarId = -1;
}

void MultiCarManager::update(float dt)
{
    for (auto& car : m_cars) {
        if (!car->isActive) continue;

        if (!car->isPlayer && car->ai && car->ai->isReady()) {
            auto state = car->vehicle->getState();
            car->ai->update(vec3(state.position.x(), state.position.y(), state.position.z()), 0, state.speed, state.gear, dt);
            car->vehicle->setThrottle(car->ai->throttle());
            car->vehicle->setBrake(car->ai->brake());
            car->vehicle->setSteering(car->ai->steering());
        }

        car->vehicle->updatePhysics(dt);

        auto state = car->vehicle->getState();
        // Build transform matrix from simulation state
        car->transform = mat4();
        // Translate
        car->transform(0,3) = state.position.x();
        car->transform(1,3) = state.position.y();
        car->transform(2,3) = state.position.z();
    }

    if (m_collisionEnabled) {
        checkCollisions();
    }
}

CarEntry* MultiCarManager::getCar(int id)
{
    for (auto& car : m_cars) {
        if (car->id == id) return car.get();
    }
    return nullptr;
}

CarEntry* MultiCarManager::getCarByClientIndex(int clientIndex)
{
    for (auto& car : m_cars) {
        if (car->clientIndex == clientIndex) return car.get();
    }
    return nullptr;
}

CarEntry* MultiCarManager::getCarById(uint32_t carId)
{
    for (auto& car : m_cars) {
        if (static_cast<uint32_t>(car->id) == carId) return car.get();
    }
    return nullptr;
}

CarEntry* MultiCarManager::playerCar()
{
    return getCar(m_playerCarId);
}

void MultiCarManager::loadAiSpline(const std::string& trackDirectory)
{
    m_aiSplinePath = trackDirectory;
    for (auto& car : m_cars) {
        if (!car->isPlayer && car->ai) {
            car->ai->loadSpline(trackDirectory);
        }
    }
}

void MultiCarManager::checkCollisions()
{
    for (size_t i = 0; i < m_cars.size(); ++i) {
        for (size_t j = i + 1; j < m_cars.size(); ++j) {
            if (!m_cars[i]->isActive || !m_cars[j]->isActive) continue;

            auto stateA = m_cars[i]->vehicle->getState();
            auto stateB = m_cars[j]->vehicle->getState();

            float dx = stateA.position.x() - stateB.position.x();
            float dy = stateA.position.y() - stateB.position.y();
            float dz = stateA.position.z() - stateB.position.z();
            float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

            if (dist < 2.0f && dist > 0.01f) {
                if (onCollisionOccurred) onCollisionOccurred(m_cars[i]->id, m_cars[j]->id);
            }
        }
    }
}

} // namespace ks::sim
