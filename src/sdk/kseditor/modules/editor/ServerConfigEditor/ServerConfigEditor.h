#pragma once

#include <QString>
#include <QMap>
#include <QVector>
#include <QJsonObject>

/**
 * @brief Server Configuration Editor for Assetto Corsa
 *
 * Manages AC dedicated server configuration files.
 * Based on community tools:
 * - AssettoServer (github.com/compujuckel/AssettoServer)
 * - acweb (github.com/assetto-corsa-web/acweb)
 * - plugin-dynamic-conditions (github.com/ac-custom-shaders-patch/plugin-dynamic-conditions)
 *
 * Supports:
 * - server_cfg.ini configuration
 * - extra_cfg.yml (Content Manager wrapper)
 * - entry_list.ini management
 * - Weather and time settings
 * - CSP extension settings
 */
class ServerConfigEditor {
public:
    struct ServerConfig {
        // Basic settings
        QString name;
        QString description;
        QString password;
        QString adminPassword;

        // Network
        QString ip;
        int port = 9600;
        int httpPort = 8080;
        int maxClients = 20;
        int slotCount = 30;

        // Session settings
        int track = 0;
        int sessionType = 0; // 0=practice, 1=qualifying, 2=race
        int sessionDuration = 600; // seconds
        int lapsCount = 0;
        int waitTime = 15;

        // Weather
        QString weather;
        float timeOfDay = 12.0f;
        float sunAngle = 0.0f;
        bool useRealWeather = false;
        int timeMultiplier = 1;

        // Physics
        QString trackConfig;
        float gripModifier = 1.0f;
        bool dynamicTrack = true;
        int trackConditions = 1; // 0=groove, 1=evolving, 2=reset

        // Rules
        bool isLocked = false;
        bool allowAutopilot = false;
        bool allowVirtualMirror = true;
        int maxBallast = 0;
        int dumpInterval = 0;

        // CSP extensions
        bool enableCsp = false;
        QString cspVersion;
        QMap<QString, QString> cspSettings;
    };

    struct EntryInfo {
        QString name;
        QString team;
        QString guid;
        QString carModel;
        QString skin;
        int spectatorMode = 0;
        int ballast = 0;
        int restrictor = 0;
    };

    // Config operations
    static ServerConfig loadConfig(const QString& serverPath);
    static bool saveConfig(const ServerConfig& config, const QString& serverPath);
    static bool loadFromIni(ServerConfig& config, const QString& iniPath);
    static bool saveToIni(const ServerConfig& config, const QString& iniPath);

    // Entry list operations
    static QVector<EntryInfo> loadEntryList(const QString& serverPath);
    static bool saveEntryList(const QVector<EntryInfo>& entries, const QString& serverPath);
    static bool addEntry(const EntryInfo& entry, const QString& serverPath);
    static bool removeEntry(const QString& guid, const QString& serverPath);
    static bool updateEntry(const EntryInfo& entry, const QString& serverPath);

    // CSP/Extra config
    static bool loadExtraConfig(ServerConfig& config, const QString& serverPath);
    static bool saveExtraConfig(const ServerConfig& config, const QString& serverPath);

    // Presets
    static ServerConfig getPresetSprint();
    static ServerConfig getPresetEndurance();
    static ServerConfig getPresetDrift();
    static ServerConfig getPresetCruise();

    // Validation
    static bool validateConfig(const ServerConfig& config, QString* error = nullptr);
    static bool validateEntryList(const QVector<EntryInfo>& entries, QString* error = nullptr);

    // Utility
    static QString getSessionTypeName(int type);
    static QString getWeatherName(const QString& weatherId);
    static QStringList getAvailableTracks(const QString& contentPath);
    static QStringList getAvailableCars(const QString& contentPath);

private:
    static bool parseEntryLine(const QString& line, EntryInfo& entry);
    static QString formatEntryLine(const EntryInfo& entry);
};


