#pragma once
#include <QObject>
#include <QJsonObject>
#include <QSet>
#include <functional>
#include <QWebSocket>

class CollabWebSocket : public QObject
{
    Q_OBJECT

public:
    explicit CollabWebSocket(QObject *parent = nullptr);
    ~CollabWebSocket();

    void connectToServer(const QString &serverUrl);
    void disconnect();
    void sendMessage(const QJsonObject &message);
    void sendFileUpdate(const QString &fileId, const QByteArray &data);
    void sendCursorPosition(const QPointF &position);
    void sendSelectionChanged(const QStringList &selectedObjects);

    bool isConnected() const { return m_connected; }

    void onMessageReceived(std::function<void(const QJsonObject&)> callback);
    void onUserJoined(std::function<void(const QString&)> callback);
    void onUserLeft(std::function<void(const QString&)> callback);

signals:
    void connected();
    void disconnected();
    void messageReceived(const QJsonObject &message);
    void userJoined(const QString &userId);
    void userLeft(const QString &userId);
    void connectionError(const QString &error);
    void authenticated(const QString &sessionId, const QString &userId);

private slots:
    void onTextMessage(const QString &message);

private:
    void handleMessage(const QJsonObject &message);
    void reconnect();
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError error);

    bool m_connected = false;
    QString m_serverUrl;
    QString m_sessionId;
    QString m_userId;
    int m_reconnectAttempts = 0;
    static constexpr int MAX_RECONNECT_ATTEMPTS = 5;
    static constexpr int RECONNECT_INTERVAL_MS = 3000;

    QWebSocket* m_socket = nullptr;
    QTimer* m_reconnectTimer = nullptr;
    QSet<QString> m_userIds;

    std::function<void(const QJsonObject&)> m_messageCallback;
    std::function<void(const QString&)> m_joinCallback;
    std::function<void(const QString&)> m_leaveCallback;
};