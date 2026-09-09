#include "EngineSimulator.h"
#include "VehiclePhysicsModels.h"
#include <algorithm>
#include <cmath>

namespace ks {
namespace physics {

EngineSimulator::EngineSimulator() : m_fuelModel(std::make_unique<FuelManagementModel>()) {
    // Initialize with default values
    m_fuelState.fuelKg = m_fuelConfig.capacityLiters * m_fuelConfig.fuelDensityKgPerLiter;
    m_fuelState.fuelCapacityKg = m_fuelState.fuelKg;
}

EngineSimulator::~EngineSimulator() = default;

void EngineSimulator::setEngineConfig(const EngineConfig& config) {
    m_engineConfig = config;
}

void EngineSimulator::setDrivetrainConfig(const DrivetrainConfig& config) {
    m_drivetrainConfig = config;
}

void EngineSimulator::setFuelConfig(const FuelConfig& config) {
    m_fuelConfig = config;
    m_fuelState.fuelCapacityKg = config.capacityLiters * config.fuelDensityKgPerLiter;
    if (m_fuelState.fuelKg > m_fuelState.fuelCapacityKg) {
        m_fuelState.fuelKg = m_fuelState.fuelCapacityKg;
    }
}

void EngineSimulator::startEngine() {
    m_engineState.isRunning = true;
    m_engineState.rpm = m_engineConfig.idleRpm;
}

void EngineSimulator::stopEngine() {
    m_engineState.isRunning = false;
    m_engineState.rpm = 0.0;
    m_engineState.torque = 0.0;
    m_engineState.power = 0.0;
}

void EngineSimulator::setThrottle(double throttle) {
    m_throttle = std::clamp(throttle, 0.0, 1.0);
}

void EngineSimulator::shiftUp() {
    if (m_drivetrainState.currentGear < m_drivetrainConfig.gearRatios.size()) {
        m_drivetrainState.currentGear++;
        m_drivetrainState.isShifting = true;
    }
}

void EngineSimulator::shiftDown() {
    if (m_drivetrainState.currentGear > 1) {
        m_drivetrainState.currentGear--;
        m_drivetrainState.isShifting = true;
    }
}

void EngineSimulator::setGear(int gear) {
    if (gear >= 1 && gear <= m_drivetrainConfig.gearRatios.size()) {
        m_drivetrainState.currentGear = gear;
    }
}

void EngineSimulator::update(double dt, double vehicleSpeed) {
    m_lastDt = dt;
    
    if (!m_engineState.isRunning) {
        return;
    }
    
    // Calculate wheel speed and engine RPM
    m_drivetrainState.wheelSpeed = vehicleSpeed / m_drivetrainConfig.wheelRadius;
    m_drivetrainState.engineRpm = calculateEngineRpm(vehicleSpeed, m_drivetrainState.currentGear);
    
    // Clamp RPM
    m_drivetrainState.engineRpm = std::clamp(m_drivetrainState.engineRpm, 
        m_engineConfig.idleRpm, m_engineConfig.revLimit);
    
    // Update engine state
    m_engineState.rpm = m_drivetrainState.engineRpm;
    m_engineState.throttle = m_throttle;
    
    // Calculate engine torque and power
    m_engineState.torque = calculateEngineTorque(m_engineState.rpm, m_throttle);
    m_engineState.power = calculateEnginePower(m_engineState.rpm);
    
    // Calculate axle torque
    m_drivetrainState.axleTorque = calculateAxleTorque(m_engineState.torque, m_drivetrainState.currentGear);
    
    // Update automatic shifting
    updateAutomaticShifting();
    
    // Update fuel consumption using advanced model
    if (m_fuelConfig.consumptionEnabled && m_engineState.isRunning) {
        // Update the advanced fuel model
        m_fuelModel->update(dt, m_throttle, m_engineState.rpm, vehicleSpeed, 0.0);  // elevation = 0
        
        // Get fuel state from advanced model
        FuelState fuelStateModel = m_fuelModel->getFuelState();
        
        // Use advanced model's consumption rate
        double fuelKgRate = fuelStateModel.fuelConsumptionRate * m_fuelConfig.fuelDensityKgPerLiter;
        
        m_fuelState.fuelKg = std::max(0.0, m_fuelState.fuelKg - fuelKgRate * dt);
        m_fuelState.consumptionRate = fuelKgRate;
        m_fuelState.lapFuelUsage = fuelStateModel.lapFuelUsage;
    }
}

void EngineSimulator::setFuelKg(double kg) {
    m_fuelState.fuelKg = std::clamp(kg, 0.0, m_fuelState.fuelCapacityKg);
}

void EngineSimulator::addFuel(double kg) {
    m_fuelState.fuelKg = std::min(m_fuelState.fuelKg + kg, m_fuelState.fuelCapacityKg);
}

double EngineSimulator::calculateEngineTorque(double rpm, double throttle) const {
    if (rpm <= 0 || rpm >= m_engineConfig.revLimit) {
        return 0.0;
    }
    
    // Simple torque curve based on RPM
    double normalized = rpm / m_engineConfig.peakTorqueRpm;
    double torqueCurve = 2.0 * normalized / (1.0 + normalized * normalized);
    
    // Apply throttle and peak torque
    double torque = m_engineConfig.peakTorqueNm * torqueCurve * throttle;
    
    // Apply engine braking when throttle is low
    if (throttle < 0.05) {
        torque -= calculateEngineBraking(rpm);
    }
    
    return torque;
}

double EngineSimulator::calculateEnginePower(double rpm) const {
    double torque = calculateEngineTorque(rpm, 1.0);
    return torque * rpm * 2.0 * M_PI / 60000.0; // kW
}

double EngineSimulator::calculateEngineBraking(double rpm) const {
    return m_engineConfig.peakTorqueNm * m_engineConfig.engineBrakingFactor * (rpm / m_engineConfig.maxRpm);
}

double EngineSimulator::calculateAxleTorque(double engineTorque, int gear) const {
    if (gear < 1 || gear > m_drivetrainConfig.gearRatios.size()) {
        return 0.0;
    }
    
    double gearRatio = m_drivetrainConfig.gearRatios[gear - 1];
    return engineTorque * gearRatio * m_drivetrainConfig.finalDriveRatio;
}

double EngineSimulator::calculateWheelSpeed(double engineRpm, int gear) const {
    if (gear < 1 || gear > m_drivetrainConfig.gearRatios.size()) {
        return 0.0;
    }
    
    double gearRatio = m_drivetrainConfig.gearRatios[gear - 1];
    double wheelRpm = engineRpm / (gearRatio * m_drivetrainConfig.finalDriveRatio);
    return wheelRpm * 2.0 * M_PI / 60.0; // rad/s
}

double EngineSimulator::calculateEngineRpm(double wheelSpeed, int gear) const {
    if (gear < 1 || gear > m_drivetrainConfig.gearRatios.size()) {
        return m_engineConfig.idleRpm;
    }
    
    double wheelRpm = wheelSpeed * 60.0 / (2.0 * M_PI);
    double gearRatio = m_drivetrainConfig.gearRatios[gear - 1];
    return wheelRpm * gearRatio * m_drivetrainConfig.finalDriveRatio;
}

void EngineSimulator::updateAutomaticShifting() {
    // Simple automatic shifting logic
    if (m_drivetrainState.isShifting) {
        m_drivetrainState.isShifting = false;
        return;
    }
    
    // Shift up at high RPM
    if (m_engineState.rpm > m_engineConfig.maxRpm * 0.95) {
        shiftUp();
    }
    // Shift down at low RPM
    else if (m_engineState.rpm < m_engineConfig.maxRpm * 0.3 && m_drivetrainState.currentGear > 1) {
        shiftDown();
    }
}

} // namespace physics
} // namespace ks