#include "ThrustmasterFFB.h"
#include <QDebug>

// ============================================================================
// DirectInput headers (Windows only)
// ============================================================================
#ifdef _WIN32
#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <dinput.h>
#include <objbase.h>

// Link against DirectInput libraries
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "ole32.lib")
#endif

namespace ks::device {

// ============================================================================
// Known Thrustmaster wheel PIDs
// ============================================================================
const std::vector<ThrustmasterFFB::DeviceInfo>& ThrustmasterFFB::knownDevices() {
    static const std::vector<DeviceInfo> devices = {
        {0xB65D, WheelModel::T300,  "T300 RS (PC mode)"},
        {0xB65E, WheelModel::T300,  "T300 RS (PS3 mode)"},
        {0xB662, WheelModel::T300,  "T300 RS (PS4 mode)"},
        {0xB661, WheelModel::T150,  "T150 (PC mode)"},
        {0xB663, WheelModel::T150,  "T150 (PS3 mode)"},
        {0xB688, WheelModel::TSPC,  "TS-PC Racer"},
        {0xB669, WheelModel::TGT,   "T-GT"},
        {0xB66A, WheelModel::TGT,   "T-GT II"},
        {0xB653, WheelModel::T80,   "T80"},
        {0xB655, WheelModel::TMX,   "TMX"},
        {0xB679, WheelModel::T300,  "T300 RS GT Edition"},
        {0xB6A8, WheelModel::T300,  "T300 Ferrari Alcantara"},
    };
    return devices;
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

ThrustmasterFFB::ThrustmasterFFB() = default;

ThrustmasterFFB::~ThrustmasterFFB() {
    shutdown();
}

// ============================================================================
// Initialization
// ============================================================================

bool ThrustmasterFFB::initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);
    qInfo() << "ThrustmasterFFB: Initializing (DirectInput8)";

#ifdef _WIN32
    HRESULT hr;

    // 1. Create DirectInput8 interface
    hr = DirectInput8Create(
        GetModuleHandle(nullptr),
        DIRECTINPUT_VERSION,
        IID_IDirectInput8A,
        reinterpret_cast<void**>(&m_dinput),
        nullptr
    );
    if (FAILED(hr) || !m_dinput) {
        qWarning() << "ThrustmasterFFB: DirectInput8Create failed:" << Qt::hex << hr;
        return false;
    }

    // 2. Find and acquire Thrustmaster wheel
    if (!enumerateDevice()) {
        qWarning() << "ThrustmasterFFB: No Thrustmaster wheel found";
        shutdown();
        return false;
    }

    // 3. Set cooperative level — exclusive access for FFB
    hr = m_device->SetCooperativeLevel(
        GetForegroundWindow(),
        DISCL_EXCLUSIVE | DISCL_FOREGROUND
    );
    if (FAILED(hr)) {
        qWarning() << "ThrustmasterFFB: SetCooperativeLevel failed:" << Qt::hex << hr;
        shutdown();
        return false;
    }

    // 4. Set data format for joystick
    hr = m_device->SetDataFormat(&c_dfDIJoystick2);
    if (FAILED(hr)) {
        qWarning() << "ThrustmasterFFB: SetDataFormat failed:" << Qt::hex << hr;
        shutdown();
        return false;
    }

    // 5. Acquire the device
    hr = m_device->Acquire();
    if (FAILED(hr)) {
        qWarning() << "ThrustmasterFFB: Acquire failed:" << Qt::hex << hr;
        shutdown();
        return false;
    }

    // 6. Create FFB effects
    m_fsbInitialized = createFFBEffect();
    if (!m_fsbInitialized) {
        qWarning() << "ThrustmasterFFB: Failed to create FFB effects";
    }

    m_acquired = true;
    qInfo() << "ThrustmasterFFB: Acquired" << modelName();
    return true;

#else
    qInfo() << "ThrustmasterFFB: DirectInput not available on this platform";
    return false;
#endif
}

// ============================================================================
// Device enumeration
// ============================================================================

bool ThrustmasterFFB::enumerateDevice() {
#ifdef _WIN32
    if (!m_dinput) return false;

    // Store 'this' for the callback
    struct EnumContext {
        ThrustmasterFFB* self;
        bool found;
    } ctx{this, false};

    // Enumerate all joysticks with FFB support
    auto enumCallback = [](const DIDEVICEINSTANCEA* inst, void* ref) -> BOOL {
        auto* ctx = static_cast<EnumContext*>(ref);

        // Check if this is a Thrustmaster device (VID 0x044F)
        if (LOWORD(inst->guidProduct.Data1) != THRUSTMASTER_VID) {
            return DIENUM_CONTINUE; // Continue enumerating
        }

        // Match against known Thrustmaster PIDs
        // Product ID is in the high word of guidProduct.Data1
        uint16_t pid = HIWORD(inst->guidProduct.Data1);
        for (const auto& dev : knownDevices()) {
            if (pid == dev.pid) {
                ctx->self->m_productId = dev.pid;
                ctx->self->m_model = dev.model;
                qInfo() << "ThrustmasterFFB: Found" << dev.name
                        << "(PID:" << Qt::hex << dev.pid << Qt::dec << ")";
                break;
            }
        }

        // Create device interface
        HRESULT hr = ctx->self->m_dinput->CreateDevice(
            inst->guidInstance, &ctx->self->m_device, nullptr
        );
        if (SUCCEEDED(hr) && ctx->self->m_device) {
            ctx->found = true;
            return DIENUM_STOP;
        }

        return DIENUM_CONTINUE;
    };

    HRESULT hr = m_dinput->EnumDevices(
        DI8DEVCLASS_GAMECTRL,
        enumCallback,
        &ctx,
        DIEDFL_ATTACHEDONLY | DIEDFL_FORCEFEEDBACK
    );

    return ctx.found;
#else
    return false;
#endif
}

// ============================================================================
// FFB effect creation
// ============================================================================

bool ThrustmasterFFB::createFFBEffect() {
#ifdef _WIN32
    if (!m_device) return false;

    // Create constant force effect
    HRESULT hr;

    // Allocate DIEFFECT structure
    m_constantEffect = new DIEFFECT;
    memset(m_constantEffect, 0, sizeof(DIEFFECT));
    m_constantEffect->dwSize = sizeof(DIEFFECT);
    m_constantEffect->dwFlags = DIEFF_CARTESIAN | DIEFF_POLAR;
    m_constantEffect->dwDuration = INFINITE;
    m_constantEffect->dwSamplePeriod = 0;
    m_constantEffect->dwGain = DI_FFNOMINALMAX;
    m_constantEffect->dwTriggerButton = DIEB_NOTRIGGER;
    m_constantEffect->cAxes = 1;
    m_constantEffect->rgdwAxes = new DWORD[1];
    m_constantEffect->rgdwAxes[0] = DIJOFS_X;
    m_constantEffect->rglDirection = new LONG[1];
    m_constantEffect->rglDirection[0] = 0;

    // Create the effect
    hr = m_device->CreateEffect(
        GUID_ConstantForce,
        m_constantEffect,
        reinterpret_cast<IDirectInputEffect**>(&m_constantEffectRef),
        nullptr
    );
    if (FAILED(hr)) {
        qWarning() << "ThrustmasterFFB: CreateEffect (Constant) failed:" << Qt::hex << hr;
    }

    // Create damper effect
    m_damperEffect = new DIEFFECT;
    memset(m_damperEffect, 0, sizeof(DIEFFECT));
    m_damperEffect->dwSize = sizeof(DIEFFECT);
    m_damperEffect->dwFlags = DIEFF_CARTESIAN | DIEFF_POLAR;
    m_damperEffect->dwDuration = INFINITE;
    m_damperEffect->dwGain = DI_FFNOMINALMAX;
    m_damperEffect->cAxes = 1;
    m_damperEffect->rgdwAxes = new DWORD[1];
    m_damperEffect->rgdwAxes[0] = DIJOFS_X;
    m_damperEffect->rglDirection = new LONG[1];
    m_damperEffect->rglDirection[0] = 0;

    hr = m_device->CreateEffect(
        GUID_Damper,
        m_damperEffect,
        reinterpret_cast<IDirectInputEffect**>(&m_damperEffectRef),
        nullptr
    );
    if (FAILED(hr)) {
        qWarning() << "ThrustmasterFFB: CreateEffect (Damper) failed:" << Qt::hex << hr;
    }

    return (m_constantEffectRef != nullptr);

#else
    return false;
#endif
}

// ============================================================================
// Update FFB
// ============================================================================

void ThrustmasterFFB::updateFFB(float torqueNm) {
    if (!m_acquired) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastTorque = torqueNm;

    // Map torque (±5 Nm typical for Thrustmaster) to DirectInput range
    // DI_FFNOMINALMAX = 10000
    float maxTorqueNm = 5.0f;
    float normalizedTorque = qBound(-1.0f, torqueNm / maxTorqueNm, 1.0f);
    updateConstantForce(normalizedTorque);
}

void ThrustmasterFFB::setConstantForce(float magnitude) {
    if (!m_acquired) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    updateConstantForce(qBound(-1.0f, magnitude, 1.0f));
}

void ThrustmasterFFB::setSpringForce(float center, float stiffness, float damping) {
    if (!m_acquired) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    // Spring effect: center = 0.0 = center position, stiffness = force gain
    Q_UNUSED(center);
    Q_UNUSED(stiffness);
    Q_UNUSED(damping);
    // TODO: Create and parameterize GUID_Spring effect
}

void ThrustmasterFFB::setDamperForce(float velocity, float coefficient) {
    if (!m_acquired) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    Q_UNUSED(velocity);
    // TODO: Set damper effect gain via IDirectInputEffect::SetParameters
}

void ThrustmasterFFB::setFrictionForce(float coefficient) {
    if (!m_acquired) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    Q_UNUSED(coefficient);
    // TODO: GUID_Friction effect
}

void ThrustmasterFFB::setRumble(float strongMotor, float weakMotor) {
    if (!m_acquired) return;
    Q_UNUSED(strongMotor);
    Q_UNUSED(weakMotor);
    // Thrustmaster wheels with motors — most use single motor for FFB
}

void ThrustmasterFFB::setMotorTemperature(float tempC) {
    Q_UNUSED(tempC);
    // T300 supports temperature compensation — reduces FFB when hot
}

void ThrustmasterFFB::setBoostLevel(float level) {
    Q_UNUSED(level);
    // T-GT / TS-PC boost mode
}

// ============================================================================
// DirectInput effect update
// ============================================================================

void ThrustmasterFFB::updateConstantForce(float forcePercent) {
#ifdef _WIN32
    if (!m_constantEffectRef) return;

    auto* effect = static_cast<IDirectInputEffect*>(m_constantEffectRef);
    if (!effect) return;

    LONG force = static_cast<LONG>(forcePercent * DI_FFNOMINALMAX);

    // Update DIEFFECT parameters
    m_constantEffect->dwFlags = DIEFF_CARTESIAN | DIEFF_POLAR;
    m_constantEffect->cAxes = 1;
    m_constantEffect->rglDirection[0] = (force >= 0) ? 1 : -1;
    m_constantEffect->dwGain = static_cast<DWORD>(std::abs(force));

    HRESULT hr = effect->SetParameters(
        m_constantEffect,
        DIEP_DIRECTION | DIEP_GAIN
    );

    if (hr == DIERR_INPUTLOST) {
        m_device->Acquire();
        effect->SetParameters(m_constantEffect, DIEP_DIRECTION | DIEP_GAIN);
    }

    // Start effect if not already running
    effect->Start(1, 0);
#endif
}

void ThrustmasterFFB::updatePeriodicEffect(float magnitude, float frequency) {
    Q_UNUSED(magnitude);
    Q_UNUSED(frequency);
    // TODO: Sine/square/triangle periodic effect for road rumble
}

// ============================================================================
// Input processing
// ============================================================================

void ThrustmasterFFB::processDIInput() {
#ifdef _WIN32
    if (!m_device) return;

    // Poll device state
    HRESULT hr = m_device->Poll();
    if (FAILED(hr)) {
        // Device lost — try to reacquire
        hr = m_device->Acquire();
        if (FAILED(hr)) return;
        m_device->Poll();
    }
#endif
}

// ============================================================================
// Shutdown
// ============================================================================

void ThrustmasterFFB::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);

#ifdef _WIN32
    // Release FFB effects
    if (m_constantEffectRef) {
        static_cast<IDirectInputEffect*>(m_constantEffectRef)->Release();
        m_constantEffectRef = nullptr;
    }
    if (m_periodicEffectRef) {
        static_cast<IDirectInputEffect*>(m_periodicEffectRef)->Release();
        m_periodicEffectRef = nullptr;
    }
    if (m_damperEffectRef) {
        static_cast<IDirectInputEffect*>(m_damperEffectRef)->Release();
        m_damperEffectRef = nullptr;
    }

    delete[] m_constantEffect->rgdwAxes;
    delete[] m_constantEffect->rglDirection;
    delete m_constantEffect; m_constantEffect = nullptr;
    delete[] m_periodicEffect->rgdwAxes;
    delete[] m_periodicEffect->rglDirection;
    delete m_periodicEffect; m_periodicEffect = nullptr;
    delete[] m_damperEffect->rgdwAxes;
    delete[] m_damperEffect->rglDirection;
    delete m_damperEffect; m_damperEffect = nullptr;

    // Release device
    if (m_device) {
        m_device->Unacquire();
        m_device->Release();
        m_device = nullptr;
    }

    // Release DirectInput
    if (m_dinput) {
        m_dinput->Release();
        m_dinput = nullptr;
    }
#endif

    m_acquired = false;
    m_fsbInitialized = false;
    m_model = WheelModel::Unknown;
    m_productId = 0;
}

// ============================================================================
// Model name
// ============================================================================

QString ThrustmasterFFB::modelName() const {
    for (const auto& dev : knownDevices()) {
        if (dev.pid == m_productId) return QString::fromLatin1(dev.name);
    }
    return "Thrustmaster (Unknown)";
}

} // namespace ks::device
