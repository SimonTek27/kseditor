#pragma once

/**
 * @file EngineSimulator.h
 * @brief Engine, drivetrain, and fuel management simulation
 * @copyright KS Physics Engine
 */

#include "PhysicsCoreTypes.h"
#include <QObject>
#include <QVector>
#include <memory>

namespace ks {
namespace physics {

struct FuelManagementModel;

// ============================================================================
// Engine Configuration
// ============================================================================

struct EngineConfig {
    double maxPowerKw = 350.0;        ///< Maximum engine power in kW
    double maxRpm = 7500.0;           ///< Maximum engine RPM
    double idleRpm = 800.0;           ///< Idle RPM
    double peakTorqueRpm = 4000.0;    ///< RPM at peak torque
    double peakTorqueNm = 400.0;      ///< Peak torque in Nm
    double revLimit = 8000.0;         ///< Hard rev limit
    double engineBrakingFactor = 0.1; ///< Engine braking factor
};

// ============================================================================
// Drivetrain Configuration
// ============================================================================

struct DrivetrainConfig {
    DriveLayout driveLayout = DriveLayout::RWD;
    QVector<double> gearRatios = {3.5, 2.5, 1.8, 1.4, 1.1, 0.9};
    double finalDriveRatio = 3.8;
    double wheelRadius = 0.33;
    double clutchSlipFactor = 0.05;
};

// ============================================================================
// Fuel Configuration
// ============================================================================

struct FuelConfig {
    bool consumptionEnabled = true;
    double capacityLiters = 80.0;
    double fuelDensityKgPerLiter = 0.75;
    double consumptionFactor = 0.2;   ///< kW to liters per hour conversion
};

// ============================================================================
// Engine State
// ============================================================================

struct EngineState {
    double rpm = 800.0;
    double torque = 0.0;
    double power = 0.0;
    double throttle = 0.0;
    bool isRunning = false;
    double temperature = 80.0;
};

// ============================================================================
// Drivetrain State
// ============================================================================

struct DrivetrainState {
    int currentGear = 1;
    double wheelSpeed = 0.0;
    double engineRpm = 0.0;
    double axleTorque = 0.0;
    double clutchEngagement = 1.0;
    bool isShifting = false;
};

// ============================================================================
// Fuel State
// ============================================================================

struct EngineFuelState {
    double fuelKg = 60.0;
    double fuelCapacityKg = 60.0;
    double consumptionRate = 0.0;
    double lapFuelUsage = 0.0;
};

// ============================================================================
// Engine Simulator Class
// ============================================================================

class EngineSimulator {
public:
    EngineSimulator();
    ~EngineSimulator();

    // Configuration
    void setEngineConfig(const EngineConfig& config);
    void setDrivetrainConfig(const DrivetrainConfig& config);
    void setFuelConfig(const FuelConfig& config);
    
    // State access
    EngineState engineState() const { return m_engineState; }
    DrivetrainState drivetrainState() const { return m_drivetrainState; }
    EngineFuelState fuelState() const { return m_fuelState; }
    
    // Control
    void startEngine();
    void stopEngine();
    void setThrottle(double throttle);
    void shiftUp();
    void shiftDown();
    void setGear(int gear);
    
    // Update
    void update(double dt, double vehicleSpeed);
    
    // Query
    double engineTorque() const { return m_engineState.torque; }
    double enginePower() const { return m_engineState.power; }
    double engineRpm() const { return m_engineState.rpm; }
    int currentGear() const { return m_drivetrainState.currentGear; }
    double fuelKg() const { return m_fuelState.fuelKg; }
    bool isRunning() const { return m_engineState.isRunning; }
    
    // Fuel
    void setFuelKg(double kg);
    void addFuel(double kg);
    double fuelConsumptionPerLap() const { return m_fuelState.lapFuelUsage; }

private:
    // Engine calculations
    double calculateEngineTorque(double rpm, double throttle) const;
    double calculateEnginePower(double rpm) const;
    double calculateEngineBraking(double rpm) const;
    
    // Drivetrain calculations
    double calculateAxleTorque(double engineTorque, int gear) const;
    double calculateWheelSpeed(double engineRpm, int gear) const;
    double calculateEngineRpm(double wheelSpeed, int gear) const;
    
    // Automatic shifting
    void updateAutomaticShifting();
    
    // Configuration
    EngineConfig m_engineConfig;
    DrivetrainConfig m_drivetrainConfig;
    FuelConfig m_fuelConfig;
    
    // State
    EngineState m_engineState;
    DrivetrainState m_drivetrainState;
    EngineFuelState m_fuelState;
    
    // Advanced models integration
    std::unique_ptr<FuelManagementModel> m_fuelModel;
    
    // Internal
    double m_throttle = 0.0;
    double m_lastDt = 0.0;
};

} // namespace physics
} // namespace ks