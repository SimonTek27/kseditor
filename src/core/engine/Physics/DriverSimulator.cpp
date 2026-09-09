#include "DriverSimulator.h"
#include <algorithm>
#include <cmath>

namespace ks {
namespace physics {

DriverSimulator::DriverSimulator() {
    m_driverModel = DriverModel();
}

void DriverSimulator::setDriverConfig(const DriverConfig& config) {
    m_driverConfig = config;
}

void DriverSimulator::update(double dt, double speed, double lateralAccel,
                            double brakingForce, double corneringLoad) {
    // Update the underlying driver model
    m_driverModel.update(dt, speed, lateralAccel, brakingForce, corneringLoad);
}

ProcessedDriverInput DriverSimulator::processInput(float rawThrottle, float rawBrake, float rawSteer,
                                                  double speed, double targetSpeed) {
    ProcessedDriverInput processed;
    
    // Get driver state
    DriverState state = m_driverModel.getDriverState();
    
    // Calculate reaction delay
    float throttleDelay = m_driverModel.calculateReactionDelay(rawThrottle);
    float brakeDelay = m_driverModel.calculateReactionDelay(rawBrake);
    float steerDelay = m_driverModel.calculateReactionDelay(rawSteer);
    
    // Apply reaction delay (simplified - just attenuate input)
    float throttleEffect = m_driverModel.calculateFatigueEffect();
    float focusEffect = state.focusLevel;
    
    // Apply errors based on driver quality
    float errorProb = m_driverModel.calculateErrorProbability();
    float throttleError = calculateInputError(rawThrottle) * errorProb;
    float brakeError = calculateInputError(rawBrake) * errorProb;
    float steerError = calculateInputError(rawSteer) * errorProb;
    
    // Process inputs
    processed.throttle = std::clamp(rawThrottle * throttleEffect * focusEffect + throttleError, 
                                   0.0f, 1.0f);
    processed.brake = std::clamp(rawBrake * throttleEffect * focusEffect + brakeError,
                                0.0f, 1.0f);
    processed.steer = std::clamp(rawSteer * focusEffect + steerError, -1.0f, 1.0f);
    
    // Store delay information
    processed.isReactionDelayed = (throttleDelay > 0.1f || brakeDelay > 0.1f || steerDelay > 0.1f);
    processed.reactionDelay = std::max({throttleDelay, brakeDelay, steerDelay});
    processed.errorMagnitude = std::abs(throttleError) + std::abs(brakeError) + std::abs(steerError);
    
    return processed;
}

float DriverSimulator::calculateInputError(float input) const {
    // Random error based on driver consistency
    float baseError = (1.0f - m_driverConfig.consistency) * 0.1f;
    float inputMagnitude = std::abs(input);
    
    // Higher errors at higher inputs
    return baseError * inputMagnitude * (2.0f * static_cast<float>(rand()) / RAND_MAX - 1.0f);
}

float DriverSimulator::applyReactionDelay(float input, float dt) {
    // Simple low-pass filter for reaction delay
    float tau = m_driverConfig.reactionTime;
    return input * (1.0f - std::exp(-dt / tau));
}

void DriverSimulator::reset() {
    m_driverModel.reset();
    m_throttleDelay = 0.0f;
    m_brakeDelay = 0.0f;
    m_steerDelay = 0.0f;
}

} // namespace physics
} // namespace ks