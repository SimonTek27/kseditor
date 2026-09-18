#pragma once

#include "MathTypes.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "engine/physics/VehiclePhysics.h"
#include "AIController.h"

namespace ks::sim {

struct CarEntry {
    int id = 0;
    int clientIndex = -1;
    std::string carName;
    std::string driverName;
    std::unique_ptr<ks::physics::VehicleSimulator> vehicle;
    std::unique_ptr<AIController> ai;
    bool isPlayer = false;
    bool isActive = true;

    mat4 transform;
    vec3 color{1, 1, 1};
};

class MultiCarManager {
public:
    MultiCarManager();
    ~MultiCarManager();

    int addCar(const std::string& carName, const std::string& driverName,
               const vec3& startPosition, bool isPlayer = false);
    void removeCar(int carId);
    void clearAllCars();

    void update(float dt);

    int carCount() const { return static_cast<int>(m_cars.size()); }
    CarEntry* getCar(int id);
    CarEntry* getCarByClientIndex(int clientIndex);
    CarEntry* getCarById(uint32_t carId);
    const std::vector<std::unique_ptr<CarEntry>>& cars() const { return m_cars; }

    CarEntry* playerCar();
    int playerCarId() const { return m_playerCarId; }

    void loadAiSpline(const std::string& trackDirectory);

    void setPlayerCarId(int id) { m_playerCarId = id; }
    void setCollisionEnabled(bool e) { m_collisionEnabled = e; }

    std::function<void(int, const std::string&)> onCarAdded;
    std::function<void(int)> onCarRemoved;
    std::function<void(int, int)> onCollisionOccurred;

private:
    void updateCarTransforms();
    void checkCollisions();
    void resolveCollision(CarEntry& a, CarEntry& b);

    std::vector<std::unique_ptr<CarEntry>> m_cars;
    int m_nextCarId = 0;
    int m_playerCarId = -1;
    bool m_collisionEnabled = true;
    std::string m_aiSplinePath;
};

} // namespace ks::sim
