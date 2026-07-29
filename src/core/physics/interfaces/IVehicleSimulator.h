#pragma once

#include <QObject>
#include <QVector3D>
#include <QString>
#include <QVector>
#include <QPointF>
#include <QElapsedTimer>

namespace ks {
namespace physics {

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

struct ValidationMetrics {
    double speedRMSE;
    double lateralGRMSE;
    double longitudinalGRMSE;
    double rpmRMSE;
    double speedMaxError;
    double lateralGMaxError;
    double longitudinalGMaxError;
    double rpmMaxError;
    double speedPercentRMSE;
    double lateralGPercentRMSE;
    double longitudinalGPercentRMSE;
    double rpmPercentRMSE;
    double lapTimeError;
    double tireTempError[4];
    double correlationSpeed;
    double correlationLateralG;
    double correlationLongitudinalG;
    double correlationRPM;
    int nSamples;
};

struct WeatherState {
    double trackWetness = 0.0;
    double rainIntensity = 0.0;
    double ambientTemp = 26.0;
    double trackTemp = 30.0;
    double airDensity = 1.225;
    double windSpeed = 0.0;
    double windDirection = 0.0;
};

struct DamageState {
    double aeroDamage = 0.0;
    double suspensionDamage[4] = {0,0,0,0};
    double engineDamage = 0.0;
    double gearboxDamage = 0.0;
    double bodyDamage = 0.0;
    double tyreDamage[4] = {0,0,0,0};
    bool isEliminated = false;
    double accumulatedImpact = 0.0;
    int collisionCount = 0;
};

enum class DriveLayout { RWD, FWD, AWD };

// ============================================================================
// Lap Timer
// ============================================================================

class phys_LapTimer : public QObject {
    Q_OBJECT
public:
    explicit phys_LapTimer(QObject* parent = nullptr) : QObject(parent) {}
    ~phys_LapTimer() override = default;

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

class IVehicleSimulator : public QObject {
    Q_OBJECT
public:
    explicit IVehicleSimulator(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IVehicleSimulator() = default;

    virtual void startSimulation() = 0;
    virtual void stopSimulation() = 0;
    virtual void reset() = 0;

    virtual void setThrottle(double value) = 0;
    virtual void setBrake(double value) = 0;
    virtual void setSteering(double value) = 0;

    virtual SimulationState getState() const = 0;
    virtual bool isRunning() const = 0;

    virtual void setTireModel(const TireSlipCurve& curve) = 0;
    virtual TireSlipCurve tireModel() const = 0;

    virtual LapTimeEstimate estimateLapTime() const = 0;

    virtual void setMass(double kg) = 0;
    virtual void setEnginePower(double kw) = 0;
    virtual void setMaxRpm(double rpm) = 0;
    virtual void setDragCoeff(double cd) = 0;
    virtual void setFrontalArea(double area) = 0;
    virtual void setWheelBase(double wb) = 0;
    virtual void setTrackWidth(double tw) = 0;

    virtual double mass() const = 0;
    virtual double enginePower() const = 0;
    virtual double maxRpm() const = 0;

    virtual void setAbsEnabled(bool enabled) = 0;
    virtual bool absEnabled() const = 0;
    virtual void setTractionControlEnabled(bool enabled) = 0;
    virtual bool tractionControlEnabled() const = 0;
    virtual void setAbsThreshold(double slipRatio) = 0;
    virtual void setTcThreshold(double slipRatio) = 0;
    virtual double absThreshold() const = 0;
    virtual double tcThreshold() const = 0;

    virtual float getBrakeDiscTemp(int wheel) const = 0;
    virtual float getBrakePadTemp(int wheel) const = 0;
    virtual float getBrakeFade(int wheel) const = 0;

    virtual void setErsEnabled(bool enabled) = 0;
    virtual bool ersEnabled() const = 0;
    virtual void setErsMode(int mode) = 0;
    virtual void activateErsAttackMode() = 0;

    virtual void setDriveLayout(DriveLayout layout) = 0;
    virtual DriveLayout driveLayout() const = 0;
    virtual void setCenterDiffPreload(double nm) = 0;
    virtual double centerDiffPreload() const = 0;
    virtual void setCenterDiffPower(double power) = 0;
    virtual double centerDiffPower() const = 0;
    virtual void setFrontRearTorqueSplit(double frontRatio) = 0;
    virtual float getErsDeployTorque() const = 0;
    virtual float getErsRegenTorque() const = 0;
    virtual float getErsBatterySoc() const = 0;
    virtual float getErsBatteryTemp() const = 0;

    virtual void setDrsEnabled(bool enabled) = 0;
    virtual bool drsEnabled() const = 0;
    virtual void setDrsAutoActivate(bool autoActivate) = 0;
    virtual bool drsAutoActivate() const = 0;
    virtual void setDrsSpeedThreshold(double kmh) = 0;
    virtual double drsSpeedThreshold() const = 0;
    virtual void setDrsZoneStart(double dist) = 0;
    virtual void setDrsZoneEnd(double dist) = 0;
    virtual bool isDrsActive() const = 0;
    virtual double getDrsDragReduction() const = 0;
    virtual void setDrsDragReduction(double factor) = 0;

    virtual const DamageState& damageState() const = 0;
    virtual DamageState& damageState() = 0;
    virtual void applyCollisionDamage(double impactForce) = 0;
    virtual void resetDamage() = 0;
    virtual void enableDamageModel(bool enabled) = 0;
    virtual bool isDamageModelEnabled() const = 0;

    virtual void setWeatherState(const WeatherState& weather) = 0;
    virtual const WeatherState& weatherState() const = 0;
    virtual WeatherState& weatherState() = 0;
    virtual void setTrackWetness(double wetness) = 0;
    virtual void setRainIntensity(double mmh) = 0;
    virtual double getAquaplaningRisk() const = 0;
    virtual double getTrackGripReduction() const = 0;
    virtual void setAirDensity(double density) = 0;

    virtual void setFuelConsumptionEnabled(bool enabled) = 0;
    virtual bool isFuelConsumptionEnabled() const = 0;
    virtual double getFuelKg() const = 0;
    virtual void setFuelKg(double kg) = 0;
    virtual double getFuelCapacity() const = 0;
    virtual void setFuelCapacity(double liters) = 0;
    virtual double getEffectiveMass() const = 0;

    virtual void loadVehicleParams(const QString& carPath) = 0;
    virtual void loadEngineFromIni(const QString& engineIniPath) = 0;
    virtual void loadTyresFromIni(const QString& tyresIniPath) = 0;
    virtual void loadDrivetrainFromIni(const QString& drivetrainIniPath) = 0;
    virtual void loadAeroFromIni(const QString& aeroIniPath) = 0;
    virtual void loadSuspensionFromIni(const QString& suspensionIniPath) = 0;

    virtual WheelState wheelState(int wheel) const = 0;

    virtual ValidationMetrics validateAgainstTelemetry(
        const QVector<double>& timestamps,
        const QVector<double>& refSpeed,
        const QVector<double>& refLateralG,
        const QVector<double>& refLongG,
        const QVector<double>& refRPM,
        const QVector<double>& refThrottle,
        const QVector<double>& refBrake,
        const QVector<double>& refSteering) const = 0;

signals:
    virtual void stateUpdated(const SimulationState& state) = 0;
    virtual void simulationStarted() = 0;
    virtual void simulationStopped() = 0;
    virtual void tireDataUpdated(double slipAngle, double lateralForce, double slipRatio, double longitudinalForce) = 0;
};

} // namespace physics
} // namespace ks