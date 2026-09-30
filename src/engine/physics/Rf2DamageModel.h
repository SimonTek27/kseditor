#pragma once
/**
 * rFactor 2–style damage model parameters and impulse pipeline for ksim.
 *
 * Inspired by rF2 damage.ini physical section (not a 1:1 file parser):
 *   Engine     — impulse to seize engine
 *   AeroMin    — min impulse (pre-multiplier) to affect aero / verts
 *   AeroDiv    — impulse * AeroDiv → aero damage fraction
 *   Radius*    — visual deformation radius (cosmetic only here)
 *
 * Mechanical effects feed VehicleSimulator multipliers:
 *   power, drag, downforce, braking, suspension geometry (toe/camber bias).
 *
 * Product identity: ksim — no rF2 branding in UI.
 */
#include "DamageSystem.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace ks {
namespace physics {

/** Tunables analogous to rF2 damage.ini [physical] block. */
struct Rf2DamageParams {
    float engineSeizeImpulse = 9500.f;   // Engine=
    float aeroMinImpulse = 800.f;        // AeroMin= (before multiplier)
    float aeroDiv = 2.2e-5f;             // AeroDiv=
    float radiusAdd = 0.6f;              // RadiusAdd=
    float radiusMult = 0.00014f;         // RadiusMult=
    float radiusMax = 1.6f;              // RadiusMax=
    float suspBreakImpulse = 4300.f;     // typical PartDetach for wheel/susp
    float bodyImpulseScale = 1.0f;       // global vs impact energy
    float damageMult = 1.0f;             // HDV-style damage multiplier
};

/**
 * Convert a collision impulse (N·s approximation / rF2 units-ish) into
 * DamageSystem state. Call from contact solver or gameplay collision.
 */
inline void applyRf2Impulse(DamageSystem& dmg, const Rf2DamageParams& p,
                            float impulse, const PhysVec3& contactPoint,
                            const PhysVec3& contactNormal) {
    if (!dmg.config().enabled || impulse <= 0.f) return;

    const float imp = impulse * p.damageMult * p.bodyImpulseScale;

    // --- Body / structural (always if above soft threshold) ---
    CollisionEvent ev;
    ev.contactPoint = contactPoint;
    ev.contactNormal = contactNormal.normalized();
    ev.impactVelocity = contactNormal * (impulse * 0.01f); // rough
    ev.impactEnergy = imp * imp * 0.5f; // E ~ ½ J-like proxy from impulse
    ev.impactForce = imp;
    ev.damageType = DamageType::BodyPanel;
    // Primary zone from contact along local Z (front+) / X (right+)
    if (contactPoint.z > 0.5f) {
        if (contactPoint.x < -0.3f) ev.primaryZone = DamageZone::FrontLeft;
        else if (contactPoint.x > 0.3f) ev.primaryZone = DamageZone::FrontRight;
        else ev.primaryZone = DamageZone::FrontCenter;
    } else if (contactPoint.z < -0.5f) {
        if (contactPoint.x < -0.3f) ev.primaryZone = DamageZone::RearLeft;
        else if (contactPoint.x > 0.3f) ev.primaryZone = DamageZone::RearRight;
        else ev.primaryZone = DamageZone::RearCenter;
    } else {
        ev.primaryZone = contactPoint.x < 0 ? DamageZone::LeftSide : DamageZone::RightSide;
    }
    dmg.processCollision(ev);

    // --- Aero: only if impulse ≥ AeroMin (rF2: computed BEFORE mult for min check) ---
    if (impulse >= p.aeroMinImpulse) {
        const float aeroFrac = std::clamp(imp * p.aeroDiv, 0.f, 1.f);
        // Inject via another process with Aero type + contact
        CollisionEvent aeroEv = ev;
        aeroEv.damageType = DamageType::Aero;
        aeroEv.impactEnergy = aeroFrac * 50000.f; // scale into distributeDamageToZones units
        dmg.processCollision(aeroEv);
    }

    // --- Engine seize ---
    if (imp >= p.engineSeizeImpulse) {
        CollisionEvent engEv = ev;
        engEv.damageType = DamageType::Engine;
        engEv.primaryZone = DamageZone::EngineBay;
        engEv.contactPoint = PhysVec3{0, 0.4f, 1.2f};
        engEv.impactEnergy = 80000.f;
        dmg.processCollision(engEv);
    }

    // --- Suspension break (corner nearest contact) ---
    if (imp >= p.suspBreakImpulse) {
        CollisionEvent sEv = ev;
        sEv.damageType = DamageType::Suspension;
        sEv.impactEnergy = 60000.f;
        dmg.processCollision(sEv);
    }

    // Cosmetic deformation radius (for UI / mesh morph later)
    (void)p.radiusAdd;
    (void)p.radiusMult;
    (void)p.radiusMax;
}

/** Convenience: wall hit at given relative velocity (m/s) and mass (kg). */
inline float impulseFromImpact(float relativeSpeedMs, float massKg, float restitution = 0.15f) {
    // J ≈ m * Δv ; inelastic → (1+e) factor soft
    const float dv = relativeSpeedMs * (1.f + restitution);
    return std::max(0.f, massKg * dv);
}

} // namespace physics
} // namespace ks
