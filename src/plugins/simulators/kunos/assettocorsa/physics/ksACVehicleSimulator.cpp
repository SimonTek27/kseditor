#include "ksACVehicleSimulator.h"

namespace ks {
namespace physics {

ksACVehicleSimulator::ksACVehicleSimulator(QObject* parent)
    : IVehicleSimulator(parent)
{
}

void ksACVehicleSimulator::startSimulation() {}
void ksACVehicleSimulator::stopSimulation() {}
void ksACVehicleSimulator::reset() {}
void ksACVehicleSimulator::setThrottle(double value) { m_throttle = value; }
void ksACVehicleSimulator::setBrake(double value) { m_brake = value; }
void ksACVehicleSimulator::setSteering(double value) { m_steering = value; }
SimulationState ksACVehicleSimulator::getState() const { return m_state; }
bool ksACVehicleSimulator::isRunning() const { return m_running; }
double ksACVehicleSimulator::maxRpm() const { return m_maxRpm; }
void ksACVehicleSimulator::setErsEnabled(bool enabled) { Q_UNUSED(enabled); }
void ksACVehicleSimulator::setErsMode(int mode) { Q_UNUSED(mode); }
void ksACVehicleSimulator::activateErsAttackMode() {}
float ksACVehicleSimulator::getErsBatterySoc() const { return 0.0f; }
float ksACVehicleSimulator::getErsBatteryTemp() const { return 0.0f; }
void ksACVehicleSimulator::setDrsEnabled(bool enabled) { m_drsEnabled = enabled; }
void ksACVehicleSimulator::setDrsAutoActivate(bool enabled) { m_drsAutoActivate = enabled; }
bool ksACVehicleSimulator::drsAutoActivate() const { return m_drsAutoActivate; }
void ksACVehicleSimulator::setDrsSpeedThreshold(double threshold) { m_drsSpeedThreshold = threshold; }
double ksACVehicleSimulator::drsSpeedThreshold() const { return m_drsSpeedThreshold; }
bool ksACVehicleSimulator::isDrsActive() const { return m_drsActive; }
double ksACVehicleSimulator::getDrsDragReduction() const { return m_drsDragReduction; }
void ksACVehicleSimulator::setDrsDragReduction(double reduction) { m_drsDragReduction = reduction; }
DamageState& ksACVehicleSimulator::damageState() { return m_damage; }
void ksACVehicleSimulator::resetDamage() { m_damage = DamageState(); }
void ksACVehicleSimulator::enableDamageModel(bool enabled) { m_damageEnabled = enabled; }
WeatherState& ksACVehicleSimulator::weatherState() { return m_weather; }
const WeatherState& ksACVehicleSimulator::weatherState() const { return m_weather; }
void ksACVehicleSimulator::setTrackWetness(double wetness) { m_trackGripReduction = wetness; }
void ksACVehicleSimulator::setRainIntensity(double mmh) { Q_UNUSED(mmh); }
double ksACVehicleSimulator::getAquaplaningRisk() const { return m_aquaplaningRisk; }
double ksACVehicleSimulator::getTrackGripReduction() const { return m_trackGripReduction; }
void ksACVehicleSimulator::setAirDensity(double density) { Q_UNUSED(density); }
void ksACVehicleSimulator::setFuelConsumptionEnabled(bool enabled) { m_fuelConsumptionEnabled = enabled; }
bool ksACVehicleSimulator::isFuelConsumptionEnabled() const { return m_fuelConsumptionEnabled; }
double ksACVehicleSimulator::getFuelKg() const { return m_fuelKg; }
void ksACVehicleSimulator::setFuelKg(double kg) { m_fuelKg = kg; }
double ksACVehicleSimulator::getFuelCapacity() const { return m_fuelCapacity; }
void ksACVehicleSimulator::setFuelCapacity(double liters) { m_fuelCapacity = liters; }
double ksACVehicleSimulator::getEffectiveMass() const { return m_mass; }
void ksACVehicleSimulator::loadVehicleParams(const QString&) {}
void ksACVehicleSimulator::loadEngineFromIni(const QString&) {}
void ksACVehicleSimulator::loadTyresFromIni(const QString&) {}
void ksACVehicleSimulator::loadDrivetrainFromIni(const QString&) {}
void ksACVehicleSimulator::loadAeroFromIni(const QString&) {}
void ksACVehicleSimulator::loadSuspensionFromIni(const QString&) {}
WheelState ksACVehicleSimulator::wheelState(int wheel) const { return m_wheels[qBound(0, wheel, 3)]; }
ValidationMetrics ksACVehicleSimulator::validateAgainstTelemetry(
    const QVector<double>&, const QVector<double>&, const QVector<double>&,
    const QVector<double>&, const QVector<double>&, const QVector<double>&,
    const QVector<double>&, const QVector<double>&) const { return {}; }

void ACVehicleSimulator::setTireModel(const TireSlipCurve& curve) { Q_UNUSED(curve); }
TireSlipCurve ACVehicleSimulator::tireModel() const { return {}; }
LapTimeEstimate ACVehicleSimulator::estimateLapTime() const { return {}; }
void ACVehicleSimulator::setMass(double kg) { m_mass = kg; }
void ACVehicleSimulator::setEnginePower(double kw) { m_enginePowerKw = kw; }
void ACVehicleSimulator::setMaxRpm(double rpm) { m_maxRpm = rpm; }
void ACVehicleSimulator::setDragCoeff(double cd) { m_cd = cd; }
void ACVehicleSimulator::setFrontalArea(double area) { m_frontalArea = area; }
void ACVehicleSimulator::setWheelBase(double wb) { m_wheelBase = wb; }
void ACVehicleSimulator::setTrackWidth(double tw) { m_trackWidth = tw; }
double ACVehicleSimulator::mass() const { return m_mass; }
double ACVehicleSimulator::enginePower() const { return m_enginePowerKw; }
void ACVehicleSimulator::setAbsEnabled(bool enabled) { m_absEnabled = enabled; }
bool ACVehicleSimulator::absEnabled() const { return m_absEnabled; }
void ACVehicleSimulator::setTractionControlEnabled(bool enabled) { Q_UNUSED(enabled); }
bool ACVehicleSimulator::tractionControlEnabled() const { return false; }
void ACVehicleSimulator::setAbsThreshold(double slipRatio) { m_absSlipThreshold = slipRatio; }
void ACVehicleSimulator::setTcThreshold(double slipRatio) { m_tcSlipThreshold = slipRatio; }
double ACVehicleSimulator::absThreshold() const { return m_absSlipThreshold; }
double ACVehicleSimulator::tcThreshold() const { return m_tcSlipThreshold; }
float ACVehicleSimulator::getBrakeDiscTemp(int wheel) const { Q_UNUSED(wheel); return 0.0f; }
float ACVehicleSimulator::getBrakePadTemp(int wheel) const { Q_UNUSED(wheel); return 0.0f; }
float ACVehicleSimulator::getBrakeFade(int wheel) const { Q_UNUSED(wheel); return 0.0f; }
bool ACVehicleSimulator::ersEnabled() const { return false; }
void ACVehicleSimulator::setDriveLayout(DriveLayout layout) { m_driveLayout = layout; }
DriveLayout ACVehicleSimulator::driveLayout() const { return m_driveLayout; }
void ACVehicleSimulator::setCenterDiffPreload(double nm) { m_centerDiffPreload = nm; }
double ACVehicleSimulator::centerDiffPreload() const { return m_centerDiffPreload; }
void ACVehicleSimulator::setCenterDiffPower(double power) { m_centerDiffPower = power; }
double ACVehicleSimulator::centerDiffPower() const { return m_centerDiffPower; }
void ACVehicleSimulator::setFrontRearTorqueSplit(double frontRatio) { m_frontTorqueSplit = frontRatio; }
float ACVehicleSimulator::getErsDeployTorque() const { return static_cast<float>(m_ersDeployTorque); }
float ACVehicleSimulator::getErsRegenTorque() const { return static_cast<float>(m_ersRegenTorque); }
bool ACVehicleSimulator::drsEnabled() const { return m_drsEnabled; }
void ACVehicleSimulator::setDrsZoneStart(double dist) { m_drsZoneStart = dist; }
void ACVehicleSimulator::setDrsZoneEnd(double dist) { m_drsZoneEnd = dist; }
const DamageState& ACVehicleSimulator::damageState() const { return m_damage; }
void ACVehicleSimulator::applyCollisionDamage(double impactForce) { Q_UNUSED(impactForce); }
bool ACVehicleSimulator::isDamageModelEnabled() const { return m_damageEnabled; }
void ACVehicleSimulator::setWeatherState(const WeatherState& weather) { m_weather = weather; }

} // namespace physics
} // namespace ks

#include "ACVehicleSimulator.moc"
