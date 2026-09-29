#include "Collaboration.h"

#include <algorithm>

#include "../sys/LogManager.h"
#include "ChatProtocol.h"
#include "NetUtil.h"

namespace ks {

using chat::makeObject;
using chat::vectorContains;
using chat::vectorRemove;

// ---------------------------------------------------------------------------
// CollaborationClient
// ---------------------------------------------------------------------------

CollaborationClient* CollaborationClient::s_instance = nullptr;

CollaborationClient* CollaborationClient::instance()
{
    if (!s_instance) s_instance = new CollaborationClient();
    return s_instance;
}

CollaborationClient::CollaborationClient()
    : m_state(CollaborationState::Disconnected)
{
}

CollaborationClient::~CollaborationClient() { s_instance = nullptr; }

void CollaborationClient::setServer(const std::string& host, std::uint16_t port)
{
    m_host = host;
    m_port = port;
}

void CollaborationClient::setUserInfo(const std::string& userId, const std::string& userName)
{
    m_userId   = userId;
    m_userName = userName;
}

bool CollaborationClient::connect()
{
    if (m_state == CollaborationState::Connected) return true;
    m_state = CollaborationState::Connecting;
    stateChanged(m_state);
    doConnect();
    return true;
}

void CollaborationClient::doConnect()
{
    if (m_host.empty()) { error("Server host not set"); return; }

    m_socket = ks::net::makeClientTransport();
    m_socket->onOpened = [this]() { onTransportOpened(); };
    m_socket->onClosed = [this](const std::string&) { onTransportClosed(); };
    m_socket->onTextReceived = [this](const std::string& text) { onMessage(text); };
    m_socket->onError = [this](const std::string& message) {
        LOG_WARNING("Collaboration", message);
        error(message);
    };
    m_socket->open(m_host, m_port, "/collab");
}

void CollaborationClient::onTransportOpened()
{
    m_state = CollaborationState::Connected;
    m_reconnectAttempts = 0;
    if (m_reconnectTimer != 0) {
        ks::net::killTimer(m_reconnectTimer);
        m_reconnectTimer = 0;
    }

    sendAuth();
    stateChanged(m_state);
    connected();
    LOG_INFO("Collaboration", "Connected to " + m_host + ":" + std::to_string(m_port));
}

void CollaborationClient::onTransportClosed()
{
    m_state = CollaborationState::Disconnected;
    stateChanged(m_state);
    disconnected();
    if (m_autoReconnect) scheduleReconnect();
}

void CollaborationClient::disconnect()
{
    m_autoReconnect = false;
    if (m_reconnectTimer != 0) {
        ks::net::killTimer(m_reconnectTimer);
        m_reconnectTimer = 0;
    }
    m_pingTimer.stop();
    if (m_socket) m_socket->close();
}

void CollaborationClient::joinDocument(const std::string& docId)
{
    if (!isConnected()) return;
    m_openDocuments.insert(docId);
    ks::Json payload = ks::Json::object();
    payload["docId"] = docId;
    sendPacket("join", payload);
}

void CollaborationClient::leaveDocument(const std::string& docId)
{
    if (!isConnected()) return;
    m_openDocuments.erase(docId);
    ks::Json payload = ks::Json::object();
    payload["docId"] = docId;
    sendPacket("leave", payload);
}

void CollaborationClient::sendChange(const std::string& docId, const ks::Json& change)
{
    if (!isConnected()) return;
    ks::Json payload = ks::Json::object();
    payload["docId"]  = docId;
    payload["change"] = change;
    payload["version"] = ++m_localVersion;
    sendPacket("change", payload);
}

void CollaborationClient::sendCursor(const std::string& docId, const ks::Json& cursor)
{
    if (!isConnected()) return;
    ks::Json payload = ks::Json::object();
    payload["docId"]  = docId;
    payload["cursor"] = cursor;
    sendPacket("cursor", payload);
}

void CollaborationClient::sendSelection(const std::string& docId, const ks::Json& selection)
{
    if (!isConnected()) return;
    ks::Json payload = ks::Json::object();
    payload["docId"]     = docId;
    payload["selection"] = selection;
    sendPacket("selection", payload);
}

void CollaborationClient::sendChat(const std::string& docId, const std::string& message)
{
    if (!isConnected()) return;
    ks::Json payload = ks::Json::object();
    payload["docId"]   = docId;
    payload["message"] = message;
    sendPacket("chat", payload);
}

void CollaborationClient::setPresence(const std::string& status)
{
    m_presence = status;
    if (isConnected()) {
        ks::Json payload = ks::Json::object();
        payload["status"] = status;
        sendPacket("presence", payload);
    }
}

void CollaborationClient::setAutoReconnect(bool enabled)
{
    m_autoReconnect = enabled;
}

std::vector<CollaborationUser> CollaborationClient::getUsers(const std::string& docId) const
{
    std::vector<CollaborationUser> users;
    for (const auto& entry : m_activeUsers) {
        auto doc = m_userDocuments.find(entry.first);
        if (doc != m_userDocuments.end() && doc->second == docId) users.push_back(entry.second);
    }
    return users;
}

std::vector<Change> CollaborationClient::getChanges(const std::string& docId, int fromVersion) const
{
    auto it = m_documents.find(docId);
    if (it == m_documents.end()) return std::vector<Change>();

    const CollaborationDocument& doc = it->second;
    std::vector<Change> filtered;

    for (const Change& change : doc.changes) {
        if (change.version > fromVersion) {
            filtered.push_back(change);
        }
    }

    return filtered;
}

// --- Private ---

void CollaborationClient::sendAuth()
{
    ks::Json payload = ks::Json::object();
    payload["userId"]   = m_userId;
    payload["userName"] = m_userName;
    payload["version"]  = 1;
    sendPacket("auth", payload);
    m_pingTimer.start(30000, [this]() {
        if (m_state == CollaborationState::Connected)
            sendPacket("ping", ks::Json::object());
    });
}

void CollaborationClient::sendPacket(const std::string& type, const ks::Json& payload)
{
    ks::Json pkt = ks::Json::object();
    pkt["type"]      = type;
    pkt["userId"]    = m_userId;
    pkt["timestamp"] = net::nowSecs();
    pkt["payload"]   = payload;
    if (m_socket) m_socket->sendText(pkt.dump());
}

void CollaborationClient::onMessage(const std::string& text)
{
    bool ok = false;
    ks::Json doc = ks::Json::parse(text, &ok);
    if (!ok || !doc.isObject()) {
        LOG_WARNING("Collaboration", "Invalid JSON message");
        return;
    }
    const ks::Json& pkt = doc;
    const std::string type = pkt["type"].toString();
    const ks::Json payload = pkt["payload"];

    if (type == "auth_ok") {
        LOG_DEBUG("Collaboration", "Auth OK");
    } else if (type == "user_joined") {
        CollaborationUser u;
        u.id       = payload["userId"].toString();
        u.name     = payload["userName"].toString();
        u.color    = payload["color"].toString("#4488ff");
        u.isOnline = true;
        m_activeUsers[u.id] = u;
        m_userDocuments[u.id] = payload["docId"].toString();
        userJoined(u);
    } else if (type == "user_left") {
        std::string uid = payload["userId"].toString();
        CollaborationUser u;
        auto it = m_activeUsers.find(uid);
        if (it != m_activeUsers.end()) {
            u = it->second;
            m_activeUsers.erase(it);
        }
        m_userDocuments.erase(uid);
        userLeft(u);
    } else if (type == "change") {
        Change ch;
        ch.id         = payload["changeId"].toString();
        ch.userId     = pkt["userId"].toString();
        ch.documentId = payload["docId"].toString();
        ch.data       = payload["change"];
        ch.version    = payload["version"].toInt();
        ch.timestamp  = static_cast<std::int64_t>(pkt["timestamp"].toDouble());
        changeReceived(ch);
    } else if (type == "cursor") {
        cursorReceived(payload["docId"].toString(),
                       pkt["userId"].toString(),
                       payload["cursor"]);
    } else if (type == "chat") {
        chatReceived(payload["docId"].toString(),
                     pkt["userId"].toString(),
                     payload["message"].toString());
    } else if (type == "pong") {
    } else if (type == "error") {
        error(payload["message"].toString());
    }
}

void CollaborationClient::scheduleReconnect()
{
    if (m_reconnectAttempts >= 10) {
        LOG_WARNING("Collaboration", "Max reconnect attempts reached");
        return;
    }
    int delay = std::min(1000 * (1 << m_reconnectAttempts), 30000);
    ++m_reconnectAttempts;
    LOG_INFO("Collaboration", "Reconnecting in " + std::to_string(delay) + "ms (attempt " +
                                  std::to_string(m_reconnectAttempts) + ")");
    if (m_reconnectTimer != 0) ks::net::killTimer(m_reconnectTimer);
    m_reconnectTimer = ks::net::startTimer(delay, [this]() {
        m_reconnectTimer = 0;
        if (m_state != CollaborationState::Connected) doConnect();
    });
}

// ---------------------------------------------------------------------------
// CollaborationServer
// ---------------------------------------------------------------------------

const std::vector<std::string> CollaborationServer::s_userColors = {
    "#e74c3c", "#3498db", "#2ecc71", "#f39c12", "#9b59b6",
    "#1abc9c", "#e67e22", "#34495e", "#e91e63", "#00bcd4",
    "#8bc34a", "#ff5722", "#607d8b", "#795548", "#cddc39"
};

CollaborationServer::CollaborationServer() = default;

CollaborationServer::~CollaborationServer()
{
    stop();
    m_server.reset();
}

bool CollaborationServer::start(std::uint16_t port)
{
    if (m_running) return true;

    m_port = port;
    m_server = ks::net::makeServerTransport();
    m_server->onClientConnected = [this](ks::net::ConnId conn) { onNewConnection(conn); };
    m_server->onClientDisconnected = [this](ks::net::ConnId conn) { onClientDisconnected(conn); };
    m_server->onTextReceived = [this](ks::net::ConnId conn, const std::string& text) {
        onTextReceived(conn, text);
    };

    std::string listenError;
    if (!m_server->listen(port, listenError)) {
        error(listenError);
        return false;
    }

    m_running = true;
    started();
    LOG_INFO("CollabServer", "Listening on port " + std::to_string(port));
    return true;
}

void CollaborationServer::stop()
{
    if (!m_running) return;

    // Close all client sockets
    if (m_server) {
        for (const auto& entry : m_socketToUser) {
            m_server->closeConnection(entry.first);
        }
        m_server->close();
    }

    m_users.clear();
    m_socketToUser.clear();
    m_userLastActivity.clear();
    m_documentChanges.clear();
    m_running = false;
    stopped();
    LOG_INFO("CollabServer", "Stopped");
}

void CollaborationServer::setMaxUsers(int max)
{
    m_maxUsers = max;
}

void CollaborationServer::setPassword(const std::string& password)
{
    m_password = password;
}

void CollaborationServer::clearPassword()
{
    m_password.clear();
}

void CollaborationServer::kickUser(const std::string& userId)
{
    auto userIt = m_users.find(userId);
    if (userIt == m_users.end()) return;

    // Find and close the connection
    for (const auto& entry : m_socketToUser) {
        if (entry.second == userId) {
            ks::Json msg = ks::Json::object();
            msg["type"] = "kicked";
            msg["payload"] = makeObject({{"reason", "Kicked by admin"}});
            if (m_server) {
                m_server->sendText(entry.first, msg.dump());
                m_server->closeConnection(entry.first);
            }
            break;
        }
    }

    m_users.erase(userIt);
    LOG_INFO("CollabServer", "Kicked user: " + userId);
}

void CollaborationServer::banUser(const std::string& userId)
{
    if (!vectorContains(m_bannedUsers, userId)) {
        m_bannedUsers.push_back(userId);
    }
    kickUser(userId);
    LOG_INFO("CollabServer", "Banned user: " + userId);
}

void CollaborationServer::unbanUser(const std::string& userId)
{
    vectorRemove(m_bannedUsers, userId);
    LOG_INFO("CollabServer", "Unbanned user: " + userId);
}

std::vector<CollaborationUser> CollaborationServer::getConnectedUsers() const
{
    std::vector<CollaborationUser> users;
    users.reserve(m_users.size());
    for (const auto& entry : m_users) users.push_back(entry.second);
    return users;
}

ks::Json CollaborationServer::getStatistics() const
{
    ks::Json stats = ks::Json::object();
    stats["userCount"] = static_cast<int>(m_users.size());
    stats["maxUsers"] = m_maxUsers;
    stats["bannedCount"] = static_cast<int>(m_bannedUsers.size());
    stats["running"] = m_running;
    stats["port"] = static_cast<int>(m_port);
    stats["hasPassword"] = !m_password.empty();

    int docCount = 0;
    for (const auto& entry : m_documentChanges)
        if (!entry.second.empty()) docCount++;
    stats["activeDocuments"] = docCount;

    return stats;
}

// --- Private ---

void CollaborationServer::onNewConnection(ks::net::ConnId conn)
{
    if (static_cast<int>(m_users.size()) >= m_maxUsers) {
        ks::Json msg = ks::Json::object();
        msg["type"] = "error";
        msg["payload"] = makeObject({{"message", "Server full"}});
        sendConn(conn, msg);
        if (m_server) m_server->closeConnection(conn);
    }
}

void CollaborationServer::onTextReceived(ks::net::ConnId conn, const std::string& text)
{
    bool ok = false;
    ks::Json doc = ks::Json::parse(text, &ok);
    if (!ok || !doc.isObject()) {
        LOG_WARNING("CollabServer", "Invalid JSON from client");
        return;
    }
    const ks::Json& pkt = doc;

    std::string userId;
    auto it = m_socketToUser.find(conn);
    if (it != m_socketToUser.end()) {
        userId = it->second;
        handleMessage(userId, pkt);
    } else {
        // First message must be auth
        const ks::Json payload = pkt["payload"];
        std::string uid = payload["userId"].toString();
        std::string userName = payload["userName"].toString();

        if (uid.empty() || userName.empty()) {
            if (m_server) m_server->closeConnection(conn);
            return;
        }

        if (vectorContains(m_bannedUsers, uid)) {
            ks::Json errMsg = ks::Json::object();
            errMsg["type"] = "error";
            errMsg["payload"] = makeObject({{"message", "You are banned from this server"}});
            sendConn(conn, errMsg);
            if (m_server) m_server->closeConnection(conn);
            return;
        }

        // Register user
        CollaborationUser user;
        user.id = uid;
        user.name = userName;
        user.color = s_userColors[static_cast<std::size_t>(m_nextColorIndex) % s_userColors.size()];
        m_nextColorIndex++;
        user.role = m_users.empty() ? UserRole::Owner : UserRole::Editor;
        user.isOnline = true;
        user.lastActivity = net::nowSecs();

        m_users[uid] = user;
        m_socketToUser[conn] = uid;
        m_userLastActivity[uid] = net::nowSecs();

        // Send auth OK
        ks::Json authOk = ks::Json::object();
        authOk["type"] = "auth_ok";
        authOk["payload"] = makeObject({
            {"userId", uid},
            {"color", user.color},
            {"role", static_cast<int>(user.role)}
        });
        sendConn(conn, authOk);

        // Notify others
        ks::Json joinMsg = ks::Json::object();
        joinMsg["type"] = "user_joined";
        joinMsg["payload"] = makeObject({
            {"userId", uid},
            {"userName", userName},
            {"color", user.color}
        });
        broadcast(joinMsg, uid);

        userConnected(user);
        LOG_INFO("CollabServer", "User connected: " + userName + " (" + uid + ")");
    }
}

void CollaborationServer::onClientDisconnected(ks::net::ConnId conn)
{
    std::string userId;
    auto it = m_socketToUser.find(conn);
    if (it == m_socketToUser.end()) return;
    userId = it->second;
    m_socketToUser.erase(it);

    CollaborationUser user;
    auto userIt = m_users.find(userId);
    if (userIt != m_users.end()) {
        user = userIt->second;
        m_users.erase(userIt);
    }
    m_userLastActivity.erase(userId);

    // Notify others
    ks::Json msg = ks::Json::object();
    msg["type"] = "user_left";
    msg["payload"] = makeObject({{"userId", userId}});
    broadcast(msg);

    userDisconnected(userId);
    LOG_INFO("CollabServer", "User disconnected: " + user.name);
}

void CollaborationServer::broadcast(const ks::Json& message, const std::string& excludeUser)
{
    const std::string data = message.dump();
    if (!m_server) return;
    for (const auto& entry : m_socketToUser) {
        if (entry.second != excludeUser) {
            m_server->sendText(entry.first, data);
        }
    }
}

void CollaborationServer::sendTo(const std::string& userId, const ks::Json& message)
{
    if (!m_server) return;
    const std::string data = message.dump();
    for (const auto& entry : m_socketToUser) {
        if (entry.second == userId) {
            m_server->sendText(entry.first, data);
            break;
        }
    }
}

void CollaborationServer::sendConn(ks::net::ConnId conn, const ks::Json& message)
{
    if (m_server) m_server->sendText(conn, message.dump());
}

void CollaborationServer::handleMessage(const std::string& userId, const ks::Json& pkt)
{
    const std::string type = pkt["type"].toString();
    const ks::Json payload = pkt["payload"];

    m_userLastActivity[userId] = net::nowSecs();

    if (type == "ping") {
        ks::Json pong = ks::Json::object();
        pong["type"] = "pong";
        sendTo(userId, pong);
    }
    else if (type == "join") {
        std::string docId = payload["docId"].toString();
        if (docId.empty()) return;

        // Send existing changes to the joining user
        auto docIt = m_documentChanges.find(docId);
        if (docIt != m_documentChanges.end()) {
            for (const Change& change : docIt->second) {
                ks::Json changeMsg = ks::Json::object();
                changeMsg["type"] = "change";
                changeMsg["userId"] = change.userId;
                changeMsg["payload"] = makeObject({
                    {"changeId", change.id},
                    {"docId", change.documentId},
                    {"change", change.data},
                    {"version", change.version}
                });
                sendTo(userId, changeMsg);
            }
        }

        // Notify others in the document
        ks::Json msg = ks::Json::object();
        msg["type"] = "user_joined";
        msg["payload"] = makeObject({
            {"userId", userId},
            {"userName", m_users[userId].name},
            {"color", m_users[userId].color},
            {"docId", docId}
        });
        broadcast(msg, userId);
    }
    else if (type == "leave") {
        std::string docId = payload["docId"].toString();
        ks::Json msg = ks::Json::object();
        msg["type"] = "user_left";
        msg["payload"] = makeObject({
            {"userId", userId},
            {"docId", docId}
        });
        broadcast(msg, userId);
    }
    else if (type == "change") {
        std::string docId = payload["docId"].toString();
        ks::Json changeData = payload["change"];
        int version = payload["version"].toInt();

        Change change;
        change.id = net::randomUuid();
        change.userId = userId;
        change.documentId = docId;
        change.data = changeData;
        change.version = version;
        change.timestamp = net::nowSecs();

        m_documentChanges[docId].push_back(change);

        // Broadcast change to all other users
        ks::Json msg = ks::Json::object();
        msg["type"] = "change";
        msg["userId"] = userId;
        msg["payload"] = makeObject({
            {"changeId", change.id},
            {"docId", docId},
            {"change", changeData},
            {"version", version}
        });
        broadcast(msg, userId);
    }
    else if (type == "cursor") {
        ks::Json msg = ks::Json::object();
        msg["type"] = "cursor";
        msg["userId"] = userId;
        msg["payload"] = payload;
        broadcast(msg, userId);
    }
    else if (type == "selection") {
        ks::Json msg = ks::Json::object();
        msg["type"] = "selection";
        msg["userId"] = userId;
        msg["payload"] = payload;
        broadcast(msg, userId);
    }
    else if (type == "chat") {
        std::string message = payload["message"].toString();
        ks::Json msg = ks::Json::object();
        msg["type"] = "chat";
        msg["userId"] = userId;
        msg["payload"] = makeObject({
            {"message", message}
        });
        broadcast(msg);
        chatReceived(userId, message);
    }
    else if (type == "presence") {
        ks::Json msg = ks::Json::object();
        msg["type"] = "presence";
        msg["userId"] = userId;
        msg["payload"] = payload;
        auto it = m_users.find(userId);
        if (it != m_users.end()) {
            std::string status = payload["status"].toString("online");
            it->second.isOnline = (status != "offline");
            presenceChanged(userId, status);
        }
    }
    else {
        LOG_WARNING("CollabServer", "Unknown message type: " + type);
    }
}

// ---------------------------------------------------------------------------
// PresenceManager
// ---------------------------------------------------------------------------

PresenceManager::PresenceManager()
{
    m_activityTimer.start(30000, [this]() { onUserActivityTimeout(); });
}

PresenceManager::~PresenceManager() {}

void PresenceManager::setCollaborationClient(CollaborationClient* client)
{
    m_client = client;
}

void PresenceManager::updatePresence(const std::string& status, const ks::Json& data)
{
    if (m_client) m_client->setPresence(status);
    if (!data.isEmpty()) m_userData["_self"] = data;
    presenceChanged("_self", status);
}

void PresenceManager::followUser(const std::string& userId)
{
    m_following.insert(userId);
}

void PresenceManager::unfollowUser(const std::string& userId)
{
    m_following.erase(userId);
}

std::string PresenceManager::getUserStatus(const std::string& userId) const
{
    auto it = m_userStatus.find(userId);
    return it != m_userStatus.end() ? it->second : std::string();
}

ks::Json PresenceManager::getUserData(const std::string& userId) const
{
    auto it = m_userData.find(userId);
    return it != m_userData.end() ? it->second : ks::Json();
}

std::vector<CollaborationUser> PresenceManager::getOnlineUsers() const
{
    return m_client ? m_client->getUsers(std::string()) : std::vector<CollaborationUser>();
}

void PresenceManager::onUserActivityTimeout()
{
    updatePresence("away");
}

// ---------------------------------------------------------------------------
// Annotations
// ---------------------------------------------------------------------------

Annotations::Annotations() {}

Annotations::~Annotations() {}

void Annotations::setDocument(const std::string& docId)
{
    m_docId = docId;
}

std::string Annotations::addAnnotation(const std::string& text, const ks::Json& position)
{
    Annotation a;
    a.id = net::randomUuid();
    a.text = text;
    a.position = position;
    a.created = net::nowSecs();
    a.modified = a.created;
    m_annotations[a.id] = a;
    annotationAdded(a);
    return a.id;
}

void Annotations::updateAnnotation(const std::string& annotationId, const std::string& text)
{
    auto it = m_annotations.find(annotationId);
    if (it != m_annotations.end()) {
        it->second.text = text;
        it->second.modified = net::nowSecs();
        annotationUpdated(it->second);
    }
}

void Annotations::resolveAnnotation(const std::string& annotationId)
{
    auto it = m_annotations.find(annotationId);
    if (it != m_annotations.end()) {
        it->second.isResolved = true;
        annotationResolved(annotationId);
    }
}

void Annotations::deleteAnnotation(const std::string& annotationId)
{
    if (m_annotations.erase(annotationId))
        annotationDeleted(annotationId);
}

std::vector<Annotation> Annotations::getAnnotations() const
{
    std::vector<Annotation> result;
    result.reserve(m_annotations.size());
    for (const auto& entry : m_annotations) result.push_back(entry.second);
    return result;
}

std::vector<Annotation> Annotations::getUnresolved() const
{
    std::vector<Annotation> result;
    for (const auto& entry : m_annotations)
        if (!entry.second.isResolved) result.push_back(entry.second);
    return result;
}

std::vector<Annotation> Annotations::getByAuthor(const std::string& authorId) const
{
    std::vector<Annotation> result;
    for (const auto& entry : m_annotations)
        if (entry.second.authorId == authorId) result.push_back(entry.second);
    return result;
}

int Annotations::getUnresolvedCount() const
{
    int count = 0;
    for (const auto& entry : m_annotations)
        if (!entry.second.isResolved) ++count;
    return count;
}

} // namespace ks
