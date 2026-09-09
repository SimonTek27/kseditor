#include "AeroSimulator.h"
#include <algorithm>
#include <cmath>

namespace ks {
namespace physics {

AeroSimulator::AeroSimulator() {
    m_groundEffect = GroundEffectStateAero();
}

void AeroSimulator::setAeroConfig(const AeroConfig& config) {
    m_aeroConfig = config;
}

void AeroSimulator::setWingConfig(const WingConfigAero& config) {
    m_wingConfig = config;
}

void AeroSimulator::setDiffuserConfig(const DiffuserConfigAero& config) {
    m_diffuserConfig = config;
}

void AeroSimulator::update(double dt, double speed, double rideHeightFront, double rideHeightRear) {
    // Update ground effect state
    m_groundEffect.rideHeightFront = rideHeightFront;
    m_groundEffect.rideHeightRear = rideHeightRear;
    
    // Calculate effective coefficients
    double effectiveCd = m_aeroConfig.dragCoefficient;
    double effectiveClFront = m_aeroConfig.liftCoefficientFront;
    double effectiveClRear = m_aeroConfig.liftCoefficientRear;
    
    // Apply wing angles (simplified)
    double frontWingEffect = 1.0 + m_wingConfig.frontAngle * 0.01;  // 1% per degree
    double rearWingEffect = 1.0 + m_wingConfig.rearAngle * 0.01;
    
    effectiveClFront *= frontWingEffect;
    effectiveClRear *= rearWingEffect;
    
    // Apply ground effect
    double groundEffectFront = calculateGroundEffect(rideHeightFront, speed);
    double groundEffectRear = calculateGroundEffect(rideHeightRear, speed);
    
    m_groundEffect.downforceGain = (groundEffectFront + groundEffectRear) / 2.0;
    effectiveClFront *= m_groundEffect.downforceGain;
    effectiveClRear *= m_groundEffect.downforceGain;
    
    // Calculate porpoising
    m_groundEffect.porpoisingAmplitude = calculatePorpoising(
        (rideHeightFront + rideHeightRear) / 2.0, speed, dt);
    
    // Apply porpoising effect
    double porpoisingEffect = 1.0 + m_groundEffect.porpoisingAmplitude * 0.1;
    effectiveClFront *= porpoisingEffect;
    effectiveClRear *= porpoisingEffect;
    
    // DRS effect
    if (m_drsActive) {
        effectiveCd *= (1.0 - m_drsDragReduction);
    }
    
    // Calculate forces
    m_forces.drag = calculateDrag(speed, effectiveCd, m_aeroConfig.frontalArea, AIR_DENSITY);
    m_forces.downforceFront = calculateDownforce(speed, effectiveClFront, 
        m_aeroConfig.frontalArea * 0.4, AIR_DENSITY);  // 40% front
    m_forces.downforceRear = calculateDownforce(speed, effectiveClRear,
        m_aeroConfig.frontalArea * 0.6, AIR_DENSITY);  // 60% rear
    m_forces.totalDownforce = m_forces.downforceFront + m_forces.downforceRear;
    
    // Store effective coefficients
    m_forces.dragCoeff = effectiveCd;
    m_forces.liftCoeffFront = effectiveClFront;
    m_forces.liftCoeffRear = effectiveClRear;
}

void AeroSimulator::setDrsActive(bool active) {
    m_drsActive = active;
}

double AeroSimulator::calculateDrag(double speed, double cd, double area, double airDensity) const {
    // Drag force = 0.5 * rho * v² * Cd * A
    return 0.5 * airDensity * speed * speed * cd * area;
}

double AeroSimulator::calculateDownforce(double speed, double cl, double area, 
                                        double airDensity) const {
    // Downforce = 0.5 * rho * v² * Cl * A
    // Note: Cl is negative for downforce, so we take absolute value
    return 0.5 * airDensity * speed * speed * std::abs(cl) * area;
}

double AeroSimulator::calculateGroundEffect(double rideHeight, double speed) const {
    // Ground effect increases as ride height decreases
    // Simplified model: effect = 1 + k / rideHeight
    if (rideHeight < 0.01) rideHeight = 0.01;  // Prevent division by zero
    
    double baseEffect = 1.0;
    double groundEffectFactor = 0.1;  // Tuning parameter
    double effect = baseEffect + groundEffectFactor / rideHeight;
    
    // Limit maximum effect
    return std::clamp(effect, 1.0, 2.0);
}

double AeroSimulator::calculatePorpoising(double rideHeight, double speed, double dt) {
    // Porpoising is an oscillation caused by ground effect
    // Simplified model: oscillation amplitude increases with speed and decreases with ride height
    
    if (speed < 50.0) return 0.0;  // No porpoising at low speed
    
    double speedFactor = (speed - 50.0) / 100.0;  // Normalized speed
    double heightFactor = 1.0 / (rideHeight * 10.0);  // Height factor
    
    // Oscillation frequency (Hz)
    double frequency = 2.0 + speedFactor * 3.0;
    
    // Update phase
    m_porpoisingPhase += frequency * dt * 2.0 * M_PI;
    if (m_porpoisingPhase > 2.0 * M_PI) {
        m_porpoisingPhase -= 2.0 * M_PI;
    }
    
    // Amplitude
    double amplitude = speedFactor * heightFactor * 0.01;  // Small amplitude
    
    return amplitude * std::sin(m_porpoisingPhase);
}

} // namespace physics
} // namespace ks