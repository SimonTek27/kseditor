#pragma once

#include <QWidget>
#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QTimer>
#include <QDial>
#include <QLabel>
#include <QProgressBar>
#include <QVector>

namespace ks {

class FfbPreviewWidget : public QWidget {
    Q_OBJECT
public:
    explicit FfbPreviewWidget(QWidget* parent = nullptr);

    void startPreview();
    void stopPreview();

signals:
    void ffbLevelChanged(double level);

private slots:
    void onDataUpdate();
    void onSimulateLoad(double angle);
    void onLoadPreset(const QString& preset);
    void onExportFfbCurve();

private:
    void buildUI();
    void drawGauge(double current, double max, QPainter* p, const QRect& rect);
    double calculateFfbTorque(double angle, double speed, double damping) const;

    QChart* m_chart = nullptr;
    QChartView* m_chartView = nullptr;
    QLineSeries* m_ffbSeries = nullptr;
    QTimer* m_updateTimer = nullptr;

    QDial* m_angleDial = nullptr;
    QDial* m_speedDial = nullptr;
    QDial* m_dampingDial = nullptr;
    QLabel* m_torqueLabel = nullptr;
    QLabel* m_peakLabel = nullptr;
    QProgressBar* m_ffbBar = nullptr;

    QVector<double> m_ffbHistory;
    double m_peakFfb = 0;
    double m_currentAngle = 0;
    double m_currentSpeed = 0;
    double m_currentDamping = 0.5;
    bool m_previewActive = false;
};

} // namespace ks