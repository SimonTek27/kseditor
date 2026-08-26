#pragma once

#include <QWidget>
#include <QChart>
#include <QChartView>
#include <Q3DBubbleSeries>
#include <Q3DScatterTheme>

namespace ks {

class TireThermalWidget : public QWidget {
    Q_OBJECT
public:
    explicit TireThermalWidget(QWidget* parent = nullptr);
    ~TireThermalWidget();

    // Update tire temperature data from physics simulator
    void updateTireData(int wheelIndex, float coreTemp, float surfaceTemp, float wearLevel,
                        const QVector3D& position, const QVector3D& dimension);

    // Set all four tires data
    void updateAllTires(float flCore, float flSurface, float flWear,
                        float frCore, float frSurface, float frWear,
                        float rlCore, float rlSurface, float rlWear,
                        float rrCore, float rrSurface, float rrWear);

signals:
    void tireDataUpdated(int wheelIndex);

private:
    void buildUI();

    QChart3D* m_chart3D = nullptr;
    QScatterSeries* m_tireSeries[4] = {nullptr, nullptr, nullptr, nullptr};

    // Per-tire data
    struct TireData {
        QVector3D position;
        QVector3D dimensions;
        float coreTemp;
        float surfaceTemp;
        float wearLevel;
    } m_tireData[4];

    QLabel* m_statusLabel = nullptr;
};

} // namespace ks