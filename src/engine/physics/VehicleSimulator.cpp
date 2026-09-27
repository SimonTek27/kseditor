#include "VehicleSimulator.h"
#include <cstdio>
#include <filesystem>

namespace fs = std::filesystem;

namespace ks {
namespace physics {

VehicleSimulator::VehicleSimulator() {
    m_state.mass = static_cast<float>(m_mass);
    m_state.position = {0, 0.35f, 0};
}

void VehicleSimulator::startSimulation() { m_running = true; }
void VehicleSimulator::stopSimulation() { m_running = false; }

void VehicleSimulator::reset() {
    m_state = SimulationState{};
    m_state.mass = static_cast<float>(m_mass);
    m_state.position = {0, 0.35f, 0};
    m_throttle = m_brake = m_steering = 0;
    m_currentGear = 1;
    m_rpm = 1000;
}

void VehicleSimulator::setThrottle(double v) { m_throttle = std::clamp(v, 0.0, 1.0); }
void VehicleSimulator::setBrake(double v) { m_brake = std::clamp(v, 0.0, 1.0); }
void VehicleSimulator::setSteering(double v) { m_steering = std::clamp(v, -1.0, 1.0); }

void VehicleSimulator::setMass(double kg) {
    m_mass = std::max(200.0, kg);
    m_state.mass = static_cast<float>(m_mass);
}
void VehicleSimulator::setEnginePower(double kw) { m_enginePowerKw = std::max(10.0, kw); }
void VehicleSimulator::setMaxRpm(double rpm) { m_maxRpm = std::max(3000.0, rpm); }
void VehicleSimulator::setDragCoeff(double cd) { m_cd = std::max(0.1, cd); }
void VehicleSimulator::setFrontalArea(double m2) { m_frontalArea = std::max(0.5, m2); }
void VehicleSimulator::setWheelBase(double m) { m_wheelBase = std::max(1.5, m); }
void VehicleSimulator::setTrackWidth(double m) { m_trackWidth = std::max(1.0, m); }

std::map<std::string, std::string> VehicleSimulator::parseIni(const std::string& path) {
    std::map<std::string, std::string> out;
    std::ifstream in(path);
    if (!in) return out;
    std::string line, section;
    while (std::getline(in, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        if (line.empty() || line[0] == ';' || line[0] == '#' || line[0] == '/') continue;
        if (line.front() == '[' && line.back() == ']') {
            section = line.substr(1, line.size() - 2);
            continue;
        }
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        auto trim = [](std::string& s) {
            while (!s.empty() && s.front() == ' ') s.erase(s.begin());
            while (!s.empty() && s.back() == ' ') s.pop_back();
        };
        trim(key); trim(val);
        auto sc = val.find(';');
        if (sc != std::string::npos) val = val.substr(0, sc);
        trim(val);
        if (!section.empty()) out[section + "/" + key] = val;
        out[key] = val;
    }
    return out;
}

float VehicleSimulator::getf(const std::map<std::string, std::string>& m, const std::string& k, float def) {
    auto it = m.find(k);
    if (it == m.end()) return def;
    try { return std::stof(it->second); } catch (...) { return def; }
}

void VehicleSimulator::loadVehicleParams(const std::string& carPath) {
    auto tryLoad = [&](const char* name, void (VehicleSimulator::*fn)(const std::string&)) {
        fs::path p1 = fs::path(carPath) / "data" / name;
        fs::path p2 = fs::path(carPath) / name;
        if (fs::exists(p1)) (this->*fn)(p1.string());
        else if (fs::exists(p2)) (this->*fn)(p2.string());
    };
    tryLoad("engine.ini", &VehicleSimulator::loadEngineFromIni);
    tryLoad("tyres.ini", &VehicleSimulator::loadTyresFromIni);
    tryLoad("tires.ini", &VehicleSimulator::loadTyresFromIni);
    tryLoad("drivetrain.ini", &VehicleSimulator::loadDrivetrainFromIni);
    tryLoad("aero.ini", &VehicleSimulator::loadAeroFromIni);
    tryLoad("suspension.ini", &VehicleSimulator::loadSuspensionFromIni);
}

void VehicleSimulator::loadEngineFromIni(const std::string& path) {
    auto m = parseIni(path);
    float power = getf(m, "POWER", getf(m, "ENGINE/POWER", static_cast<float>(m_enginePowerKw)));
    float rpm = getf(m, "LIMITER", getf(m, "ENGINE/LIMITER", static_cast<float>(m_maxRpm)));
    if (power > 1.f) m_enginePowerKw = power;
    if (rpm > 1000.f) m_maxRpm = rpm;
    std::fprintf(stderr, "VehicleSimulator: engine.ini power=%.0f kW limiter=%.0f\n", m_enginePowerKw, m_maxRpm);
}

void VehicleSimulator::loadTyresFromIni(const std::string& path) {
    auto m = parseIni(path);
    float r = getf(m, "RADIUS", getf(m, "FRONT/RADIUS", static_cast<float>(m_wheelRadius)));
    if (r > 0.1f) m_wheelRadius = r;
    std::fprintf(stderr, "VehicleSimulator: tyres.ini radius=%.3f\n", m_wheelRadius);
}

void VehicleSimulator::loadDrivetrainFromIni(const std::string& path) {
    auto m = parseIni(path);
    float fd = getf(m, "FINAL_GEAR_RATIO", getf(m, "GEARS/FINAL", static_cast<float>(m_finalDrive)));
    if (fd > 0.5f) m_finalDrive = fd;
    std::vector<double> gears;
    for (int i = 1; i <= 8; ++i) {
        std::string k = "GEAR_" + std::to_string(i);
        float g = getf(m, k, getf(m, "GEARS/" + k, -1.f));
        if (g > 0.1f) gears.push_back(g);
    }
    if (!gears.empty()) m_gearRatios = std::move(gears);
    std::fprintf(stderr, "VehicleSimulator: drivetrain final=%.2f gears=%zu\n", m_finalDrive, m_gearRatios.size());
}

void VehicleSimulator::loadAeroFromIni(const std::string& path) {
    auto m = parseIni(path);
    float cd = getf(m, "CD", getf(m, "AERO/CD", static_cast<float>(m_cd)));
    float area = getf(m, "FRONTAL_AREA", getf(m, "AERO/FRONTAL_AREA", static_cast<float>(m_frontalArea)));
    if (cd > 0.05f) m_cd = cd;
    if (area > 0.5f) m_frontalArea = area;
    std::fprintf(stderr, "VehicleSimulator: aero Cd=%.3f area=%.2f\n", m_cd, m_frontalArea);
}

void VehicleSimulator::loadSuspensionFromIni(const std::string& path) {
    auto m = parseIni(path);
    float wb = getf(m, "WHEELBASE", getf(m, "BASIC/WHEELBASE", static_cast<float>(m_wheelBase)));
    float tw = getf(m, "TRACK", getf(m, "BASIC/TRACK", static_cast<float>(m_trackWidth)));
    if (wb > 1.0f) m_wheelBase = wb;
    if (tw > 0.8f) m_trackWidth = tw;
    std::fprintf(stderr, "VehicleSimulator: suspension wb=%.2f track=%.2f\n", m_wheelBase, m_trackWidth);
}

void VehicleSimulator::shiftGears() {
    if (m_gearRatios.empty()) return;
    if (m_rpm > m_maxRpm * 0.95 && m_currentGear < static_cast<int>(m_gearRatios.size()))
        m_currentGear++;
    else if (m_rpm < 2000 && m_currentGear > 1 && m_throttle < 0.3)
        m_currentGear--;
    m_currentGear = std::clamp(m_currentGear, 1, static_cast<int>(m_gearRatios.size()));
}

void VehicleSimulator::integrate(double dt) {
    const float fdt = static_cast<float>(std::clamp(dt, 1e-4, 0.05));
    float speed = m_state.speed;
    if (speed < 0.1f && m_throttle < 0.01 && m_brake < 0.01) {
        m_state.velocity = {0, 0, 0};
        m_state.speed = 0;
        m_rpm = 1000;
        return;
    }

    float gearRatio = static_cast<float>(m_gearRatios[static_cast<size_t>(m_currentGear - 1)]);
    float wheelOmega = speed / static_cast<float>(m_wheelRadius);
    m_rpm = std::max(800.0, wheelOmega * gearRatio * m_finalDrive * 60.0 / (2.0 * 3.14159265));
    shiftGears();
    gearRatio = static_cast<float>(m_gearRatios[static_cast<size_t>(m_currentGear - 1)]);

    float peakTorque = static_cast<float>(m_enginePowerKw * 1000.0 / (m_maxRpm * 2.0 * 3.14159265 / 60.0));
    float rpmN = static_cast<float>(m_rpm / m_maxRpm);
    float torqueCurve = std::sin(std::min(rpmN, 1.0f) * 3.14159265f);
    float engineTorque = peakTorque * torqueCurve * static_cast<float>(m_throttle);
    float driveForce = (engineTorque * gearRatio * static_cast<float>(m_finalDrive))
                       / static_cast<float>(m_wheelRadius);

    float drag = 0.5f * 1.225f * static_cast<float>(m_cd) * static_cast<float>(m_frontalArea) * speed * speed;
    float rolling = static_cast<float>(m_mass) * 9.81f * 0.015f;
    float brakeForce = static_cast<float>(m_brake) * static_cast<float>(m_mass) * 12.0f;

    float longForce = driveForce - drag - rolling - brakeForce;
    float ax = longForce / static_cast<float>(m_mass);

    float yawRate = static_cast<float>(m_steering) * speed * 0.15f;
    m_state.heading += yawRate * fdt;
    m_state.rotation.y = m_state.heading;

    float c = std::cos(m_state.heading);
    float s = std::sin(m_state.heading);

    m_state.velocity.x += (ax * s) * fdt;
    m_state.velocity.z += (ax * c) * fdt;
    m_state.velocity.x *= (1.0f - 0.5f * fdt);
    m_state.velocity.y = 0;

    m_state.position.x += m_state.velocity.x * fdt;
    m_state.position.z += m_state.velocity.z * fdt;
    m_state.position.y = 0.35f;

    m_state.speed = std::sqrt(m_state.velocity.x * m_state.velocity.x +
                              m_state.velocity.z * m_state.velocity.z);
    m_state.acceleration.x = ax * s;
    m_state.acceleration.z = ax * c;
    m_state.throttle = static_cast<float>(m_throttle);
    m_state.brake = static_cast<float>(m_brake);
    m_state.steering = static_cast<float>(m_steering);
    m_state.gear = m_currentGear;
    m_state.rpm = static_cast<float>(m_rpm);
    m_state.mass = static_cast<float>(m_mass);
}

void VehicleSimulator::updatePhysics(double dt) {
    if (!m_running) return;
    integrate(dt);
}

} // namespace physics
} // namespace ks
