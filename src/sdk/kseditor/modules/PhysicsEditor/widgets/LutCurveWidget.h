#pragma once

#include <QWidget>
#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QComboBox>
#include <QLabel>
#include <QVector>
#include <QString>

namespace ks {

// ─────────────────────────────────────────────────────────────────────────────
// LutCurveWidget — loads and renders .LUT (tyre temperature/friction curves)
// ─────────────────────────────────────────────────────────────────────────────
class LutCurveWidget : public QWidget {
    Q_OBJECT
public:
    explicit LutCurveWidget(QWidget* parent = nullptr);

    bool loadLutFile(const QString& path);
    void setData(const QVector<double>& xData, const QVector<double>& yData,
                 const QString& xLabel, const QString& yLabel);
    void clear();

signals:
    void pointSelected(int index, double x, double y);

private slots:
    void onPointHovered(const QPointF& point, bool isHovering);

private:
    void buildUI();
    void renderChart();
    QVector<double> parseLutFile(const QString& content) const;

    QChart*      m_chart     = nullptr;
    QChartView*  m_chartView = nullptr;
    QLineSeries* m_series    = nullptr;
    QLabel*               m_infoLabel = nullptr;
    QComboBox*            m_lutSelector = nullptr;
    QString               m_currentLut;
    QVector<double>       m_xData;
    QVector<double>       m_yData;
};

} // namespace ks