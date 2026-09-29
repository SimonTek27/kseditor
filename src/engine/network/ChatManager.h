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

class ChatManager
{
public:
    static ChatManager* instance();

    ChatManager();
    ~ChatManager();

    void connectToServer(const std::string& host, std::uint16_t port, const std::string& userName);
    void disconnectFromServer();
    bool isConnected() const { return m_connected; }

    void createChannel(const std::string& name, ChannelType type = ChannelType::Group);
    void joinChannel(const std::string& channelId);
    void leaveChannel(const std::string& channelId);
    void setActiveChannel(const std::string& channelId);
    const std::string& activeChannel() const { return m_activeChannelId; }

    void sendMessage(const std::string& channelId, const std::string& content,
                     MessageType type = MessageType::Text, const std::string& replyTo = std::string());
    void editMessage(const std::string& messageId, const std::string& newContent);
    void deleteMessage(const std::string& messageId);
    void addReaction(const std::string& messageId, const std::string& emoji);
    void removeReaction(const std::string& messageId, const std::string& emoji);
    void sendTyping(const std::string& channelId);

    const std::vector<ChatChannel>& getChannels() const { return m_channels; }
    std::vector<ChatMessage> getMessages(const std::string& channelId) const;
    std::vector<ChatUser> getUsers() const;
    ChatUser getUser(const std::string& userId) const;
    const std::string& myUserId() const { return m_userId; }
    const std::string& myUserName() const { return m_userName; }

    void setUserName(const std::string& name);

    std::vector<ChatChannel> getDirectChannels() const;
    std::vector<ChatChannel> getGroupChannels() const;
    std::vector<ChatChannel> getProjectChannels() const;

    ks::Signal<> connected;
    ks::Signal<> disconnected;
    ks::Signal<const std::string&> connectionError;

    ks::Signal<const ChatChannel&> channelCreated;
    ks::Signal<const ChatChannel&> channelJoined;
    ks::Signal<const std::string&> channelLeft;
    ks::Signal<const std::string&> activeChannelChanged;

    ks::Signal<const ChatMessage&> messageReceived;
    ks::Signal<const ChatMessage&> messageEdited;
    ks::Signal<const std::string&> messageDeleted;

    ks::Signal<const ChatUser&> userJoined;
    ks::Signal<const std::string&> userLeft;
    ks::Signal<const std::string&, UserStatus> userStatusChanged;

    ks::Signal<const std::string&, const std::string&, const std::string&> reactionAdded;
    ks::Signal<const std::string&, const std::string&, const std::string&> reactionRemoved;

    ks::Signal<const std::string&, const std::string&> typingReceived;
    ks::Signal<const std::string&, int> unreadCountChanged;

private:
    void onTransportOpened();
    void onTransportClosed(const std::string& reason);
    void onTransportError(const std::string& message);
    void onTextReceived(const std::string& text);
    void scheduleReconnect();
    void cancelReconnect();

    void handleMessage(const ProtocolMessage& msg);
    void handleAuthResponse(const ks::Json& payload);
    void handleChannelCreated(const ks::Json& payload);
    void handleChannelJoined(const ks::Json& payload);
    void handleMessageReceived(const ks::Json& payload);
    void handleMessageEdited(const ks::Json& payload);
    void handleMessageDeleted(const ks::Json& payload);
    void handleUserPresence(const ks::Json& payload);
    void handleReaction(const ks::Json& payload);
    void handleTyping(const ks::Json& payload);

    void sendPacket(const ProtocolMessage& msg);
    void requestSync();
    void saveLocalData();
    void loadLocalData();

    std::string generateId() const;

    static ChatManager* s_instance;

    std::unique_ptr<ks::net::IClientTransport> m_client;
    bool m_connected = false;
    std::string m_userId;
    std::string m_userName;
    std::string m_activeChannelId;
    std::uint64_t m_reconnectTimer = 0;
    int m_reconnectAttempts = 0;
    bool m_autoReconnect = true;
    std::string m_lastHost;
    std::uint16_t m_lastPort = 0;

    std::vector<ChatChannel> m_channels;
    std::map<std::string, std::vector<ChatMessage>> m_messages;
    std::map<std::string, ChatUser> m_users;
};

} // namespace ks::chat
