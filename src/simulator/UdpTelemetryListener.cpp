#include "UdpTelemetryListener.h"

#include <cstring>
#include <iostream>
#include <thread>
#include <atomic>

// Platform-specific socket includes and initialization
#if defined(_WIN32) || defined(__WIN32__)
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define CLOSE_SOCKET closesocket
    #define SET_SOCKET_OPTS setsockopt
    #define BIND_SOCKET bind
    #define SOCKET_TYPE SOCKET
    #define INVALID_SOCKET_VAL INVALID_SOCKET
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #define CLOSE_SOCKET close
    #define SET_SOCKET_OPTS setsockopt
    #define BIND_SOCKET bind
    #define SOCKET_TYPE int
    #define INVALID_SOCKET_VAL -1
#endif

namespace ks::sim {

UdpTelemetryListener::UdpTelemetryListener()
    : m_socketfd(INVALID_SOCKET_VAL)
    , m_running(false)
{
}

UdpTelemetryListener::~UdpTelemetryListener() {
    stop();
}

bool UdpTelemetryListener::start(uint16_t port) {
    m_running = true;

#if defined(_WIN32) || defined(__WIN32__)
    // Initialize Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        m_running = false;
        return false;
    }
#endif

    m_socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_socketfd == INVALID_SOCKET_VAL) {
        std::cerr << "Failed to create socket" << std::endl;
        m_running = false;
        return false;
    }

    // Set socket to non-blocking / reusable
    int reuse = 1;
    SET_SOCKET_OPTS(m_socketfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));

    // On Windows, also set SO_EXCLUSIVEADDRUSE if needed
#if defined(_WIN32) || defined(__WIN32__)
    int exclusive = 1;
    SET_SOCKET_OPTS(m_socketfd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char*)&exclusive, sizeof(exclusive));
#endif

    sockaddr_in localAddr{};
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons(port);

    if (BIND_SOCKET(m_socketfd, (sockaddr*)&localAddr, sizeof(localAddr)) < 0) {
        std::cerr << "Failed to bind socket to port " << port << std::endl;
        CLOSE_SOCKET(m_socketfd);
        m_socketfd = INVALID_SOCKET_VAL;
        m_running = false;
        return false;
    }

    // Set socket to non-blocking
#if !defined(_WIN32) || defined(__WIN32__)
    int flags = fcntl(m_socketfd, F_GETFL, 0);
    fcntl(m_socketfd, F_SETFL, flags | O_NONBLOCK);
#else
    u_long mode = 1; // Non-blocking mode
    ioctlsocket(m_socketfd, FIONBBLK, &mode);
#endif

    m_receiveThread = std::thread(&UdpTelemetryListener::receiveThread, this);
    return true;
}

void UdpTelemetryListener::stop() {
    if (m_running) {
        m_running = false;
        if (m_receiveThread.joinable()) {
            m_receiveThread.join();
        }
    }
    if (m_socketfd != INVALID_SOCKET_VAL) {
        CLOSE_SOCKET(m_socketfd);
        m_socketfd = INVALID_SOCKET_VAL;
    }
#if defined(_WIN32) || defined(__WIN32__)
    WSACleanup();
#endif
}

void UdpTelemetryListener::setCallback(TelemetryCallback cb) {
    m_callback = std::move(cb);
}

bool UdpTelemetryListener::isRunning() const {
    return m_running.load();
}

AcTelemetryData UdpTelemetryListener::getLastData() const {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return m_lastData;
}

void UdpTelemetryListener::receiveThread() {
    char buffer[2048]; // AC telemetry packets are typically ~1300 bytes

    while (m_running.load()) {
#if defined(_WIN32) || defined(__WIN32__)
        int len = recv(m_socketfd, buffer, sizeof(buffer), 0);
#else
        ssize_t len = recv(m_socketfd, buffer, sizeof(buffer), 0);
#endif
        if (len > 0) {
            parsePacket(reinterpret_cast<const uint8_t*>(buffer), static_cast<size_t>(len));
        } else if (len == 0) {
            // Connection closed
            break;
        } else {
            // Error or would block - sleep briefly
#if defined(_WIN32) || defined(__WIN32__)
            Sleep(1);
#else
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
#endif
        }
    }
}

void UdpTelemetryListener::parsePacket(const uint8_t* data, size_t size) {
    // Assetto Corsa telemetry packet format (simplified)
    // The packet starts with a header containing the size and version
    if (size < 8) return;

    // First 4 bytes: packet id (should be 0x0000 for telemetry)
    // Next 4 bytes: packet size
    uint16_t packetId = *(reinterpret_cast<const uint16_t*>(data));
    uint16_t packetSize = *(reinterpret_cast<const uint16_t*>(data + 2));

    // Verify packet size matches expected
    // AC telemetry packet ID 0 is the main car telemetry
    if (packetId != 0 || packetSize < sizeof(AcTelemetryData)) return;

    // Extract data - AC uses little-endian format
    const uint8_t* ptr = data + 4; // Skip header

    AcTelemetryData telemetry{};

    // Copy data field by field (assuming layout matches AC's structure)
    // This is a simplified parsing - real implementation would need to match AC's exact struct

    // Basic fields at known offsets (this varies by AC version)
    // Telemetry packet structure in AC:
    // Offset 0: packetId (uint16)
    // Offset 2: packetSize (uint16) 
    // Offset 4: timestamp (uint64)
    // Offset 12: carPosition (float[3])
    // etc.

    // For now, we'll set what we can from the raw data
    // The actual parsing would require knowing the exact AC telemetry struct layout

    // Read timestamp (uint64 at offset 4)
    if (size >= 12) {
        telemetry.timestamp = *reinterpret_cast<const uint64_t*>(ptr);
    }

    // The remaining fields would need proper offset mapping
    // Based on Assetto Corsa telemetry documentation, the main car telemetry packet
    // contains fields at specific offsets. We'll map the most important ones.

    // Since the exact struct layout varies by game version, we'll use a generic approach:
    // Copy whatever data we can safely extract

    // Update last data with what we have
    {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_lastData = telemetry;
    }

    // Call the callback if set
    if (m_callback) {
        m_callback(telemetry);
    }
}

} // namespace ks::sim