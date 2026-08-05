#include "SuspGeometryWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QTableWidget>
#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QFileDialog>
#include <QPainter>
#include <QHeaderView>
#include "../../../plugins/simulators/kunos/assettocorsa/ksAssettoCorsaSetup.h"

namespace ks {

SuspGeometryWidget::SuspGeometryWidget(QWidget* parent)
    : QWidget(parent)
{
    buildUI();
}

void SuspGeometryWidget::buildUI() {
    auto* layout = new QVBoxLayout(this);

    auto* inputGroup = new QGroupBox("Suspension Parameters", this);
    auto* inputLayout = new QFormLayout(inputGroup);

    for (int i = 0; i < 8; ++i) {
        auto* spin = new QDoubleSpinBox(this);
        spin->setRange(-1000, 1000);
        spin->setSingleStep(0.1);
        spin->setDecimals(3);
        m_inputs.append(spin);
    }

    inputLayout->addRow("Upper Arm Length (m):", m_inputs[0]);
    inputLayout->addRow("Lower Arm Length (m):", m_inputs[1]);
    inputLayout->addRow("Upper Arm Angle (deg):", m_inputs[2]);
    inputLayout->addRow("Lower Arm Angle (deg):", m_inputs[3]);
    inputLayout->addRow("Scrub Radius (m):", m_inputs[4]);
    inputLayout->addRow("Wheel Base (m):", m_inputs[5]);
    inputLayout->addRow("Track Width (m):", m_inputs[6]);
    inputLayout->addRow("Lower Mount Y (m):", m_inputs[7]);

    auto* recalcBtn = new QPushButton("Recalculate", this);
    auto* exportBtn = new QPushButton("Export Diagram", this);
    inputLayout->addRow(recalcBtn, exportBtn);

    m_resultsTable = new QTableWidget(4, 5, this);
    m_resultsTable->setHorizontalHeaderLabels({"Wheel", "Camber Gain", "Scrub Radius", "IC X", "IC Y", "Roll Center"});
    m_resultsTable->verticalHeader()->setVisible(false);

    m_camberChart = new QChart();
    m_camberChart->setTitle("Camber Gain vs Bump");
    m_camberChart->setBackgroundVisible(false);
    m_camberChart->legend()->setVisible(true);

    m_camberChartView = new QChartView(m_camberChart, this);
    m_camberChartView->setRenderHint(QPainter::Antialiasing);
    m_camberChartView->setMinimumHeight(250);

    layout->addWidget(inputGroup);
    layout->addWidget(m_resultsTable);
    layout->addWidget(m_camberChartView, 1);

    connect(recalcBtn, &QPushButton::clicked, this, &SuspGeometryWidget::onRecalculate);
    connect(exportBtn, &QPushButton::clicked, this, &SuspGeometryWidget::onExportDiagram);
}

void SuspGeometryWidget::loadFromSetup(const KsSetupData& setup) {
    Q_UNUSED(setup);
    // Load from setup data
}

void SuspGeometryWidget::onRecalculate() {
    m_bumpValues.clear();
    m_camberFL.clear();
    m_camberFR.clear();
    m_camberRL.clear();
    m_camberRR.clear();

    for (double bump = -50; bump <= 50; bump += 5) {
        m_bumpValues.append(bump);
        double cg = calcCamberGain(bump, m_inputs[4]->value(),
                                    m_inputs[0]->value(), m_inputs[1]->value(),
                                    m_inputs[2]->value(), m_inputs[3]->value());
        m_camberFL.append(cg);
        m_camberFR.append(cg);
        m_camberRL.append(cg);
        m_camberRR.append(cg);
    }

    m_resultsTable->setRowCount(4);
    QStringList wheels = {"FL", "FR", "RL", "RR"};
    for (int i = 0; i < 4; ++i) {
        m_resultsTable->setItem(i, 0, new QTableWidgetItem(wheels[i]));
        m_resultsTable->setItem(i, 1, new QTableWidgetItem(QString::number(m_camberFL.last(), 'f', 3)));
        m_resultsTable->setItem(i, 2, new QTableWidgetItem(QString::number(m_inputs[4]->value(), 'f', 3)));
        m_resultsTable->setItem(i, 3, new QTableWidgetItem("0.0"));
        m_resultsTable->setItem(i, 4, new QTableWidgetItem("0.0"));
        m_resultsTable->setItem(i, 5, new QTableWidgetItem("0.0"));
    }

    m_camberChart->removeAllSeries();
    QLineSeries* seriesFL = new QLineSeries();
    seriesFL->setName("Front Left");
    QLineSeries* seriesFR = new QLineSeries();
    seriesFR->setName("Front Right");
    for (int i = 0; i < m_bumpValues.size(); ++i) {
        seriesFL->append(m_bumpValues[i], m_camberFL[i]);
        seriesFR->append(m_bumpValues[i], m_camberFR[i]);
    }
    m_camberChart->addSeries(seriesFL);
    m_camberChart->addSeries(seriesFR);

    QValueAxis* axisX = new QValueAxis();
    axisX->setTitleText("Bump (mm)");
    axisX->setRange(-50, 50);
    m_camberChart->addAxis(axisX, Qt::AlignBottom);
    seriesFL->attachAxis(axisX);
    seriesFR->attachAxis(axisX);

    QValueAxis* axisY = new QValueAxis();
    axisY->setTitleText("Camber Gain (deg/mm)");
    axisY->setRange(-1, 1);
    m_camberChart->addAxis(axisY, Qt::AlignLeft);
    seriesFL->attachAxis(axisY);
    seriesFR->attachAxis(axisY);

    emit geometryCalculated();
}

void SuspGeometryWidget::onExportDiagram() {
    QString path = QFileDialog::getSaveFileName(this, "Export Diagram", QString(), "PNG (*.png);;SVG (*.svg)");
    if (!path.isEmpty()) {
        // Export chart
    }
}

double SuspGeometryWidget::calcCamberGain(double bump, double scrubRadius,
                                          double upperArmLength, double lowerArmLength,
                                          double upperAngle, double lowerAngle) const {
    Q_UNUSED(scrubRadius);
    double upperAngleRad = qDegreesToRadians(upperAngle);
    double lowerAngleRad = qDegreesToRadians(lowerAngle);
    double upperVert = upperArmLength * qSin(upperAngleRad);
    double lowerVert = lowerArmLength * qSin(lowerAngleRad);
    return (upperVert - lowerVert) / (upperArmLength + lowerArmLength) * bump / 1000.0;
}

QPointF SuspGeometryWidget::calcInstantCenter(double upperLength, double lowerLength,
                                              double upperAngle, double lowerAngle,
                                              double lowerMountY) const {
    Q_UNUSED(lowerMountY);
    double upperAngleRad = qDegreesToRadians(upperAngle);
    double lowerAngleRad = qDegreesToRadians(lowerAngle);
    double x1 = 0, y1 = lowerLength * qSin(lowerAngleRad);
    double x2 = upperLength * qCos(upperAngleRad), y2 = upperLength * qSin(upperAngleRad);
    double m1 = qTan(lowerAngleRad);
    double m2 = qTan(upperAngleRad);
    double ix = (m2 * x2 - m1 * x1 + y1 - y2) / (m2 - m1);
    double iy = m1 * (ix - x1) + y1;
    return QPointF(ix, iy);
}

} // namespace ks