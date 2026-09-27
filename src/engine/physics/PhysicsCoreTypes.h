#pragma once

/**
 * @file PhysicsCoreTypes.h
 * @brief Centralized core type definitions for all physics modules
 * @copyright KS Physics Engine
 *
 * This file contains all fundamental physics types that are shared across
 * multiple modules. It should be the single source of truth for these types.
 */

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace ks {
namespace physics {

struct PhysVec3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;

    PhysVec3() = default;
    PhysVec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    float length() const { return std::sqrt(x * x + y * y + z * z); }
    float lengthSquared() const { return x * x + y * y + z * z; }

    PhysVec3 normalized() const {
        const float len = length();
        if (len < 1e-8f) return PhysVec3{};
        return PhysVec3{x / len, y / len, z / len};
    }

    PhysVec3 operator+(const PhysVec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    PhysVec3 operator-() const { return {-x, -y, -z}; }
    PhysVec3 operator-(const PhysVec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    PhysVec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    PhysVec3& operator+=(const PhysVec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    PhysVec3& operator-=(const PhysVec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
};

inline PhysVec3 operator*(float s, const PhysVec3& v) { return v * s; }
inline PhysVec3 operator/(const PhysVec3& v, float s) {
    return PhysVec3{v.x / s, v.y / s, v.z / s};
}
inline PhysVec3 cross(const PhysVec3& a, const PhysVec3& b) {
    return PhysVec3{
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}
inline float dot(const PhysVec3& a, const PhysVec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

namespace Constants {
    constexpr float GRAVITY = 9.81f;
    constexpr float DEFAULT_AIR_DENSITY = 1.225f;
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 2.0f * PI;
    constexpr float DEG_TO_RAD = PI / 180.0f;
    constexpr float RAD_TO_DEG = 180.0f / PI;
    constexpr float DEFAULT_WHEEL_RADIUS = 0.33f;
    constexpr float DEFAULT_MASS = 1500.0f;
    constexpr float DEFAULT_WHEELBASE = 2.7f;
    constexpr float DEFAULT_TRACK_WIDTH = 1.6f;
    constexpr float DEFAULT_CG_HEIGHT = 0.45f;
    constexpr float DEFAULT_FRONT_AXLE_DIST = 1.35f;
    constexpr float DEFAULT_REAR_AXLE_DIST = 1.35f;
    constexpr float OPTIMAL_TIRE_TEMP = 80.0f;
    constexpr float DEFAULT_TIRE_PRESSURE = 2.2f;
    constexpr float DEFAULT_PEAK_SLIP_ANGLE = 8.0f;
    constexpr float DEFAULT_PEAK_SLIP_RATIO = 0.12f;
    constexpr float DEFAULT_CORNERING_STIFFNESS = 80000.0f;
    constexpr float OPTIMAL_BRAKE_TEMP = 300.0f;
    constexpr float DEFAULT_BRAKE_BIAS = 60.0f;
    constexpr float DEFAULT_MAX_RPM = 7500.0f;
    constexpr float DEFAULT_IDLE_RPM = 800.0f;
    constexpr float DEFAULT_PEAK_TORQUE_RPM = 4000.0f;
    constexpr float MAX_REASONABLE_SPEED = 400.0f;
    constexpr float MAX_REASONABLE_RPM = 20000.0f;
    constexpr float MAX_REASONABLE_FORCE = 100000.0f;
    constexpr float MAX_REASONABLE_TORQUE = 10000.0f;
    constexpr float MIN_TIMESTEP = 0.0001f;
    constexpr float MAX_TIMESTEP = 0.02f;
    constexpr float DEFAULT_TIMESTEP = 0.001f;
    constexpr int MAX_INTEGRATION_STEPS = 10;
}

struct WeatherState {
    float ambientTemp = 26.0f;
    float trackTemp = 30.0f;
    float airDensity = Constants::DEFAULT_AIR_DENSITY;
    float trackWetness = 0.0f;
    float rainIntensity = 0.0f;
    float windSpeed = 0.0f;
    float windDirection = 0.0f;
    float humidity = 0.5f;
    float cloudCover = 0.0f;
    float gripReduction() const {
        float risk = aquaplaningRisk();
        float reduction = trackWetness * 0.3f + risk * 0.2f;
        return std::clamp(reduction, 0.0f, 0.8f);
    }
    float aquaplaningRisk() const {
        float risk = trackWetness * 0.5f + rainIntensity * 0.001f;
        return std::clamp(risk, 0.0f, 1.0f);
    }
    bool isDry() const { return trackWetness < 0.1f && rainIntensity < 0.1f; }
    bool isWet() const { return trackWetness > 0.3f || rainIntensity > 0.5f; }
};

struct SimulationState {
    PhysVec3 position;
    PhysVec3 velocity;
    PhysVec3 acceleration;
    PhysVec3 angularVelocity;
    PhysVec3 rotation;
    float heading = 0.0f;
    float speed = 0.0f;
    float rpm = 0.0f;
    int gear = 1;
    float throttle = 0.0f;
    float brake = 0.0f;
    float steering = 0.0f;
    float fuel = 100.0f;
    float currentLapDistance = 0.0f;
    float lapTime = 0.0f;
    float bestLapTime = 1e9f;
    float sector1Time = 0.0f;
    float sector2Time = 0.0f;
    float sector3Time = 0.0f;
    double maxSpeed = 0.0;
    double avgSpeed = 0.0;
    double tyreTemp[4] = {30.0, 30.0, 30.0, 30.0};
    double tyrePressure[4] = {2.2, 2.2, 2.0, 2.0};
    double tyreWear[4] = {0.0, 0.0, 0.0, 0.0};
    bool inPitLane = false;
    bool pitLimiterActive = false;
    bool drsActive = false;
    bool drsAvailable = false;
    bool crossedStartFinish = false;
    bool cornerCutWarning = false;
    PhysVec3 worldPosition;
    PhysVec3 worldRotation;
    float kineticEnergy(float mass) const {
        return 0.5f * mass * (velocity.x * velocity.x +
                              velocity.y * velocity.y +
                              velocity.z * velocity.z);
    }
    float potentialEnergy(float mass, float gravity = Constants::GRAVITY) const {
        return mass * gravity * position.y;
    }
    bool isValid() const {
        auto checkVec = [](const PhysVec3& v) {
            return !std::isnan(v.x) && !std::isnan(v.y) && !std::isnan(v.z) &&
                   !std::isinf(v.x) && !std::isinf(v.y) && !std::isinf(v.z);
        };
        return checkVec(position) && checkVec(velocity) && checkVec(acceleration) &&
               checkVec(angularVelocity) && checkVec(rotation) &&
               speed >= 0.0f && speed <= Constants::MAX_REASONABLE_SPEED &&
               rpm >= 0.0f && rpm <= Constants::MAX_REASONABLE_RPM;
    }
};

struct WheelState {
    PhysVec3 position;
    PhysVec3 velocity;
    float normalLoad = 1.0f;
    float slipAngle = 0.0f;
    float slipRatio = 0.0f;
    float lateralForce = 0.0f;
    float longitudinalForce = 0.0f;
    float brakeTorque = 0.0f;
    float angularVelocity = 0.0f;
    float driveTorque = 0.0f;
    float temperature = 30.0f;
    float coreTemperature = 35.0f;
    float wear = 0.0f;
    float pressure = 2.0f;
    float combinedForce() const {
        return std::sqrt(lateralForce * lateralForce + longitudinalForce * longitudinalForce);
    }
    float frictionCircleRatio(float frictionCoeff) const {
        float maxForce = normalLoad * frictionCoeff;
        if (maxForce < 0.001f) return 0.0f;
        return combinedForce() / maxForce;
    }
    bool isLocked() const { return std::abs(angularVelocity) < 0.1f && std::abs(slipRatio) > 0.5f; }
    bool isSpinning(float vehicleSpeed) const {
        float wheelSpeed = angularVelocity * Constants::DEFAULT_WHEEL_RADIUS;
        return wheelSpeed > vehicleSpeed * 1.5f;
    }
};

struct TireForceData {
    float lateralForce = 0.0f;
    float longitudinalForce = 0.0f;
    float aligningMoment = 0.0f;
    float overturningMoment = 0.0f;
    float rollingResistance = 0.0f;
    float magnitude() const {
        return std::sqrt(lateralForce * lateralForce + longitudinalForce * longitudinalForce);
    }
};

struct DamageState {
    float bodyDamage = 0.0f;
    float aeroDamage = 0.0f;
    float suspensionDamage[4] = {0.0f};
    float engineDamage = 0.0f;
    float accumulatedImpact = 0.0f;
    int collisionCount = 0;
    bool isEliminated = false;
    float engineHealth = 1.0f;
    float enginePowerLoss = 0.0f;
    float engineOverheat = 0.0f;
    float transmissionHealth = 1.0f;
    float clutchDamage = 0.0f;
    float differentialDamage = 0.0f;
    float frontWingDamage = 0.0f;
    float rearWingDamage = 0.0f;
    float diffuserDamage = 0.0f;
    float floorDamage = 0.0f;
    float radiatorDamage = 0.0f;
    float steeringDamage = 0.0f;
    float brakeDamage[4] = {0.0f};
    float brakePadWear[4] = {0.0f};
    float tireWear[4] = {0.0f};
    float tireGraining[4] = {0.0f};
    float tireBlistering[4] = {0.0f};
    float powerMultiplier = 1.0f;
    float handlingMultiplier = 1.0f;
    float brakingMultiplier = 1.0f;
    float downforceMultiplier = 1.0f;
    float dragMultiplier = 1.0f;
    void reset() {
        bodyDamage = 0.0f;
        aeroDamage = 0.0f;
        for (int i = 0; i < 4; ++i) suspensionDamage[i] = 0.0f;
        engineDamage = 0.0f;
        accumulatedImpact = 0.0f;
        collisionCount = 0;
        isEliminated = false;
        engineHealth = 1.0f;
        enginePowerLoss = 0.0f;
        engineOverheat = 0.0f;
        transmissionHealth = 1.0f;
        clutchDamage = 0.0f;
        differentialDamage = 0.0f;
        frontWingDamage = 0.0f;
        rearWingDamage = 0.0f;
        diffuserDamage = 0.0f;
        floorDamage = 0.0f;
        radiatorDamage = 0.0f;
        steeringDamage = 0.0f;
        for (int i = 0; i < 4; ++i) {
            brakeDamage[i] = 0.0f;
            brakePadWear[i] = 0.0f;
            tireWear[i] = 0.0f;
            tireGraining[i] = 0.0f;
            tireBlistering[i] = 0.0f;
        }
        powerMultiplier = 1.0f;
        handlingMultiplier = 1.0f;
        brakingMultiplier = 1.0f;
        downforceMultiplier = 1.0f;
        dragMultiplier = 1.0f;
    }
    void applyImpact(float impactEnergy) {
        accumulatedImpact += impactEnergy;
        collisionCount++;
        bodyDamage = std::min(1.0f, bodyDamage + impactEnergy * 0.0001f);
        if (bodyDamage >= 1.0f) isEliminated = true;
    }
    float overallDamage() const {
        float total = bodyDamage + aeroDamage + engineDamage;
        for (int i = 0; i < 4; ++i) total += suspensionDamage[i] + brakeDamage[i];
        return std::clamp(total / 10.0f, 0.0f, 1.0f);
    }
    bool isCriticallyDamaged() const {
        return engineHealth < 0.2f || isEliminated || overallDamage() > 0.8f;
    }
};

struct LapTimeEstimate {
    float totalLapTime = 0.0f;
    float sector1Time = 0.0f;
    float sector2Time = 0.0f;
    float sector3Time = 0.0f;
    float avgSpeed = 0.0f;
    float minCornerSpeed = 0.0f;
    float fuelConsumption = 0.0f;
    float confidenceLevel = 0.0f;
    float topSpeed = 0.0f;
    float maxLateralG = 0.0f;
    int numGearChanges = 0;
};

struct ValidationMetrics {
    int nSamples = 0;
    double speedRMSE = 0.0;
    double lateralGRMSE = 0.0;
    double longitudinalGRMSE = 0.0;
    double rpmRMSE = 0.0;
    double speedMaxError = 0.0;
    double lateralGMaxError = 0.0;
    double longitudinalGMaxError = 0.0;
    double rpmMaxError = 0.0;
    double score() const {
        if (nSamples == 0) return 0.0;
        double rmseAvg = (speedRMSE / 10.0 + lateralGRMSE + longitudinalGRMSE + rpmRMSE / 1000.0) / 4.0;
        return std::clamp(1.0 - rmseAvg, 0.0, 1.0);
    }
};

enum class DriveLayout { FWD, RWD, AWD };
enum class TireModelType { Pacejka, Generic, Fiala, Brush };
enum class IntegrationMethod { Euler, RungeKutta2, RungeKutta4, Verlet, SymplecticEuler };
enum class DrivingCondition { Dry, Wet, LightRain, HeavyRain, StandingWater, Ice, Snow, Gravel, Mixed };

struct TireSlipCurve {
    std::string name;
    std::string compound;
    double peakSlipAngle = 8.0;
    double peakSlipRatio = 0.12;
    double peakLateralMu = 1.0;
    double peakLongitudinalMu = 1.1;
    double stiffnessLateral = 30000.0;
    double stiffnessLongitudinal = 50000.0;
};

} // namespace physics
} // namespace ks
