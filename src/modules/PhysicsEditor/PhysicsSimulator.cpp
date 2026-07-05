#include "PhysicsSimulator.h"
#include "PhysicsProfiler.h"
#include <QDebug>
#include <QtMath>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

namespace ks {

// ============================================================================
// phys_TireModel (legacy, kept for backward compat with UI)
// ============================================================================

phys_TireModel::phys_TireModel()
    : m_b(6.0), m_c(1.3), m_d(1.0), m_e(-0.5)
{
    m_slipCurve.name = "Default";
    m_slipCurve.compound = "Street";
    m_slipCurve.peakSlipAngle = 8.0;
    m_slipCurve.peakSlipRatio = 0.12;
    m_slipCurve.peakLateralMu = 1.0;
    m_slipCurve.peakLongitudinalMu = 1.1;
    m_slipCurve.stiffnessLateral = 30000.0;
    m_slipCurve.stiffnessLongitudinal = 50000.0;
}

void phys_TireModel::setSlipCurve(const TireSlipCurve& curve) {
    m_slipCurve = curve;
}

double phys_TireModel::pacejkaFormula(double x, double b, double c, double d, double e) const {
    return d * qSin(c * qAtan(b * x - e * (b * x - qAtan(b * x))));
}

double phys_TireModel::calculateLateralForce(double slipAngle, double normalLoad, double frictionCoeff) {
    double x = qDegreesToRadians(slipAngle);
    double peakForce = m_slipCurve.peakLateralMu * normalLoad * frictionCoeff;
    double b = m_b / peakForce;
    return pacejkaFormula(x, b, m_c, peakForce, m_e);
}

double phys_TireModel::calculateLongitudinalForce(double slipRatio, double normalLoad, double frictionCoeff) {
    double peakForce = m_slipCurve.peakLongitudinalMu * normalLoad * frictionCoeff;
    double b = m_b / peakForce;
    return pacejkaFormula(slipRatio, b, m_c, peakForce, m_e);
}

double phys_TireModel::calculateAligningTorque(double slipAngle, double normalLoad) {
    double latForce = calculateLateralForce(slipAngle, normalLoad, 1.0);
    double pneumaticTrail = 0.05 * std::exp(-std::abs(qDegreesToRadians(slipAngle)) * 5.0);
    return latForce * pneumaticTrail;
}

double phys_TireModel::calculatePeakSlipAngle(double normalLoad) const {
    return m_slipCurve.peakSlipAngle * (1.0 + 0.1 * (normalLoad - 4000.0) / 4000.0);
}

double phys_TireModel::calculatePeakSlipRatio(double normalLoad) const {
    return m_slipCurve.peakSlipRatio * (1.0 + 0.05 * (normalLoad - 4000.0) / 4000.0);
}

void phys_TireModel::setPacejkaCoefficients(double b, double c, double d, double e) {
    m_b = b; m_c = c; m_d = d; m_e = e;
}

QVector<QPointF> phys_TireModel::generateLateralForceCurve(double normalLoad, double frictionCoeff) {
    QVector<QPointF> curve;
    for (double angle = -15.0; angle <= 15.0; angle += 0.5) {
        double force = calculateLateralForce(angle, normalLoad, frictionCoeff);
        curve.append(QPointF(angle, force));
    }
    return curve;
}

QVector<QPointF> phys_TireModel::generateLongitudinalForceCurve(double normalLoad, double frictionCoeff) {
    QVector<QPointF> curve;
    for (double slip = -1.0; slip <= 1.0; slip += 0.02) {
        double force = calculateLongitudinalForce(slip, normalLoad, frictionCoeff);
        curve.append(QPointF(slip, force));
    }
    return curve;
}

// ============================================================================
// phys_LapTimer
// ============================================================================

phys_LapTimer::phys_LapTimer(QObject* parent) : QObject(parent) {
}

void phys_LapTimer::startLap() {
    m_currentLapTime = 0.0;
    m_totalDistance = 0.0;
    m_currentSector = 1;
    m_sector1Time = 0.0;
    m_sector2Time = 0.0;
    m_sector3Time = 0.0;
    m_lastSectorDistance = 0.0;
    m_topSpeedRecorded = 0.0;
    m_maxLateralGRecorded = 0.0;
    m_gearChangesRecorded = 0;
}

void phys_LapTimer::stopLap() {
    m_lastLapTime = m_currentLapTime;
    if (m_currentLapTime > 0 && m_currentLapTime < m_bestLapTime) {
        m_bestLapTime = m_currentLapTime;
    }
    m_lapCount++;
}

void phys_LapTimer::reset() {
    m_currentLapTime = 0.0;
    m_bestLapTime = 1e9;
    m_lastLapTime = 0.0;
    m_lapCount = 0;
    m_totalDistance = 0.0;
    m_currentSector = 1;
}

void phys_LapTimer::update(double dt, double speed, double distance) {
    m_currentLapTime += dt;
    m_totalDistance = distance;

    if (speed > m_topSpeedRecorded) m_topSpeedRecorded = speed;

    if (m_sector2Distance > 0 && m_totalDistance >= m_sector2Distance && m_currentSector == 1) {
        m_sector1Time = m_currentLapTime;
        m_currentSector = 2;
        m_lastSectorDistance = m_sector2Distance;
        emit sectorCompleted(1, m_sector1Time);
    }
    if (m_sector3Distance > 0 && m_totalDistance >= m_sector3Distance && m_currentSector == 2) {
        m_sector2Time = m_currentLapTime - m_sector1Time;
        m_currentSector = 3;
        m_lastSectorDistance = m_sector3Distance;
        emit sectorCompleted(2, m_sector2Time);
    }
}

void phys_LapTimer::setSectorDistances(double sector1, double sector2, double sector3) {
    m_sector1Distance = sector1;
    m_sector2Distance = sector2;
    m_sector3Distance = sector3;
}

void phys_LapTimer::recordLateralG(double gForce) {
    if (std::abs(gForce) > std::abs(m_maxLateralGRecorded)) {
        m_maxLateralGRecorded = gForce;
    }
}

void phys_LapTimer::recordGearChange() {
    m_gearChangesRecorded++;
}

LapTimeEstimate phys_LapTimer::estimateLapTime(const QVector<double>& historicalLapTimes,
                                                  double trackLength,
                                                  double avgCornerSpeed) const {
    LapTimeEstimate est;
    est.totalLapTime = m_currentLapTime;
    est.sector1Time = m_sector1Time;
    est.sector2Time = m_sector2Time;
    est.sector3Time = m_sector3Time;
    est.avgSpeed = (m_currentLapTime > 0.001) ? trackLength / m_currentLapTime : 0.0;
    est.minCornerSpeed = avgCornerSpeed * 0.6;
    est.fuelConsumption = 0.0;

    if (!historicalLapTimes.isEmpty()) {
        double sum = 0.0;
        double maxLap = 0.0;
        for (double t : historicalLapTimes) {
            sum += t;
            maxLap = std::max(maxLap, t);
        }
        double avg = sum / historicalLapTimes.size();
        double variance = 0.0;
        for (double t : historicalLapTimes) variance += (t - avg) * (t - avg);
        variance /= historicalLapTimes.size();
        double stdDev = std::sqrt(variance);

        double projectedImprovement = stdDev * 0.3;
        est.totalLapTime = avg - projectedImprovement;
        est.confidenceLevel = (avg > 1e-9) ? std::clamp(1.0 - (stdDev / avg), 0.0, 1.0) : 0.5;
        est.topSpeed = est.avgSpeed * 1.4;

        if (est.minCornerSpeed > 0.1) {
            double avgCornerRadius = trackLength / (std::max(1.0, static_cast<double>(historicalLapTimes.size())) * 8.0);
            est.maxLateralG = (est.minCornerSpeed * est.minCornerSpeed) / (avgCornerRadius * 9.81);
        }
    } else {
        est.confidenceLevel = 0.5;
        est.topSpeed = m_topSpeedRecorded;
        est.maxLateralG = m_maxLateralGRecorded;
        est.numGearChanges = m_gearChangesRecorded;
    }

    return est;
}

// ============================================================================
// phys_Simulator
// ============================================================================

phys_Simulator* phys_Simulator::s_instance = nullptr;

phys_Simulator::phys_Simulator(QObject* parent)
    : QObject(parent)
{
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
}

phys_Simulator* phys_Simulator::instance() {
    if (!s_instance) {
        s_instance = new phys_Simulator();
    }
    return s_instance;
}

void phys_Simulator::startSimulation() {
    m_running = true;
    m_simTimer.start();
    m_lapTimer.startLap();
    emit simulationStarted();
    qDebug() << "Physics simulation started";
}

void phys_Simulator::stopSimulation() {
    m_running = false;
    m_lapTimer.stopLap();
    if (m_currentGear > 0) {
        m_lapTimeHistory.append(m_lapTimer.currentLapTime());
    }
    emit simulationStopped();
}

void phys_Simulator::reset() {
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
        m_tireTempSurface[i] = 30.0;
        m_tireTempCarcass[i] = 35.0;
        m_tireTempCore[i] = 40.0;
        m_tirePressure[i] = 2.4;
        m_tireWear[i] = 0.0;
    }
    m_engineModel.reset();
    m_diffModel.reset();
    m_brakeModel.reset();
    m_lapTimer.reset();
    emit stateUpdated(m_state);
}

void phys_Simulator::setThrottle(double value) {
    m_throttle = std::clamp(value, 0.0, 1.0);
}

void phys_Simulator::setBrake(double value) {
    m_brake = std::clamp(value, 0.0, 1.0);
}

void phys_Simulator::setSteering(double value) {
    m_steering = std::clamp(value, -1.0, 1.0);
}

void phys_Simulator::setTireModel(const TireSlipCurve& curve) {
    m_tireModel = curve;
    m_tireEngine.setSlipCurve(curve);
}

LapTimeEstimate phys_Simulator::estimateLapTime() const {
    return m_lapTimer.estimateLapTime(m_lapTimeHistory, m_trackLength, 80.0);
}

// ============================================================================
// Vehicle parameter loading from AC INI files
// ============================================================================

void phys_Simulator::loadVehicleParams(const QString& carPath) {
    loadEngineFromIni(carPath + "/data/engine.ini");
    loadTyresFromIni(carPath + "/data/tyres.ini");
    loadDrivetrainFromIni(carPath + "/data/drivetrain.ini");
    loadAeroFromIni(carPath + "/data/aero.ini");
    loadSuspensionFromIni(carPath + "/data/suspensions.ini");
}

void phys_Simulator::loadEngineFromIni(const QString& engineIniPath) {
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

    // Sync scalar params
    m_enginePowerKw = config.peakPower;
    m_maxRpm = config.maxRPM;
    m_finalDriveRatio = config.finalDrive;

    // Load power LUT if it exists
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

void phys_Simulator::loadTyresFromIni(const QString& tyresIniPath) {
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
            else if (key == "WIDTH") { /* tire width, useful for aero */ }
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

void phys_Simulator::loadDrivetrainFromIni(const QString& drivetrainIniPath) {
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
                    // Parse GEAR_1=3.5 style
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

void phys_Simulator::loadAeroFromIni(const QString& aeroIniPath) {
    AeroModel::AeroConfig config = AeroModel::loadFromIni(aeroIniPath);
    m_aeroModel = AeroModel();
    for (const AeroModel::Wing& wing : config.wings) {
        m_aeroModel.addWing(wing);
    }
    m_cd = config.dragCoefficient;
    m_frontalArea = config.frontalArea;
}

void phys_Simulator::loadSuspensionFromIni(const QString& suspensionIniPath) {
    m_suspensionModel.loadFromIni(suspensionIniPath);
}

// ============================================================================
// Physics helpers
// ============================================================================

double phys_Simulator::calculateSlipAngle(int wheel, double steeringAngle, double speed, double yawRate) const {
    if (speed < 0.5) return 0.0;

    // Front wheels: steer. Rear wheels: no steer.
    bool isFront = (wheel < 2);
    bool isLeft = (wheel == 0 || wheel == 2);

    // Wheel position relative to CG
    double wheelX = isFront ? m_frontAxleDist : -m_rearAxleDist;
    double wheelY = isLeft ? (m_trackWidth / 2.0) : (-m_trackWidth / 2.0);

    // Steer angle at wheel (Ackermann simplified: both front wheels same angle)
    double wheelSteer = isFront ? steeringAngle : 0.0;

    // Velocity at wheel contact patch (body velocity + yaw contribution)
    double vx = speed - yawRate * wheelY;
    double vy = yawRate * wheelX;

    // Slip angle = atan2(vy, vx) - wheel steer
    double slipAngle = qAtan2(vy, vx) - wheelSteer;
    return qRadiansToDegrees(slipAngle);
}

double phys_Simulator::calculateSlipRatio(int wheel) const {
    const WheelState& w = m_wheels[wheel];
    double wheelSpeed = w.angularVelocity * m_wheelRadius;
    double vehicleSpeed = m_state.speed;

    if (vehicleSpeed < 0.5) return 0.0;

    return (wheelSpeed - vehicleSpeed) / std::max(0.1, vehicleSpeed);
}

void phys_Simulator::updateWeightTransfer(double longitudinalAccel, double lateralAccel) {
    double FzTotal = m_mass * 9.81;

    // Longitudinal weight transfer: dFz = m * a * h / L
    double longTransfer = m_mass * longitudinalAccel * m_cgHeight / m_wheelBase;

    // Lateral weight transfer: dFz = m * ay * h / T
    double latTransfer = m_mass * lateralAccel * m_cgHeight / m_trackWidth;

    // Static distribution (front/rear split based on axle distances)
    double frontStatic = FzTotal * m_rearAxleDist / m_wheelBase;
    double rearStatic = FzTotal * m_frontAxleDist / m_wheelBase;

    // Per-wheel loads: FL, FR, RL, RR
    double halfFrontStatic = frontStatic / 2.0;
    double halfRearStatic = rearStatic / 2.0;
    double halfLongTransfer = longTransfer / 2.0;
    double halfLatTransfer = latTransfer / 2.0;

    // Front wheels get less weight under acceleration (weight shifts rear)
    m_wheels[0].normalLoad = halfFrontStatic - halfLongTransfer + halfLatTransfer; // FL
    m_wheels[1].normalLoad = halfFrontStatic - halfLongTransfer - halfLatTransfer; // FR
    m_wheels[2].normalLoad = halfRearStatic + halfLongTransfer + halfLatTransfer;  // RL
    m_wheels[3].normalLoad = halfRearStatic + halfLongTransfer - halfLatTransfer;  // RR

    // Clamp to prevent negative load
    for (int i = 0; i < 4; ++i) {
        m_wheels[i].normalLoad = std::max(50.0, m_wheels[i].normalLoad);
    }
}

// ============================================================================
// Main physics update
// ============================================================================

void phys_Simulator::updatePhysics(double dt) {
    if (!m_running) return;

    auto* prof = PhysicsProfiler::instance();
    prof->beginFrame();

    dt = std::min(dt, 0.02);

    // --- Engine model update ---
    prof->beginSubsystem(PhysicsProfiler::Engine);
    EngineModel::EngineConfig engConfig = m_engineModel.getConfig();

    // Calculate wheel RPM and engine RPM for current gear
    double wheelRpm = m_state.speed * 60.0 / (2.0 * M_PI * m_wheelRadius);
    double engineRpm = wheelRpm * m_gearRatios[m_currentGear - 1] * m_finalDriveRatio;
    engineRpm = std::clamp(engineRpm, static_cast<double>(engConfig.idleRPM), static_cast<double>(engConfig.revLimiter) * 1.1);

    // Calculate engine torque from LUT
    float baseTorque = m_engineModel.calculateTorque(engineRpm);

    // Apply turbo if enabled
    EngineModel::TurboConfig turbo = engConfig.turbo;
    if (turbo.enabled) {
        float boost = m_engineModel.calculateTurboBoost(engineRpm, m_throttle);
        baseTorque = m_engineModel.calculateTurboTorque(baseTorque, boost);
    }

    // Scale by throttle
    float engineTorque = baseTorque * m_throttle;

    // Engine braking when off throttle
    if (m_throttle < 0.05f) {
        engineTorque -= m_engineModel.calculateEngineBraking(engineRpm);
    }

    // Auto gear shifting
    int prevGear = m_currentGear;
    if (engineRpm > engConfig.revLimiter * 0.95 && m_currentGear < m_gearRatios.size()) {
        m_currentGear++;
    } else if (engineRpm < engConfig.peakPowerRPM * 0.5 && m_currentGear > 1) {
        m_currentGear--;
    }
    if (m_currentGear != prevGear) {
        m_lapTimer.recordGearChange();
    }

    // Torque at wheels through drivetrain
    double totalGearRatio = m_gearRatios[m_currentGear - 1] * m_finalDriveRatio;
    double axleTorque = engineTorque * totalGearRatio;

    prof->endSubsystem(PhysicsProfiler::Engine);

    // --- Differential torque split ---
    prof->beginSubsystem(PhysicsProfiler::Differential);
    double leftWheelSpeed = m_wheels[2].angularVelocity;
    double rightWheelSpeed = m_wheels[3].angularVelocity;
    m_diffModel.update(dt, axleTorque, leftWheelSpeed, rightWheelSpeed);

    double leftDriveTorque = m_diffModel.getLeftTorque();
    double rightDriveTorque = m_diffModel.getRightTorque();

    // For RWD: rear wheels get drive torque, front wheels get 0
    m_wheels[0].driveTorque = 0.0;
    m_wheels[1].driveTorque = 0.0;
    m_wheels[2].driveTorque = leftDriveTorque;
    m_wheels[3].driveTorque = rightDriveTorque;
    prof->endSubsystem(PhysicsProfiler::Differential);

    // --- Brakes ---
    prof->beginSubsystem(PhysicsProfiler::Brakes);
    double maxBrakeTorque = 8000.0;
    double brakeTorque = m_brake * maxBrakeTorque;
    for (int i = 0; i < 4; ++i) {
        m_wheels[i].brakeTorque = brakeTorque;
    }

    // Update brake thermal model
    m_brakeModel.update(dt, m_brake, m_state.speed);

    // Apply brake fade effect
    float brakeFade = m_brakeModel.getBrakeFade();
    for (int i = 0; i < 4; ++i) {
        m_wheels[i].brakeTorque *= brakeFade;
    }

    // --- ABS / TC ---
    prof->beginSubsystem(PhysicsProfiler::ABS_TC);
    if (m_absEnabled && m_brake > 0.1) {
        for (int i = 0; i < 4; ++i) {
            double slipRatio = std::abs(m_wheels[i].slipRatio);
            if (slipRatio > m_absSlipThreshold) {
                double reduction = std::max(0.0, 1.0 - (slipRatio - m_absSlipThreshold) * 5.0);
                m_wheels[i].brakeTorque *= std::max(0.2, reduction);
            }
        }
    }

    if (m_tcEnabled && m_throttle > 0.1) {
        for (int i = 0; i < 4; ++i) {
            if (m_wheels[i].driveTorque > 0) {
                double slipRatio = m_wheels[i].slipRatio;
                if (slipRatio > m_tcSlipThreshold) {
                    double reduction = std::max(0.0, 1.0 - (slipRatio - m_tcSlipThreshold) * 5.0);
                    m_wheels[i].driveTorque *= std::max(0.3, reduction);
                }
            }
        }
    }
    prof->endSubsystem(PhysicsProfiler::ABS_TC);
    prof->endSubsystem(PhysicsProfiler::Brakes);

    // --- Aerodynamic forces ---
    prof->beginSubsystem(PhysicsProfiler::Aero);
    AeroModel::AeroState aeroState;
    aeroState.speed = m_state.speed;
    aeroState.rideHeightFront = 0.05;
    aeroState.rideHeightRear = 0.07;
    AeroModel::AeroForces aeroForces = m_aeroModel.calculate(aeroState);
    prof->endSubsystem(PhysicsProfiler::Aero);

    // --- Weight transfer ---
    prof->beginSubsystem(PhysicsProfiler::WeightTransfer);
    updateWeightTransfer(m_state.acceleration.z(), m_lateralAccel);
    prof->endSubsystem(PhysicsProfiler::WeightTransfer);

    // --- Per-wheel forces ---
    prof->beginSubsystem(PhysicsProfiler::PerWheelForces);
    updatePerWheelForces(dt);
    prof->endSubsystem(PhysicsProfiler::PerWheelForces);

    // --- Sum forces ---
    double totalLateralForce = 0.0;
    double totalLongitudinalForce = 0.0;
    for (int i = 0; i < 4; ++i) {
        totalLateralForce += m_wheels[i].lateralForce;
        totalLongitudinalForce += m_wheels[i].longitudinalForce;
    }

    // --- Vehicle dynamics ---
    prof->beginSubsystem(PhysicsProfiler::VehicleDynamics);
    double dragForce = 0.5 * 1.225 * m_cd * m_frontalArea * m_state.speed * m_state.speed;
    double rollingResistance = m_mass * 9.81 * 0.015;

    double netLongitudinalForce = totalLongitudinalForce - dragForce - rollingResistance
                                  - aeroForces.drag;

    double longitudinalAccel = netLongitudinalForce / m_mass;
    double lateralAccel = totalLateralForce / m_mass;

    // Yaw dynamics (bicycle model)
    double yawInertia = m_mass * (m_wheelBase * m_wheelBase + m_trackWidth * m_trackWidth) / 12.0;
    double yawTorque = 0.0;
    double frontLateralForce = m_wheels[0].lateralForce + m_wheels[1].lateralForce;
    double rearLateralForce = m_wheels[2].lateralForce + m_wheels[3].lateralForce;
    yawTorque = frontLateralForce * m_frontAxleDist - rearLateralForce * m_rearAxleDist;

    yawTorque += aeroForces.rearDownforce * 0.01;

    double yawAccel = yawTorque / yawInertia;
    m_yawRate += yawAccel * dt;
    m_yawRate *= 0.98;

    // Update state
    m_state.acceleration = QVector3D(lateralAccel, 0, longitudinalAccel);
    m_state.velocity = m_state.velocity + QVector3D(lateralAccel * dt, 0, longitudinalAccel * dt);
    m_state.speed = m_state.velocity.length();
    m_state.rpm = std::clamp(engineRpm, 1000.0, engConfig.revLimiter * 1.1);
    m_state.heading += m_yawRate * dt;
    m_state.lapTime = m_lapTimer.currentLapTime();
    m_state.currentLapDistance += m_state.speed * dt;
    m_state.steeringAngle = m_steering * m_steerLock;
    m_lateralAccel = lateralAccel;
    prof->endSubsystem(PhysicsProfiler::VehicleDynamics);

    // --- Fuel consumption ---
    prof->beginSubsystem(PhysicsProfiler::Fuel);
    float fuelFlow = m_engineModel.calculateFuelFlow(m_engineModel.calculatePower(engineRpm));
    m_fuelKg -= fuelFlow * dt / 3600.0 * 0.75;
    m_fuelKg = std::max(0.0, m_fuelKg);
    prof->endSubsystem(PhysicsProfiler::Fuel);

    // --- Update lap timer ---
    prof->beginSubsystem(PhysicsProfiler::LapTimer);
    m_lapTimer.update(dt, m_state.speed, m_state.currentLapDistance);
    prof->endSubsystem(PhysicsProfiler::LapTimer);

    // --- Emit tire data (average across wheels) ---
    double avgSlipAngle = 0.0;
    double avgLateralForce = 0.0;
    double avgSlipRatio = 0.0;
    double avgLongForce = 0.0;
    for (int i = 0; i < 4; ++i) {
        avgSlipAngle += m_wheels[i].slipAngle;
        avgLateralForce += m_wheels[i].lateralForce;
        avgSlipRatio += m_wheels[i].slipRatio;
        avgLongForce += m_wheels[i].longitudinalForce;
    }
    emit tireDataUpdated(avgSlipAngle / 4.0, avgLateralForce, avgSlipRatio / 4.0, avgLongForce);
    emit stateUpdated(m_state);

    // --- Update thermal tire model ---
    prof->beginSubsystem(PhysicsProfiler::TireThermal);
    updateTireModel(dt);
    prof->endSubsystem(PhysicsProfiler::TireThermal);

    // --- Record lateral G ---
    double lateralG = lateralAccel / 9.81;
    m_lapTimer.recordLateralG(lateralG);

    prof->endFrame();

    // --- Check for lap completion ---
    if (m_state.currentLapDistance > m_trackLength) {
        m_lapTimer.stopLap();
        m_state.currentLapDistance = 0.0;
        m_lapTimer.startLap();
        m_lapTimeHistory.append(m_lapTimer.lastLapTime());
    }
}

// ============================================================================
// Brake temperature accessors
// ============================================================================

float phys_Simulator::getBrakeDiscTemp(int wheel) const {
    if (wheel < 0 || wheel > 3) return 0.0f;
    return m_brakeModel.getBrake(wheel).calculateDiscTemp();
}

float phys_Simulator::getBrakePadTemp(int wheel) const {
    if (wheel < 0 || wheel > 3) return 0.0f;
    return m_brakeModel.getBrake(wheel).calculatePadTemp();
}

float phys_Simulator::getBrakeFade(int wheel) const {
    if (wheel < 0 || wheel > 3) return 1.0f;
    return m_brakeModel.getBrake(wheel).calculateBrakeFade();
}

void phys_Simulator::updatePerWheelForces(double dt) {
    for (int i = 0; i < 4; ++i) {
        WheelState& w = m_wheels[i];

        // Calculate slip angle from steering geometry
        w.slipAngle = calculateSlipAngle(i, qDegreesToRadians(m_state.steeringAngle),
                                          m_state.speed, m_yawRate);

        // Calculate slip ratio
        w.slipRatio = calculateSlipRatio(i);

        // Get camber from suspension model
        w.camber = m_suspensionModel.getWheel(i).getCamber();

        // Temperature and wear effects
        double tempEffect = m_pacejkaModel.calculateTemperatureEffect(w.temperature);
        double wearEffect = m_pacejkaModel.calculateWearEffect(w.wear);
        double pressureEffect = m_pacejkaModel.calculatePressureEffect(w.pressure);
        double effectiveGrip = tempEffect * wearEffect * pressureEffect;

        // Calculate combined slip forces using full Pacejka model
        PacejkaTireModel::TireForces forces = m_pacejkaModel.calculateCombinedSlip(
            w.slipAngle, w.slipRatio, w.normalLoad, w.camber);

        w.lateralForce = forces.lateralForce * effectiveGrip;
        w.longitudinalForce = forces.longitudinalForce * effectiveGrip;

        // Apply drive and brake forces (longitudinal)
        double wheelInertia = 1.5; // kg*m^2 approximate
        double netTorque = w.driveTorque - w.brakeTorque
                           - w.longitudinalForce * m_wheelRadius;
        double angularAccel = netTorque / (wheelInertia + m_mass * m_wheelRadius * m_wheelRadius * 0.01);
        w.angularVelocity += angularAccel * dt / m_wheelRadius;
        w.angularVelocity = std::max(0.0, w.angularVelocity);

        // Update wheel speeds for next frame
        if (i < 2) {
            // Front wheels - free rolling
            w.angularVelocity = m_state.speed / m_wheelRadius;
        }
    }
}

// ============================================================================
// Thermal tire model update
// ============================================================================

void phys_Simulator::updateTireModel(double dt) {
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

        // Update wheel state
        w.temperature = m_tireTempSurface[i];

        double Tkelvin = (m_tireTempCarcass[i] + 273.15) / (30.0 + 273.15);
        m_tirePressure[i] = std::clamp(2.4 * Tkelvin, 1.5, 3.5);
        w.pressure = m_tirePressure[i];

        double wearRate = 1e-8 * m_tireModel.peakLateralMu * Fz * totalSlip * std::max(v, 1.0) / (Tcar + 50.0);
        m_tireWear[i] = std::min(1.0, m_tireWear[i] + wearRate * dt);
        w.wear = m_tireWear[i];
    }
}

// ============================================================================
// Telemetry validation
// ============================================================================

phys_Simulator::ValidationMetrics phys_Simulator::validateAgainstTelemetry(
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
    metrics.nSamples = 0;

    if (timestamps.isEmpty()) return metrics;

    int n = qMin(timestamps.size(), refSpeed.size());
    n = qMin(n, refThrottle.size());
    n = qMin(n, refBrake.size());
    n = qMin(n, refSteering.size());
    if (n < 2) return metrics;

    double sumSpeedErr2 = 0, sumLatGErr2 = 0, sumLongGErr2 = 0, sumRpmErr2 = 0;
    double sumSpeedRef2 = 0, sumLatGRef2 = 0, sumLongGRef2 = 0, sumRpmRef2 = 0;
    double sumSpeedSim2 = 0, sumLatGSim2 = 0, sumLongGSim2 = 0, sumRpmSim2 = 0;
    double sumSpeedSim = 0, sumSpeedRef = 0;
    double sumLatGSim = 0, sumLatGRef = 0;
    double sumLongGSim = 0, sumLongGRef = 0;
    double sumRpmSim = 0, sumRpmRef = 0;
    double sumSpeedSimRef = 0, sumLatGSimRef = 0;
    double sumLongGSimRef = 0, sumRpmSimRef = 0;
    double sumSpeedRefPct = 1.0; // avoid div-by-zero

    metrics.speedMaxError = 0;
    metrics.lateralGMaxError = 0;
    metrics.longitudinalGMaxError = 0;
    metrics.rpmMaxError = 0;

    phys_Simulator replaySim;
    replaySim.m_mass = m_mass;
    replaySim.m_enginePowerKw = m_enginePowerKw;
    replaySim.m_maxRpm = m_maxRpm;
    replaySim.m_cd = m_cd;
    replaySim.m_frontalArea = m_frontalArea;
    replaySim.m_wheelBase = m_wheelBase;
    replaySim.m_trackWidth = m_trackWidth;
    replaySim.m_gearRatios = m_gearRatios;
    replaySim.m_finalDriveRatio = m_finalDriveRatio;
    replaySim.m_wheelRadius = m_wheelRadius;
    replaySim.m_cgHeight = m_cgHeight;
    replaySim.m_frontAxleDist = m_frontAxleDist;
    replaySim.m_rearAxleDist = m_rearAxleDist;
    replaySim.m_engineModel = m_engineModel;
    replaySim.m_aeroModel = m_aeroModel;
    replaySim.m_diffModel = m_diffModel;
    replaySim.m_suspensionModel = m_suspensionModel;
    replaySim.m_pacejkaModel = m_pacejkaModel;
    replaySim.m_tireModel = m_tireModel;
    replaySim.m_tireEngine = m_tireEngine;
    replaySim.m_running = true;
    replaySim.m_trackLength = m_trackLength;

    // Initialize replay tire temperatures to match reference
    for (int w = 0; w < 4; ++w) {
        replaySim.m_tireTempSurface[w] = m_tireTempSurface[w];
        replaySim.m_tireTempCarcass[w] = m_tireTempCarcass[w];
        replaySim.m_tireTempCore[w] = m_tireTempCore[w];
    }
    replaySim.m_ambientTemp = m_ambientTemp;

    for (int i = 0; i < n; ++i) {
        double dt = (i == 0) ? 0.01 : (timestamps[i] - timestamps[i - 1]);
        dt = std::clamp(dt, 0.001, 0.05);

        double refTh = refThrottle[i];
        double refBr = refBrake[i];
        double refSt = refSteering[i];

        replaySim.m_throttle = refTh;
        replaySim.m_brake = refBr;
        replaySim.m_steering = refSt;

        replaySim.updatePhysics(dt);

        double simSpeed = replaySim.m_state.speed;
        double simRpm = replaySim.m_state.rpm;

        double totalLatF = 0;
        for (int w = 0; w < 4; ++w) totalLatF += replaySim.m_wheels[w].lateralForce;
        double simLatG = totalLatF / (m_mass * 9.81);

        double totalLongF = 0;
        for (int w = 0; w < 4; ++w) totalLongF += replaySim.m_wheels[w].longitudinalForce;
        double dragF = 0.5 * 1.225 * m_cd * m_frontalArea * simSpeed * simSpeed;
        double rollingF = m_mass * 9.81 * 0.015;
        double simLongG = (totalLongF - dragF - rollingF) / (m_mass * 9.81);

        double refSpd = refSpeed[i] / 3.6;
        double refLat = (i < refLateralG.size()) ? refLateralG[i] : 0;
        double refLng = (i < refLongG.size()) ? refLongG[i] : 0;
        double refRp = (i < refRPM.size()) ? refRPM[i] : 0;

        double spdErr = simSpeed - refSpd;
        double latErr = simLatG - refLat;
        double lngErr = simLongG - refLng;
        double rpmErr = simRpm - refRp;

        sumSpeedErr2 += spdErr * spdErr;
        sumLatGErr2 += latErr * latErr;
        sumLongGErr2 += lngErr * lngErr;
        sumRpmErr2 += rpmErr * rpmErr;

        metrics.speedMaxError = qMax(metrics.speedMaxError, qAbs(spdErr));
        metrics.lateralGMaxError = qMax(metrics.lateralGMaxError, qAbs(latErr));
        metrics.longitudinalGMaxError = qMax(metrics.longitudinalGMaxError, qAbs(lngErr));
        metrics.rpmMaxError = qMax(metrics.rpmMaxError, qAbs(rpmErr));

        sumSpeedRef2 += refSpd * refSpd;
        sumLatGRef2 += refLat * refLat;
        sumLongGRef2 += refLng * refLng;
        sumRpmRef2 += refRp * refRp;
        sumSpeedSim2 += simSpeed * simSpeed;
        sumLatGSim2 += simLatG * simLatG;
        sumLongGSim2 += simLongG * simLongG;
        sumRpmSim2 += simRpm * simRpm;
        sumSpeedSim += simSpeed;
        sumSpeedRef += refSpd;
        sumLatGSim += simLatG;
        sumLatGRef += refLat;
        sumLongGSim += simLongG;
        sumLongGRef += refLng;
        sumRpmSim += simRpm;
        sumRpmRef += refRp;
        sumSpeedSimRef += simSpeed * refSpd;
        sumLatGSimRef += simLatG * refLat;
        sumLongGSimRef += simLongG * refLng;
        sumRpmSimRef += simRpm * refRp;

        if (refSpd > 1.0) sumSpeedRefPct += refSpd;
    }

    metrics.nSamples = n;
    metrics.speedRMSE = std::sqrt(sumSpeedErr2 / n);
    metrics.lateralGRMSE = std::sqrt(sumLatGErr2 / n);
    metrics.longitudinalGRMSE = std::sqrt(sumLongGErr2 / n);
    metrics.rpmRMSE = std::sqrt(sumRpmErr2 / n);

    // Percentage RMSE (normalized by mean reference value)
    double meanSpeedRef = sumSpeedRef / n;
    double meanLatGRef = sumLatGRef / n;
    double meanLongGRef = sumLongGRef / n;
    double meanRpmRef = sumRpmRef / n;
    metrics.speedPercentRMSE = (meanSpeedRef > 0.1) ? (metrics.speedRMSE / meanSpeedRef * 100.0) : 0;
    metrics.lateralGPercentRMSE = (qAbs(meanLatGRef) > 0.01) ? (metrics.lateralGRMSE / qAbs(meanLatGRef) * 100.0) : 0;
    metrics.longitudinalGPercentRMSE = (qAbs(meanLongGRef) > 0.01) ? (metrics.longitudinalGRMSE / qAbs(meanLongGRef) * 100.0) : 0;
    metrics.rpmPercentRMSE = (meanRpmRef > 100.0) ? (metrics.rpmRMSE / meanRpmRef * 100.0) : 0;

    // R^2 correlations for all four signals
    auto calcR2 = [n](double sumSim2, double sumRef2, double sumSim, double sumRef, double sumSimRef) -> double {
        double varRef = sumRef2 / n - (sumRef / n) * (sumRef / n);
        double varSim = sumSim2 / n - (sumSim / n) * (sumSim / n);
        double cov = sumSimRef / n - (sumSim / n) * (sumRef / n);
        return (varRef > 1e-12 && varSim > 1e-12) ? (cov * cov / (varSim * varRef)) : 0.0;
    };

    metrics.correlationSpeed = calcR2(sumSpeedSim2, sumSpeedRef2, sumSpeedSim, sumSpeedRef, sumSpeedSimRef);
    metrics.correlationLateralG = calcR2(sumLatGSim2, sumLatGRef2, sumLatGSim, sumLatGRef, sumLatGSimRef);
    metrics.correlationLongitudinalG = calcR2(sumLongGSim2, sumLongGRef2, sumLongGSim, sumLongGRef, sumLongGSimRef);
    metrics.correlationRPM = calcR2(sumRpmSim2, sumRpmRef2, sumRpmSim, sumRpmRef, sumRpmSimRef);

    return metrics;
}

}
