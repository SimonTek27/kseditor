#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>

namespace ks {

class SetupRecommender : public QObject
{
    Q_OBJECT

public:
    explicit SetupRecommender(QObject* parent = nullptr);
    ~SetupRecommender();

    struct TrackCharacteristics {
        float totalLengthMeters = 0.0f;
        int numCorners = 0;
        float avgCornerRadius = 0.0f;
        float minCornerRadius = 0.0f;
        float maxCornerRadius = 0.0f;
        int numHighSpeedCorners = 0;
        int numMidSpeedCorners = 0;
        int numLowSpeedCorners = 0;
        float topSpeedKmh = 0.0f;
        float avgStraightLength = 0.0f;
        float elevationChange = 0.0f;
        float trackWidth = 12.0f;
        enum SurfaceType { Tarmac, Asphalt, Concrete, Mixed } surface = Tarmac;
        enum LayoutType { Permanent, Temporary, Street, Drag } layout = Permanent;
    };

    struct CarProperties {
        float massKg = 1200.0f;
        float powerKw = 450.0f;
        float maxRpm = 8000;
        float wheelbase = 2.7f;
        float trackWidth = 1.6f;
        float weightDistribution = 0.5f;
        float downforceCoeff = 2.5f;
        float dragCoeff = 0.35f;
        float frontalArea = 2.0f;
        enum DriveType { RWD, FWD, AWD } driveType = RWD;
        enum EnginePosition { Front, Mid, Rear } enginePosition = Front;
    };

    struct SetupRecommendation {
        struct Suspension {
            float frontSpringRate = 100.0f;
            float rearSpringRate = 90.0f;
            float frontDampingCompression = 3.0f;
            float frontDampingRebound = 5.0f;
            float rearDampingCompression = 3.0f;
            float rearDampingRebound = 5.0f;
            float frontRideHeight = 0.08f;
            float rearRideHeight = 0.08f;
            float frontCamber = -2.5f;
            float rearCamber = -2.0f;
            float frontToe = 0.1f;
            float rearToe = 0.05f;
            float frontARB = 2.0f;
            float rearARB = 1.5f;
        } suspension;

        struct Brakes {
            float frontBias = 0.60f;
            float rearBias = 0.40f;
            float frontPressure = 150.0f;
            float rearPressure = 100.0f;
        } brakes;

        struct Aero {
            float frontWing = 0.0f;
            float rearWing = 0.0f;
            float frontSplitter = 0.0f;
            float rearDiffuser = 0.0f;
            int diffClutch = 50;
            int diffPower = 60;
            int diffCoast = 40;
        } aero;

        struct Transmission {
            int finalDrive = 3.8f;
            int gear1 = 3.5f;
            int gear2 = 2.5f;
            int gear3 = 1.8f;
            int gear4 = 1.4f;
            int gear5 = 1.1f;
            int gear6 = 0.9f;
        } transmission;

        struct Tyres {
            float frontPressure = 2.2f;
            float rearPressure = 2.1f;
            int frontCompound = 0;
            int rearCompound = 0;
        } tyres;

        float confidence = 0.0f;
        QString reasoning;
    };

    void setTrackCharacteristics(const TrackCharacteristics& track);
    TrackCharacteristics trackCharacteristics() const { return m_track; }

    void setCarProperties(const CarProperties& car);
    CarProperties carProperties() const { return m_car; }

    SetupRecommendation generateRecommendation();

    QJsonObject recommendationToJson(const SetupRecommendation& rec) const;
    SetupRecommendation jsonToRecommendation(const QJsonObject& obj) const;

    QStringList getSetupCategories() const;
    QString getSetupDescription(const SetupRecommendation& rec) const;

    enum TrackCategory {
        Track_Sprint,
        Track_Techical,
        Track_HighSpeed,
        Track_Mixed,
        Track_Street
    };

    TrackCategory classifyTrack() const;

signals:
    void recommendationGenerated(const SetupRecommendation& rec);
    void analysisProgress(float percent);

private:
    float calculateCornerG(float speed, float radius);
    float calculateDownforce(float speed, float aoa);
    float calculateBrakingDistance(float speed, float deceleration);
    float calculateCornerEntrySpeed(float radius, float g);

    void analyzeSuspension(SetupRecommendation& rec);
    void analyzeBrakes(SetupRecommendation& rec);
    void analyzeAero(SetupRecommendation& rec);
    void analyzeTransmission(SetupRecommendation& rec);
    void analyzeTyres(SetupRecommendation& rec);

    TrackCharacteristics m_track;
    CarProperties m_car;
};

class SetupOptimizer : public QObject
{
    Q_OBJECT
public:
    explicit SetupOptimizer(QObject* parent = nullptr);
    ~SetupOptimizer();

    void setRecommender(SetupRecommender* recommender);
    void setTargetLapTime(float seconds);
    void setWeatherCondition(int temperature, float humidity);

    void optimizeIteration();
    float calculateLapTime(const SetupRecommender::SetupRecommendation& setup);
    float evaluateSetup(const SetupRecommender::SetupRecommendation& setup);

    void saveToFile(const QString& path) const;
    void loadFromFile(const QString& path);

    int iterations() const { return m_iterations; }
    float bestScore() const { return m_bestScore; }

signals:
    void iterationComplete(int iteration, float score, float lapTime);
    void optimizationComplete(const SetupRecommender::SetupRecommendation& best);

private:
    SetupRecommender* m_recommender = nullptr;
    SetupRecommender::SetupRecommendation m_bestSetup;

    float m_targetLapTime = 0.0f;
    int m_temperature = 20;
    float m_humidity = 0.5f;

    int m_iterations = 0;
    float m_bestScore = 0.0f;
};

class TelemetryAnalyzer : public QObject
{
    Q_OBJECT
public:
    explicit TelemetryAnalyzer(QObject* parent = nullptr);
    ~TelemetryAnalyzer();

    struct TelemetryPoint {
        float time;
        float speed;
        float throttle;
        float brake;
        float steering;
        float rpm;
        float gear;
        float latG;
        float longG;
        float trackPos;
        float dRS;
    };

    struct LapAnalysis {
        float lapTime = 0.0f;
        QVector<float> sectorTimes;
        float topSpeed = 0.0f;
        float avgSpeed = 0.0f;
        int numCorners = 0;
        QVector<TelemetryPoint> cornerEntryPoints;
        QVector<TelemetryPoint> cornerExitPoints;
        float maxLateralG = 0.0f;
        float maxLongitudinalG = 0.0f;
    };

    void loadTelemetry(const QVector<TelemetryPoint>& points);
    void addLap(const QVector<TelemetryPoint>& points);

    LapAnalysis analyzeCurrentLap();
    QVector<LapAnalysis> analyzeAllLaps();

    LapAnalysis findFastestLap() const;
    LapAnalysis findConsistentLap() const;

    void generateSetupSuggestions(SetupRecommender::SetupRecommendation& rec);

    void exportToJson(const QString& path) const;
    void importFromJson(const QString& path);

signals:
    void analysisComplete(const LapAnalysis& analysis);

private:
    QVector<LapAnalysis> m_laps;
    QVector<TelemetryPoint> m_currentLap;
    int m_currentLapIndex = -1;
};

} // namespace ks