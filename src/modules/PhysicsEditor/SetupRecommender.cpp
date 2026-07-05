#include "SetupRecommender.h"

#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <QFile>
#include <QIODevice>

namespace ks {

SetupRecommender::SetupRecommender(QObject* parent)
    : QObject(parent)
{
}

SetupRecommender::~SetupRecommender()
{
}

void SetupRecommender::setTrackCharacteristics(const TrackCharacteristics& track)
{
    m_track = track;
}

void SetupRecommender::setCarProperties(const CarProperties& car)
{
    m_car = car;
}

SetupRecommender::TrackCategory SetupRecommender::classifyTrack() const
{
    float cornerRatio = static_cast<float>(m_track.numCorners) / std::max(1.0f, m_track.totalLengthMeters / 100.0f);

    if (m_track.numHighSpeedCorners > m_track.numCorners * 0.5f) {
        return Track_HighSpeed;
    } else if (m_track.numLowSpeedCorners > m_track.numCorners * 0.5f) {
        return Track_Techical;
    } else if (cornerRatio > 2.0f) {
        return Track_Techical;
    } else if (m_track.layout == TrackCharacteristics::Street) {
        return Track_Street;
    } else {
        return Track_Mixed;
    }
}

SetupRecommender::SetupRecommendation SetupRecommender::generateRecommendation()
{
    SetupRecommendation rec;

    analyzeSuspension(rec);
    analyzeBrakes(rec);
    analyzeAero(rec);
    analyzeTransmission(rec);
    analyzeTyres(rec);

    TrackCategory category = classifyTrack();
    switch (category) {
        case Track_HighSpeed:
            rec.confidence = 0.85f;
            rec.reasoning = "High-speed track: prioritize stability and top speed";
            break;
        case Track_Techical:
            rec.confidence = 0.80f;
            rec.reasoning = "Technical track: prioritize turn-in and traction";
            break;
        case Track_Street:
            rec.confidence = 0.75f;
            rec.reasoning = "Street track: conservative setup for safety";
            break;
        default:
            rec.confidence = 0.80f;
            rec.reasoning = "Mixed track: balanced setup for all conditions";
            break;
    }

    emit recommendationGenerated(rec);
    return rec;
}

void SetupRecommender::analyzeSuspension(SetupRecommendation& rec)
{
    float baseSpringRate = 100.0f;

    float speedFactor = m_car.powerKw / 450.0f;
    baseSpringRate *= (1.0f + (speedFactor - 1.0f) * 0.5f);

    if (m_track.avgCornerRadius < 30.0f) {
        baseSpringRate *= 1.2f;
    } else if (m_track.avgCornerRadius > 80.0f) {
        baseSpringRate *= 0.9f;
    }

    rec.suspension.frontSpringRate = baseSpringRate * 1.1f;
    rec.suspension.rearSpringRate = baseSpringRate;

    rec.suspension.frontDampingCompression = 3.0f + (m_track.numLowSpeedCorners * 0.1f);
    rec.suspension.frontDampingRebound = 5.0f;
    rec.suspension.rearDampingCompression = 3.0f;
    rec.suspension.rearDampingRebound = 5.0f + (m_track.numHighSpeedCorners * 0.1f);

    if (m_car.driveType == CarProperties::RWD) {
        rec.suspension.rearSpringRate *= 0.95f;
        rec.suspension.rearDampingRebound *= 1.1f;
    } else if (m_car.driveType == CarProperties::FWD) {
        rec.suspension.frontSpringRate *= 1.1f;
        rec.suspension.frontDampingCompression *= 1.1f;
    }

    rec.suspension.frontCamber = -2.5f;
    rec.suspension.rearCamber = -2.0f;

    if (m_track.numHighSpeedCorners > 3) {
        rec.suspension.frontCamber = -3.0f;
        rec.suspension.rearCamber = -2.5f;
    }

    rec.suspension.frontARB = 2.0f + (m_track.numLowSpeedCorners * 0.1f);
    rec.suspension.rearARB = 1.5f + (m_track.numHighSpeedCorners * 0.05f);
}

void SetupRecommender::analyzeBrakes(SetupRecommendation& rec)
{
    float brakingEnergy = m_car.powerKw * 0.5f + (m_track.numCorners * 10.0f);

    rec.brakes.frontBias = 0.60f;
    if (m_car.enginePosition == CarProperties::Front) {
        rec.brakes.frontBias = 0.63f;
    } else if (m_car.enginePosition == CarProperties::Rear) {
        rec.brakes.frontBias = 0.58f;
    }

    rec.brakes.frontPressure = 120.0f + (brakingEnergy * 0.3f);
    rec.brakes.rearPressure = rec.brakes.frontPressure * (1.0f - rec.brakes.frontBias) / rec.brakes.frontBias;

    if (m_car.driveType == CarProperties::RWD) {
        rec.brakes.frontPressure *= 1.05f;
    }
}

void SetupRecommender::analyzeAero(SetupRecommendation& rec)
{
    float requiredDownforce = 0.0f;
    float requiredDrag = 0.0f;

    for (int i = 0; i < m_track.numHighSpeedCorners; ++i) {
        requiredDownforce += 10.0f;
    }

    float avgCornerSpeed = (m_track.topSpeedKmh * 0.5f) / 3.6f;
    float cornerG = calculateCornerG(avgCornerSpeed, m_track.avgCornerRadius);

    float missingG = 1.5f - cornerG;
    if (missingG > 0) {
        requiredDownforce += missingG * 500.0f;
    }

    rec.aero.rearWing = requiredDownforce * 0.01f;
    rec.aero.frontWing = rec.aero.rearWing * 0.6f;

    if (m_car.driveType == CarProperties::RWD) {
        rec.aero.rearWing *= 1.1f;
    }

    rec.aero.frontSplitter = rec.aero.frontWing * 0.3f;
    rec.aero.rearDiffuser = rec.aero.rearWing * 0.2f;

    rec.aero.diffClutch = 50;
    rec.aero.diffPower = 60;
    rec.aero.diffCoast = 40;

    if (m_car.driveType == CarProperties::RWD) {
        rec.aero.diffPower = 70;
        rec.aero.diffCoast = 30;
    }
}

void SetupRecommender::analyzeTransmission(SetupRecommendation& rec)
{
    float topSpeed = m_track.topSpeedKmh / 3.6f;
    float maxRpm = m_car.maxRpm;
    float wheelRadius = 0.33f;
    float tireCircumference = 2.0f * M_PI * wheelRadius;

    float maxWheelRpm = (topSpeed / tireCircumference) * 60.0f;
    float gearRatio = maxRpm / maxWheelRpm;

    rec.transmission.finalDrive = gearRatio * 3.8f;

    float baseRatio = 2.5f;
    rec.transmission.gear1 = baseRatio * 3.5f;
    rec.transmission.gear2 = baseRatio * 2.5f;
    rec.transmission.gear3 = baseRatio * 1.8f;
    rec.transmission.gear4 = baseRatio * 1.4f;
    rec.transmission.gear5 = baseRatio * 1.1f;
    rec.transmission.gear6 = baseRatio * 0.9f;

    if (m_car.powerKw > 500.0f) {
        for (int i = 1; i <= 6; ++i) {
            switch (i) {
                case 1: rec.transmission.gear1 *= 0.95f; break;
                case 2: rec.transmission.gear2 *= 0.95f; break;
                case 3: rec.transmission.gear3 *= 0.95f; break;
                case 4: rec.transmission.gear4 *= 0.95f; break;
                case 5: rec.transmission.gear5 *= 0.95f; break;
                case 6: rec.transmission.gear6 *= 0.95f; break;
            }
        }
    }
}

void SetupRecommender::analyzeTyres(SetupRecommendation& rec)
{
    rec.tyres.frontPressure = 2.2f;
    rec.tyres.rearPressure = 2.1f;

    if (m_track.surface == TrackCharacteristics::Concrete) {
        rec.tyres.frontPressure = 2.4f;
        rec.tyres.rearPressure = 2.3f;
    }

    float trackTemp = 25.0f;
    if (trackTemp > 30.0f) {
        rec.tyres.frontPressure *= 1.05f;
        rec.tyres.rearPressure *= 1.05f;
    } else if (trackTemp < 20.0f) {
        rec.tyres.frontPressure *= 0.95f;
        rec.tyres.rearPressure *= 0.95f;
    }

    rec.tyres.frontCompound = 0;
    rec.tyres.rearCompound = 0;

    if (m_track.numCorners > 15) {
        rec.tyres.frontCompound = 1;
        rec.tyres.rearCompound = 1;
    }
}

float SetupRecommender::calculateCornerG(float speed, float radius)
{
    if (radius <= 0.0f) return 0.0f;
    float v2 = speed * speed;
    return v2 / (9.81f * radius);
}

float SetupRecommender::calculateDownforce(float speed, float aoa)
{
    float dynamicPressure = 0.5f * 1.225f * speed * speed;
    float cl = m_car.downforceCoeff * sinf(aoa * M_PI / 180.0f);
    return cl * dynamicPressure * m_car.frontalArea;
}

float SetupRecommender::calculateBrakingDistance(float speed, float deceleration)
{
    if (deceleration <= 0.0f) return 1000.0f;
    return (speed * speed) / (2.0f * deceleration);
}

float SetupRecommender::calculateCornerEntrySpeed(float radius, float g)
{
    return sqrtf(g * 9.81f * radius);
}

QJsonObject SetupRecommender::recommendationToJson(const SetupRecommendation& rec) const
{
    QJsonObject obj;

    QJsonObject susp;
    susp["frontSpringRate"] = rec.suspension.frontSpringRate;
    susp["rearSpringRate"] = rec.suspension.rearSpringRate;
    susp["frontDampingCompression"] = rec.suspension.frontDampingCompression;
    susp["frontDampingRebound"] = rec.suspension.frontDampingRebound;
    susp["rearDampingCompression"] = rec.suspension.rearDampingCompression;
    susp["rearDampingRebound"] = rec.suspension.rearDampingRebound;
    susp["frontCamber"] = rec.suspension.frontCamber;
    susp["rearCamber"] = rec.suspension.rearCamber;
    obj["suspension"] = susp;

    QJsonObject brakes;
    brakes["frontBias"] = rec.brakes.frontBias;
    brakes["frontPressure"] = rec.brakes.frontPressure;
    brakes["rearPressure"] = rec.brakes.rearPressure;
    obj["brakes"] = brakes;

    QJsonObject aero;
    aero["frontWing"] = rec.aero.frontWing;
    aero["rearWing"] = rec.aero.rearWing;
    obj["aero"] = aero;

    QJsonObject trans;
    trans["finalDrive"] = rec.transmission.finalDrive;
    trans["gear1"] = rec.transmission.gear1;
    trans["gear2"] = rec.transmission.gear2;
    obj["transmission"] = trans;

    QJsonObject tyres;
    tyres["frontPressure"] = rec.tyres.frontPressure;
    tyres["rearPressure"] = rec.tyres.rearPressure;
    obj["tyres"] = tyres;

    obj["confidence"] = rec.confidence;
    obj["reasoning"] = rec.reasoning;

    return obj;
}

SetupRecommender::SetupRecommendation SetupRecommender::jsonToRecommendation(const QJsonObject& obj) const
{
    SetupRecommendation rec;

    QJsonObject susp = obj["suspension"].toObject();
    rec.suspension.frontSpringRate = susp["frontSpringRate"].toDouble(100.0f);
    rec.suspension.rearSpringRate = susp["rearSpringRate"].toDouble(90.0f);
    rec.suspension.frontCamber = susp["frontCamber"].toDouble(-2.5f);
    rec.suspension.rearCamber = susp["rearCamber"].toDouble(-2.0f);

    QJsonObject brakes = obj["brakes"].toObject();
    rec.brakes.frontBias = brakes["frontBias"].toDouble(0.6f);

    QJsonObject aero = obj["aero"].toObject();
    rec.aero.frontWing = aero["frontWing"].toDouble(0.0f);
    rec.aero.rearWing = aero["rearWing"].toDouble(0.0f);

    rec.confidence = obj["confidence"].toDouble(0.8f);
    rec.reasoning = obj["reasoning"].toString("");

    return rec;
}

QStringList SetupRecommender::getSetupCategories() const
{
    return { "Suspension", "Brakes", "Aerodynamics", "Transmission", "Tyres" };
}

QString SetupRecommender::getSetupDescription(const SetupRecommendation& rec) const
{
    QString desc;
    desc += QString("Front Camber: %1 deg\n").arg(rec.suspension.frontCamber, 0, 'f', 1);
    desc += QString("Rear Camber: %1 deg\n").arg(rec.suspension.rearCamber, 0, 'f', 1);
    desc += QString("Front Spring: %1 N/mm\n").arg(rec.suspension.frontSpringRate, 0, 'f', 0);
    desc += QString("Rear Spring: %1 N/mm\n").arg(rec.suspension.rearSpringRate, 0, 'f', 0);
    desc += QString("Front Wing: %1\n").arg(rec.aero.frontWing, 0, 'f', 1);
    desc += QString("Rear Wing: %1\n").arg(rec.aero.rearWing, 0, 'f', 1);
    desc += QString("Brake Bias: %1%\n").arg(rec.brakes.frontBias * 100.0f, 0, 'f', 0);
    desc += QString("Confidence: %1%\n").arg(rec.confidence * 100.0f, 0, 'f', 0);
    desc += "\n" + rec.reasoning;
    return desc;
}

SetupOptimizer::SetupOptimizer(QObject* parent)
    : QObject(parent)
{
}

SetupOptimizer::~SetupOptimizer()
{
}

void SetupOptimizer::setRecommender(SetupRecommender* recommender)
{
    m_recommender = recommender;
}

void SetupOptimizer::setTargetLapTime(float seconds)
{
    m_targetLapTime = seconds;
}

void SetupOptimizer::setWeatherCondition(int temperature, float humidity)
{
    m_temperature = temperature;
    m_humidity = humidity;
}

static float randomFloat(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

void SetupOptimizer::optimizeIteration()
{
    if (!m_recommender) return;

    if (m_iterations == 0) {
        m_bestSetup = m_recommender->generateRecommendation();
        m_bestScore = evaluateSetup(m_bestSetup);
        m_iterations++;
        float lapTime = calculateLapTime(m_bestSetup);
        emit iterationComplete(m_iterations, lapTime, m_bestScore);
        return;
    }

    auto candidate = m_bestSetup;

    float scale = std::max(0.05f, 1.0f - m_iterations / 200.0f);

    candidate.suspension.frontSpringRate += randomFloat(-15, 15) * scale;
    candidate.suspension.rearSpringRate  += randomFloat(-15, 15) * scale;
    candidate.suspension.frontDampingCompression += randomFloat(-1, 1) * scale;
    candidate.suspension.frontDampingRebound    += randomFloat(-1, 1) * scale;
    candidate.suspension.rearDampingCompression += randomFloat(-1, 1) * scale;
    candidate.suspension.rearDampingRebound    += randomFloat(-1, 1) * scale;
    candidate.suspension.frontRideHeight += randomFloat(-0.005f, 0.005f) * scale;
    candidate.suspension.rearRideHeight  += randomFloat(-0.005f, 0.005f) * scale;
    candidate.suspension.frontCamber += randomFloat(-0.3f, 0.3f) * scale;
    candidate.suspension.rearCamber  += randomFloat(-0.3f, 0.3f) * scale;
    candidate.suspension.frontARB += randomFloat(-0.5f, 0.5f) * scale;
    candidate.suspension.rearARB  += randomFloat(-0.5f, 0.5f) * scale;
    candidate.suspension.frontToe += randomFloat(-0.05f, 0.05f) * scale;
    candidate.suspension.rearToe  += randomFloat(-0.05f, 0.05f) * scale;
    candidate.brakes.frontBias   += randomFloat(-0.02f, 0.02f) * scale;
    candidate.brakes.frontPressure += randomFloat(-10, 10) * scale;
    candidate.aero.frontWing += randomFloat(-1, 1) * scale;
    candidate.aero.rearWing  += randomFloat(-1, 1) * scale;
    candidate.aero.diffClutch += static_cast<int>(randomFloat(-5, 5) * scale);
    candidate.aero.diffPower  += static_cast<int>(randomFloat(-5, 5) * scale);
    candidate.aero.diffCoast  += static_cast<int>(randomFloat(-5, 5) * scale);
    candidate.tyres.frontPressure += randomFloat(-0.1f, 0.1f) * scale;
    candidate.tyres.rearPressure  += randomFloat(-0.1f, 0.1f) * scale;

    candidate.suspension.frontSpringRate = std::clamp(candidate.suspension.frontSpringRate, 40.0f, 250.0f);
    candidate.suspension.rearSpringRate  = std::clamp(candidate.suspension.rearSpringRate, 40.0f, 250.0f);
    candidate.suspension.frontRideHeight = std::clamp(candidate.suspension.frontRideHeight, 0.02f, 0.20f);
    candidate.suspension.rearRideHeight  = std::clamp(candidate.suspension.rearRideHeight, 0.02f, 0.20f);
    candidate.suspension.frontCamber = std::clamp(candidate.suspension.frontCamber, -5.0f, 0.0f);
    candidate.suspension.rearCamber  = std::clamp(candidate.suspension.rearCamber, -5.0f, 0.0f);
    candidate.brakes.frontBias = std::clamp(candidate.brakes.frontBias, 0.45f, 0.75f);
    candidate.tyres.frontPressure = std::clamp(candidate.tyres.frontPressure, 1.5f, 3.5f);
    candidate.tyres.rearPressure  = std::clamp(candidate.tyres.rearPressure, 1.5f, 3.5f);

    float score = evaluateSetup(candidate);
    float lapTime = calculateLapTime(candidate);

    if (score > m_bestScore) {
        m_bestScore = score;
        m_bestSetup = candidate;
    }

    m_iterations++;

    emit iterationComplete(m_iterations, lapTime, score);

    if (m_iterations >= 200 || score > 0.97f) {
        emit optimizationComplete(m_bestSetup);
    }
}

float SetupOptimizer::calculateLapTime(const SetupRecommender::SetupRecommendation& setup)
{
    float baseTime = 90.0f;

    float gripFactor = 1.0f;
    gripFactor += (setup.suspension.frontCamber + 3.0f) * 0.008f;
    gripFactor += (setup.suspension.rearCamber + 2.5f) * 0.008f;
    gripFactor += setup.aero.rearWing * 0.003f;
    gripFactor += (0.5f - std::abs(setup.tyres.frontPressure - 2.2f) * 0.2f) * 0.02f;
    gripFactor += (0.5f - std::abs(setup.tyres.rearPressure  - 2.1f) * 0.2f) * 0.02f;

    float rideHeightPenalty = 0.0f;
    if (setup.suspension.frontRideHeight > 0.12f) rideHeightPenalty += 0.5f;
    if (setup.suspension.rearRideHeight  > 0.12f) rideHeightPenalty += 0.5f;

    float aeroDrag = (setup.aero.frontWing + setup.aero.rearWing) * 0.08f;

    float springBalance = std::abs(setup.suspension.frontSpringRate -
                                   setup.suspension.rearSpringRate) / 150.0f * 0.5f;

    return baseTime * (2.0f - gripFactor) + rideHeightPenalty + aeroDrag + springBalance;
}

float SetupOptimizer::evaluateSetup(const SetupRecommender::SetupRecommendation& setup)
{
    float score = setup.confidence * 0.25f;

    float camberScore = 1.0f - std::abs(setup.suspension.frontCamber + 2.8f) / 5.0f;
    camberScore += 1.0f - std::abs(setup.suspension.rearCamber + 2.3f) / 5.0f;
    camberScore *= 0.1f;

    float springScore = 1.0f - std::abs(setup.suspension.frontSpringRate - 100.0f) / 150.0f;
    springScore += 1.0f - std::abs(setup.suspension.rearSpringRate - 90.0f) / 150.0f;
    springScore *= 0.1f;

    float aeroScore = 1.0f - std::abs(setup.aero.rearWing - 5.0f) / 15.0f;
    aeroScore *= 0.1f;

    float brakeScore = 1.0f - std::abs(setup.brakes.frontBias - 0.60f) / 0.30f;
    brakeScore *= 0.1f;

    float tyreScore = 1.0f - std::abs(setup.tyres.frontPressure - 2.2f) / 1.5f;
    tyreScore += 1.0f - std::abs(setup.tyres.rearPressure - 2.1f) / 1.5f;
    tyreScore *= 0.05f;

    score += camberScore + springScore + aeroScore + brakeScore + tyreScore;

    float lapTime = calculateLapTime(setup);
    float timeScore = 1.0f - std::abs(lapTime - 90.0f) / 30.0f;
    score += timeScore * 0.3f;

    return std::clamp(score, 0.0f, 1.0f);
}

void SetupOptimizer::saveToFile(const QString& path) const
{
    if (!m_recommender) return;

    auto rec = m_bestSetup;
    QJsonObject obj = m_recommender->recommendationToJson(rec);

    QJsonDocument doc(obj);
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}

void SetupOptimizer::loadFromFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    m_bestSetup = m_recommender->jsonToRecommendation(doc.object());
}

TelemetryAnalyzer::TelemetryAnalyzer(QObject* parent)
    : QObject(parent)
{
}

TelemetryAnalyzer::~TelemetryAnalyzer()
{
}

void TelemetryAnalyzer::loadTelemetry(const QVector<TelemetryPoint>& points)
{
    m_currentLap = points;
}

void TelemetryAnalyzer::addLap(const QVector<TelemetryPoint>& points)
{
    loadTelemetry(points);
    LapAnalysis analysis = analyzeCurrentLap();
    m_laps.append(analysis);
    m_currentLapIndex = m_laps.size() - 1;
}

TelemetryAnalyzer::LapAnalysis TelemetryAnalyzer::analyzeCurrentLap()
{
    LapAnalysis analysis;

    if (m_currentLap.isEmpty()) return analysis;

    analysis.lapTime = m_currentLap.last().time - m_currentLap.first().time;

    float totalSpeed = 0.0f;
    for (const auto& point : m_currentLap) {
        totalSpeed += point.speed;
        if (point.speed > analysis.topSpeed) {
            analysis.topSpeed = point.speed;
        }
        if (std::abs(point.latG) > analysis.maxLateralG) {
            analysis.maxLateralG = std::abs(point.latG);
        }
        if (std::abs(point.longG) > analysis.maxLongitudinalG) {
            analysis.maxLongitudinalG = std::abs(point.longG);
        }
    }
    analysis.avgSpeed = totalSpeed / m_currentLap.size();

    emit analysisComplete(analysis);
    return analysis;
}

QVector<TelemetryAnalyzer::LapAnalysis> TelemetryAnalyzer::analyzeAllLaps()
{
    QVector<LapAnalysis> results;
    for (const auto& lap : m_laps) {
        if (lap.lapTime > 0) {
            results.append(lap);
        }
    }
    return results;
}

TelemetryAnalyzer::LapAnalysis TelemetryAnalyzer::findFastestLap() const
{
    LapAnalysis fastest;
    float minTime = 1e9f;

    for (const auto& lap : m_laps) {
        if (lap.lapTime > 0 && lap.lapTime < minTime) {
            minTime = lap.lapTime;
            fastest = lap;
        }
    }
    return fastest;
}

TelemetryAnalyzer::LapAnalysis TelemetryAnalyzer::findConsistentLap() const
{
    if (m_laps.size() < 2) return LapAnalysis();

    float minStdDev = 1e9f;
    LapAnalysis mostConsistent;

    for (const auto& lap : m_laps) {
        float mean = lap.lapTime;
        float variance = 0.0f;
        for (const auto& l : m_laps) {
            variance += (l.lapTime - mean) * (l.lapTime - mean);
        }
        variance /= m_laps.size();

        if (variance < minStdDev) {
            minStdDev = variance;
            mostConsistent = lap;
        }
    }
    return mostConsistent;
}

void TelemetryAnalyzer::generateSetupSuggestions(SetupRecommender::SetupRecommendation& rec)
{
    if (m_currentLap.isEmpty()) return;

    LapAnalysis analysis = analyzeCurrentLap();

    float thresholdSpeedMs = 250.0f / 3.6f;
    float thresholdBrakeForce = 0.8f;

    int understeerEvents = 0;
    int cornerCount = 0;

    float totalLatG = 0;
    float totalBrakePressure = 0, totalSteerAngle = 0;
    int brakeSampleCount = 0;

    int windowSize = 3;
    for (int i = 0; i < m_currentLap.size(); ++i) {
        int startIdx = std::max(0, i - windowSize);
        int endIdx = std::min(static_cast<int>(m_currentLap.size()) - 1, i + windowSize);

        float avgSteer = 0, avgLatG = 0, avgBrake = 0, avgSpeed = 0;
        for (int j = startIdx; j <= endIdx; ++j) {
            avgSteer += std::abs(m_currentLap[j].steering);
            avgLatG  += std::abs(m_currentLap[j].latG);
            avgBrake += m_currentLap[j].brake;
            avgSpeed += m_currentLap[j].speed;
        }
        int n = endIdx - startIdx + 1;
        avgSteer /= n; avgLatG /= n; avgBrake /= n; avgSpeed /= n;

        if (avgSteer > 0.25f && avgSpeed > 30.0f) {
            cornerCount++;
            totalLatG += avgLatG;
            totalSteerAngle += avgSteer;

            if (avgBrake < 0.1f && avgLatG < 0.8f && avgSteer > 0.3f)
                understeerEvents++;
        }

        if (avgBrake > thresholdBrakeForce) {
            totalBrakePressure += avgBrake;
            brakeSampleCount++;
        }
    }

    float avgLatGForce = cornerCount > 0 ? totalLatG / cornerCount : 0;
    float avgSteerAngle = cornerCount > 0 ? totalSteerAngle / cornerCount : 0;
    float avgBrakeForce = brakeSampleCount > 0 ? totalBrakePressure / brakeSampleCount : 0;

    int throttleSamples = 0, brakeSamples = 0;
    for (const auto& p : m_currentLap) {
        if (p.throttle > 0.1f) throttleSamples++;
        else if (p.brake > 0.1f) brakeSamples++;
    }
    int totalSamples = m_currentLap.size();
    float throttleRatio = totalSamples > 0 ? static_cast<float>(throttleSamples) / totalSamples : 0;
    float brakeRatio    = totalSamples > 0 ? static_cast<float>(brakeSamples) / totalSamples : 0;

    if (analysis.maxLateralG < 1.3f && avgLatGForce < 0.9f) {
        float deficit = (1.3f - analysis.maxLateralG) / 1.3f;
        rec.suspension.frontSpringRate *= (1.0f + deficit * 0.3f);
        rec.suspension.rearSpringRate  *= (1.0f + deficit * 0.2f);
        rec.suspension.frontARB *= (1.0f + deficit * 0.4f);
        rec.suspension.rearARB  *= (1.0f + deficit * 0.2f);
        rec.suspension.frontCamber = std::max(rec.suspension.frontCamber - deficit * 1.0f, -5.0f);
    }

    if (understeerEvents > cornerCount * 0.3f && cornerCount > 3) {
        float severity = static_cast<float>(understeerEvents) / cornerCount;
        rec.suspension.frontSpringRate *= (1.0f + severity * 0.15f);
        rec.suspension.rearSpringRate  *= (1.0f - severity * 0.10f);
        rec.suspension.frontARB *= (1.0f + severity * 0.25f);
        rec.suspension.frontCamber = std::max(rec.suspension.frontCamber - severity * 0.5f, -5.0f);
    }

    if (analysis.maxLongitudinalG > 1.2f && avgBrakeForce > 0.7f) {
        float brakeIntensity = std::min(analysis.maxLongitudinalG / 1.5f, 1.2f);
        rec.brakes.frontPressure *= brakeIntensity;
    }

    if (brakeRatio > 0.25f && avgBrakeForce < 0.5f) {
        rec.brakes.frontBias = std::min(rec.brakes.frontBias + 0.02f, 0.72f);
    }

    if (analysis.topSpeed > thresholdSpeedMs) {
        float aeroFactor = (analysis.topSpeed - thresholdSpeedMs) / thresholdSpeedMs;
        rec.aero.frontWing += aeroFactor * 2.0f;
        rec.aero.rearWing  += aeroFactor * 2.0f;
    }

    if (avgLatGForce > 1.3f && avgSteerAngle > 0.4f && cornerCount > 3) {
        rec.suspension.rearARB *= 1.15f;
        rec.suspension.rearCamber = std::max(rec.suspension.rearCamber - 0.3f, -5.0f);
    }

    if (throttleRatio < 0.35f && brakeRatio > 0.25f) {
        float diff = (brakeRatio - throttleRatio) * 0.5f;
        rec.aero.frontWing = std::max(rec.aero.frontWing - diff, 0.0f);
    }

    rec.confidence = std::min(rec.confidence + 0.1f, 1.0f);
}

void TelemetryAnalyzer::exportToJson(const QString& path) const
{
    QJsonObject obj;
    QJsonArray laps;

    for (const auto& lap : m_laps) {
        QJsonObject lapObj;
        lapObj["lapTime"] = lap.lapTime;
        lapObj["topSpeed"] = lap.topSpeed;
        lapObj["avgSpeed"] = lap.avgSpeed;
        lapObj["maxLateralG"] = lap.maxLateralG;
        lapObj["maxLongitudinalG"] = lap.maxLongitudinalG;
        laps.append(lapObj);
    }

    obj["laps"] = laps;

    QJsonDocument doc(obj);
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}

void TelemetryAnalyzer::importFromJson(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    m_laps.clear();
    QJsonArray laps = doc.object()["laps"].toArray();

    for (const QJsonValue& val : laps) {
        QJsonObject lapObj = val.toObject();
        LapAnalysis lap;
        lap.lapTime = lapObj["lapTime"].toDouble();
        lap.topSpeed = lapObj["topSpeed"].toDouble();
        lap.avgSpeed = lapObj["avgSpeed"].toDouble();
        lap.maxLateralG = lapObj["maxLateralG"].toDouble();
        lap.maxLongitudinalG = lapObj["maxLongitudinalG"].toDouble();
        m_laps.append(lap);
    }
}

} // namespace ks