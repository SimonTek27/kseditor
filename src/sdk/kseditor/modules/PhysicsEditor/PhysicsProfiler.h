#pragma once

/**
 * @file PhysicsProfiler.h
 * @brief Backward-compatibility wrapper for the unified PhysicsProfiler
 *
 * This module version now delegates to the core ks::physics::PhysicsProfiler.
 * New code should include "engine/physics/PhysicsProfiler.h" directly.
 */

#include "engine/physics/PhysicsProfiler.h"

namespace ks {

// Backward-compatibility alias: ks::PhysicsProfiler -> ks::physics::PhysicsProfiler
using PhysicsProfiler = ks::physics::PhysicsProfiler;
using ScopedProfile = ks::physics::ScopedProfile;

} // namespace ks
