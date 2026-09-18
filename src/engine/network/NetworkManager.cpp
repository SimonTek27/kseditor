#include "NetworkManager.h"
#include "../simulator/SimulationLoop.h"
#include "../simulator/MultiCarManager.h"
#include "Physics/VehiclePhysics.h"
#include "Physics/PhysicsCoreTypes.h"
#include <cstring>

namespace ks::sim::net {

// ============================================================================
// NetworkClient
// ============================================================================

NetworkClient::NetworkClient(QObject* parent) : QObject(parent) {}

NetworkClient::~NetworkClient() {
    disconnect();
}

bool NetworkClient::connect(const QString& address, uint16_t port,
                             const QString& driverName, const QString& carName) {
    if (m_connected) disconnect();

    yojimbo::Address serverAddress(address.toStdString().c_str(), port);

    yojimbo::ClientServerConfig config;
    config.protocolId = PROTOCOL_ID;
    config.numChannels = 2;
    config.channel[0].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    config.channel[0].packetBudget = 256;
    config.channel[1].type = yojimbo::CHANNEL_TYPE_RELIABLE_ORDERED;
    config.channel[1].packetBudget = 1024;

    yojimbo::DefaultAllocator allocator;
    yojimbo::Address clientAddress("0.0.0.0", 0);
    m_client = std::make_unique<yojimbo::Client>(allocator, clientAddress,
                                                  config, m_adapter, 0.0);
    if (!m_client) return false;

    if (!m_client->InsecureConnect(nullptr, 0, serverAddress)) {
        m_client.reset();
        return false;
    }

    m_connected = true;
    m_sendAccumulator = 0;
    m_timeSinceLastPacket = 0;

    qInfo() << "NetworkClient: Connecting to" << address << ":" << port;
    return true;
}

void NetworkClient::disconnect() {
    if (m_client) {
        m_client->Disconnect();
        m_client.reset();
    }
    m_connected = false;
    m_clientId = 0;
}

bool NetworkClient::isConnected() const {
    return m_connected && m_client &&
           m_client->GetClientState() == yojimbo::CLIENT_STATE_CONNECTED;
}

void NetworkClient::update(double dt) {
    if (!m_connected || !m_client) return;

    m_client->SendPackets();
    m_client->ReceivePackets();

    processMessages();

    yojimbo::ClientState state = m_client->GetClientState();
    if (state == yojimbo::CLIENT_STATE_DISCONNECTED ||
        state == yojimbo::CLIENT_STATE_ERROR) {
        m_connected = false;
        emit disconnected("Disconnected from server");
        return;
    }

    m_sendAccumulator += dt;
    if (m_simLoop && m_sendAccumulator >= (1.0 / 60.0) && isConnected()) {
        m_sendAccumulator = 0.0;

        PlayerInputMessage* msg = (PlayerInputMessage*)m_client->CreateMessage(MSG_PLAYER_INPUT);
        if (msg) {
            msg->data.throttle = m_simLoop->inputManager()->throttle();
            msg->data.brake = m_simLoop->inputManager()->brake();
            msg->data.steering = m_simLoop->inputManager()->steer();
            msg->data.frameNumber = m_clientId;
            m_client->SendMessage(CHANNEL_UNRELIABLE, msg);
        }
    }

    m_timeSinceLastPacket += dt;

    yojimbo::NetworkInfo info;
    m_client->GetNetworkInfo(info);
    m_stats.rtt = info.RTT;
    m_stats.packetLoss = info.packetLoss;
    m_stats.sendBandwidth = info.sentBandwidth;
    m_stats.recvBandwidth = info.receivedBandwidth;
}

void NetworkClient::processMessages() {
    for (int channel = 0; channel < 2; ++channel) {
        yojimbo::Message* msg = nullptr;
        while ((msg = m_client->ReceiveMessage(channel)) != nullptr) {
            m_timeSinceLastPacket = 0;
            int type = msg->GetType();

            switch (type) {
                case MSG_SERVER_WELCOME: {
                    auto* welcome = (ServerWelcomeMessage*)msg;
                    m_clientId = welcome->clientId;
                    emit connected(m_clientId);
                    qInfo() << "NetworkClient: Welcome, id=" << m_clientId;
                    break;
                }
                case MSG_SERVER_FULL:
                    emit disconnected("Server is full");
                    break;
                case MSG_PROTOCOL_MISMATCH:
                    emit disconnected("Protocol mismatch");
                    break;
                case MSG_CAR_STATE: {
                    auto* cs = (CarStateMessage*)msg;
                    emit carStateReceived(cs->data.carId, cs->data);
                    break;
                }
                case MSG_CAR_SPAWN: {
                    auto* sp = (CarSpawnMessage*)msg;
                    emit carSpawned(sp->carId, QString::fromUtf8(sp->driverName), sp->clientId);
                    break;
                }
                case MSG_CAR_DESPAWN: {
                    auto* dp = (CarDespawnMessage*)msg;
                    emit carDespawned(dp->carId);
                    break;
                }
                case MSG_CHAT: {
                    auto* ch = (ChatMessage*)msg;
                    emit chatReceived(ch->senderId,
                                     QString::fromUtf8(ch->senderName),
                                     QString::fromUtf8(ch->message));
                    break;
                }
                default: break;
            }

            m_client->ReleaseMessage(msg);
        }
    }
}

void NetworkClient::sendInput(const InputData& input) {
    if (!isConnected()) return;
    PlayerInputMessage* msg = (PlayerInputMessage*)m_client->CreateMessage(MSG_PLAYER_INPUT);
    if (msg) {
        msg->data = input;
        m_client->SendMessage(CHANNEL_UNRELIABLE, msg);
    }
}

// ============================================================================
// NetworkServer
// ============================================================================

NetworkServer::NetworkServer(QObject* parent) : QObject(parent) {}

NetworkServer::~NetworkServer() {
    stop();
}

bool NetworkServer::start(uint16_t port, const QString& serverName, const QString& trackName) {
    if (m_running) stop();

    yojimbo::Address serverAddress("0.0.0.0", port);

    yojimbo::ClientServerConfig config;
    config.protocolId = PROTOCOL_ID;
    config.numChannels = 2;
    config.channel[0].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    config.channel[0].packetBudget = 256;
    config.channel[1].type = yojimbo::CHANNEL_TYPE_RELIABLE_ORDERED;
    config.channel[1].packetBudget = 1024;

    yojimbo::DefaultAllocator allocator;
    m_server = std::make_unique<yojimbo::Server>(allocator, nullptr, serverAddress,
                                                  config, m_adapter, 0.0);
    if (!m_server) return false;

    if (!m_server->Start(MAX_CLIENTS)) {
        m_server.reset();
        return false;
    }

    m_running = true;
    qInfo() << "NetworkServer: Started on port" << port
            << "- Name:" << serverName << "- Track:" << trackName;
    return true;
}

void NetworkServer::stop() {
    if (m_server) {
        m_server->Stop();
        m_server.reset();
    }
    m_running = false;
    for (auto& c : m_clients) c = ClientSlot{};
}

void NetworkServer::update(double dt) {
    if (!m_running || !m_server) return;

    m_server->SendPackets();
    m_server->ReceivePackets();
    processMessages();
}

void NetworkServer::processMessages() {
    for (int ci = 0; ci < m_server->GetMaxClients(); ++ci) {
        if (!m_server->IsClientConnected(ci)) continue;

        for (int ch = 0; ch < 2; ++ch) {
            yojimbo::Message* msg = nullptr;
            while ((msg = m_server->ReceiveMessage(ci, ch)) != nullptr) {
                int type = msg->GetType();

                switch (type) {
                    case MSG_CLIENT_JOIN: {
                        auto* join = (ClientJoinMessage*)msg;
                        if (join->clientVersion != PROTOCOL_VERSION) {
                            if (m_server->CanSendMessage(ci, CHANNEL_RELIABLE)) {
                                auto* reply = (ProtocolMismatchMessage*)m_server->CreateMessage(ci, MSG_PROTOCOL_MISMATCH);
                                if (reply) m_server->SendMessage(ci, CHANNEL_RELIABLE, reply);
                            }
                        } else {
                            m_clients[ci].connected = true;
                            m_clients[ci].driverName = QString::fromUtf8(join->driverName);
                            m_clients[ci].carName = QString::fromUtf8(join->carName);

                            if (m_server->CanSendMessage(ci, CHANNEL_RELIABLE)) {
                                auto* welcome = (ServerWelcomeMessage*)m_server->CreateMessage(ci, MSG_SERVER_WELCOME);
                                if (welcome) {
                                    welcome->clientId = ci;
                                    strncpy(welcome->serverName, "ksEditor Server", sizeof(welcome->serverName) - 1);
                                    strncpy(welcome->trackName, "Unknown", sizeof(welcome->trackName) - 1);
                                    m_server->SendMessage(ci, CHANNEL_RELIABLE, welcome);
                                }
                            }

                            spawnCarForClient(ci, m_clients[ci].driverName, m_clients[ci].carName);
                            emit clientConnected(ci, ci, m_clients[ci].driverName);
                        }
                        break;
                    }
                    case MSG_PLAYER_INPUT: {
                        auto* input = (PlayerInputMessage*)msg;
                        if (m_simLoop) {
                            m_simLoop->applyRemoteInput(ci, input->data);
                        }
                        break;
                    }
                    case MSG_CHAT: {
                        auto* chat = (ChatMessage*)msg;
                        for (int dest = 0; dest < m_server->GetMaxClients(); ++dest) {
                            if (!m_server->IsClientConnected(dest)) continue;
                            if (!m_server->CanSendMessage(dest, CHANNEL_RELIABLE)) continue;
                            auto* echo = (ChatMessage*)m_server->CreateMessage(dest, MSG_CHAT);
                            if (echo) {
                                echo->senderId = ci;
                                strncpy(echo->senderName, m_clients[ci].driverName.toUtf8().constData(), sizeof(echo->senderName) - 1);
                                strncpy(echo->message, chat->message, sizeof(echo->message) - 1);
                                m_server->SendMessage(dest, CHANNEL_RELIABLE, echo);
                            }
                        }
                        break;
                    }
                    default: break;
                }

                m_server->ReleaseMessage(ci, msg);
            }
        }
    }
}

void NetworkServer::broadcastCarState(uint32_t carId, const CarStateData& state) {
    if (!m_running || !m_server) return;

    for (int ci = 0; ci < m_server->GetMaxClients(); ++ci) {
        if (!m_server->IsClientConnected(ci)) continue;
        if (!m_server->CanSendMessage(ci, CHANNEL_UNRELIABLE)) continue;

        CarStateMessage* msg = (CarStateMessage*)m_server->CreateMessage(ci, MSG_CAR_STATE);
        if (msg) {
            msg->data = state;
            msg->data.carId = carId;
            m_server->SendMessage(ci, CHANNEL_UNRELIABLE, msg);
        }
    }
}

void NetworkServer::spawnCarForClient(int clientIndex, const QString& driverName, const QString& carName) {
    if (!m_multiCar) return;

    float spawnZ = -clientIndex * 5.0f;
    uint32_t carId = m_multiCar->addCar(carName, driverName, QVector3D(0, 0.5f, spawnZ), false);
    m_clients[clientIndex].carId = carId;

    for (int dest = 0; dest < m_server->GetMaxClients(); ++dest) {
        if (!m_server->IsClientConnected(dest)) continue;
        if (!m_server->CanSendMessage(dest, CHANNEL_RELIABLE)) continue;
        CarSpawnMessage* msg = (CarSpawnMessage*)m_server->CreateMessage(dest, MSG_CAR_SPAWN);
        if (msg) {
            msg->carId = carId;
            msg->clientId = clientIndex;
            strncpy(msg->driverName, driverName.toUtf8().constData(), sizeof(msg->driverName) - 1);
            strncpy(msg->carName, carName.toUtf8().constData(), sizeof(msg->carName) - 1);
            msg->posX = 0;
            msg->posY = 0.5f;
            msg->posZ = spawnZ;
            m_server->SendMessage(dest, CHANNEL_RELIABLE, msg);
        }
    }
}

int NetworkServer::getClientCount() const {
    if (!m_server) return 0;
    int count = 0;
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (m_server->IsClientConnected(i)) count++;
    }
    return count;
}

QString NetworkServer::getClientName(int clientIndex) const {
    if (clientIndex >= 0 && clientIndex < MAX_CLIENTS)
        return m_clients[clientIndex].driverName;
    return {};
}

uint32_t NetworkServer::getClientCarId(int clientIndex) const {
    if (clientIndex >= 0 && clientIndex < MAX_CLIENTS)
        return m_clients[clientIndex].carId;
    return 0;
}

} // namespace ks::sim::net