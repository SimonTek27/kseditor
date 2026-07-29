#include "TelemetryWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QPainter>

namespace ks {

TelemetryWidget::TelemetryWidget(QWidget* parent)
    : QWidget(parent)
{
    buildUI();
}

TelemetryWidget::~TelemetryWidget() {
    stopSession();
}

void TelemetryWidget::buildUI() {
    auto* layout = new QVBoxLayout(this);

    auto* topBar = new QHBoxLayout;
    m_sessionLabel = new QLabel("Session: Stopped", this);
    m_startBtn = new QPushButton("Start Session", this);
    m_calibrateBtn = new QPushButton("Calibrate", this);
    m_calibrateBtn->setEnabled(false);
    topBar->addWidget(m_sessionLabel);
    topBar->addWidget(m_startBtn);
    topBar->addWidget(m_calibrateBtn);
    topBar->addStretch();

    auto* gaugesGrid = new QGridLayout;

    // Speed
    auto* speedBox = new QGroupBox("Speed", this);
    auto* speedLayout = new QVBoxLayout(speedBox);
    m_speedBar = new QProgressBar(this);
    m_speedBar->setRange(0, 350);
    m_speedBar->setValue(0);
    m_speedLabel = new QLabel("0 km/h", this);
    m_speedLabel->setAlignment(Qt::AlignCenter);
    m_speedLabel->setStyleSheet("font-size: 24pt; font-weight: bold;");
    speedLayout->addWidget(m_speedBar);
    speedLayout->addWidget(m_speedLabel);

    // RPM
    auto* rpmBox = new QGroupBox("RPM", this);
    auto* rpmLayout = new QVBoxLayout(rpmBox);
    m_rpmBar = new QProgressBar(this);
    m_rpmBar->setRange(0, 12000);
    m_rpmBar->setValue(0);
    m_rpmLabel = new QLabel("0", this);
    m_rpmLabel->setAlignment(Qt::AlignCenter);
    m_rpmLabel->setStyleSheet("font-size: 20pt; font-weight: bold; color: #E74C3C;");
    rpmLayout->addWidget(m_rpmBar);
    rpmLayout->addWidget(m_rpmLabel);

    // Gear
    auto* gearBox = new QGroupBox("Gear", this);
    m_gearLabel = new QLabel("N", this);
    m_gearLabel->setAlignment(Qt::AlignCenter);
    m_gearLabel->setStyleSheet("font-size: 48pt; font-weight: bold; color: #3498DB;");
    auto* gearLayout = new QVBoxLayout(gearBox);
    gearLayout->addWidget(m_gearLabel);

    // Throttle/Brake
    auto* pedalBox = new QGroupBox("Pedals", this);
    auto* pedalLayout = new QFormLayout(pedalBox);
    m_throttleBar = new QProgressBar(this);
    m_throttleBar->setRange(0, 100);
    m_brakeBar = new QProgressBar(this);
    m_brakeBar->setRange(0, 100);
    pedalLayout->addRow("Throttle:", m_throttleBar);
    pedalLayout->addRow("Brake:", m_brakeBar);

    // Tyre temps
    auto* tyreBox = new QGroupBox("Tyre Temps (°C)", this);
    auto* tyreLayout = new QVBoxLayout(tyreBox);
    m_tyreTemps = new QTableWidget(4, 2, this);
    m_tyreTemps->setHorizontalHeaderLabels({"Tyre", "Temp"});
    m_tyreTemps->verticalHeader()->setVisible(false);
    QStringList tyres = {"FL", "FR", "RL", "RR"};
    for (int i = 0; i < 4; ++i) {
        auto* item = new QTableWidgetItem(tyres[i]);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        m_tyreTemps->setItem(i, 0, item);
        m_tyreTemps->setItem(i, 1, new QTableWidgetItem("0"));
    }
    tyreLayout->addWidget(m_tyreTemps);

    gaugesGrid->addWidget(speedBox, 0, 0);
    gaugesGrid->addWidget(rpmBox, 0, 1);
    gaugesGrid->addWidget(gearBox, 1, 0);
    gaugesGrid->addWidget(pedalBox, 1, 1);
    gaugesGrid->addWidget(tyreBox, 2, 0, 1, 2);

    layout->addLayout(topBar);
    layout->addLayout(gaugesGrid);

    connect(m_startBtn, &QPushButton::clicked, this, &TelemetryWidget::onStartStopClicked);
    connect(m_calibrateBtn, &QPushButton::clicked, this, &TelemetryWidget::onCalibrateClicked);
    m_udpSocket = new QUdpSocket(this);
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &TelemetryWidget::onDataReceived);
}

void TelemetryWidget::startSession(const QString& carFolder) {
    m_sessionActive = true;
    m_sessionLabel->setText("Session: Running");
    m_startBtn->setText("Stop Session");
    m_calibrateBtn->setEnabled(true);
    m_udpSocket->bind(m_telemetryPort);
    emit sessionStarted();
}

void TelemetryWidget::stopSession() {
    m_sessionActive = false;
    m_sessionLabel->setText("Session: Stopped");
    m_startBtn->setText("Start Session");
    m_calibrateBtn->setEnabled(false);
    m_udpSocket->close();
    emit sessionStopped();
}

void TelemetryWidget::loadTelemetryConfig(const QString& path) {
    Q_UNUSED(path);
}

void TelemetryWidget::receiveTelemetrySample(const TelemetrySample& sample) {
    m_speedBar->setValue(static_cast<int>(sample.speed * 3.6));
    m_speedLabel->setText(QString("%1 km/h").arg(sample.speed * 3.6, 0, 'f', 0));

    m_rpmBar->setValue(static_cast<int>(sample.rpm));
    m_rpmLabel->setText(QString::number(static_cast<int>(sample.rpm)));

    m_gearLabel->setText(sample.gear > 0 ? QString::number(sample.gear) : "N");

    m_throttleBar->setValue(static_cast<int>(sample.throttle * 100));
    m_brakeBar->setValue(static_cast<int>(sample.brake * 100));

    for (int i = 0; i < 4; ++i) {
        m_tyreTemps->item(i, 1)->setText(QString::number(sample.tyreTemp[i], 'f', 1));
    }

    emit sampleReceived(sample);
}

void TelemetryWidget::onDataReceived() {
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_udpSocket->pendingDatagramSize());
        m_udpSocket->readDatagram(datagram.data(), datagram.size());
        // Parse telemetry data (simplified)
    }
}

void TelemetryWidget::onStartStopClicked() {
    if (m_sessionActive) {
        stopSession();
    } else {
        startSession("");
    }
}

void TelemetryWidget::onCalibrateClicked() {
    // Calibration logic
}

void TelemetryWidget::updateGauge(double value, QProgressBar* bar, QLabel* label,
                                  double min, double max, const QString& unit) {
    bar->setRange(static_cast<int>(min), static_cast<int>(max));
    bar->setValue(static_cast<int>(value));
    label->setText(QString("%1 %2").arg(value, 0, 'f', 1).arg(unit));
}

} // namespace ks