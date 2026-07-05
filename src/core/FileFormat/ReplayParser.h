#pragma once

#include <QString>
#include <QByteArray>
#include <QVector>
#include <QFile>
#include <QJsonObject>
#include <QJsonArray>

/**
 * @brief Assetto Corsa Replay File Parser
 *
 * Parses .acreplay files from Assetto Corsa for analysis and visualization.
 * Based on reverse engineering from:
 * - acreplay-parser (github.com/abchouhan/acreplay-parser)
 *
 * The replay format contains:
 * - Header with session information
 * - Per-frame car positions and rotations
 * - Physics data (speed, RPM, throttle, brake, gear)
 * - Tire data (temperature, wear, pressure)
 */
class ReplayParser {
public:
    struct ReplayFrame {
        float timestamp;
        int carId;
        float position[3];
        float rotation[4]; // quaternion
        float velocity[3];
        float speed;
        float rpm;
        float throttle;
        float brake;
        float steering;
        int gear;
        float tyreTemps[4]; // FL, FR, RL, RR
        float tyreWear[4];
        float tyrePressure[4];
        float fuel;
        float damage;
    };

    struct ReplayCar {
        int id;
        QString name;
        QString team;
        QString guid;
        QString carModel;
        QString skin;
        int gridPosition;
        bool isPlayer;
    };

    struct ReplaySession {
        QString trackName;
        QString trackConfig;
        int sessionType; // 0=practice, 1=qualifying, 2=race
        float sessionLength; // in seconds
        int lapsCount;
        float ambientTemp;
        float roadTemp;
        QString weather;
    };

    struct ReplayData {
        QString filePath;
        ReplaySession session;
        QVector<ReplayCar> cars;
        QVector<ReplayFrame> frames;
        float duration = 0;
        bool isValid = false;
    };

    // Main operations
    static ReplayData parse(const QString& replayPath, QString* error = nullptr);
    static bool exportToCSV(const ReplayData& data, const QString& csvPath, int carId = -1);
    static bool exportToJSON(const ReplayData& data, const QString& jsonPath);

    // Analysis
    static float calculateMaxSpeed(const ReplayData& data, int carId = -1);
    static float calculateMaxRPM(const ReplayData& data, int carId = -1);
    static QVector<float> calculateLapTimes(const ReplayData& data, int carId);
    static float calculateAverageSpeed(const ReplayData& data, int carId = -1);
    static QVector<float> calculateBrakePoints(const ReplayData& data, int carId);

    // Validation
    static bool isValidReplay(const QString& filePath);
    static QString getLastError() { return m_lastError; }

private:
    static bool parseHeader(QDataStream& stream, ReplayData& data);
    static bool parseCars(QDataStream& stream, ReplayData& data);
    static bool parseFrames(QDataStream& stream, ReplayData& data);

    static QString m_lastError;
};

/**
 * @brief Replay Analyzer - High-level analysis interface
 */
class ReplayAnalyzer {
public:
    explicit ReplayAnalyzer(const QString& replayPath);

    bool load();
    bool isValid() const { return m_data.isValid; }

    // Analysis results
    float getMaxSpeed() const;
    float getMaxRPM() const;
    float getAverageSpeed() const;
    QVector<float> getLapTimes() const;
    QVector<float> getSpeedTrace() const;
    QVector<float> getThrottleTrace() const;
    QVector<float> getBrakeTrace() const;

    // Export
    bool exportCSV(const QString& path, int carId = -1);
    bool exportJSON(const QString& path);

    // Data access
    const ReplayParser::ReplayData& getData() const { return m_data; }

private:
    ReplayParser::ReplayData m_data;
};
