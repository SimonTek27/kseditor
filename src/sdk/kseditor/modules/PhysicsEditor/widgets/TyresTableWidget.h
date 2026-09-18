#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QMap>
#include <QString>
#include <QStringList>

namespace ks {

// ─────────────────────────────────────────────────────────────────────────────
// TyresTableWidget — tyre pressure/temperature editor
// ─────────────────────────────────────────────────────────────────────────────
class TyresTableWidget : public QWidget {
    Q_OBJECT
public:
    explicit TyresTableWidget(QWidget* parent = nullptr);

    void loadFromIni(const QMap<QString, QString>& data);
    void saveToIni(QMap<QString, QString>& data);

signals:
    void changed();

private:
    QTableWidget* m_table;
    QStringList m_rows;
};

} // namespace ks