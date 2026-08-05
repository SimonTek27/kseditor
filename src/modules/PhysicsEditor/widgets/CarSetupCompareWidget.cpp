#include "CarSetupCompareWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QFileInfo>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include "../../../plugins/simulators/kunos/assettocorsa/ksAssettoCorsaSetup.h"

namespace ks {

using KsSetupManager = ::ks::kunos::KsSetupManager;

CarSetupCompareWidget::CarSetupCompareWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    m_labelA = new QLabel("Setup A: (none)", this);
    m_labelB = new QLabel("Setup B: (none)", this);
    m_browseA = new QPushButton("Browse A", this);
    m_browseB = new QPushButton("Browse B", this);

    auto* topLayout = new QHBoxLayout;
    topLayout->addWidget(m_labelA);
    topLayout->addWidget(m_browseA);
    topLayout->addWidget(m_labelB);
    topLayout->addWidget(m_browseB);
    layout->addLayout(topLayout);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({"Parameter", "Setup A", "Setup B"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(m_table);

    auto* btnLayout = new QHBoxLayout;
    auto* refreshBtn = new QPushButton("Refresh Diff", this);
    btnLayout->addWidget(refreshBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    connect(m_browseA, &QPushButton::clicked, this, &CarSetupCompareWidget::onBrowseA);
    connect(m_browseB, &QPushButton::clicked, this, &CarSetupCompareWidget::onBrowseB);
    connect(refreshBtn, &QPushButton::clicked, this, &CarSetupCompareWidget::onRefreshDiff);
}

void CarSetupCompareWidget::loadSetupA(const QString& path) {
    m_pathA = path;
    KsSetupManager mgr;
    if (mgr.load(path, m_setupA))
        m_labelA->setText("Setup A: " + QFileInfo(path).fileName());
    else
        m_labelA->setText("Setup A: (failed to load)");
    populateTable();
    emit setupLoaded("A", path);
}

void CarSetupCompareWidget::loadSetupB(const QString& path) {
    m_pathB = path;
    KsSetupManager mgr;
    if (mgr.load(path, m_setupB))
        m_labelB->setText("Setup B: " + QFileInfo(path).fileName());
    else
        m_labelB->setText("Setup B: (failed to load)");
    populateTable();
    emit setupLoaded("B", path);
}

void CarSetupCompareWidget::clear() {
    m_setupA = KsSetupData();
    m_setupB = KsSetupData();
    m_pathA.clear();
    m_pathB.clear();
    m_labelA->setText("Setup A: (none)");
    m_labelB->setText("Setup B: (none)");
    m_table->setRowCount(0);
}

QVector<QPair<QString, double>> CarSetupCompareWidget::buildRows(const KsSetupData& s) const {
    QVector<QPair<QString, double>> rows;
    rows.append({"Front Spring Rate", (s.springRate[0] + s.springRate[1]) / 2.0});
    rows.append({"Rear Spring Rate", (s.springRate[2] + s.springRate[3]) / 2.0});
    rows.append({"Front Damper Bump", (s.compression[0] + s.compression[1]) / 2.0});
    rows.append({"Front Damper Rebound", (s.rebound[0] + s.rebound[1]) / 2.0});
    rows.append({"Rear Damper Bump", (s.compression[2] + s.compression[3]) / 2.0});
    rows.append({"Rear Damper Rebound", (s.rebound[2] + s.rebound[3]) / 2.0});
    rows.append({"Front Ride Height", (s.rideHeight[0] + s.rideHeight[1]) / 2.0});
    rows.append({"Rear Ride Height", (s.rideHeight[2] + s.rideHeight[3]) / 2.0});
    rows.append({"Front ARB", s.frontARB});
    rows.append({"Rear ARB", s.rearARB});
    rows.append({"Front Camber", (s.frontCamber[0] + s.frontCamber[1]) / 2.0});
    rows.append({"Rear Camber", (s.rearCamber[0] + s.rearCamber[1]) / 2.0});
    rows.append({"Front Toe", (s.toeOut[0] + s.toeOut[1]) / 2.0});
    rows.append({"Rear Toe", (s.toeOut[2] + s.toeOut[3]) / 2.0});
    rows.append({"Front Pressure", (s.frontTyrePressure[0] + s.frontTyrePressure[1]) / 2.0});
    rows.append({"Rear Pressure", (s.rearTyrePressure[0] + s.rearTyrePressure[1]) / 2.0});
    rows.append({"Front Wing", s.frontWing});
    rows.append({"Rear Wing", s.rearWing});
    rows.append({"Brake Bias", s.brakeBias});
    rows.append({"TC Level", s.parameters.value("ELECTRONICS.TC", 0)});
    rows.append({"ABS Level", s.parameters.value("ELECTRONICS.ABS", 0)});
    rows.append({"Fuel Load", s.fuelLevel});
    rows.append({"Differential Power", s.diffPower});
    rows.append({"Differential Coast", s.diffCoast});
    rows.append({"Differential Drive", s.diffDrive});
    return rows;
}

void CarSetupCompareWidget::populateTable() {
    auto rowsA = buildRows(m_setupA);
    auto rowsB = buildRows(m_setupB);
    int count = qMax(rowsA.size(), rowsB.size());
    m_table->setRowCount(count);

    for (int i = 0; i < count; ++i) {
        QString name = i < rowsA.size() ? rowsA[i].first : (i < rowsB.size() ? rowsB[i].first : "");
        double valA = i < rowsA.size() ? rowsA[i].second : 0;
        double valB = i < rowsB.size() ? rowsB[i].second : 0;

        m_table->setItem(i, 0, new QTableWidgetItem(name));
        m_table->setItem(i, 1, new QTableWidgetItem(QString::number(valA, 'f', 2)));
        m_table->setItem(i, 2, new QTableWidgetItem(QString::number(valB, 'f', 2)));

        if (valA != valB) {
            m_table->item(i, 1)->setBackground(Qt::yellow);
            m_table->item(i, 2)->setBackground(Qt::yellow);
        }
    }
}

void CarSetupCompareWidget::onBrowseA() {
    QString path = QFileDialog::getOpenFileName(this, "Select Setup A", QString(), "INI files (*.ini);;All files (*.*)");
    if (!path.isEmpty()) loadSetupA(path);
}

void CarSetupCompareWidget::onBrowseB() {
    QString path = QFileDialog::getOpenFileName(this, "Select Setup B", QString(), "INI files (*.ini);;All files (*.*)");
    if (!path.isEmpty()) loadSetupB(path);
}

void CarSetupCompareWidget::onRefreshDiff() {
    populateTable();
}

} // namespace ks