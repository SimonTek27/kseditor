#pragma once

#include <QObject>
#include <QVector>
#include <QElapsedTimer>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

namespace ks {

struct SectorHistory {
    QVector<double> sector1;
    QVector<double> sector2;
    QVector<double> sector3;
    double bestSector1 = 1e9;
    double bestSector2 = 1e9;
    double bestSector3 = 1e9;
    double projectedBestLap() const {
        return bestSector1 + bestSector2 + bestSector3;
    }
};

struct LapValidationResult {
    double predictedLapTime = 0;
    double confidencePercent = 0;
    double fuelWeightPenalty = 0;
    double tireWearPenalty = 0;
    double trackEvolutionPenalty = 0;
    double driverConsistencyPenalty = 0;
    double projectedBestLap = 0;
    QStringList limitingFactors;
    QJsonObject sectorProjection;
};

class LapTimeValidation : public QObject {
    Q_OBJECT
    Q_PROPERTY(double bestSector1 READ bestSector1 NOTIFY historyChanged)
    Q_PROPERTY(double bestSector2 READ bestSector2 NOTIFY historyChanged)
    Q_PROPERTY(double bestSector3 READ bestSector3 NOTIFY historyChanged)
    Q_PROPERTY(double projectedBestLap READ projectedBestLap NOTIFY historyChanged)
    Q_PROPERTY(int validationCount READ validationCount NOTIFY historyChanged)

public:
    static LapTimeValidation* instance();

    LapValidationResult validateEstimate(double baseLapTime,
                                          double fuelKg,
                                          double trackLengthKm,
                                          const QVector<double>& historicalLaps,
                                          double tireWearPercent = 0,
                                          double trackGripDelta = 0);

    void recordSectorTimes(double s1, double s2, double s3);
    void resetHistory();

    SectorHistory sectorHistory() const { return m_sectorHistory; }
    double bestSector1() const { return m_sectorHistory.bestSector1; }
    double bestSector2() const { return m_sectorHistory.bestSector2; }
    double bestSector3() const { return m_sectorHistory.bestSector3; }
    double projectedBestLap() const { return m_sectorHistory.projectedBestLap(); }
    int validationCount() const { return m_validationCount; }
    double estimateWithFuelWeight(double baseLapTime, double fuelKg, double trackLengthKm) const;
    double estimateWithTireWear(double baseLapTime, double tireWearPercent) const;
    double calculateConfidence(const QVector<double>& historicalLaps, int numFactors) const;

    QJsonObject generateValidationReport(const LapValidationResult& result) const;

signals:
    void validationComplete(const LapValidationResult& result);
    void sectorRecorded(int sector, double time);
    void historyChanged();

private:
    explicit LapTimeValidation(QObject* parent = nullptr);
    static LapTimeValidation* s_instance;

    SectorHistory m_sectorHistory;
    double m_fuelPerKgPerKm = 0.0015;
    double m_tireWearPerPercent = 0.002;
    double m_trackEvolutionMax = 0.003;
    double m_baseLapTimeCache = 0;
    int m_validationCount = 0;
};

} // namespace ks
