#pragma once

/**
 * @file StrategySimulator.h
 * @brief Race strategy and pit stop planning simulation
 * @copyright KS Physics Engine
 */

#include "PhysicsCoreTypes.h"
#include "VehiclePhysicsModels.h"
#include <QObject>
#include <QVector>

namespace ks {
namespace physics {

// ============================================================================
// Strategy Configuration
// ============================================================================

struct StrategyConfig {
    int totalLaps = 50;                 ///< Total race laps
    float fuelPerLap = 1.5f;           ///< Fuel consumption per lap (kg)
    float tireDegredationRate = 0.02f; ///< Tire wear per lap
    float pitStopTimeLoss = 22.0f;     ///< Time loss for pit stop (seconds)
    float trackPositionWeight = 0.7f;  ///< Weight for track position strategy
};

// ============================================================================
// Strategy State
// ============================================================================

struct StrategyState {
    int currentLap = 0;
    int position = 1;
    float currentFuel = 60.0f;
    float fuelPerLap = 1.5f;
    float tireWear[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float lastLapTime = 0.0f;
    float bestLapTime = 1e9f;
    float gapAhead = 0.0f;
    float gapBehind = 0.0f;
};

// ============================================================================
// Pit Stop Recommendation
// ============================================================================

struct PitStopRecommendation {
    bool shouldPit = false;
    int pitLap = -1;
    float estimatedTimeGain = 0.0f;
    float fuelToAdd = 0.0f;
    bool changeTires = true;
    QString reason;
};

// ============================================================================
// Strategy Simulator Class
// ============================================================================

class StrategySimulator {
public:
    StrategySimulator();
    ~StrategySimulator() = default;

    // Configuration
    void setStrategyConfig(const StrategyConfig& config);
    
    // State access
    StrategyState strategyState() const { return m_strategyState; }
    
    // Update
    void update(double dt, const StrategyState& currentState);
    
    // Strategy calculations
    PitStopRecommendation calculatePitStopRecommendation() const;
    float calculateOptimalFuelLoad(int lapsRemaining) const;
    float calculateTireLife() const;
    
    // Query
    int currentLap() const { return m_strategyState.currentLap; }
    int position() const { return m_strategyState.position; }
    float fuelRemaining() const { return m_strategyState.currentFuel; }
    
    // Control
    void setCurrentLap(int lap) { m_strategyState.currentLap = lap; }
    void setPosition(int pos) { m_strategyState.position = pos; }
    void setFuel(float fuel) { m_strategyState.currentFuel = fuel; }
    void reset();

private:
    // Configuration
    StrategyConfig m_strategyConfig;
    
    // State
    StrategyState m_strategyState;
    
    // Advanced model
    RaceStrategyModel m_strategyModel;
};

} // namespace physics
} // namespace ks