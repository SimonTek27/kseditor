#pragma once

#include "NetworkConfig.h"
#include <QObject>
#include <memory>
#include <map>

namespace ks::sim {

class SimulationLoop;
class MultiCarManager;

namespace net {

// ============================================================================
// NetworkClient
// ============================================================================

class NetworkClient : public QObject {
    Q_OBJECT
public:
    explicit NetworkClient(QObject* parent = nullptr);
    ~NetworkClient() override;

    bool connect(const QString& address, uint16_t port,
                 const QString& driverName, const QString& carName);
    void disconnect();
    bool isConnected() const;

    void setSimulationLoop(SimulationLoop* loop) { m_simLoop = loop; }
    void setMultiCarManager(MultiCarManager* m) { m_multiCar = m; }

    void update(double dt);
    void sendInput(const InputData& input);

    NetworkStats getStats() const { return m_stats; }

signals:
    void connected(uint32_t clientId);
    void disconnected(const QString& reason);
    void carStateReceived(uint32_t carId, const CarStateData& state);
    void carSpawned(uint32_t carId, const QString& name, uint32_t clientId);
    void carDespawned(uint32_t carId);
    void chatReceived(uint32_t senderId, const QString& name, const QString& msg);

private:
    void processMessages();

    std::unique_ptr<yojimbo::Client> m_client;
    GameAdapter m_adapter;

    bool m_connected = false;
    uint32_t m_clientId = 0;
    double m_sendAccumulator = 0;
    double m_timeSinceLastPacket = 0;

    SimulationLoop* m_simLoop = nullptr;
    MultiCarManager* m_multiCar = nullptr;
    NetworkStats m_stats;
};

// ============================================================================
// NetworkServer
// ============================================================================

class NetworkServer : public QObject {
    Q_OBJECT
public:
    explicit NetworkServer(QObject* parent = nullptr);
    ~NetworkServer() override;

    bool start(uint16_t port, const QString& serverName, const QString& trackName);
    void stop();
    bool isRunning() const { return m_running; }

    void setSimulationLoop(SimulationLoop* loop) { m_simLoop = loop; }
    void setMultiCarManager(MultiCarManager* m) { m_multiCar = m; }

    void update(double dt);
    void broadcastCarState(uint32_t carId, const CarStateData& state);

    int getClientCount() const;
    QString getClientName(int clientIndex) const;
    uint32_t getClientCarId(int clientIndex) const;

signals:
    void clientConnected(int clientIndex, uint32_t clientId, const QString& name);
    void clientDisconnected(int clientIndex, const QString& reason);

private:
    void processMessages();
    void spawnCarForClient(int clientIndex, const QString& driverName, const QString& carName);

    struct ClientSlot {
        bool connected = false;
        uint32_t carId = 0;
        QString driverName;
        QString carName;
    };

    std::unique_ptr<yojimbo::Server> m_server;
    GameAdapter m_adapter;

    bool m_running = false;
    ClientSlot m_clients[MAX_CLIENTS];
    SimulationLoop* m_simLoop = nullptr;
    MultiCarManager* m_multiCar = nullptr;
};

} // namespace net
} // namespace ks::sim