#include "TelemetryAnalyzer.h"
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSharedMemory>
#include <QDebug>
#include <cstring>
#include <cmath>
#include <algorithm>
#include "core/editor/EditorConfig.h"

// ============================================================================
// AC Shared Memory struct (SPageFilePhysics)
// Matches the binary layout of the simulator's "Local\\acpmf_memory" segment.
// ============================================================================

namespace {

#pragma pack(push, 1)
struct AcPageFilePhysics {
    int32_t packetId;                   // 0x000
    float gas;                          // 0x004  throttle pedal (0-1)
    float brake;                        // 0x008  brake pedal (0-1)
    float fuel;                         // 0x00C  fuel level (litres)
    int32_t gear;                       // 0x010  -1=R, 0=N, 1..N=gear
    int32_t rpm;                        // 0x014  engine RPM
    float steerAngle;                   // 0x018  steering angle (radians)
    float speedKmh;                     // 0x01C  speed in km/h
    float velocity[3];                  // 0x020  world velocity (x,y,z)
    float accG[3];                      // 0x02C  acceleration G (lateral, vertical, longitudinal)
    float wheelAngle[4];                // 0x038  wheel rotation angle per wheel
    float suspensionHeight[4];          // 0x048  suspension travel
    float tireSlip[4];                  // 0x058  tire slip length
    float gasEffect;                    // 0x068  aero damage effect
    float clutchTemperature;            // 0x06C
    float clutchWear;                   // 0x070
    float clutchRpm;                    // 0x074
    float tcInAction;                   // 0x078  TC activation state
    float tcTractionControlLevel;       // 0x07C  TC intervention level
    float brakeBias;                    // 0x080
    float padding1;                     // 0x084
    float tirePressure[4];              // 0x088  tire pressures (PSI)
    float tireTemp[4];                  // 0x098  core tire temperature (Celsius)
    float treadDepth[4];               // 0x0A8
    float damageZoneHeight[4];          // 0x0B8
    float padLife[2];                   // 0x0C8
    float discTemp[4];                  // 0x0D0
};
#pragma pack(pop)

} // anonymous namespace

// ============================================================================
// Data loading
// ============================================================================

bool TelemetryAnalyzer::loadFromCsv(const QString& csvPath, QVector<TelemetrySample>& samples) {
    QFile file(csvPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    bool headerSkipped = false;
    QStringList headers;

    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();

        if (!headerSkipped) {
            headers = line.split(',');
            headerSkipped = true;
            continue;
        }

        if (line.isEmpty()) continue;

        QStringList values = line.split(',');
        if (values.size() < 10) continue;

        TelemetrySample sample;
        sample.timestamp = values[0].toFloat();
        sample.speed = values[1].toFloat();
        sample.rpm = values[2].toFloat();
        sample.gear = values[3].toInt();
        sample.throttle = values[4].toFloat();
        sample.brake = values[5].toFloat();
        sample.steering = values[6].toFloat();

        if (values.size() > 7) sample.pos[0] = values[7].toFloat();
        if (values.size() > 8) sample.pos[1] = values[8].toFloat();
        if (values.size() > 9) sample.pos[2] = values[9].toFloat();

        if (values.size() > 10) sample.tireTemp[0] = values[10].toFloat();
        if (values.size() > 11) sample.tireTemp[1] = values[11].toFloat();
        if (values.size() > 12) sample.tireTemp[2] = values[12].toFloat();
        if (values.size() > 13) sample.tireTemp[3] = values[13].toFloat();

        samples.append(sample);
    }

    file.close();
    return !samples.isEmpty();
}

bool TelemetryAnalyzer::loadFromJson(const QString& jsonPath, QVector<TelemetrySample>& samples) {
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        return false;
    }

    QJsonObject root = doc.object();
    QJsonArray samplesArray = root["samples"].toArray();

    for (const QJsonValue& val : samplesArray) {
        QJsonObject obj = val.toObject();
        TelemetrySample sample;

        sample.timestamp = obj["t"].toDouble();
        sample.speed = obj["speed"].toDouble();
        sample.rpm = obj["rpm"].toDouble();
        sample.gear = obj["gear"].toInt();
        sample.throttle = obj["throttle"].toDouble();
        sample.brake = obj["brake"].toDouble();
        sample.steering = obj["steering"].toDouble();

        QJsonArray pos = obj["pos"].toArray();
        if (pos.size() >= 3) {
            sample.pos[0] = pos[0].toDouble();
            sample.pos[1] = pos[1].toDouble();
            sample.pos[2] = pos[2].toDouble();
        }

        samples.append(sample);
    }

    return !samples.isEmpty();
}

bool TelemetryAnalyzer::loadFromAcSharedMemory(const QString& shmPath, QVector<TelemetrySample>& samples) {
    QString memoryName = shmPath.isEmpty()
        ? QStringLiteral("Local\\acpmf_memory")
        : shmPath;

    QSharedMemory sharedMemory(memoryName);

    if (!sharedMemory.attach(QSharedMemory::ReadOnly)) {
        qDebug() << "Failed to attach to AC shared memory:" << memoryName
                 << "-" << sharedMemory.errorString();
        return false;
    }

    if (sharedMemory.size() < static_cast<int>(sizeof(AcPageFilePhysics))) {
        qDebug() << "AC shared memory segment too small:" << sharedMemory.size()
                 << "bytes (expected at least" << sizeof(AcPageFilePhysics) << ")";
        sharedMemory.detach();
        return false;
    }

    const auto* phys = static_cast<const AcPageFilePhysics*>(sharedMemory.constData());

    TelemetrySample sample;
    std::memset(&sample, 0, sizeof(sample));

    sample.timestamp  = 0.0f;
    sample.speed      = phys->speedKmh;
    sample.rpm        = static_cast<float>(phys->rpm);
    sample.gear       = phys->gear;
    sample.throttle   = phys->gas;
    sample.brake      = phys->brake;
    sample.steering   = phys->steerAngle;
    sample.clutch     = phys->clutchWear;

    sample.pos[0]     = phys->velocity[0];
    sample.pos[1]     = phys->velocity[1];
    sample.pos[2]     = phys->velocity[2];

    sample.gForce[0]  = phys->accG[0];
    sample.gForce[1]  = phys->accG[1];
    sample.gForce[2]  = phys->accG[2];

    for (int i = 0; i < 4; ++i) {
        sample.tireTemp[i]     = phys->tireTemp[i];
        sample.tirePressure[i] = phys->tirePressure[i];
        sample.tireSlip[i]     = phys->tireSlip[i];
    }

    sample.fuel = phys->fuel;

    samples.append(sample);
    sharedMemory.detach();

    return !samples.isEmpty();
}

// ============================================================================
// Lap analysis
// ============================================================================

QVector<TelemetryAnalyzer::LapData> TelemetryAnalyzer::analyzeLaps(const QVector<TelemetrySample>& samples) {
    QVector<LapData> laps;

    if (samples.isEmpty()) return laps;

    int currentLap = samples.first().lapCount;
    int lapStart = 0;

    for (int i = 0; i < samples.size(); ++i) {
        if (samples[i].lapCount != currentLap) {
            // Lap completed
            QVector<TelemetrySample> lapSamples = samples.mid(lapStart, i - lapStart);
            LapData lap = analyzeLap(lapSamples, currentLap);
            laps.append(lap);

            currentLap = samples[i].lapCount;
            lapStart = i;
        }
    }

    // Last lap
    if (lapStart < samples.size()) {
        QVector<TelemetrySample> lapSamples = samples.mid(lapStart);
        LapData lap = analyzeLap(lapSamples, currentLap);
        laps.append(lap);
    }

    return laps;
}

TelemetryAnalyzer::LapData TelemetryAnalyzer::analyzeLap(const QVector<TelemetrySample>& lapSamples, int lapNumber) {
    LapData lap;
    lap.lapNumber = lapNumber;
    lap.isValid = true;

    if (lapSamples.isEmpty()) return lap;

    lap.lapTime = lapSamples.last().lapTime - lapSamples.first().lapTime;

    // Speed analysis
    float maxSpeed = 0;
    float totalSpeed = 0;
    float maxRPM = 0;
    float totalThrottle = 0;
    float totalBrake = 0;

    for (const TelemetrySample& s : lapSamples) {
        maxSpeed = std::max(maxSpeed, s.speed);
        totalSpeed += s.speed;
        maxRPM = std::max(maxRPM, s.rpm);
        totalThrottle += s.throttle;
        totalBrake += s.brake;
    }

    lap.maxSpeed = maxSpeed;
    lap.avgSpeed = totalSpeed / lapSamples.size();
    lap.maxRPM = maxRPM;
    lap.avgThrottle = totalThrottle / lapSamples.size();
    lap.avgBrake = totalBrake / lapSamples.size();

    // Tire wear
    for (int i = 0; i < 4; ++i) {
        lap.tireWear[i] = lapSamples.last().tireWear[i] - lapSamples.first().tireWear[i];
    }

    // Fuel
    lap.fuelUsed = lapSamples.first().fuel - lapSamples.last().fuel;

    return lap;
}

QVector<float> TelemetryAnalyzer::calculateLapTimes(const QVector<TelemetrySample>& samples) {
    QVector<float> lapTimes;

    if (samples.isEmpty()) return lapTimes;

    int currentLap = samples.first().lapCount;
    float lapStart = samples.first().timestamp;

    for (const TelemetrySample& s : samples) {
        if (s.lapCount != currentLap) {
            lapTimes.append(s.timestamp - lapStart);
            currentLap = s.lapCount;
            lapStart = s.timestamp;
        }
    }

    return lapTimes;
}

// ============================================================================
// Sector analysis
// ============================================================================

QVector<TelemetryAnalyzer::SectorAnalysis> TelemetryAnalyzer::analyzeSectors(
    const QVector<TelemetrySample>& lapSamples, float sectorSplits[3]) {

    QVector<SectorAnalysis> sectors;

    if (lapSamples.isEmpty()) return sectors;

    for (int sector = 0; sector < 3; ++sector) {
        SectorAnalysis analysis;
        analysis.sector = sector;

        float splitStart = (sector == 0) ? 0 : sectorSplits[sector - 1];
        float splitEnd = sectorSplits[sector];

        QVector<TelemetrySample> sectorSamples;
        for (const TelemetrySample& s : lapSamples) {
            float progress = s.timestamp / lapSamples.last().timestamp;
            if (progress >= splitStart && progress <= splitEnd) {
                sectorSamples.append(s);
            }
        }

        if (!sectorSamples.isEmpty()) {
            analysis.entrySpeed = sectorSamples.first().speed;
            analysis.exitSpeed = sectorSamples.last().speed;
            analysis.minSpeed = sectorSamples.first().speed;
            analysis.maxSpeed = sectorSamples.first().speed;
            analysis.avgSpeed = 0;

            float totalTime = 0;
            float brakeTime = 0;
            float throttleTime = 0;
            float coastTime = 0;

            for (const TelemetrySample& s : sectorSamples) {
                analysis.minSpeed = std::min(analysis.minSpeed, s.speed);
                analysis.maxSpeed = std::max(analysis.maxSpeed, s.speed);
                analysis.avgSpeed += s.speed;

                if (s.brake > 0.1f) brakeTime += 0.01f;
                else if (s.throttle > 0.1f) throttleTime += 0.01f;
                else coastTime += 0.01f;

                totalTime += 0.01f;
            }

            analysis.avgSpeed /= sectorSamples.size();
            analysis.brakingTime = brakeTime;
            analysis.throttleTime = throttleTime;
            analysis.coastingTime = coastTime;

            // Mid-corner speed (sample in middle of sector)
            int midIdx = sectorSamples.size() / 2;
            analysis.midCornerSpeed = sectorSamples[midIdx].speed;
            analysis.steeringAngle = std::abs(sectorSamples[midIdx].steering);
            analysis.gForceLateral = std::abs(sectorSamples[midIdx].gForce[0]);
        }

        sectors.append(analysis);
    }

    return sectors;
}

// ============================================================================
// Corner analysis
// ============================================================================

QVector<TelemetryAnalyzer::CornerAnalysis> TelemetryAnalyzer::analyzeCorners(
    const QVector<TelemetrySample>& lapSamples, const QVector<float>& racingLine) {

    QVector<CornerAnalysis> corners;

    if (lapSamples.size() < 10) return corners;

    // Simple corner detection based on steering and speed
    int cornerStart = -1;
    bool inCorner = false;
    int cornerNumber = 1;

    for (int i = 0; i < lapSamples.size(); ++i) {
        float steeringAngle = std::abs(lapSamples[i].steering);
        float speed = lapSamples[i].speed;

        if (steeringAngle > 0.3f && !inCorner) {
            // Entering corner
            cornerStart = i;
            inCorner = true;
        } else if (steeringAngle < 0.1f && inCorner) {
            // Exiting corner
            if (cornerStart >= 0) {
                CornerAnalysis corner;
                corner.cornerNumber = cornerNumber++;
                corner.entrySpeed = lapSamples[cornerStart].speed;

                // Find minimum speed in corner
                corner.minSpeed = lapSamples[cornerStart].speed;
                corner.apexSpeed = lapSamples[cornerStart].speed;
                int apexIdx = cornerStart;

                for (int j = cornerStart; j <= i; ++j) {
                    if (lapSamples[j].speed < corner.minSpeed) {
                        corner.minSpeed = lapSamples[j].speed;
                        corner.apexSpeed = lapSamples[j].speed;
                        apexIdx = j;
                    }
                }

                corner.exitSpeed = lapSamples[i].speed;
                corner.brakingPoint = cornerStart * 0.01f;
                corner.apexPoint = apexIdx * 0.01f;
                corner.exitPoint = i * 0.01f;
                corner.steeringAngle = lapSamples[apexIdx].steering;
                corner.throttleApplication = lapSamples[apexIdx].throttle;
                corner.brakeApplication = lapSamples[apexIdx].brake;
                corner.gForceLateral = std::abs(lapSamples[apexIdx].gForce[0]);

                corners.append(corner);
            }

            inCorner = false;
        }
    }

    return corners;
}

// ============================================================================
// Speed analysis
// ============================================================================

float TelemetryAnalyzer::getMaxSpeed(const QVector<TelemetrySample>& samples) {
    float maxSpeed = 0;
    for (const TelemetrySample& s : samples) {
        maxSpeed = std::max(maxSpeed, s.speed);
    }
    return maxSpeed;
}

float TelemetryAnalyzer::getAvgSpeed(const QVector<TelemetrySample>& samples) {
    if (samples.isEmpty()) return 0;

    float totalSpeed = 0;
    for (const TelemetrySample& s : samples) {
        totalSpeed += s.speed;
    }
    return totalSpeed / samples.size();
}

QVector<float> TelemetryAnalyzer::getSpeedTrace(const QVector<TelemetrySample>& samples) {
    QVector<float> trace;
    trace.reserve(samples.size());
    for (const TelemetrySample& s : samples) {
        trace.append(s.speed);
    }
    return trace;
}

QVector<float> TelemetryAnalyzer::getSpeedDifferential(const QVector<TelemetrySample>& lap1,
                                                        const QVector<TelemetrySample>& lap2) {
    QVector<float> diff;

    int size = std::min(lap1.size(), lap2.size());
    for (int i = 0; i < size; ++i) {
        diff.append(lap1[i].speed - lap2[i].speed);
    }

    return diff;
}

// ============================================================================
// Input analysis
// ============================================================================

QVector<float> TelemetryAnalyzer::getThrottleTrace(const QVector<TelemetrySample>& samples) {
    QVector<float> trace;
    trace.reserve(samples.size());
    for (const TelemetrySample& s : samples) {
        trace.append(s.throttle);
    }
    return trace;
}

QVector<float> TelemetryAnalyzer::getBrakeTrace(const QVector<TelemetrySample>& samples) {
    QVector<float> trace;
    trace.reserve(samples.size());
    for (const TelemetrySample& s : samples) {
        trace.append(s.brake);
    }
    return trace;
}

QVector<float> TelemetryAnalyzer::getSteeringTrace(const QVector<TelemetrySample>& samples) {
    QVector<float> trace;
    trace.reserve(samples.size());
    for (const TelemetrySample& s : samples) {
        trace.append(s.steering);
    }
    return trace;
}

float TelemetryAnalyzer::getThrottleSmoothness(const QVector<TelemetrySample>& samples) {
    if (samples.size() < 2) return 1.0f;

    float totalDelta = 0;
    for (int i = 1; i < samples.size(); ++i) {
        totalDelta += std::abs(samples[i].throttle - samples[i-1].throttle);
    }

    return 1.0f - (totalDelta / samples.size());
}

float TelemetryAnalyzer::getBrakeSmoothness(const QVector<TelemetrySample>& samples) {
    if (samples.size() < 2) return 1.0f;

    float totalDelta = 0;
    for (int i = 1; i < samples.size(); ++i) {
        totalDelta += std::abs(samples[i].brake - samples[i-1].brake);
    }

    return 1.0f - (totalDelta / samples.size());
}

// ============================================================================
// Tire analysis
// ============================================================================

QVector<float> TelemetryAnalyzer::getTireTempTrace(const QVector<TelemetrySample>& samples, int wheel) {
    QVector<float> trace;
    if (wheel < 0 || wheel > 3) return trace;

    trace.reserve(samples.size());
    for (const TelemetrySample& s : samples) {
        trace.append(s.tireTemp[wheel]);
    }
    return trace;
}

QVector<float> TelemetryAnalyzer::getTirePressureTrace(const QVector<TelemetrySample>& samples, int wheel) {
    QVector<float> trace;
    if (wheel < 0 || wheel > 3) return trace;

    trace.reserve(samples.size());
    for (const TelemetrySample& s : samples) {
        trace.append(s.tirePressure[wheel]);
    }
    return trace;
}

float TelemetryAnalyzer::getTireWearRate(const QVector<TelemetrySample>& samples, int wheel) {
    if (samples.size() < 2 || wheel < 0 || wheel > 3) return 0;

    float startWear = samples.first().tireWear[wheel];
    float endWear = samples.last().tireWear[wheel];
    float duration = samples.last().timestamp - samples.first().timestamp;

    if (duration > 0) {
        return (endWear - startWear) / duration;
    }
    return 0;
}

QVector<float> TelemetryAnalyzer::getTireSlipTrace(const QVector<TelemetrySample>& samples, int wheel) {
    QVector<float> trace;
    if (wheel < 0 || wheel > 3) return trace;

    trace.reserve(samples.size());
    for (const TelemetrySample& s : samples) {
        trace.append(s.tireSlip[wheel]);
    }
    return trace;
}

// ============================================================================
// G-force analysis
// ============================================================================

float TelemetryAnalyzer::getMaxLateralG(const QVector<TelemetrySample>& samples) {
    float maxG = 0;
    for (const TelemetrySample& s : samples) {
        maxG = std::max(maxG, std::abs(s.gForce[0]));
    }
    return maxG;
}

float TelemetryAnalyzer::getMaxLongitudinalG(const QVector<TelemetrySample>& samples) {
    float maxG = 0;
    for (const TelemetrySample& s : samples) {
        maxG = std::max(maxG, std::abs(s.gForce[2]));
    }
    return maxG;
}

QVector<float> TelemetryAnalyzer::getLateralGTrace(const QVector<TelemetrySample>& samples) {
    QVector<float> trace;
    trace.reserve(samples.size());
    for (const TelemetrySample& s : samples) {
        trace.append(s.gForce[0]);
    }
    return trace;
}

QVector<float> TelemetryAnalyzer::getLongitudinalGTrace(const QVector<TelemetrySample>& samples) {
    QVector<float> trace;
    trace.reserve(samples.size());
    for (const TelemetrySample& s : samples) {
        trace.append(s.gForce[2]);
    }
    return trace;
}

// ============================================================================
// Fuel analysis
// ============================================================================

float TelemetryAnalyzer::getFuelConsumptionRate(const QVector<TelemetrySample>& samples) {
    if (samples.size() < 2) return 0;

    float fuelUsed = samples.first().fuel - samples.last().fuel;
    float duration = samples.last().timestamp - samples.first().timestamp;

    if (duration > 0) {
        return fuelUsed / (duration / 3600.0f); // liters per hour
    }
    return 0;
}

float TelemetryAnalyzer::estimateFuelForLaps(const QVector<TelemetrySample>& samples, int laps) {
    float consumptionPerLap = getFuelConsumptionRate(samples);
    float lapTime = samples.last().lapTime - samples.first().lapTime;

    if (lapTime > 0) {
        float consumptionPerLapActual = consumptionPerLap * (lapTime / 3600.0f);
        return consumptionPerLapActual * laps;
    }
    return 0;
}

float TelemetryAnalyzer::estimateFuelForDistance(const QVector<TelemetrySample>& samples, float distance) {
    float consumptionRate = getFuelConsumptionRate(samples);
    float avgSpeed = getAvgSpeed(samples);

    if (avgSpeed > 0) {
        float timeForDistance = distance / avgSpeed * 3600.0f; // seconds
        return consumptionRate * (timeForDistance / 3600.0f);
    }
    return 0;
}

// ============================================================================
// Comparison
// ============================================================================

QJsonObject TelemetryAnalyzer::compareLaps(const LapData& lap1, const LapData& lap2) {
    QJsonObject comparison;

    comparison["lap1_number"] = lap1.lapNumber;
    comparison["lap2_number"] = lap2.lapNumber;
    comparison["lap1_time"] = lap1.lapTime;
    comparison["lap2_time"] = lap2.lapTime;
    comparison["time_difference"] = lap2.lapTime - lap1.lapTime;

    comparison["lap1_avg_speed"] = lap1.avgSpeed;
    comparison["lap2_avg_speed"] = lap2.avgSpeed;
    comparison["speed_difference"] = lap2.avgSpeed - lap1.avgSpeed;

    comparison["lap1_max_speed"] = lap1.maxSpeed;
    comparison["lap2_max_speed"] = lap2.maxSpeed;

    comparison["lap1_throttle_pct"] = lap1.avgThrottle * 100;
    comparison["lap2_throttle_pct"] = lap2.avgThrottle * 100;

    return comparison;
}

QJsonObject TelemetryAnalyzer::compareDrivers(const TelemetryReport& report1, const TelemetryReport& report2) {
    QJsonObject comparison;

    comparison["driver1"] = report1.driverName;
    comparison["driver2"] = report2.driverName;
    comparison["driver1_best_lap"] = report1.bestLapTime;
    comparison["driver2_best_lap"] = report2.bestLapTime;
    comparison["time_difference"] = report2.bestLapTime - report1.bestLapTime;

    comparison["driver1_avg_lap"] = report1.avgLapTime;
    comparison["driver2_avg_lap"] = report2.avgLapTime;
    comparison["driver1_consistency"] = report1.consistency;
    comparison["driver2_consistency"] = report2.consistency;

    return comparison;
}

// ============================================================================
// Report generation
// ============================================================================

TelemetryAnalyzer::TelemetryReport TelemetryAnalyzer::generateReport(
    const QVector<TelemetrySample>& samples, const QString& trackName, const QString& carName) {

    TelemetryReport report;
    report.trackName = trackName;
    report.carName = carName;
    report.totalLaps = samples.isEmpty() ? 0 : samples.last().lapCount;

    if (!samples.isEmpty()) {
        report.sessionTime = samples.last().timestamp - samples.first().timestamp;
    }

    // Analyze laps
    report.laps = analyzeLaps(samples);

    // Find best lap
    report.bestLapTime = 0;
    float totalTime = 0;

    for (const LapData& lap : report.laps) {
        if (lap.isValid && (report.bestLapTime == 0 || lap.lapTime < report.bestLapTime)) {
            report.bestLapTime = lap.lapTime;
        }
        totalTime += lap.lapTime;
    }

    report.avgLapTime = report.laps.isEmpty() ? 0 : totalTime / report.laps.size();

    // Calculate consistency
    if (report.laps.size() > 1) {
        float minLap = report.laps.first().lapTime;
        float maxLap = report.laps.first().lapTime;
        for (const LapData& lap : report.laps) {
            minLap = std::min(minLap, lap.lapTime);
            maxLap = std::max(maxLap, lap.lapTime);
        }
        report.consistency = (maxLap > 0) ? 1.0f - ((maxLap - minLap) / maxLap) : 1.0f;
    }

    // Improvement suggestions
    report.improvementSuggestions = analyzeBrakingPoints(samples);

    return report;
}

QJsonObject TelemetryAnalyzer::generateJsonReport(const TelemetryReport& report) {
    QJsonObject json;

    json["trackName"] = report.trackName;
    json["carName"] = report.carName;
    json["driverName"] = report.driverName;
    json["sessionTime"] = report.sessionTime;
    json["totalLaps"] = report.totalLaps;
    json["bestLapTime"] = report.bestLapTime;
    json["avgLapTime"] = report.avgLapTime;
    json["consistency"] = report.consistency;

    QJsonArray lapsArray;
    for (const LapData& lap : report.laps) {
        QJsonObject lapObj;
        lapObj["lapNumber"] = lap.lapNumber;
        lapObj["lapTime"] = lap.lapTime;
        lapObj["maxSpeed"] = lap.maxSpeed;
        lapObj["avgSpeed"] = lap.avgSpeed;
        lapObj["isValid"] = lap.isValid;
        lapsArray.append(lapObj);
    }
    json["laps"] = lapsArray;

    json["improvementSuggestions"] = report.improvementSuggestions;

    return json;
}

bool TelemetryAnalyzer::saveReport(const TelemetryReport& report, const QString& filePath) {
    QJsonObject json = generateJsonReport(report);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QJsonDocument doc(json);
    file.write(doc.toJson());
    file.close();

    return true;
}

// ============================================================================
// Improvement suggestions
// ============================================================================

QJsonObject TelemetryAnalyzer::analyzeBrakingPoints(const QVector<TelemetrySample>& samples) {
    QJsonObject suggestions;

    float maxBrake = 0;
    float avgBrake = 0;
    int brakeCount = 0;

    for (const TelemetrySample& s : samples) {
        if (s.brake > 0.1f) {
            maxBrake = std::max(maxBrake, s.brake);
            avgBrake += s.brake;
            brakeCount++;
        }
    }

    if (brakeCount > 0) {
        avgBrake /= brakeCount;
        suggestions["max_brake_pressure"] = maxBrake;
        suggestions["avg_brake_pressure"] = avgBrake;
        suggestions["brake_usage"] = brakeCount / (float)samples.size();

        if (avgBrake < 0.5f) {
            suggestions["braking_suggestion"] = "Consider braking harder - average brake pressure is low";
        }
    }

    return suggestions;
}

QJsonObject TelemetryAnalyzer::analyzeThrottleApplication(const QVector<TelemetrySample>& samples) {
    QJsonObject suggestions;

    float smoothness = getThrottleSmoothness(samples);
    suggestions["throttle_smoothness"] = smoothness;

    if (smoothness < 0.7f) {
        suggestions["throttle_suggestion"] = "Throttle application is jerky - try smoother inputs";
    }

    return suggestions;
}

QJsonObject TelemetryAnalyzer::analyzeRacingLine(const QVector<TelemetrySample>& samples) {
    QJsonObject suggestions;

    // Analyze speed through corners
    QVector<CornerAnalysis> corners = analyzeCorners(samples, QVector<float>());

    float avgCornerEfficiency = 0;
    for (const CornerAnalysis& corner : corners) {
        float efficiency = corner.apexSpeed / corner.entrySpeed;
        avgCornerEfficiency += efficiency;
    }

    if (!corners.isEmpty()) {
        avgCornerEfficiency /= corners.size();
        suggestions["corner_efficiency"] = avgCornerEfficiency;

        if (avgCornerEfficiency < 0.6f) {
            suggestions["line_suggestion"] = "Corner exit speeds are low - try later apexes";
        }
    }

    return suggestions;
}

QJsonObject TelemetryAnalyzer::analyzeGearSelection(const QVector<TelemetrySample>& samples) {
    QJsonObject suggestions;

    // Analyze RPM at shift points
    float avgShiftRPM = 0;
    int shiftCount = 0;

    for (int i = 1; i < samples.size(); ++i) {
        if (samples[i].gear > samples[i-1].gear) {
            avgShiftRPM += samples[i-1].rpm;
            shiftCount++;
        }
    }

    if (shiftCount > 0) {
        avgShiftRPM /= shiftCount;
        suggestions["avg_shift_rpm"] = avgShiftRPM;
    }

    return suggestions;
}

QJsonObject TelemetryAnalyzer::analyzeTireManagement(const QVector<TelemetrySample>& samples) {
    QJsonObject suggestions;

    for (int wheel = 0; wheel < 4; ++wheel) {
        float wearRate = getTireWearRate(samples, wheel);
        QString key = "tire_" + QString::number(wheel) + "_wear_rate";
        suggestions[key] = wearRate;
    }

    return suggestions;
}

// ============================================================================
// Utility
// ============================================================================

QVector<TelemetryAnalyzer::TelemetrySample> TelemetryAnalyzer::interpolateSamples(
    const QVector<TelemetrySample>& samples, float targetHz) {

    if (samples.size() < 2) return samples;

    QVector<TelemetrySample> result;
    float startTime = samples.first().timestamp;
    float endTime = samples.last().timestamp;
    float step = 1.0f / targetHz;

    for (float t = startTime; t <= endTime; t += step) {
        // Find surrounding samples
        int idx = 0;
        for (int i = 0; i < samples.size() - 1; ++i) {
            if (samples[i].timestamp <= t && samples[i+1].timestamp >= t) {
                idx = i;
                break;
            }
        }

        float fraction = (t - samples[idx].timestamp) /
                        (samples[idx+1].timestamp - samples[idx].timestamp);

        TelemetrySample interpolated;
        interpolated.timestamp = t;
        interpolated.speed = samples[idx].speed + (samples[idx+1].speed - samples[idx].speed) * fraction;
        interpolated.throttle = samples[idx].throttle + (samples[idx+1].throttle - samples[idx].throttle) * fraction;
        interpolated.brake = samples[idx].brake + (samples[idx+1].brake - samples[idx].brake) * fraction;
        interpolated.steering = samples[idx].steering + (samples[idx+1].steering - samples[idx].steering) * fraction;

        result.append(interpolated);
    }

    return result;
}

QVector<TelemetryAnalyzer::TelemetrySample> TelemetryAnalyzer::filterSamples(
    const QVector<TelemetrySample>& samples, float startTime, float endTime) {

    QVector<TelemetrySample> filtered;
    for (const TelemetrySample& s : samples) {
        if (s.timestamp >= startTime && s.timestamp <= endTime) {
            filtered.append(s);
        }
    }
    return filtered;
}

float TelemetryAnalyzer::calculateCorrelation(const QVector<float>& x, const QVector<float>& y) {
    int n = std::min(x.size(), y.size());
    if (n < 2) return 0;

    float sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0, sumY2 = 0;

    for (int i = 0; i < n; ++i) {
        sumX += x[i];
        sumY += y[i];
        sumXY += x[i] * y[i];
        sumX2 += x[i] * x[i];
        sumY2 += y[i] * y[i];
    }

    float denom = std::sqrt((n * sumX2 - sumX * sumX) * (n * sumY2 - sumY * sumY));
    if (denom == 0) return 0;

    return (n * sumXY - sumX * sumY) / denom;
}

// ============================================================================
// Private helpers
// ============================================================================

int TelemetryAnalyzer::findLapStart(const QVector<TelemetrySample>& samples, int startIndex) {
    for (int i = startIndex; i < samples.size(); ++i) {
        if (i == 0 || samples[i].lapCount != samples[i-1].lapCount) {
            return i;
        }
    }
    return startIndex;
}

int TelemetryAnalyzer::findLapEnd(const QVector<TelemetrySample>& samples, int startIndex) {
    int lapNum = samples[startIndex].lapCount;
    for (int i = startIndex + 1; i < samples.size(); ++i) {
        if (samples[i].lapCount != lapNum) {
            return i - 1;
        }
    }
    return samples.size() - 1;
}

float TelemetryAnalyzer::calculateCornerSpeed(const QVector<TelemetrySample>& cornerSamples) {
    if (cornerSamples.isEmpty()) return 0;

    float minSpeed = cornerSamples.first().speed;
    for (const TelemetrySample& s : cornerSamples) {
        minSpeed = std::min(minSpeed, s.speed);
    }
    return minSpeed;
}

float TelemetryAnalyzer::calculateBrakingDistance(const QVector<TelemetrySample>& brakeSamples) {
    if (brakeSamples.size() < 2) return 0;

    float distance = 0;
    for (int i = 1; i < brakeSamples.size(); ++i) {
        float dt = brakeSamples[i].timestamp - brakeSamples[i-1].timestamp;
        float avgSpeed = (brakeSamples[i].speed + brakeSamples[i-1].speed) / 2.0f / 3.6f;
        distance += avgSpeed * dt;
    }
    return distance;
}

// ============================================================================
// Consistency & performance metrics
// ============================================================================

float TelemetryAnalyzer::calculateConsistency(const QVector<LapData>& laps) {
    if (laps.size() < 2) return 100.0f;

    float sum = 0, count = 0;
    for (int i = 1; i < laps.size(); ++i) {
        float diff = std::abs(laps[i].lapTime - laps[i-1].lapTime);
        sum += diff;
        count += 1.0f;
    }
    float avgDelta = sum / count;

    float consistency = 100.0f - (avgDelta * 33.0f);
    return std::max(0.0f, std::min(100.0f, consistency));
}

float TelemetryAnalyzer::calculateLapTimeVariance(const QVector<LapData>& laps) {
    if (laps.size() < 2) return 0;

    float mean = 0;
    for (const LapData& lap : laps) mean += lap.lapTime;
    mean /= static_cast<float>(laps.size());

    float variance = 0;
    for (const LapData& lap : laps) {
        float dev = lap.lapTime - mean;
        variance += dev * dev;
    }
    return variance / static_cast<float>(laps.size());
}

float TelemetryAnalyzer::calculateDrivingScore(const QVector<TelemetrySample>& samples) {
    if (samples.isEmpty()) return 0;

    float smoothness = 0;
    int n = 0;
    for (int i = 1; i < samples.size(); ++i) {
        float dThrottle = std::abs(samples[i].throttle - samples[i-1].throttle);
        float dBrake = std::abs(samples[i].brake - samples[i-1].brake);
        float dSteering = std::abs(samples[i].steering - samples[i-1].steering);

        float score = 0;
        score += std::max(0.0f, 1.0f - dThrottle * 5.0f);
        score += std::max(0.0f, 1.0f - dBrake * 5.0f);
        score += std::max(0.0f, 1.0f - dSteering * 10.0f);
        smoothness += score / 3.0f;
        n++;
    }

    float avgScore = n > 0 ? smoothness / n : 0;

    float avgSpeed = 0, maxSpeed = 0;
    for (const TelemetrySample& s : samples) {
        avgSpeed += s.speed;
        maxSpeed = std::max(maxSpeed, s.speed);
    }
    avgSpeed /= samples.size();

    float speedRatio = maxSpeed > 0 ? avgSpeed / maxSpeed : 0;
    avgScore += speedRatio * 0.2f;

    return std::max(0.0f, std::min(100.0f, avgScore * 85.0f));
}

// ============================================================================
// Braking point analysis
// ============================================================================

QVector<TelemetryAnalyzer::BrakingEvent> TelemetryAnalyzer::detectBrakingPoints(const QVector<TelemetrySample>& samples) {
    QVector<BrakingEvent> events;
    if (samples.size() < 10) return events;

    int cornerCount = 0;
    int i = 1;
    while (i < samples.size()) {
        if (samples[i].brake > 0.05f && samples[i].speed < samples[i-1].speed) {
            int brakeStart = i;
            float entrySpeed = samples[i].speed;
            float maxBrake = samples[i].brake;
            float totalBrakePressure = 0;
            int brakeSamples = 0;
            float minSpeed = samples[i].speed;
            int brakeEnd = i;

            while (i < samples.size() && samples[i].brake > 0.02f) {
                minSpeed = std::min(minSpeed, samples[i].speed);
                maxBrake = std::max(maxBrake, samples[i].brake);
                totalBrakePressure += samples[i].brake;
                brakeSamples++;
                brakeEnd = i;
                i++;
            }

            int exitIdx = i;
            while (exitIdx < samples.size() && samples[exitIdx].speed < minSpeed + 5.0f) {
                exitIdx++;
            }
            float exitSpeed = exitIdx < samples.size() ? samples[exitIdx].speed : minSpeed;

            float distance = 0;
            for (int j = brakeStart + 1; j < brakeEnd && j < samples.size(); ++j) {
                float dt = samples[j].timestamp - samples[j-1].timestamp;
                float avgSpeedMs = (samples[j].speed + samples[j-1].speed) / 2.0f / 3.6f;
                distance += avgSpeedMs * dt;
            }

            BrakingEvent event;
            event.timestamp = samples[brakeStart].timestamp;
            event.entrySpeed = entrySpeed;
            event.exitSpeed = exitSpeed;
            event.minSpeed = minSpeed;
            event.brakingDistance = distance;
            event.brakePressure = brakeSamples > 0 ? totalBrakePressure / brakeSamples : 0;
            event.steeringAngle = samples[brakeEnd < samples.size() ? brakeEnd : brakeEnd - 1].steering;
            event.cornerNumber = ++cornerCount;

            events.append(event);
        }
        i++;
    }

    return events;
}

float TelemetryAnalyzer::calculateBrakingEfficiency(const BrakingEvent& event) {
    if (event.entrySpeed <= 0) return 0;

    float speedDrop = event.entrySpeed - event.minSpeed;
    float decelRate = speedDrop / std::max(0.1f, event.brakingDistance);
    float idealDecel = 12.0f;
    float efficiency = decelRate / idealDecel;

    float speedRatio = event.minSpeed / std::max(1.0f, event.entrySpeed);
    if (speedRatio < 0.3f) efficiency *= 0.9f;

    return std::max(0.0f, std::min(1.0f, efficiency));
}

float TelemetryAnalyzer::calculateBrakeFadeRisk(const QVector<BrakingEvent>& events) {
    if (events.size() < 3) return 0;

    float avgPressure = 0, maxPressure = 0;
    for (const BrakingEvent& event : events) {
        avgPressure += event.brakePressure;
        maxPressure = std::max(maxPressure, event.brakePressure);
    }
    avgPressure /= events.size();

    float pressureRisk = avgPressure * 0.4f;
    float peakRisk = maxPressure * 0.3f;
    float eventRisk = std::min(1.0f, events.size() / 30.0f) * 0.3f;

    return std::max(0.0f, std::min(1.0f, pressureRisk + peakRisk + eventRisk));
}

// ============================================================================
// Apex analysis
// ============================================================================

TelemetryAnalyzer::ApexData TelemetryAnalyzer::identifyApex(const QVector<TelemetrySample>& cornerSamples) {
    ApexData apex;

    if (cornerSamples.isEmpty()) return apex;

    int apexIdx = 0, maxSteerIdx = 0;
    float minSpeed = cornerSamples[0].speed;
    float maxSteer = std::abs(cornerSamples[0].steering);

    for (int i = 0; i < cornerSamples.size(); ++i) {
        if (cornerSamples[i].speed < minSpeed) {
            minSpeed = cornerSamples[i].speed;
            apexIdx = i;
        }
        float steer = std::abs(cornerSamples[i].steering);
        if (steer > maxSteer) {
            maxSteer = steer;
            maxSteerIdx = i;
        }
    }

    int finalIdx = (apexIdx + maxSteerIdx) / 2;
    const TelemetrySample& s = cornerSamples[finalIdx];

    apex.speed = s.speed;
    apex.steeringAngle = s.steering;
    apex.gForceLateral = std::abs(s.gForce[0]);
    apex.throttleAtApex = s.throttle;
    apex.brakeAtApex = s.brake;
    apex.sectorTime = cornerSamples.last().timestamp - cornerSamples.first().timestamp;

    return apex;
}

float TelemetryAnalyzer::calculateApexEfficiency(const ApexData& apex, float cornerRadius) {
    if (cornerRadius <= 0 || apex.speed <= 0) return 0;

    float theoreticalMaxSpeed = std::sqrt(1.3f * 9.81f * cornerRadius) * 3.6f;
    float speedEfficiency = apex.speed / theoreticalMaxSpeed;
    float throttlePenalty = std::max(0.0f, (apex.throttleAtApex - 0.3f) * 1.5f);

    float efficiency = speedEfficiency * (1.0f - throttlePenalty);
    return std::max(0.0f, std::min(1.0f, efficiency));
}
