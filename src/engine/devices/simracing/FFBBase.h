#pragma once

namespace ks::device {

// ============================================================================
// FFBBase - Abstract interface for all FFB SDK wrappers
// ============================================================================
// Lives in its own header because the concrete wrappers (LogitechFFB,
// MozaFFB, ...) derive from it while FFBSDKFactory.h includes those headers.
// ============================================================================

class FFBBase {
public:
    virtual ~FFBBase() = default;
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual void updateFFB(float torqueNm) = 0;
    virtual bool isSupported() const = 0;
    virtual bool isConnected() const { return false; }
};

} // namespace ks::device
