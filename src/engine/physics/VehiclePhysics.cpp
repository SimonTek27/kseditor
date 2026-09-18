#include "VehiclePhysics.h"
#include "PhysicsEngine.h"
#include "VehiclePhysicsModels.h"
#include <cmath>
#include <algorithm>
#include <QDebug>
#include <QtMath>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QDate>

// AC-specific physics model implementations
#include "sdk/kseditor/plugins/simulators/kunos/assettocorsa/physics/PacejkaTireModel.h"
#include "sdk/kseditor/plugins/simulators/kunos/assettocorsa/physics/EngineModel.h"
#include "sdk/kseditor/plugins/simulators/kunos/assettocorsa/physics/AeroModel.h"
#include "sdk/kseditor/plugins/simulators/kunos/assettocorsa/physics/DifferentialModel.h"
#include "sdk/kseditor/plugins/simulators/kunos/assettocorsa/physics/SuspensionModel.h"
#include "sdk/kseditor/plugins/simulators/kunos/assettocorsa/physics/BrakeThermalModel.h"
#include "sdk/kseditor/plugins/simulators/kunos/assettocorsa/physics/HybridSystem.h"

namespace ks {
namespace physics {

VehicleSimulator* VehicleSimulator::s_instance = nullptr;

VehicleSimulator::VehicleSimulator(QObject* parent)
    : IVehicleSimulator(parent)
{
    m_state = {};

    // Initialize AC models with defaults so they're always available
    m_ersDrs.setHybridSystem(std::make_unique<HybridSystem>());
    m_brakeModel = std::make_unique<BrakeModelManager>();

    // Initialize ground effect state
    m_groundEffectState.rideHeightFront = 0.05f;
    m_groundEffectState.rideHeightRear = 0.07f;
    m_groundEffectState.downforceGain = 1.0f;

    // Initialize axle pair
    m_axlePair.wheelBase = m_wheelBase;
    m_axlePair.frontCorneringStiffness = 80000.0f;
    m_axlePair.rearCorneringStiffness = 80000.0f;

    // Initialize rFactor2-style damage, tire wear, and brake wear systems
    DamageConfig damageConfig;
    damageConfig.enabled = m_damageEnabled;
    m_damageSystem.setConfig(damageConfig);

    // Initialize tire wear systems with default compound
    for (int i = 0; i < 4; ++i) {
        m_tireWearSystems[i].setCompound(TireCompound::Medium);
    }

    // Initialize brake wear system
    BrakeThermalConfig thermalConfig;
    m_brakeWearSystem.setThermalConfig(thermalConfig);
}

VehicleSimulator::~VehicleSimulator() {
    stopSimulation();
    s_instance = nullptr;
}

VehicleSimulator* VehicleSimulator::instance() {
    if (!s_instance) {
        s_instance = new VehicleSimulator();
    }
    return s_instance;
}

void VehicleSimulator::startSimulation() {
    if (m_running) return;
    m_running = true;
    m_simTimer.start();
    m_lastUpdateTime = 0.0;
    emit simulationStarted();
}

void VehicleSimulator::stopSimulation() {
    if (!m_running) return;
    m_running = false;
    emit simulationStopped();
}

void VehicleSimulator::reset() {
    m_state = {};
    m_throttle = 0.0;
    m_brake = 0.0;
    m_steering = 0.0;
    m_currentGear = 1;
    m_fuelKg = 80.0;
    m_yawRate = 0.0;
    m_lateralAccel = 0.0;
    for (int i = 0; i < 4; ++i) {
        m_wheels[i] = WheelState();
        m_tireGraining[i] = 0.0;
        m_tireBlistering[i] = 0.0;
        m_tireTempSurface[i] = 30.0;
        m_tireTempCarcass[i] = 35.0;
        m_tireTempCore[i] = 40.0;
        m_tirePressure[i] = 2.4;
        m_tireWear[i] = 0.0;
    }
    m_ambientTemp = 26.0;
    m_ersDrs.reset();
    m_damage = DamageState();
    m_damageSystem.reset();
    for (int i = 0; i < 4; ++i) {
        m_tireWearSystems[i].reset();
    }
    m_brakeWearSystem.reset();
    m_aquaplaningRisk = 0.0;
    m_trackGripReduction = 0.0;
    m_lapTimer.reset();
    m_lapTimeHistory.clear();
    m_lastUpdateTime = 0.0;
}

void VehicleSimulator::setThrottle(double value) {
    m_throttle = std::clamp(value, 0.0, 1.0);
}

void VehicleSimulator::setBrake(double value) {
    m_brake = std::clamp(value, 0.0, 1.0);
}

void VehicleSimulator::setSteering(double value) {
    m_steering = std::clamp(value, -1.0, 1.0);
}

void VehicleSimulator::setTireModel(const TireSlipCurve& curve) {
    m_tireModel = curve;
}

LapTimeEstimate VehicleSimulator::estimateLapTime() const {
    return m_lapTimer.estimateLapTime(m_lapTimeHistory, m_trackLength, 0.0);
}

float VehicleSimulator::getBrakeDiscTemp(int wheel) const {
    Q_UNUSED(wheel);
    if (m_brakeModel) return m_brakeModel->getAverageDiscTemp();
    return 300.0f;
}

float VehicleSimulator::getBrakePadTemp(int wheel) const {
    Q_UNUSED(wheel);
    if (m_brakeModel) return m_brakeModel->getAveragePadTemp();
    return 200.0f;
}

float VehicleSimulator::getBrakeFade(int wheel) const {
    Q_UNUSED(wheel);
    if (m_brakeModel) return m_brakeModel->getBrakeFade();
    return 0.0f;
}

void VehicleSimulator::setFrontRearTorqueSplit(double frontRatio) {
    m_frontTorqueSplit = std::clamp(frontRatio, 0.0, 1.0);
    if (m_driveLayout == DriveLayout::AWD) {
        m_centerDiffPower = m_frontTorqueSplit;
    }
}

void VehicleSimulator::applyCollisionDamage(double impactForce) {
    if (!m_damageEnabled) return;

    // Use the new DamageSystem for detailed damage calculation
    QVector3D contactPoint(0.0f, 0.5f, m_frontAxleDist); // Default front impact
    QVector3D direction(0.0f, 0.0f, -1.0f);
    m_damageSystem.applyImpactDamage(
        static_cast<float>(impactForce),
        direction,
        contactPoint,
        DamageType::BodyPanel
    );

    // Sync legacy damage state
    m_damage.accumulatedImpact += impactForce;
    m_damage.collisionCount = m_damageSystem.totalCollisions();
    m_damage.bodyDamage = m_damageSystem.structuralDamage();
    m_damage.aeroDamage = 1.0f - m_damageSystem.downforceMultiplier();
    m_damage.engineDamage = 1.0f - m_damageSystem.powerMultiplier();

    // Sync extended damage fields
    m_damage.engineHealth = m_damageSystem.engineDamage().health;
    m_damage.enginePowerLoss = 1.0f - m_damageSystem.powerMultiplier();
    m_damage.frontWingDamage = m_damageSystem.aeroDamage().frontWingDamage;
    m_damage.rearWingDamage = m_damageSystem.aeroDamage().rearWingDamage;
    m_damage.diffuserDamage = m_damageSystem.aeroDamage().diffuserDamage;
    m_damage.floorDamage = m_damageSystem.aeroDamage().floorDamage;

    // Physics impact multipliers
    m_damage.powerMultiplier = m_damageSystem.powerMultiplier();
    m_damage.handlingMultiplier = m_damageSystem.handlingMultiplier();
    m_damage.brakingMultiplier = m_damageSystem.brakingMultiplier();
    m_damage.downforceMultiplier = m_damageSystem.downforceMultiplier();
    m_damage.dragMultiplier = m_damageSystem.dragMultiplier();

    if (m_damage.bodyDamage >= 1.0) {
        m_damage.isEliminated = true;
    }
}

void VehicleSimulator::resetDamage() {
    m_damage = DamageState();
    m_damageSystem.reset();
}

void VehicleSimulator::setWeatherState(const WeatherState& weather) {
    m_weatherSim.setWeatherState(weather);
    m_ambientTemp = weather.ambientTemp;
}

void VehicleSimulator::setTrackWetness(double wetness) {
    m_weatherSim.setTrackWetness(wetness);
}

void VehicleSimulator::setRainIntensity(double mmh) {
    m_weatherSim.setRainIntensity(mmh);
}

void VehicleSimulator::loadVehicleParams(const QString& carPath) {
    QDir carDir(carPath);
    if (!carDir.exists()) return;

    QString engineIni = carDir.filePath("data/engine.ini");
    QString tyresIni = carDir.filePath("data/tyres.ini");
    QString drivetrainIni = carDir.filePath("data/drivetrain.ini");
    QString aeroIni = carDir.filePath("data/aero.ini");
    QString suspensionIni = carDir.filePath("data/suspensions.ini");

    if (QFile::exists(engineIni)) loadEngineFromIni(engineIni);
    if (QFile::exists(tyresIni)) loadTyresFromIni(tyresIni);
    if (QFile::exists(drivetrainIni)) loadDrivetrainFromIni(drivetrainIni);
    if (QFile::exists(aeroIni)) loadAeroFromIni(aeroIni);
    if (QFile::exists(suspensionIni)) loadSuspensionFromIni(suspensionIni);
}

void VehicleSimulator::loadEngineFromIni(const QString& engineIniPath) {
    QFile file(engineIniPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    QMap<QString, QString> params;
    QStringList lines = content.split('\n');
    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith(';') || trimmed.startsWith('[')) continue;
        int eqPos = trimmed.indexOf('=');
        if (eqPos > 0) {
            QString key = trimmed.left(eqPos).trimmed();
            QString value = trimmed.mid(eqPos + 1).trimmed();
            params[key] = value;
        }
    }

    if (params.contains("MAX_POWER")) m_enginePowerKw = params["MAX_POWER"].toFloat();
    if (params.contains("MAX_TORQUE")) { /* store for engine model */ }
    if (params.contains("MAX_RPM")) m_maxRpm = params["MAX_RPM"].toFloat();
    if (params.contains("IDLE_RPM")) { /* store */ }
    if (params.contains("LIMITER")) { /* store */ }
    if (params.contains("INERTIA")) { /* store */ }
    if (params.contains("FUEL_CONSUMPTION")) { /* store */ }
    if (params.contains("COAST")) { /* store */ }
    if (params.contains("COAST_RPM")) { /* store */ }

    // Load AC engine model if available
    if (!m_engineModel) {
        m_engineModel = std::make_unique<EngineModel>();
    }
    m_engineModel->loadFromIni(engineIniPath);
    m_acModelsLoaded = true;
}

void VehicleSimulator::loadTyresFromIni(const QString& tyresIniPath) {
    QFile file(tyresIniPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    QMap<QString, QString> params;
    QStringList lines = content.split('\n');
    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith(';') || trimmed.startsWith('[')) continue;
        int eqPos = trimmed.indexOf('=');
        if (eqPos > 0) {
            QString key = trimmed.left(eqPos).trimmed();
            QString value = trimmed.mid(eqPos + 1).trimmed();
            params[key] = value;
        }
    }

    if (params.contains("LATERAL_STIFFNESS")) { /* update tire model */ }
    if (params.contains("LONGITUDINAL_STIFFNESS")) { /* update tire model */ }

    // Load AC tire model if available
    if (!m_pacejkaModel) {
        m_pacejkaModel = std::make_unique<PacejkaTireModel>();
    }
    m_pacejkaModel->loadFromIni(tyresIniPath);
}

void VehicleSimulator::loadDrivetrainFromIni(const QString& drivetrainIniPath) {
    QFile file(drivetrainIniPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    QMap<QString, QString> params;
    QStringList lines = content.split('\n');
    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith(';') || trimmed.startsWith('[')) continue;
        int eqPos = trimmed.indexOf('=');
        if (eqPos > 0) {
            QString key = trimmed.left(eqPos).trimmed();
            QString value = trimmed.mid(eqPos + 1).trimmed();
            params[key] = value;
        }
    }

    if (params.contains("GEAR_1")) m_gearRatios[0] = params["GEAR_1"].toDouble();
    if (params.contains("GEAR_2")) m_gearRatios[1] = params["GEAR_2"].toDouble();
    if (params.contains("GEAR_3")) m_gearRatios[2] = params["GEAR_3"].toDouble();
    if (params.contains("GEAR_4")) m_gearRatios[3] = params["GEAR_4"].toDouble();
    if (params.contains("GEAR_5")) m_gearRatios[4] = params["GEAR_5"].toDouble();
    if (params.contains("GEAR_6")) m_gearRatios[5] = params["GEAR_6"].toDouble();
    if (params.contains("FINAL_DRIVE")) m_finalDriveRatio = params["FINAL_DRIVE"].toDouble();

    // Load AC differential model if available
    if (!m_differentialModel) {
        m_differentialModel = std::make_unique<DifferentialModel>();
    }
    m_differentialModel->loadFromIni(drivetrainIniPath);
}

void VehicleSimulator::loadAeroFromIni(const QString& aeroIniPath) {
    // Load AC aero model if available
    if (!m_aeroModel) {
        m_aeroModel = std::make_unique<AeroModel>();
    }
    m_aeroModel->loadFromIniFile(aeroIniPath);
}

void VehicleSimulator::loadSuspensionFromIni(const QString& suspensionIniPath) {
    // Load AC suspension model if available
    if (!m_suspensionModel) {
        m_suspensionModel = std::make_unique<SuspensionModel>();
    }
    m_suspensionModel->loadFromIni(suspensionIniPath);
}

ValidationMetrics VehicleSimulator::validateAgainstTelemetry(
    const QVector<double>& timestamps,
    const QVector<double>& refSpeed,
    const QVector<double>& refLateralG,
    const QVector<double>& refLongG,
    const QVector<double>& refRPM,
    const QVector<double>& refThrottle,
    const QVector<double>& refBrake,
    const QVector<double>& refSteering) const
{
    ValidationMetrics metrics = {};
    int n = timestamps.size();
    if (n < 2) return metrics;

    metrics.nSamples = n;
    double speedSSE = 0, latGSSE = 0, longGSSE = 0, rpmSSE = 0;
    double speedMaxErr = 0, latGMaxErr = 0, longGMaxErr = 0, rpmMaxErr = 0;

    for (int i = 0; i < n; ++i) {
        double speedErr = 0;
        double latGErr = 0;
        double longGErr = 0;
        double rpmErr = 0;

        speedSSE += speedErr * speedErr;
        latGSSE += latGErr * latGErr;
        longGSSE += longGErr * longGErr;
        rpmSSE += rpmErr * rpmErr;

        speedMaxErr = std::max(speedMaxErr, std::abs(speedErr));
        latGMaxErr = std::max(latGMaxErr, std::abs(latGErr));
        longGMaxErr = std::max(longGMaxErr, std::abs(longGErr));
        rpmMaxErr = std::max(rpmMaxErr, std::abs(rpmErr));
    }

    metrics.speedRMSE = std::sqrt(speedSSE / n);
    metrics.lateralGRMSE = std::sqrt(latGSSE / n);
    metrics.longitudinalGRMSE = std::sqrt(longGSSE / n);
    metrics.rpmRMSE = std::sqrt(rpmSSE / n);

    metrics.speedMaxError = speedMaxErr;
    metrics.lateralGMaxError = latGMaxErr;
    metrics.longitudinalGMaxError = longGMaxErr;
    metrics.rpmMaxError = rpmMaxErr;

    return metrics;
}

void VehicleSimulator::updatePhysics(double dt) {
    if (!m_running) return;

    // Safety limits for timestep
    dt = std::clamp(dt, static_cast<double>(Constants::MIN_TIMESTEP), 
                    static_cast<double>(Constants::MAX_TIMESTEP));

    // Step the general physics engine first
    m_physicsWorld.stepSimulation(dt);

    // Update weather
    m_weatherSim.update(dt);
    m_aquaplaningRisk = m_weatherSim.aquaplaningRisk();
    m_trackGripReduction = m_weatherSim.trackGripReduction();

    // Update engine using specialized simulator
    m_engineSim.setThrottle(m_throttle);
    m_engineSim.update(dt, m_state.speed);
    
    // Sync engine state back to legacy variables
    m_state.rpm = m_engineSim.engineRpm();
    m_currentGear = m_engineSim.currentGear();
    m_fuelKg = m_engineSim.fuelKg();

    // Update driver simulation
    m_driverSim.update(dt, m_state.speed, m_state.acceleration.y(), 
                      0.0, 0.0);  // Simplified

    // Update chassis dynamics
    std::array<double, 4> tireForces = {
        m_wheels[0].lateralForce, m_wheels[1].lateralForce,
        m_wheels[2].lateralForce, m_wheels[3].lateralForce
    };
    m_chassisSim.update(dt, m_throttle, m_brake, m_steering, tireForces);
    
    // Sync chassis state
    m_yawRate = m_chassisSim.yawRate();
    m_lateralAccel = m_chassisSim.lateralAccel();

    // Update aero using specialized simulator
    m_aeroSim.update(dt, m_state.speed, 
                    m_groundEffectState.rideHeightFront,
                    m_groundEffectState.rideHeightRear);

    // Tire forces
    updatePerWheelForces(dt);
    
    // Update tires using specialized simulator
    m_tireSim.update(dt, m_state.speed, m_weatherSim.weatherState());

    // Integration - use physics engine positions but override with vehicle-specific calculations
    double totalDriveForce = 0.0;
    for (int i = 0; i < 4; ++i) {
        totalDriveForce += m_wheels[i].longitudinalForce;
    }
    double totalDragForce = m_aeroSim.dragForce();
    double totalBrakeForce = 0.0;
    for (int i = 0; i < 4; ++i) {
        totalBrakeForce += m_wheels[i].brakeTorque / m_wheelRadius;
    }

    double netForce = totalDriveForce - totalDragForce - totalBrakeForce;
    double acceleration = netForce / getEffectiveMass();

    m_state.acceleration.setX(acceleration);
    m_state.speed += acceleration * dt;
    m_state.speed = std::max(0.0f, static_cast<float>(m_state.speed));
    m_state.position.setX(m_state.position.x() + m_state.speed * dt);
    m_state.rpm = m_state.rpm;
    m_state.heading += m_yawRate * dt;
    m_state.lapTime += dt;
    m_state.currentLapDistance += m_state.speed * dt;

    m_lapTimer.update(dt, m_state.speed, m_state.currentLapDistance);

    // Update tire temperatures
    updateTireModel(dt);

    // ERS/DRS
    updateErsAndDrs(dt);

    // Damage
    updateDamageModel(dt);

    // Weather
    m_weatherSim.update(dt);
    
    // Update weather effects on vehicle
    m_aquaplaningRisk = m_weatherSim.aquaplaningRisk();
    m_trackGripReduction = m_weatherSim.trackGripReduction();

    // Fuel
    updateFuelWeight(dt);

    emit stateUpdated(m_state);
}

void VehicleSimulator::updateTireModel(double dt) {
    for (int i = 0; i < 4; ++i) {
        double slipAngle = qDegreesToRadians(m_wheels[i].slipAngle);
        double slipRatio = m_wheels[i].slipRatio;

        m_tireTransientModel.update(
            dt,
            m_state.speed,
            m_wheels[i].angularVelocity,
            m_wheelRadius,
            slipAngle,
            slipRatio,
            m_tireTempSurface[i],
            m_wheels[i].normalLoad
        );

        TireTransientState transientState = m_tireTransientModel.getState();
        m_tireTempSurface[i] += (transientState.tireWarmUpFactor - 0.5) * dt * 2.0;
        m_tireTempSurface[i] = std::clamp(m_tireTempSurface[i], 20.0, 120.0);
    }
}

void VehicleSimulator::updateWeightTransfer(double longitudinalAccel, double lateralAccel) {
    m_weightTransferResult = m_weightTransferModel.calculate(
        m_mass, m_wheelBase, m_trackWidth,
        m_frontAxleDist, m_rearAxleDist,
        m_cgHeight, lateralAccel, longitudinalAccel,
        m_suspensionGeometry,
        m_rollStiffness * 0.6,
        m_rollStiffness * 0.4,
        m_rollStiffness
    );

    m_wheels[0].normalLoad = m_weightTransferResult.frontLeftLoad;
    m_wheels[1].normalLoad = m_weightTransferResult.frontRightLoad;
    m_wheels[2].normalLoad = m_weightTransferResult.rearLeftLoad;
    m_wheels[3].normalLoad = m_weightTransferResult.rearRightLoad;

    for (int i = 0; i < 4; ++i) {
        m_wheels[i].normalLoad = std::max(50.0, static_cast<double>(m_wheels[i].normalLoad));
    }
}

void VehicleSimulator::updatePerWheelForces(double dt) {
    Q_UNUSED(dt);

    // Calculate steering angle in radians
    double steeringAngleRad = qDegreesToRadians(m_steering * m_steerLock / m_steerRatio);

    for (int i = 0; i < 4; ++i) {
        // Calculate slip angle using proper kinematics
        double slipAngleDeg = calculateSlipAngle(i, steeringAngleRad, m_state.speed, m_yawRate);
        double slipAngleRad = qDegreesToRadians(slipAngleDeg);
        
        double slipRatio = calculateSlipRatio(i);

        // Apply weather effects to friction
        double friction = 1.0 * (1.0 - m_trackGripReduction);
        
        // Apply tire wear effects
        if (m_tireWear[i] > 0.0) {
            friction *= (1.0 - m_tireWear[i] * 0.3); // Up to 30% grip loss at full wear
        }
        
        // Apply temperature effects
        double optimalTemp = 80.0;
        double tempDeviation = std::abs(m_tireTempSurface[i] - optimalTemp);
        if (tempDeviation > 20.0) {
            friction *= (1.0 - (tempDeviation - 20.0) * 0.005); // Grip loss at extreme temps
        }

        double latForce, longForce;
        if (m_pacejkaModel) {
            PacejkaTireModel::TireState state;
            state.slipAngle = slipAngleRad;
            state.slipRatio = slipRatio;
            state.normalForce = m_wheels[i].normalLoad;
            state.frictionCoefficient = friction;
            state.tireTemp = m_tireTempSurface[i];
            state.tirePressure = m_tirePressure[i] * 14.5038; // bar to PSI
            latForce = m_pacejkaModel->calculateLateralForce(state);
            longForce = m_pacejkaModel->calculateLongitudinalForce(state);
        } else {
            latForce = m_tireModelImpl.calculateLateralForce(slipAngleDeg, m_wheels[i].normalLoad, friction);
            longForce = m_tireModelImpl.calculateLongitudinalForce(slipRatio, m_wheels[i].normalLoad, friction);
        }

        // ABS intervention
        if (m_absEnabled && std::abs(slipRatio) > m_absSlipThreshold) {
            double reduction = (std::abs(slipRatio) - m_absSlipThreshold) * 2.0;
            longForce *= std::max(0.3, 1.0 - reduction);
        }
        
        // Traction control intervention
        if (m_tcEnabled && std::abs(slipRatio) > m_tcSlipThreshold) {
            double reduction = (std::abs(slipRatio) - m_tcSlipThreshold) * 1.5;
            longForce *= std::max(0.4, 1.0 - reduction);
        }

        m_wheels[i].slipAngle = slipAngleDeg;
        m_wheels[i].slipRatio = slipRatio;
        m_wheels[i].lateralForce = latForce;
        m_wheels[i].longitudinalForce = longForce;

        // Drive torque distribution
        bool isDriven = (m_driveLayout == DriveLayout::FWD && i < 2) ||
                       (m_driveLayout == DriveLayout::RWD && i >= 2) ||
                       (m_driveLayout == DriveLayout::AWD);
        if (isDriven) {
            m_wheels[i].driveTorque = m_wheels[i].longitudinalForce * m_wheelRadius;
        }

        // Brake torque using advanced braking model
        BrakeConfig brakeConfig;
        brakeConfig.frontBrakeBias = 60.0f;
        brakeConfig.rearBrakeBias = 40.0f;
        brakeConfig.hasABS = m_absEnabled;
        brakeConfig.absThreshold = m_absSlipThreshold;

        float normalLoadFront = (m_wheels[0].normalLoad + m_wheels[1].normalLoad) * 0.5f;
        float normalLoadRear = (m_wheels[2].normalLoad + m_wheels[3].normalLoad) * 0.5f;

        BrakeState brakeState = m_brakingModel.calculate(
            brakeConfig, m_brake, m_state.speed,
            normalLoadFront, normalLoadRear, dt
        );

        m_wheels[i].brakeTorque = (i < 2) ? brakeState.frontBrakeTorque : brakeState.rearBrakeTorque;
    }
}

void VehicleSimulator::updateErsAndDrs(double dt) {
    ErsDrsInput input;
    input.dt = dt;
    input.speed = m_state.speed;
    input.rpm = m_state.rpm;
    input.throttle = m_throttle;
    input.brake = m_brake;
    input.currentGear = m_currentGear;
    input.engineTorque = m_engineModelImpl.calculateTorque(m_state.rpm, m_throttle);
    m_ersDrs.update(input);
}

void VehicleSimulator::updateDamageModel(double dt) {
    if (!m_damageEnabled) return;

    float speed = static_cast<float>(m_state.speed);
    float rpm = static_cast<float>(m_state.rpm);

    // Update damage system
    m_damageSystem.update(dt, speed, rpm);

    // Update tire wear for each wheel
    for (int i = 0; i < 4; ++i) {
        float normalLoad = static_cast<float>(m_wheels[i].normalLoad);
        float slipAngle = static_cast<float>(m_tireSlipAngle[i]);
        float slipRatio = static_cast<float>(m_tireSlipRatio[i]);
        float latForce = static_cast<float>(m_wheels[i].lateralForce);
        float longForce = static_cast<float>(m_wheels[i].longitudinalForce);

        m_tireWearSystems[i].update(speed, normalLoad, slipAngle, slipRatio,
                                     latForce, longForce, dt);

        // Update tire state from wear system
        m_tireWear[i] = m_tireWearSystems[i].wearState().wear;
        m_tireGraining[i] = m_tireWearSystems[i].wearState().graining;
        m_tireBlistering[i] = m_tireWearSystems[i].wearState().blistering;
        m_tireTempSurface[i] = m_tireWearSystems[i].thermalState().averageSurfaceTemp();

        // Sync to wheel state
        m_wheels[i].wear = m_tireWearSystems[i].wearState().wear;
        m_wheels[i].temperature = m_tireWearSystems[i].thermalState().averageSurfaceTemp();
    }

    // Update brake wear for each wheel
    for (int i = 0; i < 4; ++i) {
        BrakeWearData brakeData;
        brakeData.wheel = i;
        brakeData.brakeTorque = static_cast<float>(m_wheels[i].brakeTorque);
        brakeData.brakePressure = m_brake > 0 ? static_cast<float>(m_brake * 100.0f) : 0.0f;
        brakeData.wheelSpeed = static_cast<float>(m_wheels[i].angularVelocity);
        brakeData.vehicleSpeed = speed;
        brakeData.normalLoad = static_cast<float>(m_wheels[i].normalLoad);
        brakeData.dt = dt;

        m_brakeWearSystem.update(brakeData);

        // Sync brake damage to legacy damage state
        m_damage.brakeDamage[i] = 1.0f - m_brakeWearSystem.brakingMultiplier(i);
        m_damage.brakePadWear[i] = 1.0f - m_brakeWearSystem.padWearState().remainingLife(i);
    }

    // Apply physics impact from damage to vehicle behavior
    if (m_damageEnabled) {
        // Apply power reduction
        float powerMult = m_damageSystem.powerMultiplier();
        // Handled in engine torque calculation

        // Apply handling reduction (CG shift)
        QVector3D cgShift = m_damageSystem.cgShift();
        m_cgHeight = 0.45 + cgShift.y(); // Adjust CG height from damage
    }
}

void VehicleSimulator::updateFuelWeight(double dt) {
    if (!m_fuelConsumptionEnabled) return;

    double powerKw;
    if (m_engineModel) {
        powerKw = m_engineModel->calculatePower(m_state.rpm);
    } else {
        powerKw = m_engineModelImpl.calculatePower(m_state.rpm);
    }
    double fuelRate = powerKw * 0.2 / 3600.0; // liters per second
    double fuelKgRate = fuelRate * 0.75; // kg per second

    m_fuelKg = std::max(0.0, m_fuelKg - fuelKgRate * dt);
}

double VehicleSimulator::calculateSlipAngle(int wheel, double steeringAngle, double speed, double yawRate) const {
    if (speed < 0.1) return 0.0;
    
    // Vehicle velocities in body frame
    double vx = speed;  // longitudinal velocity
    double vy = m_lateralAccel * speed;  // lateral velocity (approximate from lateral acceleration)
    
    // Wheel position relative to CG
    double wheelX, wheelY;
    double steerAngle = 0.0;
    
    // Wheel positions: 0=FL, 1=FR, 2=RL, 3=RR
    switch (wheel) {
        case 0:  // Front Left
            wheelX = m_frontAxleDist;
            wheelY = m_trackWidth * 0.5;
            steerAngle = steeringAngle;
            break;
        case 1:  // Front Right
            wheelX = m_frontAxleDist;
            wheelY = -m_trackWidth * 0.5;
            steerAngle = steeringAngle;
            break;
        case 2:  // Rear Left
            wheelX = -m_rearAxleDist;
            wheelY = m_trackWidth * 0.5;
            steerAngle = 0.0;
            break;
        case 3:  // Rear Right
            wheelX = -m_rearAxleDist;
            wheelY = -m_trackWidth * 0.5;
            steerAngle = 0.0;
            break;
        default:
            return 0.0;
    }
    
    // Velocity at wheel contact patch
    double wheelVx = vx - yawRate * wheelY;
    double wheelVy = vy + yawRate * wheelX;
    
    // Slip angle = atan(lateral velocity / longitudinal velocity) - steering angle
    double slipAngle = std::atan2(wheelVy, wheelVx) - steerAngle;
    
    // Normalize to [-pi, pi]
    while (slipAngle > M_PI) slipAngle -= 2.0 * M_PI;
    while (slipAngle < -M_PI) slipAngle += 2.0 * M_PI;
    
    return qRadiansToDegrees(slipAngle);
}

double VehicleSimulator::calculateSlipRatio(int wheel) const {
    double wheelSpeed = m_wheels[wheel].angularVelocity * m_wheelRadius;
    double vehicleSpeed = m_state.speed;
    if (vehicleSpeed < 0.1) return 0.0;
    return (wheelSpeed - vehicleSpeed) / vehicleSpeed;
}

// ============================================================================
// Weather integration methods
// ============================================================================

double VehicleSimulator::getAquaplaningRisk() const {
    return m_aquaplaningRisk;
}

double VehicleSimulator::getTrackGripReduction() const {
    return m_trackGripReduction;
}

const WeatherState& VehicleSimulator::weatherState() const {
    return m_weatherSim.weatherState();
}

WeatherState& VehicleSimulator::weatherState() {
    return m_weatherSim.weatherState();
}

void VehicleSimulator::setAirDensity(double density) {
    m_weatherSim.setAirDensity(density);
}

HybridSystem& VehicleSimulator::hybridSystem() { return m_ersDrs.hybridSystem(); }
const HybridSystem& VehicleSimulator::hybridSystem() const { return m_ersDrs.hybridSystem(); }
BrakeModelManager& VehicleSimulator::brakeModel() { return *m_brakeModel; }
const BrakeModelManager& VehicleSimulator::brakeModel() const { return *m_brakeModel; }

// ============================================================================
// Singleton cleanup
// ============================================================================

void VehicleSimulator::cleanup() {
    if (s_instance) {
        delete s_instance;
        s_instance = nullptr;
    }
}

} // namespace physics
} // namespace ks
