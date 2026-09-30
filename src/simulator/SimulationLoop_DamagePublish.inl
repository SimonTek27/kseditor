// Merge into SimulationLoop publish paths — sampleDamage on SM / UDP / TCP / HUD
// Requires: #include "engine/physics/DamageTelemetry.h"

// --- inside publishSharedMemory after live.trackSplineLength = ... ---
#if 0
    {
        const auto dmg = ks::physics::sampleDamage(m_vehicle->damage());
        for (int i = 0; i < 5; ++i) live.carDamage[i] = dmg.carDamage[i];
        live.damageOverall = dmg.overall;
        live.engineHealth = dmg.engineHealth;
        live.powerMult = dmg.powerMult;
        live.dragMult = dmg.dragMult;
        live.downforceMult = dmg.downforceMult;
        live.damageWarning = dmg.warningLevel;
        live.engineSeized = dmg.engineSeized;
    }
#endif

// --- inside publishUdpTelemetry / publishTcpTelemetry after s.pitLimiter = ... ---
#if 0
    {
        const auto dmg = ks::physics::sampleDamage(m_vehicle->damage());
        s.damageOverall = dmg.overall;
        s.engineHealth = dmg.engineHealth;
        s.powerMult = dmg.powerMult;
        s.dragMult = dmg.dragMult;
        s.downforceMult = dmg.downforceMult;
        for (int i = 0; i < 5; ++i) s.carDamage[i] = dmg.carDamage[i];
        for (int i = 0; i < 4; ++i) s.suspIntegrity[i] = dmg.suspIntegrity[i];
        s.damageWarning = dmg.warningLevel;
        s.engineSeized = dmg.engineSeized;
    }
#endif

// --- inside syncUiFromVehicle before m_ui.pushRaceSample(s) ---
#if 0
    {
        const auto dmg = ks::physics::sampleDamage(m_vehicle->damage());
        s.damageOverall = dmg.overall;
        s.engineHealth = dmg.engineHealth;
        s.powerMult = dmg.powerMult;
        s.damageWarning = dmg.warningLevel;
        s.engineSeized = dmg.engineSeized;
    }
#endif
