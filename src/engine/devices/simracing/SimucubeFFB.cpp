#include "SimucubeFFB.h"
#include <QDebug>

// ============================================================================
// Platform-specific networking headers
// ============================================================================
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define closesocket close
#endif

namespace ks::device {

// ============================================================================
// Max torque per model
// ============================================================================

float SimucubeFFB::maxTorqueForModel(WheelModel model) {
    switch (model) {
        case WheelModel::Simucube1:          return 15.0f;
        case WheelModel::Simucube2Pro:       return 25.0f;
        case WheelModel::Simucube2Ultimate:  return 32.0f;
        default: return 20.0f;
    }
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

SimucubeFFB::SimucubeFFB() = default;

SimucubeFFB::~SimucubeFFB() {
    shutdown();
}

// ============================================================================
// Initialization — discover Simucube on network and connect
// ============================================================================

bool SimucubeFFB::initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);
    qInfo() << "SimucubeFFB: Initializing (TrueDrive UDP)";

#ifdef _WIN32
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        qWarning() << "SimucubeFFB: WSAStartup failed";
        return false;
    }
#endif

    // Create UDP socket
    m_udpSocket = reinterpret_cast<void*>(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
    if (reinterpret_cast<SOCKET>(m_udpSocket) == INVALID_SOCKET) {
        qWarning() << "SimucubeFFB: Socket creation failed";
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }

    // Set socket timeout (1 second)
    int timeout = 1000;
#ifdef _WIN32
    setsockopt(reinterpret_cast<SOCKET>(m_udpSocket), SOL_SOCKET, SO_RCVTIMEO,
               reinterpret_cast<const char*>(&timeout), sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(reinterpret_cast<SOCKET>(m_udpSocket), SOL_SOCKET, SO_RCVTIMEO,
               &tv, sizeof(tv));
#endif

    // Discover device if no IP set
    if (m_targetIP.isEmpty()) {
        if (!discoverDevice()) {
            qWarning() << "SimucubeFFB: No Simucube found on network";
            shutdown();
            return false;
        }
    }

    m_connected = true;
    qInfo() << "SimucubeFFB: Connected to" << m_targetIP << ":" << m_targetPort
            << "(" << modelName() << "," << maxTorqueNm() << "Nm)";
    return true;
}

// ============================================================================
// Device discovery — broadcast UDP probe on local network
// ============================================================================

bool SimucubeFFB::discoverDevice() {
    // TrueDrive discovery: send query to broadcast address on default port
    // Simucube responds with its model and serial number

    uint8_t queryPacket[8] = {};
    queryPacket[0] = CMD_QUERY;
    queryPacket[1] = 0x00;  // Query type: device info
    queryPacket[2] = 0x01;  // Request version

    // Try common Simucube IPs on local network
    QStringList tryIPs = {
        "192.168.1.100",
        "192.168.1.101",
        "192.168.0.100",
        "192.168.0.101",
        "10.0.0.100",
    };

    for (const QString& ip : tryIPs) {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(TRUEDRIVE_DEFAULT_PORT);
        addr.sin_addr.s_addr = inet_addr(ip.toUtf8().constData());

        // Send query
        sendto(reinterpret_cast<SOCKET>(m_udpSocket),
               reinterpret_cast<const char*>(queryPacket), sizeof(queryPacket),
               0, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));

        // Wait for response
        uint8_t response[64] = {};
        struct sockaddr_in fromAddr;
        int fromLen = sizeof(fromAddr);

        int received = recvfrom(reinterpret_cast<SOCKET>(m_udpSocket),
                                reinterpret_cast<char*>(response), sizeof(response),
                                0, reinterpret_cast<struct sockaddr*>(&fromAddr), &fromLen);

        if (received > 0 && response[0] == CMD_QUERY) {
            // Parse response to identify model
            uint8_t modelId = response[1];
            switch (modelId) {
                case 1: m_model = WheelModel::Simucube1; break;
                case 2: m_model = WheelModel::Simucube2Pro; break;
                case 3: m_model = WheelModel::Simucube2Ultimate; break;
                default: m_model = WheelModel::Unknown; break;
            }

            m_targetIP = ip;
            m_targetPort = TRUEDRIVE_DEFAULT_PORT;
            qInfo() << "SimucubeFFB: Discovered at" << ip
                    << "- Model:" << modelName();
            return true;
        }
    }

    return false;
}

// ============================================================================
// TrueDrive protocol commands
// ============================================================================

bool SimucubeFFB::sendTrueDriveCommand(const uint8_t* data, size_t len) {
    if (!m_connected || m_targetIP.isEmpty()) return false;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_targetPort);
    addr.sin_addr.s_addr = inet_addr(m_targetIP.toUtf8().constData());

    int sent = sendto(reinterpret_cast<SOCKET>(m_udpSocket),
                      reinterpret_cast<const char*>(data), static_cast<int>(len),
                      0, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));

    return sent == static_cast<int>(len);
}

bool SimucubeFFB::sendTorqueCommand(float torqueNm) {
    // TrueDrive torque command:
    // Byte 0: Command ID (0x01 = torque)
    // Byte 1-4: Torque value (float, in Nm, little-endian)
    // Byte 5: Flags (0x00 = immediate)

    float maxNm = maxTorqueNm();
    float clampedTorque = qBound(-maxNm, torqueNm, maxNm);

    uint8_t packet[8] = {};
    packet[0] = CMD_TORQUE;

    // Write float as little-endian bytes
    uint32_t torqueBits;
    memcpy(&torqueBits, &clampedTorque, sizeof(float));
    packet[1] = static_cast<uint8_t>(torqueBits & 0xFF);
    packet[2] = static_cast<uint8_t>((torqueBits >> 8) & 0xFF);
    packet[3] = static_cast<uint8_t>((torqueBits >> 16) & 0xFF);
    packet[4] = static_cast<uint8_t>((torqueBits >> 24) & 0xFF);
    packet[5] = 0x00;  // Flags: immediate

    return sendTrueDriveCommand(packet, sizeof(packet));
}

void SimucubeFFB::processUDPResponse() {
    if (!m_connected || !m_udpSocket) return;

    uint8_t response[64] = {};
    int received = recvfrom(reinterpret_cast<SOCKET>(m_udpSocket),
                            reinterpret_cast<char*>(response), sizeof(response),
                            0, nullptr, nullptr);

    if (received > 0) {
        // Parse TrueDrive response
        uint8_t cmd = response[0];
        if (cmd == 0x10) {
            // Status response
            uint8_t status = response[1];
            if (status & 0x01) {
                // Emergency stop active
                qWarning() << "SimucubeFFB: Emergency stop active!";
            }
        }
    }
}

// ============================================================================
// Update FFB
// ============================================================================

void SimucubeFFB::updateFFB(float torqueNm) {
    if (!m_connected) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastTorque = torqueNm;
    sendTorqueCommand(torqueNm);
}

void SimucubeFFB::setConstantForce(float magnitude) {
    if (!m_connected) return;
    float torqueNm = magnitude * maxTorqueNm();
    updateFFB(torqueNm);
}

void SimucubeFFB::setSpringForce(float center, float stiffness, float damping) {
    if (!m_connected) return;
    Q_UNUSED(center);
    Q_UNUSED(stiffness);
    Q_UNUSED(damping);

    // TrueDrive spring effect command
    uint8_t packet[8] = {};
    packet[0] = 0x11;  // Spring effect
    packet[1] = static_cast<uint8_t>(qBound(0.0f, center, 1.0f) * 255);
    packet[2] = static_cast<uint8_t>(qBound(0.0f, stiffness, 1.0f) * 255);
    packet[3] = static_cast<uint8_t>(qBound(0.0f, damping, 1.0f) * 255);
    sendTrueDriveCommand(packet, sizeof(packet));
}

void SimucubeFFB::setDamperForce(float velocity, float coefficient) {
    if (!m_connected) return;
    Q_UNUSED(velocity);

    uint8_t packet[8] = {};
    packet[0] = 0x12;  // Damper effect
    packet[1] = static_cast<uint8_t>(qBound(0.0f, coefficient, 1.0f) * 255);
    sendTrueDriveCommand(packet, sizeof(packet));
}

void SimucubeFFB::setFrictionForce(float coefficient) {
    if (!m_connected) return;

    uint8_t packet[8] = {};
    packet[0] = 0x13;  // Friction effect
    packet[1] = static_cast<uint8_t>(qBound(0.0f, coefficient, 1.0f) * 255);
    sendTrueDriveCommand(packet, sizeof(packet));
}

void SimucubeFFB::setRumble(float strongMotor, float weakMotor) {
    Q_UNUSED(strongMotor);
    Q_UNUSED(weakMotor);
    // Simucube direct drive wheels have no rumble motors — FFB only
}

// ============================================================================
// Shutdown
// ============================================================================

void SimucubeFFB::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Send zero torque before disconnecting
    if (m_connected) {
        sendTorqueCommand(0.0f);
    }

    if (m_udpSocket != nullptr) {
        closesocket(reinterpret_cast<SOCKET>(m_udpSocket));
        m_udpSocket = nullptr;
    }

#ifdef _WIN32
    WSACleanup();
#endif

    m_connected = false;
    m_model = WheelModel::Unknown;
    m_targetIP.clear();
    m_targetPort = 0;
}

// ============================================================================
// Model info
// ============================================================================

QString SimucubeFFB::modelName() const {
    switch (m_model) {
        case WheelModel::Simucube1:         return "Simucube 1";
        case WheelModel::Simucube2Pro:      return "Simucube 2 Pro";
        case WheelModel::Simucube2Ultimate: return "Simucube 2 Ultimate";
        default: return "Simucube (Unknown)";
    }
}

float SimucubeFFB::maxTorqueNm() const {
    return maxTorqueForModel(m_model);
}

} // namespace ks::device
