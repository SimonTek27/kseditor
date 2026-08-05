#include "AcdBrowserWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QTreeWidgetItem>
#include <QLabel>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QFile>

namespace ks {

AcdBrowserWidget::AcdBrowserWidget(QWidget* parent)
    : QWidget(parent)
{
    buildUI();
}

void AcdBrowserWidget::buildUI() {
    auto* layout = new QVBoxLayout(this);

    m_tree = new QTreeWidget(this);
    m_tree->setHeaderLabel("Files");
    m_tree->setColumnCount(2);
    m_tree->setHeaderLabels({"Name", "Type"});

    m_preview = new QTextEdit(this);
    m_preview->setReadOnly(true);
    m_preview->setFont(QFont("Consolas", 9));
    m_preview->setMaximumHeight(150);

    m_exportBtn = new QPushButton("Export Selected", this);

    layout->addWidget(new QLabel("ACD Contents:", this));
    layout->addWidget(m_tree, 1);
    layout->addWidget(new QLabel("Preview:", this));
    layout->addWidget(m_preview);
    layout->addWidget(m_exportBtn);

    connect(m_tree, &QTreeWidget::itemClicked, this, &AcdBrowserWidget::onItemClicked);
    connect(m_exportBtn, &QPushButton::clicked, this, &AcdBrowserWidget::onExportSelected);
}

void AcdBrowserWidget::setExtractedPath(const QString& path) {
    m_extractPath = path;
    refresh();
}

void AcdBrowserWidget::refresh() {
    m_tree->clear();
    if (m_extractPath.isEmpty() || !QDir(m_extractPath).exists()) return;

    QDir dir(m_extractPath);
    QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);

    for (const QFileInfo& info : entries) {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_tree);
        item->setText(0, info.fileName());
        item->setText(1, detectFileType(info.suffix()));
        item->setData(0, Qt::UserRole, info.absoluteFilePath());
        item->setIcon(0, style()->standardIcon(info.isDir() ? QStyle::SP_DirIcon : QStyle::SP_FileIcon));
    }
}

QString AcdBrowserWidget::detectFileType(const QString& ext) const {
    QString lower = ext.toLower();
    if (lower == "ini") return "INI Config";
    if (lower == "lut") return "LUT Curve";
    if (lower == "json") return "JSON Data";
    if (lower == "txt") return "Text";
    if (lower == "acd" || lower == "accd") return "ACD Archive";
    if (lower == "kn5") return "3D Model";
    return "Unknown";
}

void AcdBrowserWidget::onItemClicked(QTreeWidgetItem* item, int col) {
    Q_UNUSED(col);
    if (!item) return;
    QString path = item->data(0, Qt::UserRole).toString();
    QFileInfo fi(path);
    if (fi.isFile() && fi.size() < 1024 * 1024) { // Max 1MB preview
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            m_preview->setPlainText(QString::fromUtf8(data));
        }
    }
}

void AcdBrowserWidget::onExportSelected() {
    QList<QTreeWidgetItem*> items = m_tree->selectedItems();
    if (items.isEmpty()) return;

    QString destDir = QFileDialog::getExistingDirectory(this, "Select Export Directory");
    if (destDir.isEmpty()) return;

    for (QTreeWidgetItem* item : items) {
        QString src = item->data(0, Qt::UserRole).toString();
        QFileInfo fi(src);
        QString dst = destDir + "/" + fi.fileName();
        QFile::copy(src, dst);
    }
}

} // namespace ks