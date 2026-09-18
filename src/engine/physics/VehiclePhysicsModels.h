#pragma once

/**
 * @file VehiclePhysicsModels.h
 * @brief Analysis and simulation models for vehicle physics
 *
 * Contains the high-level analysis models that were previously in VehiclePhysics.h.
 * These models handle race strategy, telemetry, setup optimization, etc.
 */

// Forward declarations to break circular include with VehiclePhysics.h
namespace ks::physics {
struct BalanceSetup;
struct ConditionState;
}

#include <QString>
#include <QVector>
#include <QVector2D>
#include <QVector3D>

namespace ks::physics {

// ============================================================================
// DriverModel (human driving behavior)
// ============================================================================

struct DriverState {
    float reactionTime = 0.25f;
    float fatigueLevel = 0;
    float focusLevel = 1;
    float aggressiveness = 0.5f;
    float consistency = 0.8f;
    float errorRate = 0;
    float totalTimeDriven = 0;
};

struct DriverInput {
    float throttle = 0;
    float brake = 0;
    float steer = 0;
};

class DriverModel {
public:
    void update(float dt, float speed, float lateralAccel, float brakingForce, float corneringLoad);
    float calculateReactionDelay(float input) const;
    float calculateFatigueEffect() const;
    float calculateErrorProbability() const;
    DriverInput simulateDriverInput(float targetSpeed, float currentSpeed, float steeringTarget) const;
    float getDriverQuality() const;
    DriverState getDriverState() const { return m_state; }
    void reset();
private:
    DriverState m_state;
};

// ============================================================================
// TireWearModel (tire degradation)
// ============================================================================

struct TireWearState {
    float treadDepth[4] = {8.0f, 8.0f, 8.0f, 8.0f};
    float surfaceTemp[4] = {30.0f, 30.0f, 30.0f, 30.0f};
    float coreTemp[4] = {35.0f, 35.0f, 35.0f, 35.0f};
    float wearRate[4] = {0};
    float totalWear[4] = {0};
    float gripFactor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
};

class TireWearModel {
public:
    void update(float dt, float slipAngle, float slipRatio, float normalLoad, float speed, float temp, int surfaceType);
    float calculateWearRate(float load, float slip, float temp) const;
    float calculateGripReduction(float wear) const;
    float calculateTemperatureEffect(float temp) const;
    TireWearState getTireCondition() const { return m_state; }
    void resetWear();
private:
    TireWearState m_state;
    float m_ambientTemp = 25.0f;
};

// ============================================================================
// FuelManagementModel (fuel consumption & strategy)
// ============================================================================

struct FuelState {
    float currentFuel = 80.0f;
    float fuelCapacity = 80.0f;
    float fuelConsumptionRate = 0;
    float lapFuelUsage = 0;
    float fuelWeight = 0;
};

struct PitStopStrategy {
    QVector<int> pitStopLaps;
    QVector<float> fuelToAdd;
    float estimatedTimeLoss = 0;
    bool optimalStrategy = false;
};

class FuelManagementModel {
public:
    void update(float dt, float throttlePosition, float rpm, float speed, float elevation);
    float calculateFuelConsumption(float throttle, float rpm) const;
    float calculateFuelWeight() const;
    float estimateFuelForLap(float speed, float aggressiveness) const;
    PitStopStrategy calculateOptimalPitStrategy(int lapsRemaining, float fuelPerLap, int trackPosition) const;
    FuelState getFuelState() const { return m_state; }
    void reset();
private:
    FuelState m_state;
    float m_baseConsumption = 1.8f;
};

// ============================================================================
// ThermalModel (tire/brake heat dissipation)
// ============================================================================

struct ThermalState {
    float tireSurfaceTemp[4] = {30.0f, 30.0f, 30.0f, 30.0f};
    float tireCoreTemp[4] = {35.0f, 35.0f, 35.0f, 35.0f};
    float brakeTemp[4] = {300.0f, 300.0f, 300.0f, 300.0f};
    float brakeDiscTemp[4] = {300.0f, 300.0f, 300.0f, 300.0f};
    float ambientTemp = 25.0f;
    float trackTemp = 35.0f;
};

class ThermalModel {
public:
    void update(float dt, float speed, float brakePressure, float slipRatio, float normalLoad, float ambientTemp);
    float calculateTireHeatGeneration(float slip, float load, float speed) const;
    float calculateTireHeatDissipation(float temp, float ambient, float speed) const;
    float calculateBrakeHeatGeneration(float brakePressure, float speed) const;
    float calculateBrakeHeatDissipation(float temp, float ambient, float speed) const;
    ThermalState getThermalState() const { return m_state; }
    void reset();
private:
    ThermalState m_state;
};

// ============================================================================
// MultiBodyDynamics (vehicle interactions)
// ============================================================================

struct ContactForce {
    float normalForce = 0;
    float frictionForce = 0;
    float impactForce = 0;
    QVector3D position;
    QVector3D direction;
};

struct CollisionResult {
    bool collided = false;
    float penetrationDepth = 0;
    QVector3D contactNormal;
    QVector3D contactPoint;
};

class MultiBodyDynamics {
public:
    void update(float dt, const QVector<QVector3D>& vehiclePositions, const QVector<QVector3D>& vehicleVelocities);
    CollisionResult checkCollision(const QVector3D& posA, const QVector3D& sizeA, const QVector3D& posB, const QVector3D& sizeB) const;
    QVector3D calculateAerodynamicWake(const QVector3D& downstreamPos, const QVector3D& upstreamVel) const;
    float calculateSlipstreamEffect(const QVector3D& myPos, const QVector3D& leadPos, float leadSpeed) const;
    float calculateDraftingBenefit(float distance, float speed) const;
    int getVehicleCount() const { return m_vehicleCount; }
private:
    int m_vehicleCount = 0;
    float m_wakeStrength = 0;
};

// ============================================================================
// TrackConditionsModel (track degradation)
// ============================================================================

struct TrackCondition {
    float gripLevel = 1.0f;
    float abrasiveness = 0.5f;
    float rubberLevel = 0;
    float dirtLevel = 0;
    float waterLevel = 0;
};

struct RubberBuildup {
    float amount = 0;
    QVector2D position;
    int lapCount = 0;
};

class TrackConditionsModel {
public:
    void update(float dt, const QVector<QVector3D>& vehiclePositions, int vehicleCount, int weather);
    float calculateRubberBuildup(const QVector3D& position, float speed, float load) const;
    float calculateTrackGripModifier(float rubber, float dirt, float water) const;
    float calculateDirtAccumulation(int weather, int traffic) const;
    float calculateWaterEvaporation(float temp, float wind, float sunExposure) const;
    TrackCondition getTrackCondition() const { return m_condition; }
    void reset();
private:
    TrackCondition m_condition;
    QVector<RubberBuildup> m_rubberMap;
};

// ============================================================================
// WeatherEffectsModel (environment effects)
// ============================================================================

struct WeatherStateEnv {
    float temperature = 25.0f;
    float humidity = 0.5f;
    float windSpeed = 0;
    float windDirection = 0;
    float rainIntensity = 0;
    float cloudCover = 0;
    float sunIntensity = 1.0f;
};

struct GripEffects {
    float airDensity = 1.225f;
    float rainGripReduction = 0;
    float temperatureGripModifier = 1.0f;
    float windLateralForce = 0;
    float windLongitudinalForce = 0;
};

class WeatherEffectsModel {
public:
    void update(float dt, const QVector3D& trackPosition, float vehicleSpeed, float vehicleHeading);
    float calculateAirDensity(float temperature, float humidity, float altitude) const;
    float calculateRainGripReduction(float intensity) const;
    QVector3D calculateWindForce(float windSpeed, float windDir, float vehicleHeading, float vehicleSpeed) const;
    float calculateVisibility(float rainIntensity, float cloudCover) const;
    float calculateTemperatureEffect(float ambientTemp) const;
    WeatherStateEnv getWeatherState() const { return m_weatherState; }
    GripEffects getGripEffects() const { return m_gripEffects; }
    void reset();
private:
    WeatherStateEnv m_weatherState;
    GripEffects m_gripEffects;
};

// ============================================================================
// RaceStrategyModel (race strategy optimization)
// ============================================================================

struct RaceState {
    int currentLap = 0;
    int totalLaps = 0;
    int trackPosition = 0;
    int pitStopsCompleted = 0;
    float currentFuel = 80.0f;
    float tireWear[4] = {0};
    float lastLapTime = 0;
};

struct StrategyOption {
    int pitLap = 0;
    float fuelToAdd = 0;
    float estimatedTotalTime = 0;
    int positionChange = 0;
    float tireAgeAtEnd = 0;
};

struct StrategyRecommendation {
    StrategyOption bestOption;
    QVector<StrategyOption> alternatives;
    float confidence = 0;
};

class RaceStrategyModel {
public:
    void update(float dt, const RaceState& raceState);
    QVector<int> calculatePitWindow(float fuelPerLap, float currentFuel, float tireDegradationRate) const;
    StrategyOption evaluateStrategyOption(int pitLap, float fuelToAdd, int trackPosition, const QVector<float>& competitors) const;
    float calculateUndercutOpportunity(int myPitLap, int competitorPitLap) const;
    float calculateOvercutOpportunity(int myPitLap, int competitorPitLap) const;
    StrategyRecommendation getRecommendation() const { return m_recommendation; }
    RaceState getRaceState() const { return m_raceState; }
    void reset();
private:
    RaceState m_raceState;
    StrategyRecommendation m_recommendation;
    float m_avgLapTime = 90.0f;
    float m_pitLossTime = 22.0f;
    float m_baseConsumption = 1.8f;
};

// ============================================================================
// TelemetryProcessingModel (telemetry data analysis)
// ============================================================================

struct TelemetryChannel {
    QString name;
    QVector<float> samples;
    int sampleRate = 100;
    float minValue = 0, maxValue = 0, currentValue = 0;
};

struct TelemetrySession {
    QString driverName, trackName;
    int totalLaps = 0;
    float bestLapTime = 0;
    QVector<TelemetryChannel> channels;
};

struct CornerAnalysis {
    int cornerNumber = 0;
    float entrySpeed = 0, midSpeed = 0, exitSpeed = 0, minSpeed = 0;
    float apexDistance = 0, exitDistance = 0;
    float gForceLateral = 0, gForceLongitudinal = 0;
    float steeringAngle = 0, throttlePosition = 0, brakePressure = 0;
};

struct ChannelStatistics {
    float mean = 0, stdDev = 0, minValue = 0, maxValue = 0, rms = 0;
};

class TelemetryProcessingModel {
public:
    TelemetrySession loadSession(const QString& filePath) const;
    bool saveSession(const TelemetrySession& session, const QString& filePath) const;
    ChannelStatistics calculateChannelStatistics(const TelemetryChannel& channel) const;
    QVector<float> smoothChannel(const TelemetryChannel& channel, int windowSize) const;
    QVector<int> detectPeaks(const TelemetryChannel& channel, float threshold) const;
    float calculateCrossCorrelation(const TelemetryChannel& ch1, const TelemetryChannel& ch2) const;
    QVector<float> calculateFFT(const TelemetryChannel& channel) const;
    QVector<CornerAnalysis> segmentLapIntoCorners(const TelemetrySession& session) const;
    float compareLaps(const TelemetrySession& lap1, const TelemetrySession& lap2) const;
    QVector<float> filterNoise(const TelemetryChannel& channel, float cutoffFreq) const;
private:
    float calculateMean(const QVector<float>& data) const;
    float calculateRms(const QVector<float>& data) const;
};

// ============================================================================
// VehiclePerformanceAnalysisModel (performance metrics)
// ============================================================================

struct PerformanceMetrics {
    float topSpeed = 0, acceleration0_100 = 0, acceleration0_200 = 0;
    float brakingDistance100_0 = 0;
    float lateralGMax = 0, longitudinalGMax = 0;
    float lapTimePrediction = 0, fuelEfficiency = 0, tireLifeEstimate = 0;
};

struct PerformanceMap {
    float speed = 0, throttle = 0, brake = 0;
    float lateralG = 0, longitudinalG = 0;
    float efficiency = 0, lapTimeContribution = 0;
};

struct PerformanceDelta {
    float speedDelta = 0, timeDelta = 0, gForceDelta = 0, fuelDelta = 0;
};

class VehiclePerformanceAnalysisModel {
public:
    PerformanceMetrics analyzeTelemetry(const TelemetrySession& session) const;
    QVector<PerformanceMap> calculatePerformanceMap(const TelemetrySession& session) const;
    float predictLapTime(const BalanceSetup& setup, const ConditionState& conditions) const;
    PerformanceDelta calculatePerformanceDelta(const PerformanceMetrics& baseline, const PerformanceMetrics& comparison) const;
    QString identifyLimitingFactor(const TelemetrySession& session) const;
    QVector<float> calculateOptimalThrottleProfile(const TelemetrySession& session) const;
    QVector<float> calculateOptimalBrakeProfile(const TelemetrySession& session) const;
    QVector<float> calculateSpeedTrace(const TelemetrySession& session) const;
    QString generatePerformanceReport(const TelemetrySession& session) const;
};

// ============================================================================
// SetupOptimizationModel (setup optimization)
// ============================================================================

struct SetupParameter {
    QString name;
    float currentValue = 0, minValue = 0, maxValue = 0, stepSize = 0;
    bool isFixed = false;
};

struct OptimizationRecommendation {
    QString parameter;
    float suggestedValue = 0;
    float expectedImprovement = 0;
    float confidence = 0;
    QString reason;
};

struct SetupComparison {
    float lapTimeDelta = 0;
    float straightLineDelta = 0;
    float corneringDelta = 0;
    float brakingDelta = 0;
    float stabilityDelta = 0;
};

class SetupOptimizationModel {
public:
    QVector<OptimizationRecommendation> analyzeSetupEffect(const BalanceSetup& setup, const TelemetrySession& telemetry) const;
    QVector<OptimizationRecommendation> suggestSetupChanges(const BalanceSetup& currentSetup, const PerformanceMetrics& targetPerformance) const;
    SetupComparison compareSetups(const BalanceSetup& setupA, const BalanceSetup& setupB, const ConditionState& trackConditions) const;
    QVector<float> calculateOptimalSprings(float mass, float wheelbase, const QString& track) const;
    QVector<float> calculateOptimalDampers(const BalanceSetup& setup, const QString& track) const;
    struct AeroBalanceResult { float frontDownforce = 0, rearDownforce = 0, balance = 0; };
    AeroBalanceResult calculateOptimalAero(float speed, float mass, const QString& track) const;
    QVector<float> calculateOptimalCamber(float suspensionTravel, float gripTarget, const QString& track) const;
    QVector<float> calculateOptimalToe(float speed, const QString& track) const;
    QString generateSetupReport(const BalanceSetup& setup, const TelemetrySession& telemetry) const;
};

// ============================================================================
// TireTestingModel (tire characterization)
// ============================================================================

struct TireTestData {
    float slipAngle = 0, slipRatio = 0, normalLoad = 0;
    float camber = 0, inflationPressure = 0;
    float lateralForce = 0, longitudinalForce = 0;
    float aligningMoment = 0, temperature = 0;
};

struct TireTestResult {
    float corneringStiffness = 0, peakSlipAngle = 0, peakLateralForce = 0;
    float peakSlipRatio = 0, peakLongitudinalForce = 0;
    float frictionCircle = 0;
    float optimalPressure = 0, optimalCamber = 0;
};

struct TireCharacterization {
    TireTestResult dry, wet, cold, hot;
    float thermalDegRate = 0, wearRate = 0;
};

class TireTestingModel {
public:
    QVector<TireTestData> performCorneringTest(float maxSlipAngle, float load) const;
    QVector<TireTestData> performLongitudinalTest(float maxSlipRatio, float load) const;
    QVector<TireTestData> performBrakeTest(float maxSlipRatio, float load) const;
    QVector<TireTestData> performDriveTest(float maxSlipRatio, float load) const;
    TireTestResult analyzeTestData(const QVector<TireTestData>& data) const;
    float calculateFrictionCircle(const QVector<TireTestData>& testData) const;
    float calculateTireStiffness(const QVector<TireTestData>& testData) const;
    QVector<float> fitPacejkaCoefficients(const QVector<TireTestData>& testData) const;
    TireCharacterization characterizeTire(const QVector<TireTestResult>& testResults) const;
};

// ============================================================================
// AerodynamicTestingModel (aero testing)
// ============================================================================

struct AeroTestData {
    float speed = 0, yawAngle = 0;
    float rideHeightFront = 0, rideHeightRear = 0;
    float downforceFront = 0, downforceRear = 0, dragForce = 0;
    float frontLiftCoeff = 0, rearLiftCoeff = 0, dragCoeff = 0;
};

struct AeroTestResult {
    float optimalRideHeight = 0, maxDownforce = 0, minDrag = 0, bestLiftDragRatio = 0;
    float balancePoint = 0, pitchSensitivity = 0;
};

struct AeroMap {
    float speed = 0, rideHeight = 0, yawAngle = 0;
    float downforce = 0, drag = 0, efficiency = 0;
};

class AerodynamicTestingModel {
public:
    AeroTestData calculateAeroForces(float speed, float rideHeightFront, float rideHeightRear, float yawAngle) const;
    QVector<AeroTestData> performWindTunnelTest(const QVector<float>& speedRange, const QVector<float>& rideHeightRange) const;
    AeroTestResult analyzeAeroData(const QVector<AeroTestData>& data) const;
    float calculateAeroBalance(float frontDownforce, float rearDownforce) const;
    float calculateLiftDragRatio(float downforce, float drag) const;
    QVector<AeroMap> generateAeroMap(const QVector<AeroTestData>& testData) const;
    QVector<OptimizationRecommendation> optimizeAeroBalance(float targetBalance, const BalanceSetup& currentSetup) const;
    AeroTestData predictAeroEffect(float speed, const BalanceSetup& setup) const;
};

// ============================================================================
// SuspensionTuningModel (suspension tuning)
// ============================================================================

struct SuspensionTuningGeometry {
    float rollCenterHeight = 0, scrubRadius = 0;
    float casterAngle = 0, kingPinAngle = 0;
    float camberGain = 0, toeChange = 0;
    float antiDive = 0, antiSquat = 0;
    float mechanicalTrail = 0;
};

struct SuspensionCompliance {
    float lateralCompliance = 0, longitudinalCompliance = 0, verticalCompliance = 0;
    float steerCompliance = 0, camberCompliance = 0;
};

struct SuspensionRate {
    float wheelRate = 0, motionRatio = 0, rollStiffness = 0;
    float heaveStiffness = 0, pitchStiffness = 0;
};

class SuspensionTuningModel {
public:
    QVector3D calculateRollCenter(const SuspensionTuningGeometry& geo) const;
    float calculateScrubRadius(const SuspensionTuningGeometry& geo) const;
    float calculateCamberGain(const SuspensionTuningGeometry& geo, float travel) const;
    float calculateToeChange(const SuspensionTuningGeometry& geo, float travel) const;
    float calculateAntiDive(const SuspensionTuningGeometry& geo) const;
    float calculateAntiSquat(const SuspensionTuningGeometry& geo) const;
    float calculateMechanicalTrail(const SuspensionTuningGeometry& geo) const;
    SuspensionCompliance analyzeCompliance(const SuspensionTuningGeometry& geometry, const BalanceSetup& springs, const BalanceSetup& dampers) const;
    SuspensionRate calculateSuspensionRates(const SuspensionTuningGeometry& geometry, const BalanceSetup& springs) const;
    QVector<OptimizationRecommendation> optimizeSuspensionGeometry(float mass, float wheelbase, const QString& track) const;
};

// ============================================================================
// EngineMappingModel (engine mapping & calibration)
// ============================================================================

struct EngineMap {
    float rpm = 0, throttlePosition = 0;
    float torque = 0, power = 0, fuelFlow = 0;
    float ignitionTiming = 0, fuelInjection = 0;
};

struct PowerCurve {
    QVector<float> rpm, torque, power, fuelFlow;
    float peakPower = 0, peakTorque = 0;
    float optimalShiftPoint = 0;
};

struct EngineCalibration {
    float fuelAirRatio = 0, ignitionAdvance = 0, revLimit = 0;
    QVector<EngineMap> maps;
};

class EngineMappingModel {
public:
    PowerCurve calculatePowerCurve(const QVector<float>& accelerationProfile) const;
    QVector<float> calculateTorqueCurve(const QVector<float>& rpmRange) const;
    float calculateFuelFlow(float rpm, float throttle) const;
    float calculateOptimalShiftPoint(const PowerCurve& powerCurve) const;
    float calculateAcceleration(float rpm, int gear, float weight) const;
    EngineCalibration loadEngineCalibration(const QString& filePath) const;
    EngineCalibration optimizeEngineMap(const PerformanceMetrics& targetPerformance, const BalanceSetup& constraints) const;
    float calculateEngineBraking(float rpm, float throttle) const;
};

// ============================================================================
// DifferentialTuningModel (differential tuning)
// ============================================================================

struct DiffSettings {
    float preload = 0, rampAngleAccel = 0, rampAngleDecel = 0;
    float limitedSlipPercent = 0, lockingPercent = 0;
    float viscousCoefficient = 0, speedSensitiveCoeff = 0;
};

struct DiffBehavior {
    float cornerEntryDiff = 0, midCornerDiff = 0, exitDiff = 0;
    float straightLineDiff = 0;
    float transitionSmoothness = 0;
    float turnInResponse = 0, exitTraction = 0;
};

struct DiffTestResult {
    float turnInResponse = 0, midCornerStability = 0, exitTraction = 0;
    float straightLineStability = 0;
    float tireWearImpact = 0;
};

class DifferentialTuningModel {
public:
    float calculateDiffTorque(float inputTorque, float wheelSpeedDiff, const DiffSettings& settings) const;
    DiffBehavior analyzeDiffBehavior(float speed, float lateralAccel, float throttle, const DiffSettings& settings) const;
    DiffSettings optimizeDiffSetup(float mass, float power, const QString& track, const QString& drivingStyle) const;
    DiffTestResult testDiffResponse(const DiffSettings& settings, const ConditionState& conditions) const;
    float calculatePreloadEffect(float preload, float speed) const;
    float calculateRampEffect(float rampAngle, float torque, float speed) const;
    float calculateLockingPercent(const DiffSettings& settings, float speed) const;
    QVector<OptimizationRecommendation> suggestDiffChanges(const DiffSettings& currentSettings, const DiffBehavior& targetBehavior) const;
};

// ============================================================================
// GearboxModel (gearbox & transmission)
// ============================================================================

struct GearRatio {
    int gear = 0;
    float ratio = 0, theoreticalSpeed = 0, acceleration = 0;
};

struct ShiftPoint {
    int fromGear = 0, toGear = 0;
    float rpm = 0, speed = 0, timeLost = 0;
};

struct GearboxConfig {
    int numGears = 6;
    QVector<float> ratios;
    float finalDrive = 0, efficiency = 0.95f, shiftTime = 0.05f;
};

class GearboxModel {
public:
    QVector<GearRatio> calculateGearSpeeds(const QVector<float>& ratios, float finalDrive, float tireCircumference) const;
    float calculateAcceleration(float rpm, int gear, float torque, float weight) const;
    float calculateTheoreticalTopSpeed(int gear, const QVector<float>& ratios, float finalDrive, float tireCircumference) const;
    QVector<ShiftPoint> calculateOptimalShiftPoints(const PowerCurve& powerCurve, const QVector<float>& ratios, float finalDrive) const;
    QVector<float> optimizeGearRatios(float topSpeed, float acceleration, const QString& track) const;
    float calculateFuelConsumptionPerGear(float throttle, float rpm) const;
    float calculateLapTimeWithGearing(const GearboxConfig& setup, const QString& track) const;
    QString analyzeShiftStrategy(const QVector<ShiftPoint>& shiftPoints, const PowerCurve& powerCurve) const;
};

// ============================================================================
// BrakeSystemModel (brake system analysis)
// ============================================================================

struct BrakeSystemConfig {
    float frontDiscDiameter = 0, rearDiscDiameter = 0;
    float frontPadArea = 0, rearPadArea = 0;
    float bias = 0, masterCylinderDiameter = 0;
    float pedalRatio = 0, boostRatio = 0;
    float maxBrakePressure = 100.0f;
};

struct BrakePerformance {
    float stoppingDistance100_0 = 0, stoppingDistance200_0 = 0;
    float brakeBalance = 0, fadeResistance = 0;
    float pedalFeel = 0, modulation = 0;
};

struct BrakeThermal {
    float discTemp[4] = {0};
    float padTemp[4] = {0};
    float fadeFactor[4] = {0};
    float heatGeneration[4] = {0};
    float heatDissipation[4] = {0};
};

class BrakeSystemModel {
public:
    float calculateBrakeForce(float pressure, const BrakeSystemConfig& config) const;
    float calculateBrakeBias(const BrakeSystemConfig& config) const;
    float calculateStoppingDistance(float speed, float weight, float brakeForce) const;
    float calculatePedalForce(float acceleration, const BrakeSystemConfig& config) const;
    float calculateBrakeTorque(float pressure, float discDiameter, float padArea) const;
    BrakePerformance analyzeBrakePerformance(float speed, float weight, const BrakeSystemConfig& config) const;
    float calculateBrakeFade(float discTemp, float padTemp) const;
    float calculateBrakeBalanceForVehicle(float mass, float wheelbase, float frontDist, const QString& track) const;
    QVector<OptimizationRecommendation> optimizeBrakeBias(float mass, float wheelbase, float frontDist, const QString& track) const;
};

// ============================================================================
// DriverTrainingModel (driver coaching)
// ============================================================================

struct DriverPerformance {
    float throttleControl = 0, brakeControl = 0, steeringSmoothness = 0;
    float racingLineAccuracy = 0, apexHitRate = 0;
    float consistency = 0, improvement = 0;
};

struct DrivingError {
    float severity = 0;
    QString type;
    float position = 0;
    float speed = 0;
    QString suggestion;
};

struct DriverCoaching {
    QVector<DrivingError> errors;
    float overallScore = 0;
    QVector<QString> priorities;
    float estimatedLapTimeImprovement = 0;
};

class DriverTrainingModel {
public:
    DriverPerformance analyzeDriverPerformance(const TelemetrySession& session) const;
    QVector<DrivingError> identifyDrivingErrors(const TelemetrySession& session) const;
    QVector<float> calculateRacingLineDeviation(const TelemetrySession& session, const QVector<float>& idealLine) const;
    float calculateBrakeTrailBraking(const TelemetrySession& session) const;
    QVector<float> calculateThrottleApplication(const TelemetrySession& session) const;
    QVector<float> calculateSteeringInputs(const TelemetrySession& session) const;
    DriverCoaching generateCoachingReport(const TelemetrySession& session) const;
    QString compareDriverPerformance(const TelemetrySession& driver1, const TelemetrySession& driver2) const;
    QVector<QString> suggestDriverImprovements(const DriverPerformance& performance) const;
};

// ============================================================================
// RaceEngineeringModel (race engineering)
// ============================================================================

struct RaceCommunication {
    float engineerMessageTime = 0;
    QString message;
    bool priority = false;
};

struct RaceData {
    int position = 0, totalLaps = 0;
    float gapAhead = 0, gapBehind = 0;
    float currentLapTime = 0, predictedLapTime = 0;
    float fuelRemaining = 0, lapsOfFuel = 0;
};

struct RaceInstruction {
    QString type;
    float value = 0;
    QString reason;
    bool urgent = false;
};

class RaceEngineeringModel {
public:
    QVector<RaceInstruction> analyzeRaceSituation(const RaceData& raceData) const;
    QVector<int> calculatePitWindow(float fuelPerLap, float currentFuel) const;
    float calculateGapDelta(float myLapTime, float competitorLapTime) const;
    int calculatePositionChange(const BalanceSetup& strategy, const QVector<float>& competitors) const;
    QVector<RaceInstruction> generateRaceInstructions(const RaceData& raceData, const BalanceSetup& strategy) const;
    QVector<QString> analyzeTireStrategy(const QString& currentTires, int remainingLaps) const;
    QVector<QString> calculateFuelStrategy(float fuelRemaining, float fuelPerLap, int lapsRemaining) const;
    QVector<QString> calculateWeatherStrategy(const WeatherStateEnv& weatherForecast, const BalanceSetup& currentSetup) const;
};

// ============================================================================
// SetupDocumentationModel (setup documentation)
// ============================================================================

struct SetupSheet {
    QString carName, trackName, date;
    float temperature = 0, humidity = 0;
    SetupParameter springs[4];
    SetupParameter dampersBump[4];
    SetupParameter dampersRebound[4];
    float rideHeight[4] = {0};
    float camber[4] = {0};
    float toe[4] = {0};
    float aeroBalance = 0, wingAngleFront = 0, wingAngleRear = 0;
    float tirePressure[4] = {0};
    DiffSettings differential;
    float brakeBias = 0, brakePressure = 0;
    GearboxConfig gearbox;
    QString notes;
};

struct SetupComparisonReport {
    SetupSheet baseline, comparison;
    float lapTimeDelta = 0;
    QVector<QString> differences;
    QVector<QString> recommendations;
};

class SetupDocumentationModel {
public:
    SetupSheet createSetupSheet(const QString& car, const QString& track, const ConditionState& conditions) const;
    QString exportSetupToJSON(const SetupSheet& setup) const;
    SetupSheet importSetupFromJSON(const QString& json) const;
    SetupComparisonReport compareSetupSheets(const SetupSheet& baseline, const SetupSheet& comparison) const;
    QString generateSetupReport(const SetupSheet& setup) const;
    float calculateSetupDelta(const SetupSheet& setupA, const SetupSheet& setupB) const;
    QVector<QString> analyzeSetupChanges(const SetupSheet& setup, const TelemetrySession& telemetry) const;
    bool saveSetupHistory(const SetupSheet& setup, float lapTime) const;
    QVector<SetupSheet> loadSetupHistory(const QString& car, const QString& track) const;
};

} // namespace ks::physics
