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
#include <ks/plugins/simulators/kunos/assettocorsa/ksAssettoCorsaSetup.h>

namespace ks {

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
    m_setupA = KsSetupManager::load(path);
    m_labelA->setText("Setup A: " + QFileInfo(path).fileName());
    populateTable();
    emit setupLoaded("A", path);
}

void CarSetupCompareWidget::loadSetupB(const QString& path) {
    m_pathB = path;
    m_setupB = KsSetupManager::load(path);
    m_labelB->setText("Setup B: " + QFileInfo(path).fileName());
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
    rows.append({"Front Spring Rate", s.frontSpringRate});
    rows.append({"Rear Spring Rate", s.rearSpringRate});
    rows.append({"Front Damper Bump", s.frontDamperBump});
    rows.append({"Front Damper Rebound", s.frontDamperRebound});
    rows.append({"Rear Damper Bump", s.rearDamperBump});
    rows.append({"Rear Damper Rebound", s.rearDamperRebound});
    rows.append({"Front Ride Height", s.frontRideHeight});
    rows.append({"Rear Ride Height", s.rearRideHeight});
    rows.append({"Front ARB", s.frontARB});
    rows.append({"Rear ARB", s.rearARB});
    rows.append({"Front Camber", s.frontCamber});
    rows.append({"Rear Camber", s.rearCamber});
    rows.append({"Front Toe", s.frontToe});
    rows.append({"Rear Toe", s.rearToe});
    rows.append({"Front Pressure", s.frontPressure});
    rows.append({"Rear Pressure", s.rearPressure});
    rows.append({"Front Wing", s.frontWing});
    rows.append({"Rear Wing", s.rearWing});
    rows.append({"Brake Bias", s.brakeBias});
    rows.append({"Brake Pressure", s.brakePressure});
    rows.append({"TC Level", s.tcLevel});
    rows.append({"ABS Level", s.absLevel});
    rows.append({"Fuel Load", s.fuelLoad});
    rows.append({"Differential Preload", s.diffPreload});
    rows.append({"Differential Power", s.diffPower});
    rows.append({"Differential Coast", s.diffCoast});
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