#pragma once

#include "core/physics/interfaces/IVehicleSimulator.h"
#include "plugins/simulators/kunos/assettocorsa/physics/PacejkaTireModel.h"
#include "plugins/simulators/kunos/assettocorsa/physics/EngineModel.h"
#include "plugins/simulators/kunos/assettocorsa/physics/SuspensionModel.h"
#include "plugins/simulators/kunos/assettocorsa/physics/AeroModel.h"
#include "plugins/simulators/kunos/assettocorsa/physics/DifferentialModel.h"
#include "plugins/simulators/kunos/assettocorsa/physics/BrakeThermalModel.h"
#include "plugins/simulators/kunos/assettocorsa/physics/HybridSystem.h"

namespace ks {
namespace physics {

class ACVehicleSimulator : public QObject, public IVehicleSimulator {
    Q_OBJECT
public:
    explicit ACVehicleSimulator(QObject* parent = nullptr);
    ~ACVehicleSimulator() override = default;

    void startSimulation() override;
    void stopSimulation() override;
    void reset() override;

    void setThrottle(double value) override;
    void setBrake(double value) override;
    void setSteering(double value) override;

    SimulationState getState() const override;
    bool isRunning() const override;

    void setTireModel(const TireSlipCurve& curve) override;
    TireSlipCurve tireModel() const override;

    LapTimeEstimate estimateLapTime() const override;

    void setMass(double kg) override;
    void setEnginePower(double kw) override;
    void setMaxRpm(double rpm) override;
    void setDragCoeff(double cd) override;
    void setFrontalArea(double area) override;
    void setWheelBase(double wb) override;
    void setTrackWidth(double tw) override;

    double mass() const override;
    double enginePower() const override;
    double maxRpm() const override;

    void setAbsEnabled(bool enabled) override;
    bool absEnabled() const override;
    void setTractionControlEnabled(bool enabled) override;
    bool tractionControlEnabled() const override;
    void setAbsThreshold(double slipRatio) override;
    void setTcThreshold(double slipRatio) override;
    double absThreshold() const override;
    double tcThreshold() const override;

    float getBrakeDiscTemp(int wheel) const override;
    float getBrakePadTemp(int wheel) const override;
    float getBrakeFade(int wheel) const override;

    void setErsEnabled(bool enabled) override;
    bool ersEnabled() const override;
    void setErsMode(int mode) override;
    void activateErsAttackMode() override;

    void setDriveLayout(DriveLayout layout) override;
    DriveLayout driveLayout() const override;
    void setCenterDiffPreload(double nm) override;
    double centerDiffPreload() const override;
    void setCenterDiffPower(double power) override;
    double centerDiffPower() const override;
    void setFrontRearTorqueSplit(double frontRatio) override;
    float getErsDeployTorque() const override;
    float getErsRegenTorque() const override;
    float getErsBatterySoc() const override;
    float getErsBatteryTemp() const override;

    void setDrsEnabled(bool enabled) override;
    bool drsEnabled() const override;
    void setDrsAutoActivate(bool autoActivate) override;
    bool drsAutoActivate() const override;
    void setDrsSpeedThreshold(double kmh) override;
    double drsSpeedThreshold() const override;
    void setDrsZoneStart(double dist) override;
    void setDrsZoneEnd(double dist) override;
    bool isDrsActive() const override;
    double getDrsDragReduction() const override;
    void setDrsDragReduction(double factor) override;

    const DamageState& damageState() const override;
    DamageState& damageState() override;
    void applyCollisionDamage(double impactForce) override;
    void resetDamage() override;
    void enableDamageModel(bool enabled) override;
    bool isDamageModelEnabled() const override;

    void setWeatherState(const WeatherState& weather) override;
    const WeatherState& weatherState() const override;
    WeatherState& weatherState() override;
    void setTrackWetness(double wetness) override;
    void setRainIntensity(double mmh) override;
    double getAquaplaningRisk() const override;
    double getTrackGripReduction() const override;
    void setAirDensity(double density) override;

    void setFuelConsumptionEnabled(bool enabled) override;
    bool isFuelConsumptionEnabled() const override;
    double getFuelKg() const override;
    void setFuelKg(double kg) override;
    double getFuelCapacity() const override;
    void setFuelCapacity(double liters) override;
    double getEffectiveMass() const override;

    void loadVehicleParams(const QString& carPath) override;
    void loadEngineFromIni(const QString& engineIniPath) override;
    void loadTyresFromIni(const QString& tyresIniPath) override;
    void loadDrivetrainFromIni(const QString& drivetrainIniPath) override;
    void loadAeroFromIni(const QString& aeroIniPath) override;
    void loadSuspensionFromIni(const QString& suspensionIniPath) override;

    WheelState wheelState(int wheel) const override;

    ValidationMetrics validateAgainstTelemetry(
        const QVector<double>& timestamps,
        const QVector<double>& refSpeed,
        const QVector<double>& refLateralG,
        const QVector<double>& refLongG,
        const QVector<double>& refRPM,
        const QVector<double>& refThrottle,
        const QVector<double>& refBrake,
        const QVector<double>& refSteering) const override;

signals:
    void stateUpdated(const SimulationState& state);
    void simulationStarted();
    void simulationStopped();
    void tireDataUpdated(double slipAngle, double lateralForce, double slipRatio, double longitudinalForce);

private:
    void updatePhysics(double dt);
    void updateTireModel(double dt);
    void updateWeightTransfer(double longitudinalAccel, double lateralAccel);
    void updatePerWheelForces(double dt);
    void updateErsAndDrs(double dt);
    void updateDamageModel(double dt);
    void updateWeatherEffects(double dt);
    void updateFuelWeight(double dt);
    double calculateSlipAngle(int wheel, double steeringAngle, double speed, double yawRate) const;
    double calculateSlipRatio(int wheel) const;

    bool m_running = false;
    SimulationState m_state;

    double m_throttle = 0.0;
    double m_brake = 0.0;
    double m_steering = 0.0;

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
    double m_fuelCapacity = 80.0;

    double m_cgHeight = 0.45;
    double m_frontAxleDist = 1.35;
    double m_rearAxleDist = 1.35;
    double m_rollStiffness = 15000.0;
    double m_steerLock = 22.0;
    double m_steerRatio = 15.0;

    TireSlipCurve m_tireModel;
    phys_TireModel m_tireEngine;

    PacejkaTireModel m_pacejkaModel;
    EngineModel m_engineModel;
    AeroModel m_aeroModel;
    DifferentialModel m_diffModel;
    SuspensionModelManager m_suspensionModel;
    BrakeModelManager m_brakeModel;
    HybridSystem m_hybridSystem;

    bool m_absEnabled = false;
    bool m_tcEnabled = false;
    double m_absSlipThreshold = 0.15;
    double m_tcSlipThreshold = 0.12;

    WheelState m_wheels[4];
    double m_yawRate = 0.0;
    double m_lateralAccel = 0.0;

    double m_tireGraining[4] = {0.0, 0.0, 0.0, 0.0};
    double m_tireBlistering[4] = {0.0, 0.0, 0.0, 0.0};

    double m_tireTempSurface[4] = {30.0, 30.0, 30.0, 30.0};
    double m_tireTempCarcass[4] = {35.0, 35.0, 35.0, 35.0};
    double m_tireTempCore[4] = {40.0, 40.0, 40.0, 40.0};
    double m_tirePressure[4] = {2.4, 2.4, 2.4, 2.4};
    double m_tireWear[4] = {0.0, 0.0, 0.0, 0.0};
    double m_ambientTemp = 26.0;

    double m_ersDeployTorque = 0.0;
    double m_ersRegenTorque = 0.0;

    bool m_drsEnabled = false;
    bool m_drsAutoActivate = false;
    bool m_drsActive = false;
    double m_drsSpeedThreshold = 80.0;
    double m_drsZoneStart = 0.0;
    double m_drsZoneEnd = 0.0;
    double m_drsDragReduction = 0.25;
    double m_drsBaseCd = 0.35;

    bool m_damageEnabled = false;
    DamageState m_damage;

    WeatherState m_weather;
    double m_aquaplaningRisk = 0.0;
    double m_trackGripReduction = 0.0;

    DriveLayout m_driveLayout = DriveLayout::RWD;
    double m_centerDiffPreload = 0.0;
    double m_centerDiffPower = 0.5;
    double m_frontTorqueSplit = 0.0;

    bool m_fuelConsumptionEnabled = true;

    phys_LapTimer m_lapTimer;
    QElapsedTimer m_simTimer;
    double m_trackLength = 5000.0;
    double m_lastUpdateTime = 0.0;

    QVector<double> m_lapTimeHistory;
};

} // namespace physics
} // namespace ks