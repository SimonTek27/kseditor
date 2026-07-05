#include "CloudSyncModule.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDir>
#include <QDirIterator>
#include <QTimer>
#include <QDateTime>
#include <QCryptographicHash>
#include <QUrlQuery>
#include <QUrl>
#include <QEventLoop>
#include <QProcess>
#include <QHttpMultiPart>

namespace ks {

// ─── CloudSync ───────────────────────────────────────────────────────────────

bool CloudSync::login(const QString& username, const QString& password) {
    m_username = username;
    if (m_endpoint.isEmpty()) {
        m_loggedIn = true;
        m_token = QCryptographicHash::hash((username + password).toUtf8(), QCryptographicHash::Sha256).toHex().left(32);
        emit loginSuccess();
        return true;
    }

    QJsonObject loginData;
    loginData["username"] = username;
    loginData["password"] = password;
    loginData["apiKey"] = m_apiKey;

    QNetworkRequest request(QUrl(m_endpoint.toString() + "/api/auth/login"));
    request.setRawHeader("Content-Type", "application/json");
    request.setRawHeader("Accept", "application/json");

    QNetworkReply* reply = m_nam.post(request, QJsonDocument(loginData).toJson());
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit loginFailed(reply->errorString());
        return false;
    }

    QJsonObject resp = QJsonDocument::fromJson(reply->readAll()).object();
    m_token = resp["token"].toString();
    m_loggedIn = true;
    emit loginSuccess();
    return true;
}

void CloudSync::logout() {
    m_loggedIn = false;
    m_username.clear();
    m_token.clear();
    m_remoteProjects.clear();
    m_remoteAssets.clear();
}

bool CloudSync::uploadProject(const QString& localPath, const QString& remoteName) {
    if (!QFile::exists(localPath)) return false;
    emit uploadProgress(0);

    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly)) return false;
    QByteArray data = file.readAll();
    file.close();

    if (m_endpoint.isEmpty()) {
        m_remoteProjects[remoteName] = data;
        emit uploadProgress(100);
        emit syncComplete(true);
        return true;
    }

    QNetworkRequest request(QUrl(m_endpoint.toString() + "/api/projects/upload"));
    request.setRawHeader("Content-Type", "application/octet-stream");
    request.setRawHeader("X-File-Name", remoteName.toUtf8());
    if (!m_token.isEmpty())
        request.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());

    QNetworkReply* reply = m_nam.put(request, data);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    reply->deleteLater();

    emit uploadProgress(100);
    bool ok = reply->error() == QNetworkReply::NoError;
    emit syncComplete(ok);
    return ok;
}

bool CloudSync::downloadProject(const QString& remoteName, const QString& localPath) {
    if (m_endpoint.isEmpty()) {
        if (!m_remoteProjects.contains(remoteName)) return false;
        emit downloadProgress(0);

        QFile file(localPath);
        if (!file.open(QIODevice::WriteOnly)) return false;
        file.write(m_remoteProjects[remoteName]);
        file.close();

        emit downloadProgress(100);
        emit syncComplete(true);
        return true;
    }

    QUrl url(m_endpoint.toString() + "/api/projects/" + QUrl::toPercentEncoding(remoteName));
    QNetworkRequest request(url);
    if (!m_token.isEmpty())
        request.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());

    QNetworkReply* reply = m_nam.get(request);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit syncComplete(false);
        return false;
    }

    QDir dir = QFileInfo(localPath).absoluteDir();
    if (!dir.exists()) dir.mkpath(".");

    QFile file(localPath);
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(reply->readAll());
    file.close();

    emit downloadProgress(100);
    emit syncComplete(true);
    return true;
}

bool CloudSync::uploadAsset(const QString& localPath, const QString& remotePath) {
    if (!QFile::exists(localPath)) return false;

    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly)) return false;
    QByteArray data = file.readAll();
    file.close();

    if (m_endpoint.isEmpty()) {
        m_remoteAssets[remotePath] = data;
        emit assetUploaded(remotePath);
        return true;
    }

    QNetworkRequest request(QUrl(m_endpoint.toString() + "/api/assets/upload"));
    request.setRawHeader("Content-Type", "application/octet-stream");
    request.setRawHeader("X-File-Path", remotePath.toUtf8());
    if (!m_token.isEmpty())
        request.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());

    QNetworkReply* reply = m_nam.put(request, data);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    reply->deleteLater();

    bool ok = reply->error() == QNetworkReply::NoError;
    if (ok) emit assetUploaded(remotePath);
    return ok;
}

bool CloudSync::downloadAsset(const QString& remotePath, const QString& localPath) {
    if (m_endpoint.isEmpty()) {
        if (!m_remoteAssets.contains(remotePath)) return false;
        QDir dir = QFileInfo(localPath).absoluteDir();
        if (!dir.exists()) dir.mkpath(".");
        QFile file(localPath);
        if (!file.open(QIODevice::WriteOnly)) return false;
        file.write(m_remoteAssets[remotePath]);
        file.close();
        emit assetDownloaded(remotePath);
        return true;
    }

    QUrl url(m_endpoint.toString() + "/api/assets/" + QUrl::toPercentEncoding(remotePath));
    QNetworkRequest request(url);
    if (!m_token.isEmpty())
        request.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());

    QNetworkReply* reply = m_nam.get(request);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) return false;

    QDir dir = QFileInfo(localPath).absoluteDir();
    if (!dir.exists()) dir.mkpath(".");
    QFile file(localPath);
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(reply->readAll());
    file.close();

    emit assetDownloaded(remotePath);
    return true;
}

QVector<QString> CloudSync::listRemoteAssets(const QString& folder) {
    QVector<QString> result;

    if (m_endpoint.isEmpty()) {
        for (const auto& key : m_remoteAssets.keys()) {
            if (folder.isEmpty() || key.startsWith(folder)) result.append(key);
        }
        return result;
    }

    QUrl url(m_endpoint.toString() + "/api/assets/list");
    if (!folder.isEmpty()) url.setQuery("folder=" + QUrl::toPercentEncoding(folder));
    QNetworkRequest request(url);
    if (!m_token.isEmpty())
        request.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());

    QNetworkReply* reply = m_nam.get(request);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) return result;

    QJsonArray arr = QJsonDocument::fromJson(reply->readAll()).array();
    for (const QJsonValue& v : arr) result.append(v.toString());
    return result;
}

QVector<QString> CloudSync::listRemoteProjects() {
    QVector<QString> result;

    if (m_endpoint.isEmpty()) {
        for (const auto& key : m_remoteProjects.keys()) result.append(key);
        return result;
    }

    QUrl url(m_endpoint.toString() + "/api/projects");
    QNetworkRequest request(url);
    if (!m_token.isEmpty())
        request.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());

    QNetworkReply* reply = m_nam.get(request);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) return result;

    QJsonArray arr = QJsonDocument::fromJson(reply->readAll()).array();
    for (const QJsonValue& v : arr) result.append(v.toString());
    return result;
}

bool CloudSync::shareProject(const QString& projectId, const QString& userId, const QString& permission) {
    if (m_endpoint.isEmpty()) {
        if (!m_sharedProjects.contains(projectId))
            m_sharedProjects[projectId] = QMap<QString, QString>();
        m_sharedProjects[projectId][userId] = permission;
        emit projectShared(projectId, userId, permission);
        return true;
    }

    QJsonObject body;
    body["userId"] = userId;
    body["permission"] = permission;

    QNetworkRequest request(QUrl(m_endpoint.toString() + "/api/projects/" + projectId + "/share"));
    request.setRawHeader("Content-Type", "application/json");
    if (!m_token.isEmpty())
        request.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());

    QNetworkReply* reply = m_nam.post(request, QJsonDocument(body).toJson());
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    reply->deleteLater();

    bool ok = reply->error() == QNetworkReply::NoError;
    if (ok) emit projectShared(projectId, userId, permission);
    return ok;
}

QVector<QString> CloudSync::getCollaborators(const QString& projectId) {
    if (m_endpoint.isEmpty())
        return m_sharedProjects.value(projectId).keys().toVector();

    QUrl url(m_endpoint.toString() + "/api/projects/" + projectId + "/collaborators");
    QNetworkRequest request(url);
    if (!m_token.isEmpty())
        request.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());

    QNetworkReply* reply = m_nam.get(request);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    reply->deleteLater();

    QVector<QString> result;
    if (reply->error() != QNetworkReply::NoError) return result;

    QJsonArray arr = QJsonDocument::fromJson(reply->readAll()).array();
    for (const QJsonValue& v : arr) result.append(v.toString());
    return result;
}

// ─── MultiplayerServer ───────────────────────────────────────────────────────

bool MultiplayerServer::start(int port) {
    stop();
    m_port = port;
    m_server = new QTcpServer(this);

    QObject::connect(m_server, &QTcpServer::newConnection, this, [this]() {
        while (m_server->hasPendingConnections()) {
            QTcpSocket* socket = m_server->nextPendingConnection();
            QString clientId = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);

            Client client;
            client.id = clientId;
            client.name = QString("Client_%1").arg(clientId.left(6));
            client.address = socket->peerAddress();
            client.port = socket->peerPort();
            client.connected = true;
            client.socket = socket;

            QObject::connect(socket, &QTcpSocket::disconnected, this, [this, socket, clientId]() {
                for (int i = 0; i < m_clients.size(); ++i) {
                    if (m_clients[i].id == clientId) {
                        m_clients[i].connected = false;
                        emit clientDisconnected(clientId);
                        m_clients.removeAt(i);
                        break;
                    }
                }
                socket->deleteLater();
            });

            QObject::connect(socket, &QTcpSocket::readyRead, this, [this, socket, clientId]() {
                QByteArray data = socket->readAll();
                emit dataReceived(clientId, data);
            });

            m_clients.append(client);
            emit clientConnected(clientId);
        }
    });

    m_running = m_server->listen(QHostAddress::Any, port);
    if (!m_running) {
        delete m_server;
        m_server = nullptr;
    }
    return m_running;
}

void MultiplayerServer::stop() {
    m_running = false;
    if (m_server) {
        m_server->close();
        delete m_server;
        m_server = nullptr;
    }
    for (auto& client : m_clients) {
        if (client.socket) {
            client.socket->disconnectFromHost();
            client.socket->deleteLater();
        }
    }
    m_clients.clear();
}

void MultiplayerServer::broadcast(const QByteArray& data, const QString& excludeClient) {
    for (auto& client : m_clients) {
        if (client.id != excludeClient && client.socket && client.socket->isOpen()) {
            client.socket->write(data);
        }
    }
}

// ─── MultiplayerClient ───────────────────────────────────────────────────────

bool MultiplayerClient::connectToServer(const QString& host, int port, const QString& username) {
    m_host = host;
    m_port = port;
    m_username = username;

    if (!m_socket) {
        m_socket = new QTcpSocket(this);
        QObject::connect(m_socket, &QTcpSocket::connected, this, [this]() {
            m_connected = true;
            emit connected();
        });
        QObject::connect(m_socket, &QTcpSocket::disconnected, this, [this]() {
            m_connected = false;
            emit disconnected();
        });
        QObject::connect(m_socket, &QTcpSocket::readyRead, this, [this]() {
            QByteArray data = m_socket->readAll();
            emit serverMessage(data);
        });
        QObject::connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, [this](QAbstractSocket::SocketError) {
            m_connected = false;
        });
    }

    m_socket->connectToHost(host, port);
    if (m_socket->waitForConnected(5000)) {
        m_connected = true;
        emit connected();
        return true;
    }
    return false;
}

void MultiplayerClient::disconnect() {
    m_connected = false;
    if (m_socket) {
        m_socket->disconnectFromHost();
        if (m_socket->state() != QTcpSocket::UnconnectedState)
            m_socket->waitForDisconnected(3000);
    }
    emit disconnected();
}

void MultiplayerClient::sendData(const QByteArray& data) {
    if (m_socket && m_socket->state() == QTcpSocket::ConnectedState) {
        m_socket->write(data);
        emit dataSent(data.size());
    }
}

void MultiplayerClient::sendPosition(const QVector3D& pos, const QVector3D& velocity) {
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << pos.x() << pos.y() << pos.z();
    stream << velocity.x() << velocity.y() << velocity.z();
    sendData(data);
}

void MultiplayerClient::sendTelemetry(const QVariantMap& data) {
    QJsonDocument doc(QJsonObject::fromVariantMap(data));
    sendData(doc.toJson(QJsonDocument::Compact));
}

// ─── TelemetryServer ─────────────────────────────────────────────────────────

void TelemetryServer::startRecording(const QString& sessionName) {
    m_sessionName = sessionName;
    m_frameData.clear();
    m_lapTimes.clear();
    m_sectorTimes.clear();
    m_recording = true;
    emit recordingStarted();
}

void TelemetryServer::stopRecording() {
    m_recording = false;
    emit recordingStopped();
}

void TelemetryServer::recordFrame(const QVariantMap& telemetry) {
    if (m_recording) {
        m_frameData.append(telemetry);
    }
}

void TelemetryServer::addLap(int lapNumber, float time) {
    m_lapTimes.append(time);
    emit lapCompleted(lapNumber, time);
}

void TelemetryServer::addSector(int sector, float time) {
    m_sectorTimes.append(time);
    emit sectorCompleted(sector, time);
}

QString TelemetryServer::getSessionData() const {
    QJsonObject json;
    json["session"] = m_sessionName;
    QJsonArray lapsArr;
    for (int i = 0; i < m_lapTimes.size(); ++i) {
        QJsonObject lap;
        lap["lapNumber"] = i + 1;
        lap["time"] = m_lapTimes[i];
        QJsonArray sectors;
        for (float s : m_sectorTimes) sectors.append(s);
        lap["sectors"] = sectors;
        lapsArr.append(lap);
    }
    json["laps"] = lapsArr;
    return QJsonDocument(json).toJson();
}

void TelemetryServer::exportToCSV(const QString& path) {
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write("frame,speed,throttle,brake\n");
        for (int i = 0; i < m_frameData.size(); ++i) {
            const auto& f = m_frameData[i];
            file.write(QString("%1,%2,%3,%4\n")
                .arg(i).arg(f.value("speed").toDouble())
                .arg(f.value("throttle").toDouble())
                .arg(f.value("brake").toDouble()).toUtf8());
        }
        file.close();
    }
}

void TelemetryServer::exportToJSON(const QString& path) {
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonArray arr;
        for (const auto& frame : m_frameData) {
            QJsonObject obj;
            for (auto it = frame.constBegin(); it != frame.constEnd(); ++it)
                obj[it.key()] = QJsonValue::fromVariant(it.value());
            arr.append(obj);
        }
        file.write(QJsonDocument(arr).toJson());
        file.close();
    }
}

// ─── WebDashboard ────────────────────────────────────────────────────────────

void WebDashboard::startServer(int port) {
    stopServer();
    m_port = port;
    m_httpServer = new QTcpServer(this);

    QObject::connect(m_httpServer, &QTcpServer::newConnection, this, [this]() {
        while (m_httpServer->hasPendingConnections()) {
            QTcpSocket* socket = m_httpServer->nextPendingConnection();
            socket->setParent(m_httpServer);
            QObject::connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
                QByteArray data = socket->readAll();
                if (data.contains("\r\n\r\n")) {
                    QString req = QString::fromUtf8(data);
                    QStringList lines = req.split("\r\n");
                    QStringList firstLine = lines[0].split(' ');
                    QString method = firstLine.value(0);
                    QString path = firstLine.value(1);
                    handleRequest(socket, method, path);
                }
            });
        }
    });

    m_running = m_httpServer->listen(QHostAddress::Any, port);
    if (m_running)
        emit serverStarted(port);
    else {
        delete m_httpServer;
        m_httpServer = nullptr;
    }
}

void WebDashboard::stopServer() {
    m_running = false;
    if (m_httpServer) {
        m_httpServer->close();
        delete m_httpServer;
        m_httpServer = nullptr;
    }
    emit serverStopped();
}

void WebDashboard::handleRequest(QTcpSocket* socket, const QString& method, const QString& path) {
    QByteArray response;
    QString contentType = "text/plain";

    if (path == "/" || path == "/index.html") {
        contentType = "text/html; charset=utf-8";
        response = R"(<!DOCTYPE html><html><head><title>ksEditor Dashboard</title>
<style>body{font-family:sans-serif;background:#1a1a2e;color:#eee;margin:40px}
h1{color:#00d4ff}.data{background:#16213e;padding:20px;border-radius:8px;margin:10px 0}
table{width:100%;border-collapse:collapse}td,th{padding:8px;text-align:left;border-bottom:1px solid #333}
</style></head><body><h1>ksEditor Dashboard</h1>
<div class='data'><h2>Server Status</h2><p>Running on port )" + QByteArray::number(m_port) + R"(</p></div>
<div class='data'><h2>Telemetry</h2><p>Connected: )" +
(m_telemetry ? "Yes" : "No") + R"(</p></div>
<div class='data'><h2>API Endpoints</h2>
<table><tr><th>Path</th><th>Method</th><th>Description</th></tr>
<tr><td>/api/telemetry</td><td>GET</td><td>Latest telemetry data</td></tr>
<tr><td>/api/laps</td><td>GET</td><td>Lap times</td></tr>
<tr><td>/api/status</td><td>GET</td><td>Server status</td></tr>
</table></div></body></html>)";
    } else if (path == "/api/telemetry") {
        contentType = "application/json";
        QJsonObject data;
        data["server"] = "ksEditor Dashboard";
        data["recording"] = m_telemetry ? m_telemetry->isRecording() : false;
        data["lapsRecorded"] = m_telemetry ? m_telemetry->getLapTimes().size() : 0;
        response = QJsonDocument(data).toJson();
    } else if (path == "/api/laps") {
        contentType = "application/json";
        if (m_telemetry) {
            QJsonArray laps;
            for (float t : m_telemetry->getLapTimes()) laps.append(t);
            response = QJsonDocument(laps).toJson();
        } else {
            response = "[]";
        }
    } else if (path == "/api/status") {
        contentType = "application/json";
        QJsonObject status;
        status["running"] = m_running;
        status["port"] = m_port;
        status["telemetry"] = m_telemetry != nullptr;
        response = QJsonDocument(status).toJson();
    } else {
        response = "Not Found";
        socket->write("HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found");
        socket->disconnectFromHost();
        return;
    }

    QByteArray header;
    header += "HTTP/1.1 200 OK\r\n";
    header += "Content-Type: " + contentType.toLatin1() + "\r\n";
    header += "Content-Length: " + QByteArray::number(response.size()) + "\r\n";
    header += "Access-Control-Allow-Origin: *\r\n";
    header += "Connection: close\r\n\r\n";
    socket->write(header + response);
    socket->disconnectFromHost();
}

// ─── RemoteAPI ───────────────────────────────────────────────────────────────

QVariantMap RemoteAPI::getServerInfo() {
    QVariantMap info;
    info["name"] = "Local Server";
    info["version"] = "1.0";
    info["track"] = "Imola";
    info["cars"] = 0;
    return info;
}

QVector<QVariantMap> RemoteAPI::getConnectedPlayers() {
    if (!m_sessionActive) return {};
    QVector<QVariantMap> players;
    QVariantMap local;
    local["name"] = "Host";
    local["car"] = m_currentCar;
    local["track"] = m_currentTrack;
    local["ping"] = 0;
    local["isHost"] = true;
    players.append(local);
    return players;
}

bool RemoteAPI::startSession(const QString& track, const QString& car) {
    m_currentTrack = track;
    m_currentCar = car;
    m_sessionActive = true;
    emit sessionStarted(track, car);
    return true;
}

bool RemoteAPI::stopSession() {
    m_sessionActive = false;
    emit sessionStopped();
    return true;
}

bool RemoteAPI::restartSession() {
    if (m_sessionActive) {
        emit sessionRestarted();
        return true;
    }
    return false;
}

void RemoteAPI::setTimeScale(float scale) {
    m_timeScale = qBound(0.1f, scale, 10.0f);
    emit timeScaleChanged(m_timeScale);
}

void RemoteAPI::setWeather(const QString& condition) {
    m_weatherCondition = condition;
    emit weatherChanged(condition);
}

void RemoteAPI::kickPlayer(const QString& playerId) {
    m_kickedPlayers.append(playerId);
    emit playerKicked(playerId);
}

bool RemoteAPI::executeCommand(const QString& command) {
    m_commandHistory.append(command);
    emit commandExecuted(command);
    return true;
}

// ─── CloudSyncService ────────────────────────────────────────────────────────

CloudSyncService::CloudSyncService(QObject* parent)
    : QObject(parent)
{
    m_networkManager = new QNetworkAccessManager(this);
}

CloudSyncService::~CloudSyncService()
{
}

void CloudSyncService::setServerUrl(const QString& url)
{
    m_serverUrl = url;
}

void CloudSyncService::setAuthToken(const QString& token)
{
    m_authToken = token;
}

namespace {
QNetworkRequest makeApiRequest(const QString& urlStr, const QString& authToken)
{
    using Req = QNetworkRequest;
    QUrl apiUrl(urlStr);
    Req request(apiUrl);
    request.setRawHeader("Content-Type", "application/json");
    if (!authToken.isEmpty())
        request.setRawHeader("Authorization", ("Bearer " + authToken).toUtf8());
    return request;
}
}

bool CloudSyncService::login(const QString& username, const QString& password)
{
    if (m_serverUrl.isEmpty()) {
        emit loginFailed("Server URL not set");
        return false;
    }

    QJsonObject loginData;
    loginData["username"] = username;
    loginData["password"] = password;

    QNetworkRequest request = makeApiRequest(m_serverUrl + "/api/auth/login", QString());
    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(loginData).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, username]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            m_status = Status_Error;
            emit loginFailed(reply->errorString());
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();
        m_loggedIn = true;
        m_userId = obj.value("userId").toString(username);
        m_authToken = obj.value("token").toString();
        m_status = Status_Idle;
        emit loginSuccess(m_userId);
    });

    m_status = Status_Syncing;
    return true;
}

void CloudSyncService::logout()
{
    if (m_loggedIn && !m_serverUrl.isEmpty()) {
        QNetworkRequest request = makeApiRequest(m_serverUrl + "/api/auth/logout", m_authToken);
        QNetworkReply* reply = m_networkManager->post(request, QByteArray("{}"));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    }
    m_loggedIn = false;
    m_authToken.clear();
    m_userId.clear();
    emit logoutComplete();
}

void CloudSyncService::syncProject(const QString& localPath)
{
    if (!m_loggedIn) {
        emit syncError("Not logged in");
        return;
    }

    QDir dir(localPath);
    if (!dir.exists()) {
        emit syncError("Directory does not exist: " + localPath);
        return;
    }

    emit syncStarted();
    m_status = Status_Syncing;
    m_syncProgress = 0.0f;

    QStringList filters;
    QDirIterator it(localPath, QDir::Files, QDirIterator::Subdirectories);
    QStringList files;
    while (it.hasNext()) files.append(it.next());
    int totalFiles = files.size();
    if (totalFiles == 0) {
        m_status = Status_Idle;
        emit syncComplete(0, 0);
        return;
    }

    struct SyncState {
        int uploaded = 0, downloaded = 0, processed = 0;
    };
    auto state = std::make_shared<SyncState>();

    for (const QString& filePath : files) {
        QString checksum;
        if (!calculateChecksum(filePath, checksum)) {
            state->processed++;
            m_syncProgress = (state->processed * 100.0f) / totalFiles;
            emit syncProgress(m_syncProgress);
            continue;
        }

        QFileInfo info(filePath);
        QString relativePath = info.absoluteFilePath().mid(localPath.length() + 1).replace('\\', '/');

        QJsonObject meta;
        meta["path"] = relativePath;
        meta["checksum"] = checksum;
        meta["size"] = static_cast<qint64>(info.size());

        QNetworkRequest request = makeApiRequest(m_serverUrl + "/api/files/check", m_authToken);
        QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(meta).toJson());

        connect(reply, &QNetworkReply::finished, this, [this, reply, filePath, relativePath, checksum, totalFiles, state]() {
            reply->deleteLater();
            bool needsUpload = true;
            if (reply->error() == QNetworkReply::NoError) {
                QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
                needsUpload = !doc.object().value("exists").toBool();
            }

            if (needsUpload) {
                QFile file(filePath);
                if (file.open(QIODevice::ReadOnly)) {
                    QByteArray fileData = file.readAll();
                    file.close();

                    QNetworkRequest upReq = makeApiRequest(m_serverUrl + "/api/files/upload", m_authToken);
                    upReq.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
                    upReq.setRawHeader("X-File-Path", relativePath.toUtf8());
                    upReq.setRawHeader("X-File-Checksum", checksum.toUtf8());

                    QNetworkReply* upReply = m_networkManager->put(upReq, fileData);
                    connect(upReply, &QNetworkReply::finished, this, [this, upReply, state, totalFiles]() {
                        upReply->deleteLater();
                        if (upReply->error() == QNetworkReply::NoError) state->uploaded++;
                    });
                }
            } else {
                state->downloaded--;
            }

            state->processed++;
            m_syncProgress = (state->processed * 100.0f) / totalFiles;
            emit syncProgress(m_syncProgress);

            if (state->processed >= totalFiles) {
                m_status = Status_Idle;
                emit syncComplete(state->uploaded, qMax(0, state->downloaded));
            }
        });
    }
}

void CloudSyncService::syncFile(const QString& localPath)
{
    if (!m_loggedIn) {
        emit syncError("Not logged in");
        return;
    }

    QFileInfo info(localPath);
    if (!info.exists()) {
        emit syncError("File does not exist: " + localPath);
        return;
    }

    emit syncStarted();

    QString checksum;
    if (!calculateChecksum(localPath, checksum)) {
        emit syncError("Failed to calculate checksum");
        return;
    }

    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit syncError("Failed to open file: " + localPath);
        return;
    }
    QByteArray fileData = file.readAll();
    file.close();

    QNetworkRequest request = makeApiRequest(m_serverUrl + "/api/files/upload", m_authToken);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    request.setRawHeader("X-File-Path", info.fileName().toUtf8());
    request.setRawHeader("X-File-Checksum", checksum.toUtf8());

    QNetworkReply* reply = m_networkManager->put(request, fileData);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            emit syncComplete(1, 0);
        } else {
            emit syncError("Upload failed: " + reply->errorString());
        }
    });
}

void CloudSyncService::downloadFile(const QString& remoteId, const QString& localPath)
{
    if (!m_loggedIn) {
        emit syncError("Not logged in");
        return;
    }

    QDir().mkpath(QFileInfo(localPath).absolutePath());

    QNetworkRequest request = makeApiRequest(m_serverUrl + "/api/files/" + remoteId + "/download", m_authToken);
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, localPath]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit syncError("Download failed: " + reply->errorString());
            return;
        }
        QFile file(localPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(reply->readAll());
            file.close();
            emit syncProgress(100.0f);
            emit fileChanged(localPath);
        } else {
            emit syncError("Cannot write to: " + localPath);
        }
    });
}

void CloudSyncService::uploadFile(const QString& localPath)
{
    if (!m_loggedIn) {
        emit syncError("Not logged in");
        return;
    }

    QFileInfo info(localPath);
    if (!info.exists()) {
        emit syncError("File does not exist: " + localPath);
        return;
    }

    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit syncError("Cannot open file: " + localPath);
        return;
    }
    QByteArray fileData = file.readAll();
    file.close();

    QString checksum;
    calculateChecksum(localPath, checksum);

    QNetworkRequest request = makeApiRequest(m_serverUrl + "/api/files/upload", m_authToken);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    request.setRawHeader("X-File-Path", info.fileName().toUtf8());
    request.setRawHeader("X-File-Checksum", checksum.toUtf8());

    QNetworkReply* reply = m_networkManager->put(request, fileData);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            emit syncComplete(1, 0);
        } else {
            emit syncError("Upload failed: " + reply->errorString());
        }
    });
}

void CloudSyncService::refreshFileList()
{
    QNetworkRequest request = makeApiRequest(m_serverUrl + "/api/files", m_authToken);
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray arr = doc.array();

        m_remoteFiles.clear();
        for (const QJsonValue& val : arr) {
            QJsonObject obj = val.toObject();
            CloudFile f;
            f.id = obj["id"].toString();
            f.path = obj["path"].toString();
            f.name = obj["name"].toString();
            f.size = static_cast<qint64>(obj["size"].toDouble());
            f.modifiedDate = QDateTime::fromString(obj["modifiedDate"].toString(), Qt::ISODate);
            f.checksum = obj["checksum"].toString();
            f.isFolder = obj["isFolder"].toBool();
            QStringList tags;
            for (const QJsonValue& t : obj["tags"].toArray()) tags.append(t.toString());
            f.tags = tags;
            m_remoteFiles.append(f);
        }
    });
}

QVector<CloudSyncService::CloudFile> CloudSyncService::listFiles(const QString& path)
{
    if (m_loggedIn) refreshFileList();
    return m_remoteFiles;
}

QVector<CloudSyncService::CloudFile> CloudSyncService::searchFiles(const QString& query)
{
    QVector<CloudFile> results;

    if (m_loggedIn && !m_serverUrl.isEmpty()) {
        QUrl url(m_serverUrl + "/api/files");
        QUrlQuery urlQuery;
        urlQuery.addQueryItem("q", query);
        url.setQuery(urlQuery);

        QNetworkRequest request = makeApiRequest(url.toString(), m_authToken);
        QNetworkReply* reply = m_networkManager->get(request);

        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.processEvents(QEventLoop::WaitForMoreEvents);
        reply->deleteLater();
    }

    for (const CloudFile& file : m_remoteFiles) {
        if (file.name.contains(query, Qt::CaseInsensitive)) {
            results.append(file);
        }
    }

    return results;
}

void CloudSyncService::createFolder(const QString& path)
{
    if (!m_loggedIn) {
        emit syncError("Not logged in");
        return;
    }

    QJsonObject body;
    body["path"] = path;

    QNetworkRequest request = makeApiRequest(m_serverUrl + "/api/folders", m_authToken);
    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(body).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, path]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit syncError("Failed to create folder: " + reply->errorString());
            return;
        }
        CloudFile folder;
        folder.id = "folder_" + QString::number(QDateTime::currentMSecsSinceEpoch());
        folder.path = path;
        folder.name = path.section('/', -1);
        folder.isFolder = true;
        folder.modifiedDate = QDateTime::currentDateTime();
        m_remoteFiles.append(folder);
        emit syncComplete(0, 0);
    });
}

void CloudSyncService::deleteFile(const QString& remoteId)
{
    if (!m_loggedIn) {
        emit syncError("Not logged in");
        return;
    }

    QNetworkRequest request = makeApiRequest(m_serverUrl + "/api/files/" + remoteId, m_authToken);
    QNetworkReply* reply = m_networkManager->deleteResource(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, remoteId]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            for (int i = 0; i < m_remoteFiles.size(); ++i) {
                if (m_remoteFiles[i].id == remoteId) {
                    m_remoteFiles.removeAt(i);
                    break;
                }
            }
        }
    });
}

void CloudSyncService::moveFile(const QString& remoteId, const QString& newPath)
{
    if (!m_loggedIn) {
        emit syncError("Not logged in");
        return;
    }

    QJsonObject body;
    body["path"] = newPath;

    QNetworkRequest request = makeApiRequest(m_serverUrl + "/api/files/" + remoteId + "/move", m_authToken);
    QNetworkReply* reply = m_networkManager->put(request, QJsonDocument(body).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, remoteId, newPath]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            for (auto& file : m_remoteFiles) {
                if (file.id == remoteId) {
                    file.path = newPath;
                    file.name = newPath.section('/', -1);
                    break;
                }
            }
        }
    });
}

void CloudSyncService::resolveConflict(const SyncConflict& conflict)
{
    switch (conflict.resolution) {
        case SyncConflict::KeepLocal:
            uploadFile(conflict.localPath);
            break;
        case SyncConflict::KeepRemote:
            downloadFile(conflict.remotePath, conflict.localPath);
            break;
        case SyncConflict::KeepBoth:
        {
            int dot = conflict.localPath.lastIndexOf('.');
            QString suffix = (dot > 0) ? conflict.localPath.mid(dot) : "";
            QString base = (dot > 0) ? conflict.localPath.left(dot) : conflict.localPath;
            downloadFile(conflict.remotePath, base + "_remote" + suffix);
            break;
        }
    }

    m_conflicts.removeAll(conflict);
}

void CloudSyncService::setAutoSyncEnabled(bool enabled)
{
    m_autoSync = enabled;
}

void CloudSyncService::setSyncInterval(int minutes)
{
    m_syncInterval = minutes;
}

QStringList CloudSyncService::getSyncHistory(const QString& path)
{
    if (m_loggedIn) refreshFileList();

    QStringList history;
    for (const auto& file : m_remoteFiles) {
        if (file.path == path || path.isEmpty()) {
            history.append(QString("%1|%2|%3")
                .arg(file.name)
                .arg(file.modifiedDate.toString(Qt::ISODate))
                .arg(file.checksum));
        }
    }
    return history;
}

void CloudSyncService::clearSyncHistory()
{
    m_remoteFiles.clear();
}

void CloudSyncService::uploadFileAsync(const QString& localPath)
{
    uploadFile(localPath);
}

void CloudSyncService::downloadFileAsync(const QString& remoteId, const QString& localPath)
{
    downloadFile(remoteId, localPath);
}

bool CloudSyncService::calculateChecksum(const QString& path, QString& checksum)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(&file);
    checksum = hash.result().toHex();

    file.close();
    return true;
}

void CloudSyncService::processQueue()
{
    if (m_conflicts.isEmpty()) return;

    m_status = Status_Syncing;
    int total = m_conflicts.size();
    int processed = 0;

    while (!m_conflicts.isEmpty()) {
        SyncConflict conflict = m_conflicts.first();
        resolveConflict(conflict);
        processed++;
        m_syncProgress = static_cast<float>(processed) / total * 100.0f;
        emit syncProgress(m_syncProgress);
    }

    m_status = Status_Idle;
    emit syncComplete(0, processed);
}

// ─── CloudPresetLibrary ──────────────────────────────────────────────────────

CloudPresetLibrary::CloudPresetLibrary(QObject* parent)
    : QObject(parent)
{
}

CloudPresetLibrary::~CloudPresetLibrary()
{
}

void CloudPresetLibrary::setCloudService(CloudSyncService* service)
{
    m_cloudService = service;
}

void CloudPresetLibrary::setAuthToken(const QString& token)
{
    m_authToken = token;
}

QVector<CloudPresetLibrary::Preset> CloudPresetLibrary::searchPresets(const QString& query, const QString& category)
{
    QVector<Preset> results;

    if (!m_cloudService || m_cloudService->serverUrl().isEmpty()) {
        emit error("Cloud service not configured");
        return results;
    }

    QUrlQuery urlQuery;
    if (!query.isEmpty()) urlQuery.addQueryItem("q", query);
    if (!category.isEmpty()) urlQuery.addQueryItem("category", category);
    urlQuery.addQueryItem("limit", "50");

    QUrl url(m_cloudService->serverUrl() + "/api/presets/search");
    url.setQuery(urlQuery);

    QNetworkRequest request(url);
    request.setRawHeader("Content-Type", "application/json");
    if (!m_authToken.isEmpty())
        request.setRawHeader("Authorization", ("Bearer " + m_authToken).toUtf8());

    QNetworkReply* reply = m_cloudService->networkManager()->get(request);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit error(reply->errorString());
        return results;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonArray arr = doc.array();
    for (const QJsonValue& val : arr) {
        QJsonObject obj = val.toObject();
        Preset preset;
        preset.id = obj["id"].toString();
        preset.name = obj["name"].toString();
        preset.description = obj["description"].toString();
        preset.category = obj["category"].toString();
        preset.author = obj["author"].toString();
        preset.authorId = obj["authorId"].toString();
        preset.downloads = obj["downloads"].toInt();
        preset.rating = static_cast<float>(obj["rating"].toDouble());
        preset.createdDate = QDateTime::fromString(obj["createdDate"].toString(), Qt::ISODate);
        preset.updatedDate = QDateTime::fromString(obj["updatedDate"].toString(), Qt::ISODate);
        preset.downloadUrl = obj["downloadUrl"].toString();
        preset.thumbnailUrl = obj["thumbnailUrl"].toString();
        preset.fileSize = static_cast<qint64>(obj["fileSize"].toDouble());
        QJsonArray tagsArr = obj["tags"].toArray();
        for (const QJsonValue& t : tagsArr) preset.tags.append(t.toString());
        results.append(preset);
    }

    emit searchResults(results);
    return results;
}

QVector<CloudPresetLibrary::Preset> CloudPresetLibrary::getFeaturedPresets()
{
    return searchPresets("Featured");
}

QVector<CloudPresetLibrary::Preset> CloudPresetLibrary::getPopularPresets(int count)
{
    QVector<Preset> results = searchPresets("Popular");
    if (results.isEmpty()) {
        // Fallback: search by most downloaded
        results = searchPresets("trending");
    }
    // Limit to requested count
    while (results.size() > count)
        results.removeLast();
    return results;
}

QVector<CloudPresetLibrary::Preset> CloudPresetLibrary::getMyPresets()
{
    QVector<Preset> results;
    if (m_authToken.isEmpty()) return results;
    for (const auto& p : searchPresets("")) {
        if (p.authorId == m_authToken)
            results.append(p);
    }
    return results;
}

bool CloudPresetLibrary::downloadPreset(const QString& presetId, const QString& localPath)
{
    if (!m_cloudService || m_cloudService->serverUrl().isEmpty()) {
        emit downloadProgress(presetId, 0.0f);
        emit downloadProgress(presetId, 100.0f);
        emit downloadComplete(presetId);
        return true;
    }

    QString downloadUrl = m_cloudService->serverUrl() + "/api/presets/" + presetId + "/download";
    QNetworkRequest request{QUrl(downloadUrl)};
    if (!m_authToken.isEmpty())
        request.setRawHeader("Authorization", ("Bearer " + m_authToken).toUtf8());

    QNetworkReply* reply = m_cloudService->networkManager()->get(request);

    QObject::connect(reply, &QNetworkReply::downloadProgress, this,
        [this, presetId](qint64 received, qint64 total) {
            float pct = total > 0 ? (float)received / total * 100.0f : 0.0f;
            emit downloadProgress(presetId, pct);
        });

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit error(reply->errorString());
        return false;
    }

    QDir dir = QFileInfo(localPath).absoluteDir();
    if (!dir.exists()) dir.mkpath(".");

    QFile file(localPath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit error("Cannot write to " + localPath);
        return false;
    }
    file.write(reply->readAll());
    file.close();

    emit downloadProgress(presetId, 100.0f);
    emit downloadComplete(presetId);
    return true;
}

bool CloudPresetLibrary::uploadPreset(const QString& localPath, const QString& name,
                                     const QString& description, const QString& category)
{
    Preset preset;
    preset.id = "uploaded_" + QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
    preset.name = name;
    preset.description = description;
    preset.category = category;

    if (m_cloudService && !m_cloudService->serverUrl().isEmpty() && QFile::exists(localPath)) {
        QFile file(localPath);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            file.close();

            QJsonObject meta;
            meta["name"] = name;
            meta["description"] = description;
            meta["category"] = category;

            QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
            QHttpPart metaPart;
            metaPart.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            metaPart.setBody(QJsonDocument(meta).toJson());
            multiPart->append(metaPart);

            QHttpPart filePart;
            filePart.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
            filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                QString("form-data; name=\"file\"; filename=\"%1\"").arg(QFileInfo(localPath).fileName()));
            filePart.setBody(data);
            multiPart->append(filePart);

            QNetworkRequest request(QUrl(m_cloudService->serverUrl() + "/api/presets/upload"));
            if (!m_authToken.isEmpty())
                request.setRawHeader("Authorization", ("Bearer " + m_authToken).toUtf8());

            QNetworkReply* reply = m_cloudService->networkManager()->post(request, multiPart);
            multiPart->setParent(reply);

            QEventLoop loop;
            QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();
            reply->deleteLater();

            if (reply->error() == QNetworkReply::NoError) {
                QJsonObject resp = QJsonDocument::fromJson(reply->readAll()).object();
                preset.id = resp.value("id").toString(preset.id);
            }
        }
    }

    emit uploadComplete(preset.id);
    return true;
}

void CloudPresetLibrary::ratePreset(const QString& presetId, int rating)
{
    int clampedRating = qBound(1, rating, 5);
    m_presetRatings[presetId] = clampedRating;
    emit presetRated(presetId, clampedRating);
}

void CloudPresetLibrary::reportPreset(const QString& presetId, const QString& reason)
{
    m_presetReports[presetId].append(reason);
    emit presetReported(presetId, reason);
}

QStringList CloudPresetLibrary::getCategories() const
{
    return { "Car Setup", "Track", "Liveries", "Audio", "Physics", "Textures" };
}

// ─── CloudBackupSystem ───────────────────────────────────────────────────────

CloudBackupSystem::CloudBackupSystem(QObject* parent)
    : QObject(parent)
{
}

CloudBackupSystem::~CloudBackupSystem()
{
}

void CloudBackupSystem::setCloudService(CloudSyncService* service)
{
    m_cloudService = service;
}

void CloudBackupSystem::createBackup(const QString& name, const QStringList& paths)
{
    BackupPoint backup;
    backup.id = "backup_" + QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
    backup.name = name;
    backup.createdDate = QDateTime::currentDateTime();
    backup.includedPaths = paths;
    backup.fileCount = paths.size();

    int totalSize = 0;
    QString backupDir = QDir::temp().absoluteFilePath("ksbackup_" + backup.id);
    QDir().mkpath(backupDir);

    for (int i = 0; i < paths.size(); ++i) {
        const QString& path = paths[i];
        QFileInfo info(path);
        totalSize += info.size();

        emit backupProgress((float)i / paths.size() * 50.0f);

        QString dest = backupDir + "/" + info.fileName();
        if (info.isFile()) {
            QFile::copy(path, dest);
        } else if (info.isDir()) {
            QDirIterator it(path, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                QString src = it.next();
                QString rel = QDir(path).relativeFilePath(src);
                QString target = backupDir + "/" + rel;
                if (QFileInfo(src).isDir())
                    QDir().mkpath(target);
                else
                    QFile::copy(src, target);
            }
        }
    }
    backup.size = totalSize;

    // Upload to cloud if available
    if (m_cloudService && !m_cloudService->serverUrl().isEmpty()) {
        QString archivePath = backupDir + ".zip";
        QProcess zip;
        zip.setWorkingDirectory(QDir::temp().absolutePath());
        zip.start("zip", QStringList() << "-r" << archivePath << QFileInfo(backupDir).fileName());
        zip.waitForFinished(30000);

        if (zip.exitCode() == 0) {
            m_cloudService->uploadFile(archivePath);
            emit backupProgress(75.0f);
        }
    }

    m_backups.append(backup);
    emit backupCreated(backup);
    emit backupComplete();
}

void CloudBackupSystem::restoreBackup(const QString& backupId, const QString& targetPath)
{
    BackupPoint backup;
    for (const auto& b : m_backups) {
        if (b.id == backupId) {
            backup = b;
            break;
        }
    }

    QDir().mkpath(targetPath);

    emit restoreProgress(100.0f);
    emit restoreComplete();
}

void CloudBackupSystem::deleteBackup(const QString& backupId)
{
    for (int i = 0; i < m_backups.size(); ++i) {
        if (m_backups[i].id == backupId) {
            m_backups.removeAt(i);
            break;
        }
    }
}

QVector<CloudBackupSystem::BackupPoint> CloudBackupSystem::getBackups() const
{
    return m_backups;
}

CloudBackupSystem::BackupPoint CloudBackupSystem::getBackup(const QString& backupId) const
{
    for (const auto& backup : m_backups) {
        if (backup.id == backupId) {
            return backup;
        }
    }
    return BackupPoint();
}

void CloudBackupSystem::setAutoBackupEnabled(bool enabled)
{
    m_autoBackup = enabled;
}

void CloudBackupSystem::setAutoBackupInterval(int hours)
{
    m_autoBackupInterval = hours;
}

void CloudBackupSystem::setMaxBackups(int max)
{
    m_maxBackups = max;
}

} // namespace ks
