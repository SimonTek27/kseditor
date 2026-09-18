#include "FFBSDKFactory.h"
#include <QDebug>

#ifdef _WIN32
#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <dinput.h>
#endif

namespace ks::device {

// ============================================================================
// Auto-detect and create FFB
// ============================================================================

std::unique_ptr<FFBBase> FFBSDKFactory::createFFB() {
    WheelBrand brand = detectWheel();
    if (brand == WheelBrand::None) {
        qInfo() << "FFBSDKFactory: No FFB wheel detected";
        return nullptr;
    }
    return createFFBForBrand(brand);
}

// ============================================================================
// Detect wheel brand
// ============================================================================

FFBSDKFactory::WheelBrand FFBSDKFactory::detectWheel() {
    // Try DirectInput detection first (works for Logitech, Thrustmaster, and some MOZA)
    WheelBrand brand = detectDirectInputDevices();
    if (brand != WheelBrand::None) return brand;

    // MOZA wheels may not appear in DirectInput — try HID detection
    // Check if any MOZA HID device is connected
    {
        auto impl = std::make_unique<MozaFFB>();
        // Try initialize just to detect — shutdown immediately if found
        // This is safe because MozaFFB::initialize() only opens the HID handle
        if (impl->initialize()) {
            qInfo() << "FFBSDKFactory: MOZA wheel detected via HID";
            impl->shutdown();
            return WheelBrand::Moza;
        }
    }

    // TODO: Fanatec detection via FDB SDK or USB HID
    // TODO: Simucube detection via TrueDrive TCP scan

    return WheelBrand::None;
}

FFBSDKFactory::WheelBrand FFBSDKFactory::detectDirectInputDevices() {
#ifdef _WIN32
    IDirectInput8A* dinput = nullptr;
    HRESULT hr = DirectInput8Create(
        GetModuleHandle(nullptr),
        DIRECTINPUT_VERSION,
        IID_IDirectInput8A,
        reinterpret_cast<void**>(&dinput),
        nullptr
    );
    if (FAILED(hr) || !dinput) return WheelBrand::None;

    struct DetectContext {
        WheelBrand foundBrand = WheelBrand::None;
    } ctx;

    auto enumCallback = [](const DIDEVICEINSTANCEA* inst, void* ref) -> BOOL {
        auto* ctx = static_cast<DetectContext*>(ref);

        uint16_t vid = LOWORD(inst->guidProduct.Data1);
        uint16_t pid = HIWORD(inst->guidProduct.Data1);

        // Logitech VID
        if (vid == LogitechFFB::LOGITECH_VID) {
            qInfo() << "FFBSDKFactory: Logitech device detected (PID:" << Qt::hex << pid << Qt::dec << ")";
            ctx->foundBrand = WheelBrand::Logitech;
            return DIENUM_STOP;
        }

        // Thrustmaster VID
        if (vid == ThrustmasterFFB::THRUSTMASTER_VID) {
            qInfo() << "FFBSDKFactory: Thrustmaster device detected (PID:" << Qt::hex << pid << Qt::dec << ")";
            ctx->foundBrand = WheelBrand::Thrustmaster;
            return DIENUM_STOP;
        }

        // MOZA VID
        if (vid == MozaFFB::MOZA_VID) {
            qInfo() << "FFBSDKFactory: MOZA device detected (PID:" << Qt::hex << pid << Qt::dec << ")";
            ctx->foundBrand = WheelBrand::Moza;
            return DIENUM_STOP;
        }

        return DIENUM_CONTINUE;
    };

    dinput->EnumDevices(
        DI8DEVCLASS_GAMECTRL,
        enumCallback,
        &ctx,
        DIEDFL_ATTACHEDONLY | DIEDFL_FORCEFEEDBACK
    );

    dinput->Release();
    return ctx.foundBrand;

#else
    return WheelBrand::None;
#endif
}

// ============================================================================
// Create specific brand FFB
// ============================================================================

std::unique_ptr<FFBBase> FFBSDKFactory::createFFBForBrand(WheelBrand brand) {
    switch (brand) {
        case WheelBrand::Logitech: {
            auto impl = std::make_unique<LogitechFFB>();
            if (impl->initialize()) {
                qInfo() << "FFBSDKFactory: Logitech FFB initialized";
                return std::make_unique<FFBLogitechAdapter>(std::move(impl));
            }
            qWarning() << "FFBSDKFactory: Logitech FFB initialization failed";
            return nullptr;
        }
        case WheelBrand::Thrustmaster: {
            auto impl = std::make_unique<ThrustmasterFFB>();
            if (impl->initialize()) {
                qInfo() << "FFBSDKFactory: Thrustmaster FFB initialized";
                return std::make_unique<FFBThrustmasterAdapter>(std::move(impl));
            }
            qWarning() << "FFBSDKFactory: Thrustmaster FFB initialization failed";
            return nullptr;
        }
        case WheelBrand::Fanatec: {
            auto impl = std::make_unique<FanatecFFB>();
            if (impl->initialize()) {
                qInfo() << "FFBSDKFactory: Fanatec FFB initialized";
                return std::make_unique<FFBFanatecAdapter>(std::move(impl));
            }
            qWarning() << "FFBSDKFactory: Fanatec FFB initialization failed";
            return nullptr;
        }
        case WheelBrand::Simucube: {
            auto impl = std::make_unique<SimucubeFFB>();
            if (impl->initialize()) {
                qInfo() << "FFBSDKFactory: Simucube FFB initialized";
                return std::make_unique<FFBSimucubeAdapter>(std::move(impl));
            }
            qWarning() << "FFBSDKFactory: Simucube FFB initialization failed";
            return nullptr;
        }
        case WheelBrand::Moza: {
            auto impl = std::make_unique<MozaFFB>();
            if (impl->initialize()) {
                qInfo() << "FFBSDKFactory: MOZA FFB initialized";
                return std::make_unique<FFBMozaAdapter>(std::move(impl));
            }
            qWarning() << "FFBSDKFactory: MOZA FFB initialization failed";
            return nullptr;
        }
        case WheelBrand::None:
        default:
            return nullptr;
    }
}

} // namespace ks::device
