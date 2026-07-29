#include "IdeEditorFileBrowser.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileSystemModel>
#include <QDir>
#include <QFileInfo>

namespace ks {

IdeEditorFileBrowser::IdeEditorFileBrowser(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void IdeEditorFileBrowser::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(2);

    // Path bar
    auto* pathLayout = new QHBoxLayout();
    pathLayout->setContentsMargins(4, 4, 4, 0);

    m_pathEdit = new QLineEdit(this);
    m_pathEdit->setPlaceholderText("Project path...");
    m_pathEdit->setStyleSheet(
        "QLineEdit { background: #252526; border: 1px solid #3a3a3a; color: #d4d4d4; padding: 2px 6px; }");
    pathLayout->addWidget(m_pathEdit);

    m_refreshBtn = new QPushButton("R", this);
    m_refreshBtn->setFixedWidth(24);
    m_refreshBtn->setToolTip("Refresh");
    m_refreshBtn->setStyleSheet(
        "QPushButton { background: #3a5a8a; color: #fff; border: 1px solid #4a6a9a; padding: 1px; }"
        "QPushButton:hover { background: #4a6a9a; }");
    pathLayout->addWidget(m_refreshBtn);

    mainLayout->addLayout(pathLayout);

    // Filter
    m_filterEdit = new QLineEdit(this);
    m_filterEdit->setPlaceholderText("Filter files...");
    m_filterEdit->setStyleSheet(
        "QLineEdit { background: #252526; border: 1px solid #3a3a3a; color: #d4d4d4; padding: 2px 6px; }");
    mainLayout->addWidget(m_filterEdit);

    // Tree
    m_model = new QFileSystemModel(this);
    m_model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    m_model->setNameFilterDisables(false);

    m_tree = new QTreeView(this);
    m_tree->setModel(m_model);
    m_tree->setAnimated(true);
    m_tree->setIndentation(16);
    m_tree->setRootIsDecorated(true);
    m_tree->setAlternatingRowColors(false);
    m_tree->setSortingEnabled(true);
    m_tree->header()->setStretchLastSection(true);
    m_tree->header()->hideSection(1);
    m_tree->header()->hideSection(2);
    m_tree->header()->hideSection(3);
    m_tree->setStyleSheet(
        "QTreeView { background: #252526; color: #d4d4d4; border: none; }"
        "QTreeView::item { padding: 2px 4px; }"
        "QTreeView::item:selected { background: #094771; color: #fff; }"
        "QTreeView::item:hover { background: #2a2d2e; }");
    mainLayout->addWidget(m_tree, 1);

    connect(m_tree, &QTreeView::clicked, this, &IdeEditorFileBrowser::onTreeClicked);
    connect(m_tree, &QTreeView::doubleClicked, this, &IdeEditorFileBrowser::onTreeDoubleClicked);
    connect(m_refreshBtn, &QPushButton::clicked, this, &IdeEditorFileBrowser::refresh);
    connect(m_filterEdit, &QLineEdit::textChanged, this, &IdeEditorFileBrowser::onFilterChanged);
}

void IdeEditorFileBrowser::setRootPath(const QString& path)
{
    m_rootPath = path;
    if (path.isEmpty()) return;

    QDir dir(path);
    if (!dir.exists()) return;

    QModelIndex rootIndex = m_model->setRootPath(path);
    m_tree->setRootIndex(rootIndex);
    m_pathEdit->setText(path);

    // Watch for changes
    if (!m_watcher) {
        m_watcher = new QFileSystemWatcher(this);
        connect(m_watcher, &QFileSystemWatcher::directoryChanged,
                this, &IdeEditorFileBrowser::onDirectoryChanged);
    }
    m_watcher->addPath(path);
}

void IdeEditorFileBrowser::refresh()
{
    if (!m_rootPath.isEmpty()) {
        QModelIndex rootIndex = m_model->setRootPath(m_rootPath);
        m_tree->setRootIndex(rootIndex);
    }
}

void IdeEditorFileBrowser::onTreeClicked(const QModelIndex& index)
{
    if (!index.isValid()) return;
    QString filePath = m_model->filePath(index);
    if (!QFileInfo(filePath).isDir()) {
        emit fileSelected(filePath);
    }
}

void IdeEditorFileBrowser::onTreeDoubleClicked(const QModelIndex& index)
{
    if (!index.isValid()) return;
    QString filePath = m_model->filePath(index);
    if (!QFileInfo(filePath).isDir()) {
        emit fileDoubleClicked(filePath);
    }
}

void IdeEditorFileBrowser::onFilterChanged(const QString& text)
{
    if (text.isEmpty()) {
        m_model->setNameFilters(QStringList());
    } else {
        m_model->setNameFilters(QStringList() << "*" + text + "*");
    }
}

void IdeEditorFileBrowser::onDirectoryChanged(const QString& path)
{
    Q_UNUSED(path);
    refresh();
}

} // namespace ks
