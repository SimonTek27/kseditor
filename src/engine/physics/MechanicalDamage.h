#pragma once
/**
 * Continuous mechanical damage / wear on top of DamageSystem impact model.
 *
 * Sources:
 *   - Collision impulses (track, wall, pit, car–car)
 *   - Over-rev, oil/coolant abuse
 *   - Kerbs / bottoming suspension
 *   - Brake temp fade & pad wear
 *   - Clutch slip / gear abuse
 *
 * Outputs are written into DamageSystem component structs each frame.
 */
#include "DamageSystem.h"
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace ks {
namespace physics {

struct MechTelemetry {
    float speedMs = 0.f;
    float rpm = 0.f;
    float maxRpm = 7500.f;
    float throttle = 0.f;
    float brake = 0.f;
    float clutch = 1.f;          // 1 = engaged
    int gear = 0;
    bool isShifting = false;
    float engineTempC = 90.f;
    float oilTempC = 100.f;
    float oilPressureBar = 4.f;
    float coolantTempC = 90.f;
    float brakeTempC[4] = {300.f, 300.f, 300.f, 300.f};
    float suspensionTravel[4] = {};  // 0..1 compression
    float suspensionVel[4] = {};     // m/s vertical
    float verticalG = 1.f;
    float curbLoad[4] = {};          // 0..1 kerb aggressiveness
    bool engineRunning = true;
};

struct MechWearConfig {
    // Engine
    float overRevStart = 1.02f;      // fraction of maxRpm
    float overRevSeize = 1.12f;
    float overRevDamagePerSec = 0.08f;
    float overheatStartC = 110.f;
    float overheatCriticalC = 130.f;
    float overheatDamagePerSec = 0.04f;
    float lowOilPressureBar = 1.5f;
    float lowOilDamagePerSec = 0.06f;
    float misfireFromHealth = 0.55f; // below this health → misfire climbs

    // Transmission
    float clutchSlipDamagePerSec = 0.05f;
    float powerShiftDamage = 0.02f;  // per abusive shift
    float diffAbuseSpeedMs = 40.f;

    // Suspension
    float bottomTravel = 0.92f;
    float bottomDamage = 0.015f;     // per event
    float curbDamageScale = 0.01f;
    float armBreakThreshold = 0.15f; // armStrength below → broken

    // Brakes
    float padWearPerSecAtFull = 0.0008f;
    float fadeStartC = 550.f;
    float fadeCriticalC = 750.f;
    float discDamageAtCritical = 0.02f; // per sec above critical

    // Impact → mechanical
    float impactEngineCoupling = 0.25f;  // front impacts → engine
    float impactSuspCoupling = 0.45f;
    float impactAeroCoupling = 0.35f;
    float impactTransCoupling = 0.15f;
    float seizeEnergy = 80000.f;         // front impact energy to seize chance

    float globalWearScale = 1.f;
};

/**
 * Apply continuous wear for one frame into DamageSystem.
 */
inline void applyMechanicalWear(DamageSystem& dmg, float dt, const MechTelemetry& t,
                                const MechWearConfig& cfg = {}) {
    if (!dmg.config().enabled || dt <= 0.f) return;

    const float s = cfg.globalWearScale * dmg.config().wearMultiplier;
    auto& eng = const_cast<EngineDamageData&>(dmg.engineDamage());
    auto& trans = const_cast<TransmissionDamageData&>(dmg.transmissionDamage());

    // --- Engine continuous ---
    if (t.engineRunning && !eng.isSeized) {
        const float rpmRatio = t.maxRpm > 1.f ? t.rpm / t.maxRpm : 0.f;

        // Over-rev
        if (rpmRatio > cfg.overRevStart) {
            const float severity = std::clamp(
                (rpmRatio - cfg.overRevStart) / std::max(0.01f, cfg.overRevSeize - cfg.overRevStart),
                0.f, 1.f);
            eng.health = std::max(0.f, eng.health - cfg.overRevDamagePerSec * severity * dt * s);
            eng.powerLoss = std::min(0.9f, eng.powerLoss + severity * 0.02f * dt * s);
            if (rpmRatio >= cfg.overRevSeize && severity > 0.85f) {
                eng.isSeized = true;
                eng.health = 0.f;
                if (dmg.onComponentFailed)
                    dmg.onComponentFailed(DamageType::Engine);
            }
        }

        // Thermal
        if (t.coolantTempC > cfg.overheatStartC || t.engineTempC > cfg.overheatStartC) {
            const float peak = std::max(t.coolantTempC, t.engineTempC);
            const float sev = std::clamp(
                (peak - cfg.overheatStartC) / std::max(1.f, cfg.overheatCriticalC - cfg.overheatStartC),
                0.f, 1.f);
            eng.overheating = std::min(1.f, eng.overheating + sev * 0.15f * dt * s);
            eng.health = std::max(0.f, eng.health - cfg.overheatDamagePerSec * sev * dt * s);
            eng.coolantLeak = std::min(1.f, eng.coolantLeak + sev * 0.01f * dt * s);
        } else {
            eng.overheating = std::max(0.f, eng.overheating - 0.05f * dt);
        }

        // Oil
        if (t.oilPressureBar < cfg.lowOilPressureBar && t.rpm > 1500.f) {
            const float sev = 1.f - std::clamp(t.oilPressureBar / cfg.lowOilPressureBar, 0.f, 1.f);
            eng.oilPressureLoss = std::min(1.f, eng.oilPressureLoss + sev * 0.1f * dt * s);
            eng.health = std::max(0.f, eng.health - cfg.lowOilDamagePerSec * sev * dt * s);
        }

        if (eng.health < cfg.misfireFromHealth)
            eng.misfireRate = std::min(1.f, (cfg.misfireFromHealth - eng.health) * 1.5f);
    }

    // --- Transmission ---
    if (!trans.isStuck) {
        // Clutch slip while throttle + partial clutch
        if (t.clutch < 0.85f && t.throttle > 0.3f && t.rpm > 2000.f) {
            const float slip = (1.f - t.clutch) * t.throttle;
            trans.clutchDamage = std::min(1.f, trans.clutchDamage + cfg.clutchSlipDamagePerSec * slip * dt * s);
            trans.health = std::max(0.2f, trans.health - 0.01f * slip * dt * s);
        }
        if (t.isShifting && t.throttle > 0.7f) {
            const int gi = std::clamp(t.gear, 0, 7);
            trans.gearDamage[gi] = std::min(1.f, trans.gearDamage[gi] + cfg.powerShiftDamage * s);
        }
        if (trans.clutchDamage > 0.95f || trans.health < 0.25f) {
            trans.isStuck = true;
            if (dmg.onComponentFailed)
                dmg.onComponentFailed(DamageType::Transmission);
        }
    }

    // --- Suspension (4 corners) ---
    for (int i = 0; i < 4; ++i) {
        auto& sus = const_cast<SuspensionDamageData&>(dmg.suspensionDamage(i));
        if (sus.isBroken) continue;

        if (t.suspensionTravel[i] >= cfg.bottomTravel) {
            sus.springDamage = std::min(1.f, sus.springDamage + cfg.bottomDamage * s);
            sus.dampingLoss = std::min(1.f, sus.dampingLoss + cfg.bottomDamage * 0.5f * s);
            sus.geometry = std::max(0.f, sus.geometry - cfg.bottomDamage * 0.3f * s);
        }
        if (t.curbLoad[i] > 0.05f) {
            const float c = t.curbLoad[i] * cfg.curbDamageScale * dt * s;
            sus.armStrength = std::max(0.f, sus.armStrength - c * 2.f);
            sus.toeDeviation += (i % 2 == 0 ? -1.f : 1.f) * c * 8.f;
            sus.camberDeviation += c * 5.f;
            sus.geometry = std::max(0.f, sus.geometry - c);
        }
        if (sus.armStrength < cfg.armBreakThreshold || sus.geometry < 0.1f) {
            sus.isBroken = true;
            if (dmg.onComponentFailed)
                dmg.onComponentFailed(DamageType::Suspension);
        }
    }

    // --- Brakes ---
    for (int i = 0; i < 4; ++i) {
        auto& br = const_cast<BrakeDamageData&>(dmg.brakeDamage(i));
        br.temperature = t.brakeTempC[i];

        if (t.brake > 0.05f && t.speedMs > 5.f) {
            br.padWear = std::min(1.f, br.padWear + cfg.padWearPerSecAtFull * t.brake * dt * s);
        }
        if (br.temperature > cfg.fadeStartC) {
            const float sev = std::clamp(
                (br.temperature - cfg.fadeStartC) / std::max(1.f, cfg.fadeCriticalC - cfg.fadeStartC),
                0.f, 1.f);
            br.fadeLevel = std::min(1.f, sev);
            br.isFaded = sev > 0.6f;
            if (br.temperature > cfg.fadeCriticalC)
                br.discDamage = std::min(1.f, br.discDamage + cfg.discDamageAtCritical * dt * s);
        } else {
            br.fadeLevel = std::max(0.f, br.fadeLevel - 0.2f * dt);
            br.isFaded = false;
        }
    }

    // Refresh caches via public update path
    dmg.update(0.f, t.speedMs, t.rpm);
}

/**
 * Map a world-space impact into DamageSystem (collision / pit / wall).
 * @param energy  Joules-ish (½ m v² or contact impulse scale)
 * @param localPoint contact in vehicle frame (x right, y up, z forward)
 * @param normal   impact normal in vehicle frame
 */
inline void applyMechanicalImpact(
    DamageSystem& dmg,
    float energy,
    float localX, float localY, float localZ,
    float normalX, float normalY, float normalZ,
    const MechWearConfig& cfg = {})
{
    if (!dmg.config().enabled) return;

    CollisionEvent ev;
    ev.contactPoint = { localX, localY, localZ };
    ev.contactNormal = { normalX, normalY, normalZ };
    ev.impactEnergy = energy;
    ev.impactForce = energy; // placeholder
    ev.damageType = DamageType::BodyPanel;

    // Zone heuristic
    if (localZ > 0.6f) ev.primaryZone = DamageZone::FrontCenter;
    else if (localZ < -0.6f) ev.primaryZone = DamageZone::RearCenter;
    else if (localX > 0.4f) ev.primaryZone = DamageZone::RightSide;
    else if (localX < -0.4f) ev.primaryZone = DamageZone::LeftSide;
    else ev.primaryZone = DamageZone::Underbody;

    if (localY < 0.3f) ev.damageType = ev.damageType | DamageType::Suspension;
    if (localZ > 0.5f && localY > 0.4f) ev.damageType = ev.damageType | DamageType::Aero;
    if (localZ > 0.4f && std::fabs(localX) < 0.5f)
        ev.damageType = ev.damageType | DamageType::Engine;

    dmg.processCollision(ev);

    // Extra mechanical coupling beyond zone distribute
    const float n = std::min(energy / 50000.f, 1.f) * dmg.config().physicsDamageMultiplier;
    auto& eng = const_cast<EngineDamageData&>(dmg.engineDamage());
    auto& aero = const_cast<AeroDamageData&>(dmg.aeroDamage());
    auto& trans = const_cast<TransmissionDamageData&>(dmg.transmissionDamage());

    if (localZ > 0.3f) {
        eng.health = std::max(0.f, eng.health - n * cfg.impactEngineCoupling);
        eng.powerLoss = std::min(0.9f, eng.powerLoss + n * cfg.impactEngineCoupling * 0.5f);
        aero.frontWingDamage = std::min(1.f, aero.frontWingDamage + n * cfg.impactAeroCoupling);
        aero.radiatorDamage = std::min(1.f, aero.radiatorDamage + n * 0.2f);
        if (energy > cfg.seizeEnergy && localZ > 0.8f) {
            eng.isSeized = true;
            eng.health = 0.f;
            if (dmg.onComponentFailed) dmg.onComponentFailed(DamageType::Engine);
        }
    }
    if (localZ < -0.3f)
        aero.rearWingDamage = std::min(1.f, aero.rearWingDamage + n * cfg.impactAeroCoupling);
    if (localY < 0.25f)
        aero.floorDamage = std::min(1.f, aero.floorDamage + n * 0.4f);

    // Corner suspension from lateral/front hits
    int wheel = 0;
    if (localZ > 0.f) wheel = localX >= 0.f ? 1 : 0; // FL=0 FR=1
    else wheel = localX >= 0.f ? 3 : 2;              // RL=2 RR=3
    auto& sus = const_cast<SuspensionDamageData&>(dmg.suspensionDamage(wheel));
    sus.armStrength = std::max(0.f, sus.armStrength - n * cfg.impactSuspCoupling);
    sus.geometry = std::max(0.f, sus.geometry - n * cfg.impactSuspCoupling * 0.7f);
    if (sus.armStrength < 0.15f) sus.isBroken = true;

    trans.health = std::max(0.2f, trans.health - n * cfg.impactTransCoupling);

    dmg.update(0.f, 0.f, 0.f);
}

/** Bridge from pit collision contact (world impulse → approx energy). */
inline void applyPitContactDamage(
    DamageSystem& dmg,
    float impulseNs,
    float massKg,
    float localX, float localZ,
    const MechWearConfig& cfg = {})
{
    // E ≈ J² / (2m)
    const float energy = (impulseNs * impulseNs) / std::max(1.f, 2.f * massKg);
    applyMechanicalImpact(dmg, energy, localX, 0.4f, localZ, -localX, 0.f, -localZ, cfg);
}

/** Aggregated multipliers for VehicleSimulator integration. */
struct MechEffectMultipliers {
    float power = 1.f;
    float handling = 1.f;
    float braking = 1.f;
    float downforce = 1.f;
    float drag = 1.f;
    float fuelUse = 1.f;
    bool engineDead = false;
    bool transStuck = false;
};

inline MechEffectMultipliers sampleMechanicalEffects(const DamageSystem& dmg) {
    MechEffectMultipliers m;
    m.power = dmg.powerMultiplier();
    m.handling = dmg.handlingMultiplier();
    m.braking = dmg.brakingMultiplier();
    m.downforce = dmg.downforceMultiplier();
    m.drag = dmg.dragMultiplier();
    m.fuelUse = 1.f + dmg.engineDamage().fuelConsumptionIncrease();
    m.engineDead = dmg.isEngineFailed();
    m.transStuck = dmg.isTransmissionFailed();
    return m;
}

} // namespace physics
} // namespace ks
