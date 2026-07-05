#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantMap>
#include <QMap>
#include <QVector>
#include "AITelemetryTrainer.h"
#include "AiBehaviorModel.h"
#include "MultiCarAI.h"

namespace ks {

struct AIAnalysisResult {
    QString trackName;
    int difficulty;
    int aggression;
    double cornerAggression;
    double straightSpeed;
};

struct AIDriverProfile {
    QString name;
    double skillLevel;
    double aggression;
    double precision;
    double consistency;
    double trackKnowledge;
    double brakingPointOffset;
    double turnInPointOffset;
    double apexSpeedModifier;
    double tractionOutModifier;
};

struct AILapTelemetry {
    double lapTime;
    QVector<double> brakingPoints;
    QVector<double> turnInPoints;
    QVector<double> apexSpeeds;
    QVector<double> throttlePoints;
};

struct AIOptimizationResult {
    QVector<double> optimalBrakingPoints;
    QVector<double> optimalTurnInPoints;
    QVector<double> optimalApexSpeeds;
    QVector<double> optimalThrottlePoints;
    double improvementPotential;
};

struct LearnedData {
    int totalLapsDriven = 0;
    double averageLapTime = 0.0;
    int optimizationCount = 0;
    QMap<QString, AIAnalysisResult> analysisHistory;
    QMap<QString, int> lapsCompleted;
    QMap<QString, QVector<AILapTelemetry>> telemetryData;
    QVector<int> difficultyAdjustments;
};

class AIEditorQmlBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(int difficulty READ difficulty WRITE setDifficulty NOTIFY difficultyChanged)
    Q_PROPERTY(int aggression READ aggression WRITE setAggression NOTIFY aggressionChanged)
    Q_PROPERTY(int precision READ precision WRITE setPrecision NOTIFY precisionChanged)
    Q_PROPERTY(int consistency READ consistency WRITE setConsistency NOTIFY consistencyChanged)
    Q_PROPERTY(int rubberBanding READ rubberBanding WRITE setRubberBanding NOTIFY rubberBandingChanged)
    Q_PROPERTY(int energyRecovery READ energyRecovery WRITE setEnergyRecovery NOTIFY energyRecoveryChanged)

public:
    static AIEditorQmlBridge* instance();

    int difficulty() const;
    int aggression() const;
    int precision() const;
    int consistency() const;
    int rubberBanding() const;
    int energyRecovery() const;

    Q_INVOKABLE void analyzeLine(const QString& track);
    Q_INVOKABLE void generateAILine(const QString& track);
    Q_INVOKABLE void optimizeLine(const QString& track);
    Q_INVOKABLE void setDifficulty(int level);
    Q_INVOKABLE void setAggression(int level);
    Q_INVOKABLE void setPrecision(int level);
    Q_INVOKABLE void setConsistency(int level);
    Q_INVOKABLE void setRubberBanding(int level);
    Q_INVOKABLE void setEnergyRecovery(int level);
    Q_INVOKABLE QStringList getPresets() const;
    Q_INVOKABLE void applyPreset(const QString& name);
    Q_INVOKABLE QStringList getAvailableTracks() const;

    // Advanced AI learning
    Q_INVOKABLE void recordTelemetry(const QString& track, double lapTime, const QVariantList& telemetry);
    Q_INVOKABLE double getTrackKnowledge(const QString& track) const;
    Q_INVOKABLE int getTotalLapsDriven() const { return m_learnedData.totalLapsDriven; }
    Q_INVOKABLE int getOptimizationCount() const { return m_learnedData.optimizationCount; }

    // File operations
    Q_INVOKABLE void openAILine(const QString& path);
    Q_INVOKABLE void saveAILine(const QString& path);
    Q_INVOKABLE void autoComputeBrakePoints();
    Q_INVOKABLE void updateWaypoint(int index, double x, double y, double z, double speed);
    QVariantList waypoints() const;
    Q_PROPERTY(QString currentFile READ currentFile NOTIFY currentFileChanged)
    Q_PROPERTY(int waypointCount READ waypointCount NOTIFY waypointCountChanged)
    Q_PROPERTY(QVariantList waypoints READ waypoints NOTIFY waypointsChanged)
    Q_PROPERTY(int selectedWaypointIndex READ selectedWaypointIndex WRITE setSelectedWaypointIndex NOTIFY selectedWaypointIndexChanged)

    // Multi-car AI
    Q_INVOKABLE void setMultiCarEnabled(bool enabled);
    Q_INVOKABLE void setOvertakingAggression(int level);
    Q_INVOKABLE void setDefensiveDriving(int level);
    Q_INVOKABLE void setupMultiCarRace(int numDrivers, int laps, double trackLength);
    Q_INVOKABLE void startMultiCarRace();
    Q_INVOKABLE void stopMultiCarRace();
    Q_INVOKABLE void resetMultiCarRace();
    Q_INVOKABLE QVariantList getMultiCarLeaderboard();
    Q_INVOKABLE QVariantMap getMultiCarRaceStats();
    Q_INVOKABLE QVariantList getMultiCarRaceEvents();
    Q_INVOKABLE bool isMultiCarRaceComplete();
    Q_INVOKABLE double getMultiCarEstimatedRaceTime();

    // Telemetry-based AI learning
    Q_INVOKABLE void trainFromTelemetry(const QString& track, const QVariantList& samples);
    Q_INVOKABLE void analyzeTrackData(const QString& track);
    Q_INVOKABLE QVariantMap getOptimizedDriverProfile(const QString& track) const;
    Q_INVOKABLE QVariantMap getTrackStats(const QString& track) const;
    Q_INVOKABLE QStringList getTrainedTracks() const;
    Q_INVOKABLE int getTotalLapsAnalyzed() const;
    Q_INVOKABLE void clearTrainingData();
    Q_INVOKABLE double getTrackKnowledgeLevel(const QString& track) const;
    Q_INVOKABLE QVariantList getCornerAnalysis(const QString& track) const;
    Q_INVOKABLE void applyTelemetryProfile(const QString& track);

signals:
    void statusMessage(const QString& msg);
    void errorMessage(const QString& msg);
    void difficultyChanged(int level);
    void aggressionChanged(int level);
    void precisionChanged(int level);
    void consistencyChanged(int level);
    void rubberBandingChanged(int level);
    void energyRecoveryChanged(int level);
    void analysisComplete(const QString& track, int difficulty);
    void aiLineGenerated(const QString& track, int difficulty);
    void optimizationComplete(const QString& track, int difficulty);
    void presetApplied(const QString& name);
    void learningProgress(double progress);
    void trainingComplete(const QString& track, double improvement);
    void trainingDataCleared();
    void profileApplied(const QString& track, const QString& profileName);
    void currentFileChanged();
    void waypointCountChanged();
    void waypointsChanged();
    void selectedWaypointIndexChanged();
    void raceFinished();

private:
    AIEditorQmlBridge(QObject* parent = nullptr);
    static AIEditorQmlBridge* s_instance;

    void loadDefaultPresets();
    void loadLearnedData();
    void saveLearnedData();

    QString currentFile() const { return m_currentFilePath; }
    int waypointCount() const { return m_waypointCount; }
    int selectedWaypointIndex() const { return m_selectedWaypointIndex; }
    void setSelectedWaypointIndex(int idx);

    int m_currentDifficulty = 50;
    int m_aggression = 50;
    int m_precision = 50;
    int m_consistency = 50;
    int m_rubberBanding = 50;
    int m_energyRecovery = 50;
    int m_overtakingAggression = 50;
    int m_defensiveDriving = 50;
    bool m_multiCarEnabled = true;
    bool m_raceFinished = false;
    QString m_currentTrack;
    QMap<QString, QVariantMap> m_presets;
    QMap<QString, AIDriverProfile> m_generatedProfiles;
    QMap<QString, AIOptimizationResult> m_optimizationResults;
    LearnedData m_learnedData;
    AITelemetryTrainer m_telemetryTrainer;
    MultiCarAI m_multiCarAI;
    QString m_currentFilePath;
    int m_waypointCount = 0;
    int m_selectedWaypointIndex = 0;
    QVariantList m_waypoints;
};

}

