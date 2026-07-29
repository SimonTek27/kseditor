#include "FileTreeWidget.h"
#include <QDir>
#include <QFileInfo>
#include <QTreeWidgetItem>
#include <QStyle>
#include <QDirIterator>

namespace ks {

const QStringList FileTreeWidget::s_carDataExtensions = {
    "*.ini", "*.json", "*.txt", "*.lut"
};

FileTreeWidget::FileTreeWidget(QWidget* parent)
    : QTreeWidget(parent)
{
    setHeaderLabel(tr("Files"));
    setAnimated(true);
    setIndentation(16);
}

void FileTreeWidget::setRootPath(const QString& path) {
    m_rootPath = path;
    refresh();
}

void FileTreeWidget::refresh() {
    clear();
    if (m_rootPath.isEmpty() || !QDir(m_rootPath).exists()) return;

    QDir dir(m_rootPath);
    QFileInfoList entries;

    QDir dataDir(m_rootPath + "/data");
    if (dataDir.exists()) {
        entries = dataDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
    } else {
        entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
    }

    QMap<QString, QTreeWidgetItem*> folders;
    QStringList nameFilters;
    nameFilters << "*.ini" << "*.json" << "*.txt" << "*.lut" << "*.acd" << "*.kn5";

    for (const QFileInfo& info : entries) {
        if (info.isDir()) {
            QString name = info.fileName();
            QTreeWidgetItem* item = new QTreeWidgetItem(this);
            item->setText(0, name);
            item->setData(0, Qt::UserRole, info.absoluteFilePath());
            item->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
            item->setExpanded(false);
            folders[name] = item;
        }
    }

    for (const QFileInfo& info : entries) {
        if (!info.isFile()) continue;
        QString ext = info.suffix().toLower();
        if (!nameFilters.contains("*." + ext)) continue;

        QString parentPath = info.absolutePath();
        QString relPath = QDir(m_rootPath).relativeFilePath(parentPath);
        QTreeWidgetItem* parentItem = invisibleRootItem();

        if (!relPath.isEmpty() && relPath != ".") {
            QString folderName = relPath.split('/').first();
            if (folders.contains(folderName)) {
                parentItem = folders[folderName];
            }
        }

        QTreeWidgetItem* item = new QTreeWidgetItem(parentItem);
        item->setText(0, info.fileName());
        item->setData(0, Qt::UserRole, info.absoluteFilePath());
        item->setData(0, Qt::UserRole + 1, info.suffix().toLower());
        item->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
    }

    connect(this, &QTreeWidget::itemDoubleClicked, this, &FileTreeWidget::onItemDoubleClicked);
}

void FileTreeWidget::onItemDoubleClicked(QTreeWidgetItem* item, int) {
    if (!item) return;
    QString path = item->data(0, Qt::UserRole).toString();
    if (!path.isEmpty()) emit fileSelected(path);
}

} // namespace ks