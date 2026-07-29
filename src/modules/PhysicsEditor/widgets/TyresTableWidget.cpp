#include "TyresTableWidget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QTableWidgetItem>

namespace ks {

TyresTableWidget::TyresTableWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    m_rows = {"FL", "FR", "RL", "RR"};

    m_table = new QTableWidget(4, 5, this);
    m_table->setHorizontalHeaderLabels({tr("Tyre"), tr("Pressure (psi)"), tr("Temp (C)"), tr("Wear %"), tr("Dirt %")});
    m_table->verticalHeader()->setVisible(false);
    m_table->setAlternatingRowColors(true);

    for (int r = 0; r < 4; ++r) {
        auto* nameItem = new QTableWidgetItem(m_rows[r]);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(r, 0, nameItem);
        for (int c = 1; c < 5; ++c) {
            if (!m_table->item(r, c))
                m_table->setItem(r, c, new QTableWidgetItem());
        }
    }

    layout->addWidget(m_table);

    auto* infoLabel = new QLabel(
        tr("Tip: Pressure in psi (cold). Target: 32-33 psi for sport driving."), this);
    infoLabel->setStyleSheet("color: gray; font-size: 9pt;");
    layout->addWidget(infoLabel);

    connect(m_table, &QTableWidget::cellChanged, this, &TyresTableWidget::changed);
}

void TyresTableWidget::loadFromIni(const QMap<QString, QString>& data) {
    auto getVal = [&](const QString& key, double def) {
        return data.value(key, QString::number(def)).toDouble();
    };

    m_table->item(0, 1)->setText(QString::number(getVal("PRESSURE_FL", 32.0), 'f', 1));
    m_table->item(1, 1)->setText(QString::number(getVal("PRESSURE_FR", 32.0), 'f', 1));
    m_table->item(2, 1)->setText(QString::number(getVal("PRESSURE_RL", 32.0), 'f', 1));
    m_table->item(3, 1)->setText(QString::number(getVal("PRESSURE_RR", 32.0), 'f', 1));

    m_table->item(0, 2)->setText(QString::number(getVal("TEMP_FL", 90.0), 'f', 0));
    m_table->item(1, 2)->setText(QString::number(getVal("TEMP_FR", 90.0), 'f', 0));
    m_table->item(2, 2)->setText(QString::number(getVal("TEMP_RL", 90.0), 'f', 0));
    m_table->item(3, 2)->setText(QString::number(getVal("TEMP_RR", 90.0), 'f', 0));
}

void TyresTableWidget::saveToIni(QMap<QString, QString>& data) {
    for (int r = 0; r < 4; ++r) {
        QString prefix = m_rows[r];
        data["PRESSURE_" + prefix] = m_table->item(r, 1)->text();
        data["TEMP_" + prefix] = m_table->item(r, 2)->text();
        data["WEAR_" + prefix] = m_table->item(r, 3)->text();
        data["DIRT_" + prefix] = m_table->item(r, 4)->text();
    }
}

} // namespace ks