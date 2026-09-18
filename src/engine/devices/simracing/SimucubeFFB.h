#pragma once
#include "SimRacingDevices.h"
#include <memory>
#include <mutex>

namespace ks::device {

// ============================================================================
// SimucubeFFB — TrueDrive UDP protocol for Simucube wheels
// ============================================================================
// Supports: Simucube 1, Simucube 2 Pro, Simucube 2 Ultimate
// Uses the TrueDrive protocol over UDP for force feedback.
// The TrueDrive protocol is documented and open — no proprietary SDK needed.
// Communication is via UDP packets to the Simucube's IP address.
// ============================================================================

class SimucubeFFB {
public:
    SimucubeFFB();
    ~SimucubeFFB();

    bool initialize();
    void shutdown();
    bool isSupported() const { return m_connected; }

    // Core FFB
    void updateFFB(float torqueNm);

    // Effect types
    void setConstantForce(float magnitude);   // -1.0 to 1.0
    void setSpringForce(float center, float stiffness, float damping);
    void setDamperForce(float velocity, float coefficient);
    void setFrictionForce(float coefficient);
    void setRumble(float strongMotor, float weakMotor);  // Simucube has no rumble

    // Simucube-specific
    enum class WheelModel { Unknown, Simucube1, Simucube2Pro, Simucube2Ultimate };
    WheelModel detectedModel() const { return m_model; }
    QString modelName() const;
    float maxTorqueNm() const;

    void setTargetIP(const QString& ip) { m_targetIP = ip; }
    QString targetIP() const { return m_targetIP; }

private:
    bool discoverDevice();
    bool sendTrueDriveCommand(const uint8_t* data, size_t len);
    bool sendTorqueCommand(float torqueNm);
    void processUDPResponse();

    bool m_connected = false;
    std::mutex m_mutex;
    float m_lastTorque = 0.0f;

    WheelModel m_model = WheelModel::Unknown;
    QString m_targetIP;
    uint16_t m_targetPort = 0;

    // UDP socket
    void* m_udpSocket = nullptr;  // SOCKET on Windows, int on Linux

    // TrueDrive protocol constants
    static constexpr uint16_t TRUEDRIVE_DEFAULT_PORT = 1234;
    static constexpr uint8_t CMD_TORQUE = 0x01;
    static constexpr uint8_t CMD_QUERY = 0x10;
    static constexpr uint8_t CMD_SET_MAX_TORQUE = 0x20;

    // Max torque per model (Nm)
    static float maxTorqueForModel(WheelModel model);
};

} // namespace ks::device
