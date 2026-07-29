#pragma once

#include <QWidget>
#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QTableWidget>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QVector>

namespace ks {

class TyreTempModelWidget : public QWidget {
    Q_OBJECT
public:
    explicit TyreTempModelWidget(QWidget* parent = nullptr);

    void loadFromIni(const QString& carFolder);
    void simulateLap(double ambientTemp, double trackTemp,
                     double avgSpeed, int laps);

signals:
    void simulationComplete(const QVector<double>& finalTemps);

private slots:
    void onStartSim();
    void onResetSim();
    void onLoadTrackPreset(const QString& track);

private:
    void buildUI();
    void updateChart();
    void updateTable(int laps);
    double estimateTempRise(double speed, double load, double friction) const;
    double estimateCooling(double tyreTemp, double ambientTemp) const;

    QChart* m_tempChart = nullptr;
    QChartView* m_tempChartView = nullptr;
    QLineSeries* m_tempFL = nullptr;
    QLineSeries* m_tempFR = nullptr;
    QLineSeries* m_tempRL = nullptr;
    QLineSeries* m_tempRR = nullptr;

    QTableWidget* m_tempTable = nullptr;
    QDoubleSpinBox* m_ambientInput = nullptr;
    QDoubleSpinBox* m_trackTempInput = nullptr;
    QDoubleSpinBox* m_avgSpeedInput = nullptr;
    QSpinBox* m_lapsInput = nullptr;
    QPushButton* m_simBtn = nullptr;
    QLabel* m_statusLabel = nullptr;

    QVector<double> m_tempsFL, m_tempsFR, m_tempsRL, m_tempsRR;
    double m_lastPressure = 32.0;

    double m_lastSimDurationMs = 0;
    int m_lastSimStepCount = 0;
    double m_avgStepMs = 0;
};

} // namespace ks