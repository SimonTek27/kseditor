#include "FfbPreviewWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QTimer>
#include <QDial>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QFileDialog>
#include <QTextStream>
#include <cmath>

namespace ks {

FfbPreviewWidget::FfbPreviewWidget(QWidget* parent)
    : QWidget(parent)
{
    buildUI();
}

void FfbPreviewWidget::buildUI() {
    auto* layout = new QVBoxLayout(this);

    m_chart = new QChart();
    m_chart->setTitle("FFB Torque vs Steering Angle");
    m_chart->setBackgroundVisible(false);
    m_chart->legend()->hide();

    m_chartView = new QChartView(m_chart, this);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumHeight(280);

    m_ffbSeries = new QLineSeries();
    m_chart->addSeries(m_ffbSeries);

    QValueAxis* axisX = new QValueAxis();
    axisX->setTitleText("Steering Angle (deg)");
    axisX->setRange(-45, 45);
    m_chart->addAxis(axisX, Qt::AlignBottom);
    m_ffbSeries->attachAxis(axisX);

    QValueAxis* axisY = new QValueAxis();
    axisY->setTitleText("FFB Torque (Nm)");
    axisY->setRange(-30, 30);
    m_chart->addAxis(axisY, Qt::AlignLeft);
    m_ffbSeries->attachAxis(axisY);

    auto* controls = new QHBoxLayout;
    m_angleDial = new QDial(this);
    m_angleDial->setRange(-900, 900);
    m_angleDial->setNotchesVisible(true);
    m_angleDial->setWrapping(true);

    m_speedDial = new QDial(this);
    m_speedDial->setRange(0, 300);
    m_speedDial->setNotchesVisible(true);

    m_dampingDial = new QDial(this);
    m_dampingDial->setRange(0, 100);
    m_dampingDial->setNotchesVisible(true);
    m_dampingDial->setValue(50);

    m_torqueLabel = new QLabel("Torque: 0 Nm", this);
    m_peakLabel = new QLabel("Peak: 0 Nm", this);
    m_ffbBar = new QProgressBar(this);
    m_ffbBar->setRange(0, 100);

    auto* presetBox = new QComboBox(this);
    presetBox->addItems({"Street", "GT3", "Formula", "Drift", "Rally"});
    auto* exportBtn = new QPushButton("Export Curve", this);

    controls->addWidget(new QLabel("Steering Angle:"));
    controls->addWidget(m_angleDial);
    controls->addWidget(new QLabel("Speed (km/h):"));
    controls->addWidget(m_speedDial);
    controls->addWidget(new QLabel("Damping:"));
    controls->addWidget(m_dampingDial);
    controls->addWidget(presetBox);
    controls->addWidget(exportBtn);
    controls->addWidget(m_torqueLabel);
    controls->addWidget(m_peakLabel);
    controls->addWidget(m_ffbBar);

    layout->addWidget(m_chartView, 1);
    layout->addLayout(controls);

    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(50);
    connect(m_updateTimer, &QTimer::timeout, this, &FfbPreviewWidget::onDataUpdate);
    connect(m_angleDial, &QDial::valueChanged, this, &FfbPreviewWidget::onSimulateLoad);
    connect(m_speedDial, &QDial::valueChanged, this, &FfbPreviewWidget::onSimulateLoad);
    connect(m_dampingDial, &QDial::valueChanged, this, &FfbPreviewWidget::onSimulateLoad);
    connect(presetBox, QOverload<const QString&>::of(&QComboBox::currentTextChanged), this, &FfbPreviewWidget::onLoadPreset);
    connect(exportBtn, &QPushButton::clicked, this, &FfbPreviewWidget::onExportFfbCurve);
}

void FfbPreviewWidget::startPreview() {
    m_previewActive = true;
    m_updateTimer->start();
}

void FfbPreviewWidget::stopPreview() {
    m_previewActive = false;
    m_updateTimer->stop();
}

void FfbPreviewWidget::onDataUpdate() {
    if (!m_previewActive) return;

    m_ffbSeries->clear();
    for (double angle = -45; angle <= 45; angle += 1.0) {
        double torque = calculateFfbTorque(angle, m_currentSpeed, m_currentDamping);
        m_ffbSeries->append(angle, torque);
    }
}

void FfbPreviewWidget::onSimulateLoad(double value) {
    Q_UNUSED(value);
    m_currentAngle = m_angleDial->value();
    m_currentSpeed = m_speedDial->value();
    m_currentDamping = m_dampingDial->value() / 100.0;

    double torque = calculateFfbTorque(m_currentAngle, m_currentSpeed, m_currentDamping);
    m_torqueLabel->setText(QString("Torque: %1 Nm").arg(torque, 0, 'f', 1));
    if (qAbs(torque) > m_peakFfb) {
        m_peakFfb = qAbs(torque);
        m_peakLabel->setText(QString("Peak: %1 Nm").arg(m_peakFfb, 0, 'f', 1));
    }
    m_ffbBar->setValue(qMin(100, int(qAbs(torque) / 30.0 * 100)));
    emit ffbLevelChanged(torque);
}

void FfbPreviewWidget::onLoadPreset(const QString& preset) {
    if (preset == "Street") { m_currentDamping = 0.5; m_dampingDial->setValue(50); }
    else if (preset == "GT3") { m_currentDamping = 0.3; m_dampingDial->setValue(30); }
    else if (preset == "Formula") { m_currentDamping = 0.2; m_dampingDial->setValue(20); }
    else if (preset == "Drift") { m_currentDamping = 0.1; m_dampingDial->setValue(10); }
    else if (preset == "Rally") { m_currentDamping = 0.4; m_dampingDial->setValue(40); }
}

void FfbPreviewWidget::onExportFfbCurve() {
    QString path = QFileDialog::getSaveFileName(this, "Export FFB Curve", QString(), "CSV (*.csv)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream out(&file);
        out << "Angle,Torque\n";
        for (double angle = -45; angle <= 45; angle += 1.0) {
            double torque = calculateFfbTorque(angle, m_currentSpeed, m_currentDamping);
            out << angle << "," << torque << "\n";
        }
    }
}

void FfbPreviewWidget::drawGauge(double current, double max, QPainter* p, const QRect& rect) {
    Q_UNUSED(current); Q_UNUSED(max); Q_UNUSED(p); Q_UNUSED(rect);
}

double FfbPreviewWidget::calculateFfbTorque(double angle, double speed, double damping) const {
    double maxTorque = 25.0;
    double normalizedAngle = angle / 45.0;
    double speedFactor = qBound(0.2, speed / 300.0, 1.0);
    double dampingFactor = 1.0 - damping * 0.5;
    double torque = maxTorque * qSin(qDegreesToRadians(normalizedAngle * 90)) * speedFactor * dampingFactor;
    return torque;
}

} // namespace ks