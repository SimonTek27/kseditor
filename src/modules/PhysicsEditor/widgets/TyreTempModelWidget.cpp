#include "TyreTempModelWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>
#include <QTableWidget>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QApplication>
#include <QPainter>
#include <QHeaderView>
#include <algorithm>

namespace ks {

TyreTempModelWidget::TyreTempModelWidget(QWidget* parent)
    : QWidget(parent)
{
    buildUI();
}

void TyreTempModelWidget::buildUI() {
    auto* layout = new QVBoxLayout(this);

    m_tempChart = new QChart();
    m_tempChart->setTitle("Tyre Temperature Simulation");
    m_tempChart->legend()->setVisible(true);

    m_tempChartView = new QChartView(m_tempChart, this);
    m_tempChartView->setRenderHint(QPainter::Antialiasing);
    m_tempChartView->setMinimumHeight(300);

    m_tempFL = new QLineSeries();
    m_tempFL->setName("FL");
    m_tempFL->setColor(Qt::red);

    m_tempFR = new QLineSeries();
    m_tempFR->setName("FR");
    m_tempFR->setColor(Qt::blue);

    m_tempRL = new QLineSeries();
    m_tempRL->setName("RL");
    m_tempRL->setColor(Qt::green);

    m_tempRR = new QLineSeries();
    m_tempRR->setName("RR");
    m_tempRR->setColor(Qt::magenta);

    m_tempChart->addSeries(m_tempFL);
    m_tempChart->addSeries(m_tempFR);
    m_tempChart->addSeries(m_tempRL);
    m_tempChart->addSeries(m_tempRR);

    QValueAxis* axisX = new QValueAxis();
    axisX->setTitleText("Lap");
    axisX->setRange(0, 10);
    m_tempChart->addAxis(axisX, Qt::AlignBottom);
    m_tempFL->attachAxis(axisX);
    m_tempFR->attachAxis(axisX);
    m_tempRL->attachAxis(axisX);
    m_tempRR->attachAxis(axisX);

    QValueAxis* axisY = new QValueAxis();
    axisY->setTitleText("Temperature (°C)");
    axisY->setRange(20, 130);
    m_tempChart->addAxis(axisY, Qt::AlignLeft);
    m_tempFL->attachAxis(axisY);
    m_tempFR->attachAxis(axisY);
    m_tempRL->attachAxis(axisY);
    m_tempRR->attachAxis(axisY);

    m_tempTable = new QTableWidget(4, 4, this);
    m_tempTable->setHorizontalHeaderLabels({"Tyre", "Start °C", "End °C", "Peak °C"});
    m_tempTable->verticalHeader()->setVisible(false);

    QStringList tyres = {"FL", "FR", "RL", "RR"};
    for (int i = 0; i < 4; ++i) {
        m_tempTable->setItem(i, 0, new QTableWidgetItem(tyres[i]));
        for (int c = 1; c < 4; ++c) {
            m_tempTable->setItem(i, c, new QTableWidgetItem("0"));
        }
    }

    auto* paramsGroup = new QGroupBox("Simulation Parameters", this);
    auto* paramsLayout = new QFormLayout(paramsGroup);

    m_ambientInput = new QDoubleSpinBox(this);
    m_ambientInput->setRange(-20, 50);
    m_ambientInput->setValue(25);
    m_ambientInput->setSuffix(" °C");

    m_trackTempInput = new QDoubleSpinBox(this);
    m_trackTempInput->setRange(-10, 80);
    m_trackTempInput->setValue(35);
    m_trackTempInput->setSuffix(" °C");

    m_avgSpeedInput = new QDoubleSpinBox(this);
    m_avgSpeedInput->setRange(20, 350);
    m_avgSpeedInput->setValue(180);
    m_avgSpeedInput->setSuffix(" km/h");

    m_lapsInput = new QSpinBox(this);
    m_lapsInput->setRange(1, 100);
    m_lapsInput->setValue(5);

    m_simBtn = new QPushButton("Start Simulation", this);
    m_statusLabel = new QLabel("Ready", this);

    paramsLayout->addRow("Ambient Temp:", m_ambientInput);
    paramsLayout->addRow("Track Temp:", m_trackTempInput);
    paramsLayout->addRow("Avg Speed:", m_avgSpeedInput);
    paramsLayout->addRow("Laps:", m_lapsInput);
    paramsLayout->addRow("", m_simBtn);
    paramsLayout->addRow("", m_statusLabel);

    layout->addWidget(m_tempChartView, 1);
    layout->addWidget(m_tempTable);
    layout->addWidget(paramsGroup);

    connect(m_simBtn, &QPushButton::clicked, this, &TyreTempModelWidget::onStartSim);
    connect(m_simBtn, &QPushButton::clicked, this, &TyreTempModelWidget::onResetSim);
}

void TyreTempModelWidget::loadFromIni(const QString& carFolder) {
    // Load tyre parameters from car INI files
    QFile file(carFolder + "/data/tyres.ini");
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream in(&file);
        QString content = in.readAll();
        // Parse tyre data
    }
}

void TyreTempModelWidget::simulateLap(double ambientTemp, double trackTemp,
                                      double avgSpeed, int laps) {
    m_tempsFL.clear();
    m_tempsFR.clear();
    m_tempsRL.clear();
    m_tempsRR.clear();

    double tempFL = trackTemp;
    double tempFR = trackTemp;
    double tempRL = trackTemp;
    double tempRR = trackTemp;

    m_tempFL->clear();
    m_tempFR->clear();
    m_tempRL->clear();
    m_tempRR->clear();

    for (int lap = 0; lap <= laps; ++lap) {
        m_tempsFL.append(tempFL);
        m_tempsFR.append(tempFR);
        m_tempsRL.append(tempRL);
        m_tempsRR.append(tempRR);

        m_tempFL->append(lap, tempFL);
        m_tempFR->append(lap, tempFR);
        m_tempRL->append(lap, tempRL);
        m_tempRR->append(lap, tempRR);

        double load = 4000; // N per tyre
        double friction = 1.0;
        double tempRiseFL = estimateTempRise(avgSpeed, load, friction);
        double tempRiseFR = estimateTempRise(avgSpeed, load, friction);
        double tempRiseRL = estimateTempRise(avgSpeed, load, friction);
        double tempRiseRR = estimateTempRise(avgSpeed, load, friction);

        double coolingFL = estimateCooling(tempFL, ambientTemp);
        double coolingFR = estimateCooling(tempFR, ambientTemp);
        double coolingRL = estimateCooling(tempRL, ambientTemp);
        double coolingRR = estimateCooling(tempRR, ambientTemp);

        tempFL += tempRiseFL - coolingFL;
        tempFR += tempRiseFR - coolingFR;
        tempRL += tempRiseRL - coolingRL;
        tempRR += tempRiseRR - coolingRR;
    }

    updateTable(laps);

    QVector<double> finalTemps = {m_tempsFL.last(), m_tempsFR.last(), m_tempsRL.last(), m_tempsRR.last()};
    emit simulationComplete(finalTemps);
}

double TyreTempModelWidget::estimateTempRise(double speed, double load, double friction) const {
    // Simplified thermal model
    double speedFactor = speed / 200.0;
    double loadFactor = load / 4000.0;
    double frictionFactor = friction;
    return 5.0 * speedFactor * loadFactor * frictionFactor;
}

double TyreTempModelWidget::estimateCooling(double tyreTemp, double ambientTemp) const {
    double delta = tyreTemp - ambientTemp;
    return 0.15 * delta; // Newton's law of cooling approximation
}

void TyreTempModelWidget::onStartSim() {
    m_statusLabel->setText("Simulating...");
    QApplication::processEvents();

    simulateLap(m_ambientInput->value(), m_trackTempInput->value(),
                m_avgSpeedInput->value(), m_lapsInput->value());

    m_statusLabel->setText("Simulation complete");
}

void TyreTempModelWidget::onResetSim() {
    m_tempFL->clear();
    m_tempFR->clear();
    m_tempRL->clear();
    m_tempRR->clear();
    m_tempsFL.clear();
    m_tempsFR.clear();
    m_tempsRL.clear();
    m_tempsRR.clear();

    for (int i = 0; i < 4; ++i) {
        for (int c = 1; c < 4; ++c) {
            m_tempTable->item(i, c)->setText("0");
        }
    }
    m_statusLabel->setText("Reset");
}

void TyreTempModelWidget::onLoadTrackPreset(const QString& track) {
    Q_UNUSED(track);
    // Load track-specific presets
}

void TyreTempModelWidget::updateChart() {
    // Update chart axes
}

void TyreTempModelWidget::updateTable(int laps) {
    QStringList tyres = {"FL", "FR", "RL", "RR"};
    QVector<QVector<double>> allTemps = {m_tempsFL, m_tempsFR, m_tempsRL, m_tempsRR};

    for (int i = 0; i < 4; ++i) {
        double start = allTemps[i].isEmpty() ? 0 : allTemps[i].first();
        double end = allTemps[i].isEmpty() ? 0 : allTemps[i].last();
        double peak = allTemps[i].isEmpty() ? 0 : *std::max_element(allTemps[i].begin(), allTemps[i].end());

        m_tempTable->item(i, 1)->setText(QString::number(start, 'f', 1));
        m_tempTable->item(i, 2)->setText(QString::number(end, 'f', 1));
        m_tempTable->item(i, 3)->setText(QString::number(peak, 'f', 1));
    }
}

} // namespace ks