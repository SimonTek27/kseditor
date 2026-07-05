#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantMap>
#include <QVariantList>
#include <QVector>
#include <QDateTime>
#include <QtMath>
#include <QPair>

namespace ks {

class TelemetryQmlBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool recording READ isRecording NOTIFY recordingChanged)
    Q_PROPERTY(int lapCount READ lapCount NOTIFY lapCountChanged)
    Q_PROPERTY(int currentLap READ currentLapNumber NOTIFY currentLapChanged)
    Q_PROPERTY(double bestLapTime READ bestLapTime NOTIFY bestLapChanged)
    Q_PROPERTY(int bestLapNumber READ bestLapNumber NOTIFY bestLapChanged)

public:
    static TelemetryQmlBridge* instance();

    bool isRecording() const;
    int lapCount() const;
    int currentLapNumber() const;
    double bestLapTime() const;
    int bestLapNumber() const;

    Q_INVOKABLE void startSession();
    Q_INVOKABLE void stopSession();
    Q_INVOKABLE void loadFile(const QString& path);
    Q_INVOKABLE void recordSample(float speed, float rpm, float throttle, float brake,
                                   float steering, float gear, float latG, float lonG);
    Q_INVOKABLE void markLapStart();
    Q_INVOKABLE void markSector(int sector);
    Q_INVOKABLE QVariantMap getLapData(int lapNumber) const;
    Q_INVOKABLE QVariantList getAllLaps() const;
    Q_INVOKABLE void compareLaps(int lapA, int lapB);
    Q_INVOKABLE void exportSession(const QString& path);
    Q_INVOKABLE void clearSession();

    // ── Sector analysis ──────────────────────────────────────────────────
    Q_INVOKABLE QVariantMap analyzeSector(int lapNumber, int sector);
    Q_INVOKABLE QVariantList analyzeAllSectors(int lapNumber);
    Q_INVOKABLE QVariantMap getSectorComparison(int sector, int lapA, int lapB);
    Q_INVOKABLE QVariantMap findOptimalSectorTimes();
    Q_INVOKABLE QVariantList getIdealLap() const;

    // ── Driver input analysis ────────────────────────────────────────────
    Q_INVOKABLE QVariantMap analyzeDriverInputs(int lapNumber);
    Q_INVOKABLE QVariantList getSpeedTrace(int lapNumber) const;
    Q_INVOKABLE QVariantList getThrottleTrace(int lapNumber) const;
    Q_INVOKABLE QVariantMap analyzeThrottleBrakePattern(int lapNumber);
    Q_INVOKABLE QVariantMap analyzeSteeringPattern(int lapNumber);
    Q_INVOKABLE QVariantMap analyzeGearUsage(int lapNumber);
    Q_INVOKABLE QVariantList getCornerAnalysis(int lapNumber);
    Q_INVOKABLE QVariantMap compareDrivingStyle(int lapA, int lapB);

signals:
    void statusMessage(const QString& msg);
    void errorMessage(const QString& msg);
    void dataReceived(const QVariantMap& data);
    void sessionStarted(const QString& name);
    void sessionStopped(const QString& name);
    void lapCompleted(int lapNumber, double lapTime);
    void sectorTime(int sector, double time);
    void bestLapChanged(int lapNumber, double lapTime);
    void comparisonComplete(const QStringList& differences);
    void recordingChanged();
    void lapCountChanged();
    void currentLapChanged();
    void sectorAnalysisComplete(int lapNumber, int sector, const QVariantMap& analysis);
    void driverAnalysisComplete(int lapNumber, const QVariantMap& analysis);

private:
    struct TelemetrySample {
        QDateTime timestamp;
        float speed = 0;
        float rpm = 0;
        float throttle = 0;
        float brake = 0;
        float steering = 0;
        int gear = 0;
        float lateralG = 0;
        float longitudinalG = 0;
    };

    struct LapData {
        int lapNumber = 0;
        double lapTime = 0;
        double sector1 = 0;
        double sector2 = 0;
        double sector3 = 0;
        float topSpeed = 0;
        float avgSpeed = 0;
        float maxGForce = 0;
        float avgThrottle = 0;
        float avgBrake = 0;
        float avgSteering = 0;
        float throttleTimePercent = 0;
        float brakeTimePercent = 0;
        int shiftCount = 0;
        bool valid = false;
        QVector<TelemetrySample> samples;
    };

    TelemetryQmlBridge(QObject* parent = nullptr);
    static TelemetryQmlBridge* s_instance;

    void finishCurrentLap();
    QVariantMap aggregateData() const;
    void saveSessionData();
    void saveSessionToFile(const QString& path);

    bool m_isRecording = false;
    QString m_sessionName;
    QString m_currentCar;
    QDateTime m_sessionStartTime;

    QVector<LapData> m_lapData;
    QVector<TelemetrySample> m_currentLapSamples;

    int m_currentLapNumber = 0;
    double m_bestLapTime = -1;
    int m_bestLapNumber = 0;
    QDateTime m_currentLapStartTime;
    float m_currentLapTopSpeed = 0;
    float m_currentLapSpeedSum = 0;
    float m_currentLapMaxG = 0;
    int m_currentLapSampleCount = 0;
    double m_currentSector1 = 0;
    double m_currentSector2 = 0;
};

}

