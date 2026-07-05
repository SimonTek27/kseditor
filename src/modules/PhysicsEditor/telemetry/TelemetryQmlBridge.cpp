#include "TelemetryQmlBridge.h"
#include <cmath>
#include <algorithm>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTextStream>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QtMath>

namespace ks {

TelemetryQmlBridge* TelemetryQmlBridge::s_instance = nullptr;

TelemetryQmlBridge* TelemetryQmlBridge::instance() {
    if (!s_instance) {
        s_instance = new TelemetryQmlBridge();
    }
    return s_instance;
}

TelemetryQmlBridge::TelemetryQmlBridge(QObject* parent)
    : QObject(parent)
{
    m_sessionStartTime = QDateTime::currentDateTime();
}

void TelemetryQmlBridge::startSession() {
    m_isRecording = true;
    m_sessionStartTime = QDateTime::currentDateTime();
    m_sessionName = QString("Session_%1").arg(m_sessionStartTime.toString("yyyyMMdd_HHmmss"));
    m_lapData.clear();
    m_currentLapSamples.clear();
    m_currentLapNumber = 0;
    emit statusMessage("Telemetry session started: " + m_sessionName);
    emit sessionStarted(m_sessionName);
}

void TelemetryQmlBridge::stopSession() {
    if (!m_isRecording) return;
    m_isRecording = false;

    if (!m_currentLapSamples.isEmpty()) {
        finishCurrentLap();
    }

    saveSessionData();
    emit statusMessage("Telemetry session stopped");
    emit sessionStopped(m_sessionName);
}

void TelemetryQmlBridge::loadFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        emit errorMessage("Cannot load telemetry file: " + path);
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        emit errorMessage("Invalid telemetry file format");
        return;
    }

    QJsonObject root = doc.object();
    m_sessionName = root["session"].toString();
    m_lapData.clear();

    QJsonArray laps = root["laps"].toArray();
    for (const auto& lapVal : laps) {
        QJsonObject lapObj = lapVal.toObject();
        LapData lap;
        lap.lapNumber = lapObj["lapNumber"].toInt();
        lap.lapTime = lapObj["lapTime"].toDouble();
        lap.sector1 = lapObj["sector1"].toDouble();
        lap.sector2 = lapObj["sector2"].toDouble();
        lap.sector3 = lapObj["sector3"].toDouble();
        lap.topSpeed = static_cast<float>(lapObj["topSpeed"].toDouble());
        lap.avgSpeed = static_cast<float>(lapObj["avgSpeed"].toDouble());
        lap.maxGForce = static_cast<float>(lapObj["maxGForce"].toDouble());
        lap.avgThrottle = static_cast<float>(lapObj["avgThrottle"].toDouble());
        lap.avgBrake = static_cast<float>(lapObj["avgBrake"].toDouble());
        lap.avgSteering = static_cast<float>(lapObj["avgSteering"].toDouble());
        lap.throttleTimePercent = static_cast<float>(lapObj["throttleTimePercent"].toDouble());
        lap.brakeTimePercent = static_cast<float>(lapObj["brakeTimePercent"].toDouble());
        lap.shiftCount = lapObj["shiftCount"].toInt();
        lap.valid = lapObj["valid"].toBool();

        // Restore samples if available
        QJsonArray sampleArr = lapObj["samples"].toArray();
        for (const auto& sv : sampleArr) {
            QJsonObject so = sv.toObject();
            TelemetrySample ts;
            ts.speed = static_cast<float>(so["speed"].toDouble());
            ts.rpm = static_cast<float>(so["rpm"].toDouble());
            ts.throttle = static_cast<float>(so["throttle"].toDouble());
            ts.brake = static_cast<float>(so["brake"].toDouble());
            ts.steering = static_cast<float>(so["steering"].toDouble());
            ts.gear = so["gear"].toInt();
            ts.lateralG = static_cast<float>(so["latG"].toDouble());
            ts.longitudinalG = static_cast<float>(so["lonG"].toDouble());
            lap.samples.append(ts);
        }
        m_lapData.append(lap);
    }

    emit dataReceived(aggregateData());
    emit statusMessage(QString("Loaded telemetry: %1 (%2 laps)").arg(m_sessionName).arg(m_lapData.size()));
}

void TelemetryQmlBridge::recordSample(float speed, float rpm, float throttle, float brake,
                                       float steering, float gear, float latG, float lonG) {
    if (!m_isRecording) return;

    TelemetrySample sample;
    sample.timestamp = QDateTime::currentDateTime();
    sample.speed = speed;
    sample.rpm = rpm;
    sample.throttle = throttle;
    sample.brake = brake;
    sample.steering = steering;
    sample.gear = static_cast<int>(gear);
    sample.lateralG = latG;
    sample.longitudinalG = lonG;

    m_currentLapSamples.append(sample);

    // Track top speed and avg speed
    if (speed > m_currentLapTopSpeed) m_currentLapTopSpeed = speed;
    m_currentLapSpeedSum += speed;

    // Track max G force
    float totalG = std::sqrt(latG * latG + lonG * lonG);
    if (totalG > m_currentLapMaxG) m_currentLapMaxG = totalG;
}

void TelemetryQmlBridge::markLapStart() {
    if (!m_isRecording) return;

    if (!m_currentLapSamples.isEmpty()) {
        finishCurrentLap();
    }

    m_currentLapNumber++;
    m_currentLapStartTime = QDateTime::currentDateTime();
    m_currentLapSamples.clear();
    m_currentLapTopSpeed = 0;
    m_currentLapSpeedSum = 0;
    m_currentLapMaxG = 0;
    m_currentLapSampleCount = 0;

    emit statusMessage(QString("Lap %1 started").arg(m_currentLapNumber));
}

void TelemetryQmlBridge::markSector(int sector) {
    if (!m_isRecording || m_currentLapSamples.isEmpty()) return;

    QDateTime now = QDateTime::currentDateTime();
    double sectorTime = m_currentLapStartTime.msecsTo(now) / 1000.0;

    if (sector == 1) m_currentSector1 = sectorTime;
    else if (sector == 2) m_currentSector2 = sectorTime;

    Q_EMIT this->sectorTime(sector, sectorTime);
}

void TelemetryQmlBridge::finishCurrentLap() {
    if (m_currentLapSamples.isEmpty()) return;

    LapData lap;
    lap.lapNumber = m_currentLapNumber;
    lap.lapTime = m_currentLapStartTime.msecsTo(QDateTime::currentDateTime()) / 1000.0;
    lap.sector1 = m_currentSector1;
    lap.sector2 = m_currentSector2;
    lap.sector3 = lap.lapTime - m_currentSector1 - m_currentSector2;
    lap.topSpeed = m_currentLapTopSpeed;
    lap.avgSpeed = m_currentLapSampleCount > 0 ? m_currentLapSpeedSum / m_currentLapSampleCount : 0;
    lap.maxGForce = m_currentLapMaxG;
    lap.valid = m_currentLapSamples.size() > 50;

    // Compute derived metrics from samples
    float throttleSum = 0, brakeSum = 0, steeringSum = 0;
    int throttleOn = 0, brakeOn = 0, coast = 0, prevGear = 0;
    int total = m_currentLapSamples.size();
    for (const auto& s : m_currentLapSamples) {
        throttleSum += s.throttle;
        brakeSum += s.brake;
        steeringSum += std::abs(s.steering);
        if (s.throttle > 0.05f) throttleOn++;
        else if (s.brake > 0.05f) brakeOn++;
        else coast++;
        if (s.gear != prevGear && prevGear != 0) lap.shiftCount++;
        prevGear = s.gear;
    }
    if (total > 0) {
        lap.avgThrottle = throttleSum / total;
        lap.avgBrake = brakeSum / total;
        lap.avgSteering = steeringSum / total;
        lap.throttleTimePercent = 100.0f * throttleOn / total;
        lap.brakeTimePercent = 100.0f * brakeOn / total;
    }
    lap.samples = m_currentLapSamples;

    m_lapData.append(lap);

    // Track best lap
    if (lap.lapTime < m_bestLapTime || m_bestLapTime < 0) {
        m_bestLapTime = lap.lapTime;
        m_bestLapNumber = lap.lapNumber;
        emit bestLapChanged(m_bestLapNumber, m_bestLapTime);
    }

    emit lapCompleted(lap.lapNumber, lap.lapTime);
    emit dataReceived(aggregateData());
}

QVariantMap TelemetryQmlBridge::getLapData(int lapNumber) const {
    for (const auto& lap : m_lapData) {
        if (lap.lapNumber == lapNumber) {
            QVariantMap m;
            m["lapNumber"] = lap.lapNumber;
            m["lapTime"] = lap.lapTime;
            m["sector1"] = lap.sector1;
            m["sector2"] = lap.sector2;
            m["sector3"] = lap.sector3;
            m["topSpeed"] = lap.topSpeed;
            m["avgSpeed"] = lap.avgSpeed;
            m["maxGForce"] = lap.maxGForce;
            m["avgThrottle"] = lap.avgThrottle;
            m["avgBrake"] = lap.avgBrake;
            m["avgSteering"] = lap.avgSteering;
            m["throttleTimePercent"] = lap.throttleTimePercent;
            m["brakeTimePercent"] = lap.brakeTimePercent;
            m["shiftCount"] = lap.shiftCount;
            m["valid"] = lap.valid;

            // Include samples data
            QVariantList samples;
            for (const auto& s : lap.samples) {
                QVariantMap sm;
                sm["speed"] = s.speed;
                sm["rpm"] = s.rpm;
                sm["throttle"] = s.throttle;
                sm["brake"] = s.brake;
                sm["steering"] = s.steering;
                sm["gear"] = s.gear;
                sm["latG"] = s.lateralG;
                sm["lonG"] = s.longitudinalG;
                samples.append(sm);
            }
            m["samples"] = samples;
            return m;
        }
    }
    return {};
}

QVariantList TelemetryQmlBridge::getAllLaps() const {
    QVariantList laps;
    for (const auto& lap : m_lapData) {
        QVariantMap m;
        m["lapNumber"] = lap.lapNumber;
        m["lapTime"] = lap.lapTime;
        m["sector1"] = lap.sector1;
        m["sector2"] = lap.sector2;
        m["sector3"] = lap.sector3;
        m["topSpeed"] = lap.topSpeed;
        m["avgSpeed"] = lap.avgSpeed;
        m["maxGForce"] = lap.maxGForce;
        m["avgThrottle"] = lap.avgThrottle;
        m["avgBrake"] = lap.avgBrake;
        m["avgSteering"] = lap.avgSteering;
        m["throttleTimePercent"] = lap.throttleTimePercent;
        m["brakeTimePercent"] = lap.brakeTimePercent;
        m["shiftCount"] = lap.shiftCount;
        m["valid"] = lap.valid;
        laps.append(m);
    }
    return laps;
}

double TelemetryQmlBridge::bestLapTime() const { return m_bestLapTime; }
int TelemetryQmlBridge::bestLapNumber() const { return m_bestLapNumber; }
bool TelemetryQmlBridge::isRecording() const { return m_isRecording; }
int TelemetryQmlBridge::lapCount() const { return m_lapData.size(); }
int TelemetryQmlBridge::currentLapNumber() const { return m_currentLapNumber; }

void TelemetryQmlBridge::compareLaps(int lapA, int lapB) {
    QVariantMap lapAData = getLapData(lapA);
    QVariantMap lapBData = getLapData(lapB);

    if (lapAData.isEmpty() || lapBData.isEmpty()) {
        emit errorMessage("Cannot compare: one or both laps not found");
        return;
    }

    QStringList diff;
    double timeA = lapAData["lapTime"].toDouble();
    double timeB = lapBData["lapTime"].toDouble();
    double diffTime = timeA - timeB;

    diff.append(QString("Lap %1: %2s").arg(lapA).arg(timeA, 0, 'f', 3));
    diff.append(QString("Lap %1: %2s").arg(lapB).arg(timeB, 0, 'f', 3));
    diff.append(QString("Difference: %1s%2")
                .arg(diffTime > 0 ? "+" : "")
                .arg(diffTime, 0, 'f', 3));
    diff.append(QString("Lap %1 is %2faster")
                .arg(diffTime > 0 ? lapB : lapA)
                .arg(diffTime > 0 ? "" : " "));

    diff.append(QString("Top speed: %1 vs %2 km/h")
                .arg(lapAData["topSpeed"].toFloat(), 0, 'f', 1)
                .arg(lapBData["topSpeed"].toFloat(), 0, 'f', 1));
    diff.append(QString("Max G-force: %1 vs %2 G")
                .arg(lapAData["maxGForce"].toFloat(), 0, 'f', 2)
                .arg(lapBData["maxGForce"].toFloat(), 0, 'f', 2));

    emit comparisonComplete(diff);
}

void TelemetryQmlBridge::exportSession(const QString& path) {
    saveSessionToFile(path);
}

void TelemetryQmlBridge::clearSession() {
    m_lapData.clear();
    m_currentLapSamples.clear();
    m_currentLapNumber = 0;
    m_bestLapTime = -1;
    m_bestLapNumber = 0;
    m_currentLapTopSpeed = 0;
    m_currentLapSpeedSum = 0;
    m_currentLapMaxG = 0;
    m_currentLapSampleCount = 0;
    m_currentSector1 = 0;
    m_currentSector2 = 0;
    emit dataReceived(aggregateData());
    emit statusMessage("Telemetry session cleared");
}

QVariantMap TelemetryQmlBridge::aggregateData() const {
    QVariantMap data;
    data["lapCount"] = m_lapData.size();
    data["bestLapTime"] = m_bestLapTime;
    data["bestLapNumber"] = m_bestLapNumber;
    data["isRecording"] = m_isRecording;
    data["currentLap"] = m_currentLapNumber;
    return data;
}

void TelemetryQmlBridge::saveSessionData() {
    QString savePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                       + "/ksEditor/telemetry/" + m_sessionName + ".json";
    saveSessionToFile(savePath);
}

void TelemetryQmlBridge::saveSessionToFile(const QString& path) {
    QDir().mkpath(QFileInfo(path).absolutePath());

    QJsonObject root;
    root["session"] = m_sessionName;
    root["startTime"] = m_sessionStartTime.toString(Qt::ISODate);
    root["car"] = m_currentCar;

    QJsonArray laps;
    for (const auto& lap : m_lapData) {
        QJsonObject l;
        l["lapNumber"] = lap.lapNumber;
        l["lapTime"] = lap.lapTime;
        l["sector1"] = lap.sector1;
        l["sector2"] = lap.sector2;
        l["sector3"] = lap.sector3;
        l["topSpeed"] = lap.topSpeed;
        l["avgSpeed"] = lap.avgSpeed;
        l["maxGForce"] = lap.maxGForce;
        l["avgThrottle"] = lap.avgThrottle;
        l["avgBrake"] = lap.avgBrake;
        l["avgSteering"] = lap.avgSteering;
        l["throttleTimePercent"] = lap.throttleTimePercent;
        l["brakeTimePercent"] = lap.brakeTimePercent;
        l["shiftCount"] = lap.shiftCount;
        l["valid"] = lap.valid;
        laps.append(l);
    }
    root["laps"] = laps;

    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
        file.close();
        emit statusMessage("Telemetry saved to: " + path);
    }
}

// ── Sector analysis ─────────────────────────────────────────────────────

QVariantMap TelemetryQmlBridge::analyzeSector(int lapNumber, int sector) {
    QVariantMap analysis;

    for (const auto& lap : m_lapData) {
        if (lap.lapNumber != lapNumber) continue;

        double sectorTime = 0;
        switch (sector) {
            case 1: sectorTime = lap.sector1; break;
            case 2: sectorTime = lap.sector2; break;
            case 3: sectorTime = lap.sector3; break;
            default: return analysis;
        }

        analysis["sector"] = sector;
        analysis["lapNumber"] = lapNumber;
        analysis["sectorTime"] = sectorTime;
        analysis["sectorPercentOfLap"] = (sectorTime / lap.lapTime) * 100.0;

        // Find best sector time across all laps
        double bestSector = 1e9;
        for (const auto& other : m_lapData) {
            double otherTime = 0;
            switch (sector) {
                case 1: otherTime = other.sector1; break;
                case 2: otherTime = other.sector2; break;
                case 3: otherTime = other.sector3; break;
            }
            if (otherTime > 0 && otherTime < bestSector) bestSector = otherTime;
        }

        analysis["bestSectorTime"] = (bestSector < 1e8) ? bestSector : sectorTime;
        analysis["deltaToBest"] = sectorTime - bestSector;
        analysis["potential"] = sectorTime - bestSector > 0.1
            ? "Improvement possible" : "Near optimal";

        emit sectorAnalysisComplete(lapNumber, sector, analysis);
        return analysis;
    }

    return analysis;
}

QVariantList TelemetryQmlBridge::analyzeAllSectors(int lapNumber) {
    QVariantList sectorAnalyses;
    for (int s = 1; s <= 3; ++s) {
        QVariantMap sa = analyzeSector(lapNumber, s);
        if (!sa.isEmpty()) sectorAnalyses.append(sa);
    }
    return sectorAnalyses;
}

QVariantMap TelemetryQmlBridge::getSectorComparison(int sector, int lapA, int lapB) {
    QVariantMap result;
    double timeA = 0, timeB = 0;
    int lapANum = 0, lapBNum = 0;

    for (const auto& lap : m_lapData) {
        if (lap.lapNumber == lapA) {
            switch (sector) {
                case 1: timeA = lap.sector1; break;
                case 2: timeA = lap.sector2; break;
                case 3: timeA = lap.sector3; break;
            }
            lapANum = lap.lapNumber;
        }
        if (lap.lapNumber == lapB) {
            switch (sector) {
                case 1: timeB = lap.sector1; break;
                case 2: timeB = lap.sector2; break;
                case 3: timeB = lap.sector3; break;
            }
            lapBNum = lap.lapNumber;
        }
    }

    result["lapA"] = lapANum;
    result["lapB"] = lapBNum;
    result["sector"] = sector;
    result["timeA"] = timeA;
    result["timeB"] = timeB;
    result["delta"] = timeA - timeB;
    result["fasterLap"] = (timeA < timeB) ? lapA : lapB;

    return result;
}

QVariantMap TelemetryQmlBridge::findOptimalSectorTimes() {
    QVariantMap optimal;
    double bestS1 = 1e9, bestS2 = 1e9, bestS3 = 1e9;

    for (const auto& lap : m_lapData) {
        if (lap.sector1 > 0 && lap.sector1 < bestS1) bestS1 = lap.sector1;
        if (lap.sector2 > 0 && lap.sector2 < bestS2) bestS2 = lap.sector2;
        if (lap.sector3 > 0 && lap.sector3 < bestS3) bestS3 = lap.sector3;
    }

    optimal["sector1"] = (bestS1 < 1e8) ? bestS1 : 0;
    optimal["sector2"] = (bestS2 < 1e8) ? bestS2 : 0;
    optimal["sector3"] = (bestS3 < 1e8) ? bestS3 : 0;
    optimal["theoreticalBest"] = (bestS1 < 1e8 && bestS2 < 1e8 && bestS3 < 1e8)
        ? bestS1 + bestS2 + bestS3 : 0;
    optimal["deltaToBestLap"] = optimal["theoreticalBest"].toDouble() - m_bestLapTime;

    // Find which lap had each best sector
    for (const auto& lap : m_lapData) {
        if (lap.sector1 == bestS1) optimal["bestSector1Lap"] = lap.lapNumber;
        if (lap.sector2 == bestS2) optimal["bestSector2Lap"] = lap.lapNumber;
        if (lap.sector3 == bestS3) optimal["bestSector3Lap"] = lap.lapNumber;
    }

    return optimal;
}

QVariantList TelemetryQmlBridge::getIdealLap() const {
    QVariantList idealSamples;
    if (m_lapData.isEmpty()) return idealSamples;

    // Find the lap that's closest to optimal sector times
    QVector<LapData> validLaps;
    for (const auto& lap : m_lapData) {
        if (lap.valid && lap.lapTime > 0) validLaps.append(lap);
    }

    if (validLaps.isEmpty()) return idealSamples;

    // For now, return data from the best lap
    for (const auto& lap : validLaps) {
        if (lap.lapNumber == m_bestLapNumber) {
            QVariantMap entry;
            entry["lapNumber"] = lap.lapNumber;
            entry["lapTime"] = lap.lapTime;
            entry["sector1"] = lap.sector1;
            entry["sector2"] = lap.sector2;
            entry["sector3"] = lap.sector3;
            entry["isIdeal"] = true;
            idealSamples.append(entry);
        }
    }

    return idealSamples;
}

// ── Trace data access ────────────────────────────────────────────────────

QVariantList TelemetryQmlBridge::getSpeedTrace(int lapNumber) const {
    for (const auto& lap : m_lapData) {
        if (lap.lapNumber != lapNumber) continue;
        QVariantList trace;
        trace.reserve(lap.samples.size());
        for (const auto& s : lap.samples)
            trace.append(s.speed);
        return trace;
    }
    return {};
}

QVariantList TelemetryQmlBridge::getThrottleTrace(int lapNumber) const {
    for (const auto& lap : m_lapData) {
        if (lap.lapNumber != lapNumber) continue;
        QVariantList trace;
        trace.reserve(lap.samples.size());
        for (const auto& s : lap.samples)
            trace.append(s.throttle);
        return trace;
    }
    return {};
}

// ── Driver input analysis ───────────────────────────────────────────────

QVariantMap TelemetryQmlBridge::analyzeDriverInputs(int lapNumber) {
    QVariantMap analysis;
    analysis["lapNumber"] = lapNumber;
    analysis["throttleAnalysis"] = analyzeThrottleBrakePattern(lapNumber);
    analysis["steeringAnalysis"] = analyzeSteeringPattern(lapNumber);
    analysis["gearAnalysis"] = analyzeGearUsage(lapNumber);

    emit driverAnalysisComplete(lapNumber, analysis);
    return analysis;
}

QVariantMap TelemetryQmlBridge::analyzeThrottleBrakePattern(int lapNumber) {
    QVariantMap result;

    for (const auto& lap : m_lapData) {
        if (lap.lapNumber != lapNumber) continue;

        const auto& samples = lap.samples;
        if (samples.isEmpty()) {
            // Fallback to aggregated metrics
            result["avgThrottle"] = lap.avgThrottle;
            result["avgBrake"] = lap.avgBrake;
            result["throttleTimePercent"] = lap.throttleTimePercent;
            result["brakeTimePercent"] = lap.brakeTimePercent;
            result["coastTimePercent"] = 100.0 - lap.throttleTimePercent - lap.brakeTimePercent;
            result["maxBrake"] = 0.0;
            result["throttleEvents"] = 0;
            result["brakeEvents"] = 0;
        } else {
            float throttleSum = 0, brakeSum = 0;
            int throttleOn = 0, brakeOn = 0, coast = 0;
            int throttleEvents = 0, brakeEvents = 0;
            float maxBrake = 0;
            bool wasThrottle = false, wasBrake = false;

            for (const auto& s : samples) {
                throttleSum += s.throttle;
                brakeSum += s.brake;

                if (s.throttle > 0.05f) {
                    throttleOn++;
                    if (!wasThrottle) { throttleEvents++; wasThrottle = true; }
                } else { wasThrottle = false; }

                if (s.brake > 0.05f) {
                    brakeOn++;
                    if (!wasBrake) { brakeEvents++; wasBrake = true; }
                    if (s.brake > maxBrake) maxBrake = s.brake;
                } else { wasBrake = false; }

                if (s.throttle <= 0.05f && s.brake <= 0.05f) coast++;
            }

            int total = samples.size();
            result["throttleTimePercent"] = (throttleOn * 100.0) / total;
            result["brakeTimePercent"] = (brakeOn * 100.0) / total;
            result["coastTimePercent"] = (coast * 100.0) / total;
            result["avgThrottle"] = throttleSum / total;
            result["avgBrake"] = brakeSum / total;
            result["maxBrake"] = maxBrake;
            result["throttleEvents"] = throttleEvents;
            result["brakeEvents"] = brakeEvents;

            float smoothnessPenalty = (throttleEvents + brakeEvents) * 3.0f;
            smoothnessPenalty += std::abs(lap.avgSteering) * 20.0f;
            result["smoothnessScore"] = std::max(0.0, 100.0 - smoothnessPenalty);
        }

        // Driving style classification
        if (result["brakeTimePercent"].toDouble() > 30) {
            result["style"] = "aggressive";
            result["styleDescription"] = "Late, heavy braking - trail brake into corners";
        } else if (result["throttleTimePercent"].toDouble() > 70) {
            result["style"] = "progressive";
            result["styleDescription"] = "Early throttle application - smooth corner exit";
        } else {
            result["style"] = "conservative";
            result["styleDescription"] = "Balanced inputs - consistent driving";
        }

        return result;
    }

    return result;
}

QVariantMap TelemetryQmlBridge::analyzeSteeringPattern(int lapNumber) {
    QVariantMap result;

    for (const auto& lap : m_lapData) {
        if (lap.lapNumber != lapNumber) continue;

        const auto& samples = lap.samples;
        if (samples.isEmpty()) {
            result["avgSteeringAngle"] = lap.avgSteering;
            result["maxSteeringAngle"] = 0.0;
            result["steeringEvents"] = 0;
            result["highSpeedCorrections"] = 0;
        } else {
            float totalSteering = 0, maxSteering = 0;
            int steeringEvents = 0, highSpeedCorrections = 0;
            bool wasSteering = false;

            for (const auto& s : samples) {
                float absSteer = std::abs(s.steering);
                totalSteering += absSteer;
                if (absSteer > maxSteering) maxSteering = absSteer;

                if (absSteer > 0.05f) {
                    if (!wasSteering) { steeringEvents++; wasSteering = true; }
                } else { wasSteering = false; }

                // Detect high-speed steering corrections
                if (absSteer > 0.15f && s.speed > 150) {
                    highSpeedCorrections++;
                }
            }

            int total = samples.size();
            result["avgSteeringAngle"] = totalSteering / total;
            result["maxSteeringAngle"] = maxSteering;
            result["steeringEvents"] = steeringEvents;
            result["highSpeedCorrections"] = highSpeedCorrections;

            float smoothnessPenalty = steeringEvents * 3.0f + highSpeedCorrections * 5.0f;
            result["steeringSmoothness"] = std::max(0.0, 100.0 - smoothnessPenalty);
        }

        // Steering characteristics
        double highSpeed = result["highSpeedCorrections"].toDouble();
        double maxSteer = result["maxSteeringAngle"].toDouble();
        if (highSpeed > 5) {
            result["characteristic"] = "nervous";
            result["characteristicDescription"] = "Frequent high-speed corrections - check aero balance";
        } else if (maxSteer > 0.5f) {
            result["characteristic"] = "rotational";
            result["characteristicDescription"] = "Large steering angles - rotation-focused driving";
        } else {
            result["characteristic"] = "smooth";
            result["characteristicDescription"] = "Minimal steering input - clean driving line";
        }

        return result;
    }

    return result;
}

QVariantMap TelemetryQmlBridge::analyzeGearUsage(int lapNumber) {
    QVariantMap result;

    for (const auto& lap : m_lapData) {
        if (lap.lapNumber != lapNumber) continue;

        const auto& samples = lap.samples;
        if (samples.isEmpty()) {
            result["shiftCount"] = lap.shiftCount;
            result["avgRpm"] = 0.0;
            result["maxRpm"] = 0.0;
            result["minRpm"] = 0.0;
            result["mostUsedGear"] = 0;
            result["gearDistribution"] = QVariantMap();
        } else {
            QMap<int, int> gearCount;
            int prevGear = 0, shifts = 0;
            float maxRpm = 0, minRpm = 1e6f, rpmSum = 0;

            for (const auto& s : samples) {
                gearCount[s.gear]++;
                if (s.rpm > maxRpm) maxRpm = s.rpm;
                if (s.rpm < minRpm && s.rpm > 0) minRpm = s.rpm;
                rpmSum += s.rpm;

                if (s.gear != prevGear && prevGear != 0) shifts++;
                prevGear = s.gear;
            }

            int total = samples.size();
            int mostUsedGear = 1, maxCount = 0;
            QVariantMap gearDist;
            for (auto it = gearCount.begin(); it != gearCount.end(); ++it) {
                gearDist[QString::number(it.key())] = (it.value() * 100.0) / total;
                if (it.value() > maxCount) {
                    maxCount = it.value();
                    mostUsedGear = it.key();
                }
            }

            result["gearDistribution"] = gearDist;
            result["mostUsedGear"] = mostUsedGear;
            result["avgRpm"] = rpmSum / total;
            result["maxRpm"] = maxRpm;
            result["minRpm"] = minRpm;
            result["shiftCount"] = shifts;
            result["rpmRange"] = maxRpm - minRpm;
        }

        double avgRpm = result["avgRpm"].toDouble();
        if (avgRpm > 6000)
            result["shiftStyle"] = "late_shifting";
        else if (avgRpm < 4500)
            result["shiftStyle"] = "early_shifting";
        else
            result["shiftStyle"] = "optimal";

        return result;
    }

    return result;
}

QVariantList TelemetryQmlBridge::getCornerAnalysis(int lapNumber) {
    QVariantList corners;

    for (const auto& lap : m_lapData) {
        if (lap.lapNumber != lapNumber) continue;

        const auto& samples = lap.samples;
        if (samples.size() < 10) return corners;

        // Detect corners by finding braking zones followed by lateral G peaks
        // A corner is: braking (decel + steering) -> min speed -> accelerating out
        int cornerIdx = 0;
        int i = 5;
        while (i < samples.size() - 5) {
            // Detect braking entry
            float braking = 0;
            for (int j = -3; j <= 3; j++) braking += samples[i + j].brake;
            braking /= 7.0f;

            float latG = 0;
            for (int j = -3; j <= 3; j++) latG += std::abs(samples[i + j].lateralG);
            latG /= 7.0f;

            if (braking > 0.3f && latG > 0.5f) {
                // Found a corner entry - find min speed point
                int minIdx = i;
                for (int j = i; j < std::min<qsizetype>(i + 50, samples.size()); j++) {
                    if (samples[j].speed < samples[minIdx].speed) minIdx = j;
                }

                int exitIdx = std::min<qsizetype>(minIdx + 30, samples.size() - 1);

                int entryIdx = std::max(0, i - 10);
                float entrySpeed = samples[entryIdx].speed;
                float minSpeed = samples[minIdx].speed;
                float exitSpeed = samples[exitIdx].speed;

                // Max lateral G through corner
                float peakLatG = 0;
                for (int j = entryIdx; j <= exitIdx; j++) {
                    if (std::abs(samples[j].lateralG) > peakLatG) peakLatG = std::abs(samples[j].lateralG);
                }

                // Time spent in corner (estimate)
                float timeInCorner = (exitIdx - entryIdx) * 0.016f; // ~60fps sample rate estimate

                QVariantMap corner;
                corner["name"] = QString("Corner %1").arg(++cornerIdx);
                corner["entrySpeed"] = entrySpeed;
                corner["exitSpeed"] = exitSpeed;
                corner["minSpeed"] = minSpeed;
                corner["lateralG"] = peakLatG;
                corner["timeLoss"] = std::max(0.0, timeInCorner * (1.0 - exitSpeed / entrySpeed));
                corner["efficiency"] = entrySpeed > 0 ? (exitSpeed / entrySpeed) * 100.0 : 0;
                corners.append(corner);

                i = exitIdx + 10;
            }
            i++;
        }

        if (corners.isEmpty()) {
            // Fallback if no corners detected
            QVariantMap fallback;
            fallback["name"] = "No corners detected";
            fallback["entrySpeed"] = lap.avgSpeed;
            fallback["exitSpeed"] = lap.avgSpeed;
            fallback["minSpeed"] = lap.avgSpeed * 0.6;
            fallback["lateralG"] = lap.maxGForce;
            fallback["timeLoss"] = 0;
            fallback["efficiency"] = 85.0;
            corners.append(fallback);
        }

        return corners;
    }

    return corners;
}

QVariantMap TelemetryQmlBridge::compareDrivingStyle(int lapA, int lapB) {
    QVariantMap result;

    QVariantMap inputsA = analyzeDriverInputs(lapA);
    QVariantMap inputsB = analyzeDriverInputs(lapB);

    result["lapA"] = lapA;
    result["lapB"] = lapB;

    // Compare throttle
    double throttleA = inputsA["throttleAnalysis"].toMap()["throttleTimePercent"].toDouble();
    double throttleB = inputsB["throttleAnalysis"].toMap()["throttleTimePercent"].toDouble();
    result["throttleDelta"] = throttleA - throttleB;

    // Compare braking
    double brakeA = inputsA["throttleAnalysis"].toMap()["brakeTimePercent"].toDouble();
    double brakeB = inputsB["throttleAnalysis"].toMap()["brakeTimePercent"].toDouble();
    result["brakeDelta"] = brakeA - brakeB;

    // Compare smoothness
    double smoothA = inputsA["throttleAnalysis"].toMap()["smoothnessScore"].toDouble();
    double smoothB = inputsB["throttleAnalysis"].toMap()["smoothnessScore"].toDouble();
    result["smoothnessDelta"] = smoothA - smoothB;

    // Compare steering
    double steerA = inputsA["steeringAnalysis"].toMap()["avgSteeringAngle"].toDouble();
    double steerB = inputsB["steeringAnalysis"].toMap()["avgSteeringAngle"].toDouble();
    result["steeringDelta"] = steerA - steerB;

    // Determine which lap was driven better
    QStringList insights;
    if (throttleA > throttleB + 10) insights.append("Lap A has more throttle time");
    else if (throttleB > throttleA + 10) insights.append("Lap B has more throttle time");

    result["insights"] = QVariant::fromValue(insights);

    return result;
}

} // namespace ks