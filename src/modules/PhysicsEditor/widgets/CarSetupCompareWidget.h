#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QString>
#include "../../../plugins/simulators/kunos/assettocorsa/ksAssettoCorsaSetup.h"

namespace ks {

using KsSetupData = ::ks::kunos::KsSetupData;

class CarSetupCompareWidget : public QWidget {
    Q_OBJECT
public:
    explicit CarSetupCompareWidget(QWidget* parent = nullptr);

    void loadSetupA(const QString& path);
    void loadSetupB(const QString& path);
    void clear();

signals:
    void setupLoaded(const QString& which, const QString& path);

private slots:
    void onBrowseA();
    void onBrowseB();
    void onRefreshDiff();

private:
    QVector<QPair<QString, double>> buildRows(const KsSetupData& s) const;
    void populateTable();

    QLabel*           m_labelA = nullptr;
    QLabel*           m_labelB = nullptr;
    QPushButton*       m_browseA = nullptr;
    QPushButton*       m_browseB = nullptr;
    QTableWidget*      m_table   = nullptr;
    KsSetupData        m_setupA;
    KsSetupData        m_setupB;
    QString            m_pathA;
    QString            m_pathB;
};

} // namespace ks