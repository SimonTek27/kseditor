#include "CollabWebSocket.h"
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonArray>
#include <QPointF>
#include <QTimer>

CollabWebSocket::CollabWebSocket(QObject *parent)
    : QObject(parent)
    , m_socket(new QWebSocket(QString("ksEditor-Collab"), QWebSocketProtocol::VersionLatest, this))
    , m_reconnectTimer(new QTimer(this))
{
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &CollabWebSocket::reconnect);

    connect(m_socket, &QWebSocket::connected, this, &CollabWebSocket::onConnected);
    connect(m_socket, &QWebSocket::disconnected, this, &CollabWebSocket::onDisconnected);
    connect(m_socket, &QWebSocket::textMessageReceived, this, &CollabWebSocket::onTextMessage);
    connect(m_socket, &QWebSocket::errorOccurred, this, &CollabWebSocket::onError);
}

CollabWebSocket::~CollabWebSocket()
{
    disconnect();
}

void CollabWebSocket::connectToServer(const QString &serverUrl)
{
    m_serverUrl = serverUrl;
    m_reconnectAttempts = 0;
    m_socket->open(QUrl(serverUrl));
}

void CollabWebSocket::disconnect()
{
    m_reconnectTimer->stop();
    m_reconnectAttempts = MAX_RECONNECT_ATTEMPTS;
    m_socket->close();
    m_connected = false;
}

void CollabWebSocket::sendMessage(const QJsonObject &message)
{
    if (m_socket && m_connected) {
        m_socket->sendTextMessage(QJsonDocument(message).toJson(QJsonDocument::Compact));
    }
}

void CollabWebSocket::sendFileUpdate(const QString &fileId, const QByteArray &data)
{
    QJsonObject msg;
    msg["type"] = "file_update";
    msg["fileId"] = fileId;
    msg["data"] = QString::fromLatin1(data.toBase64());
    sendMessage(msg);
}

void CollabWebSocket::sendCursorPosition(const QPointF &position)
{
    QJsonObject msg;
    msg["type"] = "cursor_position";
    msg["x"] = position.x();
    msg["y"] = position.y();
    sendMessage(msg);
}

void CollabWebSocket::sendSelectionChanged(const QStringList &selectedObjects)
{
    QJsonObject msg;
    msg["type"] = "selection_changed";
    msg["objects"] = QJsonArray::fromStringList(selectedObjects);
    sendMessage(msg);
}

void CollabWebSocket::onMessageReceived(std::function<void(const QJsonObject&)> callback)
{
    m_messageCallback = callback;
}

void CollabWebSocket::onUserJoined(std::function<void(const QString&)> callback)
{
    m_joinCallback = callback;
}

void CollabWebSocket::onUserLeft(std::function<void(const QString&)> callback)
{
    m_leaveCallback = callback;
}

void CollabWebSocket::onTextMessage(const QString &message)
{
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isNull()) {
        handleMessage(doc.object());
    }
}

void CollabWebSocket::handleMessage(const QJsonObject &json)
{
    QString type = json["type"].toString();

    if (type == "message" && m_messageCallback) {
        m_messageCallback(json);
        emit messageReceived(json);
    }
    else if (type == "user_joined") {
        QString userId = json["userId"].toString();
        if (m_joinCallback) m_joinCallback(userId);
        m_userIds.insert(userId);
        emit userJoined(userId);
    }
    else if (type == "user_left") {
        QString userId = json["userId"].toString();
        if (m_leaveCallback) m_leaveCallback(userId);
        m_userIds.remove(userId);
        emit userLeft(userId);
    }
    else if (type == "auth_ok") {
        m_sessionId = json["sessionId"].toString();
        m_userId = json["userId"].toString();
        emit authenticated(m_sessionId, m_userId);
    }
}

void CollabWebSocket::onConnected()
{
    m_connected = true;
    m_reconnectAttempts = 0;
    emit connected();

    QJsonObject auth;
    auth["type"] = "auth";
    auth["app"] = "ksEditor";
    auth["version"] = "1.0";
    sendMessage(auth);
}

void CollabWebSocket::onDisconnected()
{
    m_connected = false;
    emit disconnected();

    if (m_reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
        m_reconnectTimer->start(RECONNECT_INTERVAL_MS);
    }
}

void CollabWebSocket::onError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    m_connected = false;
    emit connectionError(m_socket->errorString());
}

void CollabWebSocket::reconnect()
{
    if (m_connected) return;
    if (m_reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) return;

    m_reconnectAttempts++;
    m_socket->open(QUrl(m_serverUrl));
}
