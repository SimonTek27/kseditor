#include "StrategySimulator.h"
#include <algorithm>
#include <cmath>

namespace ks {
namespace physics {

StrategySimulator::StrategySimulator() {
    m_strategyState = StrategyState();
}

void StrategySimulator::setStrategyConfig(const StrategyConfig& config) {
    m_strategyConfig = config;
}

void StrategySimulator::update(double dt, const StrategyState& currentState) {
    // Update state
    m_strategyState = currentState;
    
    // Update advanced strategy model
    // Note: RaceStrategyModel doesn't have an update method in the provided code
    // so we'll use direct calculations
}

PitStopRecommendation StrategySimulator::calculatePitStopRecommendation() const {
    PitStopRecommendation recommendation;
    
    // Calculate fuel needed for remaining laps
    int lapsRemaining = m_strategyConfig.totalLaps - m_strategyState.currentLap;
    float fuelNeeded = lapsRemaining * m_strategyState.fuelPerLap;
    
    // Check if we need to pit for fuel
    bool needsFuel = m_strategyState.currentFuel < fuelNeeded + 5.0f;  // 5kg safety margin
    
    // Check tire wear
    float maxTireWear = *std::max_element(m_strategyState.tireWear, m_strategyState.tireWear + 4);
    bool needsTires = maxTireWear > 0.7f;  // 70% wear threshold
    
    // Determine if we should pit
    if (needsFuel || needsTires) {
        recommendation.shouldPit = true;
        
        // Calculate optimal pit lap (simplified)
        if (needsFuel) {
            float fuelDeficit = fuelNeeded - m_strategyState.currentFuel;
            int lapsUntilEmpty = static_cast<int>(m_strategyState.currentFuel / m_strategyState.fuelPerLap);
            recommendation.pitLap = m_strategyState.currentLap + std::max(1, lapsUntilEmpty - 2);
            recommendation.fuelToAdd = std::min(fuelDeficit + 10.0f, m_strategyConfig.fuelPerLap * 10.0f);
            recommendation.reason = "Low fuel";
        }
        
        if (needsTires) {
            int tireLaps = static_cast<int>((1.0f - maxTireWear) / m_strategyConfig.tireDegredationRate);
            int tirePitLap = m_strategyState.currentLap + std::max(1, tireLaps - 1);
            
            if (!recommendation.shouldPit || tirePitLap < recommendation.pitLap) {
                recommendation.pitLap = tirePitLap;
                recommendation.changeTires = true;
                recommendation.reason = "Tire wear";
            }
        }
        
        // Estimate time gain/loss
        float currentLapTime = m_strategyState.lastLapTime;
        float pitLoss = m_strategyConfig.pitStopTimeLoss;
        
        // Fresh tires should improve lap time
        float tireImprovement = maxTireWear * 2.0f;  // Up to 2 seconds improvement
        recommendation.estimatedTimeGain = tireImprovement - pitLoss;
    }
    
    return recommendation;
}

float StrategySimulator::calculateOptimalFuelLoad(int lapsRemaining) const {
    // Calculate fuel needed with safety margin
    float baseFuel = lapsRemaining * m_strategyState.fuelPerLap;
    float safetyMargin = 5.0f;  // 5kg safety margin
    
    return std::min(baseFuel + safetyMargin, m_strategyConfig.fuelPerLap * 20.0f);
}

float StrategySimulator::calculateTireLife() const {
    // Calculate average tire life remaining
    float totalWear = 0.0f;
    for (int i = 0; i < 4; ++i) {
        totalWear += m_strategyState.tireWear[i];
    }
    float avgWear = totalWear / 4.0f;
    
    return 1.0f - avgWear;  // Return remaining life (0-1)
}

void StrategySimulator::reset() {
    m_strategyState = StrategyState();
}

} // namespace physics
} // namespace ks