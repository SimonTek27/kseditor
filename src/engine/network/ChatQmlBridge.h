#pragma once

#include <string>
#include <vector>

#include "../FileFormat/Json.h"
#include "../sys/Signal.h"
#include "ChatManager.h"
#include "ChatServer.h"

namespace ks::chat {

// Qt-free view-model for the chat QML UI. The Qt/QML binding layer
// (Q_PROPERTY, Q_INVOKABLE, qmlRegisterSingletonType) lives in the
// application layer: it forwards to the methods below and re-emits the
// ks::Signal notifications as Qt property-notify signals.
class ChatQmlBridge {
public:
    static ChatQmlBridge* instance();

    bool isConnected() const;
    std::string connectionState() const;
    std::string userName() const;
    std::string host() const;
    int port() const;
    std::vector<ks::Json> channels() const;
    std::vector<ks::Json> users() const;
    std::string activeChannelId() const;
    std::vector<ks::Json> messages() const;
    bool isServerRunning() const;
    int userCount() const;

    void setUserName(const std::string& name);
    void setHost(const std::string& h);
    void setPort(int p);
    void setActiveChannelId(const std::string& id);

    void connectToServer();
    void disconnectFromServer();
    void startServer();
    void stopServer();

    void createChannel(const std::string& name, int type = 1);
    void joinChannel(const std::string& channelId);
    void leaveChannel(const std::string& channelId);

    void sendMessage(const std::string& content, const std::string& replyTo = std::string());
    void editMessage(const std::string& messageId, const std::string& newContent);
    void deleteMessage(const std::string& messageId);
    void addReaction(const std::string& messageId, const std::string& emoji);
    void removeReaction(const std::string& messageId, const std::string& emoji);
    void sendTyping();

    ks::Json getUser(const std::string& userId) const;
    std::vector<ks::Json> getChannelMessages(const std::string& channelId) const;

    void refreshChannels();
    void refreshMessages();
    void refreshUsers();

    ks::Signal<> connectedChanged;
    ks::Signal<> connectionStateChanged;
    ks::Signal<> userNameChanged;
    ks::Signal<> hostChanged;
    ks::Signal<> portChanged;
    ks::Signal<> channelsChanged;
    ks::Signal<> usersChanged;
    ks::Signal<> activeChannelChanged;
    ks::Signal<> messagesChanged;
    ks::Signal<> serverStateChanged;
    ks::Signal<> userCountChanged;

    ks::Signal<const std::string&, const std::string&, const std::string&, const std::string&, const std::string&> messageReceived;
    ks::Signal<const std::string&, const std::string&> userJoinedChat;
    ks::Signal<const std::string&, const std::string&> userLeftChat;
    ks::Signal<const std::string&, const std::string&> channelCreatedSignal;
    ks::Signal<const std::string&> errorOccurred;
    ks::Signal<const std::string&, const std::string&> typingIndicator;

private:
    ChatQmlBridge();
    static ChatQmlBridge* s_instance;

    ChatManager* m_manager = nullptr;
    ChatServer* m_server = nullptr;
    std::string m_host = "localhost";
    int m_port = 9090;
};

} // namespace ks::chat
