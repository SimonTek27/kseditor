#include "ContentBrowserWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QHeaderView>
#include <QGroupBox>
#include <QFormLayout>
#include <QMessageBox>
#include <QFileInfo>
#include <QPixmap>

namespace ks {

ContentBrowserWidget::ContentBrowserWidget(const QString& contentPath, QWidget* parent)
    : QWidget(parent), m_contentPath(contentPath)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);

    // Left panel
    QWidget* leftPanel = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* catLabel = new QLabel("<b>Content Types</b>");
    m_categoryTree = new QTreeWidget();
    m_categoryTree->setHeaderHidden(true);
    m_categoryTree->setMaximumWidth(180);

    auto addCat = [this](const QString& name, const QString& type) {
        auto* item = new QTreeWidgetItem(m_categoryTree);
        item->setText(0, name);
        item->setData(0, Qt::UserRole, type);
    };
    addCat("All Content", "");
    addCat("Cars", "car");
    addCat("Tracks", "track");
    addCat("Weather", "weather");
    addCat("Mods", "mod");
    m_categoryTree->expandAll();

    m_statsLabel = new QLabel();
    m_statsLabel->setWordWrap(true);

    leftLayout->addWidget(catLabel);
    leftLayout->addWidget(m_categoryTree);
    leftLayout->addWidget(m_statsLabel);
    leftLayout->addStretch();

    // Center panel
    QWidget* centerPanel = new QWidget();
    QVBoxLayout* centerLayout = new QVBoxLayout(centerPanel);
    centerLayout->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout* filterBar = new QHBoxLayout();
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("Search content...");
    m_typeFilter = new QComboBox();
    m_typeFilter->addItems({"All", "Cars", "Tracks", "Weather", "Mods"});
    QPushButton* refreshBtn = new QPushButton("Refresh");

    filterBar->addWidget(m_searchEdit);
    filterBar->addWidget(m_typeFilter);
    filterBar->addWidget(refreshBtn);
    centerLayout->addLayout(filterBar);

    m_contentTable = new QTableWidget();
    m_contentTable->setColumnCount(7);
    m_contentTable->setHorizontalHeaderLabels({"Name", "Type", "Author", "Rating", "Size", "Installed", ""});
    m_contentTable->horizontalHeader()->setStretchLastSection(true);
    m_contentTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_contentTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_contentTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_contentTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_contentTable->setAlternatingRowColors(true);
    centerLayout->addWidget(m_contentTable);

    // Right panel
    QWidget* rightPanel = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    m_previewLabel = new QLabel("No preview");
    m_previewLabel->setMinimumSize(200, 150);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setStyleSheet("background-color: #2a2a2a; border: 1px solid #3a3a3a;");

    QGroupBox* infoGroup = new QGroupBox("Details");
    QFormLayout* infoForm = new QFormLayout();
    m_detailName = new QLabel("-");
    m_detailType = new QLabel("-");
    m_detailAuthor = new QLabel("-");
    m_detailVersion = new QLabel("-");
    m_detailRating = new QLabel("-");
    m_detailSize = new QLabel("-");
    m_detailPath = new QLabel("-");
    m_detailPath->setWordWrap(true);

    infoForm->addRow("Name:", m_detailName);
    infoForm->addRow("Type:", m_detailType);
    infoForm->addRow("Author:", m_detailAuthor);
    infoForm->addRow("Version:", m_detailVersion);
    infoForm->addRow("Rating:", m_detailRating);
    infoForm->addRow("Size:", m_detailSize);
    infoForm->addRow("Path:", m_detailPath);
    infoGroup->setLayout(infoForm);

    QHBoxLayout* actionLayout = new QHBoxLayout();
    m_installBtn = new QPushButton("Install");
    m_uninstallBtn = new QPushButton("Uninstall");
    m_validateBtn = new QPushButton("Validate");
    actionLayout->addWidget(m_installBtn);
    actionLayout->addWidget(m_uninstallBtn);
    actionLayout->addWidget(m_validateBtn);

    rightLayout->addWidget(m_previewLabel);
    rightLayout->addWidget(infoGroup);
    rightLayout->addLayout(actionLayout);
    rightLayout->addStretch();

    splitter->addWidget(leftPanel);
    splitter->addWidget(centerPanel);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);
    splitter->setStretchFactor(2, 1);
    splitter->setSizes({180, 400, 250});

    mainLayout->addWidget(splitter);

    connect(m_categoryTree, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* item, int) {
        m_currentFilter.type = item->data(0, Qt::UserRole).toString();
        refreshContent();
    });
    connect(m_typeFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        QStringList types = {"", "car", "track", "weather", "mod"};
        m_currentFilter.type = types.value(idx);
        refreshContent();
    });
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        m_currentFilter.searchQuery = text;
        refreshContent();
    });
    connect(refreshBtn, &QPushButton::clicked, this, &ContentBrowserWidget::refreshContent);
    connect(m_contentTable, &QTableWidget::itemSelectionChanged, this, &ContentBrowserWidget::onItemSelected);
    connect(m_installBtn, &QPushButton::clicked, this, &ContentBrowserWidget::onInstall);
    connect(m_uninstallBtn, &QPushButton::clicked, this, &ContentBrowserWidget::onUninstall);
    connect(m_validateBtn, &QPushButton::clicked, this, &ContentBrowserWidget::onValidate);

    refreshContent();
}

void ContentBrowserWidget::setContentPath(const QString& path)
{
    m_contentPath = path;
    refreshContent();
}

void ContentBrowserWidget::refreshContent()
{
    if (m_contentPath.isEmpty()) return;

    if (!m_currentFilter.type.isEmpty() && m_currentFilter.type != "mod") {
        if (m_currentFilter.type == "car")
            m_currentItems = ContentBrowser::browseCars(m_contentPath + "/cars", m_currentFilter);
        else if (m_currentFilter.type == "track")
            m_currentItems = ContentBrowser::browseTracks(m_contentPath + "/tracks", m_currentFilter);
        else if (m_currentFilter.type == "weather")
            m_currentItems = ContentBrowser::browseWeather(m_contentPath + "/weather");
    } else {
        m_currentItems = ContentBrowser::browseContent(m_contentPath, m_currentFilter);
    }

    m_contentTable->setRowCount(m_currentItems.size());
    for (int i = 0; i < m_currentItems.size(); ++i) {
        const auto& item = m_currentItems[i];
        m_contentTable->setItem(i, 0, new QTableWidgetItem(item.name));
        m_contentTable->setItem(i, 1, new QTableWidgetItem(item.type));
        m_contentTable->setItem(i, 2, new QTableWidgetItem(item.author));
        m_contentTable->setItem(i, 3, new QTableWidgetItem(QString::number(item.rating, 'f', 1)));
        m_contentTable->setItem(i, 4, new QTableWidgetItem(formatSize(item.size)));
        m_contentTable->setItem(i, 5, new QTableWidgetItem(item.isInstalled ? "Yes" : "No"));
        m_contentTable->item(i, 0)->setData(Qt::UserRole, item.path);
    }

    updateStats();
}

void ContentBrowserWidget::updateStats()
{
    ContentBrowser::ContentStats stats = ContentBrowser::getContentStats(m_contentPath);
    m_statsLabel->setText(
        QString("Cars: %1\nTracks: %2\nWeather: %3\nMods: %4\nTotal size: %5")
            .arg(stats.totalCars).arg(stats.totalTracks).arg(stats.totalWeather)
            .arg(stats.totalMods).arg(formatSize(stats.totalSize)));
}

void ContentBrowserWidget::onItemSelected()
{
    int row = m_contentTable->currentRow();
    if (row < 0 || row >= m_currentItems.size()) return;
    showItemDetails(m_currentItems[row]);
}

void ContentBrowserWidget::showItemDetails(const ContentBrowser::ContentItem& item)
{
    m_detailName->setText(item.name);
    m_detailType->setText(item.type);
    m_detailAuthor->setText(item.author);
    m_detailVersion->setText(item.version);
    m_detailRating->setText(QString::number(item.rating, 'f', 1));
    m_detailSize->setText(formatSize(item.size));
    m_detailPath->setText(item.path);

    m_installBtn->setEnabled(!item.isInstalled);
    m_uninstallBtn->setEnabled(item.isInstalled);

    if (item.previewImage.isNull()) {
        if (!item.previewPath.isEmpty() && QFile::exists(item.previewPath)) {
            QPixmap pix(item.previewPath);
            if (!pix.isNull())
                m_previewLabel->setPixmap(pix.scaled(200, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            else
                m_previewLabel->setText("No preview");
        } else {
            m_previewLabel->setText("No preview");
        }
    } else {
        m_previewLabel->setPixmap(QPixmap::fromImage(item.previewImage)
            .scaled(200, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void ContentBrowserWidget::onInstall()
{
    int row = m_contentTable->currentRow();
    if (row < 0 || row >= m_currentItems.size()) return;
    const auto& item = m_currentItems[row];

    if (ContentBrowser::installMod(item.path, m_contentPath)) {
        QMessageBox::information(this, "Installed", "Content installed successfully: " + item.name);
        emit contentInstalled(item.name);
        refreshContent();
    } else {
        QMessageBox::warning(this, "Install Failed", "Failed to install: " + item.name);
    }
}

void ContentBrowserWidget::onUninstall()
{
    int row = m_contentTable->currentRow();
    if (row < 0 || row >= m_currentItems.size()) return;

    auto reply = QMessageBox::question(this, "Confirm Uninstall",
        "Are you sure you want to uninstall: " + m_currentItems[row].name + "?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (ContentBrowser::uninstallMod(m_currentItems[row].name, m_contentPath)) {
            emit contentUninstalled(m_currentItems[row].name);
            refreshContent();
        } else {
            QMessageBox::warning(this, "Uninstall Failed", "Failed to uninstall: " + m_currentItems[row].name);
        }
    }
}

void ContentBrowserWidget::onValidate()
{
    int row = m_contentTable->currentRow();
    if (row < 0 || row >= m_currentItems.size()) return;
    const auto& item = m_currentItems[row];

    QString error;
    bool valid = ContentBrowser::validateContent(item.path, &error);
    if (valid) {
        QMessageBox::information(this, "Validation", "Content is valid: " + item.name);
    } else {
        QMessageBox::warning(this, "Validation Failed",
            "Content has issues: " + item.name + "\n\n" + error);
    }
}

QString ContentBrowserWidget::formatSize(qint64 bytes) const
{
    if (bytes < 1024) return QString::number(bytes) + " B";
    if (bytes < 1024 * 1024) return QString::number(bytes / 1024.0, 'f', 1) + " KB";
    if (bytes < 1024LL * 1024 * 1024) return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
    return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
}

} // namespace ks
