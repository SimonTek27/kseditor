#include "OfficeQmlBridge.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

namespace ks::office {

OfficeQmlBridge* OfficeQmlBridge::s_instance = nullptr;

OfficeQmlBridge* OfficeQmlBridge::instance() {
    if (!s_instance) {
        s_instance = new OfficeQmlBridge();
    }
    return s_instance;
}

OfficeQmlBridge::OfficeQmlBridge(QObject* parent)
    : QObject(parent)
    , m_manager(OfficeManager::instance())
    , m_server(new OfficeServer(this))
{
    connect(m_manager, &OfficeManager::connected, this, [this]() {
        emit connectedChanged();
        emit connectionStateChanged();
        emit usersChanged();
    });
    connect(m_manager, &OfficeManager::disconnected, this, [this]() {
        emit connectedChanged();
        emit connectionStateChanged();
    });
    connect(m_manager, &OfficeManager::connectionError, this, [this](const QString& err) {
        emit errorOccurred(err);
    });

    connect(m_manager, &OfficeManager::channelCreated, this, [this](const OfficeChannel&) {
        emit channelsChanged();
    });
    connect(m_manager, &OfficeManager::activeChannelChanged, this, [this](const QString&) {
        emit activeChannelChanged();
        emit messagesChanged();
    });

    connect(m_manager, &OfficeManager::messageReceived, this, [this](const OfficeMessage& msg) {
        emit messagesChanged();
        emit messageReceived(msg.channelId, msg.authorId, msg.authorName,
                           msg.content, msg.timestamp.toString(Qt::ISODate));
    });

    connect(m_manager, &OfficeManager::userJoined, this, [this](const OfficeUser& user) {
        emit usersChanged();
        emit userCountChanged();
        emit userJoinedChat(user.id, user.name);
    });
    connect(m_manager, &OfficeManager::userLeft, this, [this](const QString& userId) {
        emit usersChanged();
        emit userCountChanged();
        emit userLeftChat(userId, userId);
    });

    connect(m_manager, &OfficeManager::messageEdited, this, [this](const OfficeMessage&) {
        emit messagesChanged();
    });
    connect(m_manager, &OfficeManager::messageDeleted, this, [this](const QString&) {
        emit messagesChanged();
    });

    connect(m_manager, &OfficeManager::typingReceived, this, [this](const QString& chId, const QString& userId) {
        emit typingIndicator(chId, userId);
    });

    connect(m_server, &OfficeServer::started, this, [this]() {
        emit serverStateChanged();
    });
    connect(m_server, &OfficeServer::stopped, this, [this]() {
        emit serverStateChanged();
    });
    connect(m_server, &OfficeServer::error, this, [this](const QString& err) {
        emit errorOccurred(err);
    });
}

bool OfficeQmlBridge::isConnected() const {
    return m_manager->isConnected();
}

QString OfficeQmlBridge::connectionState() const {
    return m_manager->isConnected() ? "Connected" : "Disconnected";
}

QString OfficeQmlBridge::userName() const {
    return m_manager->myUserName();
}

QString OfficeQmlBridge::host() const { return m_host; }
int OfficeQmlBridge::port() const { return m_port; }

QString OfficeQmlBridge::activeChannelId() const {
    return m_manager->activeChannel();
}

bool OfficeQmlBridge::isServerRunning() const {
    return m_server->isRunning();
}

int OfficeQmlBridge::userCount() const {
    return m_manager->getUsers().size();
}

QVariantList OfficeQmlBridge::channels() const {
    QVariantList result;
    for (const auto& ch : m_manager->getChannels()) {
        QVariantMap m;
        m["id"] = ch.id;
        m["name"] = ch.name;
        m["description"] = ch.description;
        m["type"] = static_cast<int>(ch.type);
        m["memberCount"] = ch.members.size();
        m["unreadCount"] = ch.unreadCount;
        m["isPinned"] = ch.isPinned;
        result.append(m);
    }
    return result;
}

QVariantList OfficeQmlBridge::users() const {
    QVariantList result;
    for (const auto& u : m_manager->getUsers()) {
        QVariantMap m;
        m["id"] = u.id;
        m["name"] = u.name;
        m["avatar"] = u.avatar;
        m["color"] = u.color.name();
        m["status"] = static_cast<int>(u.status);
        m["statusText"] = u.statusText;
        m["isBot"] = u.isBot;
        result.append(m);
    }
    return result;
}

QVariantList OfficeQmlBridge::messages() const {
    QVariantList result;
    QString activeId = m_manager->activeChannel();
    if (activeId.isEmpty()) return result;

    for (const auto& msg : m_manager->getMessages(activeId)) {
        QVariantMap m;
        m["id"] = msg.id;
        m["channelId"] = msg.channelId;
        m["authorId"] = msg.authorId;
        m["authorName"] = msg.authorName;
        m["type"] = static_cast<int>(msg.type);
        m["content"] = msg.content;
        m["timestamp"] = msg.timestamp.toString("hh:mm:ss");
        m["editedAt"] = msg.editedAt.toString("hh:mm:ss");
        m["replyToId"] = msg.replyToId;
        m["isDeleted"] = msg.isDeleted;

        QVariantMap reactions;
        for (auto it = msg.reactions.begin(); it != msg.reactions.end(); ++it) {
            reactions[it.key()] = it.value().size();
        }
        m["reactions"] = reactions;

        result.append(m);
    }
    return result;
}

void OfficeQmlBridge::setUserName(const QString& name) {
    if (m_manager->myUserName() != name) {
        m_manager->setUserName(name);
        emit userNameChanged();
    }
}

void OfficeQmlBridge::setHost(const QString& h) {
    if (m_host != h) { m_host = h; emit hostChanged(); }
}

void OfficeQmlBridge::setPort(int p) {
    if (m_port != p) { m_port = p; emit portChanged(); }
}

void OfficeQmlBridge::setActiveChannelId(const QString& id) {
    if (m_manager->activeChannel() != id) {
        m_manager->setActiveChannel(id);
    }
}

void OfficeQmlBridge::connectToServer() {
    m_manager->connectToServer(m_host, static_cast<quint16>(m_port), m_manager->myUserName());
}

void OfficeQmlBridge::disconnectFromServer() {
    m_manager->disconnectFromServer();
}

void OfficeQmlBridge::startServer() {
    if (m_server->start(static_cast<quint16>(m_port))) {
        m_manager->connectToServer("localhost", static_cast<quint16>(m_port), m_manager->myUserName());
    }
}

void OfficeQmlBridge::stopServer() {
    m_manager->disconnectFromServer();
    m_server->stop();
}

void OfficeQmlBridge::createChannel(const QString& name, int type) {
    m_manager->createChannel(name, static_cast<ChannelType>(type));
}

void OfficeQmlBridge::joinChannel(const QString& channelId) {
    m_manager->joinChannel(channelId);
}

void OfficeQmlBridge::leaveChannel(const QString& channelId) {
    m_manager->leaveChannel(channelId);
}

void OfficeQmlBridge::sendMessage(const QString& content, const QString& replyTo) {
    QString activeId = m_manager->activeChannel();
    if (!activeId.isEmpty() && !content.isEmpty()) {
        m_manager->sendMessage(activeId, content, MessageType::Text, replyTo);
    }
}

void OfficeQmlBridge::editMessage(const QString& messageId, const QString& newContent) {
    m_manager->editMessage(messageId, newContent);
}

void OfficeQmlBridge::deleteMessage(const QString& messageId) {
    m_manager->deleteMessage(messageId);
}

void OfficeQmlBridge::addReaction(const QString& messageId, const QString& emoji) {
    m_manager->addReaction(messageId, emoji);
}

void OfficeQmlBridge::removeReaction(const QString& messageId, const QString& emoji) {
    m_manager->removeReaction(messageId, emoji);
}

void OfficeQmlBridge::sendTyping() {
    QString activeId = m_manager->activeChannel();
    if (!activeId.isEmpty()) {
        m_manager->sendTyping(activeId);
    }
}

QVariantMap OfficeQmlBridge::getUser(const QString& userId) const {
    OfficeUser u = m_manager->getUser(userId);
    QVariantMap m;
    m["id"] = u.id;
    m["name"] = u.name;
    m["avatar"] = u.avatar;
    m["color"] = u.color.name();
    m["status"] = static_cast<int>(u.status);
    m["statusText"] = u.statusText;
    m["isBot"] = u.isBot;
    return m;
}

QVariantList OfficeQmlBridge::getChannelMessages(const QString& channelId) const {
    QVariantList result;
    for (const auto& msg : m_manager->getMessages(channelId)) {
        QVariantMap m;
        m["id"] = msg.id;
        m["authorId"] = msg.authorId;
        m["authorName"] = msg.authorName;
        m["content"] = msg.content;
        m["timestamp"] = msg.timestamp.toString("hh:mm:ss");
        m["isDeleted"] = msg.isDeleted;
        result.append(m);
    }
    return result;
}

void OfficeQmlBridge::refreshChannels() { emit channelsChanged(); }
void OfficeQmlBridge::refreshMessages() { emit messagesChanged(); }
void OfficeQmlBridge::refreshUsers() { emit usersChanged(); }

} // namespace ks::office
