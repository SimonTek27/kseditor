#include "LogitechFFB.h"
#include <QDebug>

// ============================================================================
// DirectInput headers (Windows only)
// ============================================================================
#ifdef _WIN32
#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <dinput.h>
#include <objbase.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "ole32.lib")
#endif

namespace ks::device {

// ============================================================================
// Known Logitech wheel PIDs
// ============================================================================
const std::vector<LogitechFFB::DeviceInfo>& LogitechFFB::knownDevices() {
    static const std::vector<DeviceInfo> devices = {
        {0xC29B, WheelModel::G27,            "G27"},
        {0xC298, WheelModel::MOMO,           "MOMO Racing"},
        {0xC294, WheelModel::DrivingForceGT, "Driving Force GT"},
        {0xC260, WheelModel::G29,            "G29 (PS3/PS4)"},
        {0xC261, WheelModel::G29,            "G29 (PC)"},
        {0xC262, WheelModel::G920,           "G920"},
        {0xC266, WheelModel::GPro,           "G Pro"},
        {0xC267, WheelModel::GPro,           "G Pro (PC)"},
    };
    return devices;
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

LogitechFFB::LogitechFFB() = default;

LogitechFFB::~LogitechFFB() {
    shutdown();
}

// ============================================================================
// Initialization
// ============================================================================

bool LogitechFFB::initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);
    qInfo() << "LogitechFFB: Initializing (DirectInput8)";

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
        qWarning() << "LogitechFFB: DirectInput8Create failed:" << Qt::hex << hr;
        return false;
    }

    // 2. Find and acquire Logitech wheel
    if (!enumerateDevice()) {
        qWarning() << "LogitechFFB: No Logitech wheel found";
        shutdown();
        return false;
    }

    // 3. Set cooperative level — exclusive for FFB
    hr = m_device->SetCooperativeLevel(
        GetForegroundWindow(),
        DISCL_EXCLUSIVE | DISCL_FOREGROUND
    );
    if (FAILED(hr)) {
        qWarning() << "LogitechFFB: SetCooperativeLevel failed:" << Qt::hex << hr;
        shutdown();
        return false;
    }

    // 4. Set data format for joystick
    hr = m_device->SetDataFormat(&c_dfDIJoystick2);
    if (FAILED(hr)) {
        qWarning() << "LogitechFFB: SetDataFormat failed:" << Qt::hex << hr;
        shutdown();
        return false;
    }

    // 5. Acquire the device
    hr = m_device->Acquire();
    if (FAILED(hr)) {
        qWarning() << "LogitechFFB: Acquire failed:" << Qt::hex << hr;
        shutdown();
        return false;
    }

    // 6. Create FFB effects
    m_fsbInitialized = createFFBEffect();
    if (!m_fsbInitialized) {
        qWarning() << "LogitechFFB: Failed to create FFB effects";
    }

    m_acquired = true;
    qInfo() << "LogitechFFB: Acquired" << modelName();
    return true;

#else
    qInfo() << "LogitechFFB: DirectInput not available on this platform";
    return false;
#endif
}

// ============================================================================
// Device enumeration
// ============================================================================

bool LogitechFFB::enumerateDevice() {
#ifdef _WIN32
    if (!m_dinput) return false;

    struct EnumContext {
        LogitechFFB* self;
        bool found;
    } ctx{this, false};

    auto enumCallback = [](const DIDEVICEINSTANCEA* inst, void* ref) -> BOOL {
        auto* ctx = static_cast<EnumContext*>(ref);

        // Check if this is a Logitech device (VID 0x046D)
        if (LOWORD(inst->guidProduct.Data1) != LOGITECH_VID) {
            return DIENUM_CONTINUE;
        }

        // Match against known Logitech PIDs
        uint16_t pid = HIWORD(inst->guidProduct.Data1);
        for (const auto& dev : knownDevices()) {
            if (pid == dev.pid) {
                ctx->self->m_productId = dev.pid;
                ctx->self->m_model = dev.model;
                qInfo() << "LogitechFFB: Found" << dev.name
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

bool LogitechFFB::createFFBEffect() {
#ifdef _WIN32
    if (!m_device) return false;

    HRESULT hr;

    // Create constant force effect
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

    hr = m_device->CreateEffect(
        GUID_ConstantForce,
        m_constantEffect,
        reinterpret_cast<IDirectInputEffect**>(&m_constantEffectRef),
        nullptr
    );
    if (FAILED(hr)) {
        qWarning() << "LogitechFFB: CreateEffect (Constant) failed:" << Qt::hex << hr;
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
        qWarning() << "LogitechFFB: CreateEffect (Damper) failed:" << Qt::hex << hr;
    }

    return (m_constantEffectRef != nullptr);

#else
    return false;
#endif
}

// ============================================================================
// Update FFB
// ============================================================================

void LogitechFFB::updateFFB(float torqueNm) {
    if (!m_acquired) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastTorque = torqueNm;

    // Logitech G27/G29/G920: max ~2.5 Nm
    // G Pro: max ~5 Nm
    float maxTorqueNm = (m_model == WheelModel::GPro) ? 5.0f : 2.5f;
    float normalizedTorque = qBound(-1.0f, torqueNm / maxTorqueNm, 1.0f);
    updateConstantForce(normalizedTorque);
}

void LogitechFFB::setConstantForce(float magnitude) {
    if (!m_acquired) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    updateConstantForce(qBound(-1.0f, magnitude, 1.0f));
}

void LogitechFFB::setSpringForce(float center, float stiffness, float damping) {
    if (!m_acquired) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    Q_UNUSED(center);
    Q_UNUSED(stiffness);
    Q_UNUSED(damping);
    // TODO: GUID_Spring effect
}

void LogitechFFB::setDamperForce(float velocity, float coefficient) {
    if (!m_acquired) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    Q_UNUSED(velocity);
    // TODO: Damper effect gain update
}

void LogitechFFB::setFrictionForce(float coefficient) {
    if (!m_acquired) return;
    Q_UNUSED(coefficient);
    // TODO: GUID_Friction effect
}

void LogitechFFB::setRumble(float strongMotor, float weakMotor) {
    if (!m_acquired) return;
    Q_UNUSED(strongMotor);
    Q_UNUSED(weakMotor);
    // Logitech wheels use single motor for FFB — no separate rumble
}

// ============================================================================
// DirectInput effect update
// ============================================================================

void LogitechFFB::updateConstantForce(float forcePercent) {
#ifdef _WIN32
    if (!m_constantEffectRef) return;

    auto* effect = static_cast<IDirectInputEffect*>(m_constantEffectRef);
    if (!effect) return;

    LONG force = static_cast<LONG>(forcePercent * DI_FFNOMINALMAX);

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

    effect->Start(1, 0);
#endif
}

void LogitechFFB::processDIInput() {
#ifdef _WIN32
    if (!m_device) return;

    HRESULT hr = m_device->Poll();
    if (FAILED(hr)) {
        hr = m_device->Acquire();
        if (FAILED(hr)) return;
        m_device->Poll();
    }
#endif
}

// ============================================================================
// Shutdown
// ============================================================================

void LogitechFFB::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);

#ifdef _WIN32
    if (m_constantEffectRef) {
        static_cast<IDirectInputEffect*>(m_constantEffectRef)->Release();
        m_constantEffectRef = nullptr;
    }
    if (m_damperEffectRef) {
        static_cast<IDirectInputEffect*>(m_damperEffectRef)->Release();
        m_damperEffectRef = nullptr;
    }

    delete[] m_constantEffect->rgdwAxes;
    delete[] m_constantEffect->rglDirection;
    delete m_constantEffect; m_constantEffect = nullptr;
    delete[] m_damperEffect->rgdwAxes;
    delete[] m_damperEffect->rglDirection;
    delete m_damperEffect; m_damperEffect = nullptr;

    if (m_device) {
        m_device->Unacquire();
        m_device->Release();
        m_device = nullptr;
    }

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

QString LogitechFFB::modelName() const {
    for (const auto& dev : knownDevices()) {
        if (dev.pid == m_productId) return QString::fromLatin1(dev.name);
    }
    return "Logitech (Unknown)";
}

} // namespace ks::device
