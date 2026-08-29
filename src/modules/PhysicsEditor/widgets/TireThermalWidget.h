#pragma once

#include <QWidget>
#include <QChart>
#include <QChartView>
#include <QLabel>
#include <QVector3D>

#if __has_include(<Q3DScatter>)
#include <Q3DScatter>
#include <QScatter3DSeries>
#include <Q3DTheme>
#define HAS_TIRE_3D 1
#else
#define HAS_TIRE_3D 0
#endif

namespace ks {

#if HAS_TIRE_3D

class TireThermalWidget : public QWidget {
    Q_OBJECT
public:
    explicit TireThermalWidget(QWidget* parent = nullptr);
    ~TireThermalWidget();

    void updateTireData(int wheelIndex, float coreTemp, float surfaceTemp, float wearLevel,
                        const QVector3D& position, const QVector3D& dimension);

    void updateAllTires(float flCore, float flSurface, float flWear,
                        float frCore, float frSurface, float frWear,
                        float rlCore, float rlSurface, float rlWear,
                        float rrCore, float rrSurface, float rrWear);

signals:
    void tireDataUpdated(int wheelIndex);

private:
    void buildUI();
    void updateTireVisualization();

    Q3DScatter* m_chart3D = nullptr;
    QScatter3DSeries* m_tireSeries[4] = {nullptr, nullptr, nullptr, nullptr};

    struct TireData {
        QVector3D position;
        QVector3D dimensions;
        float coreTemp;
        float surfaceTemp;
        float wearLevel;
    } m_tireData[4];

    QLabel* m_statusLabel = nullptr;
};

#else

class TireThermalWidget : public QWidget {
    Q_OBJECT
public:
    explicit TireThermalWidget(QWidget* parent = nullptr) : QWidget(parent) {}
    ~TireThermalWidget() override = default;
    void updateTireData(int, float, float, float, const QVector3D&, const QVector3D&) {}
    void updateAllTires(float, float, float, float, float, float, float, float, float, float, float, float) {}
signals:
    void tireDataUpdated(int);
};

#endif

} // namespace ks
