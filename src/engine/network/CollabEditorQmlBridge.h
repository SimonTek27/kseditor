#pragma once

#include <string>
#include <vector>

#include "../FileFormat/Json.h"
#include "../sys/Signal.h"
#include "Collaboration.h"

namespace ks {

// Qt-free view-model for the collaboration editor QML UI; the Qt/QML binding
// layer (Q_PROPERTY / Q_INVOKABLE / qmlRegisterSingletonType) lives in the
// application layer and mirrors these members as Qt properties and signals.
class CollabEditorQmlBridge {
public:
    static CollabEditorQmlBridge* instance();

    bool isConnected() const;
    std::string connectionState() const;
    std::vector<ks::Json> users() const;
    std::string host() const;
    int port() const;
    std::string userName() const;

    void connectToServer();
    void disconnectFromServer();
    void sendChatMessage(const std::string& message);
    void sendDocumentChange(const std::string& docId, const ks::Json& change);
    void openDocument(const std::string& docId);
    void closeDocument(const std::string& docId);
    void followUser(const std::string& userId);
    void unfollowUser(const std::string& userId);

    std::vector<ks::Json> getHistory() const;
    std::vector<ks::Json> getConflicts() const;
    void resolveConflicts();
    ks::Json getPermissions() const;
    void setPermission(const std::string& userId, const std::string& permission, bool enabled);
    std::vector<ks::Json> getHistoryEvents() const;

    void setHost(const std::string& h);
    void setPort(int p);
    void setUserName(const std::string& name);

    ks::Signal<> connectedChanged;
    ks::Signal<> stateChanged;
    ks::Signal<> usersChanged;
    ks::Signal<> hostChanged;
    ks::Signal<> portChanged;
    ks::Signal<> userNameChanged;
    ks::Signal<const std::string&, const std::string&> chatMessageReceived;
    ks::Signal<const std::string&, const std::string&> userJoined;
    ks::Signal<const std::string&, const std::string&> userLeft;
    ks::Signal<const std::string&> errorOccurred;
    ks::Signal<const std::string&> statusMessage;
    ks::Signal<> followedUsersChanged;

private:
    CollabEditorQmlBridge();
    static CollabEditorQmlBridge* s_instance;

    void rebuildUserList();

    CollaborationClient* m_client = nullptr;
    std::string m_host;
    int m_port = 8080;
    std::string m_userName = "User";
    std::vector<std::string> m_followedUsers;
    std::string m_documentId;
    int m_lastSyncedVersion = 0;
};

}
