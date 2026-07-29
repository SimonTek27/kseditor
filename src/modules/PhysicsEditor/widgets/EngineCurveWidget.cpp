#include "EngineCurveWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QValueAxis>
#include <QPainter>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QLabel>
#include <QPushButton>
#include <algorithm>
#include <ks/plugins/simulators/kunos/assettocorsa/ksAssettoCorsaIni.h>

namespace ks {

EngineCurveWidget::EngineCurveWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    m_chart = new QChart();
    m_chart->setTitle("Power / Torque Curves");
    m_chart->setBackgroundVisible(false);
    m_chart->legend()->setVisible(true);

    m_chartView = new QChartView(m_chart, this);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumHeight(280);

    m_powerSeries = new QLineSeries();
    m_powerSeries->setName("Power (HP)");
    m_powerSeries->setColor(QColor("#E74C3C"));

    m_torqueSeries = new QLineSeries();
    m_torqueSeries->setName("Torque (Nm)");
    m_torqueSeries->setColor(QColor("#3498DB"));

    m_pointsTable = new QTableWidget(0, 3, this);
    m_pointsTable->setHorizontalHeaderLabels({"RPM", "Power (HP)", "Torque (Nm)"});
    m_pointsTable->setAlternatingRowColors(true);
    m_pointsTable->setMinimumHeight(150);

    auto* statsBar = new QHBoxLayout;
    m_maxPowerLabel = new QLabel("Max Power: --", this);
    m_maxTorqueLabel = new QLabel("Max Torque: --", this);
    m_maxRpmLabel = new QLabel("Max RPM: --", this);
    auto* btnAdd    = new QPushButton("Add Point", this);
    auto* btnRemove = new QPushButton("Remove Point", this);
    auto* btnClear  = new QPushButton("Clear", this);
    auto* btnExport = new QPushButton("Export", this);
    auto* btnImport = new QPushButton("Import", this);
    statsBar->addWidget(m_maxPowerLabel);
    statsBar->addWidget(m_maxTorqueLabel);
    statsBar->addWidget(m_maxRpmLabel);
    statsBar->addStretch();
    statsBar->addWidget(btnAdd);
    statsBar->addWidget(btnRemove);
    statsBar->addWidget(btnClear);
    statsBar->addWidget(btnExport);
    statsBar->addWidget(btnImport);

    layout->addWidget(m_chartView, 1);
    layout->addLayout(statsBar);
    layout->addWidget(m_pointsTable);

    connect(btnAdd, &QPushButton::clicked, this, &EngineCurveWidget::onAddPoint);
    connect(btnRemove, &QPushButton::clicked, this, &EngineCurveWidget::onRemovePoint);
    connect(btnClear, &QPushButton::clicked, this, &EngineCurveWidget::onClearCurve);
    connect(btnExport, &QPushButton::clicked, this, &EngineCurveWidget::onExportCurve);
    connect(btnImport, &QPushButton::clicked, this, &EngineCurveWidget::onImportCurve);
}

void EngineCurveWidget::loadFromData(const QVector<double>& rpm,
                                      const QVector<double>& power,
                                      const QVector<double>& torque) {
    m_rpm   = rpm;
    m_power = power;
    m_torque = torque;
    updateChart();
}

void EngineCurveWidget::saveToData(QVector<double>& rpmOut,
                                    QVector<double>& powerOut,
                                    QVector<double>& torqueOut) {
    rpmOut    = m_rpm;
    powerOut  = m_power;
    torqueOut = m_torque;
}

bool EngineCurveWidget::loadFromIni(const QString& carFolder) {
    QString iniPath = carFolder + "/data/engine.ini";
    if (!QFile::exists(iniPath)) return false;

    KsIniDocument doc;
    if (!doc.load(iniPath)) return false;

    const KsIniSection* section = doc.section("ENGINE_DATA");
    if (!section) section = doc.section("DATA");
    if (!section) return false;

    m_rpm.clear();
    m_power.clear();
    m_torque.clear();

    for (int i = 0; i < 20; ++i) {
        QString key = QString("RPM_%1").arg(i);
        if (!section->hasKey(key)) break;
        m_rpm.append(section->getFloat(key, 0));
    }

    for (int i = 0; i < 20; ++i) {
        QString key = QString("POWER_%1").arg(i);
        if (!section->hasKey(key)) break;
        m_power.append(section->getFloat(key, 0));
    }

    for (int i = 0; i < 20; ++i) {
        QString key = QString("TORQUE_%1").arg(i);
        if (!section->hasKey(key)) break;
        m_torque.append(section->getFloat(key, 0));
    }

    if (m_rpm.isEmpty()) return false;
    updateChart();
    return true;
}

void EngineCurveWidget::updateChart() {
    m_chart->removeAllSeries();

    m_powerSeries->clear();
    m_torqueSeries->clear();

    int count = std::min(std::min(m_rpm.size(), m_power.size()), m_torque.size());
    for (int i = 0; i < count; ++i) {
        m_powerSeries->append(m_rpm[i], m_power[i]);
        m_torqueSeries->append(m_rpm[i], m_torque[i]);
    }

    m_chart->addSeries(m_powerSeries);
    m_chart->addSeries(m_torqueSeries);

    if (!m_rpm.isEmpty()) {
        double minRpm = *std::min_element(m_rpm.begin(), m_rpm.end());
        double maxRpm = *std::max_element(m_rpm.begin(), m_rpm.end());
        double maxVal = std::max(
            m_power.isEmpty() ? 0 : *std::max_element(m_power.begin(), m_power.end()),
            m_torque.isEmpty() ? 0 : *std::max_element(m_torque.begin(), m_torque.end()));

        QValueAxis* axisX = new QValueAxis();
        axisX->setTitleText("RPM");
        axisX->setRange(minRpm * 0.9, maxRpm * 1.05);
        m_chart->addAxis(axisX, Qt::AlignBottom);
        m_powerSeries->attachAxis(axisX);
        m_torqueSeries->attachAxis(axisX);

        QValueAxis* axisY = new QValueAxis();
        axisY->setTitleText("Value");
        axisY->setRange(0, maxVal * 1.15);
        m_chart->addAxis(axisY, Qt::AlignLeft);
        m_powerSeries->attachAxis(axisY);
        m_torqueSeries->attachAxis(axisY);
    }

    m_pointsTable->setRowCount(count);
    for (int i = 0; i < count; ++i) {
        m_pointsTable->setItem(i, 0, new QTableWidgetItem(QString::number(m_rpm[i], 'f', 0)));
        m_pointsTable->setItem(i, 1, new QTableWidgetItem(QString::number(m_power[i], 'f', 1)));
        m_pointsTable->setItem(i, 2, new QTableWidgetItem(QString::number(m_torque[i], 'f', 1)));
    }

    if (!m_power.isEmpty()) {
        double maxP = *std::max_element(m_power.begin(), m_power.end());
        m_maxPowerLabel->setText(QString("Max Power: %1 HP").arg(maxP, 0, 'f', 0));
    }
    if (!m_torque.isEmpty()) {
        double maxT = *std::max_element(m_torque.begin(), m_torque.end());
        m_maxTorqueLabel->setText(QString("Max Torque: %1 Nm").arg(maxT, 0, 'f', 0));
    }
    if (!m_rpm.isEmpty()) {
        double maxR = *std::max_element(m_rpm.begin(), m_rpm.end());
        m_maxRpmLabel->setText(QString("Max RPM: %1").arg(maxR, 0, 'f', 0));
    }
}

QVector<double> EngineCurveWidget::smoothCurve(const QVector<double>& values, int steps) const {
    if (values.size() < 3) return values;
    QVector<double> smoothed;
    int n = values.size();
    for (int i = 0; i < n; ++i) {
        double sum = 0;
        int cnt = 0;
        for (int j = std::max(0, i - steps); j <= std::min(n - 1, i + steps); ++j) {
            sum += values[j];
            cnt++;
        }
        smoothed.append(sum / cnt);
    }
    return smoothed;
}

void EngineCurveWidget::onAddPoint() {
    int row = m_pointsTable->currentRow();
    double rpm = row >= 0 && row < m_rpm.size() ? m_rpm[row] + 500 : (m_rpm.isEmpty() ? 1000 : m_rpm.last() + 500);
    double power = row >= 0 && row < m_power.size() ? m_power[row] : 0;
    double torque = row >= 0 && row < m_torque.size() ? m_torque[row] : 0;

    m_rpm.append(rpm);
    m_power.append(power);
    m_torque.append(torque);
    std::sort(m_rpm.begin(), m_rpm.end());
    updateChart();
}

void EngineCurveWidget::onRemovePoint() {
    int row = m_pointsTable->currentRow();
    if (row < 0 || row >= m_rpm.size()) return;
    m_rpm.removeAt(row);
    if (row < m_power.size()) m_power.removeAt(row);
    if (row < m_torque.size()) m_torque.removeAt(row);
    updateChart();
}

void EngineCurveWidget::onClearCurve() {
    m_rpm.clear();
    m_power.clear();
    m_torque.clear();
    updateChart();
}

void EngineCurveWidget::onExportCurve() {
    QString path = QFileDialog::getSaveFileName(this, "Export Curve",
        QString(), "CSV files (*.csv);;All files (*.*)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&file);
    out << "RPM,Power,Torque\n";
    int count = std::min(std::min(m_rpm.size(), m_power.size()), m_torque.size());
    for (int i = 0; i < count; ++i) {
        out << m_rpm[i] << "," << m_power[i] << "," << m_torque[i] << "\n";
    }
    file.close();
}

void EngineCurveWidget::onImportCurve() {
    QString path = QFileDialog::getOpenFileName(this, "Import Curve",
        QString(), "CSV files (*.csv);;All files (*.*)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    m_rpm.clear();
    m_power.clear();
    m_torque.clear();

    QTextStream in(&file);
    QString header = in.readLine();
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;
        QStringList parts = line.split(',');
        if (parts.size() >= 3) {
            m_rpm.append(parts[0].toDouble());
            m_power.append(parts[1].toDouble());
            m_torque.append(parts[2].toDouble());
        }
    }
    file.close();
    updateChart();
}

} // namespace ks