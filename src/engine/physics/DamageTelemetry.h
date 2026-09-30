#pragma once
/**
 * Snapshot damage state for telemetry (SM, UDP, HUD).
 * Qt-free. Maps DamageSystem → flat channels.
 */
#include "DamageSystem.h"
#include <algorithm>

namespace ks {
namespace physics {

struct DamageTelemSample {
    /** 0..1 damage fraction (1 = destroyed). AC-style carDamage[5]: F R L R overall. */
    float carDamage[5] = {};
    float overall = 0.f;       // 0..1
    float structural = 0.f;
    float cosmetic = 0.f;
    float aeroFront = 0.f;
    float aeroRear = 0.f;
    float aeroFloor = 0.f;
    float engineHealth = 1.f;  // 1 = healthy
    float powerMult = 1.f;
    float dragMult = 1.f;
    float downforceMult = 1.f;
    float brakeMult = 1.f;
    float handlingMult = 1.f;
    float suspIntegrity[4] = {1, 1, 1, 1}; // FL FR RL RR
    int collisionCount = 0;
    int warningLevel = 0; // 0 ok, 1 warn, 2 critical
    bool engineSeized = false;
    bool transmissionStuck = false;
};

inline DamageTelemSample sampleDamage(const DamageSystem& d) {
    DamageTelemSample s;
    s.overall = d.overallDamage();
    s.structural = d.structuralDamage();
    s.cosmetic = d.cosmeticDamage();

    const auto& aero = d.aeroDamage();
    s.aeroFront = aero.frontWingDamage;
    s.aeroRear = aero.rearWingDamage;
    s.aeroFloor = aero.floorDamage;

    s.engineHealth = d.engineDamage().health;
    s.engineSeized = d.isEngineFailed();
    s.transmissionStuck = d.isTransmissionFailed();

    s.powerMult = d.powerMultiplier();
    s.dragMult = d.dragMultiplier();
    s.downforceMult = d.downforceMultiplier();
    s.brakeMult = d.brakingMultiplier();
    s.handlingMult = d.handlingMultiplier();

    for (int i = 0; i < 4; ++i)
        s.suspIntegrity[i] = d.suspensionDamage(i).handlingMultiplier();

    s.collisionCount = d.totalCollisions();

    // Zone combined damage for AC carDamage layout:
    // [0] front, [1] rear, [2] left, [3] right, [4] centre/overall
    const float fl = d.zoneData(DamageZone::FrontLeft).combinedDamage();
    const float fc = d.zoneData(DamageZone::FrontCenter).combinedDamage();
    const float fr = d.zoneData(DamageZone::FrontRight).combinedDamage();
    const float rl = d.zoneData(DamageZone::RearLeft).combinedDamage();
    const float rc = d.zoneData(DamageZone::RearCenter).combinedDamage();
    const float rr = d.zoneData(DamageZone::RearRight).combinedDamage();
    const float ls = d.zoneData(DamageZone::LeftSide).combinedDamage();
    const float rs = d.zoneData(DamageZone::RightSide).combinedDamage();

    s.carDamage[0] = std::clamp((fl + fc + fr) / 3.f, 0.f, 1.f);
    s.carDamage[1] = std::clamp((rl + rc + rr) / 3.f, 0.f, 1.f);
    s.carDamage[2] = std::clamp((fl + rl + ls) / 3.f, 0.f, 1.f);
    s.carDamage[3] = std::clamp((fr + rr + rs) / 3.f, 0.f, 1.f);
    s.carDamage[4] = std::clamp(s.overall, 0.f, 1.f);

    if (s.engineSeized || s.overall > 0.8f) s.warningLevel = 2;
    else if (s.overall > 0.3f || s.engineHealth < 0.5f) s.warningLevel = 1;
    else s.warningLevel = 0;
    return s;
}

} // namespace physics
} // namespace ks
