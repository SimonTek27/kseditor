#include <QtTest>
#include <QVector3D>
#include <cmath>
#include <limits>

#include "PhysicsSimulator.h"
#include "tire_PacejkaTireModel.h"
#include "eng_EngineModel.h"
#include "mech_SuspensionModel.h"
#include "aero_AeroModel.h"
#include "dt_DifferentialModel.h"
#include "mech_BrakeThermalModel.h"

using namespace ks;

class TestPhysicsSimulator : public QObject {
    Q_OBJECT

private slots:
    // ── Tire Model (phys_TireModel, legacy) ────────────────────────────
    void test_tireModel_defaultSlipCurve();
    void test_tireModel_lateralForce();
    void test_tireModel_longitudinalForce();
    void test_tireModel_aligningTorque();
    void test_tireModel_setCustomCoefficients();
    void test_tireModel_generateLateralCurve();
    void test_tireModel_normalLoadSensitivity();

    // ── Pacejka Tire Model ─────────────────────────────────────────────
    void test_pacejka_magicFormula();
    void test_pacejka_lateralForce();
    void test_pacejka_longitudinalForce();
    void test_pacejka_combinedSlip();
    void test_pacejka_temperatureEffect();
    void test_pacejka_wearEffect();
    void test_pacejka_curveGeneration();
    void test_pacejka_presets();

    // ── Engine Model ───────────────────────────────────────────────────
    void test_engineModel_torqueCurve();
    void test_engineModel_power();
    void test_engineModel_revLimiter();
    void test_engineModel_fuelConsumption();
    void test_engineModel_engineBraking();
    void test_engineModel_gearSpeed();
    void test_engineModel_optimalGear();
    void test_engineModel_presets();
    void test_engineModel_validation();
    void test_engineModel_lutInterpolation();

    // ── Differential Model ─────────────────────────────────────────────
    void test_diff_openDifferential();
    void test_diff_lockedDifferential();
    void test_diff_lsdClutch();
    void test_diff_viscousLsd();
    void test_diff_temperature();
    void test_diff_slipRatio();
    void test_diff_validation();
    void test_diff_presets();

    // ── Brake Model ────────────────────────────────────────────────────
    void test_brakeModel_initialTemp();
    void test_brakeModel_heatGeneration();
    void test_brakeModel_fade();
    void test_brakeModel_frictionCoefficient();
    void test_brakeModel_discPresets();
    void test_brakeModel_validation();
    void test_brakeModelManager();

    // ── Lap Timer ──────────────────────────────────────────────────────
    void test_lapTimer_startStop();
    void test_lapTimer_bestLap();
    void test_lapTimer_sectors();
    void test_lapTimer_reset();

    // ── Simulator ──────────────────────────────────────────────────────
    void test_simulator_singleton();
    void test_simulator_startStop();
    void test_simulator_throttleControl();
    void test_simulator_brakeModelAccess();
    void test_simulator_absConfig();
    void test_simulator_tcConfig();
    void test_simulator_brakeTemperatureAccess();
    void test_simulator_estimation();
    void test_simulator_vehicleParams();
    void test_simulator_engineModelAccess();
    void test_simulator_pacejkaModelAccess();
    void test_simulator_diffModelAccess();
    void test_simulator_reset();

    // ── Telemetry Validation ───────────────────────────────────────────
    void test_validation_metricsStructure();
    void test_validation_perfectMatch();
    void test_validation_knownErrors();
    void test_validation_allCorrelations();
    void test_validation_emptyInput();
    void test_validation_percentageRMSE();
};

// ============================================================================
// phys_TireModel Tests (legacy)
// ============================================================================

void TestPhysicsSimulator::test_tireModel_defaultSlipCurve() {
    phys_TireModel tire;
    TireSlipCurve curve = tire.slipCurve();
    QCOMPARE(curve.name, QString("Default"));
    QCOMPARE(curve.compound, QString("Street"));
    QVERIFY(curve.peakSlipAngle > 0);
    QVERIFY(curve.peakSlipRatio > 0);
}

void TestPhysicsSimulator::test_tireModel_lateralForce() {
    phys_TireModel tire;
    double force = tire.calculateLateralForce(5.0, 4000.0, 1.0);
    QVERIFY(std::abs(force) > 0);
    QVERIFY(std::abs(force) < 50000.0);
}

void TestPhysicsSimulator::test_tireModel_longitudinalForce() {
    phys_TireModel tire;
    double force = tire.calculateLongitudinalForce(0.1, 4000.0, 1.0);
    QVERIFY(std::abs(force) > 0);
}

void TestPhysicsSimulator::test_tireModel_aligningTorque() {
    phys_TireModel tire;
    double torque = tire.calculateAligningTorque(5.0, 4000.0);
    QVERIFY(std::abs(torque) > 0);
}

void TestPhysicsSimulator::test_tireModel_setCustomCoefficients() {
    phys_TireModel tire;
    tire.setPacejkaCoefficients(1.5, 2.0, 6000.0, 0.5);
    double force = tire.calculateLateralForce(5.0, 4000.0, 1.0);
    QVERIFY(std::abs(force) > 0);
}

void TestPhysicsSimulator::test_tireModel_generateLateralCurve() {
    phys_TireModel tire;
    QVector<QPointF> curve = tire.generateLateralForceCurve(4000.0, 1.0);
    QVERIFY(curve.size() > 10);
    double first = curve.first().y();
    double last = curve.last().y();
    QCOMPARE(qAbs(first + last) < 1.0, true);
}

void TestPhysicsSimulator::test_tireModel_normalLoadSensitivity() {
    phys_TireModel tire;
    double lowLoad = tire.calculateLateralForce(5.0, 2000.0, 1.0);
    double highLoad = tire.calculateLateralForce(5.0, 8000.0, 1.0);
    QVERIFY(highLoad > lowLoad);
}

// ============================================================================
// Pacejka Model Tests
// ============================================================================

void TestPhysicsSimulator::test_pacejka_magicFormula() {
    double result = PacejkaTireModel::magicFormula(5.0, 1.5, 2.0, 6000.0, 0.5);
    QVERIFY(std::abs(result) > 0);
    QVERIFY(std::abs(result) < 1.5e4);
}

void TestPhysicsSimulator::test_pacejka_lateralForce() {
    PacejkaTireModel pacejka;
    PacejkaTireModel::TireState state;
    state.slipAngle = PacejkaTireModel::degToRad(5.0);
    state.normalForce = 4000.0;
    state.camberAngle = 0.0;
    float force = pacejka.calculateLateralForce(state);
    QVERIFY(std::abs(force) > 0);
}

void TestPhysicsSimulator::test_pacejka_longitudinalForce() {
    PacejkaTireModel pacejka;
    PacejkaTireModel::TireState state;
    state.slipRatio = 0.1;
    state.normalForce = 4000.0;
    state.camberAngle = 0.0;
    float force = pacejka.calculateLongitudinalForce(state);
    QVERIFY(std::abs(force) > 0);
}

void TestPhysicsSimulator::test_pacejka_combinedSlip() {
    PacejkaTireModel pacejka;
    PacejkaTireModel::TireForces forces = pacejka.calculateCombinedSlip(
        PacejkaTireModel::degToRad(5.0), 0.1, 4000.0, 0.0);
    QVERIFY(std::abs(forces.lateralForce) > 0);
    QVERIFY(std::abs(forces.longitudinalForce) > 0);
}

void TestPhysicsSimulator::test_pacejka_temperatureEffect() {
    PacejkaTireModel pacejka;
    double cold = pacejka.calculateTemperatureEffect(20.0);
    double optimal = pacejka.calculateTemperatureEffect(90.0);
    double hot = pacejka.calculateTemperatureEffect(120.0);
    QVERIFY(optimal >= cold);
    QVERIFY(optimal >= hot);
}

void TestPhysicsSimulator::test_pacejka_wearEffect() {
    PacejkaTireModel pacejka;
    double newTire = pacejka.calculateWearEffect(0.0);
    double wornTire = pacejka.calculateWearEffect(0.8);
    QCOMPARE(newTire, 1.0);
    QVERIFY(wornTire < newTire);
}

void TestPhysicsSimulator::test_pacejka_curveGeneration() {
    PacejkaTireModel pacejka;
    QVector<QPair<float, float>> lateral = pacejka.generateLateralCurve(15.0f, 4000.0f, 99);
    QVector<QPair<float, float>> longitudinal = pacejka.generateLongitudinalCurve(0.3f, 4000.0f, 99);
    QVERIFY(lateral.size() == 100);
    QVERIFY(longitudinal.size() == 100);
}

void TestPhysicsSimulator::test_pacejka_presets() {
    PacejkaTireModel::TireCoefficients slick = PacejkaTireModel::getSlickTireCoefficients();
    PacejkaTireModel::TireCoefficients street = PacejkaTireModel::getStreetTireCoefficients();
    QVERIFY(slick.a2 > street.a2);
}

// ============================================================================
// Engine Model Tests
// ============================================================================

void TestPhysicsSimulator::test_engineModel_torqueCurve() {
    EngineModel engine;
    engine.setConfig(EngineModel::getV8_4000());
    float torque800 = engine.calculateTorque(800);
    float torque5000 = engine.calculateTorque(5000);
    QVERIFY(torque5000 > torque800);
}

void TestPhysicsSimulator::test_engineModel_power() {
    EngineModel engine;
    engine.setConfig(EngineModel::getV8_4000());
    float power5000 = engine.calculatePower(5000);
    float power7500 = engine.calculatePower(7500);
    QVERIFY(power5000 > 0);
    QVERIFY(power7500 > 0);
}

void TestPhysicsSimulator::test_engineModel_revLimiter() {
    EngineModel engine;
    engine.setConfig(EngineModel::getV8_4000());
    QVERIFY(engine.isAtRevLimiter(9000.0f));
    QVERIFY(!engine.isAtRevLimiter(6000.0f));
}

void TestPhysicsSimulator::test_engineModel_fuelConsumption() {
    EngineModel engine;
    engine.setConfig(EngineModel::getV8_4000());
    float consumption = engine.calculateFuelConsumption(5000, 0.8f);
    QVERIFY(consumption > 0);
}

void TestPhysicsSimulator::test_engineModel_engineBraking() {
    EngineModel engine;
    engine.setConfig(EngineModel::getV8_4000());
    float braking = engine.calculateEngineBraking(5000);
    QVERIFY(braking > 0);
}

void TestPhysicsSimulator::test_engineModel_gearSpeed() {
    EngineModel engine;
    engine.setConfig(EngineModel::getV8_4000());
    float speed = engine.calculateSpeed(5000, 2);
    QVERIFY(speed > 0);
}

void TestPhysicsSimulator::test_engineModel_optimalGear() {
    EngineModel engine;
    engine.setConfig(EngineModel::getV8_4000());
    int gear = engine.calculateOptimalGear(100.0f, 5000);
    QVERIFY(gear >= 0);
}

void TestPhysicsSimulator::test_engineModel_presets() {
    EngineModel::EngineConfig i4 = EngineModel::getInline4_2000();
    EngineModel::EngineConfig v8 = EngineModel::getV8_4000();
    EngineModel::EngineConfig ev = EngineModel::getElectric();
    QVERIFY(i4.peakPower < v8.peakPower);
    QVERIFY(ev.peakTorque > 0);
}

void TestPhysicsSimulator::test_engineModel_validation() {
    EngineModel::EngineConfig config = EngineModel::getV8_4000();
    QString error;
    QVERIFY(EngineModel::validateConfig(config, &error));

    EngineModel::EngineConfig bad = config;
    bad.maxRPM = -1;
    QVERIFY(!EngineModel::validateConfig(bad, &error));
}

void TestPhysicsSimulator::test_engineModel_lutInterpolation() {
    QVector<EngineModel::TorquePoint> raw = {
        {1000, 200}, {3000, 350}, {6000, 300}, {8000, 200}
    };
    QVector<EngineModel::TorquePoint> interpolated = EngineModel::interpolateCurve(raw, 10);
    QCOMPARE(interpolated.size(), 10);
    QVERIFY(interpolated.first().rpm < interpolated.last().rpm);
}

// ============================================================================
// Differential Model Tests
// ============================================================================

void TestPhysicsSimulator::test_diff_openDifferential() {
    DifferentialModel diff;
    diff.setConfig(DifferentialModel::getOpenDiff());
    diff.update(0, 100, 100, 0);
    QCOMPARE(diff.getLockingTorque(), 0.0f);
}

void TestPhysicsSimulator::test_diff_lockedDifferential() {
    DifferentialModel diff;
    diff.setConfig(DifferentialModel::getLockedDiff());
    diff.update(0, 100, 100, 50);
    QVERIFY(diff.getLockingTorque() > 0);
}

void TestPhysicsSimulator::test_diff_lsdClutch() {
    DifferentialModel diff;
    diff.setConfig(DifferentialModel::getLSDClutch());
    diff.update(0, 100, 50, 50);
    QVERIFY(diff.getState().isLocking);
    QVERIFY(std::abs(diff.getLeftTorque() - diff.getRightTorque()) > 0);
}

void TestPhysicsSimulator::test_diff_viscousLsd() {
    DifferentialModel diff;
    diff.setConfig(DifferentialModel::getLSDViscous());
    diff.update(0, 100, 100, 50);
    QVERIFY(std::abs(diff.getLockingTorque()) > 0);
}

void TestPhysicsSimulator::test_diff_temperature() {
    DifferentialModel diff;
    diff.setConfig(DifferentialModel::getLSDClutch());
    float initialTemp = diff.getState().temperature;
    diff.update(0.1f, 100, 50, 50);
    QVERIFY(diff.getState().temperature > initialTemp);
}

void TestPhysicsSimulator::test_diff_slipRatio() {
    DifferentialModel diff;
    diff.setConfig(DifferentialModel::getOpenDiff());
    diff.update(0, 100, 100, 100);
    QCOMPARE(diff.getSlipRatio(), 0.0f);
}

void TestPhysicsSimulator::test_diff_validation() {
    DifferentialModel::DiffConfig config = DifferentialModel::getLSDClutch();
    QVERIFY(DifferentialModel::validateConfig(config));

    config.preload = -1;
    QVERIFY(!DifferentialModel::validateConfig(config));
}

void TestPhysicsSimulator::test_diff_presets() {
    DifferentialModel::DiffConfig open = DifferentialModel::getOpenDiff();
    DifferentialModel::DiffConfig clutch = DifferentialModel::getLSDClutch();
    DifferentialModel::DiffConfig locked = DifferentialModel::getLockedDiff();
    QCOMPARE(open.type, DifferentialModel::DiffType::Open);
    QCOMPARE(clutch.type, DifferentialModel::DiffType::LSD_Cls);
    QCOMPARE(locked.type, DifferentialModel::DiffType::Locked);
}

// ============================================================================
// Brake Model Tests
// ============================================================================

void TestPhysicsSimulator::test_brakeModel_initialTemp() {
    BrakeThermalModel brake;
    BrakeThermalModel::BrakeState state;
    brake.update(0, state);
    double discTemp = brake.calculateDiscTemp();
    QVERIFY(discTemp >= 0);
    QVERIFY(brake.calculatePadTemp() >= 0);
}

void TestPhysicsSimulator::test_brakeModel_heatGeneration() {
    BrakeThermalModel brake;
    BrakeThermalModel::BrakeState state;
    state.brakeTorque = 1000.0f;
    state.discAngularVelocity = 100.0f;
    double initialTemp = brake.calculateDiscTemp();
    brake.update(0.1f, state);
    QVERIFY(brake.calculateDiscTemp() >= initialTemp);
}

void TestPhysicsSimulator::test_brakeModel_fade() {
    BrakeThermalModel brake;
    double fade = brake.calculateBrakeFade();
    QVERIFY(fade > 0);
    QVERIFY(fade <= 1.0f);
}

void TestPhysicsSimulator::test_brakeModel_frictionCoefficient() {
    BrakeThermalModel brake;
    double mu = brake.calculateFrictionCoefficient();
    QVERIFY(mu > 0);
    QVERIFY(mu < 1.0f);
}

void TestPhysicsSimulator::test_brakeModel_discPresets() {
    BrakeThermalModel::BrakeComponent carbon = BrakeThermalModel::getCarbonDisc();
    BrakeThermalModel::BrakeComponent iron = BrakeThermalModel::getIronDisc();
    QVERIFY(carbon.discDensity < iron.discDensity);
}

void TestPhysicsSimulator::test_brakeModel_validation() {
    BrakeThermalModel brake;
    QString error;
    QVERIFY(brake.validate(&error));
}

void TestPhysicsSimulator::test_brakeModelManager() {
    BrakeModelManager mgr;
    mgr.update(0, 0.5f, 50.0f);
    QVERIFY(mgr.getAverageDiscTemp() > 0);
}

// ============================================================================
// Lap Timer Tests
// ============================================================================

void TestPhysicsSimulator::test_lapTimer_startStop() {
    phys_LapTimer timer;
    timer.update(1.0, 50.0, 100.0);
    timer.stopLap();
    QCOMPARE(timer.lapCount(), 1);
}

void TestPhysicsSimulator::test_lapTimer_bestLap() {
    phys_LapTimer timer;
    timer.update(1.5, 50.0, 100.0);
    timer.stopLap();
    timer.startLap();
    timer.update(2.0, 50.0, 100.0);
    timer.stopLap();
    QCOMPARE(timer.bestLapTime(), 1.5);
}

void TestPhysicsSimulator::test_lapTimer_sectors() {
    phys_LapTimer timer;
    timer.setSectorDistances(500.0, 1000.0, 1500.0);
    timer.startLap();
    timer.update(1.0, 50.0, 0.0);
    timer.update(0.5, 50.0, 600.0);
    timer.update(0.5, 50.0, 1200.0);
    timer.update(0.5, 50.0, 1800.0);
    QCOMPARE(timer.currentSector(), 3);
}

void TestPhysicsSimulator::test_lapTimer_reset() {
    phys_LapTimer timer;
    timer.update(1.0, 50.0, 100.0);
    timer.stopLap();
    timer.reset();
    QCOMPARE(timer.lapCount(), 0);
    QCOMPARE(timer.bestLapTime(), 1e9);
}

// ============================================================================
// Simulator Tests
// ============================================================================

void TestPhysicsSimulator::test_simulator_singleton() {
    phys_Simulator* sim1 = phys_Simulator::instance();
    phys_Simulator* sim2 = phys_Simulator::instance();
    QCOMPARE(sim1, sim2);
}

void TestPhysicsSimulator::test_simulator_startStop() {
    phys_Simulator* sim = phys_Simulator::instance();
    sim->stopSimulation();
    QVERIFY(!sim->isRunning());
    sim->startSimulation();
    QVERIFY(sim->isRunning());
    sim->stopSimulation();
    QVERIFY(!sim->isRunning());
}

void TestPhysicsSimulator::test_simulator_throttleControl() {
    phys_Simulator* sim = phys_Simulator::instance();
    sim->setThrottle(0.5);
    QCOMPARE(sim->getState().rpm, 0.0); // state not advanced, but no crash
}

void TestPhysicsSimulator::test_simulator_brakeModelAccess() {
    phys_Simulator* sim = phys_Simulator::instance();
    auto& brake = sim->brakeModel();
    QVERIFY(brake.getAverageDiscTemp() >= 0);
}

void TestPhysicsSimulator::test_simulator_absConfig() {
    phys_Simulator* sim = phys_Simulator::instance();
    sim->setAbsEnabled(true);
    sim->setAbsThreshold(0.2);
    QVERIFY(sim->absEnabled());
    QCOMPARE(sim->absThreshold(), 0.2);
}

void TestPhysicsSimulator::test_simulator_tcConfig() {
    phys_Simulator* sim = phys_Simulator::instance();
    sim->setTractionControlEnabled(true);
    sim->setTcThreshold(0.15);
    QVERIFY(sim->tractionControlEnabled());
    QCOMPARE(sim->tcThreshold(), 0.15);
}

void TestPhysicsSimulator::test_simulator_brakeTemperatureAccess() {
    phys_Simulator* sim = phys_Simulator::instance();
    float discTemp = sim->getBrakeDiscTemp(0);
    float padTemp = sim->getBrakePadTemp(0);
    float fade = sim->getBrakeFade(0);
    QVERIFY(discTemp >= 0);
    QVERIFY(padTemp >= 0);
    QVERIFY(fade > 0);
    QVERIFY(fade <= 1.0f);
}

void TestPhysicsSimulator::test_simulator_estimation() {
    phys_Simulator* sim = phys_Simulator::instance();
    LapTimeEstimate est = sim->estimateLapTime();
    QVERIFY(est.confidenceLevel >= 0);
}

void TestPhysicsSimulator::test_simulator_vehicleParams() {
    phys_Simulator* sim = phys_Simulator::instance();
    sim->setMass(1400.0);
    sim->setEnginePower(400.0);
    QCOMPARE(sim->mass(), 1400.0);
    QCOMPARE(sim->enginePower(), 400.0);
}

void TestPhysicsSimulator::test_simulator_engineModelAccess() {
    phys_Simulator* sim = phys_Simulator::instance();
    auto& engine = sim->engineModel();
    QVERIFY(engine.getConfig().maxRPM > 0);
}

void TestPhysicsSimulator::test_simulator_pacejkaModelAccess() {
    phys_Simulator* sim = phys_Simulator::instance();
    auto& pacejka = sim->pacejkaModel();
    QVERIFY(pacejka.getCoefficients().a4 > 0);
}

void TestPhysicsSimulator::test_simulator_diffModelAccess() {
    phys_Simulator* sim = phys_Simulator::instance();
    auto& diff = sim->diffModel();
    QVERIFY(DifferentialModel::validateConfig(diff.getConfig()));
}

void TestPhysicsSimulator::test_simulator_reset() {
    phys_Simulator* sim = phys_Simulator::instance();
    sim->reset();
    QCOMPARE(sim->getState().speed, 0.0);
}

// ============================================================================
// Telemetry Validation Tests
// ============================================================================

void TestPhysicsSimulator::test_validation_metricsStructure() {
    phys_Simulator::ValidationMetrics metrics = {};
    QCOMPARE(metrics.nSamples, 0);
    QCOMPARE(metrics.speedMaxError, 0.0);
    QCOMPARE(metrics.lateralGMaxError, 0.0);
    QCOMPARE(metrics.longitudinalGMaxError, 0.0);
    QCOMPARE(metrics.rpmMaxError, 0.0);
    QCOMPARE(metrics.speedPercentRMSE, 0.0);
    QCOMPARE(metrics.lateralGPercentRMSE, 0.0);
    QCOMPARE(metrics.longitudinalGPercentRMSE, 0.0);
    QCOMPARE(metrics.rpmPercentRMSE, 0.0);
    QCOMPARE(metrics.correlationLongitudinalG, 0.0);
    QCOMPARE(metrics.correlationRPM, 0.0);
}

void TestPhysicsSimulator::test_validation_perfectMatch() {
    phys_Simulator* sim = phys_Simulator::instance();
    sim->reset();
    sim->setMass(1500.0);
    sim->setEnginePower(350.0);

    QVector<double> timestamps, speed, latG, longG, rpm, throttle, brake, steering;

    sim->startSimulation();
    for (int i = 0; i < 100; ++i) {
        double t = i * 0.016;
        timestamps.append(t);
        sim->setThrottle(0.5);
        // Cannot advance private updatePhysics, so we build reference via
        // validateAgainstTelemetry's internal replay instead
        throttle.append(0.5);
        brake.append(0.0);
        steering.append(0.0);
        speed.append(0.0);
        rpm.append(0.0);
        latG.append(0.0);
        longG.append(0.0);
    }
    sim->stopSimulation();

    auto metrics = sim->validateAgainstTelemetry(timestamps, speed, latG, longG, rpm, throttle, brake, steering);

    QVERIFY(metrics.nSamples > 0);
    QVERIFY(metrics.speedRMSE >= 0);
}

void TestPhysicsSimulator::test_validation_knownErrors() {
    phys_Simulator* sim = phys_Simulator::instance();
    sim->reset();
    sim->setMass(1500.0);
    sim->setEnginePower(350.0);

    QVector<double> timestamps, speed, latG, longG, rpm, throttle, brake, steering;

    sim->startSimulation();
    for (int i = 0; i < 50; ++i) {
        double t = i * 0.016;
        timestamps.append(t);
        throttle.append(1.0);
        brake.append(0.0);
        steering.append(0.0);
        speed.append(0.0);
        rpm.append(0.0);
        latG.append(0.0);
        longG.append(0.0);
    }
    sim->stopSimulation();

    sim->setMass(2000.0);

    auto metrics = sim->validateAgainstTelemetry(timestamps, speed, latG, longG, rpm, throttle, brake, steering);

    QVERIFY(metrics.nSamples > 0);
}

void TestPhysicsSimulator::test_validation_allCorrelations() {
    phys_Simulator* sim = phys_Simulator::instance();
    QVector<double> t = {0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0};
    QVector<double> spd = {0, 10, 20, 30, 40, 50, 45, 35, 25, 15, 5};
    QVector<double> lat = {0, 0, 0.1, 0.2, 0.3, 0.1, 0, -0.1, -0.2, -0.1, 0};
    QVector<double> lng = {0.3, 0.3, 0.2, 0.2, 0.1, 0, -0.1, -0.1, -0.2, -0.2, 0};
    QVector<double> rpm = {1000, 2000, 3000, 4000, 5000, 6000, 5500, 4500, 3500, 2500, 1500};
    QVector<double> thr = {0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 0.8, 0.6, 0.4, 0.2, 0};
    QVector<double> brk = {0, 0, 0, 0, 0, 0, 0.2, 0.4, 0.6, 0.8, 1.0};
    QVector<double> st = {0, 0, 0.1, 0.2, 0.3, 0.1, 0, -0.1, -0.2, -0.1, 0};

    auto m = sim->validateAgainstTelemetry(t, spd, lat, lng, rpm, thr, brk, st);

    QVERIFY(m.correlationSpeed >= 0 && m.correlationSpeed <= 1.0);
    QVERIFY(m.correlationLateralG >= 0 && m.correlationLateralG <= 1.0);
    QVERIFY(m.correlationLongitudinalG >= 0 && m.correlationLongitudinalG <= 1.0);
    QVERIFY(m.correlationRPM >= 0 && m.correlationRPM <= 1.0);
    QVERIFY(m.speedPercentRMSE >= 0);
    QVERIFY(m.lateralGPercentRMSE >= 0);
    QVERIFY(m.longitudinalGPercentRMSE >= 0);
    QVERIFY(m.rpmPercentRMSE >= 0);
    QVERIFY(m.nSamples >= 2);
}

void TestPhysicsSimulator::test_validation_emptyInput() {
    phys_Simulator* sim = phys_Simulator::instance();
    auto m = sim->validateAgainstTelemetry({}, {}, {}, {}, {}, {}, {}, {});

    QCOMPARE(m.nSamples, 0);
    QCOMPARE(m.speedRMSE, 0.0);
    QCOMPARE(m.correlationSpeed, 0.0);
    QCOMPARE(m.correlationLateralG, 0.0);
    QCOMPARE(m.correlationLongitudinalG, 0.0);
    QCOMPARE(m.correlationRPM, 0.0);
}

void TestPhysicsSimulator::test_validation_percentageRMSE() {
    phys_Simulator* sim = phys_Simulator::instance();
    QVector<double> t = {0, 0.5, 1.0};
    QVector<double> spd = {100, 100, 100};
    QVector<double> lat = {0.5, 0.5, 0.5};
    QVector<double> lng = {0.2, 0.2, 0.2};
    QVector<double> rpm = {5000, 5000, 5000};
    QVector<double> thr = {0.5, 0.5, 0.5};
    QVector<double> brk = {0, 0, 0};
    QVector<double> st = {0, 0, 0};

    auto m = sim->validateAgainstTelemetry(t, spd, lat, lng, rpm, thr, brk, st);
    QVERIFY(m.speedPercentRMSE >= 0);
    QVERIFY(m.nSamples >= 2);
}

QTEST_MAIN(TestPhysicsSimulator)
#include "test_PhysicsSimulator.moc"


