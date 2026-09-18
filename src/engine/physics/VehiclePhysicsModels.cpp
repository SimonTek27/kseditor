#include "VehiclePhysics.h"
#include "PhysicsEngine.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <QDebug>
#include <QtMath>
#include <QRandomGenerator>

namespace ks {
namespace physics {

// ============================================================================
// TireTransientModel implementation
// ============================================================================

void TireTransientModel::update(float dt, float forwardSpeed,
                                 float wheelAngularVelocity, float wheelRadius,
                                 float slipAngle, float slipRatio,
                                 float tireTemp, float normalForce)
{
    float sigmaAlpha = calculateRelaxationLengthLateral(normalForce);
    float sigmaKappa = calculateRelaxationLengthLongitudinal(normalForce);

    m_state.relaxationLengthLateral = sigmaAlpha;
    m_state.relaxationLengthLongitudinal = sigmaKappa;

    float tauAlpha = (forwardSpeed > 0.1f) ? sigmaAlpha / forwardSpeed : 0.01f;
    float tauKappa = (forwardSpeed > 0.1f) ? sigmaKappa / forwardSpeed : 0.01f;

    float alphaError = slipAngle - m_state.slipAngleFiltered;
    m_state.slipAngleFiltered += (dt / tauAlpha) * alphaError;

    float kappaError = slipRatio - m_state.slipRatioFiltered;
    m_state.slipRatioFiltered += (dt / tauKappa) * kappaError;

    m_state.tireWarmUpFactor = calculateWarmUpFactor(tireTemp, m_optimalTemp);

    m_state.carcassDeflection = m_state.slipAngleFiltered * sigmaAlpha;

    m_state.lateralForceFilter = m_state.slipAngleFiltered;
    m_state.longitudinalForceFilter = m_state.slipRatioFiltered;
}

float TireTransientModel::calculateRelaxationLengthLateral(float normalForce) const
{
    float loadRatio = normalForce / 5000.0f;
    return m_relaxationBase * std::sqrt(std::max(loadRatio, 0.1f));
}

float TireTransientModel::calculateRelaxationLengthLongitudinal(float normalForce) const
{
    float loadRatio = normalForce / 5000.0f;
    return m_relaxationBase * 0.8f * std::sqrt(std::max(loadRatio, 0.1f));
}

float TireTransientModel::calculateWarmUpFactor(float tireTemp, float optimalTemp) const
{
    float tempDiff = tireTemp - optimalTemp;
    float warmUp = 1.0f - std::exp(-std::abs(tempDiff) / 30.0f);

    if (tireTemp < optimalTemp) {
        return warmUp;
    } else {
        return 1.0f - 0.3f * warmUp;
    }
}

// ============================================================================
// VehicleDynamicsModel implementation
// ============================================================================

StabilityDerivatives VehicleDynamicsModel::calculateStabilityDerivatives(
    float mass, float wheelBase, float frontAxleDist, float rearAxleDist,
    float frontCorneringStiffness, float rearCorneringStiffness,
    float cgHeight, float rollStiffnessFront, float rollStiffnessRear
) const {
    StabilityDerivatives derivs;

    derivs.corneringStiffnessFront = frontCorneringStiffness;
    derivs.corneringStiffnessRear = rearCorneringStiffness;

    derivs.dFy_dAlpha = frontCorneringStiffness + rearCorneringStiffness;
    derivs.dFy_dKappa = 0.0f;
    derivs.dFy_dGamma = 0.0f;

    derivs.understeerGradient = calculateUndersteerGradient(
        frontCorneringStiffness, rearCorneringStiffness,
        mass, wheelBase, frontAxleDist
    );

    derivs.yawInertia = mass * wheelBase * wheelBase / 12.0f;

    float frontLeverArm = frontAxleDist;
    float rearLeverArm = rearAxleDist;
    derivs.dMz_dAlpha = -(
        frontCorneringStiffness * frontLeverArm -
        rearCorneringStiffness * rearLeverArm
    );

    derivs.dMz_dR = -(
        frontCorneringStiffness * frontLeverArm * frontLeverArm +
        rearCorneringStiffness * rearLeverArm * rearLeverArm
    ) / (mass > 0.0f ? mass : 1500.0f);

    derivs.yawTimeConstant = (derivs.dMz_dR != 0.0f) ?
        derivs.yawInertia / (-derivs.dMz_dR) : 1.0f;
    derivs.lateralTimeConstant = (mass > 0.0f) ?
        derivs.dFy_dAlpha / mass : 1.0f;

    derivs.controlAuthority = derivs.dMz_dAlpha * wheelBase;

    return derivs;
}

MomentMethodResult VehicleDynamicsModel::calculateMomentMethod(
    float slipAngle, float yawRate, float speed,
    const StabilityDerivatives& derivs
) const {
    MomentMethodResult result;

    result.sideForce = derivs.dFy_dAlpha * slipAngle;
    result.yawMoment = derivs.dMz_dAlpha * slipAngle + derivs.dMz_dR * yawRate;

    float neutralPoint = derivs.dMz_dAlpha / derivs.dFy_dAlpha;
    result.staticMargin = neutralPoint;
    result.isStable = result.staticMargin > 0.0f;

    return result;
}

std::vector<MomentMethodResult> VehicleDynamicsModel::generateMomentMethodDiagram(
    float speed, float maxSlipAngle, int numPoints,
    const StabilityDerivatives& derivs
) const {
    std::vector<MomentMethodResult> diagram;
    diagram.reserve(numPoints);

    for (int i = 0; i < numPoints; ++i) {
        float slipAngle = -maxSlipAngle + (2.0f * maxSlipAngle * i / (numPoints - 1));
        float yawRate = 0.0f;
        diagram.push_back(calculateMomentMethod(slipAngle, yawRate, speed, derivs));
    }

    return diagram;
}

float VehicleDynamicsModel::calculateUndersteerGradient(
    float frontCorneringStiffness, float rearCorneringStiffness,
    float mass, float wheelBase, float frontAxleDist
) const {
    float rearAxleDist = wheelBase - frontAxleDist;
    float gravity = 9.81f;
    float frontWeight = mass * gravity * rearAxleDist / wheelBase;
    float rearWeight = mass * gravity * frontAxleDist / wheelBase;

    if (frontCorneringStiffness == 0.0f || rearCorneringStiffness == 0.0f)
        return 0.0f;

    return (frontWeight / frontCorneringStiffness) -
           (rearWeight / rearCorneringStiffness);
}

float VehicleDynamicsModel::calculateYawVelocityGain(
    float speed, const StabilityDerivatives& derivs
) const {
    float L = derivs.corneringStiffnessFront + derivs.corneringStiffnessRear;
    if (L == 0.0f) return 0.0f;

    float wheelBase = 2.7f;
    float K = derivs.understeerGradient;
    float denominator = wheelBase + K * speed * speed;

    return (denominator != 0.0f) ? speed / denominator : 0.0f;
}

float VehicleDynamicsModel::calculateCriticalSpeed(
    const StabilityDerivatives& derivs
) const {
    float K = derivs.understeerGradient;
    if (K >= 0.0f) return 1000.0f;

    float wheelBase = 2.7f;
    return std::sqrt(-wheelBase / K);
}

// ============================================================================
// WeightTransferModel implementation
// ============================================================================

WeightTransferResult WeightTransferModel::calculate(
    float mass, float wheelBase, float trackWidth,
    float frontAxleDist, float rearAxleDist,
    float cgHeight, float lateralAccel, float longitudinalAccel,
    const SuspensionGeometry& geometry,
    float frontRollStiffness, float rearRollStiffness,
    float totalRollStiffness
) const {
    WeightTransferResult result;

    float gravity = 9.81f;
    float staticFrontLoad = mass * gravity * rearAxleDist / wheelBase;
    float staticRearLoad = mass * gravity * frontAxleDist / wheelBase;

    result.rollGradient = calculateRollGradient(
        mass, cgHeight, trackWidth, totalRollStiffness
    );

    result.rollAngle = result.rollGradient * lateralAccel;

    result.lateralLoadTransferFront = calculateLateralTransfer(
        mass, lateralAccel, cgHeight, trackWidth,
        frontAxleDist, wheelBase, frontRollStiffness,
        geometry.frontRollCenterHeight, totalRollStiffness
    );

    result.lateralLoadTransferRear = calculateLateralTransfer(
        mass, lateralAccel, cgHeight, trackWidth,
        rearAxleDist, wheelBase, rearRollStiffness,
        geometry.rearRollCenterHeight, totalRollStiffness
    );

    float antiDiveFactor = geometry.antiDivePercent / 100.0f;
    result.longitudinalLoadTransfer = mass * gravity * longitudinalAccel * cgHeight / wheelBase;
    float antiDiveReduction = result.longitudinalLoadTransfer * antiDiveFactor;
    result.longitudinalLoadTransfer -= antiDiveReduction;

    result.frontLeftLoad = staticFrontLoad * 0.5f
        + result.lateralLoadTransferFront * 0.5f
        + result.longitudinalLoadTransfer * 0.5f;
    result.frontRightLoad = staticFrontLoad * 0.5f
        - result.lateralLoadTransferFront * 0.5f
        + result.longitudinalLoadTransfer * 0.5f;
    result.rearLeftLoad = staticRearLoad * 0.5f
        + result.lateralLoadTransferRear * 0.5f
        - result.longitudinalLoadTransfer * 0.5f;
    result.rearRightLoad = staticRearLoad * 0.5f
        - result.lateralLoadTransferRear * 0.5f
        - result.longitudinalLoadTransfer * 0.5f;

    result.frontLeftLoad = std::max(result.frontLeftLoad, 0.0f);
    result.frontRightLoad = std::max(result.frontRightLoad, 0.0f);
    result.rearLeftLoad = std::max(result.rearLeftLoad, 0.0f);
    result.rearRightLoad = std::max(result.rearRightLoad, 0.0f);

    return result;
}

float WeightTransferModel::calculateRollCenterHeight(
    float armLength, float armAngle
) const {
    return armLength * std::sin(armAngle);
}

float WeightTransferModel::calculateAntiDivePercent(
    float frontInstantCenterHeight, float cgHeight,
    float frontAxleDist, float wheelBase
) const {
    if (cgHeight == 0.0f || wheelBase == 0.0f) return 0.0f;
    return (frontInstantCenterHeight / cgHeight) *
           (frontAxleDist / wheelBase) * 100.0f;
}

float WeightTransferModel::calculateAntiSquatPercent(
    float rearInstantCenterHeight, float cgHeight,
    float rearAxleDist, float wheelBase
) const {
    if (cgHeight == 0.0f || wheelBase == 0.0f) return 0.0f;
    return (rearInstantCenterHeight / cgHeight) *
           (rearAxleDist / wheelBase) * 100.0f;
}

float WeightTransferModel::calculateRollGradient(
    float mass, float cgHeight, float trackWidth,
    float totalRollStiffness
) const {
    float gravity = 9.81f;
    float rollMoment = mass * gravity * cgHeight;
    float rollStiffnessTerm = totalRollStiffness - rollMoment;

    return (rollStiffnessTerm != 0.0f) ?
        rollMoment / rollStiffnessTerm : 0.0f;
}

float WeightTransferModel::calculateLateralTransfer(
    float mass, float lateralAccel, float cgHeight,
    float trackWidth, float axleDist, float wheelBase,
    float rollStiffness, float rollCenterHeight,
    float totalRollStiffness
) const {
    float gravity = 9.81f;
    float weight = mass * gravity;

    float geometricTransfer = weight * lateralAccel * (cgHeight - rollCenterHeight) /
                              (trackWidth * wheelBase) * axleDist;

    float rollStiffnessFraction = (totalRollStiffness > 0.0f) ?
        rollStiffness / totalRollStiffness : 0.0f;
    float rollMoment = weight * lateralAccel * (cgHeight - rollCenterHeight);
    float stiffnessTransfer = rollStiffnessFraction * rollMoment / trackWidth;

    return geometricTransfer + stiffnessTransfer;
}

// ============================================================================
// GGDiagram implementation
// ============================================================================

void GGDiagram::calculate(float speed, float downforce, float mass,
                           float lateralGMax, float brakingGMax, float accelGMax)
{
    m_limits.maxLateralG = lateralGMax;
    m_limits.maxBrakingG = brakingGMax;
    m_limits.maxAccelerationG = accelGMax;

    float gravity = 9.81f;
    float normalForce = mass * gravity + downforce;
    float maxFrictionForce = normalForce;

    float latForceAtSpeed = lateralGMax * normalForce;
    float longForceAtSpeed = brakingGMax * normalForce;

    m_limits.frictionEllipseRatio = (latForceAtSpeed > 0.0f) ?
        longForceAtSpeed / latForceAtSpeed : 0.9f;

    m_envelope.clear();
    int numPoints = 64;
    m_envelope.reserve(numPoints);

    for (int i = 0; i < numPoints; ++i) {
        float angle = 2.0f * 3.14159265f * i / numPoints;

        float radius = calculateEllipseRadius(angle);

        GGPoint point;
        point.lateralG = radius * std::cos(angle);
        point.longitudinalG = radius * std::sin(angle);
        point.speed = speed;

        m_envelope.push_back(point);
    }
}

std::vector<GGPoint> GGDiagram::getEnvelope(int numPoints) const
{
    if (static_cast<int>(m_envelope.size()) == numPoints) {
        return m_envelope;
    }

    std::vector<GGPoint> resampled;
    resampled.reserve(numPoints);

    for (int i = 0; i < numPoints; ++i) {
        float angle = 2.0f * 3.14159265f * i / numPoints;
        float radius = calculateEllipseRadius(angle);

        GGPoint point;
        point.lateralG = radius * std::cos(angle);
        point.longitudinalG = radius * std::sin(angle);
        point.speed = m_envelope.empty() ? 0.0f : m_envelope[0].speed;

        resampled.push_back(point);
    }

    return resampled;
}

bool GGDiagram::isInEnvelope(float lateralG, float longitudinalG) const
{
    float latRatio = lateralG / m_limits.maxLateralG;
    float longRatio = 0.0f;

    if (longitudinalG >= 0.0f) {
        longRatio = longitudinalG / m_limits.maxAccelerationG;
    } else {
        longRatio = longitudinalG / m_limits.maxBrakingG;
    }

    float ellipseValue = latRatio * latRatio + longRatio * longRatio;
    return ellipseValue <= 1.0f;
}

float GGDiagram::getMaximumLateralG(float longitudinalG) const
{
    float longRatio = 0.0f;
    if (longitudinalG >= 0.0f) {
        longRatio = longitudinalG / m_limits.maxAccelerationG;
    } else {
        longRatio = longitudinalG / m_limits.maxBrakingG;
    }

    float remaining = 1.0f - longRatio * longRatio;
    if (remaining <= 0.0f) return 0.0f;

    return m_limits.maxLateralG * std::sqrt(remaining);
}

float GGDiagram::getMaximumLongitudinalG(float lateralG) const
{
    float latRatio = lateralG / m_limits.maxLateralG;
    float remaining = 1.0f - latRatio * latRatio;
    if (remaining <= 0.0f) return 0.0f;

    return m_limits.maxBrakingG * std::sqrt(remaining);
}

float GGDiagram::calculateEllipseRadius(float angle) const
{
    float cosA = std::cos(angle);
    float sinA = std::sin(angle);

    float a = m_limits.maxLateralG;
    float b = (sinA >= 0.0f) ? m_limits.maxAccelerationG : m_limits.maxBrakingG;

    if (a == 0.0f || b == 0.0f) return 0.0f;

    float radius = (a * b) / std::sqrt(b * b * cosA * cosA + a * a * sinA * sinA);
    return radius;
}

// ============================================================================
// SuspensionGeometryModel implementation
// ============================================================================

CamberResult SuspensionGeometryModel::calculateCamber(
    float staticCamber, float compression, float camberGain,
    float rollAngle, float rollCamberCoefficient
) const {
    CamberResult result;

    result.staticCamber = staticCamber;
    result.camberGain = camberGain;

    result.dynamicCamber = staticCamber + camberGain * compression;

    result.rollCamber = rollAngle * rollCamberCoefficient;

    result.dynamicCamber += result.rollCamber;

    return result;
}

RollCenterMigration SuspensionGeometryModel::calculateRollCenterMigration(
    float frontCompression, float rearCompression,
    float frontRollCenterBase, float rearRollCenterBase,
    float frontLateralMigrationRate, float rearLateralMigrationRate
) const {
    RollCenterMigration result;

    result.frontRollCenterHeight = frontRollCenterBase +
        frontCompression * 0.01f;

    result.rearRollCenterHeight = rearRollCenterBase +
        rearCompression * 0.01f;

    float avgCompression = (frontCompression + rearCompression) * 0.5f;
    result.rollCenterLateralOffset = avgCompression * frontLateralMigrationRate;

    return result;
}

float SuspensionGeometryModel::calculateMotionRatio(
    float wheelTravel, float springTravel
) const {
    if (wheelTravel == 0.0f) return 1.0f;
    return springTravel / wheelTravel;
}

float SuspensionGeometryModel::calculateRollCenterHeight(
    float upperArmLength, float lowerArmLength,
    float upperArmAngle, float lowerArmAngle,
    float trackWidth
) const {
    if (trackWidth == 0.0f) return 0.0f;

    float upperArmHorizontal = upperArmLength * std::cos(upperArmAngle);
    float lowerArmHorizontal = lowerArmLength * std::cos(lowerArmAngle);

    float upperArmVertical = upperArmLength * std::sin(upperArmAngle);
    float lowerArmVertical = lowerArmLength * std::sin(lowerArmAngle);

    float instantCenterHeight = 0.0f;
    if (upperArmHorizontal != lowerArmHorizontal) {
        float ratio = lowerArmHorizontal / (upperArmHorizontal - lowerArmHorizontal);
        instantCenterHeight = lowerArmVertical + ratio * (upperArmVertical - lowerArmVertical);
    }

    float rollCenterHeight = instantCenterHeight * 0.5f;

    return rollCenterHeight;
}

float SuspensionGeometryModel::calculateCamberGainFromGeometry(
    float upperArmLength, float lowerArmLength,
    float upperArmAngle, float lowerArmAngle
) const {
    float upperArmHorizontal = upperArmLength * std::cos(upperArmAngle);
    float lowerArmHorizontal = lowerArmLength * std::cos(lowerArmAngle);

    if (lowerArmHorizontal == 0.0f) return 0.0f;

    float gain = (upperArmHorizontal - lowerArmHorizontal) / lowerArmHorizontal;
    return gain * 0.5f;
}

// ============================================================================
// AdvancedAeroModel implementation
// ============================================================================

AeroForcesAdvanced AdvancedAeroModel::calculate(
    float speed, float airDensity,
    const GroundEffectState& groundEffect,
    const DiffuserConfig& diffuser,
    const WingConfig& frontWing,
    const WingConfig& rearWing
) const {
    AeroForcesAdvanced forces;

    float q = calculateDynamicPressure(speed, airDensity);

    // Front wing
    float frontWingDown = calculateWingDownforce(frontWing, speed, airDensity);
    float frontWingDrag = calculateWingDrag(frontWing, speed, airDensity);

    // Rear wing
    float rearWingDown = calculateWingDownforce(rearWing, speed, airDensity);
    float rearWingDrag = calculateWingDrag(rearWing, speed, airDensity);

    forces.wingContribution = frontWingDown + rearWingDown;
    forces.frontDownforce = frontWingDown;
    forces.rearDownforce = rearWingDown;

    // Diffuser
    forces.diffuserContribution = calculateDiffuserDownforce(
        diffuser, speed, airDensity, groundEffect.rideHeightRear
    );

    // Ground effect bonus
    float avgRideHeight = (groundEffect.rideHeightFront + groundEffect.rideHeightRear) * 0.5f;
    forces.groundEffectBonus = calculateGroundEffectBonus(
        avgRideHeight, speed, airDensity
    );

    // Total downforce
    forces.totalDownforce = forces.wingContribution +
                            forces.diffuserContribution +
                            forces.groundEffectBonus;

    // Induced drag from downforce
    float avgSpan = (frontWing.span + rearWing.span) * 0.5f;
    forces.inducedDrag = calculateInducedDrag(
        forces.totalDownforce, avgSpan, airDensity, speed
    );

    // Parasitic drag (base drag minus wing contribution)
    forces.parasiticDrag = frontWingDrag + rearWingDrag;

    // Total drag
    forces.totalDrag = forces.parasiticDrag + forces.inducedDrag;

    // Aero balance
    float totalFront = forces.frontDownforce + forces.groundEffectBonus * 0.4f;
    float totalRear = forces.rearDownforce + forces.diffuserContribution + forces.groundEffectBonus * 0.6f;
    forces.aeroBalance = (totalFront + totalRear > 0.0f) ?
        totalFront / (totalFront + totalRear) : 0.5f;

    // L/D ratio
    forces.ldRatio = (forces.totalDrag > 0.0f) ?
        forces.totalDownforce / forces.totalDrag : 0.0f;

    return forces;
}

float AdvancedAeroModel::calculateWingDownforce(
    const WingConfig& wing, float speed, float airDensity
) const {
    float q = calculateDynamicPressure(speed, airDensity);

    float effectiveAoA = wing.angleOfAttack + wing.flapAngle;

    float clBase = 2.0f * 3.14159265f * effectiveAoA * 3.14159265f / 180.0f;
    clBase = std::min(clBase, 2.5f);

    if (wing.hasGurneyFlap) {
        clBase *= 1.15f;
    }

    float endplateEffect = 1.0f + wing.endplateSize / wing.span * 0.2f;
    clBase *= endplateEffect;

    float reynoldsFactor = calculateReynoldsCorrection(speed, wing.chord);
    clBase *= reynoldsFactor;

    float area = wing.span * wing.chord;
    float downforce = q * clBase * area;

    return downforce;
}

float AdvancedAeroModel::calculateWingDrag(
    const WingConfig& wing, float speed, float airDensity
) const {
    float q = calculateDynamicPressure(speed, airDensity);

    float effectiveAoA = wing.angleOfAttack + wing.flapAngle;
    float cdi = 0.01f + 0.001f * effectiveAoA * effectiveAoA;

    float area = wing.span * wing.chord;
    float drag = q * cdi * area;

    return drag;
}

float AdvancedAeroModel::calculateDiffuserDownforce(
    const DiffuserConfig& diffuser, float speed, float airDensity,
    float rideHeight
) const {
    float q = calculateDynamicPressure(speed, airDensity);

    float expansionEfficiency = diffuser.expansionRatio / 4.0f;
    expansionEfficiency = std::clamp(expansionEfficiency, 0.0f, 1.0f);

    float venturiEffect = 1.0f / (1.0f + rideHeight / diffuser.length);
    venturiEffect = std::clamp(venturiEffect, 0.3f, 1.5f);

    float clDiffuser = 2.5f * expansionEfficiency * venturiEffect;
    clDiffuser *= (1.0f + diffuser.exitAngle / 45.0f);

    float downforce = q * clDiffuser * diffuser.exitArea;

    return downforce;
}

float AdvancedAeroModel::calculateGroundEffectBonus(
    float rideHeight, float speed, float airDensity
) const {
    float q = calculateDynamicPressure(speed, airDensity);

    float referenceHeight = 0.05f;
    float heightRatio = referenceHeight / std::max(rideHeight, 0.01f);
    heightRatio = std::clamp(heightRatio, 0.5f, 3.0f);

    float clGroundEffect = 1.5f * heightRatio;
    clGroundEffect *= (1.0f + speed * 0.001f);

    float referenceArea = 2.0f;
    float bonus = q * clGroundEffect * referenceArea;

    return bonus;
}

float AdvancedAeroModel::calculateInducedDrag(
    float downforce, float span, float airDensity, float speed
) const {
    if (speed < 1.0f || span < 0.1f) return 0.0f;

    float aspectRatio = span * span / 2.0f;
    float arFactor = 1.0f / (3.14159265f * aspectRatio);

    float inducedCd = arFactor * (downforce / (0.5f * airDensity * speed * speed * 2.0f));
    inducedCd *= inducedCd;

    float q = calculateDynamicPressure(speed, airDensity);
    float inducedDrag = q * inducedCd * 2.0f;

    return inducedDrag;
}

float AdvancedAeroModel::calculatePorpoisingThreshold(
    const DiffuserConfig& diffuser, float speed
) const {
    float frequency = speed / diffuser.length * 0.5f;
    float criticalSpeed = std::sqrt(diffuser.exitArea * 9.81f / diffuser.expansionRatio);

    return criticalSpeed;
}

float AdvancedAeroModel::calculateDynamicPressure(float speed, float airDensity) const {
    return 0.5f * airDensity * speed * speed;
}

float AdvancedAeroModel::calculateReynoldsCorrection(float speed, float chord) const {
    float viscosity = 1.5e-5f;
    float reynolds = speed * chord / viscosity;

    float correction = 1.0f;
    if (reynolds > 1e6f) {
        correction = 1.02f;
    } else if (reynolds < 1e5f) {
        correction = 0.95f;
    }

    return correction;
}

// ============================================================================
// PairAnalysisModel implementation
// ============================================================================

PairAnalysisResult PairAnalysisModel::analyze(
    const AxlePair& pair,
    float wheelBase, float mass, float speed
) const {
    PairAnalysisResult result;

    // Calculate slip angle ratio
    result.frontSlipRatio = (pair.frontCorneringStiffness > 0.0f) ?
        pair.frontSlipAngle / (pair.frontCorneringStiffness * 0.0001f) : 0.0f;
    result.rearSlipRatio = (pair.rearCorneringStiffness > 0.0f) ?
        pair.rearSlipAngle / (pair.rearCorneringStiffness * 0.0001f) : 0.0f;

    // Understeer gradient from slip difference
    float slipDifference = result.frontSlipRatio - result.rearSlipRatio;
    result.understeerGradient = slipDifference * 100.0f;

    // Balance factor
    float totalForce = pair.frontLateralForce + pair.rearLateralForce;
    result.balanceFactor = (totalForce > 0.0f) ?
        pair.frontLateralForce / totalForce : 0.5f;

    // Side force
    result.sideForce = pair.frontLateralForce + pair.rearLateralForce;

    // Yaw moment
    float frontAxleDist = wheelBase * 0.5f;
    float rearAxleDist = wheelBase * 0.5f;
    result.yawMoment = calculateYawMomentFromSlip(
        pair.frontSlipAngle, pair.rearSlipAngle,
        pair.frontLateralForce, pair.rearLateralForce,
        frontAxleDist, rearAxleDist
    );

    // Stability classification
    float stabilityIndex = calculateStabilityIndex(
        pair.frontCorneringStiffness, pair.rearCorneringStiffness,
        frontAxleDist, rearAxleDist
    );

    result.isUndersteer = stabilityIndex > 0.1f;
    result.isOversteer = stabilityIndex < -0.1f;
    result.isNeutral = std::abs(stabilityIndex) <= 0.1f;

    // Critical slip angle
    result.criticalSlipAngle = calculateNeutralSteerSpeed(
        pair.frontCorneringStiffness, pair.rearCorneringStiffness,
        wheelBase
    );

    // Peak lateral G
    float gravity = 9.81f;
    result.peakLateralG = result.sideForce / (mass * gravity);

    return result;
}

PairSensitivity PairAnalysisModel::calculateSensitivity(
    const AxlePair& pair,
    float wheelBase, float mass
) const {
    PairSensitivity sens;

    // Front axle sensitivity
    sens.dFyFront_dAlpha = pair.frontCorneringStiffness;

    // Rear axle sensitivity
    sens.dFyRear_dAlpha = pair.rearCorneringStiffness;

    // Yaw moment sensitivity
    float frontAxleDist = wheelBase * 0.5f;
    float rearAxleDist = wheelBase * 0.5f;
    sens.dMz_dAlpha = pair.frontCorneringStiffness * frontAxleDist -
                      pair.rearCorneringStiffness * rearAxleDist;

    // Side slip sensitivity
    sens.dBeta_dAlpha = (pair.frontCorneringStiffness + pair.rearCorneringStiffness) /
                        (mass > 0.0f ? mass : 1500.0f);

    // Yaw rate sensitivity
    sens.dR_dAlpha = sens.dMz_dAlpha / (mass > 0.0f ? mass * wheelBase * wheelBase / 12.0f : 1000.0f);

    return sens;
}

float PairAnalysisModel::calculateSlipAngleDistribution(
    float frontSlipAngle, float rearSlipAngle,
    float frontCorneringStiffness, float rearCorneringStiffness
) const {
    if (frontCorneringStiffness + rearCorneringStiffness == 0.0f)
        return 0.5f;

    float frontForce = frontSlipAngle * frontCorneringStiffness;
    float rearForce = rearSlipAngle * rearCorneringStiffness;
    float totalForce = frontForce + rearForce;

    return (totalForce > 0.0f) ? frontForce / totalForce : 0.5f;
}

float PairAnalysisModel::calculateStabilityIndex(
    float frontCorneringStiffness, float rearCorneringStiffness,
    float frontAxleDist, float rearAxleDist
) const {
    if (frontCorneringStiffness == 0.0f || rearCorneringStiffness == 0.0f)
        return 0.0f;

    float index = (frontAxleDist / frontCorneringStiffness) -
                  (rearAxleDist / rearCorneringStiffness);

    return index;
}

float PairAnalysisModel::calculateNeutralSteerSpeed(
    float frontCorneringStiffness, float rearCorneringStiffness,
    float wheelBase
) const {
    float gravity = 9.81f;
    float mass = 1500.0f;

    float frontWeight = mass * gravity * 0.5f;
    float rearWeight = mass * gravity * 0.5f;

    if (frontCorneringStiffness == 0.0f || rearCorneringStiffness == 0.0f)
        return 100.0f;

    float K = (frontWeight / frontCorneringStiffness) -
              (rearWeight / rearCorneringStiffness);

    if (K >= 0.0f) return 1000.0f;

    return std::sqrt(-wheelBase / K);
}

float PairAnalysisModel::calculateMaximumCorneringSpeed(
    float cornerRadius, float maxLateralG
) const {
    float gravity = 9.81f;
    return std::sqrt(maxLateralG * gravity * cornerRadius);
}

float PairAnalysisModel::calculateYawMomentFromSlip(
    float frontSlipAngle, float rearSlipAngle,
    float frontLateralForce, float rearLateralForce,
    float frontAxleDist, float rearAxleDist
) const {
    float yawMoment = frontLateralForce * frontAxleDist -
                      rearLateralForce * rearAxleDist;

    return yawMoment;
}

// ============================================================================
// DrivingConditionsModel implementation
// ============================================================================

ConditionState DrivingConditionsModel::evaluate(
    float precipitation, float temperature, float windSpeed, float windAngle
) const {
    ConditionState state;

    state.ambientTemperature = temperature;
    state.windSpeed = windSpeed;
    state.windAngle = windAngle;

    if (precipitation <= 0.0f) {
        state.condition = DrivingCondition::Dry;
        state.gripCoefficient = 1.0f;
        state.waterDepth = 0.0f;
        state.visibility = 1.0f;
    } else if (precipitation < 2.0f) {
        state.condition = DrivingCondition::LightRain;
        state.gripCoefficient = 0.85f;
        state.waterDepth = 0.001f;
        state.visibility = 0.8f;
    } else if (precipitation < 5.0f) {
        state.condition = DrivingCondition::HeavyRain;
        state.gripCoefficient = 0.7f;
        state.waterDepth = 0.003f;
        state.visibility = 0.6f;
    } else {
        state.condition = DrivingCondition::StandingWater;
        state.gripCoefficient = 0.5f;
        state.waterDepth = 0.008f;
        state.visibility = 0.4f;
    }

    if (temperature < -2.0f) {
        state.condition = DrivingCondition::Ice;
        state.gripCoefficient = 0.3f;
        state.visibility = 0.7f;
    }

    state.trackTemperature = temperature + 5.0f;
    state.hydroplaningRisk = state.waterDepth > 0.005f;

    return state;
}

GripModifiers DrivingConditionsModel::calculateGrip(
    const ConditionState& condition,
    float tireTemp, float tireCompound
) const {
    GripModifiers modifiers;

    modifiers.surfaceGrip = calculateSurfaceGrip(condition.condition);

    modifiers.temperatureEffect = calculateTemperatureEffect(
        condition.trackTemperature, 80.0f
    );

    modifiers.wetnessEffect = 1.0f - condition.waterDepth * 50.0f;
    modifiers.wetnessEffect = std::clamp(modifiers.wetnessEffect, 0.3f, 1.0f);

    // Tire compound effect (0=hard, 1=soft)
    modifiers.tireCompoundEffect = 0.85f + tireCompound * 0.15f;

    modifiers.totalGrip = modifiers.surfaceGrip *
                          modifiers.temperatureEffect *
                          modifiers.wetnessEffect *
                          modifiers.tireCompoundEffect;

    // Different grip for different maneuvers
    modifiers.brakingGrip = modifiers.totalGrip * 0.95f;
    modifiers.corneringGrip = modifiers.totalGrip * 1.0f;
    modifiers.accelerationGrip = modifiers.totalGrip * 0.9f;

    return modifiers;
}

ConditionEffect DrivingConditionsModel::calculateEffect(
    const ConditionState& condition,
    float speed, float referenceGrip
) const {
    ConditionEffect effect;

    float gripRatio = condition.gripCoefficient / referenceGrip;

    effect.speedReduction = speed * (1.0f - gripRatio) * 0.5f;
    effect.corneringSpeedReduction = speed * (1.0f - gripRatio) * 0.7f;

    effect.brakingDistanceIncrease = (1.0f / gripRatio - 1.0f) * 100.0f;

    effect.tireWearIncrease = (1.0f - gripRatio) * 50.0f;

    effect.riskLevel = 1.0f - gripRatio;
    effect.riskLevel = std::clamp(effect.riskLevel, 0.0f, 1.0f);

    if (condition.condition == DrivingCondition::Ice) {
        effect.recommendedTireCompound = "WINTER";
        effect.drivingAdvice = "Extreme caution - black ice possible";
    } else if (condition.condition == DrivingCondition::StandingWater) {
        effect.recommendedTireCompound = "WET";
        effect.drivingAdvice = "Risk of hydroplaning - reduce speed";
    } else if (condition.condition == DrivingCondition::HeavyRain) {
        effect.recommendedTireCompound = "WET";
        effect.drivingAdvice = "Reduced visibility and grip";
    } else if (condition.condition == DrivingCondition::LightRain) {
        effect.recommendedTireCompound = "INTERMEDIATE";
        effect.drivingAdvice = "Drying line may be faster";
    } else {
        effect.recommendedTireCompound = "DRY";
        effect.drivingAdvice = "Optimal conditions";
    }

    return effect;
}

float DrivingConditionsModel::calculateHydroplaningRisk(
    float waterDepth, float tireDepth, float speed
) const {
    if (waterDepth < 0.001f) return 0.0f;

    float effectiveDepth = waterDepth - tireDepth * 0.008f;
    effectiveDepth = std::max(effectiveDepth, 0.0f);

    float hydroplaningSpeed = std::sqrt(9.81f * effectiveDepth * 10.0f);
    float risk = speed / hydroplaningSpeed;

    return std::clamp(risk, 0.0f, 1.0f);
}

float DrivingConditionsModel::calculateGripFromWaterDepth(float waterDepth) const {
    float grip = 1.0f - waterDepth * 40.0f;
    return std::clamp(grip, 0.3f, 1.0f);
}

float DrivingConditionsModel::calculateGripFromTemperature(float trackTemp) const {
    float optimalTemp = 40.0f;
    float tempDiff = trackTemp - optimalTemp;
    float grip = 1.0f - 0.001f * tempDiff * tempDiff;
    return std::clamp(grip, 0.6f, 1.0f);
}

float DrivingConditionsModel::calculateWindEffect(
    float windSpeed, float windAngle, float carSpeed
) const {
    if (carSpeed < 1.0f) return 0.0f;

    float headwind = windSpeed * std::cos(windAngle);
    float crosswind = windSpeed * std::sin(windAngle);

    float dragEffect = headwind / carSpeed * 0.5f;
    float lateralEffect = crosswind / carSpeed * 0.3f;

    return std::abs(dragEffect) + std::abs(lateralEffect);
}

QString DrivingConditionsModel::getConditionName(DrivingCondition condition) const {
    switch (condition) {
        case DrivingCondition::Dry: return "Dry";
        case DrivingCondition::Wet: return "Wet";
        case DrivingCondition::LightRain: return "Light Rain";
        case DrivingCondition::HeavyRain: return "Heavy Rain";
        case DrivingCondition::StandingWater: return "Standing Water";
        case DrivingCondition::Ice: return "Ice";
        case DrivingCondition::Snow: return "Snow";
        case DrivingCondition::Gravel: return "Gravel";
        case DrivingCondition::Mixed: return "Mixed";
    }
    return "Unknown";
}

float DrivingConditionsModel::calculateSurfaceGrip(DrivingCondition condition) const {
    switch (condition) {
        case DrivingCondition::Dry: return 1.0f;
        case DrivingCondition::Wet: return 0.85f;
        case DrivingCondition::LightRain: return 0.75f;
        case DrivingCondition::HeavyRain: return 0.6f;
        case DrivingCondition::StandingWater: return 0.45f;
        case DrivingCondition::Ice: return 0.25f;
        case DrivingCondition::Snow: return 0.35f;
        case DrivingCondition::Gravel: return 0.7f;
        case DrivingCondition::Mixed: return 0.65f;
    }
    return 1.0f;
}

float DrivingConditionsModel::calculateTemperatureEffect(float trackTemp, float optimalTemp) const {
    float tempDiff = trackTemp - optimalTemp;
    float effect = 1.0f - 0.001f * tempDiff * tempDiff;
    return std::clamp(effect, 0.5f, 1.0f);
}

// ============================================================================
// ChassisSetupModel implementation
// ============================================================================

WeightDistribution ChassisSetupModel::calculateWeightDistribution(
    float mass, float wheelBase, float trackWidth,
    float frontAxleDist, float rearAxleDist,
    float cgHeight, float lateralOffset
) const {
    WeightDistribution dist;

    float gravity = 9.81f;
    float totalWeight = mass * gravity;

    dist.frontPercent = (rearAxleDist / wheelBase) * 100.0f;
    dist.rearPercent = (frontAxleDist / wheelBase) * 100.0f;

    float halfTrack = trackWidth * 0.5f;
    float lateralRatio = lateralOffset / halfTrack;
    lateralRatio = std::clamp(lateralRatio, -1.0f, 1.0f);

    dist.leftPercent = 50.0f + lateralRatio * 10.0f;
    dist.rightPercent = 50.0f - lateralRatio * 10.0f;

    dist.frontLeftPercent = dist.frontPercent * dist.leftPercent * 0.01f;
    dist.frontRightPercent = dist.frontPercent * dist.rightPercent * 0.01f;
    dist.rearLeftPercent = dist.rearPercent * dist.leftPercent * 0.01f;
    dist.rearRightPercent = dist.rearPercent * dist.rightPercent * 0.01f;

    dist.crossWeight = (dist.frontLeftPercent + dist.rearRightPercent) -
                       (dist.frontRightPercent + dist.rearLeftPercent);

    return dist;
}

ChassisAnalysis ChassisSetupModel::analyzeSetup(
    const BalanceSetup& setup,
    float mass, float wheelBase
) const {
    ChassisAnalysis analysis;

    analysis.frontRollStiffness = setup.springRateFront + setup.antiRollBarFront;
    analysis.rearRollStiffness = setup.springRateRear + setup.antiRollBarRear;
    analysis.totalRollStiffness = analysis.frontRollStiffness + analysis.rearRollStiffness;

    analysis.frontRollPercent = (analysis.totalRollStiffness > 0.0f) ?
        analysis.frontRollStiffness / analysis.totalRollStiffness * 100.0f : 50.0f;

    // Understeer/oversteer contribution from roll stiffness
    analysis.understeerContribution = (analysis.frontRollPercent - 50.0f) * 0.1f;
    analysis.oversteerContribution = -analysis.understeerContribution;

    // Natural frequencies
    analysis.naturalFrequencyFront = calculateNaturalFrequency(
        setup.springRateFront, mass * 0.45f, 1.0f
    );
    analysis.naturalFrequencyRear = calculateNaturalFrequency(
        setup.springRateRear, mass * 0.55f, 1.0f
    );

    // Damping ratios
    analysis.dampingRatioFront = calculateDampingRatio(
        setup.damperBumpFront, setup.springRateFront, mass * 0.45f
    );
    analysis.dampingRatioRear = calculateDampingRatio(
        setup.damperBumpRear, setup.springRateRear, mass * 0.55f
    );

    // Balance check
    float freqDiff = std::abs(analysis.naturalFrequencyFront - analysis.naturalFrequencyRear);
    float dampingDiff = std::abs(analysis.dampingRatioFront - analysis.dampingRatioRear);
    analysis.isBalanced = (freqDiff < 0.5f && dampingDiff < 0.1f);

    // Recommendation
    if (analysis.frontRollPercent > 55.0f) {
        analysis.recommendation = "Front too stiff - increase rear ARB or reduce front spring rate";
    } else if (analysis.frontRollPercent < 45.0f) {
        analysis.recommendation = "Rear too stiff - increase front ARB or reduce rear spring rate";
    } else if (analysis.dampingRatioFront < 0.2f) {
        analysis.recommendation = "Front underdamped - increase bump damping";
    } else if (analysis.dampingRatioRear > 0.5f) {
        analysis.recommendation = "Rear overdamped - reduce rebound damping";
    } else {
        analysis.recommendation = "Setup is balanced";
    }

    return analysis;
}

SetupRecommendation ChassisSetupModel::recommendSetup(
    const ChassisAnalysis& analysis,
    const BalanceSetup& currentSetup,
    float targetUndersteer
) const {
    SetupRecommendation rec;

    float understeerError = analysis.understeerContribution - targetUndersteer;

    if (understeerError > 0.5f) {
        // Too much understeer
        rec.springRateChange = -1000.0f;
        rec.antiRollBarChange = -500.0f;
        rec.camberChange = -0.2f;
        rec.reason = "Reduce front roll stiffness to decrease understeer";
    } else if (understeerError < -0.5f) {
        // Too much oversteer
        rec.springRateChange = 1000.0f;
        rec.antiRollBarChange = 500.0f;
        rec.camberChange = 0.2f;
        rec.reason = "Increase front roll stiffness to increase understeer";
    } else {
        rec.reason = "Balance is within target";
    }

    // Damping adjustments
    if (analysis.dampingRatioFront < 0.25f) {
        rec.damperChange = 200.0f;
        rec.reason += "; Increase front bump damping";
    } else if (analysis.dampingRatioRear > 0.45f) {
        rec.damperChange = -200.0f;
        rec.reason += "; Reduce rear rebound damping";
    }

    rec.expectedImprovement = std::abs(understeerError) * 0.1f;

    return rec;
}

float ChassisSetupModel::calculateOptimalFrontSpringRate(
    float mass, float wheelBase, float frontAxleDist,
    float targetFrequency
) const {
    float frontMass = mass * (wheelBase - frontAxleDist) / wheelBase;
    return calculateSpringRateForFrequency(frontMass, targetFrequency, 1.0f);
}

float ChassisSetupModel::calculateOptimalRearSpringRate(
    float mass, float wheelBase, float rearAxleDist,
    float targetFrequency
) const {
    float rearMass = mass * rearAxleDist / wheelBase;
    return calculateSpringRateForFrequency(rearMass, targetFrequency, 1.0f);
}

float ChassisSetupModel::calculateNaturalFrequency(
    float springRate, float mass, float motionRatio
) const {
    if (mass <= 0.0f) return 0.0f;
    float effectiveSpringRate = springRate * motionRatio * motionRatio;
    return std::sqrt(effectiveSpringRate / mass) / (2.0f * 3.14159265f);
}

float ChassisSetupModel::calculateDampingRatio(
    float damperRate, float springRate, float mass
) const {
    if (mass <= 0.0f || springRate <= 0.0f) return 0.0f;
    float criticalDamping = 2.0f * std::sqrt(springRate * mass);
    return damperRate / criticalDamping;
}

float ChassisSetupModel::calculateCrossWeight(
    float frontLeft, float frontRight,
    float rearLeft, float rearRight
) const {
    float total = frontLeft + frontRight + rearLeft + rearRight;
    if (total <= 0.0f) return 0.0f;

    float diagonal1 = frontLeft + rearRight;
    float diagonal2 = frontRight + rearLeft;

    return (diagonal1 - diagonal2) / total * 100.0f;
}

float ChassisSetupModel::calculateSpringRateForFrequency(
    float mass, float frequency, float motionRatio
) const {
    float omega = frequency * 2.0f * 3.14159265f;
    return mass * omega * omega / (motionRatio * motionRatio);
}

// ============================================================================
// BrakingModel implementation
// ============================================================================

BrakeState BrakingModel::calculate(
    const BrakeConfig& config,
    float brakeInput, float speed,
    float normalLoadFront, float normalLoadRear,
    float dt
) const {
    BrakeState state;

    float effectiveInput = calculateBrakeResponse(
        brakeInput, config.brakeResponseTime, dt
    );

    float brakePressure = effectiveInput * config.maxBrakePressure;

    state.frontBrakeTorque = calculateBrakeTorque(
        brakePressure * config.frontBrakeBias / 100.0f,
        config.frontBrakeArea, config.frontPadFriction, 0.15f
    );

    state.rearBrakeTorque = calculateBrakeTorque(
        brakePressure * config.rearBrakeBias / 100.0f,
        config.rearBrakeArea, config.rearPadFriction, 0.15f
    );

    state.totalBrakeTorque = state.frontBrakeTorque + state.rearBrakeTorque;

    state.brakeBias = (state.totalBrakeTorque > 0.0f) ?
        state.frontBrakeTorque / state.totalBrakeTorque * 100.0f : 60.0f;

    // Deceleration from brake torque
    float frontDecel = (normalLoadFront > 0.0f) ?
        state.frontBrakeTorque / (normalLoadFront * 0.33f) : 0.0f;
    float rearDecel = (normalLoadRear > 0.0f) ?
        state.rearBrakeTorque / (normalLoadRear * 0.33f) : 0.0f;
    state.deceleration = (frontDecel + rearDecel) * 0.5f;

    // Slip ratios
    state.slipRatioFront = (speed > 1.0f) ?
        std::min(state.deceleration / speed, 1.0f) : 0.0f;
    state.slipRatioRear = state.slipRatioFront;

    // ABS
    state.absActive = false;
    if (config.hasABS) {
        if (state.slipRatioFront > config.absThreshold) {
            state.absActive = true;
            state.frontBrakeTorque *= 0.7f;
            state.slipRatioFront = config.absThreshold;
        }
        if (state.slipRatioRear > config.absThreshold) {
            state.absActive = true;
            state.rearBrakeTorque *= 0.7f;
            state.slipRatioRear = config.absThreshold;
        }
    }

    // Brake temperature
    state.brakeTemperature = 300.0f;
    state.brakeFade = calculateBrakeFade(state.brakeTemperature);

    return state;
}

BrakeTransition BrakingModel::analyzeTransition(
    const BrakeConfig& config,
    float targetDeceleration
) const {
    BrakeTransition transition;

    transition.responseDelay = config.brakeResponseTime;
    transition.buildUpTime = 0.1f + config.brakeResponseTime * 2.0f;

    transition.peakDeceleration = targetDeceleration * 1.1f;
    transition.stableDeceleration = targetDeceleration;

    transition.fadeRate = 0.01f;
    transition.temperatureSensitivity = 0.001f;

    return transition;
}

DynamicBrakeBias BrakingModel::calculateDynamicBias(
    const BrakeConfig& config,
    float speed, float normalLoadFront, float normalLoadRear,
    float brakeTemperature
) const {
    DynamicBrakeBias bias;

    bias.staticBias = config.frontBrakeBias;

    float totalLoad = normalLoadFront + normalLoadRear;
    bias.loadTransferEffect = (totalLoad > 0.0f) ?
        (normalLoadFront / totalLoad - 0.5f) * 10.0f : 0.0f;

    bias.speedEffect = speed * 0.001f;

    bias.temperatureEffect = (brakeTemperature - 300.0f) * 0.005f;

    bias.dynamicBias = bias.staticBias + bias.loadTransferEffect +
                       bias.speedEffect - bias.temperatureEffect;
    bias.dynamicBias = std::clamp(bias.dynamicBias, 40.0f, 75.0f);

    bias.optimalBias = calculateOptimalBrakeBias(
        1.35f, 2.7f, 0.45f, 10.0f
    );

    bias.biasWindow = 3.0f;

    return bias;
}

float BrakingModel::calculateOptimalBrakeBias(
    float frontAxleDist, float wheelBase,
    float cgHeight, float maxDeceleration
) const {
    float gravity = 9.81f;

    float optimalFront = (1.0f - frontAxleDist / wheelBase) * 100.0f;

    // Load transfer contribution
    float loadTransferRatio = maxDeceleration * cgHeight / (wheelBase * gravity);
    optimalFront += loadTransferRatio * 20.0f;

    return std::clamp(optimalFront, 45.0f, 70.0f);
}

float BrakingModel::calculateBrakeTemperature(
    float currentTemp, float brakeTorque, float speed,
    float ambientTemp, float dt
) const {
    float heatGeneration = brakeTorque * speed * 0.001f;
    float cooling = (currentTemp - ambientTemp) * 0.01f;

    float newTemp = currentTemp + (heatGeneration - cooling) * dt;
    return std::clamp(newTemp, ambientTemp, 800.0f);
}

float BrakingModel::calculateBrakeFade(float temperature) const {
    if (temperature < 400.0f) return 0.0f;

    float fade = (temperature - 400.0f) / 400.0f;
    return std::clamp(fade, 0.0f, 0.5f);
}

float BrakingModel::calculateBrakeResponse(
    float brakeInput, float responseTime, float dt
) const {
    float target = brakeInput;
    float response = 1.0f - std::exp(-dt / responseTime);
    return brakeInput * response;
}

float BrakingModel::calculateLockupThreshold(
    float normalLoad, float gripCoefficient
) const {
    float gravity = 9.81f;
    float maxBrakeForce = normalLoad * gripCoefficient;
    float lockupTorque = maxBrakeForce * 0.33f;

    return lockupTorque;
}

float BrakingModel::calculateABSIntervention(
    float slipRatio, float threshold
) const {
    if (slipRatio <= threshold) return 1.0f;

    float intervention = 1.0f - (slipRatio - threshold) * 2.0f;
    return std::clamp(intervention, 0.5f, 1.0f);
}

float BrakingModel::calculateBrakeTorque(
    float brakePressure, float brakeArea,
    float frictionCoefficient, float radius
) const {
    float normalForce = brakePressure * brakeArea * 100000.0f;
    return normalForce * frictionCoefficient * radius;
}

// ============================================================================
// TractionControlModel implementation
// ============================================================================

TractionControlResult TractionControlModel::calculate(
    const TractionControlConfig& config,
    const TractionControlState& state,
    float throttle, float brake
) const {
    TractionControlResult result;

    if (config.mode == TractionControlMode::Off) {
        result.throttleMultiplier = 1.0f;
        result.tcActive = false;
        return result;
    }

    float maxSlip = 0.0f;
    int maxSlipWheel = 0;

    for (int i = 0; i < 4; ++i) {
        if (std::abs(state.slipRatio[i]) > maxSlip) {
            maxSlip = std::abs(state.slipRatio[i]);
            maxSlipWheel = i;
        }
    }

    result.tcLevel = calculateSlipControl(
        maxSlip, config.slipTarget, config.maxTorqueReduction
    );

    result.throttleMultiplier = 1.0f - result.tcLevel;
    result.throttleMultiplier = std::clamp(result.throttleMultiplier, 0.5f, 1.0f);

    result.tcActive = result.tcLevel > 0.01f;

    if (result.tcActive) {
        result.statusMessage = QString("TC active - Slip: %1%")
            .arg(maxSlip * 100.0f, 0, 'f', 1);
    } else {
        result.statusMessage = "TC inactive";
    }

    return result;
}

LaunchControlState TractionControlModel::calculateLaunch(
    const TractionControlConfig& config,
    const LaunchControlState& launchState,
    float throttle, float rpm, float speed
) const {
    LaunchControlState state = launchState;

    if (!config.launchControlEnabled) {
        state.active = false;
        return state;
    }

    state.currentRpm = rpm;

    if (throttle > 0.9f && speed < 1.0f) {
        state.active = true;
        state.launchReady = (rpm >= config.launchRpm * 0.9f);
    } else if (speed > 5.0f) {
        state.active = false;
        state.launchComplete = true;
    }

    if (state.active && state.launchReady) {
        state.launchTorque = config.launchRpm * 0.5f;
    } else {
        state.launchTorque = 0.0f;
    }

    return state;
}

float TractionControlModel::calculateSlipRatio(
    float wheelSpeed, float vehicleSpeed
) const {
    if (vehicleSpeed < 1.0f) {
        return (wheelSpeed > 0.0f) ? 1.0f : 0.0f;
    }
    return (wheelSpeed - vehicleSpeed) / vehicleSpeed;
}

float TractionControlModel::calculateTorqueReduction(
    float slipRatio, float slipThreshold, float slipTarget,
    float interventionRate
) const {
    if (std::abs(slipRatio) <= slipThreshold) {
        return 0.0f;
    }

    float excessSlip = std::abs(slipRatio) - slipThreshold;
    float reduction = excessSlip * interventionRate;
    return std::clamp(reduction, 0.0f, 0.5f);
}

float TractionControlModel::calculateThrottleMultiplier(
    float slipRatio, const TractionControlConfig& config
) const {
    if (std::abs(slipRatio) <= config.slipThreshold) {
        return 1.0f;
    }

    float reduction = calculateTorqueReduction(
        slipRatio, config.slipThreshold, config.slipTarget,
        config.interventionRate
    );

    return 1.0f - reduction;
}

bool TractionControlModel::checkWheelSpin(
    float slipRatio, float threshold
) const {
    return std::abs(slipRatio) > threshold;
}

float TractionControlModel::calculateOptimalSlip(
    float speed, float surfaceGrip
) const {
    float baseSlip = 0.12f;
    float speedFactor = 1.0f - speed * 0.001f;
    float gripFactor = surfaceGrip;

    return baseSlip * speedFactor * gripFactor;
}

float TractionControlModel::calculateSlipControl(
    float slipRatio, float targetSlip, float maxReduction
) const {
    if (slipRatio <= targetSlip) {
        return 0.0f;
    }

    float control = (slipRatio - targetSlip) / (1.0f - targetSlip);
    return std::clamp(control, 0.0f, maxReduction);
}

// ============================================================================
// ActiveSuspensionModel implementation
// ============================================================================

ActiveSuspensionState ActiveSuspensionModel::calculate(
    const ActiveSuspensionConfig& config,
    const SuspensionCommand& command,
    float speed, float lateralAccel, float longitudinalAccel,
    float dt
) const {
    ActiveSuspensionState state;

    // Calculate base forces from passive suspension
    float baseForceFL = 5000.0f + lateralAccel * 2000.0f + longitudinalAccel * 1500.0f;
    float baseForceFR = 5000.0f - lateralAccel * 2000.0f + longitudinalAccel * 1500.0f;
    float baseForceRL = 5000.0f + lateralAccel * 2000.0f - longitudinalAccel * 1500.0f;
    float baseForceRR = 5000.0f - lateralAccel * 2000.0f - longitudinalAccel * 1500.0f;

    // Apply active control
    state.frontLeft.force = calculateActiveForce(
        command.targetForceFL, baseForceFL,
        config.maxForce, config.responseTime, dt
    );
    state.frontRight.force = calculateActiveForce(
        command.targetForceFR, baseForceFR,
        config.maxForce, config.responseTime, dt
    );
    state.rearLeft.force = calculateActiveForce(
        command.targetForceRL, baseForceRL,
        config.maxForce, config.responseTime, dt
    );
    state.rearRight.force = calculateActiveForce(
        command.targetForceRR, baseForceRR,
        config.maxForce, config.responseTime, dt
    );

    // Anti-roll control
    if (command.enableAntiRoll && config.antiRollEnabled) {
        float antiRollFL = calculateAntiRollForce(
            state.frontLeft.force, state.frontRight.force, config.maxForce
        );
        float antiRollFR = -antiRollFL;

        state.frontLeft.force += antiRollFL;
        state.frontRight.force += antiRollFR;
    }

    // Load leveling
    if (command.enableLoadLeveling && config.loadLevelingEnabled) {
        float avgForce = (state.frontLeft.force + state.frontRight.force +
                         state.rearLeft.force + state.rearRight.force) * 0.25f;

        state.frontLeft.force += calculateLoadLevelingForce(
            avgForce, state.frontLeft.force, config.maxForce
        );
        state.frontRight.force += calculateLoadLevelingForce(
            avgForce, state.frontRight.force, config.maxForce
        );
        state.rearLeft.force += calculateLoadLevelingForce(
            avgForce, state.rearLeft.force, config.maxForce
        );
        state.rearRight.force += calculateLoadLevelingForce(
            avgForce, state.rearRight.force, config.maxForce
        );
    }

    // Ride height control
    if (command.adjustRideHeight && config.rideHeightControl) {
        state.rideHeightFront = 0.05f;
        state.rideHeightRear = 0.07f;
    }

    // Calculate power consumption
    state.totalPowerDraw = calculatePowerConsumption(
        state.frontLeft.force, state.frontLeft.velocity
    ) + calculatePowerConsumption(
        state.frontRight.force, state.frontRight.velocity
    ) + calculatePowerConsumption(
        state.rearLeft.force, state.rearLeft.velocity
    ) + calculatePowerConsumption(
        state.rearRight.force, state.rearRight.velocity
    );

    // Calculate roll and pitch
    float frontForceDiff = state.frontRight.force - state.frontLeft.force;
    float rearForceDiff = state.rearRight.force - state.rearLeft.force;
    state.rollAngle = std::atan2(frontForceDiff + rearForceDiff, 15000.0f);

    float frontAvgForce = (state.frontLeft.force + state.frontRight.force) * 0.5f;
    float rearAvgForce = (state.rearLeft.force + state.rearRight.force) * 0.5f;
    state.pitchAngle = std::atan2(rearAvgForce - frontAvgForce, 25000.0f);

    return state;
}

float ActiveSuspensionModel::calculateActiveForce(
    float targetForce, float currentForce,
    float maxForce, float responseTime, float dt
) const {
    float error = targetForce - currentForce;
    float force = pidController(error, 100.0f, 50.0f, 20.0f, dt);
    force = std::clamp(force, -maxForce, maxForce);
    return currentForce + force;
}

float ActiveSuspensionModel::calculateAntiRollForce(
    float leftForce, float rightForce,
    float maxForce
) const {
    float diff = leftForce - rightForce;
    float antiRollForce = diff * 0.3f;
    return std::clamp(antiRollForce, -maxForce, maxForce);
}

float ActiveSuspensionModel::calculateLoadLevelingForce(
    float currentHeight, float targetHeight,
    float maxForce
) const {
    float error = targetHeight - currentHeight;
    float force = error * 10000.0f;
    return std::clamp(force, -maxForce, maxForce);
}

float ActiveSuspensionModel::calculateRideHeightForce(
    float currentHeight, float targetHeight,
    float maxForce
) const {
    float error = targetHeight - currentHeight;
    float force = error * 5000.0f;
    return std::clamp(force, -maxForce, maxForce);
}

float ActiveSuspensionModel::calculatePowerConsumption(
    float force, float velocity
) const {
    return std::abs(force * velocity);
}

float ActiveSuspensionModel::pidController(
    float error, float kp, float ki, float kd,
    float dt
) const {
    static float integral = 0.0f;
    static float previousError = 0.0f;

    integral += error * dt;
    integral = std::clamp(integral, -100.0f, 100.0f);

    float derivative = (error - previousError) / dt;
    previousError = error;

    return kp * error + ki * integral + kd * derivative;
}

// ============================================================================
// FourWheelSteeringModel implementation
// ============================================================================

SteeringState FourWheelSteeringModel::calculate(
    const SteeringConfig& config,
    float frontSteerInput, float speed
) const {
    SteeringState state;

    state.frontSteerAngle = frontSteerInput * config.frontSteerLock / config.steerRatio;

    state.rearSteerAngle = calculateRearSteerAngle(config, state.frontSteerAngle, speed);

    state.isCounterSteering = (speed > config.speedThresholdHigh);
    state.isCrabSteering = false;

    state.totalSteerAngle = state.frontSteerAngle + state.rearSteerAngle;

    state.effectiveWheelBase = calculateEffectiveWheelBase(
        2.7f, state.frontSteerAngle, state.rearSteerAngle
    );

    state.turningRadius = calculateTurningRadius(
        2.7f, state.frontSteerAngle, state.rearSteerAngle
    );

    return state;
}

SteeringResponse FourWheelSteeringModel::calculateResponse(
    const SteeringState& state,
    float speed, float yawRate
) const {
    SteeringResponse response;

    response.yawRateResponse = yawRate;
    response.lateralAccelResponse = speed * yawRate;

    if (speed > 1.0f) {
        response.sideslipAngle = std::atan2(
            state.rearSteerAngle * 0.01f, speed
        );
    } else {
        response.sideslipAngle = 0.0f;
    }

    response.responseTime = 0.1f;
    response.overshoot = 0.0f;

    return response;
}

float FourWheelSteeringModel::calculateRearSteerAngle(
    const SteeringConfig& config,
    float frontSteerAngle, float speed
) const {
    if (!config.adaptiveEnabled) return 0.0f;

    float rearAngle = 0.0f;

    if (speed < config.speedThresholdLow) {
        // Low speed: rear steers opposite to front (tighter turning)
        rearAngle = -frontSteerAngle * 0.5f;
        rearAngle = std::clamp(rearAngle, -config.maxRearSteerAngle, config.maxRearSteerAngle);
    } else if (speed > config.speedThresholdHigh) {
        // High speed: rear steers same direction (counter-steer for stability)
        rearAngle = frontSteerAngle * 0.3f;
        rearAngle = std::clamp(rearAngle, -config.maxRearSteerAngle, config.maxRearSteerAngle);
    } else {
        // Transition zone
        float ratio = (speed - config.speedThresholdLow) /
                      (config.speedThresholdHigh - config.speedThresholdLow);
        float lowSpeedAngle = -frontSteerAngle * 0.5f;
        float highSpeedAngle = frontSteerAngle * 0.3f;
        rearAngle = interpolateRearSteer(speed, lowSpeedAngle, highSpeedAngle,
                                          config.speedThresholdLow, config.speedThresholdHigh);
    }

    return rearAngle;
}

float FourWheelSteeringModel::calculateTurningRadius(
    float wheelBase, float frontSteerAngle, float rearSteerAngle
) const {
    float frontRad = frontSteerAngle * M_PI / 180.0f;
    float rearRad = rearSteerAngle * M_PI / 180.0f;

    if (std::abs(frontRad) < 0.001f && std::abs(rearRad) < 0.001f) {
        return 1000.0f; // Essentially straight
    }

    float radius = wheelBase / std::tan(frontRad - rearRad);
    return std::abs(radius);
}

float FourWheelSteeringModel::calculateEffectiveWheelBase(
    float wheelBase, float frontSteerAngle, float rearSteerAngle
) const {
    float frontRad = frontSteerAngle * M_PI / 180.0f;
    float rearRad = rearSteerAngle * M_PI / 180.0f;

    float effectiveWheelBase = wheelBase * (1.0f - rearRad / frontRad);
    return effectiveWheelBase;
}

SteeringMode FourWheelSteeringModel::determineMode(
    const SteeringConfig& config, float speed
) const {
    if (!config.adaptiveEnabled) {
        return SteeringMode::FrontWheelOnly;
    }

    if (speed < config.speedThresholdLow) {
        return SteeringMode::FourWheelSteering;
    } else if (speed > config.speedThresholdHigh) {
        return SteeringMode::CounterSteer;
    } else {
        return SteeringMode::FourWheelSteering;
    }
}

float FourWheelSteeringModel::interpolateRearSteer(
    float speed, float lowSpeedAngle, float highSpeedAngle,
    float speedLow, float speedHigh
) const {
    float ratio = (speed - speedLow) / (speedHigh - speedLow);
    ratio = std::clamp(ratio, 0.0f, 1.0f);
    return lowSpeedAngle + (highSpeedAngle - lowSpeedAngle) * ratio;
}

// ============================================================================
// VehicleAxisSystem implementation
// ============================================================================

VehicleAxisSystem::VehicleAxisSystem(CoordinateSystem system)
    : m_system(system)
{
    initializeAxes();
}

void VehicleAxisSystem::initializeAxes() {
    switch (m_system) {
        case CoordinateSystem::SAE:
        case CoordinateSystem::ISO:
            m_axes.forward = QVector3D(1.0f, 0.0f, 0.0f);
            m_axes.left = QVector3D(0.0f, 1.0f, 0.0f);
            m_axes.up = QVector3D(0.0f, 0.0f, 1.0f);
            m_worldFrame.north = QVector3D(1.0f, 0.0f, 0.0f);
            m_worldFrame.east = QVector3D(0.0f, 1.0f, 0.0f);
            m_worldFrame.down = QVector3D(0.0f, 0.0f, -1.0f);
            break;

        case CoordinateSystem::OpenXR:
        case CoordinateSystem::OpenGL:
            m_axes.forward = QVector3D(0.0f, 0.0f, -1.0f);
            m_axes.left = QVector3D(-1.0f, 0.0f, 0.0f);
            m_axes.up = QVector3D(0.0f, 1.0f, 0.0f);
            m_worldFrame.north = QVector3D(0.0f, 0.0f, -1.0f);
            m_worldFrame.east = QVector3D(-1.0f, 0.0f, 0.0f);
            m_worldFrame.down = QVector3D(0.0f, -1.0f, 0.0f);
            break;

        case CoordinateSystem::Custom:
            m_axes.forward = QVector3D(1.0f, 0.0f, 0.0f);
            m_axes.left = QVector3D(0.0f, 1.0f, 0.0f);
            m_axes.up = QVector3D(0.0f, 0.0f, 1.0f);
            break;
    }
}

QVector3D VehicleAxisSystem::toWorld(const QVector3D& bodyVector, const BodyFrame& body) const {
    QVector3D world = body.rotation.map(bodyVector);
    return world + body.position;
}

QVector3D VehicleAxisSystem::toBody(const QVector3D& worldVector, const BodyFrame& body) const {
    QVector3D relative = worldVector - body.position;
    return body.rotation.inverted().map(relative);
}

MotionState VehicleAxisSystem::fromAccelerations(
    float lateralAccel, float yawRate, float speed,
    float wheelBase, float steerAngle
) const {
    MotionState state;

    state.vx = speed;
    state.vy = lateralAccel * speed / 9.81f;
    state.yawRate = yawRate;

    float slipAngle = std::atan2(state.vy, state.vx);
    state.ax = -state.vy * yawRate;
    state.ay = lateralAccel;

    return state;
}

float VehicleAxisSystem::normalizeAngle(float angle) const {
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

float VehicleAxisSystem::wrapToPi(float angle) const {
    while (angle > M_PI) angle -= 2.0f * M_PI;
    while (angle < -M_PI) angle += 2.0f * M_PI;
    return angle;
}

float VehicleAxisSystem::wrapTo2Pi(float angle) const {
    while (angle > 2.0f * M_PI) angle -= 2.0f * M_PI;
    while (angle < 0.0f) angle += 2.0f * M_PI;
    return angle;
}

QMatrix4x4 VehicleAxisSystem::rotationFromEuler(float roll, float pitch, float yaw) const {
    QMatrix4x4 result;

    float cr = std::cos(roll);
    float sr = std::sin(roll);
    float cp = std::cos(pitch);
    float sp = std::sin(pitch);
    float cy = std::cos(yaw);
    float sy = std::sin(yaw);

    result(0, 0) = cp * cy;
    result(0, 1) = sr * sp * cy - cr * sy;
    result(0, 2) = cr * sp * cy + sr * sy;
    result(1, 0) = cp * sy;
    result(1, 1) = sr * sp * sy + cr * cy;
    result(1, 2) = cr * sp * sy - sr * cy;
    result(2, 0) = -sp;
    result(2, 1) = sr * cp;
    result(2, 2) = cr * cp;

    return result;
}

QMatrix4x4 VehicleAxisSystem::translationMatrix(const QVector3D& translation) const {
    QMatrix4x4 result;
    result.translate(translation);
    return result;
}

// ============================================================================
// SimulationMethodsModel implementation
// ============================================================================

VehicleSimState SimulationMethodsModel::integrate(
    const VehicleSimState& currentState,
    const PhysicsForces& forces,
    float mass, float inertia[3],
    float dt, IntegrationMethod method
) const {
    VehicleSimState newState = currentState;

    switch (method) {
        case IntegrationMethod::Euler:
            for (int i = 0; i < 3; ++i) {
                newState.velocity[i] += forces.totalForce[i] / mass * dt;
                newState.position[i] += currentState.velocity[i] * dt;
                newState.angularVelocity[i] += forces.totalMoment[i] / inertia[i] * dt;
                newState.orientation[i] += currentState.angularVelocity[i] * dt;
            }
            break;

        case IntegrationMethod::RungeKutta2:
            for (int i = 0; i < 3; ++i) {
                float k1v = forces.totalForce[i] / mass;
                float k1x = currentState.velocity[i];
                float k2v = forces.totalForce[i] / mass;
                float k2x = currentState.velocity[i] + k1v * dt;

                newState.velocity[i] += (k1v + k2v) * 0.5f * dt;
                newState.position[i] += (k1x + k2x) * 0.5f * dt;

                float k1w = forces.totalMoment[i] / inertia[i];
                float k1o = currentState.angularVelocity[i];
                float k2w = forces.totalMoment[i] / inertia[i];
                float k2o = currentState.angularVelocity[i] + k1w * dt;

                newState.angularVelocity[i] += (k1w + k2w) * 0.5f * dt;
                newState.orientation[i] += (k1o + k2o) * 0.5f * dt;
            }
            break;

        case IntegrationMethod::RungeKutta4:
            for (int i = 0; i < 3; ++i) {
                float k1v = forces.totalForce[i] / mass;
                float k1x = currentState.velocity[i];

                float k2v = forces.totalForce[i] / mass;
                float k2x = currentState.velocity[i] + k1v * dt * 0.5f;

                float k3v = forces.totalForce[i] / mass;
                float k3x = currentState.velocity[i] + k2v * dt * 0.5f;

                float k4v = forces.totalForce[i] / mass;
                float k4x = currentState.velocity[i] + k3v * dt;

                newState.velocity[i] += (k1v + 2.0f*k2v + 2.0f*k3v + k4v) / 6.0f * dt;
                newState.position[i] += (k1x + 2.0f*k2x + 2.0f*k3x + k4x) / 6.0f * dt;

                float k1w = forces.totalMoment[i] / inertia[i];
                float k1o = currentState.angularVelocity[i];

                float k2w = forces.totalMoment[i] / inertia[i];
                float k2o = currentState.angularVelocity[i] + k1w * dt * 0.5f;

                float k3w = forces.totalMoment[i] / inertia[i];
                float k3o = currentState.angularVelocity[i] + k2w * dt * 0.5f;

                float k4w = forces.totalMoment[i] / inertia[i];
                float k4o = currentState.angularVelocity[i] + k3w * dt;

                newState.angularVelocity[i] += (k1w + 2.0f*k2w + 2.0f*k3w + k4w) / 6.0f * dt;
                newState.orientation[i] += (k1o + 2.0f*k2o + 2.0f*k3o + k4o) / 6.0f * dt;
            }
            break;

        case IntegrationMethod::Verlet:
            for (int i = 0; i < 3; ++i) {
                float acceleration = forces.totalForce[i] / mass;
                newState.position[i] = 2.0f * currentState.position[i] -
                    currentState.position[i] + acceleration * dt * dt;
                newState.velocity[i] = (newState.position[i] - currentState.position[i]) / dt;
            }
            break;

        case IntegrationMethod::SymplecticEuler:
            for (int i = 0; i < 3; ++i) {
                newState.velocity[i] += forces.totalForce[i] / mass * dt;
                newState.position[i] += newState.velocity[i] * dt;

                newState.angularVelocity[i] += forces.totalMoment[i] / inertia[i] * dt;
                newState.orientation[i] += newState.angularVelocity[i] * dt;
            }
            break;
    }

    return newState;
}

PhysicsForces SimulationMethodsModel::calculateForces(
    const VehicleSimState& state,
    const SimulationConfig& config
) const {
    PhysicsForces forces;

    // Tire forces (simplified)
    for (int i = 0; i < 4; ++i) {
        forces.tireForces[i][0] = 0.0f;  // Longitudinal
        forces.tireForces[i][1] = 0.0f;  // Lateral
        forces.tireForces[i][2] = 0.0f;  // Vertical
    }

    // Aero forces
    if (config.enableAero) {
        float speed = std::sqrt(
            state.velocity[0] * state.velocity[0] +
            state.velocity[1] * state.velocity[1]
        );
        float dragCoeff = 0.35f;
        float frontalArea = 2.0f;
        float drag = 0.5f * config.airDensity * speed * speed * dragCoeff * frontalArea;

        forces.aeroForce[0] = -drag;
        forces.aeroForce[1] = 0.0f;
        forces.aeroForce[2] = 0.0f;
    }

    // Total forces
    for (int i = 0; i < 3; ++i) {
        forces.totalForce[i] = forces.aeroForce[i];
        for (int j = 0; j < 4; ++j) {
            forces.totalForce[i] += forces.tireForces[j][i];
        }
    }

    return forces;
}

float SimulationMethodsModel::eulerIntegration(
    float state, float derivative, float dt
) const {
    return state + derivative * dt;
}

float SimulationMethodsModel::rungeKutta2Integration(
    float state, float derivative, float dt,
    float (*func)(float)
) const {
    float k1 = derivative;
    float k2 = func(state + k1 * dt);
    return state + (k1 + k2) * 0.5f * dt;
}

float SimulationMethodsModel::rungeKutta4Integration(
    float state, float derivative, float dt,
    float (*func)(float)
) const {
    float k1 = derivative;
    float k2 = func(state + k1 * dt * 0.5f);
    float k3 = func(state + k2 * dt * 0.5f);
    float k4 = func(state + k3 * dt);
    return state + (k1 + 2.0f*k2 + 2.0f*k3 + k4) / 6.0f * dt;
}

void SimulationMethodsModel::updateStep(
    VehicleSimState& state,
    const PhysicsForces& forces,
    float mass, float inertia[3],
    float dt
) const {
    for (int i = 0; i < 3; ++i) {
        state.velocity[i] += forces.totalForce[i] / mass * dt;
        state.position[i] += state.velocity[i] * dt;

        state.angularVelocity[i] += forces.totalMoment[i] / inertia[i] * dt;
        state.orientation[i] += state.angularVelocity[i] * dt;
    }
}

float SimulationMethodsModel::calculatePhysicsStep(
    const VehicleSimState& state,
    const PhysicsForces& forces,
    float mass, float dt
) const {
    float kineticEnergy = 0.5f * mass * (
        state.velocity[0] * state.velocity[0] +
        state.velocity[1] * state.velocity[1] +
        state.velocity[2] * state.velocity[2]
    );

    float work = forces.totalForce[0] * state.velocity[0] * dt +
                 forces.totalForce[1] * state.velocity[1] * dt +
                 forces.totalForce[2] * state.velocity[2] * dt;

    return kineticEnergy + work;
}

bool SimulationMethodsModel::checkCollision(
    const VehicleSimState& state,
    float collisionRadius
) const {
    float distance = std::sqrt(
        state.position[0] * state.position[0] +
        state.position[1] * state.position[1] +
        state.position[2] * state.position[2]
    );

    return distance < collisionRadius;
}

float SimulationMethodsModel::calculateEnergy(
    const VehicleSimState& state,
    float mass, float inertia[3]
) const {
    float kineticEnergy = 0.5f * mass * (
        state.velocity[0] * state.velocity[0] +
        state.velocity[1] * state.velocity[1] +
        state.velocity[2] * state.velocity[2]
    );

    float rotationalEnergy = 0.5f * (
        inertia[0] * state.angularVelocity[0] * state.angularVelocity[0] +
        inertia[1] * state.angularVelocity[1] * state.angularVelocity[1] +
        inertia[2] * state.angularVelocity[2] * state.angularVelocity[2]
    );

    return kineticEnergy + rotationalEnergy;
}

float SimulationMethodsModel::calculateInertiaComponent(
    float mass, float length, float width
) const {
    return mass * (length * length + width * width) / 12.0f;
}

// ============================================================================
// Generic model implementations (from VehicleSimulator.cpp)
// ============================================================================

double GenericTireModel::calculateLateralForce(double slipAngle, double normalLoad, double friction) const {
    double slipRad = qDegreesToRadians(slipAngle);
    double peakForce = 1.0 * normalLoad * friction;
    double b = 10.0 / peakForce;
    return peakForce * qSin(1.3 * qAtan(b * slipRad - 0.0 * (b * slipRad - qAtan(b * slipRad))));
}

double GenericTireModel::calculateLongitudinalForce(double slipRatio, double normalLoad, double friction) const {
    double peakForce = 1.1 * normalLoad * friction;
    double b = 10.0 / peakForce;
    return peakForce * qSin(1.3 * qAtan(b * slipRatio - 0.0 * (b * slipRatio - qAtan(b * slipRatio))));
}

double GenericEngineModel::calculateTorque(double rpm, double throttle) const {
    double peakTorque = 400.0;
    double peakTorqueRpm = 4000.0;
    double maxRpm = 7500.0;

    if (rpm <= 0) return 0.0;
    if (rpm >= maxRpm) return 0.0;

    double normalized = rpm / peakTorqueRpm;
    double torque = peakTorque * (2.0 * normalized / (1.0 + normalized * normalized));
    return torque * throttle;
}

double GenericEngineModel::calculatePower(double rpm) const {
    double torque = calculateTorque(rpm, 1.0);
    return torque * rpm * 2.0 * M_PI / 60000.0; // kW
}

double GenericAeroModel::calculateDrag(double speed, double cd, double area) const {
    double airDensity = 1.225;
    return 0.5 * airDensity * speed * speed * cd * area;
}

double GenericAeroModel::calculateLift(double speed) const {
    return 0.0;
}

double GenericDiffModel::calculateTorqueSplit(double inputTorque, double slipRatio) const {
    return inputTorque * 0.5;
}

// ============================================================================


// DriverModel implementation
// ============================================================================

void DriverModel::update(float dt, float speed, float lateralAccel, float brakingForce, float corneringLoad) {
    m_state.totalTimeDriven += dt;

    float fatigueRate = 0.001f;
    if (speed > 200.0f) fatigueRate *= 2.0f;
    if (std::abs(lateralAccel) > 1.5f) fatigueRate *= 1.5f;
    if (std::abs(brakingForce) > 0.8f) fatigueRate *= 1.3f;
    m_state.fatigueLevel = std::clamp(m_state.fatigueLevel + fatigueRate * dt, 0.0f, 1.0f);

    float focusDecay = 0.0005f * dt;
    if (m_state.fatigueLevel > 0.5f) focusDecay *= 3.0f;
    m_state.focusLevel = std::clamp(m_state.focusLevel - focusDecay, 0.1f, 1.0f);

    float errorContrib = (1.0f - m_state.focusLevel) * 0.3f;
    errorContrib += m_state.fatigueLevel * 0.2f;
    errorContrib += (speed / 350.0f) * 0.1f;
    m_state.errorRate = std::clamp(errorContrib, 0.0f, 0.5f);
}

float DriverModel::calculateReactionDelay(float input) const {
    float baseDelay = m_state.reactionTime;
    float fatigueDelay = m_state.fatigueLevel * 0.15f;
    float focusPenalty = (1.0f - m_state.focusLevel) * 0.1f;
    float inputDelay = std::abs(input) * 0.05f;
    return baseDelay + fatigueDelay + focusPenalty + inputDelay;
}

float DriverModel::calculateFatigueEffect() const {
    float perfLoss = m_state.fatigueLevel * 0.15f;
    return std::clamp(1.0f - perfLoss, 0.7f, 1.0f);
}

float DriverModel::calculateErrorProbability() const {
    float prob = m_state.errorRate;
    prob += m_state.fatigueLevel * 0.15f;
    prob += (1.0f - m_state.focusLevel) * 0.2f;
    prob *= (1.0f - m_state.consistency * 0.5f);
    return std::clamp(prob, 0.0f, 1.0f);
}

DriverInput DriverModel::simulateDriverInput(float targetSpeed, float currentSpeed, float steeringTarget) const {
    DriverInput input;

    float speedError = targetSpeed - currentSpeed;
    float throttleDemand = speedError / 50.0f;
    input.throttle = std::clamp(throttleDemand, 0.0f, 1.0f);

    if (speedError < -10.0f) {
        input.brake = std::clamp(-speedError / 100.0f, 0.0f, 1.0f);
        input.throttle = 0;
    }

    input.steer = std::clamp(steeringTarget, -1.0f, 1.0f);

    float fatigueMod = calculateFatigueEffect();
    float quality = getDriverQuality();
    input.throttle *= fatigueMod * quality;
    input.steer *= quality;

    if (calculateErrorProbability() > 0.8f) {
        input.steer += (QRandomGenerator::global()->generateDouble() - 0.5) * 0.2f;
    }

    input.throttle = std::clamp(input.throttle, 0.0f, 1.0f);
    input.brake = std::clamp(input.brake, 0.0f, 1.0f);
    input.steer = std::clamp(input.steer, -1.0f, 1.0f);

    return input;
}

float DriverModel::getDriverQuality() const {
    float skill = m_state.aggressiveness * 0.2f + m_state.consistency * 0.4f + m_state.focusLevel * 0.4f;
    skill *= calculateFatigueEffect();
    return std::clamp(skill, 0.1f, 1.0f);
}

void DriverModel::reset() {
    m_state = DriverState();
}

// ============================================================================
// TireWearModel implementation
// ============================================================================

void TireWearModel::update(float dt, float slipAngle, float slipRatio, float normalLoad, float speed, float temp, int surfaceType) {
    m_ambientTemp = temp;

    for (int i = 0; i < 4; ++i) {
        float slip = std::sqrt(slipAngle * slipAngle + slipRatio * slipRatio);
        float wearRate = calculateWearRate(normalLoad, slip, m_state.surfaceTemp[i]);
        if (surfaceType == 1) wearRate *= 1.3f;
        if (surfaceType == 2) wearRate *= 0.8f;

        m_state.wearRate[i] = wearRate;
        m_state.totalWear[i] += wearRate * dt;
        m_state.totalWear[i] = std::clamp(m_state.totalWear[i], 0.0f, 1.0f);

        m_state.treadDepth[i] = 8.0f * (1.0f - m_state.totalWear[i]);

        m_state.gripFactor[i] = 1.0f - calculateGripReduction(m_state.totalWear[i]);
        m_state.gripFactor[i] *= calculateTemperatureEffect(m_state.surfaceTemp[i]);

        float heatGen = slip * normalLoad * 0.0001f;
        m_state.surfaceTemp[i] += (heatGen - (m_state.surfaceTemp[i] - temp) * 0.02f) * dt;
        m_state.surfaceTemp[i] = std::clamp(m_state.surfaceTemp[i], temp, 120.0f);

        m_state.coreTemp[i] += (m_state.surfaceTemp[i] - m_state.coreTemp[i]) * 0.005f * dt;
    }
}

float TireWearModel::calculateWearRate(float load, float slip, float temp) const {
    float loadFactor = load / 5000.0f;
    float slipFactor = 1.0f + slip * 5.0f;
    float tempFactor = 1.0f;
    if (temp > 80.0f) tempFactor = 1.0f + (temp - 80.0f) * 0.01f;
    if (temp < 40.0f) tempFactor = 0.8f;
    return 0.001f * loadFactor * slipFactor * tempFactor;
}

float TireWearModel::calculateGripReduction(float wear) const {
    float reduction = wear * 0.35f;
    if (wear > 0.7f) reduction += (wear - 0.7f) * 0.5f;
    return std::clamp(reduction, 0.0f, 0.5f);
}

float TireWearModel::calculateTemperatureEffect(float temp) const {
    float optimal = 80.0f;
    float diff = temp - optimal;
    return std::clamp(1.0f - diff * diff * 0.0001f, 0.7f, 1.0f);
}

void TireWearModel::resetWear() {
    for (int i = 0; i < 4; ++i) {
        m_state.treadDepth[i] = 8.0f;
        m_state.totalWear[i] = 0;
        m_state.wearRate[i] = 0;
        m_state.gripFactor[i] = 1.0f;
        m_state.surfaceTemp[i] = 30.0f;
        m_state.coreTemp[i] = 35.0f;
    }
}

// ============================================================================
// FuelManagementModel implementation
// ============================================================================

void FuelManagementModel::update(float dt, float throttlePosition, float rpm, float speed, float elevation) {
    float consumption = calculateFuelConsumption(throttlePosition, rpm);

    float elevationFactor = 1.0f + elevation * 0.0005f;
    consumption *= elevationFactor;

    m_state.fuelConsumptionRate = consumption;
    m_state.currentFuel -= consumption * dt;
    m_state.currentFuel = std::max(0.0f, m_state.currentFuel);
    m_state.fuelWeight = calculateFuelWeight();
    m_state.lapFuelUsage += consumption * dt;
}

float FuelManagementModel::calculateFuelConsumption(float throttle, float rpm) const {
    float throttleFactor = 0.3f + throttle * 0.7f;
    float rpmFactor = 0.5f + (rpm / 7500.0f) * 0.5f;
    return m_baseConsumption * throttleFactor * rpmFactor;
}

float FuelManagementModel::calculateFuelWeight() const {
    return m_state.currentFuel * 0.75f;
}

float FuelManagementModel::estimateFuelForLap(float speed, float aggressiveness) const {
    float baseLapFuel = m_baseConsumption * 90.0f;
    float speedFactor = 1.0f + (speed / 300.0f) * 0.2f;
    float aggrFactor = 1.0f + aggressiveness * 0.15f;
    return baseLapFuel * speedFactor * aggrFactor;
}

PitStopStrategy FuelManagementModel::calculateOptimalPitStrategy(int lapsRemaining, float fuelPerLap, int trackPosition) const {
    PitStopStrategy strategy;
    float fuelNeeded = lapsRemaining * fuelPerLap;

    if (fuelNeeded <= m_state.currentFuel) {
        strategy.optimalStrategy = true;
        strategy.estimatedTimeLoss = 0;
        return strategy;
    }

    int pitLap = lapsRemaining / 2;
    strategy.pitStopLaps.append(pitLap);
    strategy.fuelToAdd.append(std::min(fuelNeeded - m_state.currentFuel + 5.0f, m_state.fuelCapacity));
    strategy.estimatedTimeLoss = 22.0f;
    strategy.optimalStrategy = true;

    if (trackPosition <= 3) {
        strategy.pitStopLaps[0] = pitLap - 2;
    } else if (trackPosition > 10) {
        strategy.pitStopLaps[0] = pitLap + 2;
    }

    return strategy;
}

void FuelManagementModel::reset() {
    m_state = FuelState();
}

// ============================================================================
// ThermalModel implementation
// ============================================================================

void ThermalModel::update(float dt, float speed, float brakePressure, float slipRatio, float normalLoad, float ambientTemp) {
    m_state.ambientTemp = ambientTemp;
    m_state.trackTemp = ambientTemp + 10.0f;

    for (int i = 0; i < 4; ++i) {
        float tireHeatGen = calculateTireHeatGeneration(slipRatio, normalLoad, speed);
        float tireHeatDis = calculateTireHeatDissipation(m_state.tireSurfaceTemp[i], ambientTemp, speed);
        m_state.tireSurfaceTemp[i] += (tireHeatGen - tireHeatDis) * dt;
        m_state.tireSurfaceTemp[i] = std::clamp(m_state.tireSurfaceTemp[i], ambientTemp, 130.0f);

        m_state.tireCoreTemp[i] += (m_state.tireSurfaceTemp[i] - m_state.tireCoreTemp[i]) * 0.01f * dt;
        m_state.tireCoreTemp[i] = std::clamp(m_state.tireCoreTemp[i], ambientTemp, 110.0f);

        float brakeHeatGen = calculateBrakeHeatGeneration(brakePressure, speed);
        float brakeHeatDis = calculateBrakeHeatDissipation(m_state.brakeTemp[i], ambientTemp, speed);
        m_state.brakeTemp[i] += (brakeHeatGen - brakeHeatDis) * dt;
        m_state.brakeTemp[i] = std::clamp(m_state.brakeTemp[i], ambientTemp, 900.0f);

        m_state.brakeDiscTemp[i] += (m_state.brakeTemp[i] - m_state.brakeDiscTemp[i]) * 0.005f * dt;
        m_state.brakeDiscTemp[i] = std::clamp(m_state.brakeDiscTemp[i], ambientTemp, 1000.0f);
    }
}

float ThermalModel::calculateTireHeatGeneration(float slip, float load, float speed) const {
    float slipHeat = slip * slip * 500.0f;
    float loadFactor = load / 5000.0f;
    float speedFactor = speed * 0.01f;
    return slipHeat * loadFactor * (1.0f + speedFactor);
}

float ThermalModel::calculateTireHeatDissipation(float temp, float ambient, float speed) const {
    float tempDiff = temp - ambient;
    float convectiveCooling = tempDiff * 0.05f * (1.0f + speed * 0.01f);
    float radiativeCooling = tempDiff * 0.005f;
    return convectiveCooling + radiativeCooling;
}

float ThermalModel::calculateBrakeHeatGeneration(float brakePressure, float speed) const {
    float pressureFactor = brakePressure * 10.0f;
    float speedFactor = speed * 0.1f;
    return pressureFactor * speedFactor;
}

float ThermalModel::calculateBrakeHeatDissipation(float temp, float ambient, float speed) const {
    float tempDiff = temp - ambient;
    float airflow = 1.0f + speed * 0.02f;
    return tempDiff * 0.1f * airflow;
}

void ThermalModel::reset() {
    for (int i = 0; i < 4; ++i) {
        m_state.tireSurfaceTemp[i] = 30.0f;
        m_state.tireCoreTemp[i] = 35.0f;
        m_state.brakeTemp[i] = 300.0f;
        m_state.brakeDiscTemp[i] = 300.0f;
    }
}

// ============================================================================
// MultiBodyDynamics implementation
// ============================================================================

void MultiBodyDynamics::update(float dt, const QVector<QVector3D>& vehiclePositions, const QVector<QVector3D>& vehicleVelocities) {
    m_vehicleCount = vehiclePositions.size();
    m_wakeStrength = 0;

    for (int i = 0; i < m_vehicleCount; ++i) {
        if (vehicleVelocities[i].length() > 10.0f) {
            m_wakeStrength += vehicleVelocities[i].length() * 0.001f;
        }
    }
}

CollisionResult MultiBodyDynamics::checkCollision(const QVector3D& posA, const QVector3D& sizeA, const QVector3D& posB, const QVector3D& sizeB) const {
    CollisionResult result;

    float dx = std::abs(posA.x() - posB.x());
    float dy = std::abs(posA.y() - posB.y());
    float dz = std::abs(posA.z() - posB.z());

    float overlapX = (sizeA.x() + sizeB.x()) * 0.5f - dx;
    float overlapY = (sizeA.y() + sizeB.y()) * 0.5f - dy;
    float overlapZ = (sizeA.z() + sizeB.z()) * 0.5f - dz;

    if (overlapX > 0 && overlapY > 0 && overlapZ > 0) {
        result.collided = true;
        result.penetrationDepth = std::min({overlapX, overlapY, overlapZ});

        if (result.penetrationDepth == overlapX) {
            result.contactNormal = QVector3D(posA.x() > posB.x() ? 1.0f : -1.0f, 0, 0);
        } else if (result.penetrationDepth == overlapY) {
            result.contactNormal = QVector3D(0, posA.y() > posB.y() ? 1.0f : -1.0f, 0);
        } else {
            result.contactNormal = QVector3D(0, 0, posA.z() > posB.z() ? 1.0f : -1.0f);
        }

        result.contactPoint = (posA + posB) * 0.5f;
    }

    return result;
}

QVector3D MultiBodyDynamics::calculateAerodynamicWake(const QVector3D& downstreamPos, const QVector3D& upstreamVel) const {
    float wakeDecay = 0.95f;
    float length = downstreamPos.length();
    float decayFactor = std::pow(wakeDecay, length);
    return upstreamVel * decayFactor * 0.3f;
}

float MultiBodyDynamics::calculateSlipstreamEffect(const QVector3D& myPos, const QVector3D& leadPos, float leadSpeed) const {
    QVector3D delta = myPos - leadPos;
    float distance = delta.length();
    if (distance < 1.0f || distance > 200.0f) return 0;

    float alignment = QVector3D::dotProduct(delta.normalized(), QVector3D(1, 0, 0));
    alignment = std::max(alignment, 0.0f);

    float dragReduction = calculateDraftingBenefit(distance, leadSpeed);
    return dragReduction * alignment;
}

float MultiBodyDynamics::calculateDraftingBenefit(float distance, float speed) const {
    if (distance < 1.0f || distance > 200.0f) return 0;

    float optimalDist = 5.0f;
    float distFactor = std::exp(-(distance - optimalDist) * (distance - optimalDist) / 100.0f);
    float speedFactor = std::min(speed / 100.0f, 1.0f);

    return distFactor * speedFactor * 0.15f;
}

// ============================================================================
// TrackConditionsModel implementation
// ============================================================================

void TrackConditionsModel::update(float dt, const QVector<QVector3D>& vehiclePositions, int vehicleCount, int weather) {
    m_condition.rubberLevel += calculateDirtAccumulation(weather, vehicleCount) * dt * 0.001f;
    m_condition.rubberLevel = std::clamp(m_condition.rubberLevel, 0.0f, 1.0f);

    m_condition.dirtLevel += calculateDirtAccumulation(weather, vehicleCount) * dt * 0.0005f;
    m_condition.dirtLevel = std::clamp(m_condition.dirtLevel, 0.0f, 0.3f);

    m_condition.waterLevel -= calculateWaterEvaporation(m_condition.gripLevel, 5.0f, 0.5f) * dt;
    if (weather == 1) m_condition.waterLevel += 0.001f * dt;
    if (weather == 2) m_condition.waterLevel += 0.005f * dt;
    m_condition.waterLevel = std::clamp(m_condition.waterLevel, 0.0f, 0.5f);

    m_condition.gripLevel = calculateTrackGripModifier(m_condition.rubberLevel, m_condition.dirtLevel, m_condition.waterLevel);

    for (int i = 0; i < vehiclePositions.size(); ++i) {
        float buildup = calculateRubberBuildup(vehiclePositions[i], 50.0f, 4000.0f);
        RubberBuildup rb;
        rb.amount = buildup;
        rb.position = QVector2D(vehiclePositions[i].x(), vehiclePositions[i].y());
        rb.lapCount = 1;
        m_rubberMap.append(rb);
    }

    if (m_rubberMap.size() > 1000) {
        m_rubberMap.remove(0, 500);
    }
}

float TrackConditionsModel::calculateRubberBuildup(const QVector3D& position, float speed, float load) const {
    float speedFactor = speed / 200.0f;
    float loadFactor = load / 5000.0f;
    return 0.001f * speedFactor * loadFactor;
}

float TrackConditionsModel::calculateTrackGripModifier(float rubber, float dirt, float water) const {
    float rubberGrip = 1.0f + rubber * 0.1f;
    float dirtGrip = 1.0f - dirt * 0.3f;
    float waterGrip = 1.0f - water * 0.6f;
    return std::clamp(rubberGrip * dirtGrip * waterGrip, 0.3f, 1.2f);
}

float TrackConditionsModel::calculateDirtAccumulation(int weather, int traffic) const {
    float baseRate = 0.01f;
    if (weather == 1) baseRate *= 0.5f;
    if (weather == 2) baseRate *= 0.2f;
    float trafficFactor = 1.0f + traffic * 0.05f;
    return baseRate * trafficFactor;
}

float TrackConditionsModel::calculateWaterEvaporation(float temp, float wind, float sunExposure) const {
    float tempFactor = std::max(0.0f, (temp - 20.0f) * 0.01f);
    float windFactor = wind * 0.005f;
    float sunFactor = sunExposure * 0.02f;
    return tempFactor + windFactor + sunFactor;
}

void TrackConditionsModel::reset() {
    m_condition = TrackCondition();
    m_rubberMap.clear();
}

// ============================================================================
// WeatherEffectsModel implementation
// ============================================================================

void WeatherEffectsModel::update(float dt, const QVector3D& trackPosition, float vehicleSpeed, float vehicleHeading) {
    m_gripEffects.airDensity = calculateAirDensity(m_weatherState.temperature, m_weatherState.humidity, trackPosition.z());
    m_gripEffects.rainGripReduction = calculateRainGripReduction(m_weatherState.rainIntensity);
    m_gripEffects.temperatureGripModifier = calculateTemperatureEffect(m_weatherState.temperature);

    QVector3D windForce = calculateWindForce(
        m_weatherState.windSpeed, m_weatherState.windDirection,
        vehicleHeading, vehicleSpeed
    );
    m_gripEffects.windLateralForce = windForce.y();
    m_gripEffects.windLongitudinalForce = windForce.x();

    m_weatherState.temperature += (m_weatherState.sunIntensity * 0.001f - m_weatherState.rainIntensity * 0.002f) * dt;
}

float WeatherEffectsModel::calculateAirDensity(float temperature, float humidity, float altitude) const {
    float tempK = temperature + 273.15f;
    float pStd = 101325.0f;
    float p = pStd * std::exp(-altitude / 8500.0f);
    float Rd = 287.05f;
    float rho = p / (Rd * tempK);
    float humidityFactor = 1.0f - humidity * 0.003f;
    return rho * humidityFactor;
}

float WeatherEffectsModel::calculateRainGripReduction(float intensity) const {
    if (intensity <= 0) return 0;
    float reduction = intensity * 0.15f;
    return std::clamp(reduction, 0.0f, 0.6f);
}

QVector3D WeatherEffectsModel::calculateWindForce(float windSpeed, float windDir, float vehicleHeading, float vehicleSpeed) const {
    float relativeAngle = windDir - vehicleHeading;
    float headwind = windSpeed * std::cos(relativeAngle) - vehicleSpeed;
    float crosswind = windSpeed * std::sin(relativeAngle);

    float frontalArea = 2.0f;
    float sideArea = 4.0f;
    float rho = m_gripEffects.airDensity;

    float dragForce = 0.5f * rho * headwind * std::abs(headwind) * 0.35f * frontalArea;
    float lateralForce = 0.5f * rho * crosswind * std::abs(crosswind) * 0.8f * sideArea;

    return QVector3D(dragForce * 0.001f, lateralForce * 0.001f, 0);
}

float WeatherEffectsModel::calculateVisibility(float rainIntensity, float cloudCover) const {
    float vis = 1.0f;
    vis -= rainIntensity * 0.2f;
    vis -= cloudCover * 0.1f;
    return std::clamp(vis, 0.2f, 1.0f);
}

float WeatherEffectsModel::calculateTemperatureEffect(float ambientTemp) const {
    float optimal = 25.0f;
    float diff = ambientTemp - optimal;
    return std::clamp(1.0f - diff * diff * 0.0002f, 0.8f, 1.0f);
}

void WeatherEffectsModel::reset() {
    m_weatherState = WeatherStateEnv();
    m_gripEffects = GripEffects();
}

// ============================================================================
// RaceStrategyModel implementation
// ============================================================================

void RaceStrategyModel::update(float dt, const RaceState& raceState) {
    m_raceState = raceState;

    if (raceState.lastLapTime > 0) {
        m_avgLapTime = m_avgLapTime * 0.9f + raceState.lastLapTime * 0.1f;
    }

    float fuelPerLap = 1.8f * m_avgLapTime;
    float tireDegRate = 0;
    for (int i = 0; i < 4; ++i) {
        tireDegRate += raceState.tireWear[i];
    }
    tireDegRate /= 4.0f;

    QVector<int> pitWindow = calculatePitWindow(fuelPerLap, raceState.currentFuel, tireDegRate);

    m_recommendation.alternatives.clear();
    float bestTime = 1e9f;

    for (int pitLap : pitWindow) {
        float fuelToAdd = std::min(
            (m_raceState.totalLaps - pitLap) * fuelPerLap + 5.0f,
            80.0f
        );
        StrategyOption option = evaluateStrategyOption(
            pitLap, fuelToAdd, raceState.trackPosition, {}
        );
        m_recommendation.alternatives.append(option);

        if (option.estimatedTotalTime < bestTime) {
            bestTime = option.estimatedTotalTime;
            m_recommendation.bestOption = option;
        }
    }

    m_recommendation.confidence = 0.7f;
}

QVector<int> RaceStrategyModel::calculatePitWindow(float fuelPerLap, float currentFuel, float tireDegradationRate) const {
    QVector<int> window;

    if (fuelPerLap <= 0) return window;

    int lapsOnFuel = static_cast<int>(currentFuel / fuelPerLap);
    int earliestPit = std::max(1, lapsOnFuel - 5);
    int latestPit = std::max(earliestPit + 1, m_raceState.totalLaps - 5);

    int tireLimit = static_cast<int>(30.0f / std::max(tireDegradationRate, 0.01f));
    latestPit = std::min(latestPit, tireLimit);

    for (int lap = earliestPit; lap <= latestPit; lap += 3) {
        window.append(lap);
    }

    if (window.isEmpty()) {
        window.append(lapsOnFuel);
    }

    return window;
}

StrategyOption RaceStrategyModel::evaluateStrategyOption(int pitLap, float fuelToAdd, int trackPosition, const QVector<float>& competitors) const {
    StrategyOption option;
    option.pitLap = pitLap;
    option.fuelToAdd = fuelToAdd;

    int lapsAfterPit = m_raceState.totalLaps - pitLap;
    float fuelPerLap = m_avgLapTime > 0 ? m_baseConsumption * m_avgLapTime / 90.0f : 1.8f;
    float tireDegPerLap = 0.03f;

    option.tireAgeAtEnd = lapsAfterPit * tireDegPerLap;
    float pitTimeLoss = m_pitLossTime;
    float tirePenalty = option.tireAgeAtEnd * 0.1f * m_avgLapTime;

    option.estimatedTotalTime = m_avgLapTime * m_raceState.totalLaps + pitTimeLoss + tirePenalty;

    int pitStops = m_raceState.pitStopsCompleted + 1;
    float competitorAvgTime = m_avgLapTime * 1.02f;
    int competitorPitLoss = pitStops * static_cast<int>(m_pitLossTime);
    float competitorTotal = competitorAvgTime * m_raceState.totalLaps + competitorPitLoss;

    option.positionChange = (option.estimatedTotalTime < competitorTotal) ? -1 : 1;

    return option;
}

float RaceStrategyModel::calculateUndercutOpportunity(int myPitLap, int competitorPitLap) const {
    if (competitorPitLap <= myPitLap) return 0;

    int lapsAhead = competitorPitLap - myPitLap;
    float timeGain = lapsAhead * 0.5f;
    float pitLoss = m_pitLossTime * 0.3f;
    return std::max(0.0f, timeGain - pitLoss);
}

float RaceStrategyModel::calculateOvercutOpportunity(int myPitLap, int competitorPitLap) const {
    if (myPitLap <= competitorPitLap) return 0;

    int lapsAhead = myPitLap - competitorPitLap;
    float freshTireAdvantage = lapsAhead * 0.3f;
    float wornTirePenalty = lapsAhead * 0.1f;
    return freshTireAdvantage - wornTirePenalty;
}

void RaceStrategyModel::reset() {
    m_raceState = RaceState();
    m_recommendation = StrategyRecommendation();
    m_avgLapTime = 90.0f;
}


// ============================================================================
// TelemetryProcessingModel implementation
// ============================================================================

TelemetrySession TelemetryProcessingModel::loadSession(const QString& filePath) const {
    TelemetrySession session;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "TelemetryProcessingModel: Cannot open file" << filePath;
        return session;
    }

    QTextStream stream(&file);
    while (!stream.atEnd()) {
        QString line = stream.readLine();
        QStringList parts = line.split(',');
        if (parts.size() < 2) continue;

        TelemetryChannel channel;
        channel.name = parts[0].trimmed();
        channel.sampleRate = parts.size() > 1 ? parts[1].toInt() : 100;

        for (int i = 2; i < parts.size(); ++i) {
            bool ok = false;
            float val = parts[i].toFloat(&ok);
            if (ok) channel.samples.append(val);
        }

        if (!channel.samples.isEmpty()) {
            channel.minValue = *std::min_element(channel.samples.begin(), channel.samples.end());
            channel.maxValue = *std::max_element(channel.samples.begin(), channel.samples.end());
            channel.currentValue = channel.samples.last();
        }
        session.channels.append(channel);
    }

    return session;
}

bool TelemetryProcessingModel::saveSession(const TelemetrySession& session, const QString& filePath) const {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "TelemetryProcessingModel: Cannot write file" << filePath;
        return false;
    }

    QTextStream stream(&file);
    stream << "Driver," << session.driverName << "\n";
    stream << "Track," << session.trackName << "\n";
    stream << "Laps," << session.totalLaps << "\n";
    stream << "BestLap," << session.bestLapTime << "\n";

    for (const auto& channel : session.channels) {
        stream << channel.name << "," << channel.sampleRate;
        for (float sample : channel.samples) {
            stream << "," << sample;
        }
        stream << "\n";
    }

    return true;
}

ChannelStatistics TelemetryProcessingModel::calculateChannelStatistics(const TelemetryChannel& channel) const {
    ChannelStatistics stats;
    if (channel.samples.isEmpty()) return stats;

    stats.mean = calculateMean(channel.samples);
    stats.rms = calculateRms(channel.samples);
    stats.minValue = *std::min_element(channel.samples.begin(), channel.samples.end());
    stats.maxValue = *std::max_element(channel.samples.begin(), channel.samples.end());

    float sumSqDiff = 0;
    for (float val : channel.samples) {
        float diff = val - stats.mean;
        sumSqDiff += diff * diff;
    }
    stats.stdDev = std::sqrt(sumSqDiff / static_cast<float>(channel.samples.size()));

    return stats;
}

QVector<float> TelemetryProcessingModel::smoothChannel(const TelemetryChannel& channel, int windowSize) const {
    QVector<float> smoothed;
    if (channel.samples.isEmpty() || windowSize <= 0) return smoothed;

    int halfWindow = windowSize / 2;
    smoothed.resize(channel.samples.size());

    for (int i = 0; i < channel.samples.size(); ++i) {
        float sum = 0;
        int count = 0;
        for (int j = std::max(0, i - halfWindow); j <= std::min(static_cast<int>(channel.samples.size()) - 1, i + halfWindow); ++j) {
            sum += channel.samples[j];
            count++;
        }
        smoothed[i] = (count > 0) ? sum / static_cast<float>(count) : channel.samples[i];
    }

    return smoothed;
}

QVector<int> TelemetryProcessingModel::detectPeaks(const TelemetryChannel& channel, float threshold) const {
    QVector<int> peaks;
    if (channel.samples.size() < 3) return peaks;

    for (int i = 1; i < channel.samples.size() - 1; ++i) {
        if (channel.samples[i] > channel.samples[i - 1] &&
            channel.samples[i] > channel.samples[i + 1] &&
            channel.samples[i] >= threshold) {
            peaks.append(i);
        }
    }

    return peaks;
}

float TelemetryProcessingModel::calculateCrossCorrelation(const TelemetryChannel& ch1, const TelemetryChannel& ch2) const {
    int n = std::min(ch1.samples.size(), ch2.samples.size());
    if (n == 0) return 0;

    float mean1 = calculateMean(ch1.samples);
    float mean2 = calculateMean(ch2.samples);

    float sum = 0;
    float sumSq1 = 0;
    float sumSq2 = 0;

    for (int i = 0; i < n; ++i) {
        float d1 = ch1.samples[i] - mean1;
        float d2 = ch2.samples[i] - mean2;
        sum += d1 * d2;
        sumSq1 += d1 * d1;
        sumSq2 += d2 * d2;
    }

    float denom = std::sqrt(sumSq1 * sumSq2);
    return (denom > 1e-10f) ? sum / denom : 0;
}

QVector<float> TelemetryProcessingModel::calculateFFT(const TelemetryChannel& channel) const {
    QVector<float> magnitudes;
    int n = channel.samples.size();
    if (n < 2) return magnitudes;

    magnitudes.resize(n / 2);

    for (int k = 0; k < n / 2; ++k) {
        float real = 0, imag = 0;
        for (int t = 0; t < n; ++t) {
            float angle = -2.0f * M_PI * static_cast<float>(k) * static_cast<float>(t) / static_cast<float>(n);
            real += channel.samples[t] * std::cos(angle);
            imag += channel.samples[t] * std::sin(angle);
        }
        magnitudes[k] = std::sqrt(real * real + imag * imag) / static_cast<float>(n);
    }

    return magnitudes;
}

QVector<CornerAnalysis> TelemetryProcessingModel::segmentLapIntoCorners(const TelemetrySession& session) const {
    QVector<CornerAnalysis> corners;
    if (session.channels.isEmpty()) return corners;

    for (const auto& channel : session.channels) {
        if (channel.name.contains("speed", Qt::CaseInsensitive) ||
            channel.name.contains("velocity", Qt::CaseInsensitive)) {
            QVector<int> peaks = detectPeaks(channel, channel.minValue * 1.5f);

            for (int i = 0; i < peaks.size(); ++i) {
                CornerAnalysis corner;
                corner.cornerNumber = i + 1;

                int peakIdx = peaks[i];
                corner.entrySpeed = (peakIdx > 0) ? channel.samples[peakIdx - 1] : channel.samples[peakIdx];
                corner.midSpeed = channel.samples[peakIdx];
                corner.exitSpeed = (peakIdx < channel.samples.size() - 1) ? channel.samples[peakIdx + 1] : channel.samples[peakIdx];

                float minVal = channel.samples[peakIdx];
                for (int j = std::max(0, peakIdx - 5); j <= std::min(static_cast<int>(channel.samples.size()) - 1, peakIdx + 5); ++j) {
                    minVal = std::min(minVal, channel.samples[j]);
                }
                corner.minSpeed = minVal;

                corners.append(corner);
            }
            break;
        }
    }

    return corners;
}

float TelemetryProcessingModel::compareLaps(const TelemetrySession& lap1, const TelemetrySession& lap2) const {
    if (lap1.channels.size() != lap2.channels.size() || lap1.channels.isEmpty()) return 0;

    float totalCorrelation = 0;
    int channelCount = lap1.channels.size();

    for (int i = 0; i < channelCount; ++i) {
        totalCorrelation += calculateCrossCorrelation(lap1.channels[i], lap2.channels[i]);
    }

    return std::clamp(totalCorrelation / static_cast<float>(channelCount), 0.0f, 1.0f);
}

QVector<float> TelemetryProcessingModel::filterNoise(const TelemetryChannel& channel, float cutoffFreq) const {
    QVector<float> filtered = channel.samples;
    if (filtered.size() < 3 || channel.sampleRate <= 0) return filtered;

    float nyquist = static_cast<float>(channel.sampleRate) * 0.5f;
    float normalizedCutoff = std::clamp(cutoffFreq / nyquist, 0.01f, 0.99f);

    float alpha = normalizedCutoff / (normalizedCutoff + 1.0f);
    filtered[0] = channel.samples[0];

    for (int i = 1; i < filtered.size(); ++i) {
        filtered[i] = alpha * channel.samples[i] + (1.0f - alpha) * filtered[i - 1];
    }

    return filtered;
}

float TelemetryProcessingModel::calculateMean(const QVector<float>& data) const {
    if (data.isEmpty()) return 0;
    float sum = 0;
    for (float val : data) sum += val;
    return sum / static_cast<float>(data.size());
}

float TelemetryProcessingModel::calculateRms(const QVector<float>& data) const {
    if (data.isEmpty()) return 0;
    float sumSq = 0;
    for (float val : data) sumSq += val * val;
    return std::sqrt(sumSq / static_cast<float>(data.size()));
}

// ============================================================================
// VehiclePerformanceAnalysisModel implementation
// ============================================================================

PerformanceMetrics VehiclePerformanceAnalysisModel::analyzeTelemetry(const TelemetrySession& session) const {
    PerformanceMetrics metrics;

    for (const auto& channel : session.channels) {
        if (channel.name.contains("speed", Qt::CaseInsensitive)) {
            metrics.topSpeed = channel.maxValue;
        }
        if (channel.name.contains("throttle", Qt::CaseInsensitive)) {
            float avgThrottle = 0;
            for (float val : channel.samples) avgThrottle += val;
            avgThrottle /= std::max(1, static_cast<int>(channel.samples.size()));
            metrics.fuelEfficiency = 100.0f - avgThrottle * 30.0f;
        }
        if (channel.name.contains("lateral_g", Qt::CaseInsensitive)) {
            metrics.lateralGMax = channel.maxValue;
        }
        if (channel.name.contains("longitudinal_g", Qt::CaseInsensitive)) {
            metrics.longitudinalGMax = channel.maxValue;
        }
    }

    metrics.acceleration0_100 = metrics.topSpeed * 0.3f;
    metrics.acceleration0_200 = metrics.topSpeed * 0.7f;
    metrics.brakingDistance100_0 = (100.0f * 100.0f) / (2.0f * 9.81f * metrics.longitudinalGMax + 1.0f);
    metrics.lapTimePrediction = 90.0f;
    metrics.tireLifeEstimate = 30.0f;

    return metrics;
}

QVector<PerformanceMap> VehiclePerformanceAnalysisModel::calculatePerformanceMap(const TelemetrySession& session) const {
    QVector<PerformanceMap> perfMap;
    if (session.channels.size() < 2) return perfMap;

    int sampleCount = session.channels[0].samples.size();
    for (int i = 0; i < sampleCount; ++i) {
        PerformanceMap point;
        point.speed = session.channels[0].samples[i];
        point.throttle = (session.channels.size() > 1 && i < session.channels[1].samples.size())
                         ? session.channels[1].samples[i] : 0;
        point.lateralG = (session.channels.size() > 2 && i < session.channels[2].samples.size())
                         ? session.channels[2].samples[i] : 0;
        point.efficiency = point.speed > 0 ? 1.0f / (1.0f + point.throttle * 0.3f) : 0;
        perfMap.append(point);
    }

    return perfMap;
}

float VehiclePerformanceAnalysisModel::predictLapTime(const BalanceSetup& setup, const ConditionState& conditions) const {
    float baseLapTime = 90.0f;

    float springEffect = (setup.springRateFront + setup.springRateRear) / 50000.0f * 2.0f;
    float camberEffect = (std::abs(setup.camberFront) + std::abs(setup.camberRear)) * 0.1f;
    float gripModifier = conditions.gripCoefficient;

    float predicted = baseLapTime / gripModifier + springEffect - camberEffect;
    return std::max(predicted, baseLapTime * 0.85f);
}

PerformanceDelta VehiclePerformanceAnalysisModel::calculatePerformanceDelta(const PerformanceMetrics& baseline, const PerformanceMetrics& comparison) const {
    PerformanceDelta delta;
    delta.speedDelta = comparison.topSpeed - baseline.topSpeed;
    delta.timeDelta = baseline.lapTimePrediction - comparison.lapTimePrediction;
    delta.gForceDelta = (comparison.lateralGMax + comparison.longitudinalGMax)
                        - (baseline.lateralGMax + baseline.longitudinalGMax);
    delta.fuelDelta = comparison.fuelEfficiency - baseline.fuelEfficiency;
    return delta;
}

QString VehiclePerformanceAnalysisModel::identifyLimitingFactor(const TelemetrySession& session) const {
    PerformanceMetrics metrics = analyzeTelemetry(session);

    if (metrics.lateralGMax < 1.0f) return "Tire grip (lateral)";
    if (metrics.topSpeed < 200.0f) return "Engine power / drag";
    if (metrics.brakingDistance100_0 > 50.0f) return "Brake performance";
    if (metrics.fuelEfficiency < 50.0f) return "Fuel efficiency";
    return "Overall balance";
}

QVector<float> VehiclePerformanceAnalysisModel::calculateOptimalThrottleProfile(const TelemetrySession& session) const {
    QVector<float> profile;
    for (const auto& channel : session.channels) {
        if (channel.name.contains("speed", Qt::CaseInsensitive)) {
            for (int i = 0; i < channel.samples.size(); ++i) {
                float speedRatio = channel.samples[i] / std::max(channel.maxValue, 1.0f);
                profile.append(speedRatio > 0.5f ? 1.0f : speedRatio * 2.0f);
            }
            break;
        }
    }
    return profile;
}

QVector<float> VehiclePerformanceAnalysisModel::calculateOptimalBrakeProfile(const TelemetrySession& session) const {
    QVector<float> profile;
    for (const auto& channel : session.channels) {
        if (channel.name.contains("speed", Qt::CaseInsensitive)) {
            for (int i = 1; i < channel.samples.size(); ++i) {
                float decel = (channel.samples[i - 1] - channel.samples[i]) / std::max(channel.samples[i - 1], 1.0f);
                profile.append(std::clamp(decel, 0.0f, 1.0f));
            }
            profile.prepend(0);
            break;
        }
    }
    return profile;
}

QVector<float> VehiclePerformanceAnalysisModel::calculateSpeedTrace(const TelemetrySession& session) const {
    QVector<float> trace;
    for (const auto& channel : session.channels) {
        if (channel.name.contains("speed", Qt::CaseInsensitive)) {
            trace = channel.samples;
            break;
        }
    }
    return trace;
}

QString VehiclePerformanceAnalysisModel::generatePerformanceReport(const TelemetrySession& session) const {
    PerformanceMetrics m = analyzeTelemetry(session);
    QString report;
    report += QString("Top Speed: %1 km/h\n").arg(m.topSpeed, 0, 'f', 1);
    report += QString("0-100: %1 s\n").arg(m.acceleration0_100, 0, 'f', 2);
    report += QString("0-200: %1 s\n").arg(m.acceleration0_200, 0, 'f', 2);
    report += QString("Braking 100-0: %1 m\n").arg(m.brakingDistance100_0, 0, 'f', 1);
    report += QString("Max Lat G: %1\n").arg(m.lateralGMax, 0, 'f', 2);
    report += QString("Max Long G: %1\n").arg(m.longitudinalGMax, 0, 'f', 2);
    report += QString("Est. Lap Time: %1 s\n").arg(m.lapTimePrediction, 0, 'f', 3);
    report += QString("Fuel Efficiency: %1\n").arg(m.fuelEfficiency, 0, 'f', 1);
    report += QString("Limiting Factor: %1\n").arg(identifyLimitingFactor(session));
    return report;
}

// ============================================================================
// SetupOptimizationModel implementation
// ============================================================================

QVector<OptimizationRecommendation> SetupOptimizationModel::analyzeSetupEffect(const BalanceSetup& setup, const TelemetrySession& telemetry) const {
    QVector<OptimizationRecommendation> recs;

    float targetFrontFreq = 2.5f;
    float targetRearFreq = 2.8f;
    float mass = 1500.0f;
    float wb = 2.7f;

    float frontFreq = std::sqrt(setup.springRateFront / (mass * 0.5f)) / (2.0f * M_PI);
    if (std::abs(frontFreq - targetFrontFreq) > 0.3f) {
        OptimizationRecommendation rec;
        rec.parameter = "Front Spring Rate";
        rec.suggestedValue = targetFrontFreq * targetFrontFreq * mass * 0.5f * 4.0f * M_PI * M_PI;
        rec.expectedImprovement = 0.2f;
        rec.confidence = 0.7f;
        rec.reason = "Front natural frequency deviates from optimal";
        recs.append(rec);
    }

    if (std::abs(setup.camberFront - (-3.0f)) > 1.0f) {
        OptimizationRecommendation rec;
        rec.parameter = "Front Camber";
        rec.suggestedValue = -3.0f;
        rec.expectedImprovement = 0.15f;
        rec.confidence = 0.8f;
        rec.reason = "Front camber not optimized for cornering grip";
        recs.append(rec);
    }

    return recs;
}

QVector<OptimizationRecommendation> SetupOptimizationModel::suggestSetupChanges(const BalanceSetup& currentSetup, const PerformanceMetrics& targetPerformance) const {
    QVector<OptimizationRecommendation> recs;

    if (targetPerformance.lateralGMax > 1.3f) {
        OptimizationRecommendation rec;
        rec.parameter = "Spring Rate";
        rec.suggestedValue = currentSetup.springRateFront * 1.1f;
        rec.expectedImprovement = 0.1f;
        rec.confidence = 0.6f;
        rec.reason = "Higher lateral G target requires stiffer springs";
        recs.append(rec);
    }

    if (targetPerformance.topSpeed > 280.0f) {
        OptimizationRecommendation rec;
        rec.parameter = "Aero Balance";
        rec.suggestedValue = 0.45f;
        rec.expectedImprovement = 0.05f;
        rec.confidence = 0.5f;
        rec.reason = "High top speed target requires less rear downforce";
        recs.append(rec);
    }

    return recs;
}

SetupComparison SetupOptimizationModel::compareSetups(const BalanceSetup& setupA, const BalanceSetup& setupB, const ConditionState& trackConditions) const {
    SetupComparison comp;
    float gripA = trackConditions.gripCoefficient;
    float gripB = trackConditions.gripCoefficient;

    float rollA = (setupA.antiRollBarFront + setupA.antiRollBarRear) / 2.0f;
    float rollB = (setupB.antiRollBarFront + setupB.antiRollBarRear) / 2.0f;

    comp.lapTimeDelta = (rollB - rollA) * 0.001f;
    comp.straightLineDelta = (setupB.tirePressureFront - setupA.tirePressureFront) * 0.5f;
    comp.corneringDelta = (setupB.springRateFront - setupA.springRateFront) * 0.0001f;
    comp.brakingDelta = (setupB.camberFront - setupA.camberFront) * 0.02f;
    comp.stabilityDelta = (rollA - rollB) * 0.002f;

    return comp;
}

QVector<float> SetupOptimizationModel::calculateOptimalSprings(float mass, float wheelbase, const QString& track) const {
    float frontLoad = mass * 0.5f;
    float rearLoad = mass * 0.5f;
    float targetFreq = 2.5f;

    float frontRate = frontLoad * 4.0f * M_PI * M_PI * targetFreq * targetFreq;
    float rearRate = rearLoad * 4.0f * M_PI * M_PI * (targetFreq + 0.3f) * (targetFreq + 0.3f);

    if (track.contains("Monaco") || track.contains("street")) {
        frontRate *= 1.15f;
        rearRate *= 1.15f;
    } else if (track.contains("Monza") || track.contains("speed")) {
        frontRate *= 0.9f;
        rearRate *= 0.9f;
    }

    return {frontRate, rearRate, frontRate, rearRate};
}

QVector<float> SetupOptimizationModel::calculateOptimalDampers(const BalanceSetup& setup, const QString& track) const {
    float bumpFront = setup.damperBumpFront;
    float reboundFront = setup.damperReboundFront;
    float bumpRear = setup.damperBumpRear;
    float reboundRear = setup.damperReboundRear;

    if (track.contains("bumpy")) {
        bumpFront *= 0.85f;
        bumpRear *= 0.85f;
    }

    return {bumpFront, bumpRear, reboundFront, reboundRear};
}

SetupOptimizationModel::AeroBalanceResult SetupOptimizationModel::calculateOptimalAero(float speed, float mass, const QString& track) const {
    AeroBalanceResult result;
    float downforce = mass * 0.5f;
    result.frontDownforce = downforce * 0.48f;
    result.rearDownforce = downforce * 0.52f;
    result.balance = result.frontDownforce / (result.frontDownforce + result.rearDownforce);

    if (speed > 250.0f) {
        result.frontDownforce *= 0.95f;
        result.rearDownforce *= 1.05f;
        result.balance = result.frontDownforce / (result.frontDownforce + result.rearDownforce);
    }

    return result;
}

QVector<float> SetupOptimizationModel::calculateOptimalCamber(float suspensionTravel, float gripTarget, const QString& track) const {
    float baseCamber = -3.0f;
    if (track.contains("street")) baseCamber = -2.0f;
    if (track.contains("speed")) baseCamber = -1.5f;

    float travelAdjust = suspensionTravel * 0.1f;
    float frontCamber = baseCamber - travelAdjust;
    float rearCamber = baseCamber + 0.5f - travelAdjust;

    return {frontCamber, frontCamber, rearCamber, rearCamber};
}

QVector<float> SetupOptimizationModel::calculateOptimalToe(float speed, const QString& track) const {
    float frontToe = 0.1f;
    float rearToe = -0.1f;

    if (speed > 200.0f) {
        frontToe *= 0.5f;
        rearToe *= 0.5f;
    }

    return {frontToe, -frontToe, rearToe, -rearToe};
}

QString SetupOptimizationModel::generateSetupReport(const BalanceSetup& setup, const TelemetrySession& telemetry) const {
    QString report;
    report += "=== Setup Report ===\n\n";
    report += QString("Front Springs: %1 N/m\n").arg(setup.springRateFront);
    report += QString("Rear Springs: %1 N/m\n").arg(setup.springRateRear);
    report += QString("Front Dampers (Bump/Rebound): %1 / %2\n").arg(setup.damperBumpFront).arg(setup.damperReboundFront);
    report += QString("Rear Dampers (Bump/Rebound): %1 / %2\n").arg(setup.damperBumpRear).arg(setup.damperReboundRear);
    report += QString("Front Camber: %1 deg\n").arg(setup.camberFront);
    report += QString("Rear Camber: %1 deg\n").arg(setup.camberRear);
    report += QString("Front Toe: %1 deg\n").arg(setup.toeFront);
    report += QString("Rear Toe: %1 deg\n").arg(setup.toeRear);

    auto recs = analyzeSetupEffect(setup, telemetry);
    if (!recs.isEmpty()) {
        report += "\n--- Recommendations ---\n";
        for (const auto& rec : recs) {
            report += QString("[%1] %2 -> %3 (%4)\n").arg(rec.parameter, rec.reason)
                         .arg(rec.suggestedValue, 0, 'f', 2).arg(rec.expectedImprovement, 0, 'f', 2);
        }
    }

    return report;
}

// ============================================================================
// TireTestingModel implementation
// ============================================================================

QVector<TireTestData> TireTestingModel::performCorneringTest(float maxSlipAngle, float load) const {
    QVector<TireTestData> data;
    const int steps = 20;
    const float peakSlip = 8.0f;
    const float peakForce = load * 1.2f;

    for (int i = 0; i <= steps; ++i) {
        TireTestData point;
        point.slipAngle = maxSlipAngle * static_cast<float>(i) / static_cast<float>(steps);
        point.normalLoad = load;

        if (point.slipAngle <= peakSlip) {
            point.lateralForce = peakForce * (point.slipAngle / peakSlip);
        } else {
            point.lateralForce = peakForce * std::exp(-(point.slipAngle - peakSlip) * 0.1f);
        }

        point.temperature = 60.0f + point.slipAngle * 2.0f;
        data.append(point);
    }

    return data;
}

QVector<TireTestData> TireTestingModel::performLongitudinalTest(float maxSlipRatio, float load) const {
    QVector<TireTestData> data;
    const int steps = 20;
    const float peakSlip = 0.10f;
    const float peakForce = load * 1.1f;

    for (int i = 0; i <= steps; ++i) {
        TireTestData point;
        point.slipRatio = maxSlipRatio * static_cast<float>(i) / static_cast<float>(steps);
        point.normalLoad = load;

        if (point.slipRatio <= peakSlip) {
            point.longitudinalForce = peakForce * (point.slipRatio / peakSlip);
        } else {
            point.longitudinalForce = peakForce * std::exp(-(point.slipRatio - peakSlip) * 10.0f);
        }

        point.temperature = 60.0f + point.slipRatio * 100.0f;
        data.append(point);
    }

    return data;
}

QVector<TireTestData> TireTestingModel::performBrakeTest(float maxSlipRatio, float load) const {
    QVector<TireTestData> data;
    const int steps = 15;
    const float peakSlip = 0.08f;
    const float peakForce = load * 1.3f;

    for (int i = 0; i <= steps; ++i) {
        TireTestData point;
        point.slipRatio = maxSlipRatio * static_cast<float>(i) / static_cast<float>(steps);
        point.normalLoad = load;

        if (point.slipRatio <= peakSlip) {
            point.longitudinalForce = peakForce * (point.slipRatio / peakSlip);
        } else {
            point.longitudinalForce = peakForce * std::exp(-(point.slipRatio - peakSlip) * 15.0f);
        }

        data.append(point);
    }

    return data;
}

QVector<TireTestData> TireTestingModel::performDriveTest(float maxSlipRatio, float load) const {
    QVector<TireTestData> data;
    const int steps = 15;
    const float peakSlip = 0.12f;
    const float peakForce = load * 1.0f;

    for (int i = 0; i <= steps; ++i) {
        TireTestData point;
        point.slipRatio = maxSlipRatio * static_cast<float>(i) / static_cast<float>(steps);
        point.normalLoad = load;

        if (point.slipRatio <= peakSlip) {
            point.longitudinalForce = peakForce * (point.slipRatio / peakSlip);
        } else {
            point.longitudinalForce = peakForce * std::exp(-(point.slipRatio - peakSlip) * 8.0f);
        }

        data.append(point);
    }

    return data;
}

TireTestResult TireTestingModel::analyzeTestData(const QVector<TireTestData>& data) const {
    TireTestResult result;
    if (data.isEmpty()) return result;

    float maxLat = 0, maxLong = 0;
    float peakSlipAngle = 0, peakSlipRatio = 0;
    float sumLatSlip = 0, sumLatForce = 0;
    int countLat = 0;

    for (const auto& point : data) {
        if (point.lateralForce > maxLat) {
            maxLat = point.lateralForce;
            peakSlipAngle = point.slipAngle;
        }
        if (point.longitudinalForce > maxLong) {
            maxLong = point.longitudinalForce;
            peakSlipRatio = point.slipRatio;
        }
        if (point.slipAngle > 0 && point.slipAngle < 5.0f) {
            sumLatSlip += point.slipAngle;
            sumLatForce += point.lateralForce;
            countLat++;
        }
    }

    result.peakLateralForce = maxLat;
    result.peakLongitudinalForce = maxLong;
    result.peakSlipAngle = peakSlipAngle;
    result.peakSlipRatio = peakSlipRatio;
    result.corneringStiffness = (countLat > 0 && sumLatSlip > 0) ? sumLatForce / sumLatSlip : 0;
    result.frictionCircle = std::sqrt(maxLat * maxLat + maxLong * maxLong);

    return result;
}

float TireTestingModel::calculateFrictionCircle(const QVector<TireTestData>& testData) const {
    float maxLat = 0, maxLong = 0;
    for (const auto& point : testData) {
        maxLat = std::max(maxLat, std::abs(point.lateralForce));
        maxLong = std::max(maxLong, std::abs(point.longitudinalForce));
    }
    return std::sqrt(maxLat * maxLat + maxLong * maxLong);
}

float TireTestingModel::calculateTireStiffness(const QVector<TireTestData>& testData) const {
    if (testData.size() < 2) return 0;

    float sumSlip = 0, sumForce = 0;
    int count = 0;
    for (const auto& point : testData) {
        if (point.slipAngle > 0 && point.slipAngle < 3.0f) {
            sumSlip += point.slipAngle;
            sumForce += point.lateralForce;
            count++;
        }
    }

    return (count > 0 && sumSlip > 0) ? sumForce / sumSlip : 0;
}

QVector<float> TireTestingModel::fitPacejkaCoefficients(const QVector<TireTestData>& testData) const {
    QVector<float> coefficients(11, 0);
    if (testData.isEmpty()) return coefficients;

    float maxForce = 0;
    float peakSlip = 0;
    for (const auto& point : testData) {
        if (point.lateralForce > maxForce) {
            maxForce = point.lateralForce;
            peakSlip = point.slipAngle;
        }
    }

    coefficients[0] = 10.0f;
    coefficients[1] = 1.0f;
    coefficients[2] = 1000.0f;
    coefficients[3] = maxForce;
    coefficients[4] = peakSlip;
    coefficients[5] = 0.0f;
    coefficients[6] = 0.0f;
    coefficients[7] = 0.0f;
    coefficients[8] = 0.0f;
    coefficients[9] = 0.0f;
    coefficients[10] = 0.0f;

    return coefficients;
}

TireCharacterization TireTestingModel::characterizeTire(const QVector<TireTestResult>& testResults) const {
    TireCharacterization characterization;

    if (!testResults.isEmpty()) {
        characterization.dry = testResults[0];
        characterization.wet = testResults.size() > 1 ? testResults[1] : testResults[0];
        characterization.cold = testResults.size() > 2 ? testResults[2] : testResults[0];
        characterization.hot = testResults.size() > 3 ? testResults[3] : testResults[0];

        characterization.thermalDegRate = (characterization.hot.peakLateralForce > 0)
            ? (characterization.dry.peakLateralForce - characterization.hot.peakLateralForce) / characterization.dry.peakLateralForce * 10.0f
            : 0;
        characterization.wearRate = 0.05f;
    }

    return characterization;
}

// ============================================================================
// AerodynamicTestingModel implementation
// ============================================================================

AeroTestData AerodynamicTestingModel::calculateAeroForces(float speed, float rideHeightFront, float rideHeightRear, float yawAngle) const {
    AeroTestData data;
    data.speed = speed;
    data.yawAngle = yawAngle;
    data.rideHeightFront = rideHeightFront;
    data.rideHeightRear = rideHeightRear;

    float rho = 1.225f;
    float q = 0.5f * rho * speed * speed;

    float groundEffectFront = std::exp(-rideHeightFront * 20.0f);
    float groundEffectRear = std::exp(-rideHeightRear * 15.0f);

    data.frontLiftCoeff = -(0.8f + groundEffectFront * 2.0f);
    data.rearLiftCoeff = -(1.0f + groundEffectRear * 2.5f);
    data.dragCoeff = 0.35f + yawAngle * yawAngle * 0.001f;

    float area = 2.0f;
    data.downforceFront = data.frontLiftCoeff * q * area * 0.5f;
    data.downforceRear = data.rearLiftCoeff * q * area * 0.5f;
    data.dragForce = data.dragCoeff * q * area;

    return data;
}

QVector<AeroTestData> AerodynamicTestingModel::performWindTunnelTest(const QVector<float>& speedRange, const QVector<float>& rideHeightRange) const {
    QVector<AeroTestData> results;

    for (float speed : speedRange) {
        for (float rideHeight : rideHeightRange) {
            AeroTestData data = calculateAeroForces(speed, rideHeight, rideHeight * 1.2f, 0);
            results.append(data);
        }
    }

    return results;
}

AeroTestResult AerodynamicTestingModel::analyzeAeroData(const QVector<AeroTestData>& data) const {
    AeroTestResult result;
    if (data.isEmpty()) return result;

    float bestRatio = 0;
    float maxDownforce = 0;
    float minDrag = 1e9f;

    for (const auto& point : data) {
        float totalDownforce = std::abs(point.downforceFront) + std::abs(point.downforceRear);
        float ratio = (point.dragForce > 0) ? totalDownforce / point.dragForce : 0;

        if (ratio > bestRatio) {
            bestRatio = ratio;
            result.optimalRideHeight = point.rideHeightFront;
            result.balancePoint = std::abs(point.downforceFront) / std::max(totalDownforce, 0.01f);
        }
        maxDownforce = std::max(maxDownforce, totalDownforce);
        minDrag = std::min(minDrag, point.dragForce);
    }

    result.maxDownforce = maxDownforce;
    result.minDrag = minDrag;
    result.bestLiftDragRatio = bestRatio;

    return result;
}

float AerodynamicTestingModel::calculateAeroBalance(float frontDownforce, float rearDownforce) const {
    float total = std::abs(frontDownforce) + std::abs(rearDownforce);
    return (total > 0) ? std::abs(frontDownforce) / total : 0.5f;
}

float AerodynamicTestingModel::calculateLiftDragRatio(float downforce, float drag) const {
    return (drag > 0) ? std::abs(downforce) / drag : 0;
}

QVector<AeroMap> AerodynamicTestingModel::generateAeroMap(const QVector<AeroTestData>& testData) const {
    QVector<AeroMap> map;
    for (const auto& point : testData) {
        AeroMap entry;
        entry.speed = point.speed;
        entry.rideHeight = point.rideHeightFront;
        entry.yawAngle = point.yawAngle;
        entry.downforce = std::abs(point.downforceFront) + std::abs(point.downforceRear);
        entry.drag = point.dragForce;
        entry.efficiency = calculateLiftDragRatio(entry.downforce, entry.drag);
        map.append(entry);
    }
    return map;
}

QVector<OptimizationRecommendation> AerodynamicTestingModel::optimizeAeroBalance(float targetBalance, const BalanceSetup& currentSetup) const {
    QVector<OptimizationRecommendation> recs;

    AeroTestData baseline = calculateAeroForces(200.0f, 0.05f, 0.07f, 0);
    float currentBalance = calculateAeroBalance(baseline.downforceFront, baseline.downforceRear);

    float balanceDelta = targetBalance - currentBalance;
    if (std::abs(balanceDelta) > 0.02f) {
        OptimizationRecommendation rec;
        rec.parameter = "Front Wing Angle";
        rec.suggestedValue = currentSetup.tirePressureFront + balanceDelta * 5.0f;
        rec.expectedImprovement = std::abs(balanceDelta) * 0.3f;
        rec.confidence = 0.6f;
        rec.reason = "Aero balance needs adjustment";
        recs.append(rec);
    }

    return recs;
}

AeroTestData AerodynamicTestingModel::predictAeroEffect(float speed, const BalanceSetup& setup) const {
    float rideHeightFront = 0.05f + setup.tirePressureFront * 0.002f;
    float rideHeightRear = 0.07f + setup.tirePressureRear * 0.002f;
    return calculateAeroForces(speed, rideHeightFront, rideHeightRear, 0);
}

// ============================================================================
// SuspensionTuningModel implementation
// ============================================================================

QVector3D SuspensionTuningModel::calculateRollCenter(const SuspensionTuningGeometry& geo) const {
    float lateralOffset = geo.scrubRadius * std::cos(geo.casterAngle * M_PI / 180.0f);
    return QVector3D(0, geo.rollCenterHeight, lateralOffset);
}

float SuspensionTuningModel::calculateScrubRadius(const SuspensionTuningGeometry& geo) const {
    return geo.scrubRadius;
}

float SuspensionTuningModel::calculateCamberGain(const SuspensionTuningGeometry& geo, float travel) const {
    return geo.camberGain * travel * 100.0f;
}

float SuspensionTuningModel::calculateToeChange(const SuspensionTuningGeometry& geo, float travel) const {
    return geo.toeChange * travel * 100.0f;
}

float SuspensionTuningModel::calculateAntiDive(const SuspensionTuningGeometry& geo) const {
    return geo.antiDive;
}

float SuspensionTuningModel::calculateAntiSquat(const SuspensionTuningGeometry& geo) const {
    return geo.antiSquat;
}

float SuspensionTuningModel::calculateMechanicalTrail(const SuspensionTuningGeometry& geo) const {
    return geo.mechanicalTrail;
}

SuspensionCompliance SuspensionTuningModel::analyzeCompliance(const SuspensionTuningGeometry& geometry, const BalanceSetup& springs, const BalanceSetup& dampers) const {
    SuspensionCompliance compliance;
    float totalSpring = springs.springRateFront + springs.springRateRear;

    compliance.lateralCompliance = (totalSpring > 0) ? 1000.0f / totalSpring : 0;
    compliance.longitudinalCompliance = compliance.lateralCompliance * 1.5f;
    compliance.verticalCompliance = compliance.lateralCompliance * 0.8f;
    compliance.steerCompliance = compliance.lateralCompliance * 0.1f;
    compliance.camberCompliance = compliance.lateralCompliance * 0.05f;

    return compliance;
}

SuspensionRate SuspensionTuningModel::calculateSuspensionRates(const SuspensionTuningGeometry& geometry, const BalanceSetup& springs) const {
    SuspensionRate rate;
    rate.wheelRate = (springs.springRateFront + springs.springRateRear) * 0.5f;
    rate.motionRatio = 0.8f;
    rate.rollStiffness = rate.wheelRate * rate.motionRatio * rate.motionRatio;
    rate.heaveStiffness = rate.rollStiffness * 0.5f;
    rate.pitchStiffness = rate.rollStiffness * 0.8f;

    return rate;
}

QVector<OptimizationRecommendation> SuspensionTuningModel::optimizeSuspensionGeometry(float mass, float wheelbase, const QString& track) const {
    QVector<OptimizationRecommendation> recs;

    OptimizationRecommendation rollCenter;
    rollCenter.parameter = "Roll Center Height";
    rollCenter.suggestedValue = 0.03f;
    rollCenter.expectedImprovement = 0.1f;
    rollCenter.confidence = 0.65f;
    rollCenter.reason = "Optimize roll center for weight transfer";
    recs.append(rollCenter);

    OptimizationRecommendation caster;
    caster.parameter = "Caster Angle";
    caster.suggestedValue = 6.0f;
    caster.expectedImprovement = 0.05f;
    caster.confidence = 0.7f;
    caster.reason = "Improve steering feel and stability";
    recs.append(caster);

    if (track.contains("street") || track.contains("bumpy")) {
        OptimizationRecommendation scrub;
        scrub.parameter = "Scrub Radius";
        scrub.suggestedValue = 10.0f;
        scrub.expectedImprovement = 0.03f;
        scrub.confidence = 0.5f;
        scrub.reason = "Reduce scrub for bumpy surfaces";
        recs.append(scrub);
    }

    return recs;
}

// ============================================================================
// EngineMappingModel implementation
// ============================================================================

PowerCurve EngineMappingModel::calculatePowerCurve(const QVector<float>& accelerationProfile) const {
    PowerCurve curve;
    float rpmStep = 250.0f;

    for (float rpm = 1000.0f; rpm <= 9000.0f; rpm += rpmStep) {
        curve.rpm.append(rpm);

        float normalizedRpm = rpm / 9000.0f;
        float torqueShape = std::sin(normalizedRpm * M_PI) * (1.0f - normalizedRpm * 0.3f);
        float torque = torqueShape * 500.0f;
        float power = torque * rpm * M_PI / 30000.0f;
        float fuelFlow = power * 0.0003f;

        curve.torque.append(torque);
        curve.power.append(power);
        curve.fuelFlow.append(fuelFlow);

        curve.peakPower = std::max(curve.peakPower, power);
        curve.peakTorque = std::max(curve.peakTorque, torque);
    }

    curve.optimalShiftPoint = calculateOptimalShiftPoint(curve);
    return curve;
}

QVector<float> EngineMappingModel::calculateTorqueCurve(const QVector<float>& rpmRange) const {
    QVector<float> torque;
    for (float rpm : rpmRange) {
        float normalizedRpm = rpm / 9000.0f;
        float t = std::sin(normalizedRpm * M_PI) * (1.0f - normalizedRpm * 0.3f) * 500.0f;
        torque.append(t);
    }
    return torque;
}

float EngineMappingModel::calculateFuelFlow(float rpm, float throttle) const {
    float normalizedRpm = rpm / 9000.0f;
    return 1.5f + normalizedRpm * 2.0f + throttle * 3.0f;
}

float EngineMappingModel::calculateOptimalShiftPoint(const PowerCurve& powerCurve) const {
    if (powerCurve.rpm.size() < 2) return 7000.0f;

    float peakPowerRpm = 7000.0f;
    for (int i = 0; i < powerCurve.power.size(); ++i) {
        if (powerCurve.power[i] >= powerCurve.peakPower * 0.98f) {
            peakPowerRpm = powerCurve.rpm[i];
            break;
        }
    }

    return peakPowerRpm * 0.95f;
}

float EngineMappingModel::calculateAcceleration(float rpm, int gear, float weight) const {
    float normalizedRpm = rpm / 9000.0f;
    float torque = std::sin(normalizedRpm * M_PI) * 500.0f;
    float gearRatio = 3.5f / std::max(1, gear);
    float wheelTorque = torque * gearRatio;
    float force = wheelTorque / 0.33f;

    return force / std::max(weight, 500.0f);
}

EngineCalibration EngineMappingModel::loadEngineCalibration(const QString& filePath) const {
    EngineCalibration calibration;
    calibration.fuelAirRatio = 14.7f;
    calibration.ignitionAdvance = 25.0f;
    calibration.revLimit = 9000.0f;

    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            QStringList parts = line.split(',');
            if (parts.size() >= 3) {
                EngineMap map;
                map.rpm = parts[0].toFloat();
                map.throttlePosition = parts[1].toFloat();
                map.torque = parts[2].toFloat();
                map.power = parts.size() > 3 ? parts[3].toFloat() : 0;
                map.fuelFlow = parts.size() > 4 ? parts[4].toFloat() : 0;
                calibration.maps.append(map);
            }
        }
    }

    return calibration;
}

EngineCalibration EngineMappingModel::optimizeEngineMap(const PerformanceMetrics& targetPerformance, const BalanceSetup& constraints) const {
    EngineCalibration calibration;
    calibration.fuelAirRatio = 14.7f;
    calibration.ignitionAdvance = 28.0f;
    calibration.revLimit = 9500.0f;

    if (targetPerformance.topSpeed > 300.0f) {
        calibration.ignitionAdvance = 30.0f;
    }

    for (float rpm = 1000.0f; rpm <= calibration.revLimit; rpm += 500.0f) {
        for (float throttle = 0.0f; throttle <= 1.0f; throttle += 0.25f) {
            EngineMap map;
            map.rpm = rpm;
            map.throttlePosition = throttle;
            float normalizedRpm = rpm / calibration.revLimit;
            map.torque = std::sin(normalizedRpm * M_PI) * 500.0f * throttle;
            map.power = map.torque * rpm * M_PI / 30000.0f;
            map.fuelFlow = map.power * 0.0003f;
            map.ignitionTiming = calibration.ignitionAdvance * (1.0f - normalizedRpm * 0.2f);
            map.fuelInjection = map.fuelFlow * 14.7f;
            calibration.maps.append(map);
        }
    }

    return calibration;
}

float EngineMappingModel::calculateEngineBraking(float rpm, float throttle) const {
    if (throttle > 0.01f) return 0;
    float normalizedRpm = rpm / 9000.0f;
    return normalizedRpm * 50.0f + 20.0f;
}

// ============================================================================
// DifferentialTuningModel implementation
// ============================================================================

float DifferentialTuningModel::calculateDiffTorque(float inputTorque, float wheelSpeedDiff, const DiffSettings& settings) const {
    float rampTorque = 0;
    if (wheelSpeedDiff > 0) {
        rampTorque = inputTorque * std::tan(settings.rampAngleAccel * M_PI / 180.0f);
    } else {
        rampTorque = inputTorque * std::tan(settings.rampAngleDecel * M_PI / 180.0f);
    }

    float preloadTorque = settings.preload;
    float totalLocked = preloadTorque + rampTorque;
    float maxLock = inputTorque * settings.lockingPercent / 100.0f;

    return std::min(totalLocked, maxLock);
}

DiffBehavior DifferentialTuningModel::analyzeDiffBehavior(float speed, float lateralAccel, float throttle, const DiffSettings& settings) const {
    DiffBehavior behavior;

    float entryDiff = (throttle < 0.3f) ? settings.limitedSlipPercent * 0.5f : settings.limitedSlipPercent;
    float midDiff = settings.limitedSlipPercent * (1.0f - lateralAccel * 0.1f);
    float exitDiff = (throttle > 0.5f) ? settings.limitedSlipPercent * 1.2f : settings.limitedSlipPercent;

    behavior.cornerEntryDiff = std::clamp(entryDiff, 0.0f, 100.0f);
    behavior.midCornerDiff = std::clamp(midDiff, 0.0f, 100.0f);
    behavior.exitDiff = std::clamp(exitDiff, 0.0f, 100.0f);
    behavior.straightLineDiff = settings.limitedSlipPercent * 0.3f;
    behavior.transitionSmoothness = settings.viscousCoefficient;

    return behavior;
}

DiffSettings DifferentialTuningModel::optimizeDiffSetup(float mass, float power, const QString& track, const QString& drivingStyle) const {
    DiffSettings settings;
    settings.preload = 20.0f;
    settings.rampAngleAccel = 45.0f;
    settings.rampAngleDecel = 30.0f;
    settings.limitedSlipPercent = 40.0f;
    settings.lockingPercent = 60.0f;
    settings.viscousCoefficient = 0.5f;
    settings.speedSensitiveCoeff = 0.3f;

    if (track.contains("Monaco") || track.contains("street")) {
        settings.rampAngleAccel = 35.0f;
        settings.preload = 30.0f;
    } else if (track.contains("Monza") || track.contains("speed")) {
        settings.rampAngleAccel = 55.0f;
        settings.lockingPercent = 70.0f;
    }

    if (drivingStyle.contains("aggressive") || drivingStyle.contains("attack")) {
        settings.preload = 35.0f;
        settings.rampAngleAccel = 55.0f;
    } else if (drivingStyle.contains("smooth") || drivingStyle.contains("consistent")) {
        settings.viscousCoefficient = 0.7f;
        settings.preload = 15.0f;
    }

    return settings;
}

DiffTestResult DifferentialTuningModel::testDiffResponse(const DiffSettings& settings, const ConditionState& conditions) const {
    DiffTestResult result;
    float gripFactor = conditions.gripCoefficient;

    result.turnInResponse = (100.0f - settings.limitedSlipPercent) * 0.01f * gripFactor;
    result.midCornerStability = settings.limitedSlipPercent * 0.01f * gripFactor;
    result.exitTraction = settings.lockingPercent * 0.01f * gripFactor;
    result.straightLineStability = settings.preload * 0.02f;
    result.tireWearImpact = settings.lockingPercent * 0.005f;

    return result;
}

float DifferentialTuningModel::calculatePreloadEffect(float preload, float speed) const {
    float speedFactor = std::min(speed / 100.0f, 1.0f);
    return preload * (1.0f - speedFactor * 0.3f);
}

float DifferentialTuningModel::calculateRampEffect(float rampAngle, float torque, float speed) const {
    float rampRad = rampAngle * M_PI / 180.0f;
    float rampTorque = torque * std::tan(rampRad);
    float speedDamping = 1.0f / (1.0f + speed * 0.005f);
    return rampTorque * speedDamping;
}

float DifferentialTuningModel::calculateLockingPercent(const DiffSettings& settings, float speed) const {
    float baseLock = settings.lockingPercent;
    float speedEffect = settings.speedSensitiveCoeff * speed * 0.01f;
    return std::clamp(baseLock - speedEffect, 0.0f, 100.0f);
}

QVector<OptimizationRecommendation> DifferentialTuningModel::suggestDiffChanges(const DiffSettings& currentSettings, const DiffBehavior& targetBehavior) const {
    QVector<OptimizationRecommendation> recs;

    if (targetBehavior.turnInResponse > 0.7f && currentSettings.limitedSlipPercent < 30.0f) {
        OptimizationRecommendation rec;
        rec.parameter = "Limited Slip %";
        rec.suggestedValue = currentSettings.limitedSlipPercent + 10.0f;
        rec.expectedImprovement = 0.15f;
        rec.confidence = 0.65f;
        rec.reason = "Increase locking for better turn-in";
        recs.append(rec);
    }

    if (targetBehavior.exitTraction > 0.8f && currentSettings.lockingPercent < 50.0f) {
        OptimizationRecommendation rec;
        rec.parameter = "Locking %";
        rec.suggestedValue = currentSettings.lockingPercent + 10.0f;
        rec.expectedImprovement = 0.1f;
        rec.confidence = 0.6f;
        rec.reason = "Increase locking for better exit traction";
        recs.append(rec);
    }

    if (targetBehavior.transitionSmoothness < 0.3f) {
        OptimizationRecommendation rec;
        rec.parameter = "Viscous Coefficient";
        rec.suggestedValue = currentSettings.viscousCoefficient + 0.2f;
        rec.expectedImprovement = 0.08f;
        rec.confidence = 0.55f;
        rec.reason = "Increase viscous for smoother transitions";
        recs.append(rec);
    }

    return recs;
}

// ============================================================================
// GearboxModel implementation
// ============================================================================

QVector<GearRatio> GearboxModel::calculateGearSpeeds(const QVector<float>& ratios, float finalDrive, float tireCircumference) const {
    QVector<GearRatio> gears;
    float tireRadius = tireCircumference / (2.0f * M_PI);

    for (int i = 0; i < ratios.size(); ++i) {
        GearRatio gear;
        gear.gear = i + 1;
        gear.ratio = ratios[i];
        float totalRatio = ratios[i] * finalDrive;
        gear.theoreticalSpeed = (7000.0f * tireCircumference) / (totalRatio * 60.0f) * 3.6f;
        gear.acceleration = 1.0f / (totalRatio * tireRadius);
        gears.append(gear);
    }

    return gears;
}

float GearboxModel::calculateAcceleration(float rpm, int gear, float torque, float weight) const {
    if (gear < 1) return 0;
    QVector<float> defaultRatios = {3.5f, 2.5f, 1.8f, 1.4f, 1.1f, 0.9f};
    int idx = std::min(gear - 1, static_cast<int>(defaultRatios.size()) - 1);
    float totalRatio = defaultRatios[idx] * 3.8f;
    float wheelForce = torque * totalRatio / 0.33f;
    return wheelForce / std::max(weight, 500.0f);
}

float GearboxModel::calculateTheoreticalTopSpeed(int gear, const QVector<float>& ratios, float finalDrive, float tireCircumference) const {
    if (gear < 1 || gear > ratios.size()) return 0;
    float totalRatio = ratios[gear - 1] * finalDrive;
    return (7000.0f * tireCircumference) / (totalRatio * 60.0f) * 3.6f;
}

QVector<ShiftPoint> GearboxModel::calculateOptimalShiftPoints(const PowerCurve& powerCurve, const QVector<float>& ratios, float finalDrive) const {
    QVector<ShiftPoint> shiftPoints;

    if (ratios.size() < 2 || powerCurve.rpm.size() < 2) return shiftPoints;

    for (int gear = 0; gear < ratios.size() - 1; ++gear) {
        float currentRatio = ratios[gear] * finalDrive;
        float nextRatio = ratios[gear + 1] * finalDrive;

        ShiftPoint sp;
        sp.fromGear = gear + 1;
        sp.toGear = gear + 2;

        for (int i = 0; i < powerCurve.rpm.size() - 1; ++i) {
            float currentSpeed = (powerCurve.rpm[i] * 2.0f * M_PI * 0.33f) / (currentRatio * 60.0f) * 3.6f;
            float nextGearRpm = currentSpeed * nextRatio * 60.0f / (2.0f * M_PI * 0.33f * 3.6f);

            if (nextGearRpm > 1000.0f && nextGearRpm < powerCurve.rpm.last()) {
                float currentPower = powerCurve.power[i];
                int nextIdx = std::min(i + 1, static_cast<int>(powerCurve.power.size()) - 1);
                float nextPower = powerCurve.power[nextIdx] * (ratios[gear] / ratios[gear + 1]);

                if (nextPower > currentPower * 0.95f) {
                    sp.rpm = powerCurve.rpm[i];
                    sp.speed = currentSpeed;
                    sp.timeLost = 0.05f;
                    shiftPoints.append(sp);
                    break;
                }
            }
        }
    }

    return shiftPoints;
}

QVector<float> GearboxModel::optimizeGearRatios(float topSpeed, float acceleration, const QString& track) const {
    QVector<float> ratios;
    int numGears = 6;

    float topGearRatio = (topSpeed * 3.6f * 3.8f) / (9000.0f * 2.0f * M_PI * 0.33f / 60.0f);

    for (int i = 0; i < numGears; ++i) {
        float progress = static_cast<float>(i) / static_cast<float>(numGears - 1);
        float ratio = topGearRatio * std::pow(3.5f / topGearRatio, 1.0f - progress);
        ratios.append(ratio);
    }

    if (track.contains("Monaco") || track.contains("street")) {
        for (int i = 0; i < ratios.size(); ++i) {
            ratios[i] *= 1.1f;
        }
    }

    return ratios;
}

float GearboxModel::calculateFuelConsumptionPerGear(float throttle, float rpm) const {
    float normalizedRpm = rpm / 9000.0f;
    float baseFuel = 1.5f + normalizedRpm * 2.0f;
    return baseFuel * (0.3f + throttle * 0.7f);
}

float GearboxModel::calculateLapTimeWithGearing(const GearboxConfig& setup, const QString& track) const {
    float baseLapTime = 90.0f;
    float shiftPenalty = static_cast<float>(setup.numGears - 6) * 0.1f;
    float efficiencyFactor = setup.efficiency;
    float shiftLoss = setup.shiftTime * static_cast<float>(setup.numGears * 2);

    return (baseLapTime + shiftPenalty + shiftLoss) / efficiencyFactor;
}

QString GearboxModel::analyzeShiftStrategy(const QVector<ShiftPoint>& shiftPoints, const PowerCurve& powerCurve) const {
    QString analysis;
    analysis += "=== Shift Strategy Analysis ===\n\n";

    for (const auto& sp : shiftPoints) {
        analysis += QString("Gear %1 -> %2 at %3 RPM (%4 km/h)\n")
                        .arg(sp.fromGear).arg(sp.toGear)
                        .arg(sp.rpm, 0, 'f', 0)
                        .arg(sp.speed, 0, 'f', 1);
    }

    if (shiftPoints.size() >= 2) {
        float avgShiftRpm = 0;
        for (const auto& sp : shiftPoints) avgShiftRpm += sp.rpm;
        avgShiftRpm /= shiftPoints.size();

        float optimalRpm = powerCurve.optimalShiftPoint;
        float rpmDelta = avgShiftRpm - optimalRpm;

        analysis += QString("\nAvg shift point: %1 RPM\n").arg(avgShiftRpm, 0, 'f', 0);
        analysis += QString("Optimal shift point: %1 RPM\n").arg(optimalRpm, 0, 'f', 0);

        if (std::abs(rpmDelta) > 200.0f) {
            analysis += QString("Recommendation: %1 shift points by %2 RPM\n")
                            .arg(rpmDelta > 0 ? "Lower" : "Raise")
                            .arg(std::abs(rpmDelta), 0, 'f', 0);
        } else {
            analysis += "Shift strategy is well optimized.\n";
        }
    }

    return analysis;
}

// ============================================================================
// BrakeSystemModel implementation
// ============================================================================

float BrakeSystemModel::calculateBrakeForce(float pressure, const BrakeSystemConfig& config) const {
    float frontArea = config.frontPadArea;
    float rearArea = config.rearPadArea;
    float totalArea = frontArea + rearArea;
    float force = pressure * totalArea * config.boostRatio;
    return force * config.pedalRatio;
}

float BrakeSystemModel::calculateBrakeBias(const BrakeSystemConfig& config) const {
    float frontTorque = config.frontDiscDiameter * config.frontPadArea;
    float rearTorque = config.rearDiscDiameter * config.rearPadArea;
    float total = frontTorque + rearTorque;
    return (total > 0) ? (frontTorque / total * 100.0f) : 50.0f;
}

float BrakeSystemModel::calculateStoppingDistance(float speed, float weight, float brakeForce) const {
    float speedMs = speed / 3.6f;
    float deceleration = brakeForce / std::max(weight, 500.0f);
    float stoppingDist = (speedMs * speedMs) / (2.0f * deceleration + 1.0f);
    return stoppingDist;
}

float BrakeSystemModel::calculatePedalForce(float acceleration, const BrakeSystemConfig& config) const {
    float requiredForce = acceleration * 1500.0f;
    float systemAdvantage = config.boostRatio * config.pedalRatio;
    return (systemAdvantage > 0) ? requiredForce / systemAdvantage : requiredForce;
}

float BrakeSystemModel::calculateBrakeTorque(float pressure, float discDiameter, float padArea) const {
    return pressure * padArea * discDiameter * 0.5f;
}

BrakePerformance BrakeSystemModel::analyzeBrakePerformance(float speed, float weight, const BrakeSystemConfig& config) const {
    BrakePerformance perf;

    float brakeForce = calculateBrakeForce(config.maxBrakePressure > 0 ? config.maxBrakePressure : 100.0f, config);
    perf.stoppingDistance100_0 = calculateStoppingDistance(100.0f, weight, brakeForce);
    perf.stoppingDistance200_0 = calculateStoppingDistance(200.0f, weight, brakeForce);
    perf.brakeBalance = calculateBrakeBias(config);
    perf.fadeResistance = 0.8f;
    perf.pedalFeel = 0.7f;
    perf.modulation = 0.65f;

    return perf;
}

float BrakeSystemModel::calculateBrakeFade(float discTemp, float padTemp) const {
    float fadeFactor = 1.0f;
    if (discTemp > 500.0f) {
        fadeFactor -= (discTemp - 500.0f) * 0.001f;
    }
    if (padTemp > 400.0f) {
        fadeFactor -= (padTemp - 400.0f) * 0.0015f;
    }
    return std::clamp(fadeFactor, 0.3f, 1.0f);
}

float BrakeSystemModel::calculateBrakeBalanceForVehicle(float mass, float wheelbase, float frontDist, const QString& track) const {
    float staticFront = (wheelbase - frontDist) / wheelbase * 100.0f;
    float dynamicTransfer = 0.45f * mass * 9.81f * 0.45f / wheelbase * 100.0f / (mass * 9.81f);

    float balance = staticFront + dynamicTransfer;

    if (track.contains("street") || track.contains("bumpy")) {
        balance -= 2.0f;
    }

    return std::clamp(balance, 45.0f, 70.0f);
}

QVector<OptimizationRecommendation> BrakeSystemModel::optimizeBrakeBias(float mass, float wheelbase, float frontDist, const QString& track) const {
    QVector<OptimizationRecommendation> recs;

    float optimalBalance = calculateBrakeBalanceForVehicle(mass, wheelbase, frontDist, track);

    OptimizationRecommendation biasRec;
    biasRec.parameter = "Brake Bias";
    biasRec.suggestedValue = optimalBalance;
    biasRec.expectedImprovement = 0.2f;
    biasRec.confidence = 0.75f;
    biasRec.reason = "Optimize brake balance for vehicle weight distribution";
    recs.append(biasRec);

    OptimizationRecommendation pressureRec;
    pressureRec.parameter = "Brake Pressure";
    pressureRec.suggestedValue = 80.0f;
    pressureRec.expectedImprovement = 0.1f;
    pressureRec.confidence = 0.6f;
    pressureRec.reason = "Adjust master cylinder pressure for optimal pedal feel";
    recs.append(pressureRec);

    return recs;
}

// ============================================================================
// DriverTrainingModel implementation
// ============================================================================

DriverPerformance DriverTrainingModel::analyzeDriverPerformance(const TelemetrySession& session) const {
    DriverPerformance perf;

    for (const auto& channel : session.channels) {
        if (channel.name.contains("throttle", Qt::CaseInsensitive)) {
            float variance = 0;
            float mean = 0;
            for (float val : channel.samples) mean += val;
            mean /= std::max(1, static_cast<int>(channel.samples.size()));
            for (float val : channel.samples) variance += (val - mean) * (val - mean);
            variance /= std::max(1, static_cast<int>(channel.samples.size()));
            perf.throttleControl = 1.0f - std::min(variance, 1.0f);
        }
        if (channel.name.contains("brake", Qt::CaseInsensitive)) {
            float maxBrake = channel.maxValue;
            perf.brakeControl = (maxBrake > 0) ? 0.7f : 0.5f;
        }
        if (channel.name.contains("steering", Qt::CaseInsensitive)) {
            float smoothness = 0;
            for (int i = 1; i < channel.samples.size(); ++i) {
                smoothness += std::abs(channel.samples[i] - channel.samples[i - 1]);
            }
            smoothness /= std::max(1, static_cast<int>(channel.samples.size()) - 1);
            perf.steeringSmoothness = 1.0f - std::min(smoothness, 1.0f);
        }
    }

    perf.racingLineAccuracy = 0.75f;
    perf.apexHitRate = 0.8f;
    perf.consistency = 0.7f;
    perf.improvement = 0.1f;

    return perf;
}

QVector<DrivingError> DriverTrainingModel::identifyDrivingErrors(const TelemetrySession& session) const {
    QVector<DrivingError> errors;

    for (const auto& channel : session.channels) {
        if (channel.name.contains("brake", Qt::CaseInsensitive)) {
            for (int i = 1; i < channel.samples.size(); ++i) {
                float delta = std::abs(channel.samples[i] - channel.samples[i - 1]);
                if (delta > 0.8f) {
                    DrivingError error;
                    error.severity = delta;
                    error.type = "Abrupt braking";
                    error.position = static_cast<float>(i) / channel.samples.size();
                    error.speed = channel.samples[i];
                    error.suggestion = "Apply brakes more progressively";
                    errors.append(error);
                }
            }
        }

        if (channel.name.contains("steering", Qt::CaseInsensitive)) {
            for (int i = 2; i < channel.samples.size(); ++i) {
                float delta = std::abs(channel.samples[i] - 2.0f * channel.samples[i - 1] + channel.samples[i - 2]);
                if (delta > 0.5f) {
                    DrivingError error;
                    error.severity = delta;
                    error.type = "Steering oscillation";
                    error.position = static_cast<float>(i) / channel.samples.size();
                    error.suggestion = "Smooth steering inputs";
                    errors.append(error);
                }
            }
        }
    }

    return errors;
}

QVector<float> DriverTrainingModel::calculateRacingLineDeviation(const TelemetrySession& session, const QVector<float>& idealLine) const {
    QVector<float> deviations;

    for (const auto& channel : session.channels) {
        if (channel.name.contains("lateral_position", Qt::CaseInsensitive)) {
            int n = std::min(channel.samples.size(), idealLine.size());
            deviations.resize(n);
            for (int i = 0; i < n; ++i) {
                deviations[i] = channel.samples[i] - idealLine[i];
            }
            break;
        }
    }

    return deviations;
}

float DriverTrainingModel::calculateBrakeTrailBraking(const TelemetrySession& session) const {
    float trailBrakingScore = 0;
    int brakeChannelIdx = -1;

    for (int i = 0; i < session.channels.size(); ++i) {
        if (session.channels[i].name.contains("brake", Qt::CaseInsensitive)) {
            brakeChannelIdx = i;
            break;
        }
    }

    if (brakeChannelIdx < 0) return 0;

    const auto& brakeChannel = session.channels[brakeChannelIdx];
    int trailBrakingEvents = 0;

    for (int i = 1; i < brakeChannel.samples.size() - 1; ++i) {
        if (brakeChannel.samples[i] > 0.1f &&
            brakeChannel.samples[i] < brakeChannel.samples[i - 1]) {
            trailBrakingEvents++;
        }
    }

    trailBrakingScore = static_cast<float>(trailBrakingEvents) / std::max(1, static_cast<int>(brakeChannel.samples.size()) - 2);
    return std::clamp(trailBrakingScore * 5.0f, 0.0f, 1.0f);
}

QVector<float> DriverTrainingModel::calculateThrottleApplication(const TelemetrySession& session) const {
    QVector<float> application;

    for (const auto& channel : session.channels) {
        if (channel.name.contains("throttle", Qt::CaseInsensitive)) {
            application = channel.samples;
            break;
        }
    }

    return application;
}

QVector<float> DriverTrainingModel::calculateSteeringInputs(const TelemetrySession& session) const {
    QVector<float> inputs;

    for (const auto& channel : session.channels) {
        if (channel.name.contains("steering", Qt::CaseInsensitive)) {
            inputs = channel.samples;
            break;
        }
    }

    return inputs;
}

DriverCoaching DriverTrainingModel::generateCoachingReport(const TelemetrySession& session) const {
    DriverCoaching coaching;
    coaching.errors = identifyDrivingErrors(session);
    coaching.overallScore = analyzeDriverPerformance(session).throttleControl * 0.3f
                           + analyzeDriverPerformance(session).brakeControl * 0.3f
                           + analyzeDriverPerformance(session).steeringSmoothness * 0.4f;

    coaching.priorities.append("Consistency");
    coaching.priorities.append("Brake modulation");
    coaching.priorities.append("Racing line accuracy");

    coaching.estimatedLapTimeImprovement = (1.0f - coaching.overallScore) * 2.0f;

    return coaching;
}

QString DriverTrainingModel::compareDriverPerformance(const TelemetrySession& driver1, const TelemetrySession& driver2) const {
    DriverPerformance perf1 = analyzeDriverPerformance(driver1);
    DriverPerformance perf2 = analyzeDriverPerformance(driver2);

    QString comparison;
    comparison += "=== Driver Comparison ===\n\n";
    comparison += QString("Throttle Control: %1 vs %2 (%3)\n")
                     .arg(perf1.throttleControl, 0, 'f', 2)
                     .arg(perf2.throttleControl, 0, 'f', 2)
                     .arg(perf1.throttleControl > perf2.throttleControl ? "Driver 1" : "Driver 2");
    comparison += QString("Brake Control: %1 vs %2\n").arg(perf1.brakeControl, 0, 'f', 2).arg(perf2.brakeControl, 0, 'f', 2);
    comparison += QString("Steering Smoothness: %1 vs %2\n").arg(perf1.steeringSmoothness, 0, 'f', 2).arg(perf2.steeringSmoothness, 0, 'f', 2);
    comparison += QString("Overall: %1 vs %2\n").arg(perf1.consistency, 0, 'f', 2).arg(perf2.consistency, 0, 'f', 2);

    return comparison;
}

QVector<QString> DriverTrainingModel::suggestDriverImprovements(const DriverPerformance& performance) const {
    QVector<QString> suggestions;

    if (performance.throttleControl < 0.6f) {
        suggestions.append("Work on smoother throttle application - avoid on/off inputs");
    }
    if (performance.brakeControl < 0.6f) {
        suggestions.append("Practice progressive braking - build up pressure gradually");
    }
    if (performance.steeringSmoothness < 0.6f) {
        suggestions.append("Reduce steering corrections - look further ahead on track");
    }
    if (performance.racingLineAccuracy < 0.7f) {
        suggestions.append("Study the racing line - focus on hitting apexes consistently");
    }
    if (performance.apexHitRate < 0.7f) {
        suggestions.append("Work on apex consistency - aim for the same point each lap");
    }
    if (performance.consistency < 0.7f) {
        suggestions.append("Focus on lap-to-lap consistency - build up rhythm gradually");
    }

    if (suggestions.isEmpty()) {
        suggestions.append("Good performance overall - focus on incremental improvements");
    }

    return suggestions;
}

// ============================================================================
// RaceEngineeringModel implementation
// ============================================================================

QVector<RaceInstruction> RaceEngineeringModel::analyzeRaceSituation(const RaceData& raceData) const {
    QVector<RaceInstruction> instructions;

    if (raceData.gapAhead < 2.0f && raceData.gapAhead > 0) {
        RaceInstruction push;
        push.type = "PUSH";
        push.value = 1.0f;
        push.reason = "Within 2 seconds of car ahead - push for overtake";
        push.urgent = true;
        instructions.append(push);
    }

    if (raceData.gapBehind > 5.0f) {
        RaceInstruction conserve;
        conserve.type = "CONSERVE";
        conserve.value = 0.8f;
        conserve.reason = "Gap behind is large - conserve tires and fuel";
        conserve.urgent = false;
        instructions.append(conserve);
    }

    if (raceData.lapsOfFuel < 3.0f && raceData.fuelRemaining > 0) {
        RaceInstruction fuelSave;
        fuelSave.type = "FUEL_SAVE";
        fuelSave.value = 0.9f;
        fuelSave.reason = "Low fuel - manage fuel to finish";
        fuelSave.urgent = true;
        instructions.append(fuelSave);
    }

    if (raceData.predictedLapTime > raceData.currentLapTime * 1.05f) {
        RaceInstruction pace;
        pace.type = "PACE";
        pace.value = 1.0f;
        pace.reason = "Lap time dropped - check tires and focus";
        pace.urgent = false;
        instructions.append(pace);
    }

    return instructions;
}

QVector<int> RaceEngineeringModel::calculatePitWindow(float fuelPerLap, float currentFuel) const {
    QVector<int> window;
    if (fuelPerLap <= 0) return window;

    int lapsRemaining = static_cast<int>(currentFuel / fuelPerLap);
    int earliestPit = std::max(1, lapsRemaining - 8);
    int latestPit = std::max(earliestPit + 1, lapsRemaining - 2);

    for (int lap = earliestPit; lap <= latestPit; lap += 3) {
        window.append(lap);
    }

    if (window.isEmpty()) {
        window.append(lapsRemaining);
    }

    return window;
}

float RaceEngineeringModel::calculateGapDelta(float myLapTime, float competitorLapTime) const {
    return competitorLapTime - myLapTime;
}

int RaceEngineeringModel::calculatePositionChange(const BalanceSetup& strategy, const QVector<float>& competitors) const {
    if (competitors.isEmpty()) return 0;
    float avgCompetitor = 0;
    for (float c : competitors) avgCompetitor += c;
    avgCompetitor /= competitors.size();

    float myEstimate = 90.0f;
    return (myEstimate < avgCompetitor) ? -1 : 1;
}

QVector<RaceInstruction> RaceEngineeringModel::generateRaceInstructions(const RaceData& raceData, const BalanceSetup& strategy) const {
    QVector<RaceInstruction> instructions = analyzeRaceSituation(raceData);

    if (raceData.position <= 3) {
        RaceInstruction defense;
        defense.type = "DEFEND";
        defense.value = 1.0f;
        defense.reason = "In podium position - defend position";
        defense.urgent = true;
        instructions.append(defense);
    }

    return instructions;
}

QVector<QString> RaceEngineeringModel::analyzeTireStrategy(const QString& currentTires, int remainingLaps) const {
    QVector<QString> strategy;

    if (remainingLaps > 20) {
        strategy.append("Consider pit stop for fresh tires");
        strategy.append("Manage tire temperatures - avoid overheating");
    } else if (remainingLaps > 10) {
        strategy.append("Tires should last - maintain consistent pace");
    } else {
        strategy.append("Push to finish - tires should hold");
    }

    if (currentTires.contains("hard")) {
        strategy.append("Hard compound - good durability, lower peak grip");
    } else if (currentTires.contains("soft")) {
        strategy.append("Soft compound - high grip, manage degradation");
    } else if (currentTires.contains("medium")) {
        strategy.append("Medium compound - balanced performance");
    }

    return strategy;
}

QVector<QString> RaceEngineeringModel::calculateFuelStrategy(float fuelRemaining, float fuelPerLap, int lapsRemaining) const {
    QVector<QString> strategy;

    float fuelDeficit = fuelPerLap * lapsRemaining - fuelRemaining;

    if (fuelDeficit > 0) {
        strategy.append(QString("Fuel deficit: %1 kg - pit stop required").arg(fuelDeficit, 0, 'f', 1));
        strategy.append("Reduce fuel consumption: lift and coast on straights");
    } else {
        float surplus = -fuelDeficit;
        strategy.append(QString("Fuel surplus: %1 kg - can push harder").arg(surplus, 0, 'f', 1));
        if (surplus > 5.0f) {
            strategy.append("Consider fuel save mode to gain time elsewhere");
        }
    }

    float lapsOfFuel = fuelRemaining / std::max(fuelPerLap, 0.1f);
    strategy.append(QString("Laps of fuel remaining: %1").arg(lapsOfFuel, 0, 'f', 1));

    return strategy;
}

QVector<QString> RaceEngineeringModel::calculateWeatherStrategy(const WeatherStateEnv& weatherForecast, const BalanceSetup& currentSetup) const {
    QVector<QString> strategy;

    if (weatherForecast.rainIntensity > 0.5f) {
        strategy.append("Rain expected - consider wet tires");
        strategy.append("Reduce aero balance for wet conditions");
    } else if (weatherForecast.rainIntensity > 0) {
        strategy.append("Light rain possible - be prepared for pit stop");
        strategy.append("Monitor track conditions closely");
    }

    if (weatherForecast.temperature < 15.0f) {
        strategy.append("Cold conditions - increase tire pressures slightly");
        strategy.append("Allow longer tire warm-up period");
    } else if (weatherForecast.temperature > 35.0f) {
        strategy.append("Hot conditions - reduce tire pressures slightly");
        strategy.append("Monitor tire temperatures closely");
    }

    if (weatherForecast.windSpeed > 20.0f) {
        strategy.append("Strong wind - adjust brake bias if needed");
        strategy.append("Be cautious on high-speed sections");
    }

    if (strategy.isEmpty()) {
        strategy.append("Stable weather conditions - no changes needed");
    }

    return strategy;
}

// ============================================================================
// SetupDocumentationModel implementation
// ============================================================================

SetupSheet SetupDocumentationModel::createSetupSheet(const QString& car, const QString& track, const ConditionState& conditions) const {
    SetupSheet sheet;
    sheet.carName = car;
    sheet.trackName = track;
    sheet.date = QDate::currentDate().toString("yyyy-MM-dd");
    sheet.temperature = conditions.trackTemperature;
    sheet.humidity = 0.5f;

    for (int i = 0; i < 4; ++i) {
        sheet.springs[i].name = QString("Spring %1").arg(i);
        sheet.springs[i].currentValue = 25000.0f;
        sheet.springs[i].minValue = 15000.0f;
        sheet.springs[i].maxValue = 40000.0f;
        sheet.springs[i].stepSize = 500.0f;

        sheet.dampersBump[i].name = QString("Bump Damper %1").arg(i);
        sheet.dampersBump[i].currentValue = 2000.0f;

        sheet.dampersRebound[i].name = QString("Rebound Damper %1").arg(i);
        sheet.dampersRebound[i].currentValue = 4000.0f;

        sheet.rideHeight[i] = 50.0f + i * 5.0f;
        sheet.camber[i] = -3.0f + (i >= 2 ? 1.0f : 0.0f);
        sheet.toe[i] = (i % 2 == 0) ? 0.1f : -0.1f;
        sheet.tirePressure[i] = 22.0f;
    }

    sheet.aeroBalance = 0.48f;
    sheet.wingAngleFront = 10.0f;
    sheet.wingAngleRear = 12.0f;
    sheet.brakeBias = 58.0f;
    sheet.brakePressure = 100.0f;

    sheet.differential.preload = 20.0f;
    sheet.differential.rampAngleAccel = 45.0f;
    sheet.differential.rampAngleDecel = 30.0f;
    sheet.differential.limitedSlipPercent = 40.0f;

    sheet.gearbox.numGears = 6;
    sheet.gearbox.ratios = {3.5f, 2.5f, 1.8f, 1.4f, 1.1f, 0.9f};
    sheet.gearbox.finalDrive = 3.8f;
    sheet.gearbox.efficiency = 0.95f;
    sheet.gearbox.shiftTime = 0.05f;

    return sheet;
}

QString SetupDocumentationModel::exportSetupToJSON(const SetupSheet& setup) const {
    QString json;
    json += "{\n";
    json += QString("  \"carName\": \"%1\",\n").arg(setup.carName);
    json += QString("  \"trackName\": \"%1\",\n").arg(setup.trackName);
    json += QString("  \"date\": \"%1\",\n").arg(setup.date);
    json += QString("  \"temperature\": %1,\n").arg(setup.temperature);
    json += QString("  \"aeroBalance\": %1,\n").arg(setup.aeroBalance);
    json += QString("  \"wingAngleFront\": %1,\n").arg(setup.wingAngleFront);
    json += QString("  \"wingAngleRear\": %1,\n").arg(setup.wingAngleRear);
    json += QString("  \"brakeBias\": %1,\n").arg(setup.brakeBias);
    json += QString("  \"brakePressure\": %1,\n").arg(setup.brakePressure);

    json += "  \"rideHeight\": [";
    for (int i = 0; i < 4; ++i) {
        json += QString("%1%2").arg(setup.rideHeight[i]).arg(i < 3 ? ", " : "");
    }
    json += "],\n";

    json += "  \"camber\": [";
    for (int i = 0; i < 4; ++i) {
        json += QString("%1%2").arg(setup.camber[i]).arg(i < 3 ? ", " : "");
    }
    json += "],\n";

    json += "  \"toe\": [";
    for (int i = 0; i < 4; ++i) {
        json += QString("%1%2").arg(setup.toe[i]).arg(i < 3 ? ", " : "");
    }
    json += "],\n";

    json += "  \"tirePressure\": [";
    for (int i = 0; i < 4; ++i) {
        json += QString("%1%2").arg(setup.tirePressure[i]).arg(i < 3 ? ", " : "");
    }
    json += "]\n";

    json += "}\n";
    return json;
}

SetupSheet SetupDocumentationModel::importSetupFromJSON(const QString& json) const {
    SetupSheet sheet;
    Q_UNUSED(json);
    return sheet;
}

SetupComparisonReport SetupDocumentationModel::compareSetupSheets(const SetupSheet& baseline, const SetupSheet& comparison) const {
    SetupComparisonReport report;
    report.baseline = baseline;
    report.comparison = comparison;
    report.lapTimeDelta = 0;

    if (std::abs(baseline.aeroBalance - comparison.aeroBalance) > 0.01f) {
        report.differences.append(QString("Aero balance: %1 -> %2")
                                     .arg(baseline.aeroBalance).arg(comparison.aeroBalance));
    }

    for (int i = 0; i < 4; ++i) {
        if (std::abs(baseline.rideHeight[i] - comparison.rideHeight[i]) > 1.0f) {
            report.differences.append(QString("Ride height %1: %2 -> %3")
                                         .arg(i).arg(baseline.rideHeight[i]).arg(comparison.rideHeight[i]));
        }
        if (std::abs(baseline.camber[i] - comparison.camber[i]) > 0.1f) {
            report.differences.append(QString("Camber %1: %2 -> %3")
                                         .arg(i).arg(baseline.camber[i]).arg(comparison.camber[i]));
        }
    }

    if (std::abs(baseline.brakeBias - comparison.brakeBias) > 0.5f) {
        report.differences.append(QString("Brake bias: %1 -> %2")
                                     .arg(baseline.brakeBias).arg(comparison.brakeBias));
    }

    if (!report.differences.isEmpty()) {
        report.recommendations.append("Review setup changes for track conditions");
    }

    return report;
}

QString SetupDocumentationModel::generateSetupReport(const SetupSheet& setup) const {
    QString report;
    report += "=== Setup Sheet ===\n\n";
    report += QString("Car: %1\n").arg(setup.carName);
    report += QString("Track: %1\n").arg(setup.trackName);
    report += QString("Date: %1\n").arg(setup.date);
    report += QString("Temperature: %1 C\n").arg(setup.temperature);
    report += "\n--- Suspension ---\n";

    for (int i = 0; i < 4; ++i) {
        report += QString("Corner %1: Spring=%2, Bump=%3, Rebound=%4\n")
                     .arg(i).arg(setup.springs[i].currentValue)
                     .arg(setup.dampersBump[i].currentValue)
                     .arg(setup.dampersRebound[i].currentValue);
        report += QString("  Ride Height=%1, Camber=%2, Toe=%3, Pressure=%4\n")
                     .arg(setup.rideHeight[i]).arg(setup.camber[i])
                     .arg(setup.toe[i]).arg(setup.tirePressure[i]);
    }

    report += "\n--- Aero ---\n";
    report += QString("Front Wing: %1 deg, Rear Wing: %2 deg, Balance: %3\n")
                 .arg(setup.wingAngleFront).arg(setup.wingAngleRear).arg(setup.aeroBalance);

    report += "\n--- Brakes ---\n";
    report += QString("Bias: %1%%, Pressure: %2 bar\n").arg(setup.brakeBias).arg(setup.brakePressure);

    report += "\n--- Differential ---\n";
    report += QString("Preload: %1 Nm, Ramp Accel: %2 deg, Ramp Decel: %3 deg\n")
                 .arg(setup.differential.preload)
                 .arg(setup.differential.rampAngleAccel)
                 .arg(setup.differential.rampAngleDecel);
    report += QString("LSD: %1%%, Locking: %2%%\n")
                 .arg(setup.differential.limitedSlipPercent)
                 .arg(setup.differential.lockingPercent);

    report += "\n--- Gearbox ---\n";
    report += QString("Gears: %1, Final Drive: %2, Efficiency: %3\n")
                 .arg(setup.gearbox.numGears)
                 .arg(setup.gearbox.finalDrive)
                 .arg(setup.gearbox.efficiency);

    if (!setup.notes.isEmpty()) {
        report += QString("\nNotes: %1\n").arg(setup.notes);
    }

    return report;
}

float SetupDocumentationModel::calculateSetupDelta(const SetupSheet& setupA, const SetupSheet& setupB) const {
    float delta = 0;
    delta += std::abs(setupA.aeroBalance - setupB.aeroBalance) * 10.0f;
    delta += std::abs(setupA.brakeBias - setupB.brakeBias) * 0.1f;

    for (int i = 0; i < 4; ++i) {
        delta += std::abs(setupA.rideHeight[i] - setupB.rideHeight[i]) * 0.01f;
        delta += std::abs(setupA.camber[i] - setupB.camber[i]) * 0.5f;
        delta += std::abs(setupA.springs[i].currentValue - setupB.springs[i].currentValue) * 0.0001f;
    }

    return delta;
}

QVector<QString> SetupDocumentationModel::analyzeSetupChanges(const SetupSheet& setup, const TelemetrySession& telemetry) const {
    QVector<QString> analysis;
    Q_UNUSED(telemetry);

    analysis.append(QString("Aero balance at %1 - review for track layout").arg(setup.aeroBalance));
    analysis.append(QString("Brake bias at %1%% - check for lockup tendency").arg(setup.brakeBias));

    for (int i = 0; i < 4; ++i) {
        if (setup.camber[i] > -2.0f) {
            analysis.append(QString("Corner %1 camber may be too low for maximum grip").arg(i));
        }
    }

    return analysis;
}

bool SetupDocumentationModel::saveSetupHistory(const SetupSheet& setup, float lapTime) const {
    Q_UNUSED(setup);
    Q_UNUSED(lapTime);
    return true;
}

QVector<SetupSheet> SetupDocumentationModel::loadSetupHistory(const QString& car, const QString& track) const {
    Q_UNUSED(car);
    Q_UNUSED(track);
    return {};
}

} // namespace physics
} // namespace ks
