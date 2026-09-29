#pragma once
#include <memory>

namespace ks::device {

// ============================================================================
// FFBConditionEffects — DirectInput condition effects (spring / damper /
// friction) shared by the DirectInput wheel backends.
// ============================================================================
// Condition effects are evaluated by the driver itself: it samples the wheel
// position/velocity and only needs the DICONDITION coefficients (rest
// offset, stiffness, saturation, dead band) pushed whenever they change,
// instead of a per-frame force. Effects are created lazily on first use and
// must be released before the owning device is destroyed.
//
// attach()/release() take IDirectInputDevice8A* as void* so this header
// stays free of DirectInput declarations (which are Windows-only).
// ============================================================================

class FFBConditionEffects {
public:
    FFBConditionEffects();
    ~FFBConditionEffects();

    FFBConditionEffects(const FFBConditionEffects&) = delete;
    FFBConditionEffects& operator=(const FFBConditionEffects&) = delete;

    // Bind to the wheel device (non-owning). Releases any effect left over
    // from a previous device. Returns false without a device.
    bool attach(void* device);

    // Release every effect created through this object. Safe to call twice.
    void release();

    bool isBound() const;

    // center: rest position of the spring, -1.0 (full left) .. 1.0 (full right)
    // stiffness / damping: 0.0 .. 1.0 gains. Returns false when no effect
    // could be created (device gone, driver refused the effect type).
    bool setSpring(float center, float stiffness, float damping);

    // coefficient: 0.0 .. 1.0 damper gain.
    bool setDamper(float coefficient);

    // coefficient: 0.0 .. 1.0 friction gain.
    bool setFriction(float coefficient);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace ks::device
