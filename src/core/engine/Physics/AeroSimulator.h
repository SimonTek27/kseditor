#pragma once

/**
 * @file AeroSimulator.h
 * @brief Aerodynamic forces and ground effect simulation
 * @copyright KS Physics Engine
 */

#include "PhysicsCoreTypes.h"
#include <QObject>

namespace ks {
namespace physics {

// ============================================================================
// Aerodynamic Configuration
// ============================================================================

struct AeroConfig {
    double dragCoefficient = 0.35;      ///< Drag coefficient (Cd)
    double frontalArea = 2.0;           ///< Frontal area (m²)
    double liftCoefficientFront = -0.1; ///< Front lift coefficient (negative = downforce)
    double liftCoefficientRear = -0.15; ///< Rear lift coefficient
    double efficiency = 0.85;           ///< Aerodynamic efficiency
    double groundEffectBonus = 1.2;     ///< Ground effect multiplier
};

// ============================================================================
// Wing Configuration
// ============================================================================

struct WingConfigAero {
    double frontAngle = 0.0;            ///< Front wing angle (degrees)
    double rearAngle = 0.0;             ///< Rear wing angle (degrees)
    double frontSpan = 1.0;             ///< Front wing span (m)
    double rearSpan = 1.0;              ///< Rear wing span (m)
    double efficiency = 0.85;           ///< Wing efficiency
};

// ============================================================================
// Diffuser Configuration
// ============================================================================

struct DiffuserConfigAero {
    double expansionAngle = 10.0;       ///< Diffuser expansion angle (degrees)
    double efficiency = 0.7;            ///< Diffuser efficiency
    double groundClearance = 0.05;      ///< Ground clearance (m)
    bool enabled = true;                ///< Diffuser enabled
};

// ============================================================================
// Ground Effect State
// ============================================================================

struct GroundEffectStateAero {
    double rideHeightFront = 0.05;      ///< Front ride height (m)
    double rideHeightRear = 0.07;       ///< Rear ride height (m)
    double porpoisingAmplitude = 0.0;   ///< Porpoising amplitude
    double porpoisingFrequency = 0.0;   ///< Porpoising frequency
    double venturiPressure = 0.0;       ///< Venturi pressure
    double downforceGain = 1.0;         ///< Downforce gain factor
};

// ============================================================================
// Aerodynamic Forces
// ============================================================================

struct AeroForces {
    double drag = 0.0;                  ///< Total drag force (N)
    double downforceFront = 0.0;        ///< Front downforce (N)
    double downforceRear = 0.0;         ///< Rear downforce (N)
    double totalDownforce = 0.0;        ///< Total downforce (N)
    double sideForce = 0.0;             ///< Side force (N)
    double dragCoeff = 0.0;             ///< Effective drag coefficient
    double liftCoeffFront = 0.0;        ///< Effective front lift coefficient
    double liftCoeffRear = 0.0;         ///< Effective rear lift coefficient
};

// ============================================================================
// Aero Simulator Class
// ============================================================================

class AeroSimulator {
public:
    AeroSimulator();
    ~AeroSimulator() = default;

    // Configuration
    void setAeroConfig(const AeroConfig& config);
    void setWingConfig(const WingConfigAero& config);
    void setDiffuserConfig(const DiffuserConfigAero& config);
    
    // State access
    AeroForces currentForces() const { return m_forces; }
    GroundEffectStateAero groundEffectState() const { return m_groundEffect; }
    
    // Update
    void update(double dt, double speed, double rideHeightFront, double rideHeightRear);
    
    // Query
    double dragForce() const { return m_forces.drag; }
    double downforce() const { return m_forces.totalDownforce; }
    double downforceFront() const { return m_forces.downforceFront; }
    double downforceRear() const { return m_forces.downforceRear; }
    
    // DRS
    void setDrsActive(bool active);
    bool drsActive() const { return m_drsActive; }
    double drsDragReduction() const { return m_drsDragReduction; }

private:
    // Internal calculations
    double calculateDrag(double speed, double cd, double area, double airDensity) const;
    double calculateDownforce(double speed, double cl, double area, double airDensity) const;
    double calculateGroundEffect(double rideHeight, double speed) const;
    double calculatePorpoising(double rideHeight, double speed, double dt);
    
    // Configuration
    AeroConfig m_aeroConfig;
    WingConfigAero m_wingConfig;
    DiffuserConfigAero m_diffuserConfig;
    
    // State
    AeroForces m_forces;
    GroundEffectStateAero m_groundEffect;
    bool m_drsActive = false;
    double m_drsDragReduction = 0.35;  ///< 35% drag reduction when DRS active
    double m_porpoisingPhase = 0.0;
    
    // Physics constants
    static constexpr double AIR_DENSITY = 1.225;  ///< kg/m³ at sea level
};

} // namespace physics
} // namespace ks