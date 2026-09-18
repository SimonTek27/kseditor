#include "LapTimeValidation.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <QJsonDocument>

namespace ks {

LapTimeValidation* LapTimeValidation::s_instance = nullptr;

LapTimeValidation* LapTimeValidation::instance() {
    if (!s_instance) {
        s_instance = new LapTimeValidation();
    }
    return s_instance;
}

LapTimeValidation::LapTimeValidation(QObject* parent)
    : QObject(parent)
{
}

LapValidationResult LapTimeValidation::validateEstimate(
    double baseLapTime,
    double fuelKg,
    double trackLengthKm,
    const QVector<double>& historicalLaps,
    double tireWearPercent,
    double trackGripDelta)
{
    LapValidationResult result;
    result.predictedLapTime = baseLapTime;
    ++m_validationCount;

    // Fuel weight correction (~0.3s per 10kg per km)
    double fuelFactor = 1.0 + m_fuelPerKgPerKm * fuelKg * trackLengthKm;
    result.fuelWeightPenalty = baseLapTime * (fuelFactor - 1.0);

    // Tire wear correction
    result.tireWearPenalty = baseLapTime * m_tireWearPerPercent * tireWearPercent;

    // Track evolution (rubbering in / cooling)
    double evoFactor = 1.0 + m_trackEvolutionMax * (1.0 - trackGripDelta);
    result.trackEvolutionPenalty = baseLapTime * (evoFactor - 1.0);

    // Driver consistency from historical laps
    if (historicalLaps.size() >= 3) {
        double mean = std::accumulate(historicalLaps.begin(), historicalLaps.end(), 0.0)
                      / historicalLaps.size();
        double variance = 0;
        for (double lap : historicalLaps) {
            variance += (lap - mean) * (lap - mean);
        }
        variance /= historicalLaps.size();
        double stddev = std::sqrt(variance);
        result.driverConsistencyPenalty = stddev * 0.15;
    }

    // Projected best lap from sector history
    auto best = m_sectorHistory.projectedBestLap();
    if (best < 1e8) {
        result.projectedBestLap = best;
    }

    // Final prediction
    double totalPenalty = result.fuelWeightPenalty
                        + result.tireWearPenalty
                        + result.trackEvolutionPenalty
                        + result.driverConsistencyPenalty;
    result.predictedLapTime = baseLapTime + totalPenalty;

    // Confidence scoring
    int limitingFactorCount = 0;
    if (result.fuelWeightPenalty > 0.5) {
        result.limitingFactors << "High fuel weight penalty";
        ++limitingFactorCount;
    }
    if (result.tireWearPenalty > 0.3) {
        result.limitingFactors << "Significant tire wear";
        ++limitingFactorCount;
    }
    if (result.trackEvolutionPenalty > 0.1) {
        result.limitingFactors << "Track evolution offset";
        ++limitingFactorCount;
    }
    if (result.driverConsistencyPenalty > 0.2) {
        result.limitingFactors << "Inconsistent lap times";
        ++limitingFactorCount;
    }
    if (result.projectedBestLap > 0 && result.projectedBestLap < baseLapTime * 0.97) {
        result.limitingFactors << "Sector-based projection faster than estimate";
        ++limitingFactorCount;
    }

    result.confidencePercent = calculateConfidence(historicalLaps, limitingFactorCount);

    // Sector projection
    QJsonObject sectors;
    if (!m_sectorHistory.sector1.isEmpty()) {
        double avg = std::accumulate(m_sectorHistory.sector1.begin(),
                     m_sectorHistory.sector1.end(), 0.0)
                     / m_sectorHistory.sector1.size();
        sectors["sector1Avg"] = avg;
        sectors["sector1Best"] = m_sectorHistory.bestSector1;
    }
    if (!m_sectorHistory.sector2.isEmpty()) {
        double avg = std::accumulate(m_sectorHistory.sector2.begin(),
                     m_sectorHistory.sector2.end(), 0.0)
                     / m_sectorHistory.sector2.size();
        sectors["sector2Avg"] = avg;
        sectors["sector2Best"] = m_sectorHistory.bestSector2;
    }
    if (!m_sectorHistory.sector3.isEmpty()) {
        double avg = std::accumulate(m_sectorHistory.sector3.begin(),
                     m_sectorHistory.sector3.end(), 0.0)
                     / m_sectorHistory.sector3.size();
        sectors["sector3Avg"] = avg;
        sectors["sector3Best"] = m_sectorHistory.bestSector3;
    }
    sectors["projectedBestLap"] = result.projectedBestLap;
    result.sectorProjection = sectors;

    emit validationComplete(result);
    return result;
}

void LapTimeValidation::recordSectorTimes(double s1, double s2, double s3) {
    m_sectorHistory.sector1.append(s1);
    m_sectorHistory.sector2.append(s2);
    m_sectorHistory.sector3.append(s3);

    if (s1 < m_sectorHistory.bestSector1) m_sectorHistory.bestSector1 = s1;
    if (s2 < m_sectorHistory.bestSector2) m_sectorHistory.bestSector2 = s2;
    if (s3 < m_sectorHistory.bestSector3) m_sectorHistory.bestSector3 = s3;

    emit sectorRecorded(1, s1);
    emit sectorRecorded(2, s2);
    emit sectorRecorded(3, s3);
    emit historyChanged();
}

void LapTimeValidation::resetHistory() {
    m_sectorHistory = {};
    m_sectorHistory.bestSector1 = 1e9;
    m_sectorHistory.bestSector2 = 1e9;
    m_sectorHistory.bestSector3 = 1e9;
    m_baseLapTimeCache = 0;
    m_validationCount = 0;
    emit historyChanged();
}

double LapTimeValidation::estimateWithFuelWeight(double baseLapTime, double fuelKg, double trackLengthKm) const {
    double factor = 1.0 + m_fuelPerKgPerKm * fuelKg * trackLengthKm;
    return baseLapTime * factor;
}

double LapTimeValidation::estimateWithTireWear(double baseLapTime, double tireWearPercent) const {
    return baseLapTime * (1.0 + m_tireWearPerPercent * tireWearPercent);
}

double LapTimeValidation::calculateConfidence(const QVector<double>& historicalLaps,
                                               int numFactors) const
{
    double base = 95.0;
    if (historicalLaps.size() < 3) base -= 15.0;
    else if (historicalLaps.size() < 10) base -= 5.0;

    base -= numFactors * 5.0;
    if (m_sectorHistory.sector1.isEmpty()) base -= 5.0;
    if (m_sectorHistory.sector2.isEmpty()) base -= 5.0;
    if (m_sectorHistory.sector3.isEmpty()) base -= 5.0;

    return std::max(10.0, std::min(99.0, base));
}

QJsonObject LapTimeValidation::generateValidationReport(const LapValidationResult& result) const {
    QJsonObject report;
    report["predictedLapTime"] = result.predictedLapTime;
    report["confidencePercent"] = result.confidencePercent;
    report["fuelWeightPenalty"] = result.fuelWeightPenalty;
    report["tireWearPenalty"] = result.tireWearPenalty;
    report["trackEvolutionPenalty"] = result.trackEvolutionPenalty;
    report["driverConsistencyPenalty"] = result.driverConsistencyPenalty;
    report["projectedBestLap"] = result.projectedBestLap;

    QJsonArray factors;
    for (const auto& f : result.limitingFactors) {
        factors.append(f);
    }
    report["limitingFactors"] = factors;
    report["sectorProjection"] = result.sectorProjection;

    return report;
}

} // namespace ks
