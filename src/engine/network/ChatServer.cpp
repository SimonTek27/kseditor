#include "ChatServer.h"

#include <algorithm>

#include "../sys/LogManager.h"
#include "NetUtil.h"

namespace ks::chat {

using net::nowMs;

namespace {

std::string mapValue(const std::map<std::string, ChatUser>& users, const std::string& userId) {
    auto it = users.find(userId);
    return it != users.end() ? it->second.name : std::string();
}

} // namespace

ChatServer::ChatServer() = default;

ChatServer::~ChatServer() {
    stop();
    m_transport.reset();
}

bool ChatServer::start(std::uint16_t port) {
    if (m_running) return false;

    m_port = port;
    m_transport = ks::net::makeServerTransport();
    m_transport->onClientConnected = [this](ks::net::ConnId conn) { onClientConnected(conn); };
    m_transport->onClientDisconnected = [this](ks::net::ConnId conn) { onClientDisconnected(conn); };
    m_transport->onTextReceived = [this](ks::net::ConnId conn, const std::string& text) {
        onTextReceived(conn, text);
    };

    std::string listenError;
    if (!m_transport->listen(port, listenError)) {
        error(listenError);
        return false;
    }

    m_running = true;

    // Create default channels
    bool hasGeneral = false;
    for (const auto& ch : m_channels) {
        if (ch.name == "general") { hasGeneral = true; break; }
    }
    if (!hasGeneral) {
        ChatChannel general;
        general.id = generateId();
        general.name = "general";
        general.description = "General discussion";
        general.type = ChannelType::Group;
        general.owner = "system";
        general.createdAt = nowMs();
        m_channels.push_back(general);
    }

    started();
    LOG_INFO("ChatServer", "Started on port " + std::to_string(port));
    return true;
}

void ChatServer::stop() {
    if (!m_running) return;

    if (m_transport) {
        m_transport->close();
    }
    m_users.clear();
    m_userSockets.clear();
    m_socketToUser.clear();
    m_running = false;

    stopped();
    LOG_INFO("ChatServer", "Stopped");
}

void ChatServer::onClientConnected(ks::net::ConnId conn) {
    if (static_cast<int>(m_users.size()) >= m_maxUsers) {
        sendToConn(conn, ProtocolMessage::make("error", makeObject({{"message", "Server is full"}})));
        if (m_transport) m_transport->closeConnection(conn);
    }
}

void ChatServer::onClientDisconnected(ks::net::ConnId conn) {
    std::string userId;
    auto it = m_socketToUser.find(conn);
    if (it != m_socketToUser.end()) {
        userId = it->second;
        m_socketToUser.erase(it);
    }

    if (!userId.empty()) {
        m_users.erase(userId);
        m_userSockets.erase(userId);

        // Notify others
        broadcast(ProtocolMessage::make("user_presence", makeObject({
            {"userId", userId},
            {"status", static_cast<int>(UserStatus::Offline)}
        })), userId);

        // Remove from channels
        for (auto& ch : m_channels) {
            vectorRemove(ch.members, userId);
        }

        userDisconnected(userId);
        LOG_INFO("ChatServer", "User " + userId + " disconnected");
    }
}

void ChatServer::onTextReceived(ks::net::ConnId conn, const std::string& text) {
    bool ok = false;
    ks::Json doc = ks::Json::parse(text, &ok);
    if (!ok || !doc.isObject()) return;

    ProtocolMessage msg = ProtocolMessage::fromJson(doc);
    handlePacket(conn, msg);
}

void ChatServer::handlePacket(ks::net::ConnId conn, const ProtocolMessage& msg) {
    std::string userId = findUserByConn(conn);

    if (msg.type == "auth") {
        handleAuth(conn, msg.payload);
    } else if (userId.empty()) {
        sendToConn(conn, ProtocolMessage::make("error", makeObject({{"message", "Not authenticated"}})));
        return;
    } else if (msg.type == "sync") {
        handleSync(conn);
    } else if (msg.type == "create_channel") {
        handleCreateChannel(msg.payload);
    } else if (msg.type == "join_channel") {
        handleJoinChannel(msg.payload, userId);
    } else if (msg.type == "leave_channel") {
        handleLeaveChannel(msg.payload, userId);
    } else if (msg.type == "message") {
        handleMessage(msg.payload, userId);
    } else if (msg.type == "edit_message") {
        handleEditMessage(msg.payload, userId);
    } else if (msg.type == "delete_message") {
        handleDeleteMessage(msg.payload, userId);
    } else if (msg.type == "reaction") {
        handleReaction(msg.payload, userId);
    } else if (msg.type == "typing") {
        handleTyping(msg.payload, userId);
    }
}

void ChatServer::handleAuth(ks::net::ConnId conn, const ks::Json& payload) {
    std::string userId = payload["userId"].toString();
    std::string userName = payload["userName"].toString();

    if (userId.empty() || userName.empty()) {
        sendToConn(conn, ProtocolMessage::make("error", makeObject({{"message", "Invalid auth"}})));
        return;
    }

    ChatUser user;
    user.id = userId;
    user.name = userName;
    user.color = Color::fromHsv(ks::net::randomInt(0, 359), 180, 200);
    user.status = UserStatus::Online;
    user.lastSeen = nowMs();

    m_users[userId] = user;
    m_userSockets[userId] = conn;
    m_socketToUser[conn] = userId;

    // Send auth response with current users
    ks::Json usersArr = ks::Json::array();
    for (const auto& entry : m_users) {
        usersArr.append(entry.second.toJson());
    }

    sendToConn(conn, ProtocolMessage::make("auth_ok", makeObject({
        {"users", usersArr}
    })));

    // Notify others
    broadcast(ProtocolMessage::make("user_presence", makeObject({
        {"userId", userId},
        {"status", static_cast<int>(UserStatus::Online)},
        {"userName", userName}
    })), userId);

    userConnected(userId, userName);
    LOG_INFO("ChatServer", "User " + userName + " authenticated");
}

void ChatServer::handleSync(ks::net::ConnId conn) {
    ks::Json channelsArr = ks::Json::array();
    for (const auto& ch : m_channels) {
        channelsArr.append(ch.toJson());
    }

    sendToConn(conn, ProtocolMessage::make("sync", makeObject({
        {"channels", channelsArr}
    })));
}

void ChatServer::handleCreateChannel(const ks::Json& payload) {
    ChatChannel ch;
    ch.id = generateId();
    ch.name = payload["name"].toString();
    ch.type = static_cast<ChannelType>(payload["type"].toInt());
    ch.owner = "system";
    ch.createdAt = nowMs();

    m_channels.push_back(ch);
    broadcast(ProtocolMessage::make("channel_created", ch.toJson()));
}

void ChatServer::handleJoinChannel(const ks::Json& payload, const std::string& userId) {
    std::string channelId = payload["channelId"].toString();

    for (auto& ch : m_channels) {
        if (ch.id == channelId && !vectorContains(ch.members, userId)) {
            ch.members.push_back(userId);
            broadcast(ProtocolMessage::make("channel_joined", makeObject({
                {"channelId", channelId},
                {"userId", userId},
                {"userName", mapValue(m_users, userId)}
            })));
            break;
        }
    }
}

void ChatServer::handleLeaveChannel(const ks::Json& payload, const std::string& userId) {
    std::string channelId = payload["channelId"].toString();

    for (auto& ch : m_channels) {
        if (ch.id == channelId) {
            vectorRemove(ch.members, userId);
            broadcast(ProtocolMessage::make("channel_left", makeObject({
                {"channelId", channelId},
                {"userId", userId}
            })));
            break;
        }
    }
}

void ChatServer::handleMessage(const ks::Json& payload, const std::string& userId) {
    ChatMessage msg = ChatMessage::fromJson(payload);
    msg.authorId = userId;
    msg.authorName = mapValue(m_users, userId);
    msg.timestamp = nowMs();

    m_messages[msg.channelId].push_back(msg);
    broadcast(ProtocolMessage::make("message", msg.toJson()));
    messageSent(msg.channelId, userId, msg.content);
}

void ChatServer::handleEditMessage(const ks::Json& payload, const std::string& userId) {
    std::string msgId = payload["id"].toString();
    std::string channelId = payload["channelId"].toString();
    std::string newContent = payload["content"].toString();

    auto& msgs = m_messages[channelId];
    for (auto& m : msgs) {
        if (m.id == msgId && m.authorId == userId) {
            m.content = newContent;
            m.editedAt = nowMs();
            broadcast(ProtocolMessage::make("message_edited", m.toJson()));
            return;
        }
    }
}

void ChatServer::handleDeleteMessage(const ks::Json& payload, const std::string& userId) {
    std::string msgId = payload["messageId"].toString();
    std::string channelId = payload["channelId"].toString();

    auto& msgs = m_messages[channelId];
    for (auto& m : msgs) {
        if (m.id == msgId && m.authorId == userId) {
            m.isDeleted = true;
            broadcast(ProtocolMessage::make("message_deleted", makeObject({
                {"messageId", msgId},
                {"channelId", channelId}
            })));
            return;
        }
    }
}

void ChatServer::handleReaction(const ks::Json& payload, const std::string& userId) {
    std::string msgId = payload["messageId"].toString();
    std::string emoji = payload["emoji"].toString();
    bool added = payload["added"].toBool();

    // Update local state and broadcast
    for (auto& entry : m_messages) {
        for (auto& m : entry.second) {
            if (m.id == msgId) {
                if (added) {
                    if (!vectorContains(m.reactions[emoji], userId))
                        m.reactions[emoji].push_back(userId);
                } else {
                    vectorRemove(m.reactions[emoji], userId);
                    if (m.reactions[emoji].empty())
                        m.reactions.erase(emoji);
                }
                break;
            }
        }
    }

    broadcast(ProtocolMessage::make("reaction", makeObject({
        {"messageId", msgId},
        {"emoji", emoji},
        {"userId", userId},
        {"added", added}
    })));
}

void ChatServer::handleTyping(const ks::Json& payload, const std::string& userId) {
    std::string channelId = payload["channelId"].toString();

    for (auto& ch : m_channels) {
        if (ch.id == channelId) {
            for (const auto& memberId : ch.members) {
                if (memberId != userId) {
                    sendTo(memberId, ProtocolMessage::make("typing", makeObject({
                        {"channelId", channelId},
                        {"userId", userId}
                    })));
                }
            }
            break;
        }
    }
}

void ChatServer::broadcast(const ProtocolMessage& msg, const std::string& excludeUserId) {
    const std::string data = msg.toJson().dump();

    for (const auto& entry : m_userSockets) {
        if (entry.first != excludeUserId && m_transport) {
            m_transport->sendText(entry.second, data);
        }
    }
}

void ChatServer::sendTo(const std::string& userId, const ProtocolMessage& msg) {
    auto it = m_userSockets.find(userId);
    if (it != m_userSockets.end() && m_transport) {
        m_transport->sendText(it->second, msg.toJson().dump());
    }
}

void ChatServer::sendToConn(ks::net::ConnId conn, const ProtocolMessage& msg) {
    if (m_transport) {
        m_transport->sendText(conn, msg.toJson().dump());
    }
}

std::string ChatServer::findUserByConn(ks::net::ConnId conn) const {
    auto it = m_socketToUser.find(conn);
    return it != m_socketToUser.end() ? it->second : std::string();
}

std::vector<ChatUser> ChatServer::getConnectedUsers() const {
    std::vector<ChatUser> users;
    users.reserve(m_users.size());
    for (const auto& entry : m_users) users.push_back(entry.second);
    return users;
}

void ChatServer::createChannel(const std::string& name, ChannelType type) {
    ChatChannel ch;
    ch.id = generateId();
    ch.name = name;
    ch.type = type;
    ch.owner = "system";
    ch.createdAt = nowMs();

    m_channels.push_back(ch);
    broadcast(ProtocolMessage::make("channel_created", ch.toJson()));
}

std::string ChatServer::generateId() const {
    return ks::net::randomId();
}

} // namespace ks::chat
