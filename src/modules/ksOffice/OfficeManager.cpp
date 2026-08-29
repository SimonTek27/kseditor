#include "OfficeManager.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QUuid>
#include <QDebug>
#include <QDateTime>
#include <QRandomGenerator>
#include <QColor>

namespace ks::office {

OfficeManager* OfficeManager::s_instance = nullptr;

OfficeManager* OfficeManager::instance() {
    if (!s_instance) {
        s_instance = new OfficeManager();
    }
    return s_instance;
}

OfficeManager::OfficeManager(QObject* parent)
    : QObject(parent)
    , m_socket(new QWebSocket(QStringLiteral("ksOffice"), QWebSocketProtocol::Version13, this))
    , m_reconnectTimer(new QTimer(this))
{
    m_userId = generateId();

    connect(m_socket, &QWebSocket::connected, this, &OfficeManager::onConnected);
    connect(m_socket, &QWebSocket::disconnected, this, &OfficeManager::onDisconnected);
    connect(m_socket, &QWebSocket::textMessageReceived, this, &OfficeManager::onTextMessageReceived);
    connect(m_socket, &QWebSocket::sslErrors, this, &OfficeManager::onSslErrors);

    connect(m_reconnectTimer, &QTimer::timeout, this, [this]() {
        if (!m_connected && m_autoReconnect && m_reconnectAttempts < 5) {
            m_reconnectAttempts++;
            qDebug() << "ksOffice: Reconnect attempt" << m_reconnectAttempts;
            m_socket->open(QUrl(QStringLiteral("ws://%1:%2").arg(m_lastHost).arg(m_lastPort)));
        }
    });

    loadLocalData();
}

OfficeManager::~OfficeManager() {
    saveLocalData();
    if (m_socket) {
        m_socket->close();
    }
}

void OfficeManager::connectToServer(const QString& host, quint16 port, const QString& userName) {
    m_userName = userName;
    m_lastHost = host;
    m_lastPort = port;
    m_reconnectAttempts = 0;

    QUrl url(QStringLiteral("ws://%1:%2").arg(host).arg(port));
    qDebug() << "ksOffice: Connecting to" << url.toString();
    m_socket->open(url);
}

void OfficeManager::disconnectFromServer() {
    m_autoReconnect = false;
    m_reconnectTimer->stop();
    m_socket->close();
}

void OfficeManager::onConnected() {
    m_connected = true;
    m_reconnectAttempts = 0;
    m_reconnectTimer->stop();

    ProtocolMessage auth = ProtocolMessage::make("auth", {
        {"userId", m_userId},
        {"userName", m_userName}
    });
    sendPacket(auth);

    qDebug() << "ksOffice: Connected, sent auth";
    emit connected();
}

void OfficeManager::onDisconnected() {
    bool wasConnected = m_connected;
    m_connected = false;

    if (wasConnected) {
        qDebug() << "ksOffice: Disconnected";
        emit disconnected();
    }

    if (m_autoReconnect && m_reconnectAttempts < 5) {
        m_reconnectTimer->start(3000 * (m_reconnectAttempts + 1));
    }
}

void OfficeManager::onTextMessageReceived(const QString& message) {
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull() || !doc.isObject()) return;

    ProtocolMessage msg = ProtocolMessage::fromJson(doc.object());
    handleMessage(msg);
}

void OfficeManager::onSslErrors(const QList<QSslError>& errors) {
    for (const auto& e : errors) {
        qDebug() << "ksOffice: SSL Error:" << e.errorString();
    }
    m_socket->ignoreSslErrors();
}

void OfficeManager::handleMessage(const ProtocolMessage& msg) {
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
        QJsonArray channelsArr = msg.payload["channels"].toArray();
        for (const auto& v : channelsArr) {
            OfficeChannel ch = OfficeChannel::fromJson(v.toObject());
            bool found = false;
            for (auto& existing : m_channels) {
                if (existing.id == ch.id) { existing = ch; found = true; break; }
            }
            if (!found) m_channels.append(ch);
        }
    }
}

void OfficeManager::handleAuthResponse(const QJsonObject& payload) {
    QJsonArray usersArr = payload["users"].toArray();
    for (const auto& v : usersArr) {
        OfficeUser u = OfficeUser::fromJson(v.toObject());
        m_users[u.id] = u;
        emit userJoined(u);
    }

    requestSync();
    qDebug() << "ksOffice: Auth OK, synced" << m_channels.size() << "channels";
}

void OfficeManager::handleChannelCreated(const QJsonObject& payload) {
    OfficeChannel ch = OfficeChannel::fromJson(payload);
    m_channels.append(ch);
    emit channelCreated(ch);
}

void OfficeManager::handleChannelJoined(const QJsonObject& payload) {
    QString channelId = payload["channelId"].toString();
    QString userId = payload["userId"].toString();
    QString userName = payload["userName"].toString();

    for (auto& ch : m_channels) {
        if (ch.id == channelId) {
            if (!ch.members.contains(userId)) {
                ch.members.append(userId);
            }
            break;
        }
    }

    if (!m_users.contains(userId)) {
        OfficeUser u;
        u.id = userId;
        u.name = userName;
        u.color = QColor::fromHsv(QRandomGenerator::global()->bounded(360), 180, 200);
        u.status = UserStatus::Online;
        m_users[userId] = u;
        emit userJoined(u);
    }
}

void OfficeManager::handleMessageReceived(const QJsonObject& payload) {
    OfficeMessage msg = OfficeMessage::fromJson(payload);
    m_messages[msg.channelId].append(msg);
    emit messageReceived(msg);

    for (auto& ch : m_channels) {
        if (ch.id == msg.channelId && msg.channelId != m_activeChannelId) {
            ch.unreadCount++;
            emit unreadCountChanged(ch.id, ch.unreadCount);
            break;
        }
    }
}

void OfficeManager::handleMessageEdited(const QJsonObject& payload) {
    OfficeMessage msg = OfficeMessage::fromJson(payload);
    auto& msgs = m_messages[msg.channelId];
    for (auto& m : msgs) {
        if (m.id == msg.id) {
            m = msg;
            emit messageEdited(msg);
            return;
        }
    }
}

void OfficeManager::handleMessageDeleted(const QJsonObject& payload) {
    QString msgId = payload["messageId"].toString();
    QString channelId = payload["channelId"].toString();
    auto& msgs = m_messages[channelId];
    for (auto& m : msgs) {
        if (m.id == msgId) {
            m.isDeleted = true;
            emit messageDeleted(msgId);
            return;
        }
    }
}

void OfficeManager::handleUserPresence(const QJsonObject& payload) {
    QString userId = payload["userId"].toString();
    UserStatus status = static_cast<UserStatus>(payload["status"].toInt());

    if (m_users.contains(userId)) {
        m_users[userId].status = status;
        emit userStatusChanged(userId, status);
    }
}

void OfficeManager::handleReaction(const QJsonObject& payload) {
    QString msgId = payload["messageId"].toString();
    QString emoji = payload["emoji"].toString();
    QString userId = payload["userId"].toString();
    bool added = payload["added"].toBool();

    for (auto& msgs : m_messages) {
        for (auto& m : msgs) {
            if (m.id == msgId) {
                if (added) {
                    if (!m.reactions[emoji].contains(userId))
                        m.reactions[emoji].append(userId);
                    emit reactionAdded(msgId, emoji, userId);
                } else {
                    m.reactions[emoji].removeOne(userId);
                    if (m.reactions[emoji].isEmpty())
                        m.reactions.remove(emoji);
                    emit reactionRemoved(msgId, emoji, userId);
                }
                return;
            }
        }
    }
}

void OfficeManager::handleTyping(const QJsonObject& payload) {
    QString channelId = payload["channelId"].toString();
    QString userId = payload["userId"].toString();
    emit typingReceived(channelId, userId);
}

void OfficeManager::sendPacket(const ProtocolMessage& msg) {
    if (m_socket && m_connected) {
        QJsonDocument doc(msg.toJson());
        m_socket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
    }
}

void OfficeManager::requestSync() {
    sendPacket(ProtocolMessage::make("sync", {}));
}

// --- Public API ---

void OfficeManager::createChannel(const QString& name, ChannelType type) {
    sendPacket(ProtocolMessage::make("create_channel", {
        {"name", name},
        {"type", static_cast<int>(type)}
    }));
}

void OfficeManager::joinChannel(const QString& channelId) {
    sendPacket(ProtocolMessage::make("join_channel", {{"channelId", channelId}}));
}

void OfficeManager::leaveChannel(const QString& channelId) {
    sendPacket(ProtocolMessage::make("leave_channel", {{"channelId", channelId}}));
}

void OfficeManager::setActiveChannel(const QString& channelId) {
    if (m_activeChannelId != channelId) {
        m_activeChannelId = channelId;

        for (auto& ch : m_channels) {
            if (ch.id == channelId && ch.unreadCount > 0) {
                ch.unreadCount = 0;
                emit unreadCountChanged(channelId, 0);
            }
        }

        emit activeChannelChanged(channelId);
    }
}

void OfficeManager::sendMessage(const QString& channelId, const QString& content,
                                 MessageType type, const QString& replyTo) {
    OfficeMessage msg;
    msg.id = generateId();
    msg.channelId = channelId;
    msg.authorId = m_userId;
    msg.authorName = m_userName;
    msg.type = type;
    msg.content = content;
    msg.timestamp = QDateTime::currentDateTime();
    msg.replyToId = replyTo;

    m_messages[channelId].append(msg);
    emit messageReceived(msg);

    sendPacket(ProtocolMessage::make("message", msg.toJson()));
}

void OfficeManager::editMessage(const QString& messageId, const QString& newContent) {
    for (auto& msgs : m_messages) {
        for (auto& m : msgs) {
            if (m.id == messageId) {
                m.content = newContent;
                m.editedAt = QDateTime::currentDateTime();
                emit messageEdited(m);
                sendPacket(ProtocolMessage::make("edit_message", m.toJson()));
                return;
            }
        }
    }
}

void OfficeManager::deleteMessage(const QString& messageId) {
    for (auto it = m_messages.begin(); it != m_messages.end(); ++it) {
        for (auto& m : it.value()) {
            if (m.id == messageId) {
                m.isDeleted = true;
                emit messageDeleted(messageId);
                QJsonObject payload;
                payload["messageId"] = messageId;
                payload["channelId"] = it.key();
                sendPacket(ProtocolMessage::make("delete_message", payload));
                return;
            }
        }
    }
}

void OfficeManager::addReaction(const QString& messageId, const QString& emoji) {
    for (auto& msgs : m_messages) {
        for (auto& m : msgs) {
            if (m.id == messageId) {
                if (!m.reactions[emoji].contains(m_userId)) {
                    m.reactions[emoji].append(m_userId);
                    emit reactionAdded(messageId, emoji, m_userId);
                }
                sendPacket(ProtocolMessage::make("reaction", {
                    {"messageId", messageId},
                    {"emoji", emoji},
                    {"added", true}
                }));
                return;
            }
        }
    }
}

void OfficeManager::removeReaction(const QString& messageId, const QString& emoji) {
    for (auto& msgs : m_messages) {
        for (auto& m : msgs) {
            if (m.id == messageId) {
                m.reactions[emoji].removeOne(m_userId);
                if (m.reactions[emoji].isEmpty())
                    m.reactions.remove(emoji);
                emit reactionRemoved(messageId, emoji, m_userId);
                sendPacket(ProtocolMessage::make("reaction", {
                    {"messageId", messageId},
                    {"emoji", emoji},
                    {"added", false}
                }));
                return;
            }
        }
    }
}

void OfficeManager::sendTyping(const QString& channelId) {
    sendPacket(ProtocolMessage::make("typing", {{"channelId", channelId}}));
}

void OfficeManager::setUserName(const QString& name) {
    if (m_userName != name) {
        m_userName = name;
        if (m_connected) {
            sendPacket(ProtocolMessage::make("set_name", {{"userName", name}}));
        }
    }
}

QVector<OfficeMessage> OfficeManager::getMessages(const QString& channelId) const {
    return m_messages.value(channelId);
}

OfficeUser OfficeManager::getUser(const QString& userId) const {
    return m_users.value(userId);
}

QVector<OfficeChannel> OfficeManager::getDirectChannels() const {
    QVector<OfficeChannel> result;
    for (const auto& ch : m_channels) {
        if (ch.type == ChannelType::Direct) result.append(ch);
    }
    return result;
}

QVector<OfficeChannel> OfficeManager::getGroupChannels() const {
    QVector<OfficeChannel> result;
    for (const auto& ch : m_channels) {
        if (ch.type == ChannelType::Group) result.append(ch);
    }
    return result;
}

QVector<OfficeChannel> OfficeManager::getProjectChannels() const {
    QVector<OfficeChannel> result;
    for (const auto& ch : m_channels) {
        if (ch.type == ChannelType::Project) result.append(ch);
    }
    return result;
}

QString OfficeManager::generateId() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces).remove('-');
}

void OfficeManager::saveLocalData() {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(path);
    QFile file(path + "/ksoffice_local.json");
    if (!file.open(QIODevice::WriteOnly)) return;

    QJsonObject obj;
    obj["userId"] = m_userId;
    obj["userName"] = m_userName;

    QJsonArray chArr;
    for (const auto& ch : m_channels) chArr.append(ch.toJson());
    obj["channels"] = chArr;

    file.write(QJsonDocument(obj).toJson());
}

void OfficeManager::loadLocalData() {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QFile file(path + "/ksoffice_local.json");
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull()) return;

    QJsonObject obj = doc.object();
    m_userId = obj["userId"].toString();
    if (m_userId.isEmpty()) m_userId = generateId();
    m_userName = obj["userName"].toString();

    for (const auto& v : obj["channels"].toArray()) {
        m_channels.append(OfficeChannel::fromJson(v.toObject()));
    }
}

} // namespace ks::office
