#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../sys/Signal.h"
#include "ChatProtocol.h"
#include "ChatTransport.h"

namespace ks::chat {

class ChatServer
{
public:
    ChatServer();
    ~ChatServer();

    bool start(std::uint16_t port);
    void stop();
    bool isRunning() const { return m_running; }
    std::uint16_t port() const { return m_port; }

    void setMaxUsers(int max) { m_maxUsers = max; }
    void setPassword(const std::string& password) { m_password = password; }

    std::vector<ChatChannel> getChannels() const { return m_channels; }
    std::vector<ChatUser> getConnectedUsers() const;
    int userCount() const { return static_cast<int>(m_users.size()); }

    void createChannel(const std::string& name, ChannelType type = ChannelType::Group);

    ks::Signal<> started;
    ks::Signal<> stopped;
    ks::Signal<const std::string&> error;
    ks::Signal<const std::string&, const std::string&> userConnected;
    ks::Signal<const std::string&> userDisconnected;
    ks::Signal<const std::string&, const std::string&, const std::string&> messageSent;

private:
    void onClientConnected(ks::net::ConnId conn);
    void onClientDisconnected(ks::net::ConnId conn);
    void onTextReceived(ks::net::ConnId conn, const std::string& text);

    void handlePacket(ks::net::ConnId conn, const ProtocolMessage& msg);
    void handleAuth(ks::net::ConnId conn, const ks::Json& payload);
    void handleSync(ks::net::ConnId conn);
    void handleCreateChannel(const ks::Json& payload);
    void handleJoinChannel(const ks::Json& payload, const std::string& userId);
    void handleLeaveChannel(const ks::Json& payload, const std::string& userId);
    void handleMessage(const ks::Json& payload, const std::string& userId);
    void handleEditMessage(const ks::Json& payload, const std::string& userId);
    void handleDeleteMessage(const ks::Json& payload, const std::string& userId);
    void handleReaction(const ks::Json& payload, const std::string& userId);
    void handleTyping(const ks::Json& payload, const std::string& userId);

    void broadcast(const ProtocolMessage& msg, const std::string& excludeUserId = std::string());
    void sendTo(const std::string& userId, const ProtocolMessage& msg);
    void sendToConn(ks::net::ConnId conn, const ProtocolMessage& msg);
    std::string findUserByConn(ks::net::ConnId conn) const;
    std::string generateId() const;

    bool m_running = false;
    std::uint16_t m_port = 0;
    int m_maxUsers = 50;
    std::string m_password;

    std::unique_ptr<ks::net::IServerTransport> m_transport;
    std::map<std::string, ChatUser> m_users;
    std::map<std::string, ks::net::ConnId> m_userSockets;
    std::map<ks::net::ConnId, std::string> m_socketToUser;
    std::vector<ChatChannel> m_channels;
    std::map<std::string, std::vector<ChatMessage>> m_messages;
};

} // namespace ks::chat
