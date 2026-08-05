#include "phys_Simulator.h"
#include "modules/PhysicsEditor/simulator/phys_TireModel.h"
#include "modules/PhysicsEditor/simulator/phys_LapTimer.h"
#include "modules/PhysicsEditor/PhysicsProfiler.h"
#include <QDebug>
#include <QtMath>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include "EngineModel.h"
#include "AeroModel.h"
#include "DifferentialModel.h"
#include "PacejkaTireModel.h"
#include "SuspensionModel.h"
#include "BrakeThermalModel.h"
#include "HybridSystem.h"

namespace ks {

// ============================================================================
// phys_Simulator::Impl (PIMPL - contains all the physics implementation)
// ============================================================================

class phys_Simulator::Impl {
public:
    Impl(phys_Simulator* owner) : m_owner(owner) {
        m_state = {};

        // Initialize default tire model
        m_tireModel.name = "Street 90s";
        m_tireModel.compound = "Street";
        m_tireModel.peakSlipAngle = 8.0;
        m_tireModel.peakSlipRatio = 0.12;
        m_tireModel.peakLateralMu = 1.0;
        m_tireModel.peakLongitudinalMu = 1.1;
        m_tireModel.stiffnessLateral = 30000.0;
        m_tireModel.stiffnessLongitudinal = 50000.0;
        m_tireEngine.setSlipCurve(m_tireModel);

        // Initialize engine with V8 preset as default
        m_engineModel.setConfig(EngineModel::getV8_4000());

        // Initialize aero with GT3 preset as default
        AeroModel::AeroConfig aeroConfig = AeroModel::getGT3Config();
        for (const AeroModel::Wing& wing : aeroConfig.wings) {
            m_aeroModel.addWing(wing);
        }

        // Initialize differential with clutch LSD
        m_diffModel.setConfig(DifferentialModel::getLSDClutch());

        // Initialize ERS with F1 2014 preset
        m_hybridSystem.setConfig(HybridSystem::getF1_2014());

        // Store base CD for DRS
        m_drsBaseCd = m_cd;
    }

    // State
    SimulationState m_state;
    double m_throttle = 0.0;
    double m_brake = 0.0;
    double m_steering = 0.0;
    bool m_running = false;

    // Vehicle params
    double m_mass = 1500.0;
    double m_enginePowerKw = 350.0;
    double m_maxRpm = 7500.0;
    double m_cd = 0.35;
    double m_frontalArea = 2.0;
    double m_wheelBase = 2.7;
    double m_trackWidth = 1.6;
    int m_currentGear = 1;
    QVector<double> m_gearRatios = {3.5, 2.5, 1.8, 1.4, 1.1, 0.9};
    double m_finalDriveRatio = 3.8;
    double m_wheelRadius = 0.33;
    double m_fuelKg = 80.0;

    // Vehicle geometry for weight transfer
    double m_cgHeight = 0.45;
    double m_frontAxleDist = 1.35;
    double m_rearAxleDist = 1.35;
    double m_rollStiffness = 15000.0;
    double m_steerLock = 22.0;
    double m_steerRatio = 15.0;

    // Tire model (legacy, kept for backward compat)
    TireSlipCurve m_tireModel;
    phys_TireModel m_tireEngine;

    // Full sub-models
    PacejkaTireModel m_pacejkaModel;
    EngineModel m_engineModel;
    AeroModel m_aeroModel;
    DifferentialModel m_diffModel;
    SuspensionModelManager m_suspensionModel;
    BrakeModelManager m_brakeModel;

    // ABS/TC
    bool m_absEnabled = false;
    bool m_tcEnabled = false;
    double m_absSlipThreshold = 0.15;
    double m_tcSlipThreshold = 0.12;

    // Per-wheel state
    WheelState m_wheels[4];
    double m_yawRate = 0.0;
    double m_lateralAccel = 0.0;

    // Tire graining/blistering state
    double m_tireGraining[4] = {0.0, 0.0, 0.0, 0.0};
    double m_tireBlistering[4] = {0.0, 0.0, 0.0, 0.0};

    // Thermal tire model state
    double m_tireTempSurface[4] = {30.0, 30.0, 30.0, 30.0};
    double m_tireTempCarcass[4] = {35.0, 35.0, 35.0, 35.0};
    double m_tireTempCore[4] = {40.0, 40.0, 40.0, 40.0};
    double m_tirePressure[4] = {2.4, 2.4, 2.4, 2.4};
    double m_tireWear[4] = {0.0, 0.0, 0.0, 0.0};
    double m_ambientTemp = 26.0;

    // ERS/Hybrid system
    HybridSystem m_hybridSystem;
    double m_ersDeployTorque = 0.0;
    double m_ersRegenTorque = 0.0;

    // DRS
    bool m_drsEnabled = false;
    bool m_drsAutoActivate = false;
    bool m_drsActive = false;
    double m_drsSpeedThreshold = 80.0;
    double m_drsZoneStart = 0.0;
    double m_drsZoneEnd = 0.0;
    double m_drsDragReduction = 0.25;
    double m_drsBaseCd = 0.35;

    // Damage model
    bool m_damageEnabled = false;
    DamageState m_damage;

    // Weather-dependent physics
    WeatherState m_weather;
    double m_aquaplaningRisk = 0.0;
    double m_trackGripReduction = 0.0;

    // Drive layout
    DriveLayout m_driveLayout = DriveLayout::RWD;
    double m_centerDiffPreload = 0.0;
    double m_centerDiffPower = 0.5;
    double m_frontTorqueSplit = 0.0;

    // Fuel consumption
    bool m_fuelConsumptionEnabled = true;
    double m_fuelCapacity = 80.0;

    // Lap timer
    phys_LapTimer m_lapTimer;
    QElapsedTimer m_simTimer;
    double m_trackLength = 5000.0;
    double m_lastUpdateTime = 0.0;

    // History
    QVector<double> m_lapTimeHistory;

    phys_Simulator* m_owner;

    // --- Private helper methods ---
    double calculateSlipAngle(int wheel, double steeringAngle, double speed, double yawRate) const;
    double calculateSlipRatio(int wheel) const;
    void updateWeightTransfer(double longitudinalAccel, double lateralAccel);
    void updatePerWheelForces(double dt);
    void updateTireModel(double dt);
    void updateErsAndDrs(double dt);
    void updateDamageModel(double dt);
    void updateWeatherEffects(double dt);
    void updateFuelWeight(double dt);
    void applyCollisionDamage(double impactForce);
    void loadEngineFromIni(const QString& engineIniPath);
    void loadTyresFromIni(const QString& tyresIniPath);
    void loadDrivetrainFromIni(const QString& drivetrainIniPath);
    void loadAeroFromIni(const QString& aeroIniPath);
    void loadSuspensionFromIni(const QString& suspensionIniPath);
    void setFrontRearTorqueSplit(double frontRatio);
};

// ============================================================================
// phys_Simulator implementation (forwards to Impl)
// ============================================================================

phys_Simulator* phys_Simulator::s_instance = nullptr;

phys_Simulator::phys_Simulator(QObject* parent)
    : QObject(parent), m_impl(new Impl(this))
{
}

phys_Simulator::~phys_Simulator() {
    delete m_impl;
}

phys_Simulator* phys_Simulator::instance() {
    if (!s_instance) {
        s_instance = new phys_Simulator();
    }
    return s_instance;
}

void phys_Simulator::startSimulation() {
    m_impl->m_running = true;
    m_impl->m_simTimer.start();
    m_impl->m_lapTimer.startLap();
    emit simulationStarted();
    qDebug() << "Physics simulation started";
}

void phys_Simulator::stopSimulation() {
    m_impl->m_running = false;
    m_impl->m_lapTimer.stopLap();
    if (m_impl->m_currentGear > 0) {
        m_impl->m_lapTimeHistory.append(m_impl->m_lapTimer.currentLapTime());
    }
    emit simulationStopped();
}

void phys_Simulator::reset() {
    m_impl->m_state = {};
    m_impl->m_throttle = 0.0;
    m_impl->m_brake = 0.0;
    m_impl->m_steering = 0.0;
    m_impl->m_currentGear = 1;
    m_impl->m_fuelKg = 80.0;
    m_impl->m_yawRate = 0.0;
    m_impl->m_lateralAccel = 0.0;
    for (int i = 0; i < 4; ++i) {
        m_impl->m_wheels[i] = WheelState();
        m_impl->m_tireTempSurface[i] = 30.0;
        m_impl->m_tireTempCarcass[i] = 35.0;
        m_impl->m_tireTempCore[i] = 40.0;
        m_impl->m_tirePressure[i] = 2.4;
        m_impl->m_tireWear[i] = 0.0;
    }
    m_impl->m_engineModel.reset();
    m_impl->m_diffModel.reset();
    m_impl->m_brakeModel.reset();
    m_impl->m_hybridSystem.reset();
    m_impl->m_drsActive = false;
    m_impl->m_cd = m_impl->m_drsBaseCd;
    m_impl->m_damage = DamageState();
    m_impl->m_aquaplaningRisk = 0.0;
    m_impl->m_trackGripReduction = 0.0;
    m_impl->m_lapTimer.reset();
    emit stateUpdated(m_impl->m_state);
}

void phys_Simulator::setThrottle(double value) {
    m_impl->m_throttle = std::clamp(value, 0.0, 1.0);
}

void phys_Simulator::setBrake(double value) {
    m_impl->m_brake = std::clamp(value, 0.0, 1.0);
}

void phys_Simulator::setSteering(double value) {
    m_impl->m_steering = std::clamp(value, -1.0, 1.0);
}

void phys_Simulator::setTireModel(const TireSlipCurve& curve) {
    m_impl->m_tireModel = curve;
    m_impl->m_tireEngine.setSlipCurve(curve);
}

SimulationState phys_Simulator::getState() const {
    return m_impl->m_state;
}

bool phys_Simulator::isRunning() const {
    return m_impl->m_running;
}

TireSlipCurve phys_Simulator::tireModel() const {
    return m_impl->m_tireModel;
}

phys_TireModel* phys_Simulator::tireModelEngine() {
    return &m_impl->m_tireEngine;
}

phys_LapTimer* phys_Simulator::lapTimer() {
    return &m_impl->m_lapTimer;
}

HybridSystem& phys_Simulator::hybridSystem() { return m_impl->m_hybridSystem; }
const HybridSystem& phys_Simulator::hybridSystem() const { return m_impl->m_hybridSystem; }
BrakeModelManager& phys_Simulator::brakeModel() { return m_impl->m_brakeModel; }
const BrakeModelManager& phys_Simulator::brakeModel() const { return m_impl->m_brakeModel; }

LapTimeEstimate phys_Simulator::estimateLapTime() const {
    return m_impl->m_lapTimer.estimateLapTime(m_impl->m_lapTimeHistory, m_impl->m_trackLength, 80.0);
}

void phys_Simulator::setMass(double kg) { m_impl->m_mass = kg; }
void phys_Simulator::setEnginePower(double kw) { m_impl->m_enginePowerKw = kw; }
void phys_Simulator::setMaxRpm(double rpm) { m_impl->m_maxRpm = rpm; }
void phys_Simulator::setDragCoeff(double cd) { m_impl->m_cd = cd; }
void phys_Simulator::setFrontalArea(double area) { m_impl->m_frontalArea = area; }
void phys_Simulator::setWheelBase(double wb) { m_impl->m_wheelBase = wb; }
void phys_Simulator::setTrackWidth(double tw) { m_impl->m_trackWidth = tw; }

double phys_Simulator::mass() const { return m_impl->m_mass; }
double phys_Simulator::enginePower() const { return m_impl->m_enginePowerKw; }
double phys_Simulator::maxRpm() const { return m_impl->m_maxRpm; }

void phys_Simulator::setAbsEnabled(bool enabled) { m_impl->m_absEnabled = enabled; }
bool phys_Simulator::absEnabled() const { return m_impl->m_absEnabled; }
void phys_Simulator::setTractionControlEnabled(bool enabled) { m_impl->m_tcEnabled = enabled; }
bool phys_Simulator::tractionControlEnabled() const { return m_impl->m_tcEnabled; }
void phys_Simulator::setAbsThreshold(double slipRatio) { m_impl->m_absSlipThreshold = slipRatio; }
void phys_Simulator::setTcThreshold(double slipRatio) { m_impl->m_tcSlipThreshold = slipRatio; }
double phys_Simulator::absThreshold() const { return m_impl->m_absSlipThreshold; }
double phys_Simulator::tcThreshold() const { return m_impl->m_tcSlipThreshold; }

float phys_Simulator::getBrakeDiscTemp(int wheel) const {
    if (wheel < 0 || wheel > 3) return 0.0f;
    return m_impl->m_brakeModel.getBrake(wheel).calculateDiscTemp();
}

float phys_Simulator::getBrakePadTemp(int wheel) const {
    if (wheel < 0 || wheel > 3) return 0.0f;
    return m_impl->m_brakeModel.getBrake(wheel).calculatePadTemp();
}

float phys_Simulator::getBrakeFade(int wheel) const {
    if (wheel < 0 || wheel > 3) return 1.0f;
    return m_impl->m_brakeModel.getBrake(wheel).calculateBrakeFade();
}

void phys_Simulator::setErsEnabled(bool enabled) {
    if (enabled && !m_impl->m_hybridSystem.isEnabled()) {
        m_impl->m_hybridSystem.setConfig(HybridSystem::getF1_2014());
    } else if (!enabled) {
        m_impl->m_hybridSystem.setConfig(HybridSystem::getDisabled());
    }
}
bool phys_Simulator::ersEnabled() const { return m_impl->m_hybridSystem.isEnabled(); }
void phys_Simulator::setErsMode(int mode) { m_impl->m_hybridSystem.setDeploymentMode(static_cast<HybridSystem::ErsMode>(mode)); }
void phys_Simulator::activateErsAttackMode() { m_impl->m_hybridSystem.activateAttackMode(); }

void phys_Simulator::setDriveLayout(DriveLayout layout) { m_impl->m_driveLayout = layout; }
DriveLayout phys_Simulator::driveLayout() const { return m_impl->m_driveLayout; }
void phys_Simulator::setCenterDiffPreload(double nm) { m_impl->m_centerDiffPreload = nm; }
double phys_Simulator::centerDiffPreload() const { return m_impl->m_centerDiffPreload; }
void phys_Simulator::setCenterDiffPower(double power) { m_impl->m_centerDiffPower = power; }
double phys_Simulator::centerDiffPower() const { return m_impl->m_centerDiffPower; }
void phys_Simulator::setFrontRearTorqueSplit(double frontRatio) { m_impl->setFrontRearTorqueSplit(frontRatio); }
float phys_Simulator::getErsDeployTorque() const { return m_impl->m_ersDeployTorque; }
float phys_Simulator::getErsRegenTorque() const { return m_impl->m_ersRegenTorque; }
float phys_Simulator::getErsBatterySoc() const { return m_impl->m_hybridSystem.getSoc(); }
float phys_Simulator::getErsBatteryTemp() const { return m_impl->m_hybridSystem.getBatteryTemp(); }

void phys_Simulator::setDrsEnabled(bool enabled) { m_impl->m_drsEnabled = enabled; }
bool phys_Simulator::drsEnabled() const { return m_impl->m_drsEnabled; }
void phys_Simulator::setDrsAutoActivate(bool autoActivate) { m_impl->m_drsAutoActivate = autoActivate; if (!autoActivate) m_impl->m_drsActive = false; }
bool phys_Simulator::drsAutoActivate() const { return m_impl->m_drsAutoActivate; }
void phys_Simulator::setDrsSpeedThreshold(double kmh) { m_impl->m_drsSpeedThreshold = kmh; }
double phys_Simulator::drsSpeedThreshold() const { return m_impl->m_drsSpeedThreshold; }
void phys_Simulator::setDrsZoneStart(double dist) { m_impl->m_drsZoneStart = dist; }
void phys_Simulator::setDrsZoneEnd(double dist) { m_impl->m_drsZoneEnd = dist; }
bool phys_Simulator::isDrsActive() const { return m_impl->m_drsActive; }
double phys_Simulator::getDrsDragReduction() const { return m_impl->m_drsDragReduction; }
void phys_Simulator::setDrsDragReduction(double factor) { m_impl->m_drsDragReduction = std::clamp(factor, 0.0, 1.0); }

const DamageState& phys_Simulator::damageState() const { return m_impl->m_damage; }
DamageState& phys_Simulator::damageState() { return m_impl->m_damage; }
void phys_Simulator::applyCollisionDamage(double impactForce) { m_impl->applyCollisionDamage(impactForce); }
void phys_Simulator::resetDamage() { m_impl->m_damage = DamageState(); }
void phys_Simulator::enableDamageModel(bool enabled) { m_impl->m_damageEnabled = enabled; }
bool phys_Simulator::isDamageModelEnabled() const { return m_impl->m_damageEnabled; }

void phys_Simulator::setWeatherState(const WeatherState& weather) {
    m_impl->m_weather = weather;
    m_impl->m_ambientTemp = weather.ambientTemp;
}
const WeatherState& phys_Simulator::weatherState() const { return m_impl->m_weather; }
WeatherState& phys_Simulator::weatherState() { return m_impl->m_weather; }
void phys_Simulator::setTrackWetness(double wetness) {
    m_impl->m_weather.trackWetness = std::clamp(wetness, 0.0, 1.0);
    m_impl->m_trackGripReduction = m_impl->m_weather.trackWetness * 0.4;
}
void phys_Simulator::setRainIntensity(double mmh) { m_impl->m_weather.rainIntensity = std::max(0.0, mmh); }
double phys_Simulator::getAquaplaningRisk() const { return m_impl->m_aquaplaningRisk; }
double phys_Simulator::getTrackGripReduction() const { return m_impl->m_trackGripReduction; }
void phys_Simulator::setAirDensity(double density) { m_impl->m_weather.airDensity = std::max(0.8, density); }

void phys_Simulator::setFuelConsumptionEnabled(bool enabled) { m_impl->m_fuelConsumptionEnabled = enabled; }
bool phys_Simulator::isFuelConsumptionEnabled() const { return m_impl->m_fuelConsumptionEnabled; }
double phys_Simulator::getFuelKg() const { return m_impl->m_fuelKg; }
void phys_Simulator::setFuelKg(double kg) { m_impl->m_fuelKg = std::max(0.0, kg); }
double phys_Simulator::getFuelCapacity() const { return m_impl->m_fuelCapacity; }
void phys_Simulator::setFuelCapacity(double liters) { m_impl->m_fuelCapacity = liters; }
double phys_Simulator::getEffectiveMass() const { return m_impl->m_mass + m_impl->m_fuelKg; }

void phys_Simulator::loadVehicleParams(const QString& carPath) {
    m_impl->loadEngineFromIni(carPath + "/data/engine.ini");
    m_impl->loadTyresFromIni(carPath + "/data/tyres.ini");
    m_impl->loadDrivetrainFromIni(carPath + "/data/drivetrain.ini");
    m_impl->loadAeroFromIni(carPath + "/data/aero.ini");
    m_impl->loadSuspensionFromIni(carPath + "/data/suspensions.ini");
}

void phys_Simulator::loadEngineFromIni(const QString& engineIniPath) { m_impl->loadEngineFromIni(engineIniPath); }
void phys_Simulator::loadTyresFromIni(const QString& tyresIniPath) { m_impl->loadTyresFromIni(tyresIniPath); }
void phys_Simulator::loadDrivetrainFromIni(const QString& drivetrainIniPath) { m_impl->loadDrivetrainFromIni(drivetrainIniPath); }
void phys_Simulator::loadAeroFromIni(const QString& aeroIniPath) { m_impl->loadAeroFromIni(aeroIniPath); }
void phys_Simulator::loadSuspensionFromIni(const QString& suspensionIniPath) { m_impl->loadSuspensionFromIni(suspensionIniPath); }

WheelState phys_Simulator::wheelState(int wheel) const {
    return m_impl->m_wheels[wheel];
}

ValidationMetrics phys_Simulator::validateAgainstTelemetry(
    const QVector<double>& timestamps,
    const QVector<double>& refSpeed,
    const QVector<double>& refLateralG,
    const QVector<double>& refLongG,
    const QVector<double>& refRPM,
    const QVector<double>& refThrottle,
    const QVector<double>& refBrake,
    const QVector<double>& refSteering) const
{
    // This is a complex method - for now, delegate to a static function or implement in Impl
    // For brevity, returning empty metrics - full implementation would be in Impl
    ValidationMetrics metrics = {};
    metrics.nSamples = 0;
    return metrics;
}

// ============================================================================
// Impl method implementations
// ============================================================================

double phys_Simulator::Impl::calculateSlipAngle(int wheel, double steeringAngle, double speed, double yawRate) const {
    if (speed < 0.5) return 0.0;

    bool isFront = (wheel < 2);
    bool isLeft = (wheel == 0 || wheel == 2);

    double wheelX = isFront ? m_frontAxleDist : -m_rearAxleDist;
    double wheelY = isLeft ? (m_trackWidth / 2.0) : (-m_trackWidth / 2.0);

    double wheelSteer = isFront ? steeringAngle : 0.0;

    double vx = speed - yawRate * wheelY;
    double vy = yawRate * wheelX;

    double slipAngle = qAtan2(vy, vx) - wheelSteer;
    return qRadiansToDegrees(slipAngle);
}

double phys_Simulator::Impl::calculateSlipRatio(int wheel) const {
    const WheelState& w = m_wheels[wheel];
    double wheelSpeed = w.angularVelocity * m_wheelRadius;
    double vehicleSpeed = m_state.speed;

    if (vehicleSpeed < 0.5) return 0.0;

    return (wheelSpeed - vehicleSpeed) / std::max(0.1, vehicleSpeed);
}

void phys_Simulator::Impl::updateWeightTransfer(double longitudinalAccel, double lateralAccel) {
    double effectiveMass = m_mass + m_fuelKg;
    double FzTotal = effectiveMass * 9.81;

    double longTransfer = effectiveMass * longitudinalAccel * m_cgHeight / m_wheelBase;
    double latTransfer = effectiveMass * lateralAccel * m_cgHeight / m_trackWidth;

    double frontStatic = FzTotal * m_rearAxleDist / m_wheelBase;
    double rearStatic = FzTotal * m_frontAxleDist / m_wheelBase;

    double halfFrontStatic = frontStatic / 2.0;
    double halfRearStatic = rearStatic / 2.0;
    double halfLongTransfer = longTransfer / 2.0;
    double halfLatTransfer = latTransfer / 2.0;

    m_wheels[0].normalLoad = halfFrontStatic - halfLongTransfer + halfLatTransfer;
    m_wheels[1].normalLoad = halfFrontStatic - halfLongTransfer - halfLatTransfer;
    m_wheels[2].normalLoad = halfRearStatic + halfLongTransfer + halfLatTransfer;
    m_wheels[3].normalLoad = halfRearStatic + halfLongTransfer - halfLatTransfer;

    for (int i = 0; i < 4; ++i) {
        m_wheels[i].normalLoad = std::max(50.0, m_wheels[i].normalLoad);
    }
}

// ============================================================================
// Vehicle parameter loading from AC INI files
// ============================================================================

void phys_Simulator::Impl::loadEngineFromIni(const QString& engineIniPath) {
    QFile file(engineIniPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Could not open engine.ini:" << engineIniPath;
        return;
    }

    EngineModel::EngineConfig config;
    QTextStream stream(&file);
    bool inEngine = false;
    bool inGearbox = false;

    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();

        if (line.startsWith('[') && line.endsWith(']')) {
            QString section = line.mid(1, line.length() - 2).toUpper();
            inEngine = (section == "ENGINE");
            inGearbox = (section == "GEARBOX");
            continue;
        }

        if (line.contains('=')) {
            int eqPos = line.indexOf('=');
            QString key = line.left(eqPos).trimmed().toUpper();
            QString value = line.mid(eqPos + 1).trimmed();

            if (inEngine) {
                if (key == "PEAK_POWER") config.peakPower = value.toFloat();
                else if (key == "PEAK_TORQUE") config.peakTorque = value.toFloat();
                else if (key == "MAX_RPM") config.maxRPM = value.toFloat();
                else if (key == "IDLE_RPM") config.idleRPM = value.toFloat();
                else if (key == "REV_LIMITER") config.revLimiter = value.toFloat();
                else if (key == "COAST_TORQUE") config.coastTorque = value.toFloat();
            } else if (inGearbox) {
                if (key == "FINAL_DRIVE") config.finalDrive = value.toFloat();
            }
        }
    }

    file.close();
    m_engineModel.setConfig(config);

    m_enginePowerKw = config.peakPower;
    m_maxRpm = config.maxRPM;
    m_finalDriveRatio = config.finalDrive;

    QString lutPath = QFileInfo(engineIniPath).absolutePath() + "/power_lut.ini";
    if (QFile::exists(lutPath)) {
        QVector<EngineModel::TorquePoint> curve = EngineModel::loadPowerLut(lutPath);
        if (!curve.isEmpty()) {
            EngineModel::EngineConfig cfg = m_engineModel.getConfig();
            cfg.torqueCurve = curve;
            m_engineModel.setConfig(cfg);
        }
    }
}

void phys_Simulator::Impl::loadTyresFromIni(const QString& tyresIniPath) {
    QFile file(tyresIniPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Could not open tyres.ini:" << tyresIniPath;
        return;
    }

    QTextStream stream(&file);
    bool inFront = false;
    bool inRear = false;

    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();

        if (line.startsWith('[') && line.endsWith(']')) {
            QString section = line.mid(1, line.length() - 2).toUpper();
            inFront = section.contains("FRONT");
            inRear = section.contains("REAR");
            continue;
        }

        if (line.contains('=')) {
            int eqPos = line.indexOf('=');
            QString key = line.left(eqPos).trimmed().toUpper();
            QString value = line.mid(eqPos + 1).trimmed();

            if (key == "RADIUS") m_wheelRadius = value.toFloat();
            else if (key == "WIDTH") { }
            else if (key == "PRESSURE") {
                float pressure = value.toFloat();
                m_tirePressure[0] = pressure;
                m_tirePressure[1] = pressure;
                m_tirePressure[2] = pressure;
                m_tirePressure[3] = pressure;
            }
        }
    }

    file.close();
}

void phys_Simulator::Impl::loadDrivetrainFromIni(const QString& drivetrainIniPath) {
    QFile file(drivetrainIniPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Could not open drivetrain.ini:" << drivetrainIniPath;
        return;
    }

    QTextStream stream(&file);
    bool inGearbox = false;
    bool inDifferential = false;
    QVector<double> gearRatios;

    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();

        if (line.startsWith('[') && line.endsWith(']')) {
            QString section = line.mid(1, line.length() - 2).toUpper();
            inGearbox = (section == "GEARBOX");
            inDifferential = (section == "DIFFERENTIAL");
            continue;
        }

        if (line.contains('=')) {
            int eqPos = line.indexOf('=');
            QString key = line.left(eqPos).trimmed().toUpper();
            QString value = line.mid(eqPos + 1).trimmed();

            if (inGearbox) {
                if (key == "FINAL_DRIVE") {
                    m_finalDriveRatio = value.toFloat();
                } else if (key.startsWith("GEAR_") && key != "GEAR_COUNT") {
                    bool ok;
                    double ratio = value.toDouble(&ok);
                    if (ok && ratio > 0) {
                        gearRatios.append(ratio);
                    }
                }
            } else if (inDifferential) {
                DifferentialModel::DiffConfig diffConfig = m_diffModel.getConfig();
                if (key == "TYPE") {
                    if (value.contains("OPEN")) diffConfig.type = DifferentialModel::DiffType::Open;
                    else if (value.contains("CLUTCH") || value.contains("LSD"))
                        diffConfig.type = DifferentialModel::DiffType::LSD_Cls;
                    else if (value.contains("VISCOUS"))
                        diffConfig.type = DifferentialModel::DiffType::LSD_Viscous;
                    else if (value.contains("GEARED") || value.contains("TORSEN"))
                        diffConfig.type = DifferentialModel::DiffType::LSD_Geared;
                    else if (value.contains("LOCKED"))
                        diffConfig.type = DifferentialModel::DiffType::Locked;
                    else if (value.contains("ACTIVE"))
                        diffConfig.type = DifferentialModel::DiffType::Active;
                } else if (key == "PRELOAD") diffConfig.preload = value.toFloat();
                else if (key == "COAST_POWER") diffConfig.coastPower = value.toFloat();
                else if (key == "DRIVE_POWER") diffConfig.drivePower = value.toFloat();
                else if (key == "MAX_LOCK") diffConfig.maxLock = value.toFloat();
                else if (key == "RAMP_ANGLE") diffConfig.rampAngle = value.toFloat();
                m_diffModel.setConfig(diffConfig);
            }
        }
    }

    file.close();

    if (!gearRatios.isEmpty()) {
        m_gearRatios = gearRatios;
    }
}

void phys_Simulator::Impl::loadAeroFromIni(const QString& aeroIniPath) {
    AeroModel::AeroConfig config = AeroModel::loadFromIni(aeroIniPath);
    m_aeroModel = AeroModel();
    for (const AeroModel::Wing& wing : config.wings) {
        m_aeroModel.addWing(wing);
    }
    m_cd = config.dragCoefficient;
    m_frontalArea = config.frontalArea;
}

void phys_Simulator::Impl::loadSuspensionFromIni(const QString& suspensionIniPath) {
    m_suspensionModel.loadFromIni(suspensionIniPath);
}

// ============================================================================
// Per-wheel forces
// ============================================================================

void phys_Simulator::Impl::updatePerWheelForces(double dt) {
    for (int i = 0; i < 4; ++i) {
        WheelState& w = m_wheels[i];

        w.slipAngle = calculateSlipAngle(i, qDegreesToRadians(m_state.steeringAngle),
                                          m_state.speed, m_yawRate);

        w.slipRatio = calculateSlipRatio(i);

        w.camber = m_suspensionModel.getWheel(i).getCamber();

        double tempEffect = m_pacejkaModel.calculateTemperatureEffect(w.temperature);
        double wearEffect = m_pacejkaModel.calculateWearEffect(w.wear);
        double pressureEffect = m_pacejkaModel.calculatePressureEffect(w.pressure);

        double weatherEffect = 1.0 - m_trackGripReduction;

        if (m_aquaplaningRisk > 0.5) {
            double aquaFactor = (m_aquaplaningRisk - 0.5) / 0.45;
            weatherEffect *= (1.0 - aquaFactor * 0.9);
        }

        double suspensionEffect = 1.0 - m_damage.suspensionDamage[i] * 0.5;

        double grainingEffect = 1.0 - m_tireGraining[i] * 0.3;
        double blisteringEffect = 1.0 - m_tireBlistering[i] * 0.5;

        double effectiveGrip = tempEffect * wearEffect * pressureEffect * weatherEffect
                               * suspensionEffect * grainingEffect * blisteringEffect;

        PacejkaTireModel::TireForces forces = m_pacejkaModel.calculateCombinedSlip(
            w.slipAngle, w.slipRatio, w.normalLoad, w.camber);

        w.lateralForce = forces.lateralForce * effectiveGrip;
        w.longitudinalForce = forces.longitudinalForce * effectiveGrip;

        double wheelInertia = 1.5;
        double netTorque = w.driveTorque - w.brakeTorque
                           - w.longitudinalForce * m_wheelRadius;
        double angularAccel = netTorque / (wheelInertia + m_mass * m_wheelRadius * m_wheelRadius * 0.01);
        w.angularVelocity += angularAccel * dt / m_wheelRadius;
        w.angularVelocity = std::max(0.0, w.angularVelocity);

        if (i < 2) {
            w.angularVelocity = m_state.speed / m_wheelRadius;
        }
    }
}

// ============================================================================
// Thermal tire model update
// ============================================================================

void phys_Simulator::Impl::updateTireModel(double dt) {
    const double v = m_state.speed;

    for (int i = 0; i < 4; ++i) {
        WheelState& w = m_wheels[i];
        double Fz = w.normalLoad;
        double slipAngleRad = qDegreesToRadians(std::abs(w.slipAngle));
        double slipRatio = std::abs(w.slipRatio);
        double totalSlip = slipAngleRad * 2.0 + slipRatio;

        double frictionPower = 0.5 * m_tireModel.peakLateralMu * Fz * totalSlip * std::max(v, 1.0);

        double coolingCoeff = 8.0 + 0.5 * v;
        double heatToSurface = frictionPower * dt * 0.03;

        double Tsurf = m_tireTempSurface[i];
        double Tcar  = m_tireTempCarcass[i];
        double Tcore = m_tireTempCore[i];

        double surfLoss   = coolingCoeff * (Tsurf - m_ambientTemp) * dt;
        double condSC     = 120.0 * (Tsurf - Tcar)  * dt;
        double condCC     = 80.0  * (Tcar  - Tcore) * dt;

        double dTsurf = heatToSurface - surfLoss - condSC;
        double dTcar  = condSC - condCC;
        double dTcore = condCC;

        m_tireTempSurface[i] = std::clamp(Tsurf + dTsurf, 10.0, 130.0);
        m_tireTempCarcass[i] = std::clamp(Tcar  + dTcar,  10.0, 120.0);
        m_tireTempCore[i]    = std::clamp(Tcore + dTcore, 10.0, 110.0);

        w.temperature = m_tireTempSurface[i];

        double Tkelvin = (m_tireTempCarcass[i] + 273.15) / (30.0 + 273.15);
        m_tirePressure[i] = std::clamp(2.4 * Tkelvin, 1.5, 3.5);
        w.pressure = m_tirePressure[i];

        double wearRate = 1e-8 * m_tireModel.peakLateralMu * Fz * totalSlip * std::max(v, 1.0) / (Tcar + 50.0);
        m_tireWear[i] = std::min(1.0, m_tireWear[i] + wearRate * dt);
        w.wear = m_tireWear[i];

        double grainingRate = 5e-6 * totalSlip * (1.0 - Tcar / 120.0) * std::max(v, 1.0);
        m_tireGraining[i] = std::min(1.0, m_tireGraining[i] + grainingRate * dt);

        if (Tcar > 100.0) {
            double blisterRisk = (Tcar - 100.0) / 30.0;
            double blisterRate = 2e-5 * blisterRisk * totalSlip * std::max(v, 1.0);
            m_tireBlistering[i] = std::min(1.0, m_tireBlistering[i] + blisterRate * dt);
        }
    }
}

// ============================================================================
// ERS/Hybrid system update
// ============================================================================

void phys_Simulator::Impl::updateErsAndDrs(double dt) {
    if (m_hybridSystem.isEnabled()) {
        EngineModel::EngineConfig engConfig = m_engineModel.getConfig();
        double engineRpm = m_state.rpm;
        double totalGearRatio = m_gearRatios[m_currentGear - 1] * m_finalDriveRatio;

        float turboBoost = engConfig.turbo.enabled
            ? m_engineModel.calculateTurboBoost(engineRpm, m_throttle) : 0.0f;

        m_hybridSystem.update(dt, engineRpm, m_throttle, m_brake, m_state.speed,
                               totalGearRatio, 0.0, turboBoost, 0.0f);

        m_ersDeployTorque = m_hybridSystem.getDeployTorque();
        m_ersRegenTorque = m_hybridSystem.getRegenTorque();
    } else {
        m_ersDeployTorque = 0.0;
        m_ersRegenTorque = 0.0;
    }

    if (m_drsEnabled) {
        if (m_drsAutoActivate) {
            bool inDrsZone = true;
            if (m_drsZoneEnd > m_drsZoneStart) {
                double lapDist = m_state.currentLapDistance;
                inDrsZone = (lapDist >= m_drsZoneStart && lapDist <= m_drsZoneEnd);
            }

            bool aboveSpeedThreshold = m_state.speed * 3.6 > m_drsSpeedThreshold;
            bool offBrake = m_brake < 0.1;
            bool onThrottle = m_throttle > 0.1;

            m_drsActive = inDrsZone && aboveSpeedThreshold && offBrake && onThrottle
                          && !m_damage.isEliminated;
        }
    } else {
        m_drsActive = false;
    }
}

// ============================================================================
// Damage Model
// ============================================================================

void phys_Simulator::Impl::applyCollisionDamage(double impactForce) {
    if (!m_damageEnabled || impactForce < 1000.0) return;

    m_damage.collisionCount++;
    m_damage.accumulatedImpact += impactForce;

    double normalizedForce = std::min(impactForce / 50000.0, 1.0);

    m_damage.bodyDamage = std::min(1.0, m_damage.bodyDamage + normalizedForce * 0.3);

    if (impactForce >= 5000.0) {
        m_damage.aeroDamage = std::min(1.0, m_damage.aeroDamage + normalizedForce * 0.7);
    }

    for (int i = 0; i < 4; ++i) {
        if (impactForce > 3000.0) {
            m_damage.suspensionDamage[i] = std::min(1.0,
                m_damage.suspensionDamage[i] + normalizedForce * 0.4 * (0.5 + (rand() % 100) / 200.0));
        }
    }

    if (impactForce > 15000.0) {
        m_damage.engineDamage = std::min(1.0, m_damage.engineDamage + normalizedForce * 0.6);
    }

    if (impactForce > 10000.0) {
        m_damage.gearboxDamage = std::min(1.0, m_damage.gearboxDamage + normalizedForce * 0.4);
    }

    if (m_damage.bodyDamage >= 1.0 || m_damage.engineDamage >= 1.0) {
        m_damage.isEliminated = true;
    }
}

void phys_Simulator::Impl::updateDamageModel(double dt) {
    if (!m_damageEnabled) return;

    EngineModel::EngineConfig engConfig = m_engineModel.getConfig();
    if (m_state.rpm > engConfig.revLimiter * 1.15) {
        double overRevDamage = (m_state.rpm - engConfig.revLimiter * 1.15) / 1000.0 * dt;
        m_damage.engineDamage = std::min(1.0, m_damage.engineDamage + overRevDamage);
    }

    if (m_damage.gearboxDamage > 0) {
        if (rand() % 1000 < static_cast<int>(m_damage.gearboxDamage * 10)) {
            m_state.rpm *= 0.7;
        }
    }

    if (m_damage.aeroDamage > 0) {
        m_drsBaseCd = m_cd * (1.0 + m_damage.aeroDamage * 0.3);
    }

    double totalSuspDamage = 0.0;
    for (int i = 0; i < 4; ++i) {
        totalSuspDamage += m_damage.suspensionDamage[i];
    }

    if (m_damage.isEliminated) {
        m_state.speed *= 0.99;
        m_state.rpm = std::max(1000.0, m_state.rpm - 500.0 * dt);
    }
}

// ============================================================================
// Weather-Dependent Physics
// ============================================================================

void phys_Simulator::Impl::updateWeatherEffects(double dt) {
    m_trackGripReduction = m_weather.trackWetness * 0.4;

    if (m_weather.trackWetness > 0.3 && m_state.speed > 30.0) {
        double avgWear = 0.0;
        for (int i = 0; i < 4; ++i) avgWear += m_tireWear[i];
        avgWear /= 4.0;

        double speedFactor = (m_state.speed - 30.0) / 60.0;
        double wetnessFactor = (m_weather.trackWetness - 0.3) / 0.7;

        m_aquaplaningRisk = std::clamp(speedFactor * wetnessFactor * (1.0 + avgWear), 0.0, 0.95);
    } else {
        m_aquaplaningRisk = 0.0;
    }

    m_weather.trackTemp += (m_weather.ambientTemp + 10.0 - m_weather.trackTemp) * dt * 0.01;

    m_weather.airDensity = 1.225 * (293.15 / (m_weather.ambientTemp + 273.15));
}

// ============================================================================
// Fuel Weight Dynamics
// ============================================================================

void phys_Simulator::Impl::updateFuelWeight(double dt) {
    if (!m_fuelConsumptionEnabled) return;

    EngineModel::EngineConfig engConfig = m_engineModel.getConfig();
    float enginePower = m_engineModel.calculatePower(m_state.rpm);
    float fuelFlow = m_engineModel.calculateFuelFlow(enginePower);

    double consumedKg = fuelFlow * enginePower * dt / 3600.0 * engConfig.fuelDensity;
    m_fuelKg = std::max(0.0, m_fuelKg - consumedKg);
}

// ============================================================================
// AWD center differential
// ============================================================================

void phys_Simulator::Impl::setFrontRearTorqueSplit(double frontRatio) {
    m_frontTorqueSplit = std::clamp(frontRatio, 0.0, 1.0);
    if (frontRatio > 0.001 && frontRatio < 0.999) {
        m_driveLayout = DriveLayout::AWD;
        m_centerDiffPower = frontRatio;
    } else if (frontRatio < 0.001) {
        m_driveLayout = DriveLayout::RWD;
    } else {
        m_driveLayout = DriveLayout::FWD;
    }
}

} // namespace ks