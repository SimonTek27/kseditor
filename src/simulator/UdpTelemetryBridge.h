#pragma once
/** UDP telemetry — KSIM binary v2 includes damage channels. */
#include <cstdint>
#include <cstring>
#include <string>
#include <mutex>
#include <cstdio>
#include <cmath>

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "ws2_32.lib")
#else
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <unistd.h>
#  include <fcntl.h>
#endif

namespace ks {
namespace sim {

#pragma pack(push, 1)
struct UdpTelemPacket {
    char magic[4];
    uint16_t version; // 2
    uint16_t size;
    uint32_t sequence;
    double timeSec;
    float speedMs, rpm, throttle, brake, steer;
    int32_t gear;
    float fuelL;
    float posX, posY, posZ;
    float velX, velY, velZ;
    float accGX, accGY, accGZ;
    float heading;
    float tyreTemp[4], tyreWear[4], tyrePressure[4];
    int32_t completedLaps, currentSector, currentTimeMs, lastTimeMs, bestTimeMs;
    int32_t position, sessionType, status;
    float normalizedSpline, surfaceGrip, airTemp, roadTemp;
    uint8_t inPit, pitLimiter, damageWarning, engineSeized;
    float damageOverall, engineHealth, powerMult, dragMult, downforceMult;
    float carDamage[5];
    float suspIntegrity[4];
};
#pragma pack(pop)

static_assert(sizeof(UdpTelemPacket) < 1500, "UDP packet should fit one datagram");

struct UdpTelemSample {
    double timeSec = 0;
    float speedMs = 0, rpm = 0;
    float throttle = 0, brake = 0, steer = 0;
    int gear = 1;
    float fuelL = 0;
    float posX = 0, posY = 0, posZ = 0;
    float velX = 0, velY = 0, velZ = 0;
    float accGX = 0, accGY = 0, accGZ = 0;
    float heading = 0;
    float tyreTemp[4] = {80, 80, 80, 80};
    float tyreWear[4] = {};
    float tyrePressure[4] = {2.2f, 2.2f, 2.0f, 2.0f};
    int completedLaps = 0, currentSector = 0;
    int currentTimeMs = 0, lastTimeMs = 0, bestTimeMs = 0;
    int position = 1, sessionType = 2, status = 2;
    float normalizedSpline = 0, surfaceGrip = 1.f;
    float airTemp = 25.f, roadTemp = 30.f;
    bool inPit = false, pitLimiter = false;
    float damageOverall = 0.f, engineHealth = 1.f;
    float powerMult = 1.f, dragMult = 1.f, downforceMult = 1.f;
    float carDamage[5] = {};
    float suspIntegrity[4] = {1, 1, 1, 1};
    int damageWarning = 0;
    bool engineSeized = false;
};

class UdpTelemetryBridge {
public:
    UdpTelemetryBridge() = default;
    ~UdpTelemetryBridge() { close(); }

    bool open(const char* host = "127.0.0.1", uint16_t port = 20777) {
        std::lock_guard<std::mutex> lock(m_mutex);
        closeUnlocked();
#ifdef _WIN32
        if (!m_wsa) {
            WSADATA wsa;
            if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;
            m_wsa = true;
        }
        m_sock = static_cast<int>(::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
        if (m_sock == INVALID_SOCKET) { m_sock = -1; return false; }
#else
        m_sock = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (m_sock < 0) return false;
        int flags = fcntl(m_sock, F_GETFL, 0);
        if (flags >= 0) fcntl(m_sock, F_SETFL, flags | O_NONBLOCK);
#endif
        std::memset(&m_dest, 0, sizeof(m_dest));
        m_dest.sin_family = AF_INET;
        m_dest.sin_port = htons(port);
        if (::inet_pton(AF_INET, host, &m_dest.sin_addr) != 1) { closeUnlocked(); return false; }
        m_host = host; m_port = port; m_ok = true;
        std::fprintf(stderr, "UdpTelemetryBridge: → %s:%u (v2+damage)\n", host, unsigned(port));
        return true;
    }

    void close() { std::lock_guard<std::mutex> lock(m_mutex); closeUnlocked(); }
    void setEnabled(bool e) { m_enabled = e; }
    bool isEnabled() const { return m_enabled; }
    bool isOpen() const { return m_ok; }
    void setJsonMode(bool j) { m_json = j; }

    bool publish(const UdpTelemSample& s) {
        if (!m_enabled) return false;
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_ok || m_sock < 0) return false;
        m_seq++;
        if (m_json) return sendJson(s);
        return sendBinary(s);
    }

private:
    bool sendBinary(const UdpTelemSample& s) {
        UdpTelemPacket p{};
        p.magic[0] = 'K'; p.magic[1] = 'S'; p.magic[2] = 'I'; p.magic[3] = 'M';
        p.version = 2;
        p.size = static_cast<uint16_t>(sizeof(UdpTelemPacket));
        p.sequence = m_seq;
        p.timeSec = s.timeSec;
        p.speedMs = s.speedMs; p.rpm = s.rpm;
        p.throttle = s.throttle; p.brake = s.brake; p.steer = s.steer;
        p.gear = s.gear; p.fuelL = s.fuelL;
        p.posX = s.posX; p.posY = s.posY; p.posZ = s.posZ;
        p.velX = s.velX; p.velY = s.velY; p.velZ = s.velZ;
        p.accGX = s.accGX; p.accGY = s.accGY; p.accGZ = s.accGZ;
        p.heading = s.heading;
        for (int i = 0; i < 4; ++i) {
            p.tyreTemp[i] = s.tyreTemp[i];
            p.tyreWear[i] = s.tyreWear[i];
            p.tyrePressure[i] = s.tyrePressure[i];
            p.suspIntegrity[i] = s.suspIntegrity[i];
        }
        p.completedLaps = s.completedLaps; p.currentSector = s.currentSector;
        p.currentTimeMs = s.currentTimeMs; p.lastTimeMs = s.lastTimeMs; p.bestTimeMs = s.bestTimeMs;
        p.position = s.position; p.sessionType = s.sessionType; p.status = s.status;
        p.normalizedSpline = s.normalizedSpline; p.surfaceGrip = s.surfaceGrip;
        p.airTemp = s.airTemp; p.roadTemp = s.roadTemp;
        p.inPit = s.inPit ? 1 : 0; p.pitLimiter = s.pitLimiter ? 1 : 0;
        p.damageWarning = static_cast<uint8_t>(s.damageWarning);
        p.engineSeized = s.engineSeized ? 1 : 0;
        p.damageOverall = s.damageOverall; p.engineHealth = s.engineHealth;
        p.powerMult = s.powerMult; p.dragMult = s.dragMult; p.downforceMult = s.downforceMult;
        for (int i = 0; i < 5; ++i) p.carDamage[i] = s.carDamage[i];
        return sendBytes(reinterpret_cast<const char*>(&p), sizeof(p));
    }

    bool sendJson(const UdpTelemSample& s) {
        char buf[1280];
        int n = std::snprintf(buf, sizeof(buf),
            "{\"seq\":%u,\"t\":%.3f,\"v\":%.1f,\"rpm\":%.0f,\"dmg\":%.3f,\"engH\":%.2f,\"pwr\":%.2f,"
            "\"warn\":%d,\"seized\":%d,\"cd\":[%.2f,%.2f,%.2f,%.2f,%.2f]}\n",
            m_seq, s.timeSec, s.speedMs * 3.6, s.rpm, s.damageOverall, s.engineHealth, s.powerMult,
            s.damageWarning, int(s.engineSeized),
            s.carDamage[0], s.carDamage[1], s.carDamage[2], s.carDamage[3], s.carDamage[4]);
        if (n <= 0) return false;
        return sendBytes(buf, static_cast<size_t>(n));
    }

    bool sendBytes(const char* data, size_t n) {
#ifdef _WIN32
        int sent = ::sendto(m_sock, data, static_cast<int>(n), 0,
                            reinterpret_cast<sockaddr*>(&m_dest), sizeof(m_dest));
        return sent == static_cast<int>(n);
#else
        ssize_t sent = ::sendto(m_sock, data, n, 0,
                                reinterpret_cast<sockaddr*>(&m_dest), sizeof(m_dest));
        return sent == static_cast<ssize_t>(n);
#endif
    }

    void closeUnlocked() {
        if (m_sock >= 0) {
#ifdef _WIN32
            closesocket(static_cast<SOCKET>(m_sock));
#else
            ::close(m_sock);
#endif
            m_sock = -1;
        }
        m_ok = false;
    }

    std::mutex m_mutex;
    bool m_ok = false, m_enabled = true, m_json = false, m_wsa = false;
    int m_sock = -1;
    sockaddr_in m_dest{};
    std::string m_host;
    uint16_t m_port = 20777;
    uint32_t m_seq = 0;
};

} // namespace sim
} // namespace ks
