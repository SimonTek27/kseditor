#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "../FileFormat/Json.h"
#include "../sys/Signal.h"
#include "ChatTransport.h"

namespace ks {

enum class CollaborationState {
    Disconnected,
    Connecting,
    Connected,
    Reconnecting,
    Error
};

enum class UserRole {
    Viewer,
    Editor,
    Admin,
    Owner
};

struct CollaborationUser {
    std::string id;
    std::string name;
    std::string color;
    UserRole role = UserRole::Viewer;
    std::string address;
    bool isOnline = false;
    std::int64_t lastActivity = 0;
};

struct Change {
    std::string id;
    std::string userId;
    std::string documentId;
    std::string type;
    ks::Json data;
    std::int64_t timestamp = 0;
    int version = 0;

    bool operator<(const Change& other) const { return timestamp < other.timestamp; }
};

struct CollaborationDocument {
    std::vector<Change> changes;
};

class CollaborationClient
{
public:
    static CollaborationClient* instance();
    CollaborationClient();
    ~CollaborationClient();

    void setServer(const std::string& host, std::uint16_t port);
    void setUserInfo(const std::string& userId, const std::string& userName);

    bool connect();
    void disconnect();
    bool isConnected() const { return m_state == CollaborationState::Connected; }
    CollaborationState getState() const { return m_state; }

    void joinDocument(const std::string& docId);
    void leaveDocument(const std::string& docId);

    void sendChange(const std::string& docId, const ks::Json& change);
    void sendCursor(const std::string& docId, const ks::Json& cursor);
    void sendSelection(const std::string& docId, const ks::Json& selection);
    void sendChat(const std::string& docId, const std::string& message);

    std::vector<CollaborationUser> getUsers(const std::string& docId) const;
    std::vector<Change> getChanges(const std::string& docId, int fromVersion) const;

    void setAutoReconnect(bool enabled);
    bool isAutoReconnect() const { return m_autoReconnect; }

    void setPresence(const std::string& status);
    const std::string& getPresence() const { return m_presence; }

    ks::Signal<CollaborationState> stateChanged;
    ks::Signal<> connected;
    ks::Signal<> disconnected;
    ks::Signal<const std::string&> error;

    ks::Signal<const CollaborationUser&> userJoined;
    ks::Signal<const CollaborationUser&> userLeft;

    ks::Signal<const Change&> changeReceived;
    ks::Signal<const std::string&, const std::string&, const ks::Json&> cursorReceived;
    ks::Signal<const std::string&, const std::string&, const ks::Json&> selectionReceived;
    ks::Signal<const std::string&, const std::string&, const std::string&> chatReceived;

    ks::Signal<const std::string&, int> versionUpdated;

private:
    void onTransportOpened();
    void onTransportClosed();
    void onMessage(const std::string& text);
    void doConnect();
    void scheduleReconnect();

    void sendAuth();
    void sendPacket(const std::string& type, const ks::Json& payload = ks::Json::object());

    static CollaborationClient* s_instance;

    std::unique_ptr<ks::net::IClientTransport> m_socket;
    std::string m_host;
    std::uint16_t m_port = 0;
    std::string m_userId;
    std::string m_userName;
    std::string m_presence = "available";

    CollaborationState m_state = CollaborationState::Disconnected;
    bool m_autoReconnect = true;
    int m_reconnectAttempts = 0;
    std::uint64_t m_reconnectTimer = 0;
    ks::net::RepeatingTimer m_pingTimer;
    int m_localVersion = 0;
    std::set<std::string> m_openDocuments;
    std::map<std::string, CollaborationUser> m_activeUsers;
    std::map<std::string, std::string> m_userDocuments;
    std::map<std::string, CollaborationDocument> m_documents;
};

class CollaborationServer
{
public:
    CollaborationServer();
    ~CollaborationServer();

    bool start(std::uint16_t port);
    void stop();

    bool isRunning() const { return m_running; }
    std::uint16_t getPort() const { return m_port; }

    void setMaxUsers(int max);
    int getMaxUsers() const { return m_maxUsers; }

    void setPassword(const std::string& password);
    void clearPassword();

    void kickUser(const std::string& userId);
    void banUser(const std::string& userId);
    void unbanUser(const std::string& userId);

    std::vector<CollaborationUser> getConnectedUsers() const;
    int getUserCount() const { return static_cast<int>(m_users.size()); }

    const std::vector<std::string>& getBannedUsers() const { return m_bannedUsers; }

    ks::Json getStatistics() const;

    ks::Signal<> started;
    ks::Signal<> stopped;
    ks::Signal<const std::string&> error;

    ks::Signal<const CollaborationUser&> userConnected;
    ks::Signal<const std::string&> userDisconnected;

    ks::Signal<const std::string&, const ks::Json&> messageReceived;
    ks::Signal<const std::string&, const std::string&> chatReceived;
    ks::Signal<const std::string&, const std::string&> presenceChanged;

private:
    void onNewConnection(ks::net::ConnId conn);
    void onClientDisconnected(ks::net::ConnId conn);
    void onTextReceived(ks::net::ConnId conn, const std::string& text);

    void broadcast(const ks::Json& message, const std::string& excludeUser = std::string());
    void sendTo(const std::string& userId, const ks::Json& message);
    void sendConn(ks::net::ConnId conn, const ks::Json& message);
    void handleMessage(const std::string& userId, const ks::Json& pkt);

    bool m_running = false;
    std::uint16_t m_port = 0;
    int m_maxUsers = 10;
    std::string m_password;

    std::unique_ptr<ks::net::IServerTransport> m_server;
    std::map<std::string, CollaborationUser> m_users;
    std::map<ks::net::ConnId, std::string> m_socketToUser;
    std::vector<std::string> m_bannedUsers;
    std::map<std::string, std::int64_t> m_userLastActivity;
    std::map<std::string, std::vector<Change>> m_documentChanges;
    int m_nextColorIndex = 0;

    static const std::vector<std::string> s_userColors;
};

class PresenceManager
{
public:
    PresenceManager();
    ~PresenceManager();

    void setCollaborationClient(CollaborationClient* client);

    void updatePresence(const std::string& status, const ks::Json& data = ks::Json::object());

    void followUser(const std::string& userId);
    void unfollowUser(const std::string& userId);

    std::string getUserStatus(const std::string& userId) const;
    ks::Json getUserData(const std::string& userId) const;

    std::vector<CollaborationUser> getOnlineUsers() const;

    ks::Signal<const std::string&, const std::string&> presenceChanged;
    ks::Signal<const std::string&, const ks::Json&> userDataChanged;
    ks::Signal<const std::string&> userWentOnline;
    ks::Signal<const std::string&> userWentOffline;

private:
    void onUserActivityTimeout();

    CollaborationClient* m_client = nullptr;
    std::map<std::string, std::string> m_userStatus;
    std::map<std::string, ks::Json> m_userData;
    std::set<std::string> m_following;
    ks::net::RepeatingTimer m_activityTimer;
};

struct Cursor {
    double x = 0;
    double y = 0;
    int line = 0;
    int column = 0;
    std::string selection;
};

struct Annotation {
    std::string id;
    std::string authorId;
    std::string authorName;
    std::string text;
    std::string color;
    ks::Json position;
    std::int64_t created = 0;
    std::int64_t modified = 0;
    bool isResolved = false;
};

class Annotations
{
public:
    Annotations();
    ~Annotations();

    void setDocument(const std::string& docId);
    const std::string& getDocument() const { return m_docId; }

    std::string addAnnotation(const std::string& text, const ks::Json& position);
    void updateAnnotation(const std::string& annotationId, const std::string& text);
    void resolveAnnotation(const std::string& annotationId);
    void deleteAnnotation(const std::string& annotationId);

    std::vector<Annotation> getAnnotations() const;
    std::vector<Annotation> getUnresolved() const;
    std::vector<Annotation> getByAuthor(const std::string& authorId) const;

    int getCount() const { return static_cast<int>(m_annotations.size()); }
    int getUnresolvedCount() const;

    ks::Signal<const Annotation&> annotationAdded;
    ks::Signal<const Annotation&> annotationUpdated;
    ks::Signal<const std::string&> annotationResolved;
    ks::Signal<const std::string&> annotationDeleted;

private:
    std::string m_docId;
    std::map<std::string, Annotation> m_annotations;
    std::string m_nextAnnotationId;
};

} // namespace ks
