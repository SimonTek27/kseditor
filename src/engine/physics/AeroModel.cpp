#include "AeroModel.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <cctype>

namespace ks {
namespace physics {

AeroModel::AeroForces AeroModel::calculate(const AeroState& state) const {
    AeroForces forces;
    float q = calculateDynamicPressure(state.speed, state.airDensity);

    float totalDownforce = 0;
    float totalDrag = 0;
    float frontDownforce = 0;
    float rearDownforce = 0;

    for (const Wing& wing : m_config.wings) {
        float aoa = wing.angle + state.pitchAngle * 180.0f / 3.14159f;
        float cl = interpolateCl(wing, aoa) * wing.clGain;
        float cd = interpolateCd(wing, aoa) * wing.cdGain;
        float height = (wing.position[2] > 0) ? state.rideHeightFront : state.rideHeightRear;
        cl *= interpolateHeightCl(wing, height);
        cd *= interpolateHeightCd(wing, height);
        // Default LUT empty -> use gain as Cl/Cd proxy
        if (wing.aoaClLut.empty()) cl = 1.2f * wing.clGain * (wing.angle + 2.0f) / 10.0f;
        if (wing.aoaCdLut.empty()) cd = 0.05f + 0.02f * wing.cdGain * std::abs(wing.angle);

        float wingDf = q * wing.area() * cl;
        float wingDrag = q * wing.area() * cd;
        totalDownforce += wingDf;
        totalDrag += wingDrag;
        if (wing.position[2] > 0) frontDownforce += wingDf;
        else rearDownforce += wingDf;
    }

    float bodyDf = q * m_config.frontalArea * m_config.liftCoefficient;
    float bodyDrag = q * m_config.frontalArea * m_config.dragCoefficient;
    totalDownforce += bodyDf;
    totalDrag += bodyDrag;

    float groundEffect = 1.0f;
    float avgRideHeight = (state.rideHeightFront + state.rideHeightRear) / 2.0f;
    if (avgRideHeight < 0.1f) {
        groundEffect = 1.0f + m_config.groundEffectFactor * (0.1f - avgRideHeight) / 0.1f;
    }
    totalDownforce *= groundEffect;
    frontDownforce *= groundEffect;
    rearDownforce *= groundEffect;

    forces.downforce = totalDownforce;
    forces.drag = totalDrag;
    forces.frontDownforce = frontDownforce;
    forces.rearDownforce = rearDownforce;
    float totalDf = frontDownforce + rearDownforce;
    forces.aeroBalance = (totalDf > 0) ? frontDownforce / totalDf : 0.5f;
    forces.ldRatio = (totalDrag > 0) ? totalDownforce / totalDrag : 0;

    forces.drag *= state.draftDragScale;
    forces.downforce *= state.draftDownforceScale;
    forces.frontDownforce *= state.draftDownforceScale;
    forces.rearDownforce *= state.draftDownforceScale;
    return forces;
}

float AeroModel::calculateWingForce(const Wing& wing, const AeroState& state) const {
    float q = calculateDynamicPressure(state.speed, state.airDensity);
    float aoa = wing.angle + state.pitchAngle * 180.0f / 3.14159f;
    float cl = interpolateCl(wing, aoa) * wing.clGain;
    float height = (wing.position[2] > 0) ? state.rideHeightFront : state.rideHeightRear;
    cl *= interpolateHeightCl(wing, height);
    if (wing.aoaClLut.empty()) cl = 1.2f * wing.clGain * (wing.angle + 2.0f) / 10.0f;
    return q * wing.area() * cl;
}

float AeroModel::interpolateCl(const Wing& wing, float aoa) const { return interpolateLut(wing.aoaClLut, aoa); }
float AeroModel::interpolateCd(const Wing& wing, float aoa) const { return interpolateLut(wing.aoaCdLut, aoa); }
float AeroModel::interpolateHeightCl(const Wing& wing, float height) const { return interpolateLut(wing.heightClLut, height); }
float AeroModel::interpolateHeightCd(const Wing& wing, float height) const { return interpolateLut(wing.heightCdLut, height); }

float AeroModel::interpolateLut(const std::vector<std::pair<float, float>>& lut, float x) const {
    if (lut.empty()) return 1.0f;
    for (size_t i = 0; i + 1 < lut.size(); ++i) {
        if (x >= lut[i].first && x <= lut[i + 1].first) {
            float t = (x - lut[i].first) / (lut[i + 1].first - lut[i].first + 1e-6f);
            return lut[i].second + (lut[i + 1].second - lut[i].second) * t;
        }
    }
    if (x < lut.front().first) return lut.front().second;
    return lut.back().second;
}

void AeroModel::addWing(const Wing& wing) { m_config.wings.push_back(wing); }
void AeroModel::removeWing(int index) {
    if (index >= 0 && index < static_cast<int>(m_config.wings.size()))
        m_config.wings.erase(m_config.wings.begin() + index);
}
void AeroModel::clearWings() { m_config.wings.clear(); }

AeroModel::AeroConfig AeroModel::getSedanConfig() {
    AeroConfig config;
    config.frontalArea = 2.2f; config.dragCoefficient = 0.32f; config.liftCoefficient = 0.05f;
    Wing frontWing; frontWing.name = "Front Splitter"; frontWing.chord = 0.3f; frontWing.span = 1.6f;
    frontWing.angle = 2.0f; frontWing.position[2] = 2.3f; frontWing.clGain = 0.3f; frontWing.cdGain = 0.5f;
    Wing rearWing; rearWing.name = "Rear Wing"; rearWing.chord = 0.2f; rearWing.span = 1.2f;
    rearWing.angle = 8.0f; rearWing.position[2] = -1.8f; rearWing.clGain = 0.5f; rearWing.cdGain = 0.6f;
    config.wings.push_back(frontWing); config.wings.push_back(rearWing);
    return config;
}

AeroModel::AeroConfig AeroModel::getGT3Config() {
    AeroConfig config;
    config.frontalArea = 2.0f; config.dragCoefficient = 0.35f; config.liftCoefficient = -0.1f;
    Wing frontWing; frontWing.name = "Front Splitter"; frontWing.chord = 0.4f; frontWing.span = 1.8f;
    frontWing.angle = 3.0f; frontWing.position[2] = 2.4f; frontWing.clGain = 0.6f; frontWing.cdGain = 0.7f;
    Wing rearWing; rearWing.name = "Rear Wing"; rearWing.chord = 0.25f; rearWing.span = 1.4f;
    rearWing.angle = 12.0f; rearWing.position[2] = -1.9f; rearWing.clGain = 0.8f; rearWing.cdGain = 0.8f;
    Wing diffuser; diffuser.name = "Diffuser"; diffuser.chord = 0.5f; diffuser.span = 1.2f;
    diffuser.angle = 10.0f; diffuser.position[2] = -2.0f; diffuser.position[1] = -0.2f;
    diffuser.clGain = 1.0f; diffuser.cdGain = 0.3f;
    config.wings.push_back(frontWing); config.wings.push_back(rearWing); config.wings.push_back(diffuser);
    return config;
}

AeroModel::AeroConfig AeroModel::getFormulaConfig() {
    AeroConfig config;
    config.frontalArea = 1.8f; config.dragCoefficient = 0.30f; config.liftCoefficient = -0.2f;
    Wing frontWing; frontWing.name = "Front Wing"; frontWing.chord = 0.35f; frontWing.span = 1.8f;
    frontWing.angle = 4.0f; frontWing.position[2] = 3.0f; frontWing.clGain = 1.0f; frontWing.cdGain = 0.8f;
    Wing rearWing; rearWing.name = "Rear Wing"; rearWing.chord = 0.3f; rearWing.span = 1.0f;
    rearWing.angle = 15.0f; rearWing.position[2] = -2.5f; rearWing.clGain = 1.2f; rearWing.cdGain = 1.0f;
    Wing floor; floor.name = "Floor"; floor.chord = 3.0f; floor.span = 1.8f;
    floor.position[1] = -0.3f; floor.clGain = 2.0f; floor.cdGain = 0.1f;
    config.wings.push_back(frontWing); config.wings.push_back(rearWing); config.wings.push_back(floor);
    return config;
}

AeroModel::AeroConfig AeroModel::getRoadCarConfig() {
    AeroConfig config;
    config.frontalArea = 2.3f; config.dragCoefficient = 0.30f; config.liftCoefficient = 0.1f;
    return config;
}

AeroModel::AeroConfig AeroModel::loadFromIni(const std::string& iniPath) {
    AeroConfig config = getRoadCarConfig();
    std::ifstream file(iniPath);
    if (!file) return config;
    std::string line, currentSection;
    Wing currentWing;
    bool inWing = false;
    auto trim = [](std::string s) {
        while (!s.empty() && (s.back() == '\r' || s.back() == ' ' || s.back() == '\t')) s.pop_back();
        size_t i = 0; while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) ++i;
        return s.substr(i);
    };
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;
        if (line.front() == '[' && line.back() == ']') {
            if (inWing && !currentWing.name.empty()) config.wings.push_back(currentWing);
            currentSection = line.substr(1, line.size() - 2);
            inWing = (currentSection.find("WING") != std::string::npos || currentSection.find("AERO") != std::string::npos);
            if (inWing) { currentWing = Wing{}; currentWing.name = currentSection; }
            continue;
        }
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = trim(line.substr(0, eq));
        std::string value = trim(line.substr(eq + 1));
        for (char& c : key) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        if (currentSection == "AERO" || currentSection == "DATA") {
            if (key == "FRONTAL_AREA") config.frontalArea = std::stof(value);
            else if (key == "DRAG_COEFFICIENT" || key == "CD") config.dragCoefficient = std::stof(value);
            else if (key == "LIFT_COEFFICIENT" || key == "CL") config.liftCoefficient = std::stof(value);
            else if (key == "GROUND_EFFECT") config.groundEffectFactor = std::stof(value);
        } else if (inWing) {
            if (key == "CHORD") currentWing.chord = std::stof(value);
            else if (key == "SPAN") currentWing.span = std::stof(value);
            else if (key == "ANGLE") currentWing.angle = std::stof(value);
            else if (key == "CL_GAIN") currentWing.clGain = std::stof(value);
            else if (key == "CD_GAIN") currentWing.cdGain = std::stof(value);
            else if (key == "POSITION") {
                std::stringstream ss(value); std::string part; int i = 0;
                while (std::getline(ss, part, ',') && i < 3)
                    currentWing.position[i++] = std::stof(trim(part));
            }
        }
    }
    if (inWing && !currentWing.name.empty()) config.wings.push_back(currentWing);
    return config;
}

bool AeroModel::saveToIni(const AeroConfig& config, const std::string& iniPath) {
    std::ofstream file(iniPath);
    if (!file) return false;
    file << "[AERO]\n";
    file << "FRONTAL_AREA=" << config.frontalArea << "\n";
    file << "DRAG_COEFFICIENT=" << config.dragCoefficient << "\n";
    file << "LIFT_COEFFICIENT=" << config.liftCoefficient << "\n";
    file << "GROUND_EFFECT=" << config.groundEffectFactor << "\n\n";
    for (size_t i = 0; i < config.wings.size(); ++i) {
        const auto& wing = config.wings[i];
        file << "[WING_" << i << "]\n";
        file << "NAME=" << wing.name << "\n";
        file << "CHORD=" << wing.chord << "\n";
        file << "SPAN=" << wing.span << "\n";
        file << "ANGLE=" << wing.angle << "\n";
        file << "CL_GAIN=" << wing.clGain << "\n";
        file << "CD_GAIN=" << wing.cdGain << "\n";
        file << "POSITION=" << wing.position[0] << "," << wing.position[1] << "," << wing.position[2] << "\n\n";
    }
    return true;
}

bool AeroModel::validateConfig(const AeroConfig& config, std::string* error) {
    if (config.frontalArea <= 0 || config.frontalArea > 5.0f) {
        if (error) *error = "Frontal area out of range (0-5 m^2)";
        return false;
    }
    if (config.dragCoefficient < 0 || config.dragCoefficient > 1.0f) {
        if (error) *error = "Drag coefficient out of range (0-1)";
        return false;
    }
    for (const Wing& wing : config.wings) {
        if (wing.chord <= 0 || wing.span <= 0) {
            if (error) *error = "Wing dimensions must be positive";
            return false;
        }
    }
    return true;
}

float AeroModel::calculateDynamicPressure(float speed, float airDensity) {
    return 0.5f * airDensity * speed * speed;
}
float AeroModel::calculateReynoldsNumber(float speed, float chord, float viscosity) {
    return speed * chord / viscosity;
}

AeroModelManager::AeroModelManager() {}

void AeroModelManager::loadFromIni(const std::string& carPath) {
    std::string iniPath = carPath + "/data/aero.ini";
    m_model = AeroModel();
    AeroModel::AeroConfig config = AeroModel::loadFromIni(iniPath);
    m_model.setConfig(config);
}

void AeroModelManager::saveToIni(const std::string& carPath) const {
    AeroModel::saveToIni(m_model.getConfig(), carPath + "/data/aero.ini");
}

AeroModel::AeroForces AeroModelManager::calculateForces(
    float speed, float rideHeightFront, float rideHeightRear, float airDensity) const {
    AeroModel::AeroState state;
    state.speed = speed;
    state.rideHeightFront = rideHeightFront;
    state.rideHeightRear = rideHeightRear;
    state.airDensity = airDensity;
    return m_model.calculate(state);
}

float AeroModelManager::calculateTopSpeed(float enginePower, float /*weight*/) const {
    auto cfg = m_model.getConfig();
    float CdA = cfg.dragCoefficient * cfg.frontalArea;
    if (CdA > 0) {
        float V = std::pow(2.0f * enginePower / (1.225f * CdA), 1.0f / 3.0f);
        return V * 3.6f;
    }
    return 200.0f;
}

float AeroModelManager::calculateCorneringForce(float speed, float cornerRadius) const {
    float v = speed / 3.6f;
    return m_weight * v * v / cornerRadius;
}

float AeroModelManager::calculateBrakeDistance(float speed, float friction) const {
    float v = speed / 3.6f;
    return v * v / (2.0f * friction * 9.81f);
}

std::map<std::string, std::pair<float, float>> AeroModelManager::compareAero(const AeroModelManager& other) const {
    std::map<std::string, std::pair<float, float>> comparison;
    AeroModel::AeroState state; state.speed = 50.0f;
    auto f1 = m_model.calculate(state);
    auto f2 = other.m_model.calculate(state);
    comparison["Downforce"] = {f1.downforce, f2.downforce};
    comparison["Drag"] = {f1.drag, f2.drag};
    comparison["L/D Ratio"] = {f1.ldRatio, f2.ldRatio};
    comparison["Aero Balance"] = {f1.aeroBalance, f2.aeroBalance};
    return comparison;
}

AeroModel::AeroForces AeroModelManager::calculateIntegrated(
    float speed, float rideHeightFront, float rideHeightRear,
    const PhysVec3& egoPos, const PhysVec3& egoForward,
    const PhysVec3* leaderPos, float airDensity) const {
    AeroModel::AeroState state;
    state.speed = speed;
    state.rideHeightFront = rideHeightFront;
    state.rideHeightRear = rideHeightRear;
    state.airDensity = airDensity;
    if (leaderPos) {
        DraftState draft = computeDraft(egoPos, egoForward, *leaderPos);
        state.draftDragScale = 1.0f - draft.dragReduction;
        state.draftDownforceScale = 1.0f - draft.downforceLoss;
    }
    return m_model.calculate(state);
}

} // namespace physics
} // namespace ks
