#include "FFBConditionEffects.h"

#include <cstdio>

#ifdef _WIN32
#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <dinput.h>
#include <objbase.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "ole32.lib")
#endif

#include <algorithm>
#include <cmath>

namespace ks::device {

#ifdef _WIN32
namespace {

// 0.0 .. 1.0 gain -> DirectInput force units (0 .. DI_FFNOMINALMAX)
LONG gainToForce(float gain) {
    return static_cast<LONG>(std::clamp(gain, 0.0f, 1.0f) * DI_FFNOMINALMAX);
}

} // namespace
#endif

// ============================================================================
// Impl — DirectInput state (only meaningful on Windows)
// ============================================================================

struct FFBConditionEffects::Impl {
#ifdef _WIN32
    // One slot per condition effect type. The DIEFFECT keeps pointers into
    // this very struct, so the slot must never be moved after creation.
    struct Effect {
        DIEFFECT params{};
        DWORD axes[1] = {DIJOFS_X};
        LONG direction[1] = {0};
        DICONDITION condition{};
        IDirectInputEffect* ref = nullptr;

        void fillDefaults() {
            params = DIEFFECT{};
            params.dwSize = sizeof(DIEFFECT);
            params.dwFlags = DIEFF_CARTESIAN | DIEFF_POLAR;
            params.dwDuration = INFINITE;
            params.dwSamplePeriod = 0;
            params.dwGain = DI_FFNOMINALMAX;
            params.dwTriggerButton = DIEB_NOTRIGGER;
            params.dwTriggerRepeatInterval = 0;
            params.cAxes = 1;
            params.rgdwAxes = axes;
            params.rglDirection = direction;
            params.lpEnvelope = nullptr;
            params.cbTypeSpecificParams = sizeof(DICONDITION);
            params.lpvTypeSpecificParams = &condition;
            params.dwStartDelay = 0;

            condition.lOffset = 0;
            condition.lPositiveCoefficient = 0;
            condition.lNegativeCoefficient = 0;
            condition.dwPositiveSaturation = DI_FFNOMINALMAX;
            condition.dwNegativeSaturation = DI_FFNOMINALMAX;
            condition.lDeadBand = 0;
        }
    };

    IDirectInputDevice8A* device = nullptr;
    Effect spring;
    Effect damper;
    Effect friction;
    LONG axisHalfRange = 1000;  // DirectInput default joystick axis range
#else
    void* device = nullptr;
#endif
};

#ifdef _WIN32
namespace {

// Create (once) the effect behind a slot and push the current parameters.
// Templated so the private Impl::Effect type never has to be named here.
template <typename EffectSlot>
bool ensureEffect(IDirectInputDevice8A* device, const GUID& guid, EffectSlot& slot, const char* what) {
    if (slot.ref) return true;
    if (!device) return false;

    slot.fillDefaults();

    HRESULT hr = device->CreateEffect(guid, &slot.params, &slot.ref, nullptr);
    if (FAILED(hr) || !slot.ref) {
        std::fprintf(stderr, "FFBConditionEffects: CreateEffect (%s) failed: 0x%08lx\n",
                     what, static_cast<unsigned long>(hr));
        slot.ref = nullptr;
        return false;
    }
    return true;
}

// Push condition coefficients + gain and start the effect.
template <typename EffectSlot>
bool applyEffect(IDirectInputDevice8A* device, EffectSlot& slot) {
    if (!slot.ref) return false;

    HRESULT hr = slot.ref->SetParameters(
        &slot.params, DIEP_TYPESPECIFICPARAMS | DIEP_GAIN);
    if (hr == DIERR_INPUTLOST && device) {
        device->Acquire();
        hr = slot.ref->SetParameters(&slot.params, DIEP_TYPESPECIFICPARAMS | DIEP_GAIN);
    }
    if (FAILED(hr)) {
        std::fprintf(stderr, "FFBConditionEffects: SetParameters failed: 0x%08lx\n",
                     static_cast<unsigned long>(hr));
        return false;
    }

    slot.ref->Start(1, 0);
    return true;
}

} // namespace
#endif

// ============================================================================
// FFBConditionEffects
// ============================================================================

FFBConditionEffects::FFBConditionEffects()
    : m_impl(std::make_unique<Impl>()) {}

FFBConditionEffects::~FFBConditionEffects() {
    release();
}

bool FFBConditionEffects::attach(void* device) {
    release();
    if (!device) return false;

#ifdef _WIN32
    auto* di = static_cast<IDirectInputDevice8A*>(device);
    m_impl->device = di;

    // Cache the X axis range so spring centers can be expressed as a
    // -1..1 fraction instead of hard-coding driver units.
    DIPROPRANGE range{};
    range.diph.dwSize = sizeof(DIPROPRANGE);
    range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    range.diph.dwHow = DIPH_BYOFFSET;
    range.diph.dwObj = DIJOFS_X;
    if (SUCCEEDED(di->GetProperty(DIPROP_RANGE, &range.diph)) && range.lMax > range.lMin) {
        m_impl->axisHalfRange = (range.lMax - range.lMin) / 2;
    }
    return true;
#else
    (void)device;
    return false;
#endif
}

void FFBConditionEffects::release() {
#ifdef _WIN32
    if (!m_impl) return;
    for (Impl::Effect* slot : {&m_impl->spring, &m_impl->damper, &m_impl->friction}) {
        if (slot->ref) {
            slot->ref->Stop();
            slot->ref->Release();
            slot->ref = nullptr;
        }
    }
    m_impl->device = nullptr;
    m_impl->axisHalfRange = 1000;
#endif
}

bool FFBConditionEffects::isBound() const {
#ifdef _WIN32
    return m_impl && m_impl->device != nullptr;
#else
    return false;
#endif
}

bool FFBConditionEffects::setSpring(float center, float stiffness, float damping) {
#ifdef _WIN32
    if (!m_impl || !m_impl->device) return false;
    if (!ensureEffect(m_impl->device, GUID_Spring, m_impl->spring, "Spring")) return false;

    auto& slot = m_impl->spring;
    const LONG offset = static_cast<LONG>(
        std::clamp(center, -1.0f, 1.0f) * static_cast<float>(m_impl->axisHalfRange));
    const LONG coeff = gainToForce(stiffness);

    slot.condition.lOffset = offset;
    slot.condition.lPositiveCoefficient = coeff;
    slot.condition.lNegativeCoefficient = coeff;
    slot.condition.dwPositiveSaturation = static_cast<DWORD>(std::max<LONG>(coeff, 1));
    slot.condition.dwNegativeSaturation = static_cast<DWORD>(std::max<LONG>(coeff, 1));

    // A spring cannot damp, so route the damping term to the damper effect —
    // that is exactly what a self-centring wheel feels like.
    const bool ok = applyEffect(m_impl->device, slot);
    if (damping > 0.0f) setDamper(damping);
    return ok;
#else
    (void)center; (void)stiffness; (void)damping;
    return false;
#endif
}

bool FFBConditionEffects::setDamper(float coefficient) {
#ifdef _WIN32
    if (!m_impl || !m_impl->device) return false;
    if (!ensureEffect(m_impl->device, GUID_Damper, m_impl->damper, "Damper")) return false;

    auto& slot = m_impl->damper;
    const LONG coeff = gainToForce(coefficient);
    slot.condition.lPositiveCoefficient = coeff;
    slot.condition.lNegativeCoefficient = coeff;
    slot.condition.dwPositiveSaturation = DI_FFNOMINALMAX;
    slot.condition.dwNegativeSaturation = DI_FFNOMINALMAX;
    return applyEffect(m_impl->device, slot);
#else
    (void)coefficient;
    return false;
#endif
}

bool FFBConditionEffects::setFriction(float coefficient) {
#ifdef _WIN32
    if (!m_impl || !m_impl->device) return false;
    if (!ensureEffect(m_impl->device, GUID_Friction, m_impl->friction, "Friction")) return false;

    auto& slot = m_impl->friction;
    const LONG coeff = gainToForce(coefficient);
    slot.condition.lPositiveCoefficient = coeff;
    slot.condition.lNegativeCoefficient = coeff;
    slot.condition.dwPositiveSaturation = DI_FFNOMINALMAX;
    slot.condition.dwNegativeSaturation = DI_FFNOMINALMAX;
    return applyEffect(m_impl->device, slot);
#else
    (void)coefficient;
    return false;
#endif
}

} // namespace ks::device
