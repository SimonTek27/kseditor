#ifndef LAPTIMEIMPACTCALCULATOR_H
#define LAPTIMEIMPACTCALCULATOR_H

#include <QObject>
#include <QVector>

namespace ks {

class LapTimeImpactCalculator : public QObject {
    Q_OBJECT
public:
    explicit LapTimeImpactCalculator(QObject* parent = nullptr);
    ~LapTimeImpactCalculator();

    struct SetupDelta {
        QString parameter;
        float oldValue;
        float newValue;
        float estimatedGainSeconds;
        float confidence;
    };

    struct ImpactReport {
        QVector<SetupDelta> deltas;
        float totalEstimatedGain;
        QStringList recommendedChanges;
    };

    struct CarStats {
        float powerKw;
        float weightKg;
        float dragCoeff;
        float frontalAreaM2;
        float downforceCoeff;
        float maxLateralG;
        float tireGrip;
    };

    struct SetupParameter {
        float frontSpring;
        float rearSpring;
        float frontCamber;
        float rearCamber;
        float frontARB;
        float rearARB;
        float frontWing;
        float rearWing;
        float brakeBias;
    };

    ImpactReport estimateLapTimeImpact(const SetupParameter& oldSetup,
                                       const SetupParameter& newSetup,
                                       const CarStats& car,
                                       float trackLengthKm,
                                       int numCorners,
                                       float avgCornerSpeedKmh);

signals:
    void impactCalculated(const ImpactReport& report);

private:
    float estimateSuspensionImpact(const SetupParameter& oldSetup, const SetupParameter& newSetup,
                                   const CarStats& car, int numCorners);
    float estimateAeroImpact(const SetupParameter& oldSetup, const SetupParameter& newSetup,
                             const CarStats& car, float avgCornerSpeedKmh);
    float estimateCamberImpact(const SetupParameter& oldSetup, const SetupParameter& newSetup,
                               int numCorners);
    float estimateARBImpact(const SetupParameter& oldSetup, const SetupParameter& newSetup,
                            int numCorners);
    float estimateBrakeBiasImpact(const SetupParameter& oldSetup, const SetupParameter& newSetup,
                                  int numCorners);
};

} // namespace ks

#endif // LAPTIMEIMPACTCALCULATOR_H