#pragma once
#include <QObject>
#include <QVector3D>
#include <QString>
#include <QVector>
#include <QPointF>
#include <QElapsedTimer>

#include "tire_PacejkaTireModel.h"
#include "eng_EngineModel.h"
#include "mech_SuspensionModel.h"
#include "aero_AeroModel.h"
#include "dt_DifferentialModel.h"
#include "mech_BrakeThermalModel.h"

namespace ks {

struct SimulationState {
    QVector3D position;
    QVector3D velocity;
    QVector3D acceleration;
    double rpm;
    double speed;
    double heading;
    double lapTime;
    double currentLapDistance;
    double steeringAngle;
    double slipAngle;
    double tireLoad;
};

struct TireSlipCurve {
    QString name;
    QString compound;
    QVector<QPointF> slipAngleVsLateralForce;
    QVector<QPointF> slipRatioVsLongitudinalForce;
    double peakSlipAngle;
    double peakSlipRatio;
    double peakLateralMu;
    double peakLongitudinalMu;
    double stiffnessLateral;
    double stiffnessLongitudinal;
};

struct LapTimeEstimate {
    double totalLapTime;
    double sector1Time;
    double sector2Time;
    double sector3Time;
    double topSpeed;
    double avgSpeed;
    double minCornerSpeed;
    double maxLateralG;
    int numGearChanges;
    double fuelConsumption;
    double confidenceLevel;
};

class phys_TireModel {
public:
    phys_TireModel();

    void setSlipCurve(const TireSlipCurve& curve);
    TireSlipCurve slipCurve() const { return m_slipCurve; }

    double calculateLateralForce(double slipAngle, double normalLoad, double frictionCoeff);
    double calculateLongitudinalForce(double slipRatio, double normalLoad, double frictionCoeff);
    double calculateAligningTorque(double slipAngle, double normalLoad);

    double calculatePeakSlipAngle(double normalLoad) const;
    double calculatePeakSlipRatio(double normalLoad) const;

    void setPacejkaCoefficients(double b, double c, double d, double e);
    QVector<QPointF> generateLateralForceCurve(double normalLoad, double frictionCoeff);
    QVector<QPointF> generateLongitudinalForceCurve(double normalLoad, double frictionCoeff);

private:
    double pacejkaFormula(double x, double b, double c, double d, double e) const;
    double m_b, m_c, m_d, m_e;
    TireSlipCurve m_slipCurve;
};

class phys_LapTimer : public QObject {
    Q_OBJECT
public:
    explicit phys_LapTimer(QObject* parent = nullptr);

    void startLap();
    void stopLap();
    void reset();
    void update(double dt, double speed, double distance);

    double currentLapTime() const { return m_currentLapTime; }
    double bestLapTime() const { return m_bestLapTime; }
    double lastLapTime() const { return m_lastLapTime; }
    int lapCount() const { return m_lapCount; }

    void setSectorDistances(double sector1, double sector2, double sector3);
    void recordLateralG(double gForce);
    void recordGearChange();
    double sector1Time() const { return m_sector1Time; }
    double sector2Time() const { return m_sector2Time; }
    double sector3Time() const { return m_sector3Time; }
    int currentSector() const { return m_currentSector; }

    LapTimeEstimate estimateLapTime(const QVector<double>& historicalLapTimes,
                                     double trackLength,
                                     double avgCornerSpeed) const;

signals:
    void lapCompleted(double lapTime, double bestLapTime);
    void sectorCompleted(int sector, double sectorTime);
    void lapTimeUpdated(double currentTime);

private:
    double m_currentLapTime = 0.0;
    double m_bestLapTime = 1e9;
    double m_lastLapTime = 0.0;
    int m_lapCount = 0;
    double m_totalDistance = 0.0;
    double m_sector1Distance = 0.0;
    double m_sector2Distance = 0.0;
    double m_sector3Distance = 0.0;
    double m_sector1Time = 0.0;
    double m_sector2Time = 0.0;
    double m_sector3Time = 0.0;
    int m_currentSector = 1;
    double m_lastSectorDistance = 0.0;

    double m_topSpeedRecorded = 0.0;
    double m_maxLateralGRecorded = 0.0;
    int m_gearChangesRecorded = 0;
};

class phys_Simulator : public QObject {
    Q_OBJECT

public:
    static phys_Simulator* instance();

    void startSimulation();
    void stopSimulation();
    void reset();

    void setThrottle(double value);
    void setBrake(double value);
    void setSteering(double value);

    SimulationState getState() const { return m_state; }
    bool isRunning() const { return m_running; }

    void setTireModel(const TireSlipCurve& curve);
    TireSlipCurve tireModel() const { return m_tireModel; }
    phys_TireModel* tireModelEngine() { return &m_tireEngine; }

    phys_LapTimer* lapTimer() { return &m_lapTimer; }
    LapTimeEstimate estimateLapTime() const;

    void setMass(double kg) { m_mass = kg; }
    void setEnginePower(double kw) { m_enginePowerKw = kw; }
    void setMaxRpm(double rpm) { m_maxRpm = rpm; }
    void setDragCoeff(double cd) { m_cd = cd; }
    void setFrontalArea(double area) { m_frontalArea = area; }
    void setWheelBase(double wb) { m_wheelBase = wb; }
    void setTrackWidth(double tw) { m_trackWidth = tw; }

    double mass() const { return m_mass; }
    double enginePower() const { return m_enginePowerKw; }
    double maxRpm() const { return m_maxRpm; }

    // Sub-model access for configuration
    EngineModel& engineModel() { return m_engineModel; }
    const EngineModel& engineModel() const { return m_engineModel; }
    PacejkaTireModel& pacejkaModel() { return m_pacejkaModel; }
    const PacejkaTireModel& pacejkaModel() const { return m_pacejkaModel; }
    AeroModel& aeroModel() { return m_aeroModel; }
    const AeroModel& aeroModel() const { return m_aeroModel; }
    DifferentialModel& diffModel() { return m_diffModel; }
    const DifferentialModel& diffModel() const { return m_diffModel; }
    SuspensionModelManager& suspensionModel() { return m_suspensionModel; }
    const SuspensionModelManager& suspensionModel() const { return m_suspensionModel; }
    BrakeModelManager& brakeModel() { return m_brakeModel; }
    const BrakeModelManager& brakeModel() const { return m_brakeModel; }

    // ABS/TC configuration
    void setAbsEnabled(bool enabled) { m_absEnabled = enabled; }
    bool absEnabled() const { return m_absEnabled; }
    void setTractionControlEnabled(bool enabled) { m_tcEnabled = enabled; }
    bool tractionControlEnabled() const { return m_tcEnabled; }
    void setAbsThreshold(double slipRatio) { m_absSlipThreshold = slipRatio; }
    void setTcThreshold(double slipRatio) { m_tcSlipThreshold = slipRatio; }
    double absThreshold() const { return m_absSlipThreshold; }
    double tcThreshold() const { return m_tcSlipThreshold; }

    // Brake temperature access
    float getBrakeDiscTemp(int wheel) const;
    float getBrakePadTemp(int wheel) const;
    float getBrakeFade(int wheel) const;

    // Vehicle parameter loading from AC INI files
    void loadVehicleParams(const QString& carPath);
    void loadEngineFromIni(const QString& engineIniPath);
    void loadTyresFromIni(const QString& tyresIniPath);
    void loadDrivetrainFromIni(const QString& drivetrainIniPath);
    void loadAeroFromIni(const QString& aeroIniPath);
    void loadSuspensionFromIni(const QString& suspensionIniPath);

    // Per-wheel state access
    struct WheelState {
        double normalLoad = 0.0;
        double slipAngle = 0.0;
        double slipRatio = 0.0;
        double lateralForce = 0.0;
        double longitudinalForce = 0.0;
        double driveTorque = 0.0;
        double brakeTorque = 0.0;
        double angularVelocity = 0.0;
        double camber = 0.0;
        double temperature = 30.0;
        double pressure = 2.4;
        double wear = 0.0;
    };
    WheelState wheelState(int wheel) const { return m_wheels[wheel]; }

    // Telemetry validation
    struct ValidationMetrics {
        double speedRMSE;           // RMSE of speed error (m/s)
        double lateralGRMSE;        // RMSE of lateral G error
        double longitudinalGRMSE;   // RMSE of longitudinal G error
        double rpmRMSE;             // RMSE of RPM error
        double speedMaxError;       // Max absolute speed error (m/s)
        double lateralGMaxError;    // Max absolute lateral G error
        double longitudinalGMaxError; // Max absolute longitudinal G error
        double rpmMaxError;         // Max absolute RPM error
        double speedPercentRMSE;    // Percentage RMSE of speed (%)
        double lateralGPercentRMSE; // Percentage RMSE of lateral G (%)
        double longitudinalGPercentRMSE; // Percentage RMSE of long G (%)
        double rpmPercentRMSE;      // Percentage RMSE of RPM (%)
        double lapTimeError;        // Absolute lap time error (s)
        double tireTempError[4];    // Per-wheel temperature error (C)
        double correlationSpeed;    // R^2 correlation for speed trace
        double correlationLateralG; // R^2 correlation for lateral G
        double correlationLongitudinalG; // R^2 correlation for longitudinal G
        double correlationRPM;      // R^2 correlation for RPM trace
        int nSamples;               // Number of samples used in validation
    };
    ValidationMetrics validateAgainstTelemetry(const QVector<double>& timestamps,
                                                const QVector<double>& refSpeed,
                                                const QVector<double>& refLateralG,
                                                const QVector<double>& refLongG,
                                                const QVector<double>& refRPM,
                                                const QVector<double>& refThrottle,
                                                const QVector<double>& refBrake,
                                                const QVector<double>& refSteering) const;

signals:
    void stateUpdated(const SimulationState& state);
    void simulationStarted();
    void simulationStopped();
    void tireDataUpdated(double slipAngle, double lateralForce, double slipRatio, double longitudinalForce);

private:
    explicit phys_Simulator(QObject* parent = nullptr);
    static phys_Simulator* s_instance;

    void updatePhysics(double dt);
    void updateTireModel(double dt);
    void updateWeightTransfer(double longitudinalAccel, double lateralAccel);
    void updatePerWheelForces(double dt);
    double calculateSlipAngle(int wheel, double steeringAngle, double speed, double yawRate) const;
    double calculateSlipRatio(int wheel) const;

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

    // Thermal tire model state
    double m_tireTempSurface[4] = {30.0, 30.0, 30.0, 30.0};
    double m_tireTempCarcass[4] = {35.0, 35.0, 35.0, 35.0};
    double m_tireTempCore[4] = {40.0, 40.0, 40.0, 40.0};
    double m_tirePressure[4] = {2.4, 2.4, 2.4, 2.4};
    double m_tireWear[4] = {0.0, 0.0, 0.0, 0.0};
    double m_ambientTemp = 26.0;

    // Lap timer
    phys_LapTimer m_lapTimer;
    QElapsedTimer m_simTimer;
    double m_trackLength = 5000.0;
    double m_lastUpdateTime = 0.0;

    // History
    QVector<double> m_lapTimeHistory;
};

}
