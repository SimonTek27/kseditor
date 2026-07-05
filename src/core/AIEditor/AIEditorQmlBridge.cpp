#include "AIEditorQmlBridge.h"
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QStandardPaths>
#include <cmath>

namespace ks {

AIEditorQmlBridge* AIEditorQmlBridge::s_instance = nullptr;

AIEditorQmlBridge* AIEditorQmlBridge::instance() {
    if (!s_instance) {
        s_instance = new AIEditorQmlBridge();
    }
    return s_instance;
}

AIEditorQmlBridge::AIEditorQmlBridge(QObject* parent)
    : QObject(parent)
{
    loadDefaultPresets();
    loadLearnedData();
}

void AIEditorQmlBridge::analyzeLine(const QString& track) {
    if (track.isEmpty()) {
        emit errorMessage("No track specified for AI line analysis");
        return;
    }

    // Analyze track characteristics for AI line optimization
    emit statusMessage("Analyzing AI line for: " + track);

    AIAnalysisResult result;
    result.trackName = track;
    result.difficulty = m_currentDifficulty;
    result.aggression = m_aggression;

    // Calculate corner aggression based on track type
    if (track.contains("nurburgring") || track.contains("ring")) {
        result.cornerAggression = 0.6;
        result.straightSpeed = 0.8;
    } else if (track.contains("monza")) {
        result.cornerAggression = 0.4;
        result.straightSpeed = 0.95;
    } else if (track.contains("spa")) {
        result.cornerAggression = 0.7;
        result.straightSpeed = 0.85;
    } else {
        result.cornerAggression = 0.5 + (m_aggression - 50) / 100.0;
        result.straightSpeed = 0.7 + (m_precision - 30) / 100.0;
    }

    // Learning: store analysis for future improvement
    m_learnedData.analysisHistory[track] = result;
    saveLearnedData();

    emit analysisComplete(track, m_currentDifficulty);
}

void AIEditorQmlBridge::generateAILine(const QString& track) {
    if (track.isEmpty()) {
        emit errorMessage("No track specified for AI line generation");
        return;
    }

    emit statusMessage("Generating AI line for: " + track);
    m_currentTrack = track;

    // Generate AI line based on driver profile
    double skillFactor = m_currentDifficulty / 100.0;
    double precisionFactor = m_precision / 100.0;
    double consistencyVar = (100 - m_consistency) / 100.0;

    // Build AI driver profile
    AIDriverProfile profile;
    profile.name = "AI Driver";
    profile.skillLevel = skillFactor;
    profile.aggression = m_aggression / 100.0;
    profile.precision = precisionFactor;
    profile.consistency = m_consistency / 100.0;
    profile.trackKnowledge = 0.5 + (m_learnedData.lapsCompleted[track] * 0.01);

    // Calculate braking points, turn-in points, apex speeds
    profile.brakingPointOffset = (1.0 - precisionFactor) * 20.0; // meters
    profile.turnInPointOffset = (1.0 - precisionFactor) * 10.0;
    profile.apexSpeedModifier = 0.8 + (skillFactor * 0.2);
    profile.tractionOutModifier = 0.7 + (precisionFactor * 0.3);

    // Store generated profile
    m_generatedProfiles[track] = profile;

    emit aiLineGenerated(track, m_currentDifficulty);
}

void AIEditorQmlBridge::optimizeLine(const QString& track) {
    if (track.isEmpty()) {
        emit errorMessage("No track specified for AI line optimization");
        return;
    }

    emit statusMessage("Optimizing AI line for: " + track);

    // Telemetry-based AI improvement
    if (m_learnedData.telemetryData.contains(track)) {
        const auto& telemetry = m_learnedData.telemetryData[track];
        int totalLaps = telemetry.size();

        // Find optimal line across all recorded laps
        double bestTime = 1e9;
        int bestLapIndex = -1;
        for (int i = 0; i < telemetry.size(); ++i) {
            if (telemetry[i].lapTime > 0 && telemetry[i].lapTime < bestTime) {
                bestTime = telemetry[i].lapTime;
                bestLapIndex = i;
            }
        }

        if (bestLapIndex >= 0) {
            const auto& bestLap = telemetry[bestLapIndex];

            // Analyze optimal trajectory
            AIOptimizationResult opt;
            opt.optimalBrakingPoints = bestLap.brakingPoints;
            opt.optimalTurnInPoints = bestLap.turnInPoints;
            opt.optimalApexSpeeds = bestLap.apexSpeeds;
            opt.optimalThrottlePoints = bestLap.throttlePoints;
            double trackAvg = 0.0;
            for (const auto& t : telemetry) trackAvg += t.lapTime;
            trackAvg /= qMax(1, totalLaps);
            opt.improvementPotential = (trackAvg - bestTime) / qMax(bestTime, 0.001);

            m_optimizationResults[track] = opt;
            m_learnedData.optimizationCount++;
            saveLearnedData();

            emit statusMessage(QString("Optimized from %1 laps (best: %2s, avg: %3s)")
                .arg(totalLaps)
                .arg(bestTime, 0, 'f', 3)
                .arg(trackAvg, 0, 'f', 3));
        }
    } else {
        emit statusMessage("No telemetry data available for optimization");
    }

    emit optimizationComplete(track, m_currentDifficulty);
}

void AIEditorQmlBridge::setDifficulty(int level) {
    if (level < 0 || level > 100) return;
    m_currentDifficulty = level;
    m_learnedData.difficultyAdjustments.append(level);
    emit difficultyChanged(level);
    emit statusMessage(QString("AI difficulty set to %1%").arg(level));
}

void AIEditorQmlBridge::setAggression(int level) {
    m_aggression = qBound(0, level, 100);
    emit aggressionChanged(m_aggression);
}

void AIEditorQmlBridge::setPrecision(int level) {
    m_precision = qBound(0, level, 100);
    emit precisionChanged(m_precision);
}

void AIEditorQmlBridge::setConsistency(int level) {
    m_consistency = qBound(0, level, 100);
    emit consistencyChanged(m_consistency);
}

void AIEditorQmlBridge::setRubberBanding(int level) {
    m_rubberBanding = qBound(0, level, 100);
    emit rubberBandingChanged(m_rubberBanding);
}

void AIEditorQmlBridge::setEnergyRecovery(int level) {
    m_energyRecovery = qBound(0, level, 100);
    emit energyRecoveryChanged(m_energyRecovery);
}

int AIEditorQmlBridge::difficulty() const { return m_currentDifficulty; }
int AIEditorQmlBridge::aggression() const { return m_aggression; }
int AIEditorQmlBridge::precision() const { return m_precision; }
int AIEditorQmlBridge::consistency() const { return m_consistency; }
int AIEditorQmlBridge::rubberBanding() const { return m_rubberBanding; }
int AIEditorQmlBridge::energyRecovery() const { return m_energyRecovery; }

QStringList AIEditorQmlBridge::getPresets() const {
    return m_presets.keys();
}

void AIEditorQmlBridge::applyPreset(const QString& name) {
    if (!m_presets.contains(name)) {
        emit errorMessage("Unknown preset: " + name);
        return;
    }

    QVariantMap preset = m_presets[name];
    setDifficulty(preset["difficulty"].toInt());
    setAggression(preset["aggression"].toInt());
    setPrecision(preset["precision"].toInt());
    setConsistency(preset["consistency"].toInt());
    setRubberBanding(preset["rubberBanding"].toInt());
    setEnergyRecovery(preset["energyRecovery"].toInt());
    emit presetApplied(name);
    emit statusMessage("Applied AI preset: " + name);
}

QStringList AIEditorQmlBridge::getAvailableTracks() const {
    // Include tracks from learning data
    QStringList tracks = {"ks_nurburgring", "ks_monza", "ks_silverstone", "ks_spa",
                          "ks_suzuka", "ks_barcelona", "ks_imola", "ks_mugello"};
    for (auto it = m_learnedData.analysisHistory.begin(); it != m_learnedData.analysisHistory.end(); ++it) {
        if (!tracks.contains(it.key())) {
            tracks.append(it.key());
        }
    }
    return tracks;
}

void AIEditorQmlBridge::loadDefaultPresets() {
    QVariantMap rookie;
    rookie["difficulty"] = 30;
    rookie["aggression"] = 20;
    rookie["precision"] = 30;
    rookie["consistency"] = 60;
    rookie["rubberBanding"] = 80;
    rookie["energyRecovery"] = 70;
    m_presets["Rookie"] = rookie;

    QVariantMap amateur;
    amateur["difficulty"] = 50;
    amateur["aggression"] = 40;
    amateur["precision"] = 50;
    amateur["consistency"] = 50;
    amateur["rubberBanding"] = 50;
    amateur["energyRecovery"] = 50;
    m_presets["Amateur"] = amateur;

    QVariantMap professional;
    professional["difficulty"] = 80;
    professional["aggression"] = 60;
    professional["precision"] = 80;
    professional["consistency"] = 70;
    professional["rubberBanding"] = 30;
    professional["energyRecovery"] = 30;
    m_presets["Professional"] = professional;

    QVariantMap alien;
    alien["difficulty"] = 98;
    alien["aggression"] = 70;
    alien["precision"] = 95;
    alien["consistency"] = 90;
    alien["rubberBanding"] = 10;
    alien["energyRecovery"] = 15;
    m_presets["Alien"] = alien;

    QVariantMap smooth;
    smooth["difficulty"] = 70;
    smooth["aggression"] = 25;
    smooth["precision"] = 75;
    smooth["consistency"] = 90;
    smooth["rubberBanding"] = 40;
    smooth["energyRecovery"] = 60;
    m_presets["Smooth"] = smooth;
}

void AIEditorQmlBridge::loadLearnedData() {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/ai_learning.json";
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) return;

    QJsonObject root = doc.object();
    m_learnedData.totalLapsDriven = root["totalLapsDriven"].toInt();
    m_learnedData.averageLapTime = root["averageLapTime"].toDouble();
    m_learnedData.optimizationCount = root["optimizationCount"].toInt();

    QJsonObject history = root["analysisHistory"].toObject();
    for (auto it = history.begin(); it != history.end(); ++it) {
        QJsonObject h = it.value().toObject();
        AIAnalysisResult result;
        result.trackName = it.key();
        result.difficulty = h["difficulty"].toInt();
        result.aggression = h["aggression"].toInt();
        result.cornerAggression = h["cornerAggression"].toDouble();
        result.straightSpeed = h["straightSpeed"].toDouble();
        m_learnedData.analysisHistory[it.key()] = result;
    }

    QJsonObject laps = root["lapsCompleted"].toObject();
    for (auto it = laps.begin(); it != laps.end(); ++it) {
        m_learnedData.lapsCompleted[it.key()] = it.value().toInt();
    }

    QJsonArray adjustments = root["difficultyAdjustments"].toArray();
    for (const auto& v : adjustments) {
        m_learnedData.difficultyAdjustments.append(v.toInt());
    }
}

void AIEditorQmlBridge::saveLearnedData() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dir);

    QString path = dir + "/ai_learning.json";
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return;

    QJsonObject root;
    root["totalLapsDriven"] = m_learnedData.totalLapsDriven;
    root["averageLapTime"] = m_learnedData.averageLapTime;
    root["optimizationCount"] = m_learnedData.optimizationCount;

    QJsonObject history;
    for (auto it = m_learnedData.analysisHistory.begin(); it != m_learnedData.analysisHistory.end(); ++it) {
        QJsonObject h;
        h["difficulty"] = it.value().difficulty;
        h["aggression"] = it.value().aggression;
        h["cornerAggression"] = it.value().cornerAggression;
        h["straightSpeed"] = it.value().straightSpeed;
        history[it.key()] = h;
    }
    root["analysisHistory"] = history;

    QJsonObject laps;
    for (auto it = m_learnedData.lapsCompleted.begin(); it != m_learnedData.lapsCompleted.end(); ++it) {
        laps[it.key()] = it.value();
    }
    root["lapsCompleted"] = laps;

    QJsonArray adjustments;
    for (int v : m_learnedData.difficultyAdjustments) {
        adjustments.append(v);
    }
    root["difficultyAdjustments"] = adjustments;

    file.write(QJsonDocument(root).toJson());
    file.close();
}

void AIEditorQmlBridge::recordTelemetry(const QString& track, double lapTime, const QVariantList& telemetry)
{
    AILapTelemetry t;
    t.lapTime = lapTime;

    for (const QVariant& sample : telemetry) {
        QVariantList s = sample.toList();
        if (s.size() < 5) continue;
        t.brakingPoints.append(s[0].toDouble());
        t.turnInPoints.append(s[1].toDouble());
        t.apexSpeeds.append(s[2].toDouble());
        t.throttlePoints.append(s[3].toDouble());
    }

    m_learnedData.telemetryData[track].append(t);

    double total = m_learnedData.averageLapTime * m_learnedData.totalLapsDriven;
    m_learnedData.totalLapsDriven++;
    m_learnedData.averageLapTime = (total + lapTime) / m_learnedData.totalLapsDriven;
    m_learnedData.lapsCompleted[track] = m_learnedData.lapsCompleted.value(track, 0) + 1;

    m_learnedData.optimizationCount++;

    // Feedback loop: after enough laps, auto-apply learned adjustments
    int lapsForTrack = m_learnedData.lapsCompleted[track];
    if (lapsForTrack >= 5 && lapsForTrack % 5 == 0) {
        // Convert AILapTelemetry to TelemetrySample for the trainer
        QVector<TelemetrySample> samples;
        for (const auto& lap : m_learnedData.telemetryData[track]) {
            // Simple conversion - use the lap data to create a TelemetrySample
            TelemetrySample sample;
            sample.lapTime = lap.lapTime;
            samples.append(sample);
        }
        m_telemetryTrainer.ingestTelemetry(track, samples);
        m_telemetryTrainer.analyzeTrack(track);
        auto profile = m_telemetryTrainer.generateOptimizedProfile(track);

        int newDifficulty = qBound(0, static_cast<int>(profile.skill * 100), 100);
        int newAggression = qBound(0, static_cast<int>(profile.aggression * 100), 100);
        int newConsistency = qBound(0, static_cast<int>(profile.consistency * 100), 100);

        m_currentDifficulty = newDifficulty;
        m_aggression = newAggression;
        m_consistency = newConsistency;

        emit difficultyChanged(newDifficulty);
        emit aggressionChanged(newAggression);
        emit consistencyChanged(newConsistency);
        emit learningProgress(m_telemetryTrainer.getTrackKnowledge(track));
        emit statusMessage(QString("Feedback loop: updated profile from %1 laps on %2 (skill=%3%)")
            .arg(lapsForTrack).arg(track).arg(newDifficulty));
    }

    saveLearnedData();
    emit statusMessage(QString("Recorded lap %1 for %2: %3s").arg(m_learnedData.lapsCompleted[track]).arg(track).arg(lapTime, 0, 'f', 3));
}

double AIEditorQmlBridge::getTrackKnowledge(const QString& track) const
{
    if (m_learnedData.telemetryData.contains(track))
        return qMin(1.0, m_learnedData.telemetryData[track].size() / 50.0);
    if (m_learnedData.lapsCompleted.contains(track))
        return qMin(1.0, m_learnedData.lapsCompleted[track] * 0.01);
    return 0.0;
}

void AIEditorQmlBridge::setMultiCarEnabled(bool enabled)
{
    m_multiCarEnabled = enabled;
}

void AIEditorQmlBridge::setOvertakingAggression(int level)
{
    m_overtakingAggression = qBound(0, level, 100);
}

void AIEditorQmlBridge::setDefensiveDriving(int level)
{
    m_defensiveDriving = qBound(0, level, 100);
}

QVariantList AIEditorQmlBridge::waypoints() const
{
    return m_waypoints;
}

void AIEditorQmlBridge::openAILine(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit errorMessage("Cannot open: " + path);
        return;
    }

    m_waypoints.clear();
    m_waypointCount = 0;
    QTextStream in(&file);
    bool inData = false;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.startsWith(';') || line.startsWith('#')) continue;
        if (line.startsWith('[') && line.endsWith(']')) {
            inData = (line.toUpper() == "[DATA]");
            continue;
        }
        if (!inData) continue;

        QStringList parts = line.split(',');
        if (parts.size() >= 4) {
            QVariantMap wp;
            wp["x"] = parts[0].toDouble();
            wp["y"] = parts[1].toDouble();
            wp["z"] = parts[2].toDouble();
            wp["speed"] = parts[3].toDouble();
            m_waypoints.append(wp);
        }
    }
    file.close();

    m_waypointCount = m_waypoints.size();
    m_selectedWaypointIndex = 0;
    emit selectedWaypointIndexChanged();
    m_currentFilePath = path;
    emit waypointsChanged();
    emit waypointCountChanged();
    emit currentFileChanged();
    emit statusMessage("Opened: " + path + " (" + QString::number(m_waypointCount) + " waypoints)");
}

void AIEditorQmlBridge::saveAILine(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit errorMessage("Cannot save: " + path);
        return;
    }
    QTextStream out(&file);
    out << "; ksEditor AI Line\n";
    out << "[HEADER]\n";
    out << "VERSION=1\n";
    out << "POINTS=" << m_waypoints.size() << "\n\n";
    out << "[DATA]\n";
    for (const auto& wp : m_waypoints) {
        QVariantMap m = wp.toMap();
        out << m.value("x").toDouble() << ","
            << m.value("y").toDouble() << ","
            << m.value("z").toDouble() << ","
            << m.value("speed").toDouble() << "\n";
    }
    file.close();
    m_currentFilePath = path;
    emit currentFileChanged();
    emit statusMessage("Saved: " + path);
}

void AIEditorQmlBridge::autoComputeBrakePoints()
{
    if (m_waypoints.isEmpty()) {
        emit statusMessage("No waypoints loaded. Load an AI spline first.");
        return;
    }

    double skillFactor = m_currentDifficulty / 100.0;
    double precisionFactor = m_precision / 100.0;
    double brakingOffset = (1.0 - precisionFactor) * 30.0;

    int brakePointsComputed = 0;
    for (int i = 1; i < m_waypoints.size() - 1; ++i) {
        QVariantMap prev = m_waypoints[i - 1].toMap();
        QVariantMap curr = m_waypoints[i].toMap();
        QVariantMap next = m_waypoints[i + 1].toMap();

        double dx1 = curr["x"].toDouble() - prev["x"].toDouble();
        double dz1 = curr["z"].toDouble() - prev["z"].toDouble();
        double dx2 = next["x"].toDouble() - curr["x"].toDouble();
        double dz2 = next["z"].toDouble() - curr["z"].toDouble();

        double cross = dx1 * dz2 - dz1 * dx2;
        double len1 = std::sqrt(dx1 * dx1 + dz1 * dz1);
        double len2 = std::sqrt(dx2 * dx2 + dz2 * dz2);
        double curvature = (len1 > 0.001 && len2 > 0.001) ? std::abs(cross) / (len1 * len2) : 0.0;

        if (curvature > 0.15) {
            double currentSpeed = curr["speed"].toDouble();
            double brakeSpeed = currentSpeed * (0.5 + 0.5 * (1.0 - curvature));
            double brakeDist = brakingOffset * (curvature / 0.5);

            curr["speed"] = brakeSpeed;
            curr["brakePoint"] = true;
            curr["brakeDistance"] = brakeDist;
            m_waypoints[i] = curr;
            brakePointsComputed++;
        }
    }

    emit waypointsChanged();
    emit statusMessage(QString("Auto-computed %1 brake points (skill: %2%, offset: %3m)")
        .arg(brakePointsComputed)
        .arg(static_cast<int>(skillFactor * 100))
        .arg(brakingOffset, 0, 'f', 1));
    emit analysisComplete("brakePoints", m_currentDifficulty);
}

void AIEditorQmlBridge::setSelectedWaypointIndex(int idx)
{
    if (idx < 0) idx = 0;
    if (idx >= m_waypointCount) idx = m_waypointCount > 0 ? m_waypointCount - 1 : 0;
    if (m_selectedWaypointIndex != idx) {
        m_selectedWaypointIndex = idx;
        emit selectedWaypointIndexChanged();
    }
}

void AIEditorQmlBridge::updateWaypoint(int index, double x, double y, double z, double speed)
{
    if (index < 0 || index >= m_waypoints.size()) return;
    QVariantMap wp = m_waypoints[index].toMap();
    wp["x"] = x;
    wp["y"] = y;
    wp["z"] = z;
    wp["speed"] = speed;
    m_waypoints[index] = wp;
    emit waypointsChanged();
}

// --- Telemetry-based AI learning ---

static TelemetrySample variantToTelemetrySample(const QVariantList& list)
{
    TelemetrySample s{};
    if (list.size() >= 9) {
        s.timestamp = list[0].toFloat();
        s.speed = list[1].toFloat();
        s.rpm = list[2].toFloat();
        s.gear = list[3].toInt();
        s.throttle = list[4].toFloat();
        s.brake = list[5].toFloat();
        s.steering = list[6].toFloat();
        s.pos[0] = list[7].toFloat();
        s.pos[1] = list[8].toFloat();
        if (list.size() > 9) s.pos[2] = list[9].toFloat();
        if (list.size() > 10) s.gForce[0] = list[10].toFloat();
        if (list.size() > 11) s.gForce[1] = list[11].toFloat();
        if (list.size() > 12) s.gForce[2] = list[12].toFloat();
    }
    return s;
}

void AIEditorQmlBridge::trainFromTelemetry(const QString& track, const QVariantList& samples)
{
    if (track.isEmpty() || samples.isEmpty()) {
        emit errorMessage("Invalid training data");
        return;
    }

    QVector<TelemetrySample> converted;
    converted.reserve(samples.size());
    for (const auto& v : samples) {
        converted.append(variantToTelemetrySample(v.toList()));
    }

    m_telemetryTrainer.ingestTelemetry(track, converted);
    m_telemetryTrainer.analyzeTrack(track);

    double knowledge = m_telemetryTrainer.getTrackKnowledge(track);
    emit learningProgress(knowledge);

    QString msg = QString("Trained on %1 from %2 samples (%3 laps analyzed)")
        .arg(track).arg(samples.size()).arg(m_telemetryTrainer.getTotalLapsAnalyzed());
    emit statusMessage(msg);
    emit trainingComplete(track, knowledge);

    m_learnedData.totalLapsDriven = m_telemetryTrainer.getTotalLapsAnalyzed();
    saveLearnedData();
}

void AIEditorQmlBridge::analyzeTrackData(const QString& track)
{
    if (track.isEmpty()) return;
    m_telemetryTrainer.analyzeTrack(track);

    auto data = m_telemetryTrainer.getTrackData(track);
    if (data.lapCount > 0) {
        emit statusMessage(QString("Track %1 analyzed: %2 laps, best %.3fs, consistency %.0f%%")
            .arg(track).arg(data.lapCount).arg(data.bestLapTime).arg(data.consistency * 100));
    } else {
        emit statusMessage(QString("No data available for %1").arg(track));
    }
}

QVariantMap AIEditorQmlBridge::getOptimizedDriverProfile(const QString& track) const
{
    auto profile = m_telemetryTrainer.generateOptimizedProfile(track);
    QVariantMap map;
    map["name"] = profile.name;
    map["skill"] = profile.skill;
    map["aggression"] = profile.aggression;
    map["defensive"] = profile.defensive;
    map["consistency"] = profile.consistency;
    map["mistakeRate"] = profile.mistakeRate;
    map["tireManagement"] = profile.tireManagement;
    map["fuelManagement"] = profile.fuelManagement;
    map["wetSkill"] = profile.wetSkill;
    map["qualifyingPace"] = profile.qualifyingPace;
    map["racePace"] = profile.racePace;
    return map;
}

QVariantMap AIEditorQmlBridge::getTrackStats(const QString& track) const
{
    auto data = m_telemetryTrainer.getTrackData(track);
    QVariantMap map;
    map["trackName"] = data.trackName;
    map["lapCount"] = data.lapCount;
    map["bestLapTime"] = data.bestLapTime;
    map["averageLapTime"] = data.averageLapTime;
    map["consistency"] = data.consistency;
    map["drivingScore"] = data.drivingScore;
    map["cornerCount"] = data.corners.size();
    return map;
}

QStringList AIEditorQmlBridge::getTrainedTracks() const
{
    return m_telemetryTrainer.getKnownTracks();
}

int AIEditorQmlBridge::getTotalLapsAnalyzed() const
{
    return m_telemetryTrainer.getTotalLapsAnalyzed();
}

void AIEditorQmlBridge::clearTrainingData()
{
    m_telemetryTrainer.clear();
    emit trainingDataCleared();
    emit statusMessage("Training data cleared");
}

double AIEditorQmlBridge::getTrackKnowledgeLevel(const QString& track) const
{
    return m_telemetryTrainer.getTrackKnowledge(track);
}

QVariantList AIEditorQmlBridge::getCornerAnalysis(const QString& track) const
{
    auto data = m_telemetryTrainer.getTrackData(track);
    QVariantList result;
    for (const auto& c : data.corners) {
        QVariantMap cm;
        cm["cornerIndex"] = c.cornerIndex;
        cm["entrySpeed"] = c.entrySpeed;
        cm["apexSpeed"] = c.apexSpeed;
        cm["exitSpeed"] = c.exitSpeed;
        cm["brakingPoint"] = c.brakingPoint;
        cm["brakePressure"] = c.brakePressure;
        cm["steeringAngle"] = c.steeringAngle;
        cm["timeLost"] = c.timeLost;
        result.append(cm);
    }
    return result;
}

void AIEditorQmlBridge::applyTelemetryProfile(const QString& track)
{
    auto profile = m_telemetryTrainer.generateOptimizedProfile(track);

    int difficulty = qBound(0, static_cast<int>(profile.skill * 100), 100);
    int aggression = qBound(0, static_cast<int>(profile.aggression * 100), 100);
    int consistency = qBound(0, static_cast<int>(profile.consistency * 100), 100);

    m_currentDifficulty = difficulty;
    m_aggression = aggression;
    m_consistency = consistency;

    emit difficultyChanged(difficulty);
    emit aggressionChanged(aggression);
    emit consistencyChanged(consistency);
    emit profileApplied(track, profile.name);
    emit statusMessage(QString("Applied telemetry-derived profile for %1").arg(track));
}

// --- Multi-car race methods ---

void AIEditorQmlBridge::setupMultiCarRace(int numDrivers, int laps, double trackLength)
{
    m_multiCarAI.setupRace(numDrivers, laps, static_cast<float>(trackLength));
    emit statusMessage(QString("Multi-car race setup: %1 drivers, %2 laps, %3m track")
        .arg(numDrivers).arg(laps).arg(trackLength, 0, 'f', 0));
}

void AIEditorQmlBridge::startMultiCarRace()
{
    if (m_multiCarAI.grid().drivers.isEmpty()) {
        emit errorMessage("No drivers configured. Call setupMultiCarRace first.");
        return;
    }

    // Assign named profiles with distributed skill
    QStringList names = {"Verstappen","Hamilton","Leclerc","Norris","Sainz",
                         "Piastri","Russell","Alonso","Perez","Stroll",
                         "Gasly","Ocon","Tsunoda","Ricciardo","Hulkenberg",
                         "Magnussen","Albon","Sargeant","Bottas","Zhou"};

    int numDrivers = m_multiCarAI.grid().drivers.size();
    for (int i = 0; i < numDrivers; ++i) {
        auto* d = m_multiCarAI.getDriver(i);
        if (!d) continue;
        if (i < names.size()) d->name = names[i];

        float skillBase = 0.5f + (1.0f - static_cast<float>(i) / numDrivers) * 0.5f;
        auto p = d->profile;
        p.skill = qBound(0.3f, skillBase + AiBehaviorModel::randomFloat(-0.1f, 0.1f), 1.0f);
        p.aggression = qBound(0.2f, 0.5f + AiBehaviorModel::randomFloat(-0.3f, 0.3f), 1.0f);
        p.consistency = qBound(0.3f, 0.6f + AiBehaviorModel::randomFloat(-0.2f, 0.2f), 1.0f);
        p.racePace = qBound(0.3f, p.skill * 0.9f + AiBehaviorModel::randomFloat(-0.1f, 0.1f), 1.0f);
        p.qualifyingPace = qBound(0.3f, p.skill * 0.95f + AiBehaviorModel::randomFloat(-0.05f, 0.1f), 1.0f);
        p.tireManagement = qBound(0.2f, 0.5f + AiBehaviorModel::randomFloat(-0.2f, 0.2f), 1.0f);
        p.mistakeRate = qBound(0.01f, 0.15f * (1.0f - p.skill * 0.7f), 0.3f);
        d->profile = p;
    }

    emit statusMessage("Multi-car race started with " + QString::number(numDrivers) + " drivers");
}

void AIEditorQmlBridge::stopMultiCarRace()
{
    if (m_multiCarAI.getLeaderboard().isEmpty()) {
        emit statusMessage("No race in progress");
        return;
    }

    auto leaderboard = m_multiCarAI.getLeaderboard();
    for (auto& driver : leaderboard) {
        if (!driver.dnf && driver.lap < m_multiCarAI.getLeaderboard().size()) {
            m_multiCarAI.advanceDriver(driver, 0.0f);
        }
    }

    m_raceFinished = true;
    emit raceFinished();
    emit statusMessage("Multi-car race stopped - final positions determined");
}

void AIEditorQmlBridge::resetMultiCarRace()
{
    m_multiCarAI.clearGrid();
    emit statusMessage("Multi-car race reset");
}

QVariantList AIEditorQmlBridge::getMultiCarLeaderboard()
{
    QVariantList result;
    auto sorted = m_multiCarAI.getLeaderboard();
    for (const auto& d : sorted) {
        QVariantMap entry;
        entry["position"] = d.dnf ? -1 : d.position;
        entry["name"] = d.name;
        entry["lap"] = d.lap;
        entry["totalLaps"] = d.totalLaps;
        entry["gapAhead"] = d.distanceToCarAhead < 1e5f ? d.distanceToCarAhead / 10.0f : -1.0;
        entry["bestLapTime"] = d.bestLapTime < 1e8f ? d.bestLapTime : -1.0;
        entry["tireWear"] = static_cast<int>(d.tireWear);
        entry["fuel"] = static_cast<int>(d.fuel);
        entry["dnf"] = d.dnf;
        entry["finished"] = d.finished;
        entry["pitStops"] = d.pitStops;
        entry["speed"] = d.currentSpeed;
        result.append(entry);
    }
    return result;
}

QVariantMap AIEditorQmlBridge::getMultiCarRaceStats()
{
    QVariantMap stats;
    stats["raceTime"] = m_multiCarAI.grid().drivers.isEmpty() ? 0.0 : m_multiCarAI.grid().drivers[0].raceTime;
    stats["fastestLap"] = m_multiCarAI.getFastestLap();
    stats["fastestLapDriver"] = m_multiCarAI.getFastestLapDriver();
    stats["totalOvertakes"] = m_multiCarAI.getTotalOvertakes();
    stats["totalPositionChanges"] = m_multiCarAI.getTotalPositionChanges();
    stats["isComplete"] = m_multiCarAI.isRaceComplete();
    stats["leaderLap"] = m_multiCarAI.grid().leaderLap;
    stats["totalLaps"] = m_multiCarAI.grid().totalLaps;
    return stats;
}

QVariantList AIEditorQmlBridge::getMultiCarRaceEvents()
{
    QVariantList result;
    auto events = m_multiCarAI.pendingEvents();
    for (const auto& ev : events) {
        QVariantMap e;
        e["type"] = static_cast<int>(ev.type);
        e["timestamp"] = ev.timestamp;
        e["driverId"] = ev.driverId;
        e["otherDriverId"] = ev.otherDriverId;
        e["description"] = ev.description;
        result.append(e);
    }
    return result;
}

bool AIEditorQmlBridge::isMultiCarRaceComplete()
{
    return m_multiCarAI.isRaceComplete();
}

double AIEditorQmlBridge::getMultiCarEstimatedRaceTime()
{
    return m_multiCarAI.getEstimatedRaceTime();
}

} // namespace ks
