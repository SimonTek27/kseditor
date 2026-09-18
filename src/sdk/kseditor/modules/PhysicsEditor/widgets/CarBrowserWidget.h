#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QString>

namespace ks {

// ─────────────────────────────────────────────────────────────────────────────
// CarBrowserWidget — left sidebar: search + car list
// ─────────────────────────────────────────────────────────────────────────────
class CarBrowserWidget : public QWidget {
    Q_OBJECT
public:
    explicit CarBrowserWidget(QWidget* parent = nullptr);

    void setCarsPath(const QString& path);
    QString currentCar() const { return m_currentCar; }

signals:
    void carSelected(const QString& carFolder);
    void carReloadRequested(const QString& carFolder);

private slots:
    void onSearchTextChanged(const QString& text);
    void onCarItemClicked(QListWidgetItem* item);
    void onReloadClicked();

private:
    void refreshCarList(const QString& filter = QString());

    QString         m_carsPath;
    QString         m_currentCar;
    QLineEdit*      m_searchBox;
    QListWidget*    m_carList;
    QPushButton*    m_reloadBtn;
    QLabel*         m_pathLabel;
};

} // namespace ks