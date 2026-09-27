#pragma once

/**
 * @file AeroSimulator.h
 * @brief Vehicle-facing aero step: AeroModel + ground effect + draft — Qt-free
 */

#include "AeroModel.h"
#include "AeroDraft.h"
#include "PhysicsCoreTypes.h"

namespace ks {
namespace physics {

class AeroSimulator {
public:
    AeroSimulator() = default;

    void setConfigPreset(const std::string& preset);
    void loadFromCarPath(const std::string& carPath);

    AeroModelManager& manager() { return m_mgr; }
    const AeroModelManager& manager() const { return m_mgr; }

    struct Input {
        float speed = 0.0f;
        float rideHeightFront = 0.05f;
        float rideHeightRear = 0.07f;
        float airDensity = Constants::DEFAULT_AIR_DENSITY;
        PhysVec3 position;
        PhysVec3 forward{0, 0, 1};
        const PhysVec3* leaderPosition = nullptr;
    };

    struct Output {
        AeroModel::AeroForces forces;
        PhysVec3 forceWorld;
        float frontLoadN = 0.0f;
        float rearLoadN = 0.0f;
    };

    Output step(const Input& in) const;

    static void applyToBodyForces(PhysVec3& forceAccum, PhysVec3& torqueAccum,
                                  const Output& out, const PhysVec3& cog,
                                  float wheelbase);

private:
    AeroModelManager m_mgr;
};

} // namespace physics
} // namespace ks
