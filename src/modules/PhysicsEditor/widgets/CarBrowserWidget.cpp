#include "CarBrowserWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDir>
#include <QListWidgetItem>

namespace ks {

CarBrowserWidget::CarBrowserWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    m_searchBox = new QLineEdit(this);
    m_searchBox->setPlaceholderText(tr("Search cars..."));

    m_carList = new QListWidget(this);
    m_carList->setIconSize(QSize(24, 24));

    m_reloadBtn = new QPushButton(tr("Reload"), this);
    m_pathLabel = new QLabel(tr("No folder selected"), this);
    m_pathLabel->setWordWrap(true);
    m_pathLabel->setStyleSheet("color: gray; font-size: 9pt;");

    layout->addWidget(new QLabel(tr("Search:"), this));
    layout->addWidget(m_searchBox);
    layout->addWidget(m_carList);

    auto* btnRow = new QHBoxLayout;
    btnRow->addWidget(m_reloadBtn);
    btnRow->addWidget(new QLabel(tr("Path:"), this));
    layout->addLayout(btnRow);
    layout->addWidget(m_pathLabel);

    connect(m_searchBox, &QLineEdit::textChanged, this, &CarBrowserWidget::onSearchTextChanged);
    connect(m_carList, &QListWidget::itemClicked, this, &CarBrowserWidget::onCarItemClicked);
    connect(m_reloadBtn, &QPushButton::clicked, this, &CarBrowserWidget::onReloadClicked);
}

void CarBrowserWidget::setCarsPath(const QString& path) {
    m_carsPath = path;
    m_pathLabel->setText(path);
    refreshCarList();
}

void CarBrowserWidget::refreshCarList(const QString& filter) {
    m_carList->clear();
    if (m_carsPath.isEmpty() || !QDir(m_carsPath).exists()) return;

    QDir dir(m_carsPath);
    QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    entries.sort();

    for (const QString& name : entries) {
        if (!filter.isEmpty() && !name.contains(filter, Qt::CaseInsensitive)) continue;
        auto* item = new QListWidgetItem(name, m_carList);
        item->setData(Qt::UserRole, name);
    }
}

void CarBrowserWidget::onSearchTextChanged(const QString& text) {
    refreshCarList(text);
}

void CarBrowserWidget::onCarItemClicked(QListWidgetItem* item) {
    if (!item) return;
    QString car = item->data(Qt::UserRole).toString();
    m_currentCar = car;
    emit carSelected(m_carsPath + "/" + car);
}

void CarBrowserWidget::onReloadClicked() {
    refreshCarList(m_searchBox->text());
    if (!m_currentCar.isEmpty())
        emit carReloadRequested(m_carsPath + "/" + m_currentCar);
}

} // namespace ks