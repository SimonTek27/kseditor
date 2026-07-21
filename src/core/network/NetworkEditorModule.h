#pragma once

#include "core/editor/ModuleGuiBase.h"
#include <QTabWidget>
#include <QTreeWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QProgressBar>
#include <QLabel>
#include <QTimer>
#include <QWebSocket>
#include <QWebSocketServer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace ks {
namespace network {

class NetworkEditorModule : public ModuleGuiBase {
    Q_OBJECT
public:
    explicit NetworkEditorModule(QWidget* parent = nullptr);
    ~NetworkEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;

    QString moduleName() const override { return "Network & Cloud"; }
    QString moduleId() const override { return "network"; }
    int getModulePriority() const override { return 85; }

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

protected:
    void buildUI() override;
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onCloudSyncClicked();
    void onCollabStartClicked();
    void onCollabStopClicked();
    void onServerStartClicked();
    void onServerStopClicked();
    void onDiscoverClicked();
    void onSettingsChanged();
    void onWebSocketConnected();
    void onWebSocketDisconnected();
    void onWebSocketTextReceived(const QString& message);
    void onWebSocketError(QAbstractSocket::SocketError error);
    void onServerNewConnection();
    void onServerClosed();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onUploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void onSettingsSave();
    void onSettingsLoad();
    void refreshDeviceList();
    void onSyncStatusChanged();

private:
    void setupCloudSyncTab();
    void setupCollaborationTab();
    void setupWebSocketTab();
    void setupServerTab();
    void setupDiscoveryTab();
    void setupSettingsTab();
    void loadSettings();
    void saveSettings();
    void updateConnectionStatus(const QString& status, bool connected);
    void addLogEntry(const QString& msg, const QString& level = "info");
    void sendCollabMessage(const QString& text);
    void testCloudConnection();

    QTabWidget* m_tabWidget = nullptr;
    
    // Cloud Sync tab
    QWidget* m_cloudSyncTab = nullptr;
    QTreeWidget* m_syncTree = nullptr;
    QPushButton* m_syncBtn = nullptr;
    QPushButton* m_syncAllBtn = nullptr;
    QProgressBar* m_syncProgress = nullptr;
    QLabel* m_syncStatus = nullptr;
    QCheckBox* m_autoSyncCheck = nullptr;
    QSpinBox* m_syncIntervalSpin = nullptr;
    
    // Collaboration tab
    QWidget* m_collabTab = nullptr;
    QLineEdit* m_collabNameEdit = nullptr;
    QLineEdit* m_collabRoomEdit = nullptr;
    QPushButton* m_collabStartBtn = nullptr;
    QPushButton* m_collabStopBtn = nullptr;
    QTreeWidget* m_collabUsersTree = nullptr;
    QTextEdit* m_collabChat = nullptr;
    QLineEdit* m_collabMessageEdit = nullptr;
    QPushButton* m_collabSendBtn = nullptr;
    
    // WebSocket tab
    QWidget* m_wsTab = nullptr;
    QLineEdit* m_wsUrlEdit = nullptr;
    QPushButton* m_wsConnectBtn = nullptr;
    QPushButton* m_wsDisconnectBtn = nullptr;
    QTextEdit* m_wsLog = nullptr;
    QLineEdit* m_wsSendEdit = nullptr;
    QPushButton* m_wsSendBtn = nullptr;
    QComboBox* m_wsProtocolCombo = nullptr;
    
    // Server tab
    QWidget* m_serverTab = nullptr;
    QSpinBox* m_serverPortSpin = nullptr;
    QPushButton* m_serverStartBtn = nullptr;
    QPushButton* m_serverStopBtn = nullptr;
    QTreeWidget* m_serverClientsTree = nullptr;
    QTextEdit* m_serverLog = nullptr;
    QCheckBox* m_serverSslCheck = nullptr;
    
    // Discovery tab
    QWidget* m_discoveryTab = nullptr;
    QPushButton* m_discoverBtn = nullptr;
    QTreeWidget* m_discoveredDevicesTree = nullptr;
    QComboBox* m_discoveryTypeCombo = nullptr;
    QSpinBox* m_discoveryTimeoutSpin = nullptr;
    
    // Settings tab
    QWidget* m_settingsTab = nullptr;
    QComboBox* m_cloudProviderEdit = nullptr;
    QLineEdit* m_cloudApiKeyEdit = nullptr;
    QLineEdit* m_cloudBucketEdit = nullptr;
    QComboBox* m_cloudRegionCombo = nullptr;
    QCheckBox* m_cloudEncryptCheck = nullptr;
    QPushButton* m_settingsSaveBtn = nullptr;
    QPushButton* m_settingsLoadBtn = nullptr;

    // Network components
    QWebSocket* m_webSocket = nullptr;
    QWebSocketServer* m_webSocketServer = nullptr;
    QNetworkAccessManager* m_networkManager = nullptr;
    QTimer* m_syncTimer = nullptr;
    QTimer* m_discoveryTimer = nullptr;
    
    bool m_collabActive = false;
    bool m_serverActive = false;
    QString m_collabRoom;
    QString m_collabUserName;
};

} // namespace network
} // namespace ks