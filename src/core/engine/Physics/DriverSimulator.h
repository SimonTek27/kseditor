#pragma once

/**
 * @file DriverSimulator.h
 * @brief Driver behavior simulation and input processing
 * @copyright KS Physics Engine
 */

#include "PhysicsCoreTypes.h"
#include "VehiclePhysicsModels.h"
#include <QObject>

namespace ks {
namespace physics {

// ============================================================================
// Driver Configuration
// ============================================================================

struct DriverConfig {
    float reactionTime = 0.25f;        ///< Driver reaction time (seconds)
    float fatigueRate = 0.001f;        ///< Fatigue accumulation rate
    float focusDecayRate = 0.0005f;    ///< Focus decay rate
    float aggressiveness = 0.5f;       ///< Driver aggressiveness (0-1)
    float consistency = 0.8f;          ///< Driver consistency (0-1)
};

// ============================================================================
// Driver Input with Processing
// ============================================================================

struct ProcessedDriverInput {
    float throttle = 0.0f;             ///< Processed throttle (0-1)
    float brake = 0.0f;                ///< Processed brake (0-1)
    float steer = 0.0f;                ///< Processed steering (-1 to 1)
    bool isReactionDelayed = false;    ///< Whether input is delayed
    float reactionDelay = 0.0f;        ///< Current reaction delay
    float errorMagnitude = 0.0f;       ///< Input error magnitude
};

// ============================================================================
// Driver Simulator Class
// ============================================================================

class DriverSimulator {
public:
    DriverSimulator();
    ~DriverSimulator() = default;

    // Configuration
    void setDriverConfig(const DriverConfig& config);
    
    // State access
    DriverState driverState() const { return m_driverModel.getDriverState(); }
    
    // Update
    void update(double dt, double speed, double lateralAccel, 
               double brakingForce, double corneringLoad);
    
    // Input processing
    ProcessedDriverInput processInput(float rawThrottle, float rawBrake, float rawSteer,
                                     double speed, double targetSpeed);
    
    // Query
    float reactionTime() const { return m_driverModel.calculateReactionDelay(0.0f); }
    float fatigueLevel() const { return m_driverModel.getDriverState().fatigueLevel; }
    float focusLevel() const { return m_driverModel.getDriverState().focusLevel; }
    float driverQuality() const { return m_driverModel.getDriverQuality(); }
    
    // Reset
    void reset();

private:
    // Internal calculations
    float calculateInputError(float input) const;
    float applyReactionDelay(float input, float dt);
    
    // Configuration
    DriverConfig m_driverConfig;
    
    // Advanced model
    DriverModel m_driverModel;
    
    // State
    float m_throttleDelay = 0.0f;
    float m_brakeDelay = 0.0f;
    float m_steerDelay = 0.0f;
};

} // namespace physics
} // namespace ks