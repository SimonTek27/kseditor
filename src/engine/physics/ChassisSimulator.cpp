#include "ChassisSimulator.h"
#include <algorithm>
#include <cmath>

namespace ks {
namespace physics {

ChassisSimulator::ChassisSimulator() {
    m_chassisState = ChassisState();
    m_weightTransfer = WeightTransferResultChassis();
}

void ChassisSimulator::setChassisConfig(const ChassisConfig& config) {
    m_chassisConfig = config;
}

void ChassisSimulator::setSuspensionConfig(const SuspensionConfig& config) {
    m_suspensionConfig = config;
}

void ChassisSimulator::update(double dt, double throttle, double brake, double steering,
                              const std::array<double, 4>& tireForces) {
    // Extract tire forces (simplified)
    double frontLateralForce = (tireForces[0] + tireForces[1]) / 2.0;
    double rearLateralForce = (tireForces[2] + tireForces[3]) / 2.0;
    
    // Calculate accelerations
    double totalLateralForce = frontLateralForce + rearLateralForce;
    m_chassisState.lateralAccel = totalLateralForce / m_chassisConfig.mass;
    
    // Calculate yaw acceleration
    double frontLeverArm = m_chassisConfig.frontAxleDist;
    double rearLeverArm = m_chassisConfig.rearAxleDist;
    double yawAccel = calculateYawAccel(frontLateralForce, rearLateralForce,
                                       frontLeverArm, rearLeverArm);
    
    // Update yaw rate
    m_chassisState.yawRate += yawAccel * dt;
    m_chassisState.yawRate = std::clamp(m_chassisState.yawRate, -5.0, 5.0);  // Limit yaw rate
    
    // Update speed (simplified)
    double longitudinalForce = throttle * 5000.0 - brake * 8000.0;  // Simplified
    m_chassisState.longitudinalAccel = longitudinalForce / m_chassisConfig.mass;
    m_chassisState.speed += m_chassisState.longitudinalAccel * dt;
    m_chassisState.speed = std::max(0.0, m_chassisState.speed);
    
    // Calculate sideslip angle
    m_chassisState.sideslipAngle = calculateSideslipAngle(
        m_chassisState.speed,
        m_chassisState.lateralAccel * m_chassisState.speed,  // Approximate lateral velocity
        m_chassisState.yawRate
    );
    
    // Calculate weight transfer
    m_weightTransfer = calculateWeightTransfer(
        m_chassisState.lateralAccel,
        m_chassisState.longitudinalAccel
    );
    
    // Update roll and pitch angles
    m_chassisState.rollAngle = m_weightTransfer.rollAngle;
    m_chassisState.pitchAngle = m_weightTransfer.pitchAngle;
}

WeightTransferResultChassis ChassisSimulator::calculateWeightTransfer(
    double lateralAccel, double longitudinalAccel) const {
    
    WeightTransferResultChassis result;
    
    // Static loads
    double totalWeight = m_chassisConfig.mass * GRAVITY;
    double frontStatic = totalWeight * m_chassisConfig.rearAxleDist / m_chassisConfig.wheelBase;
    double rearStatic = totalWeight * m_chassisConfig.frontAxleDist / m_chassisConfig.wheelBase;
    double leftStatic = totalWeight / 2.0;
    double rightStatic = totalWeight / 2.0;
    
    // Lateral load transfer
    double lateralTransfer = m_chassisConfig.mass * lateralAccel * m_chassisConfig.cgHeight /
                            m_chassisConfig.trackWidth;
    
    // Longitudinal load transfer
    double longitudinalTransfer = m_chassisConfig.mass * longitudinalAccel * 
                                 m_chassisConfig.cgHeight / m_chassisConfig.wheelBase;
    
    // Apply transfers
    result.frontLeftLoad = frontStatic / 2.0 + lateralTransfer / 2.0 - longitudinalTransfer / 2.0;
    result.frontRightLoad = frontStatic / 2.0 - lateralTransfer / 2.0 - longitudinalTransfer / 2.0;
    result.rearLeftLoad = rearStatic / 2.0 + lateralTransfer / 2.0 + longitudinalTransfer / 2.0;
    result.rearRightLoad = rearStatic / 2.0 - lateralTransfer / 2.0 + longitudinalTransfer / 2.0;
    
    // Ensure non-negative loads
    result.frontLeftLoad = std::max(0.0, result.frontLeftLoad);
    result.frontRightLoad = std::max(0.0, result.frontRightLoad);
    result.rearLeftLoad = std::max(0.0, result.rearLeftLoad);
    result.rearRightLoad = std::max(0.0, result.rearRightLoad);
    
    // Calculate roll and pitch angles
    double rollStiffness = m_suspensionConfig.antiRollBarFront + m_suspensionConfig.antiRollBarRear;
    result.rollAngle = lateralTransfer * m_chassisConfig.trackWidth / rollStiffness;
    result.pitchAngle = longitudinalTransfer * m_chassisConfig.wheelBase / 
                       (m_suspensionConfig.springRateFront + m_suspensionConfig.springRateRear);
    
    result.totalLoadTransfer = lateralTransfer + longitudinalTransfer;
    result.lateralLoadTransfer = lateralTransfer;
    result.longitudinalLoadTransfer = longitudinalTransfer;
    
    return result;
}

ChassisSimulator::StabilityDerivatives ChassisSimulator::calculateStabilityDerivatives(
    double frontCorneringStiffness, double rearCorneringStiffness) const {
    
    StabilityDerivatives derivs;
    
    double m = m_chassisConfig.mass;
    double a = m_chassisConfig.frontAxleDist;
    double b = m_chassisConfig.rearAxleDist;
    double L = m_chassisConfig.wheelBase;
    
    // Cornering stiffness derivatives
    double Cf = frontCorneringStiffness;
    double Cr = rearCorneringStiffness;
    
    // Understeer gradient
    derivs.understeerGradient = (a * Cf - b * Cr) / (Cf * Cr * L);
    
    // Yaw velocity gain
    derivs.yawVelocityGain = m_chassisState.speed / (L * (1 + derivs.understeerGradient * 
                             m_chassisState.speed * m_chassisState.speed));
    
    // Sideforce derivatives
    derivs.dFy_dAlpha = -(Cf + Cr);
    
    // Yaw moment derivatives
    derivs.dMz_dAlpha = -(a * Cf - b * Cr);
    derivs.dMz_dR = -(a * a * Cf + b * b * Cr) / m_chassisState.speed;
    
    return derivs;
}

double ChassisSimulator::calculateLateralAccel(double yawRate, double speed) const {
    // Simplified lateral acceleration from yaw rate
    return yawRate * speed;
}

double ChassisSimulator::calculateYawAccel(double frontLateralForce, double rearLateralForce,
                                          double frontLeverArm, double rearLeverArm) const {
    // Yaw moment = front force * front lever arm - rear force * rear lever arm
    double yawMoment = frontLateralForce * frontLeverArm - rearLateralForce * rearLeverArm;
    
    // Yaw acceleration = yaw moment / yaw inertia
    return yawMoment / m_chassisConfig.yawInertia;
}

double ChassisSimulator::calculateSideslipAngle(double vx, double vy, double yawRate) const {
    if (std::abs(vx) < 0.1) return 0.0;
    
    // Sideslip angle = atan(lateral velocity / longitudinal velocity)
    double sideslip = std::atan2(vy, vx);
    
    // Normalize to [-pi, pi]
    while (sideslip > M_PI) sideslip -= 2.0 * M_PI;
    while (sideslip < -M_PI) sideslip += 2.0 * M_PI;
    
    return sideslip;
}

} // namespace physics
} // namespace ks