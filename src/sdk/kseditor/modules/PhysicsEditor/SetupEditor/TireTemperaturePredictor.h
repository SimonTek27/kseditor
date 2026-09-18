#ifndef TIRETEMPERATUREPREDICTOR_H
#define TIRETEMPERATUREPREDICTOR_H

#include <QObject>
#include <QVector>

namespace ks {

class TireTemperaturePredictor : public QObject {
    Q_OBJECT
public:
    explicit TireTemperaturePredictor(QObject* parent = nullptr);
    ~TireTemperaturePredictor();

    struct TireState {
        float innerTemp;
        float middleTemp;
        float outerTemp;
        float pressure;
        float wear;
    };

    struct TirePrediction {
        TireState predictedState;
        float optimalPressure;
        float pressureDelta;
        float tempRangeOk;
        float overheatingRisk;
        float underTempRisk;
    };

    struct TrackConditions {
        float ambientTemp;
        float trackTemp;
        float humidity;
        int numCorners;
        float avgCornerSpeed;
        float straightLength;
    };

    TirePrediction predictTireTemps(const TireState& current, const TrackConditions& track);
    float calculateOptimalPressure(const TrackConditions& track);
    float estimateWearRate(float pressure, float temp);

signals:
    void predictionReady(const TirePrediction& prediction);

private:
    float calculateHeatGeneration(const TrackConditions& track);
    float calculateCoolingRate(float temp, float speed, const TrackConditions& track);
    float calculatePressureBuild(float tempDelta);
};

} // namespace ks

#endif // TIRETEMPERATUREPREDICTOR_H