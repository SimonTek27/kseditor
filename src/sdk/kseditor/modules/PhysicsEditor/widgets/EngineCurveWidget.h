#pragma once

#include <QWidget>
#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QTableWidget>
#include <QLabel>
#include <QVector>
#include <QString>

namespace ks {

class EngineCurveWidget : public QWidget {
    Q_OBJECT
public:
    explicit EngineCurveWidget(QWidget* parent = nullptr);

    void loadFromData(const QVector<double>& rpm,
                      const QVector<double>& power,
                      const QVector<double>& torque);
    void saveToData(QVector<double>& rpm,
                    QVector<double>& power,
                    QVector<double>& torque);
    bool loadFromIni(const QString& carFolder);

signals:
    void curveChanged();

private slots:
    void onAddPoint();
    void onRemovePoint();
    void onClearCurve();
    void onExportCurve();
    void onImportCurve();

private:
    void buildUI();
    void updateChart();
    QVector<double> smoothCurve(const QVector<double>& values, int steps) const;

    QChart*      m_chart     = nullptr;
    QChartView*  m_chartView = nullptr;
    QLineSeries* m_powerSeries = nullptr;
    QLineSeries* m_torqueSeries = nullptr;

    QTableWidget*          m_pointsTable = nullptr;
    QVector<double>        m_rpm;
    QVector<double>        m_power;
    QVector<double>        m_torque;

    QLabel*                m_maxPowerLabel = nullptr;
    QLabel*                m_maxTorqueLabel = nullptr;
    QLabel*                m_maxRpmLabel = nullptr;
};

} // namespace ks