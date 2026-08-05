#pragma once

#include <QWidget>
#include <QDoubleSpinBox>
#include <QTableWidget>
#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QVector>
#include <QPointF>
#include "../../../plugins/simulators/kunos/assettocorsa/ksAssettoCorsaSetup.h"

namespace ks {

using KsSetupData = ::ks::kunos::KsSetupData;

class SuspGeometryWidget : public QWidget {
    Q_OBJECT
public:
    explicit SuspGeometryWidget(QWidget* parent = nullptr);

    void loadFromSetup(const KsSetupData& setup);
    void setWheelBase(double wb) { m_wheelBase = wb; }
    void setTrackWidth(double tw) { m_trackWidth = tw; }

signals:
    void geometryCalculated();

private slots:
    void onRecalculate();
    void onExportDiagram();

private:
    void buildUI();
    double calcCamberGain(double bump, double scrubRadius,
                          double upperArmLength, double lowerArmLength,
                          double upperAngle, double lowerAngle) const;
    QPointF calcInstantCenter(double upperLength, double lowerLength,
                              double upperAngle, double lowerAngle,
                              double lowerMountY) const;

    struct WheelGeometry {
        double camberGain;
        double scrubRadius;
        double instantCenterX;
        double instantCenterY;
        double rollCenterHeight;
    };

    QVector<QDoubleSpinBox*> m_inputs;
    QTableWidget* m_resultsTable = nullptr;
    QChart* m_camberChart = nullptr;
    QChartView* m_camberChartView = nullptr;

    double m_wheelBase = 2.7;
    double m_trackWidth = 1.6;
    QVector<double> m_bumpValues;
    QVector<double> m_camberFL, m_camberFR, m_camberRL, m_camberRR;
};

} // namespace ks