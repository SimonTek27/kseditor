#pragma once
/** Bridge: VehicleUpgradeSystem → VehicleSimulator. */
#include "VehicleUpgradeSystem.h"
#include "engine/physics/VehicleSimulator.h"

namespace ks {
namespace vehicle {

struct BaselineVehicleParams {
    double massKg = 1200;
    double powerKw = 260;
    double maxRpm = 8500;
    double cd = 0.35;
    double frontalArea = 2.2;
};

/**
 * Apply selected upgrades on top of a stored baseline (not stacking on already-modified state).
 * Call after loadVehicleParams; keep baseline from stock car.
 */
inline void applyUpgradesToVehicleSimulator(ks::physics::VehicleSimulator& veh,
                                            const BaselineVehicleParams& base,
                                            const PhysicsMods& m) {
    veh.setMass(base.massKg + m.massAddKg);
    const double power = (base.powerKw + m.powerKwAdd) * static_cast<double>(m.powerMult);
    veh.setEnginePower(power);
    veh.setMaxRpm(base.maxRpm + m.maxRpmAdd);
    veh.setDragCoeff((base.cd + m.cdAdd) * static_cast<double>(m.cdMult));
    veh.setFrontalArea(base.frontalArea + m.frontalAreaAdd);

    // Soft aero DF via AeroModel if available
    auto& aero = veh.aero();
    auto cfg = aero.getConfig();
    cfg.liftCoefficient = (cfg.liftCoefficient + m.clAdd) * m.clMult;
    // wing mults folded into lift as approximation
    const float wing = (m.frontWingDfMult + m.rearWingDfMult + m.diffuserDfMult) / 3.f;
    cfg.liftCoefficient *= wing;
    aero.setConfig(cfg);

    (void)m.brakeForceMult;
    (void)m.gripMult;
    (void)m.finalDriveMult;
    (void)m.fuelUseMult;
}

inline void applyUpgradesToVehicleSimulator(ks::physics::VehicleSimulator& veh,
                                            const BaselineVehicleParams& base,
                                            const VehicleUpgradeSystem& ups) {
    applyUpgradesToVehicleSimulator(veh, base, ups.appliedMods());
}

} // namespace vehicle
} // namespace ks
