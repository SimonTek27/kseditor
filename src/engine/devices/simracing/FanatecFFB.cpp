#include "FanatecFFB.h"
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
// Known Fanatec wheel PIDs and their max torque
// ============================================================================
const std::vector<FanatecFFB::DeviceInfo>& FanatecFFB::knownDevices() {
    static const std::vector<DeviceInfo> devices = {
        {0x0001, WheelModel::CSLDD,        "CSL DD",           5.0f},
        {0x0002, WheelModel::CSLDDPlus,     "CSL DD+",          8.0f},
        {0x0003, WheelModel::CSLElite,      "CSL Elite",        6.0f},
        {0x0004, WheelModel::ClubSportV2,   "ClubSport V2",     8.0f},
        {0x0005, WheelModel::ClubSportV3,   "ClubSport V3",     9.0f},
        {0x0006, WheelModel::PodiumDD1,     "Podium DD1",      20.0f},
        {0x0007, WheelModel::PodiumDD2,     "Podium DD2",      25.0f},
        {0x0008, WheelModel::PodiumDDPlus,  "Podium DD+",      15.0f},
    };
    return devices;
}

float FanatecFFB::maxTorqueForModel(WheelModel model) {
    for (const auto& dev : knownDevices()) {
        if (dev.model == model) return dev.maxTorque;
    }
    return 10.0f;
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

FanatecFFB::FanatecFFB() = default;

FanatecFFB::~FanatecFFB() {
    shutdown();
}

// ============================================================================
// Initialization
// ============================================================================

bool FanatecFFB::initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);
    qInfo() << "FanatecFFB: Initializing (DirectInput8)";

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
        qWarning() << "FanatecFFB: DirectInput8Create failed:" << Qt::hex << hr;
        return false;
    }

    // 2. Find and acquire Fanatec wheel
    if (!enumerateDevice()) {
        qWarning() << "FanatecFFB: No Fanatec wheel found";
        shutdown();
        return false;
    }

    // 3. Set cooperative level — exclusive for FFB
    hr = m_device->SetCooperativeLevel(
        GetForegroundWindow(),
        DISCL_EXCLUSIVE | DISCL_FOREGROUND
    );
    if (FAILED(hr)) {
        qWarning() << "FanatecFFB: SetCooperativeLevel failed:" << Qt::hex << hr;
        shutdown();
        return false;
    }

    // 4. Set data format for joystick
    hr = m_device->SetDataFormat(&c_dfDIJoystick2);
    if (FAILED(hr)) {
        qWarning() << "FanatecFFB: SetDataFormat failed:" << Qt::hex << hr;
        shutdown();
        return false;
    }

    // 5. Acquire the device
    hr = m_device->Acquire();
    if (FAILED(hr)) {
        qWarning() << "FanatecFFB: Acquire failed:" << Qt::hex << hr;
        shutdown();
        return false;
    }

    // 6. Create FFB effects
    m_fsbInitialized = createFFBEffect();
    if (!m_fsbInitialized) {
        qWarning() << "FanatecFFB: Failed to create FFB effects";
    }

    m_acquired = true;
    qInfo() << "FanatecFFB: Acquired" << modelName()
            << "(max" << maxTorqueNm() << "Nm)";
    return true;

#else
    qInfo() << "FanatecFFB: DirectInput not available on this platform";
    return false;
#endif
}

// ============================================================================
// Device enumeration
// ============================================================================

bool FanatecFFB::enumerateDevice() {
#ifdef _WIN32
    if (!m_dinput) return false;

    struct EnumContext {
        FanatecFFB* self;
        bool found;
    } ctx{this, false};

    auto enumCallback = [](const DIDEVICEINSTANCEA* inst, void* ref) -> BOOL {
        auto* ctx = static_cast<EnumContext*>(ref);

        // Check if this is a Fanatec device (VID 0x0EB7)
        if (LOWORD(inst->guidProduct.Data1) != FANATEC_VID) {
            return DIENUM_CONTINUE;
        }

        // Match against known Fanatec PIDs
        uint16_t pid = HIWORD(inst->guidProduct.Data1);
        for (const auto& dev : knownDevices()) {
            if (pid == dev.pid) {
                ctx->self->m_productId = dev.pid;
                ctx->self->m_model = dev.model;
                qInfo() << "FanatecFFB: Found" << dev.name
                        << "(PID:" << Qt::hex << dev.pid << Qt::dec
                        << "," << dev.maxTorque << "Nm)";
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

bool FanatecFFB::createFFBEffect() {
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
        qWarning() << "FanatecFFB: CreateEffect (Constant) failed:" << Qt::hex << hr;
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
        qWarning() << "FanatecFFB: CreateEffect (Damper) failed:" << Qt::hex << hr;
    }

    return (m_constantEffectRef != nullptr);

#else
    return false;
#endif
}

// ============================================================================
// Update FFB
// ============================================================================

void FanatecFFB::updateFFB(float torqueNm) {
    if (!m_acquired) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastTorque = torqueNm;

    float maxNm = maxTorqueNm();
    float normalizedTorque = qBound(-1.0f, torqueNm / maxNm, 1.0f);
    updateConstantForce(normalizedTorque);
}

void FanatecFFB::setConstantForce(float magnitude) {
    if (!m_acquired) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    updateConstantForce(qBound(-1.0f, magnitude, 1.0f));
}

void FanatecFFB::setSpringForce(float center, float stiffness, float damping) {
    if (!m_acquired) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    Q_UNUSED(center);
    Q_UNUSED(stiffness);
    Q_UNUSED(damping);
    // TODO: GUID_Spring effect
}

void FanatecFFB::setDamperForce(float velocity, float coefficient) {
    if (!m_acquired) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    Q_UNUSED(velocity);
    // TODO: Damper effect gain update
}

void FanatecFFB::setFrictionForce(float coefficient) {
    if (!m_acquired) return;
    Q_UNUSED(coefficient);
    // TODO: GUID_Friction effect
}

void FanatecFFB::setRumble(float strongMotor, float weakMotor) {
    if (!m_acquired) return;
    Q_UNUSED(strongMotor);
    Q_UNUSED(weakMotor);
    // Fanatec DD wheels have no rumble motors — FFB only
}

// ============================================================================
// DirectInput effect update
// ============================================================================

void FanatecFFB::updateConstantForce(float forcePercent) {
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

void FanatecFFB::processDIInput() {
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

void FanatecFFB::shutdown() {
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
// Model info
// ============================================================================

QString FanatecFFB::modelName() const {
    for (const auto& dev : knownDevices()) {
        if (dev.pid == m_productId) return QString::fromLatin1(dev.name);
    }
    return "Fanatec (Unknown)";
}

float FanatecFFB::maxTorqueNm() const {
    return maxTorqueForModel(m_model);
}

} // namespace ks::device
