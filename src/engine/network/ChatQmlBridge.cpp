#include "ChatQmlBridge.h"

#include "NetUtil.h"

namespace ks::chat {

ChatQmlBridge* ChatQmlBridge::s_instance = nullptr;

ChatQmlBridge* ChatQmlBridge::instance() {
    if (!s_instance) {
        s_instance = new ChatQmlBridge();
    }
    return s_instance;
}

ChatQmlBridge::ChatQmlBridge()
    : m_manager(ChatManager::instance())
    , m_server(new ChatServer())
{
    m_manager->connected.connect([this]() {
        connectedChanged();
        connectionStateChanged();
        usersChanged();
    });
    m_manager->disconnected.connect([this]() {
        connectedChanged();
        connectionStateChanged();
    });
    m_manager->connectionError.connect([this](const std::string& err) {
        errorOccurred(err);
    });

    m_manager->channelCreated.connect([this](const ChatChannel&) {
        channelsChanged();
    });
    m_manager->activeChannelChanged.connect([this](const std::string&) {
        activeChannelChanged();
        messagesChanged();
    });

    m_manager->messageReceived.connect([this](const ChatMessage& msg) {
        messagesChanged();
        messageReceived(msg.channelId, msg.authorId, msg.authorName,
                        msg.content, net::formatIsoDate(msg.timestamp));
    });

    m_manager->userJoined.connect([this](const ChatUser& user) {
        usersChanged();
        userCountChanged();
        userJoinedChat(user.id, user.name);
    });
    m_manager->userLeft.connect([this](const std::string& userId) {
        usersChanged();
        userCountChanged();
        userLeftChat(userId, userId);
    });

    m_manager->messageEdited.connect([this](const ChatMessage&) {
        messagesChanged();
    });
    m_manager->messageDeleted.connect([this](const std::string&) {
        messagesChanged();
    });

    m_manager->typingReceived.connect([this](const std::string& chId, const std::string& userId) {
        typingIndicator(chId, userId);
    });

    m_server->started.connect([this]() {
        serverStateChanged();
    });
    m_server->stopped.connect([this]() {
        serverStateChanged();
    });
    m_server->error.connect([this](const std::string& err) {
        errorOccurred(err);
    });
}

bool ChatQmlBridge::isConnected() const {
    return m_manager->isConnected();
}

std::string ChatQmlBridge::connectionState() const {
    return m_manager->isConnected() ? "Connected" : "Disconnected";
}

std::string ChatQmlBridge::userName() const {
    return m_manager->myUserName();
}

std::string ChatQmlBridge::host() const { return m_host; }
int ChatQmlBridge::port() const { return m_port; }

std::string ChatQmlBridge::activeChannelId() const {
    return m_manager->activeChannel();
}

bool ChatQmlBridge::isServerRunning() const {
    return m_server->isRunning();
}

int ChatQmlBridge::userCount() const {
    return static_cast<int>(m_manager->getUsers().size());
}

std::vector<ks::Json> ChatQmlBridge::channels() const {
    std::vector<ks::Json> result;
    for (const auto& ch : m_manager->getChannels()) {
        ks::Json m = ks::Json::object();
        m["id"] = ch.id;
        m["name"] = ch.name;
        m["description"] = ch.description;
        m["type"] = static_cast<int>(ch.type);
        m["memberCount"] = static_cast<int>(ch.members.size());
        m["unreadCount"] = ch.unreadCount;
        m["isPinned"] = ch.isPinned;
        result.push_back(m);
    }
    return result;
}

std::vector<ks::Json> ChatQmlBridge::users() const {
    std::vector<ks::Json> result;
    for (const auto& u : m_manager->getUsers()) {
        ks::Json m = ks::Json::object();
        m["id"] = u.id;
        m["name"] = u.name;
        m["avatar"] = u.avatar;
        m["color"] = u.color.name();
        m["status"] = static_cast<int>(u.status);
        m["statusText"] = u.statusText;
        m["isBot"] = u.isBot;
        result.push_back(m);
    }
    return result;
}

std::vector<ks::Json> ChatQmlBridge::messages() const {
    std::vector<ks::Json> result;
    std::string activeId = m_manager->activeChannel();
    if (activeId.empty()) return result;

    for (const auto& msg : m_manager->getMessages(activeId)) {
        ks::Json m = ks::Json::object();
        m["id"] = msg.id;
        m["channelId"] = msg.channelId;
        m["authorId"] = msg.authorId;
        m["authorName"] = msg.authorName;
        m["type"] = static_cast<int>(msg.type);
        m["content"] = msg.content;
        m["timestamp"] = net::formatHms(msg.timestamp / 1000);
        m["editedAt"] = net::formatHms(msg.editedAt / 1000);
        m["replyToId"] = msg.replyToId;
        m["isDeleted"] = msg.isDeleted;

        ks::Json reactions = ks::Json::object();
        for (const auto& entry : msg.reactions) {
            reactions[entry.first] = static_cast<int>(entry.second.size());
        }
        m["reactions"] = reactions;

        result.push_back(m);
    }
    return result;
}

void ChatQmlBridge::setUserName(const std::string& name) {
    if (m_manager->myUserName() != name) {
        m_manager->setUserName(name);
        userNameChanged();
    }
}

void ChatQmlBridge::setHost(const std::string& h) {
    if (m_host != h) { m_host = h; hostChanged(); }
}

void ChatQmlBridge::setPort(int p) {
    if (m_port != p) { m_port = p; portChanged(); }
}

void ChatQmlBridge::setActiveChannelId(const std::string& id) {
    if (m_manager->activeChannel() != id) {
        m_manager->setActiveChannel(id);
    }
}

void ChatQmlBridge::connectToServer() {
    m_manager->connectToServer(m_host, static_cast<std::uint16_t>(m_port), m_manager->myUserName());
}

void ChatQmlBridge::disconnectFromServer() {
    m_manager->disconnectFromServer();
}

void ChatQmlBridge::startServer() {
    if (m_server->start(static_cast<std::uint16_t>(m_port))) {
        // Auto-connect to local server
        m_manager->connectToServer("localhost", static_cast<std::uint16_t>(m_port), m_manager->myUserName());
    }
}

void ChatQmlBridge::stopServer() {
    m_manager->disconnectFromServer();
    m_server->stop();
}

void ChatQmlBridge::createChannel(const std::string& name, int type) {
    m_manager->createChannel(name, static_cast<ChannelType>(type));
}

void ChatQmlBridge::joinChannel(const std::string& channelId) {
    m_manager->joinChannel(channelId);
}

void ChatQmlBridge::leaveChannel(const std::string& channelId) {
    m_manager->leaveChannel(channelId);
}

void ChatQmlBridge::sendMessage(const std::string& content, const std::string& replyTo) {
    std::string activeId = m_manager->activeChannel();
    if (!activeId.empty() && !content.empty()) {
        m_manager->sendMessage(activeId, content, MessageType::Text, replyTo);
    }
}

void ChatQmlBridge::editMessage(const std::string& messageId, const std::string& newContent) {
    m_manager->editMessage(messageId, newContent);
}

void ChatQmlBridge::deleteMessage(const std::string& messageId) {
    m_manager->deleteMessage(messageId);
}

void ChatQmlBridge::addReaction(const std::string& messageId, const std::string& emoji) {
    m_manager->addReaction(messageId, emoji);
}

void ChatQmlBridge::removeReaction(const std::string& messageId, const std::string& emoji) {
    m_manager->removeReaction(messageId, emoji);
}

void ChatQmlBridge::sendTyping() {
    std::string activeId = m_manager->activeChannel();
    if (!activeId.empty()) {
        m_manager->sendTyping(activeId);
    }
}

ks::Json ChatQmlBridge::getUser(const std::string& userId) const {
    ChatUser u = m_manager->getUser(userId);
    ks::Json m = ks::Json::object();
    m["id"] = u.id;
    m["name"] = u.name;
    m["avatar"] = u.avatar;
    m["color"] = u.color.name();
    m["status"] = static_cast<int>(u.status);
    m["statusText"] = u.statusText;
    m["isBot"] = u.isBot;
    return m;
}

std::vector<ks::Json> ChatQmlBridge::getChannelMessages(const std::string& channelId) const {
    std::vector<ks::Json> result;
    for (const auto& msg : m_manager->getMessages(channelId)) {
        ks::Json m = ks::Json::object();
        m["id"] = msg.id;
        m["authorId"] = msg.authorId;
        m["authorName"] = msg.authorName;
        m["content"] = msg.content;
        m["timestamp"] = net::formatHms(msg.timestamp / 1000);
        m["isDeleted"] = msg.isDeleted;
        result.push_back(m);
    }
    return result;
}

void ChatQmlBridge::refreshChannels() { channelsChanged(); }
void ChatQmlBridge::refreshMessages() { messagesChanged(); }
void ChatQmlBridge::refreshUsers() { usersChanged(); }

} // namespace ks::chat
