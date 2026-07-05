#include "TelemetryViewerQmlBridge.h"
#include <cmath>
#include <algorithm>
#include "../../../core/sys/LogManager.h"
#include <cmath>
#include <algorithm>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QFileInfo>
#include <QWidget>

namespace ks {

TelemetryViewerQmlBridge* TelemetryViewerQmlBridge::s_instance = nullptr;

TelemetryViewerQmlBridge* TelemetryViewerQmlBridge::instance() {
    if (!s_instance) {
        s_instance = new TelemetryViewerQmlBridge();
    }
    return s_instance;
}

TelemetryViewerQmlBridge::TelemetryViewerQmlBridge(QObject* parent)
    : QObject(parent)
{
    m_dataDir = QDir::homePath() + "/ksEditor/telemetry";
    QDir().mkpath(m_dataDir);
}

bool TelemetryViewerQmlBridge::loadTelemetry(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        emit errorMessage("Cannot open telemetry file: " + path);
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseErr);
    if (parseErr.error != QJsonParseError::NoError) {
        emit errorMessage("Parse error: " + parseErr.errorString());
        return false;
    }

    QJsonObject root = doc.object();
    m_currentFile = path;
    m_laps.clear();
    m_lapCount = 0;
    m_bestLapTime.clear();
    m_currentSessionData = root;

    // Parse laps array
    QJsonArray lapsArr = root["laps"].toArray();
    for (const auto& lapVal : lapsArr) {
        QJsonObject lapObj = lapVal.toObject();
        LapInfo lap;
        lap.lapNumber = lapObj["lap"].toInt(0);
        lap.lapTime = static_cast<float>(lapObj["time"].toDouble(0));
        lap.sector1 = static_cast<float>(lapObj["sector1"].toDouble(0));
        lap.sector2 = static_cast<float>(lapObj["sector2"].toDouble(0));
        lap.sector3 = static_cast<float>(lapObj["sector3"].toDouble(0));
        lap.topSpeed = static_cast<float>(lapObj["topSpeed"].toDouble(0));
        lap.avgSpeed = static_cast<float>(lapObj["avgSpeed"].toDouble(0));

        // Parse trace arrays
        QJsonArray speedArr = lapObj["speedTrace"].toArray();
        for (const auto& v : speedArr) lap.speedTrace.append(v.toDouble());
        QJsonArray throttleArr = lapObj["throttleTrace"].toArray();
        for (const auto& v : throttleArr) lap.throttleTrace.append(v.toDouble());
        QJsonArray brakeArr = lapObj["brakeTrace"].toArray();
        for (const auto& v : brakeArr) lap.brakeTrace.append(v.toDouble());
        QJsonArray steeringArr = lapObj["steeringTrace"].toArray();
        for (const auto& v : steeringArr) lap.steeringTrace.append(v.toDouble());

        // Parse corner analysis
        QJsonArray cornersArr = lapObj["corners"].toArray();
        for (const auto& c : cornersArr) {
            QJsonObject co = c.toObject();
            LapInfo::CornerInfo ci;
            ci.name = co["name"].toString();
            ci.entrySpeed = static_cast<float>(co["entrySpeed"].toDouble(0));
            ci.apexSpeed = static_cast<float>(co["apexSpeed"].toDouble(0));
            ci.exitSpeed = static_cast<float>(co["exitSpeed"].toDouble(0));
            ci.minGap = static_cast<float>(co["minGap"].toDouble(0));
            lap.corners.append(ci);
        }

        m_laps.append(lap);
        m_lapCount++;

        // Track best lap
        if (lap.lapTime > 0 && (m_bestLapTime.isEmpty() || lap.lapTime < m_bestLapTime.toFloat())) {
            m_bestLapTime = QString::number(lap.lapTime, 'f', 3);
        }
    }

    emit lapCountChanged();
    if (!m_bestLapTime.isEmpty()) emit bestLapChanged();
    emit telemetryLoaded(path);
    emit statusMessage("Loaded " + QString::number(m_lapCount) + " laps from " + QFileInfo(path).fileName());
    return true;
}

// ============================================================================
// TelemetryViewerModule
// ============================================================================

TelemetryViewerModule::TelemetryViewerModule(QWidget* parent)
    : EditorModule(parent)
{}

bool TelemetryViewerModule::initialize()
{
    LOG_INFO("TelemetryViewerModule", "Initializing Telemetry Viewer module");
    return true;
}

void TelemetryViewerModule::shutdown()
{
    LOG_INFO("TelemetryViewerModule", "Shutting down Telemetry Viewer module");
}

void TelemetryViewerModule::importFile(const QString& filePath)
{
    if (auto* bridge = TelemetryViewerQmlBridge::instance()) {
        bridge->loadTelemetry(filePath);
    }
}

void TelemetryViewerModule::exportFile(const QString& filePath)
{
    if (auto* bridge = TelemetryViewerQmlBridge::instance()) {
        bridge->exportTelemetry(filePath);
    }
}

bool TelemetryViewerQmlBridge::exportTelemetry(const QString& path)
{
    QJsonObject root = m_currentSessionData;
    QJsonDocument doc(root);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        emit errorMessage("Cannot write to: " + path);
        return false;
    }
    file.write(doc.toJson());
    file.close();

    emit statusMessage("Telemetry exported to " + path);
    return true;
}

void TelemetryViewerQmlBridge::analyzeCurrent()
{
    if (m_laps.isEmpty()) {
        emit statusMessage("No lap data to analyze");
        emit analysisComplete();
        return;
    }

    // Analyze the latest lap
    const LapInfo& lap = m_laps.last();

    // Compute statistics from traces
    float maxSpeed = 0, minSpeed = 1e6f, avgSpeed = 0;
    float throttleApplication = 0, brakeApplication = 0;
    int throttleSamples = 0, brakeSamples = 0;

    for (int i = 0; i < lap.speedTrace.size(); ++i) {
        float spd = static_cast<float>(lap.speedTrace[i]);
        if (spd > maxSpeed) maxSpeed = spd;
        if (spd < minSpeed) minSpeed = spd;
        avgSpeed += spd;

        if (i < lap.throttleTrace.size() && lap.throttleTrace[i] > 0.1) {
            throttleApplication += static_cast<float>(lap.throttleTrace[i]);
            throttleSamples++;
        }
        if (i < lap.brakeTrace.size() && lap.brakeTrace[i] > 0.1) {
            brakeApplication += static_cast<float>(lap.brakeTrace[i]);
            brakeSamples++;
        }
    }

    int totalSamples = lap.speedTrace.size();
    if (totalSamples > 0) avgSpeed /= totalSamples;
    if (throttleSamples > 0) throttleApplication /= throttleSamples;
    if (brakeSamples > 0) brakeApplication /= brakeSamples;

    // Determine strongest and weakest sectors
    float sectors[3] = { lap.sector1, lap.sector2, lap.sector3 };
    int bestSector = 0, worstSector = 0;
    for (int i = 1; i < 3; ++i) {
        if (sectors[i] < sectors[bestSector]) bestSector = i;
        if (sectors[i] > sectors[worstSector]) worstSector = i;
    }

    QString summary = QString("Lap %1: %.3fs | Best sector: S%2 (%.3fs) | Worst: S%3 (%.3fs) | Avg speed: %.1f km/h")
        .arg(lap.lapNumber)
        .arg(lap.lapTime)
        .arg(bestSector + 1).arg(sectors[bestSector])
        .arg(worstSector + 1).arg(sectors[worstSector])
        .arg(avgSpeed * 3.6f);

    emit statusMessage(summary);
    emit analysisComplete();
}

QVariantList TelemetryViewerQmlBridge::getLapData() const
{
    QVariantList result;
    for (const auto& lap : m_laps) {
        QVariantMap entry;
        entry["lap"] = lap.lapNumber;
        entry["time"] = lap.lapTime;
        entry["sector1"] = lap.sector1;
        entry["sector2"] = lap.sector2;
        entry["sector3"] = lap.sector3;
        entry["topSpeed"] = lap.topSpeed;
        result.append(entry);
    }
    return result;
}

QVariantMap TelemetryViewerQmlBridge::getLapDetails(int lapIndex) const
{
    QVariantMap result;
    if (lapIndex < 0 || lapIndex >= m_laps.size()) return result;

    const LapInfo& lap = m_laps[lapIndex];
    result["lap"] = lap.lapNumber;
    result["time"] = lap.lapTime;
    result["sector1"] = lap.sector1;
    result["sector2"] = lap.sector2;
    result["sector3"] = lap.sector3;
    result["topSpeed"] = lap.topSpeed;
    result["avgSpeed"] = lap.avgSpeed;
    result["speedTraceCount"] = lap.speedTrace.size();
    result["cornerCount"] = lap.corners.size();

    // Build sector delta from best lap
    if (!m_bestLapTime.isEmpty()) {
        float best = m_bestLapTime.toFloat();
        result["deltaToBest"] = lap.lapTime - best;
    }

    return result;
}

QVariantList TelemetryViewerQmlBridge::compareLaps(int lapA, int lapB)
{
    QVariantList diffs;
    if (lapA < 0 || lapA >= m_laps.size() || lapB < 0 || lapB >= m_laps.size()) {
        emit comparisonReady(QStringList());
        return diffs;
    }

    const LapInfo& a = m_laps[lapA];
    const LapInfo& b = m_laps[lapB];

    // Sector-by-sector comparison
    QStringList summary;
    float totalDelta = 0;

    auto addDiff = [&](const QString& label, float va, float vb) {
        float delta = vb - va;
        totalDelta += delta;
        QVariantMap d;
        d["label"] = label;
        d["lapA"] = va;
        d["lapB"] = vb;
        d["delta"] = delta;
        d["faster"] = delta < 0 ? "A" : (delta > 0 ? "B" : "equal");
        diffs.append(d);
        summary.append(QString("%1: %2%3")
            .arg(label)
            .arg(delta > 0 ? "+" : "")
            .arg(delta, 0, 'f', 3));
    };

    addDiff("Sector 1", a.sector1, b.sector1);
    addDiff("Sector 2", a.sector2, b.sector2);
    addDiff("Sector 3", a.sector3, b.sector3);
    addDiff("Total", a.lapTime, b.lapTime);
    addDiff("Top Speed", a.topSpeed, b.topSpeed);

    // Speed trace delta (sample-based)
    int sampleCount = std::min(a.speedTrace.size(), b.speedTrace.size());
    if (sampleCount > 0) {
        double sumA = 0, sumB = 0;
        for (int i = 0; i < sampleCount; ++i) {
            sumA += a.speedTrace[i];
            sumB += b.speedTrace[i];
        }
        QVariantMap speedDelta;
        speedDelta["label"] = "Avg Speed Delta";
        speedDelta["lapA"] = sumA / sampleCount;
        speedDelta["lapB"] = sumB / sampleCount;
        speedDelta["delta"] = (sumB - sumA) / sampleCount;
        diffs.append(speedDelta);
    }

    emit comparisonReady(summary);
    return diffs;
}

QVariantMap TelemetryViewerQmlBridge::getSpeedTrace(int lapIndex) const
{
    QVariantMap result;
    if (lapIndex < 0 || lapIndex >= m_laps.size()) return result;

    const LapInfo& lap = m_laps[lapIndex];
    QVariantList times, values;
    for (int i = 0; i < lap.speedTrace.size(); ++i) {
        times.append(i * 0.01f); // 100Hz sampling
        values.append(lap.speedTrace[i]);
    }
    result["times"] = times;
    result["values"] = values;
    result["count"] = lap.speedTrace.size();
    result["max"] = lap.topSpeed;
    return result;
}

QVariantMap TelemetryViewerQmlBridge::getThrottleTrace(int lapIndex) const
{
    QVariantMap result;
    if (lapIndex < 0 || lapIndex >= m_laps.size()) return result;

    const LapInfo& lap = m_laps[lapIndex];
    QVariantList times, values;
    for (int i = 0; i < lap.throttleTrace.size(); ++i) {
        times.append(i * 0.01f);
        values.append(lap.throttleTrace[i]);
    }
    result["times"] = times;
    result["values"] = values;
    result["count"] = lap.throttleTrace.size();
    return result;
}

QVariantMap TelemetryViewerQmlBridge::getBrakeTrace(int lapIndex) const
{
    QVariantMap result;
    if (lapIndex < 0 || lapIndex >= m_laps.size()) return result;

    const LapInfo& lap = m_laps[lapIndex];
    QVariantList times, values;
    for (int i = 0; i < lap.brakeTrace.size(); ++i) {
        times.append(i * 0.01f);
        values.append(lap.brakeTrace[i]);
    }
    result["times"] = times;
    result["values"] = values;
    result["count"] = lap.brakeTrace.size();
    return result;
}

QVariantMap TelemetryViewerQmlBridge::getSteeringTrace(int lapIndex) const
{
    QVariantMap result;
    if (lapIndex < 0 || lapIndex >= m_laps.size()) return result;

    const LapInfo& lap = m_laps[lapIndex];
    QVariantList times, values;
    for (int i = 0; i < lap.steeringTrace.size(); ++i) {
        times.append(i * 0.01f);
        values.append(lap.steeringTrace[i]);
    }
    result["times"] = times;
    result["values"] = values;
    result["count"] = lap.steeringTrace.size();
    return result;
}

QVariantMap TelemetryViewerQmlBridge::getSectorAnalysis(int lapIndex) const
{
    QVariantMap result;
    if (lapIndex < 0 || lapIndex >= m_laps.size()) return result;

    const LapInfo& lap = m_laps[lapIndex];
    float total = lap.sector1 + lap.sector2 + lap.sector3;

    result["sector1"] = lap.sector1;
    result["sector2"] = lap.sector2;
    result["sector3"] = lap.sector3;
    result["total"] = total;
    result["sector1Pct"] = total > 0 ? (lap.sector1 / total * 100.0) : 0;
    result["sector2Pct"] = total > 0 ? (lap.sector2 / total * 100.0) : 0;
    result["sector3Pct"] = total > 0 ? (lap.sector3 / total * 100.0) : 0;

    // Find best sector times across all laps
    float bestS1 = 1e30f, bestS2 = 1e30f, bestS3 = 1e30f;
    for (const auto& l : m_laps) {
        if (l.sector1 > 0 && l.sector1 < bestS1) bestS1 = l.sector1;
        if (l.sector2 > 0 && l.sector2 < bestS2) bestS2 = l.sector2;
        if (l.sector3 > 0 && l.sector3 < bestS3) bestS3 = l.sector3;
    }
    result["bestSector1"] = bestS1 < 1e30f ? bestS1 : 0;
    result["bestSector2"] = bestS2 < 1e30f ? bestS2 : 0;
    result["bestSector3"] = bestS3 < 1e30f ? bestS3 : 0;
    result["deltaSector1"] = lap.sector1 - result["bestSector1"].toFloat();
    result["deltaSector2"] = lap.sector2 - result["bestSector2"].toFloat();
    result["deltaSector3"] = lap.sector3 - result["bestSector3"].toFloat();

    return result;
}

QVariantList TelemetryViewerQmlBridge::getCornerAnalysis(int lapIndex) const
{
    QVariantList result;
    if (lapIndex < 0 || lapIndex >= m_laps.size()) return result;

    const LapInfo& lap = m_laps[lapIndex];
    for (const auto& corner : lap.corners) {
        QVariantMap c;
        c["name"] = corner.name;
        c["entrySpeed"] = corner.entrySpeed;
        c["apexSpeed"] = corner.apexSpeed;
        c["exitSpeed"] = corner.exitSpeed;
        c["minGap"] = corner.minGap;
        c["speedLoss"] = corner.entrySpeed - corner.apexSpeed;
        c["speedGain"] = corner.exitSpeed - corner.apexSpeed;
        result.append(c);
    }

    return result;
}

QVariantMap TelemetryViewerQmlBridge::getImprovementSuggestions() const
{
    QVariantMap suggestions;

    if (m_laps.isEmpty()) return suggestions;

    // Find the best lap
    int bestIdx = 0;
    for (int i = 1; i < m_laps.size(); ++i) {
        if (m_laps[i].lapTime > 0 && m_laps[i].lapTime < m_laps[bestIdx].lapTime)
            bestIdx = i;
    }
    const LapInfo& best = m_laps[bestIdx];

    // Find weakest sector
    float worstSectorTime = 0;
    int worstSector = 0;
    float bestS1 = 1e30f, bestS2 = 1e30f, bestS3 = 1e30f;
    for (const auto& l : m_laps) {
        if (l.sector1 > 0 && l.sector1 < bestS1) bestS1 = l.sector1;
        if (l.sector2 > 0 && l.sector2 < bestS2) bestS2 = l.sector2;
        if (l.sector3 > 0 && l.sector3 < bestS3) bestS3 = l.sector3;
    }

    float deltaS1 = best.sector1 - bestS1;
    float deltaS2 = best.sector2 - bestS2;
    float deltaS3 = best.sector3 - bestS3;

    QVariantList tips;

    if (deltaS1 > 0.1f) {
        QVariantMap tip;
        tip["area"] = "Sector 1";
        tip["delta"] = deltaS1;
        tip["suggestion"] = "You lost " + QString::number(deltaS1, 'f', 3) + "s in Sector 1 vs your best. Focus on entry speed and braking points.";
        tips.append(tip);
    }
    if (deltaS2 > 0.1f) {
        QVariantMap tip;
        tip["area"] = "Sector 2";
        tip["delta"] = deltaS2;
        tip["suggestion"] = "Sector 2 has " + QString::number(deltaS2, 'f', 3) + "s room for improvement. Check throttle application through mid-corner.";
        tips.append(tip);
    }
    if (deltaS3 > 0.1f) {
        QVariantMap tip;
        tip["area"] = "Sector 3";
        tip["delta"] = deltaS3;
        tip["suggestion"] = "Sector 3 gap of " + QString::number(deltaS3, 'f', 3) + "s. Consider later braking into the final corners.";
        tips.append(tip);
    }

    // Consistency check
    if (m_laps.size() >= 3) {
        float mean = 0;
        for (const auto& l : m_laps) mean += l.lapTime;
        mean /= m_laps.size();
        float variance = 0;
        for (const auto& l : m_laps) variance += (l.lapTime - mean) * (l.lapTime - mean);
        variance /= m_laps.size();
        float stdDev = std::sqrt(variance);

        QVariantMap consistency;
        consistency["area"] = "Consistency";
        consistency["stdDev"] = stdDev;
        consistency["suggestion"] = stdDev > 1.0f
            ? "Your lap times vary by " + QString::number(stdDev, 'f', 2) + "s. Work on hitting the same marks each lap."
            : "Good consistency with " + QString::number(stdDev, 'f', 2) + "s variation.";
        tips.append(consistency);
    }

    suggestions["tips"] = tips;
    suggestions["bestLapIndex"] = bestIdx;
    suggestions["bestLapTime"] = best.lapTime;
    suggestions["potentialGain"] = deltaS1 + deltaS2 + deltaS3;

    return suggestions;
}

QStringList TelemetryViewerQmlBridge::getAvailableSessions() const
{
    QStringList sessions;
    QDir dir(m_dataDir);
    if (!dir.exists()) return sessions;

    QStringList filters;
    filters << "*.json" << "*.acs" << "*.ld";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Time);
    for (const auto& f : files) {
        sessions.append(f.absoluteFilePath());
    }
    return sessions;
}

QJsonObject TelemetryViewerModule::serializeProject() const
{
    QJsonObject data;
    auto* bridge = TelemetryViewerQmlBridge::instance();
    if (bridge) {
        data["currentFile"] = bridge->currentFile();
    }
    return data;
}

void TelemetryViewerModule::deserializeProject(const QJsonObject& data)
{
    if (data.contains("currentFile")) {
        QString filePath = data["currentFile"].toString();
        if (!filePath.isEmpty()) {
            importFile(filePath);
        }
    }
}

}

