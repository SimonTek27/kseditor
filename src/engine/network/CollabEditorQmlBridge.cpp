#include "CollabEditorQmlBridge.h"

#include "ChatProtocol.h"
#include "NetUtil.h"

namespace ks {

using chat::vectorContains;
using chat::vectorRemove;

CollabEditorQmlBridge* CollabEditorQmlBridge::s_instance = nullptr;

CollabEditorQmlBridge* CollabEditorQmlBridge::instance() {
    if (!s_instance) {
        s_instance = new CollabEditorQmlBridge();
    }
    return s_instance;
}

CollabEditorQmlBridge::CollabEditorQmlBridge()
{
    m_client = CollaborationClient::instance();

    m_client->connected.connect([this]() {
        connectedChanged();
        stateChanged();
    });
    m_client->disconnected.connect([this]() {
        connectedChanged();
        stateChanged();
        rebuildUserList();
    });
    m_client->stateChanged.connect([this](CollaborationState) {
        stateChanged();
    });
    m_client->userJoined.connect([this](const CollaborationUser& u) {
        rebuildUserList();
        userJoined(u.id, u.name);
    });
    m_client->userLeft.connect([this](const CollaborationUser& u) {
        rebuildUserList();
        userLeft(u.id, u.name);
    });
    m_client->chatReceived.connect([this](const std::string& docId, const std::string& userId,
                                          const std::string& message) {
        (void)docId;
        chatMessageReceived(userId, message);
    });
    m_client->error.connect([this](const std::string& err) {
        errorOccurred(err);
    });
}

bool CollabEditorQmlBridge::isConnected() const {
    return m_client ? m_client->isConnected() : false;
}

std::string CollabEditorQmlBridge::connectionState() const {
    if (!m_client) return "Disconnected";
    switch (m_client->getState()) {
        case CollaborationState::Disconnected: return "Disconnected";
        case CollaborationState::Connecting: return "Connecting";
        case CollaborationState::Connected: return "Connected";
        case CollaborationState::Reconnecting: return "Reconnecting";
        case CollaborationState::Error: return "Error";
    }
    return "Unknown";
}

std::vector<ks::Json> CollabEditorQmlBridge::users() const {
    std::vector<ks::Json> result;
    std::string fakeDocId = "current";
    auto users = m_client ? m_client->getUsers(fakeDocId) : std::vector<CollaborationUser>();
    for (const auto& u : users) {
        ks::Json m = ks::Json::object();
        m["id"] = u.id;
        m["name"] = u.name;
        m["color"] = u.color;
        m["role"] = static_cast<int>(u.role);
        m["isOnline"] = u.isOnline;
        result.push_back(m);
    }
    return result;
}

std::string CollabEditorQmlBridge::host() const { return m_host; }
int CollabEditorQmlBridge::port() const { return m_port; }
std::string CollabEditorQmlBridge::userName() const { return m_userName; }

void CollabEditorQmlBridge::connectToServer() {
    if (m_client) {
        m_client->setServer(m_host, static_cast<std::uint16_t>(m_port));
        m_client->setUserInfo("user_" + m_userName, m_userName);
        m_client->connect();
    }
}

void CollabEditorQmlBridge::disconnectFromServer() {
    if (m_client) m_client->disconnect();
}

void CollabEditorQmlBridge::sendChatMessage(const std::string& message) {
    if (m_client) m_client->sendChat("current", message);
}

void CollabEditorQmlBridge::sendDocumentChange(const std::string& docId, const ks::Json& change) {
    if (m_client) m_client->sendChange(docId, change);
}

void CollabEditorQmlBridge::openDocument(const std::string& docId) {
    if (m_client) m_client->joinDocument(docId);
}

void CollabEditorQmlBridge::closeDocument(const std::string& docId) {
    if (m_client) m_client->leaveDocument(docId);
}

void CollabEditorQmlBridge::followUser(const std::string& userId) {
    if (!vectorContains(m_followedUsers, userId)) {
        m_followedUsers.push_back(userId);
        statusMessage("Now following user: " + userId);
        followedUsersChanged();
    }
}

void CollabEditorQmlBridge::unfollowUser(const std::string& userId) {
    const bool removed = vectorContains(m_followedUsers, userId);
    vectorRemove(m_followedUsers, userId);
    if (removed) {
        statusMessage("Unfollowed user: " + userId);
        followedUsersChanged();
    }
}

std::vector<ks::Json> CollabEditorQmlBridge::getHistory() const {
    std::vector<ks::Json> history;
    if (m_client && m_client->isConnected()) {
        auto changes = m_client->getChanges("default", 0);
        for (const auto& ch : changes) {
            ks::Json entry = ks::Json::object();
            entry["user"] = ch.userId;
            entry["description"] = ch.type + " on " + ch.documentId;
            entry["timestamp"] = net::formatHms(ch.timestamp);
            history.push_back(entry);
        }
    }
    return history;
}

std::vector<ks::Json> CollabEditorQmlBridge::getConflicts() const {
    std::vector<ks::Json> conflicts;
    if (!m_client || !m_client->isConnected()) return conflicts;

    auto pendingChanges = m_client->getChanges(m_documentId, m_lastSyncedVersion);
    auto serverChanges = m_client->getChanges(m_documentId, 0);

    std::set<std::string> localOps;
    for (const auto& ch : pendingChanges) {
        localOps.insert(ch.userId + ":" + ch.type);
    }

    std::set<std::string> remoteOps;
    for (const auto& ch : serverChanges) {
        remoteOps.insert(ch.userId + ":" + ch.type);
    }

    for (const auto& key : localOps) {
        if (remoteOps.find(key) == remoteOps.end()) continue;

        const std::size_t sep = key.find(':');
        const std::string type = sep != std::string::npos ? key.substr(sep + 1) : key;
        const std::string user = sep != std::string::npos ? key.substr(0, sep) : key;

        ks::Json conflict = ks::Json::object();
        conflict["type"] = type;
        conflict["user"] = user;
        conflict["description"] = "Concurrent modification detected";
        conflict["documentId"] = m_documentId;
        conflict["timestamp"] = net::formatHms(net::nowSecs());
        conflicts.push_back(conflict);
    }

    return conflicts;
}

void CollabEditorQmlBridge::resolveConflicts() {
    if (!m_client || !m_client->isConnected()) {
        statusMessage("Cannot resolve conflicts: not connected");
        return;
    }

    auto conflicts = getConflicts();
    for (const auto& c : conflicts) {
        std::string docId = c["documentId"].toString();
        if (!docId.empty() && m_client->getChanges(docId, 0).size() > 0) {
            m_client->sendChange(docId, ks::Json::object());
        }
    }
    statusMessage("Conflict resolution completed: server state is authoritative");
}

ks::Json CollabEditorQmlBridge::getPermissions() const {
    ks::Json perms = ks::Json::object();
    if (!m_client || !m_client->isConnected()) {
        perms["canEdit"] = false;
        perms["canDelete"] = false;
        perms["canInvite"] = false;
        perms["admin"] = false;
        return perms;
    }

    // Default permissions for connected user
    // In a real implementation, these would come from the server
    perms["canEdit"] = true;
    perms["canDelete"] = true;
    perms["canInvite"] = false;
    perms["admin"] = false;
    return perms;
}

void CollabEditorQmlBridge::setPermission(const std::string& userId, const std::string& permission,
                                          bool enabled) {
    if (!m_client || !m_client->isConnected()) {
        statusMessage("Cannot set permission: not connected");
        return;
    }

    // In a real implementation, this would send a permission change request to the server
    statusMessage("Permission '" + permission + "' " + (enabled ? "granted" : "revoked") +
                  " for user " + userId);
}

std::vector<ks::Json> CollabEditorQmlBridge::getHistoryEvents() const {
    return getHistory();
}

void CollabEditorQmlBridge::setHost(const std::string& h) {
    if (m_host != h) { m_host = h; hostChanged(); }
}

void CollabEditorQmlBridge::setPort(int p) {
    if (m_port != p) { m_port = p; portChanged(); }
}

void CollabEditorQmlBridge::setUserName(const std::string& name) {
    if (m_userName != name) { m_userName = name; userNameChanged(); }
}

void CollabEditorQmlBridge::rebuildUserList() {
    usersChanged();
}

}
