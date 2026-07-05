#pragma once

#include <QString>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>

/**
 * @brief Telemetry Analyzer for Assetto Corsa
 *
 * Analyzes telemetry data from AC sessions for performance improvement.
 * Based on community tools:
 * - ac_telemetry (github.com/Yaandle/ac_telemetry)
 * - tobi/ac-tracer - CSP Lua telemetry app
 * - ldparser (github.com/gotzl/ldparser) - MoTeC ld file parser
 * - Telemetrick (ACFever) - Telemetry recording app
 *
 * Features:
 * - Lap time analysis and comparison
 * - Sector time breakdown
 * - Speed/throttle/brake traces
 * - Tire temperature and pressure analysis
 * - Corner analysis
 * - Improvement suggestions
 */
class TelemetryAnalyzer {
public:
    struct TelemetrySample {
        float timestamp;
        float speed;           // km/h
        float rpm;
        int gear;
        float throttle;        // 0-1
        float brake;           // 0-1
        float steering;        // -1 to 1
        float clutch;          // 0-1

        // Position
        float pos[3];
        float rot[4];          // quaternion

        // Tire data
        float tireTemp[4];     // FL, FR, RL, RR
        float tirePressure[4];
        float tireWear[4];
        float tireSlip[4];

        // Vehicle dynamics
        float gForce[3];       // lateral, vertical, longitudinal
        float yawRate;
        float slideAngle;

        // Session
        float lapTime;
        float lastLapTime;
        float bestLapTime;
        int lapCount;
        float fuel;
        float fuelConsumption;
    };

    struct LapData {
        int lapNumber;
        float lapTime;
        float sectorTimes[3];
        float maxSpeed;
        float avgSpeed;
        float maxRPM;
        float avgThrottle;
        float avgBrake;
        float tireWear[4];
        float fuelUsed;
        bool isValid;
    };

    struct SectorAnalysis {
        int sector;
        float entrySpeed;
        float midCornerSpeed;
        float exitSpeed;
        float minSpeed;
        float maxSpeed;
        float avgSpeed;
        float brakingDistance;
        float throttleTime;
        float coastingTime;
        float brakingTime;
        float steeringAngle;
        float gForceLateral;
    };

    struct CornerAnalysis {
        int cornerNumber;
        QString cornerName;
        float entrySpeed;
        float apexSpeed;
        float exitSpeed;
        float minSpeed;
        float brakingPoint;
        float turnInPoint;
        float apexPoint;
        float exitPoint;
        float steeringAngle;
        float throttleApplication;
        float brakeApplication;
        float gForceLateral;
        float timeLost;
    };

    struct TelemetryReport {
        QString trackName;
        QString carName;
        QString driverName;
        float sessionTime;
        int totalLaps;
        float bestLapTime;
        float avgLapTime;
        float consistency;
        QVector<LapData> laps;
        QVector<SectorAnalysis> sectors;
        QVector<CornerAnalysis> corners;
        QJsonObject improvementSuggestions;
    };

    // Data loading
    static bool loadFromCsv(const QString& csvPath, QVector<TelemetrySample>& samples);
    static bool loadFromJson(const QString& jsonPath, QVector<TelemetrySample>& samples);
    static bool loadFromAcSharedMemory(const QString& shmPath, QVector<TelemetrySample>& samples);

    // Lap analysis
    static QVector<LapData> analyzeLaps(const QVector<TelemetrySample>& samples);
    static LapData analyzeLap(const QVector<TelemetrySample>& lapSamples, int lapNumber);
    static QVector<float> calculateLapTimes(const QVector<TelemetrySample>& samples);

    // Sector analysis
    static QVector<SectorAnalysis> analyzeSectors(const QVector<TelemetrySample>& lapSamples,
                                                   float sectorSplits[3]);

    // Corner analysis
    static QVector<CornerAnalysis> analyzeCorners(const QVector<TelemetrySample>& lapSamples,
                                                   const QVector<float>& racingLine);

    // Speed analysis
    static float getMaxSpeed(const QVector<TelemetrySample>& samples);
    static float getAvgSpeed(const QVector<TelemetrySample>& samples);
    static QVector<float> getSpeedTrace(const QVector<TelemetrySample>& samples);
    static QVector<float> getSpeedDifferential(const QVector<TelemetrySample>& lap1,
                                                const QVector<TelemetrySample>& lap2);

    // Input analysis
    static QVector<float> getThrottleTrace(const QVector<TelemetrySample>& samples);
    static QVector<float> getBrakeTrace(const QVector<TelemetrySample>& samples);
    static QVector<float> getSteeringTrace(const QVector<TelemetrySample>& samples);
    static float getThrottleSmoothness(const QVector<TelemetrySample>& samples);
    static float getBrakeSmoothness(const QVector<TelemetrySample>& samples);

    // Tire analysis
    static QVector<float> getTireTempTrace(const QVector<TelemetrySample>& samples, int wheel);
    static QVector<float> getTirePressureTrace(const QVector<TelemetrySample>& samples, int wheel);
    static float getTireWearRate(const QVector<TelemetrySample>& samples, int wheel);
    static QVector<float> getTireSlipTrace(const QVector<TelemetrySample>& samples, int wheel);

    // G-force analysis
    static float getMaxLateralG(const QVector<TelemetrySample>& samples);
    static float getMaxLongitudinalG(const QVector<TelemetrySample>& samples);
    static QVector<float> getLateralGTrace(const QVector<TelemetrySample>& samples);
    static QVector<float> getLongitudinalGTrace(const QVector<TelemetrySample>& samples);

    // Fuel analysis
    static float getFuelConsumptionRate(const QVector<TelemetrySample>& samples);
    static float estimateFuelForLaps(const QVector<TelemetrySample>& samples, int laps);
    static float estimateFuelForDistance(const QVector<TelemetrySample>& samples, float distance);

    // Comparison
    static QJsonObject compareLaps(const LapData& lap1, const LapData& lap2);
    static QJsonObject compareDrivers(const TelemetryReport& report1, const TelemetryReport& report2);

    // Report generation
    static TelemetryReport generateReport(const QVector<TelemetrySample>& samples,
                                           const QString& trackName = QString(),
                                           const QString& carName = QString());
    static QJsonObject generateJsonReport(const TelemetryReport& report);
    static bool saveReport(const TelemetryReport& report, const QString& filePath);

    // Improvement suggestions
    static QJsonObject analyzeBrakingPoints(const QVector<TelemetrySample>& samples);
    static QJsonObject analyzeThrottleApplication(const QVector<TelemetrySample>& samples);
    static QJsonObject analyzeRacingLine(const QVector<TelemetrySample>& samples);
    static QJsonObject analyzeGearSelection(const QVector<TelemetrySample>& samples);
    static QJsonObject analyzeTireManagement(const QVector<TelemetrySample>& samples);

    // Consistency & performance metrics
    static float calculateConsistency(const QVector<LapData>& laps);
    static float calculateLapTimeVariance(const QVector<LapData>& laps);
    static float calculateDrivingScore(const QVector<TelemetrySample>& samples);

    // Braking point analysis
    struct BrakingEvent {
        float timestamp;
        float entrySpeed;
        float exitSpeed;
        float minSpeed;
        float brakingDistance;
        float brakePressure;
        float steeringAngle;
        int cornerNumber;
    };
    static QVector<BrakingEvent> detectBrakingPoints(const QVector<TelemetrySample>& samples);
    static float calculateBrakingEfficiency(const BrakingEvent& event);
    static float calculateBrakeFadeRisk(const QVector<BrakingEvent>& events);

    // Apex analysis
    struct ApexData {
        float speed;
        float steeringAngle;
        float gForceLateral;
        float throttleAtApex;
        float brakeAtApex;
        float sectorTime;
    };
    static ApexData identifyApex(const QVector<TelemetrySample>& cornerSamples);
    static float calculateApexEfficiency(const ApexData& apex, float cornerRadius);

    // Utility
    static QVector<TelemetrySample> interpolateSamples(const QVector<TelemetrySample>& samples,
                                                        float targetHz = 100.0f);
    static QVector<TelemetrySample> filterSamples(const QVector<TelemetrySample>& samples,
                                                   float startTime, float endTime);
    static float calculateCorrelation(const QVector<float>& x, const QVector<float>& y);

private:
    static int findLapStart(const QVector<TelemetrySample>& samples, int startIndex);
    static int findLapEnd(const QVector<TelemetrySample>& samples, int startIndex);
    static float calculateCornerSpeed(const QVector<TelemetrySample>& cornerSamples);
    static float calculateBrakingDistance(const QVector<TelemetrySample>& brakeSamples);
};
