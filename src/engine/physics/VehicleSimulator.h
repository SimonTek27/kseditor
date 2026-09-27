#pragma once

#include "PhysicsCoreTypes.h"
#include "EngineModel.h"
#include "PacejkaTireModel.h"
#include "AeroModel.h"
#include "DifferentialModel.h"
#include "SuspensionModel.h"

#include <string>
#include <vector>
#include <memory>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <map>

namespace ks {
namespace physics {

/** Qt-free vehicle runtime for SimulationLoop / ksengine. */
class VehicleSimulator {
public:
    VehicleSimulator();
    ~VehicleSimulator() = default;

    void startSimulation();
    void stopSimulation();
    void reset();
    bool isRunning() const { return m_running; }

    void updatePhysics(double dt);

    void setThrottle(double v);
    void setBrake(double v);
    void setSteering(double v);

    SimulationState getState() const { return m_state; }
    SimulationState& state() { return m_state; }

    void setMass(double kg);
    void setEnginePower(double kw);
    void setMaxRpm(double rpm);
    void setDragCoeff(double cd);
    void setFrontalArea(double m2);
    void setWheelBase(double m);
    void setTrackWidth(double m);

    void loadVehicleParams(const std::string& carPath);
    void loadEngineFromIni(const std::string& path);
    void loadTyresFromIni(const std::string& path);
    void loadDrivetrainFromIni(const std::string& path);
    void loadAeroFromIni(const std::string& path);
    void loadSuspensionFromIni(const std::string& path);

    int currentGear() const { return m_currentGear; }
    double rpm() const { return m_rpm; }

private:
    static std::map<std::string, std::string> parseIni(const std::string& path);
    static float getf(const std::map<std::string, std::string>& m, const std::string& k, float def);
    void integrate(double dt);
    void shiftGears();

    SimulationState m_state;
    bool m_running = false;
    double m_throttle = 0, m_brake = 0, m_steering = 0;
    double m_mass = 1200, m_enginePowerKw = 260, m_maxRpm = 8500;
    double m_cd = 0.35, m_frontalArea = 2.2, m_wheelBase = 2.6, m_trackWidth = 1.6;
    double m_wheelRadius = 0.33, m_finalDrive = 3.9;
    std::vector<double> m_gearRatios = {3.5, 2.5, 1.8, 1.4, 1.1, 0.9};
    int m_currentGear = 1;
    double m_rpm = 1000;

    EngineModel m_engine;
    PacejkaTireModel m_tires;
    AeroModel m_aero;
    DifferentialModel m_diff;
    SuspensionModel m_suspension;
};

} // namespace physics
} // namespace ks
