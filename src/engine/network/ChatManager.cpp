#include "ChatManager.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "../sys/LogManager.h"
#include "NetUtil.h"

namespace ks::chat {

using net::nowMs;

namespace {

namespace fs = std::filesystem;

// QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) with
// organisation "ksEditor" and application "ksEditor".
std::string appDataLocation() {
#ifdef _WIN32
    const char* base = std::getenv("APPDATA");
    fs::path dir = base ? fs::path(base) : fs::temp_directory_path();
#else
    const char* xdg = std::getenv("XDG_DATA_HOME");
    fs::path dir;
    if (xdg && *xdg) {
        dir = xdg;
    } else if (const char* home = std::getenv("HOME")) {
        dir = fs::path(home) / ".local" / "share";
    } else {
        dir = fs::temp_directory_path();
    }
#endif
    dir /= "ksEditor";
    dir /= "ksEditor";
    std::error_code ec;
    fs::create_directories(dir, ec);
    return dir.string();
}

std::string readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return std::string();
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool writeFile(const std::string& path, const std::string& data) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) return false;
    file.write(data.data(), static_cast<std::streamsize>(data.size()));
    return static_cast<bool>(file);
}

} // namespace

ChatManager* ChatManager::s_instance = nullptr;

ChatManager* ChatManager::instance() {
    if (!s_instance) {
        s_instance = new ChatManager();
    }
    return s_instance;
}

ChatManager::ChatManager() {
    loadLocalData();
    if (m_userId.empty()) m_userId = generateId();
}

ChatManager::~ChatManager() {
    saveLocalData();
    cancelReconnect();
    if (m_client) {
        m_client->close();
    }
}

void ChatManager::connectToServer(const std::string& host, std::uint16_t port,
                                  const std::string& userName) {
    m_userName = userName;
    m_lastHost = host;
    m_lastPort = port;
    m_reconnectAttempts = 0;
    m_autoReconnect = true;
    cancelReconnect();

    m_client = ks::net::makeClientTransport();
    m_client->onOpened = [this]() { onTransportOpened(); };
    m_client->onClosed = [this](const std::string& reason) { onTransportClosed(reason); };
    m_client->onTextReceived = [this](const std::string& text) { onTextReceived(text); };
    m_client->onError = [this](const std::string& message) { onTransportError(message); };

    LOG_INFO("ChatManager", "Connecting to " + host + ":" + std::to_string(port));
    m_client->open(host, port, std::string());
}

void ChatManager::disconnectFromServer() {
    m_autoReconnect = false;
    cancelReconnect();
    if (m_client) {
        m_client->close();
    }
}

void ChatManager::onTransportOpened() {
    m_connected = true;
    m_reconnectAttempts = 0;
    cancelReconnect();

    ProtocolMessage auth = ProtocolMessage::make("auth", makeObject({
        {"userId", m_userId},
        {"userName", m_userName}
    }));
    sendPacket(auth);

    LOG_INFO("ChatManager", "Connected, sent auth");
    connected();
}

void ChatManager::onTransportClosed(const std::string&) {
    bool wasConnected = m_connected;
    m_connected = false;

    if (wasConnected) {
        LOG_INFO("ChatManager", "Disconnected");
        disconnected();
    }

    if (m_autoReconnect && m_reconnectAttempts < 5) {
        scheduleReconnect();
    }
}

void ChatManager::onTransportError(const std::string& message) {
    LOG_WARNING("ChatManager", message);
    connectionError(message);
}

void ChatManager::onTextReceived(const std::string& text) {
    bool ok = false;
    ks::Json doc = ks::Json::parse(text, &ok);
    if (!ok || !doc.isObject()) return;

    ProtocolMessage msg = ProtocolMessage::fromJson(doc);
    handleMessage(msg);
}

void ChatManager::scheduleReconnect() {
    cancelReconnect();
    const int delayMs = 3000 * (m_reconnectAttempts + 1);
    m_reconnectTimer = ks::net::startTimer(delayMs, [this]() {
        m_reconnectTimer = 0;
        if (m_connected) return;

        if (m_autoReconnect && m_reconnectAttempts < 5) {
            m_reconnectAttempts++;
            LOG_INFO("ChatManager", "Reconnect attempt " + std::to_string(m_reconnectAttempts));
            if (m_client) m_client->open(m_lastHost, m_lastPort, std::string());
        }

        if (!m_connected && m_autoReconnect && m_reconnectAttempts < 5) {
            scheduleReconnect();
        }
    });
}

void ChatManager::cancelReconnect() {
    if (m_reconnectTimer != 0) {
        ks::net::killTimer(m_reconnectTimer);
        m_reconnectTimer = 0;
    }
}

void ChatManager::handleMessage(const ProtocolMessage& msg) {
    if (msg.type == "auth_ok") {
        handleAuthResponse(msg.payload);
    } else if (msg.type == "channel_created") {
        handleChannelCreated(msg.payload);
    } else if (msg.type == "channel_joined") {
        handleChannelJoined(msg.payload);
    } else if (msg.type == "message") {
        handleMessageReceived(msg.payload);
    } else if (msg.type == "message_edited") {
        handleMessageEdited(msg.payload);
    } else if (msg.type == "message_deleted") {
        handleMessageDeleted(msg.payload);
    } else if (msg.type == "user_presence") {
        handleUserPresence(msg.payload);
    } else if (msg.type == "reaction") {
        handleReaction(msg.payload);
    } else if (msg.type == "typing") {
        handleTyping(msg.payload);
    } else if (msg.type == "sync") {
        const ks::Json& channelsArr = msg.payload["channels"];
        for (int i = 0; i < channelsArr.size(); ++i) {
            ChatChannel ch = ChatChannel::fromJson(channelsArr.at(i));
            bool found = false;
            for (auto& existing : m_channels) {
                if (existing.id == ch.id) { existing = ch; found = true; break; }
            }
            if (!found) m_channels.push_back(ch);
        }
    }
}

void ChatManager::handleAuthResponse(const ks::Json& payload) {
    const ks::Json& usersArr = payload["users"];
    for (int i = 0; i < usersArr.size(); ++i) {
        ChatUser u = ChatUser::fromJson(usersArr.at(i));
        m_users[u.id] = u;
        userJoined(u);
    }

    requestSync();
    LOG_INFO("ChatManager", "Auth OK, synced " + std::to_string(m_channels.size()) + " channels");
}

void ChatManager::handleChannelCreated(const ks::Json& payload) {
    ChatChannel ch = ChatChannel::fromJson(payload);
    m_channels.push_back(ch);
    channelCreated(ch);
}

void ChatManager::handleChannelJoined(const ks::Json& payload) {
    std::string channelId = payload["channelId"].toString();
    std::string userId = payload["userId"].toString();
    std::string userName = payload["userName"].toString();

    for (auto& ch : m_channels) {
        if (ch.id == channelId) {
            if (!vectorContains(ch.members, userId)) {
                ch.members.push_back(userId);
            }
            break;
        }
    }

    if (m_users.find(userId) == m_users.end()) {
        ChatUser u;
        u.id = userId;
        u.name = userName;
        u.color = Color::fromHsv(ks::net::randomInt(0, 359), 180, 200);
        u.status = UserStatus::Online;
        m_users[userId] = u;
        userJoined(u);
    }
}

void ChatManager::handleMessageReceived(const ks::Json& payload) {
    ChatMessage msg = ChatMessage::fromJson(payload);
    m_messages[msg.channelId].push_back(msg);
    messageReceived(msg);

    for (auto& ch : m_channels) {
        if (ch.id == msg.channelId && msg.channelId != m_activeChannelId) {
            ch.unreadCount++;
            unreadCountChanged(ch.id, ch.unreadCount);
            break;
        }
    }
}

void ChatManager::handleMessageEdited(const ks::Json& payload) {
    ChatMessage msg = ChatMessage::fromJson(payload);
    auto& msgs = m_messages[msg.channelId];
    for (auto& m : msgs) {
        if (m.id == msg.id) {
            m = msg;
            messageEdited(msg);
            return;
        }
    }
}

void ChatManager::handleMessageDeleted(const ks::Json& payload) {
    std::string msgId = payload["messageId"].toString();
    std::string channelId = payload["channelId"].toString();
    auto& msgs = m_messages[channelId];
    for (auto& m : msgs) {
        if (m.id == msgId) {
            m.isDeleted = true;
            messageDeleted(msgId);
            return;
        }
    }
}

void ChatManager::handleUserPresence(const ks::Json& payload) {
    std::string userId = payload["userId"].toString();
    UserStatus status = static_cast<UserStatus>(payload["status"].toInt());

    auto it = m_users.find(userId);
    if (it != m_users.end()) {
        it->second.status = status;
        userStatusChanged(userId, status);
    }
}

void ChatManager::handleReaction(const ks::Json& payload) {
    std::string msgId = payload["messageId"].toString();
    std::string emoji = payload["emoji"].toString();
    std::string userId = payload["userId"].toString();
    bool added = payload["added"].toBool();

    for (auto& entry : m_messages) {
        for (auto& m : entry.second) {
            if (m.id == msgId) {
                if (added) {
                    if (!vectorContains(m.reactions[emoji], userId))
                        m.reactions[emoji].push_back(userId);
                    reactionAdded(msgId, emoji, userId);
                } else {
                    vectorRemove(m.reactions[emoji], userId);
                    if (m.reactions[emoji].empty())
                        m.reactions.erase(emoji);
                    reactionRemoved(msgId, emoji, userId);
                }
                return;
            }
        }
    }
}

void ChatManager::handleTyping(const ks::Json& payload) {
    std::string channelId = payload["channelId"].toString();
    std::string userId = payload["userId"].toString();
    typingReceived(channelId, userId);
}

void ChatManager::sendPacket(const ProtocolMessage& msg) {
    if (m_client && m_connected) {
        m_client->sendText(msg.toJson().dump());
    }
}

void ChatManager::requestSync() {
    sendPacket(ProtocolMessage::make("sync", makeObject({})));
}

// --- Public API ---

void ChatManager::createChannel(const std::string& name, ChannelType type) {
    sendPacket(ProtocolMessage::make("create_channel", makeObject({
        {"name", name},
        {"type", static_cast<int>(type)}
    })));
}

void ChatManager::joinChannel(const std::string& channelId) {
    sendPacket(ProtocolMessage::make("join_channel", makeObject({{"channelId", channelId}})));
}

void ChatManager::leaveChannel(const std::string& channelId) {
    sendPacket(ProtocolMessage::make("leave_channel", makeObject({{"channelId", channelId}})));
}

void ChatManager::setActiveChannel(const std::string& channelId) {
    if (m_activeChannelId != channelId) {
        m_activeChannelId = channelId;

        for (auto& ch : m_channels) {
            if (ch.id == channelId && ch.unreadCount > 0) {
                ch.unreadCount = 0;
                unreadCountChanged(channelId, 0);
            }
        }

        activeChannelChanged(channelId);
    }
}

void ChatManager::sendMessage(const std::string& channelId, const std::string& content,
                              MessageType type, const std::string& replyTo) {
    ChatMessage msg;
    msg.id = generateId();
    msg.channelId = channelId;
    msg.authorId = m_userId;
    msg.authorName = m_userName;
    msg.type = type;
    msg.content = content;
    msg.timestamp = nowMs();
    msg.replyToId = replyTo;

    m_messages[channelId].push_back(msg);
    messageReceived(msg);

    sendPacket(ProtocolMessage::make("message", msg.toJson()));
}

void ChatManager::editMessage(const std::string& messageId, const std::string& newContent) {
    for (auto& entry : m_messages) {
        for (auto& m : entry.second) {
            if (m.id == messageId) {
                m.content = newContent;
                m.editedAt = nowMs();
                messageEdited(m);
                sendPacket(ProtocolMessage::make("edit_message", m.toJson()));
                return;
            }
        }
    }
}

void ChatManager::deleteMessage(const std::string& messageId) {
    for (auto it = m_messages.begin(); it != m_messages.end(); ++it) {
        for (auto& m : it->second) {
            if (m.id == messageId) {
                m.isDeleted = true;
                messageDeleted(messageId);
                ks::Json payload = ks::Json::object();
                payload["messageId"] = messageId;
                payload["channelId"] = it->first;
                sendPacket(ProtocolMessage::make("delete_message", payload));
                return;
            }
        }
    }
}

void ChatManager::addReaction(const std::string& messageId, const std::string& emoji) {
    for (auto& entry : m_messages) {
        for (auto& m : entry.second) {
            if (m.id == messageId) {
                if (!vectorContains(m.reactions[emoji], m_userId)) {
                    m.reactions[emoji].push_back(m_userId);
                    reactionAdded(messageId, emoji, m_userId);
                }
                sendPacket(ProtocolMessage::make("reaction", makeObject({
                    {"messageId", messageId},
                    {"emoji", emoji},
                    {"added", true}
                })));
                return;
            }
        }
    }
}

void ChatManager::removeReaction(const std::string& messageId, const std::string& emoji) {
    for (auto& entry : m_messages) {
        for (auto& m : entry.second) {
            if (m.id == messageId) {
                vectorRemove(m.reactions[emoji], m_userId);
                if (m.reactions[emoji].empty())
                    m.reactions.erase(emoji);
                reactionRemoved(messageId, emoji, m_userId);
                sendPacket(ProtocolMessage::make("reaction", makeObject({
                    {"messageId", messageId},
                    {"emoji", emoji},
                    {"added", false}
                })));
                return;
            }
        }
    }
}

void ChatManager::sendTyping(const std::string& channelId) {
    sendPacket(ProtocolMessage::make("typing", makeObject({{"channelId", channelId}})));
}

void ChatManager::setUserName(const std::string& name) {
    if (m_userName != name) {
        m_userName = name;
        if (m_connected) {
            sendPacket(ProtocolMessage::make("set_name", makeObject({{"userName", name}})));
        }
    }
}

std::vector<ChatMessage> ChatManager::getMessages(const std::string& channelId) const {
    auto it = m_messages.find(channelId);
    if (it != m_messages.end()) return it->second;
    return std::vector<ChatMessage>();
}

std::vector<ChatUser> ChatManager::getUsers() const {
    std::vector<ChatUser> users;
    users.reserve(m_users.size());
    for (const auto& entry : m_users) users.push_back(entry.second);
    return users;
}

ChatUser ChatManager::getUser(const std::string& userId) const {
    auto it = m_users.find(userId);
    if (it != m_users.end()) return it->second;
    return ChatUser();
}

std::vector<ChatChannel> ChatManager::getDirectChannels() const {
    std::vector<ChatChannel> result;
    for (const auto& ch : m_channels) {
        if (ch.type == ChannelType::Direct) result.push_back(ch);
    }
    return result;
}

std::vector<ChatChannel> ChatManager::getGroupChannels() const {
    std::vector<ChatChannel> result;
    for (const auto& ch : m_channels) {
        if (ch.type == ChannelType::Group) result.push_back(ch);
    }
    return result;
}

std::vector<ChatChannel> ChatManager::getProjectChannels() const {
    std::vector<ChatChannel> result;
    for (const auto& ch : m_channels) {
        if (ch.type == ChannelType::Project) result.push_back(ch);
    }
    return result;
}

std::string ChatManager::generateId() const {
    return ks::net::randomId();
}

void ChatManager::saveLocalData() {
    ks::Json obj = ks::Json::object();
    obj["userId"] = m_userId;
    obj["userName"] = m_userName;

    ks::Json chArr = ks::Json::array();
    for (const auto& ch : m_channels) chArr.append(ch.toJson());
    obj["channels"] = chArr;

    writeFile(appDataLocation() + "/kschat_local.json", obj.dump(true));
}

void ChatManager::loadLocalData() {
    const std::string text = readFile(appDataLocation() + "/kschat_local.json");
    if (text.empty()) return;

    bool ok = false;
    ks::Json doc = ks::Json::parse(text, &ok);
    if (!ok || !doc.isObject()) return;

    m_userId = doc["userId"].toString();
    if (m_userId.empty()) m_userId = generateId();
    m_userName = doc["userName"].toString();

    const ks::Json& channels = doc["channels"];
    for (int i = 0; i < channels.size(); ++i) {
        m_channels.push_back(ChatChannel::fromJson(channels.at(i)));
    }
}

} // namespace ks::chat
