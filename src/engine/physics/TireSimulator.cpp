#include "TireSimulator.h"
#include <algorithm>
#include <cmath>

namespace ks {
namespace physics {

TireSimulator::TireSimulator() {
    // Initialize all wheels with default state
    for (int i = 0; i < 4; ++i) {
        m_wheelStates[i] = TireWheelState();
        m_configs[i] = TireConfig();
    }
}

void TireSimulator::setTireConfig(const TireConfig& config) {
    for (int i = 0; i < 4; ++i) {
        m_configs[i] = config;
    }
}

void TireSimulator::setTireConfig(int wheel, const TireConfig& config) {
    if (wheel >= 0 && wheel < 4) {
        m_configs[wheel] = config;
    }
}

TireWheelState TireSimulator::wheelState(int wheel) const {
    if (wheel >= 0 && wheel < 4) {
        return m_wheelStates[wheel];
    }
    return TireWheelState();
}

void TireSimulator::update(double dt, double vehicleSpeed, const WeatherState& weather) {
    for (int i = 0; i < 4; ++i) {
        // Update temperature
        updateTemperature(i, dt, m_wheelStates[i].slipAngle, m_wheelStates[i].slipRatio,
                         m_wheelStates[i].normalLoad, weather.ambientTemp);
        
        // Update wear using advanced TireWearModel
        m_wearModels[i].update(dt, m_wheelStates[i].slipAngle, m_wheelStates[i].slipRatio,
                              m_wheelStates[i].normalLoad, vehicleSpeed, 
                              m_wheelStates[i].temperature, 0);  // surfaceType = 0 (asphalt)
        
        // Get wear state from advanced model
        TireWearState wearState = m_wearModels[i].getTireCondition();
        m_wheelStates[i].wear = wearState.totalWear[i] / 8.0f;  // Normalize to 0-1
        
        // Update friction coefficient using advanced model calculations
        double tempEffect = m_wearModels[i].calculateTemperatureEffect(m_wheelStates[i].temperature);
        double wearEffect = m_wearModels[i].calculateGripReduction(m_wheelStates[i].wear);
        double weatherEffect = 1.0 - weather.gripReduction();
        
        m_wheelStates[i].frictionCoefficient = tempEffect * wearEffect * weatherEffect;
    }
}

void TireSimulator::calculateForces(int wheel, double slipAngle, double slipRatio,
                                    double normalLoad, double frictionCoefficient) {
    if (wheel < 0 || wheel >= 4) return;
    
    auto& state = m_wheelStates[wheel];
    state.slipAngle = slipAngle;
    state.slipRatio = slipRatio;
    state.normalLoad = normalLoad;
    
    // Calculate forces
    state.lateralForce = calculateLateralForce(slipAngle, normalLoad, frictionCoefficient);
    state.longitudinalForce = calculateLongitudinalForce(slipRatio, normalLoad, frictionCoefficient);
}

double TireSimulator::frictionCoefficient(int wheel) const {
    if (wheel >= 0 && wheel < 4) {
        return m_wheelStates[wheel].frictionCoefficient;
    }
    return 1.0;
}

double TireSimulator::wear(int wheel) const {
    if (wheel >= 0 && wheel < 4) {
        return m_wheelStates[wheel].wear;
    }
    return 0.0;
}

double TireSimulator::temperature(int wheel) const {
    if (wheel >= 0 && wheel < 4) {
        return m_wheelStates[wheel].temperature;
    }
    return 30.0;
}

void TireSimulator::setNormalLoad(int wheel, double load) {
    if (wheel >= 0 && wheel < 4) {
        m_wheelStates[wheel].normalLoad = load;
    }
}

void TireSimulator::setWear(int wheel, double wear) {
    if (wheel >= 0 && wheel < 4) {
        m_wheelStates[wheel].wear = std::clamp(wear, 0.0, 1.0);
    }
}

void TireSimulator::resetWear() {
    for (int i = 0; i < 4; ++i) {
        m_wheelStates[i].wear = 0.0;
    }
}

void TireSimulator::reset() {
    for (int i = 0; i < 4; ++i) {
        m_wheelStates[i] = TireWheelState();
    }
}

double TireSimulator::calculateLateralForce(double slipAngle, double normalLoad, 
                                           double friction) const {
    // Simplified Pacejka-like model
    double slipRad = slipAngle * DEG_TO_RAD;
    double peakForce = normalLoad * friction;
    double b = 10.0 / peakForce;
    
    // Magic Formula approximation
    double force = peakForce * std::sin(1.3 * std::atan(b * slipRad - 
                   0.0 * (b * slipRad - std::atan(b * slipRad))));
    
    return force;
}

double TireSimulator::calculateLongitudinalForce(double slipRatio, double normalLoad,
                                                double friction) const {
    // Simplified longitudinal force model
    double peakForce = 1.1 * normalLoad * friction;
    double b = 10.0 / peakForce;
    
    // Magic Formula approximation
    double force = peakForce * std::sin(1.3 * std::atan(b * slipRatio - 
                   0.0 * (b * slipRatio - std::atan(b * slipRatio))));
    
    return force;
}

double TireSimulator::calculateSlipAngle(double vx, double vy, double yawRate,
                                        double wheelX, double wheelY, double steerAngle) const {
    if (std::abs(vx) < 0.1) return 0.0;
    
    // Velocity at wheel contact patch
    double wheelVx = vx - yawRate * wheelY;
    double wheelVy = vy + yawRate * wheelX;
    
    // Slip angle
    double slipAngle = std::atan2(wheelVy, wheelVx) - steerAngle;
    
    // Normalize to [-pi, pi]
    while (slipAngle > M_PI) slipAngle -= 2.0 * M_PI;
    while (slipAngle < -M_PI) slipAngle += 2.0 * M_PI;
    
    return slipAngle * RAD_TO_DEG;
}

double TireSimulator::calculateSlipRatio(double wheelSpeed, double vehicleSpeed) const {
    if (std::abs(vehicleSpeed) < 0.1) return 0.0;
    return (wheelSpeed - vehicleSpeed) / vehicleSpeed;
}

void TireSimulator::updateTemperature(int wheel, double dt, double slipAngle, double slipRatio,
                                     double normalLoad, double ambientTemp) {
    if (wheel < 0 || wheel >= 4) return;
    
    auto& state = m_wheelStates[wheel];
    const auto& config = m_configs[wheel];
    
    // Heat generation from slip
    double heatGen = calculateHeatGeneration(slipAngle, slipRatio, normalLoad);
    
    // Heat dissipation to environment
    double heatDis = calculateHeatDissipation(state.temperature, ambientTemp);
    
    // Update temperature
    state.temperature += (heatGen - heatDis) * dt / config.thermalMass;
    state.temperature = std::clamp(state.temperature, ambientTemp, 150.0);
    
    // Core temperature follows surface with delay
    double coreRate = 0.1; // Slow response
    state.coreTemperature += (state.temperature - state.coreTemperature) * coreRate * dt;
}

double TireSimulator::calculateHeatGeneration(double slipAngle, double slipRatio, 
                                            double normalLoad) const {
    // Heat generation proportional to slip energy
    double slipEnergy = std::abs(slipAngle) * std::abs(slipRatio) * normalLoad;
    return slipEnergy * 0.01; // Scaling factor
}

double TireSimulator::calculateHeatDissipation(double temp, double ambientTemp) const {
    // Newton's cooling law
    double coolingCoefficient = 0.1;
    return coolingCoefficient * (temp - ambientTemp);
}

void TireSimulator::updateWear(int wheel, double dt, double slipAngle, double slipRatio,
                              double normalLoad, double temperature) {
    if (wheel < 0 || wheel >= 4) return;
    
    auto& state = m_wheelStates[wheel];
    
    // Calculate wear rate
    double wearRate = calculateWearRate(slipAngle, slipRatio, normalLoad, temperature);
    
    // Update wear
    state.wear += wearRate * dt;
    state.wear = std::clamp(state.wear, 0.0, 1.0);
}

double TireSimulator::calculateWearRate(double slipAngle, double slipRatio, 
                                       double normalLoad, double temperature) const {
    // Wear rate increases with slip and load
    double slipWear = (std::abs(slipAngle) * 0.1 + std::abs(slipRatio) * 0.5);
    double loadWear = normalLoad / 5000.0; // Normalized load
    
    // Temperature effect (more wear at extreme temps)
    double tempEffect = 1.0;
    if (temperature > 100.0) {
        tempEffect = 1.0 + (temperature - 100.0) * 0.02;
    }
    
    return slipWear * loadWear * tempEffect * 0.001; // Base wear rate
}

} // namespace physics
} // namespace ks