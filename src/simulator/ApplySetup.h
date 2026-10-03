#pragma once
#include "SetupGarage.h"
#include "SetupFile.h"
#include <functional>
#include <string>

namespace ks {
namespace sim {

struct AppliedSetupSnapshot {
    float brakeBias = 0.56f;
    float fuelL = 50.f;
    float ballastKg = 0.f;
    float frontWing = 10.f;
    float rearWing = 12.f;
    float tirePsi[4] = {2.2f, 2.2f, 2.0f, 2.0f};
    int tc = 0;
    int abs = 0;
};

inline AppliedSetupSnapshot snapshotFromSetup(const SetupData& s) {
    AppliedSetupSnapshot a;
    a.brakeBias = s.brakeBias; a.fuelL = s.fuel; a.ballastKg = s.ballast;
    a.frontWing = s.frontWingAngle; a.rearWing = s.rearWingAngle;
    a.tirePsi[0] = s.tirePressureFL; a.tirePsi[1] = s.tirePressureFR;
    a.tirePsi[2] = s.tirePressureRL; a.tirePsi[3] = s.tirePressureRR;
    a.tc = s.tcLevel; a.abs = s.absLevel;
    return a;
}

struct SetupApplyHooks {
    std::function<void(float)> setBrakeBias;
    std::function<void(float)> setFuel;
    std::function<void(float)> setBallast;
    std::function<void(float, float)> setWingAngles;
    std::function<void(const float[4])> setTirePressure;
    std::function<void(int, int)> setAids;
};

inline void applySetup(const SetupData& s, const SetupApplyHooks& hooks) {
    auto a = snapshotFromSetup(s);
    if (hooks.setBrakeBias) hooks.setBrakeBias(a.brakeBias);
    if (hooks.setFuel) hooks.setFuel(a.fuelL);
    if (hooks.setBallast) hooks.setBallast(a.ballastKg);
    if (hooks.setWingAngles) hooks.setWingAngles(a.frontWing, a.rearWing);
    if (hooks.setTirePressure) hooks.setTirePressure(a.tirePsi);
    if (hooks.setAids) hooks.setAids(a.tc, a.abs);
}

inline bool loadAndApplySetup(const std::string& path, SetupData& s, const SetupApplyHooks& hooks) {
    if (!loadSetupFromFile(s, path)) return false;
    applySetup(s, hooks);
    return true;
}

} // namespace sim
} // namespace ks
