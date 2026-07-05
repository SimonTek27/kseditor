#include "FileTreeWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileInfo>
#include <QMenu>
#include <QAction>
#include <QDesktopServices>
#include <QProcess>
#include <QUrl>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QDir>
#include <QApplication>
#include <QClipboard>

FileTreeFilterModel::FileTreeFilterModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
    setRecursiveFilteringEnabled(true);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
}

void FileTreeFilterModel::setSearchText(const QString& text)
{
    m_searchText = text.trimmed();
    setFilterFixedString(m_searchText);
    invalidateFilter();
}

bool FileTreeFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    if (m_searchText.isEmpty()) return true;

    auto* fsModel = qobject_cast<QFileSystemModel*>(sourceModel());
    if (!fsModel) return true;

    QModelIndex idx = fsModel->index(sourceRow, 0, sourceParent);
    QString fileName = fsModel->fileName(idx);

    if (fileName.contains(m_searchText, Qt::CaseInsensitive)) return true;

    if (fsModel->isDir(idx)) {
        int rows = fsModel->rowCount(idx);
        for (int i = 0; i < rows; ++i) {
            if (filterAcceptsRow(i, idx)) return true;
        }
    }

    return false;
}

// ---------------------------------------------------------------------------

FileTreeWidget::FileTreeWidget(QWidget* parent)
    : QWidget(parent)
    , m_treeView(nullptr)
    , m_fileModel(nullptr)
    , m_filterProxy(nullptr)
    , m_filterCombo(nullptr)
    , m_refreshBtn(nullptr)
    , m_homeBtn(nullptr)
    , m_rootLabel(nullptr)
    , m_searchEdit(nullptr)
{
    setupUI();
    setupModel();
}

void FileTreeWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Toolbar row
    auto* toolbarLayout = new QHBoxLayout();
    toolbarLayout->setContentsMargins(4, 4, 4, 4);
    toolbarLayout->setSpacing(4);

    m_filterCombo = new QComboBox(this);
    m_filterCombo->addItem("All Files");
    m_filterCombo->addItem("Source Code");
    m_filterCombo->addItem("Config / Text");
    m_filterCombo->addItem("Images");
    m_filterCombo->addItem("3D Models");
    m_filterCombo->addItem("Audio");
    m_filterCombo->setMinimumWidth(100);
    toolbarLayout->addWidget(m_filterCombo, 1);

    m_refreshBtn = new QPushButton("R", this);
    m_refreshBtn->setToolTip("Refresh");
    m_refreshBtn->setFixedSize(24, 24);
    toolbarLayout->addWidget(m_refreshBtn);

    m_homeBtn = new QPushButton("H", this);
    m_homeBtn->setToolTip("Go to project root");
    m_homeBtn->setFixedSize(24, 24);
    toolbarLayout->addWidget(m_homeBtn);

    mainLayout->addLayout(toolbarLayout);

    // Search
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Search files...");
    m_searchEdit->setClearButtonEnabled(true);
    mainLayout->addWidget(m_searchEdit);

    // Tree view
    m_treeView = new QTreeView(this);
    m_treeView->setAnimated(true);
    m_treeView->setAlternatingRowColors(false);
    m_treeView->setDragEnabled(true);
    m_treeView->setAcceptDrops(false);
    m_treeView->setDropIndicatorShown(true);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setSortingEnabled(true);
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeView->header()->setStretchLastSection(true);
    m_treeView->header()->setSectionResizeMode(QHeaderView::Interactive);
    m_treeView->setColumnWidth(0, 220);
    mainLayout->addWidget(m_treeView, 1);

    // Root label
    m_rootLabel = new QLabel("No project open", this);
    m_rootLabel->setStyleSheet("color: #888; padding: 2px 4px; font-size: 11px;");
    m_rootLabel->setWordWrap(true);
    mainLayout->addWidget(m_rootLabel);

    // Connections
    connect(m_treeView, &QTreeView::doubleClicked, this, &FileTreeWidget::onDoubleClicked);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &FileTreeWidget::onCustomContextMenu);
    connect(m_treeView, &QTreeView::collapsed, this, &FileTreeWidget::onTreeCollapsed);
    connect(m_treeView, &QTreeView::expanded, this, &FileTreeWidget::onTreeExpanded);
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FileTreeWidget::onFilterChanged);
    connect(m_refreshBtn, &QPushButton::clicked, this, [this]() { refresh(); });
    connect(m_homeBtn, &QPushButton::clicked, [this]() {
        if (!m_rootPath.isEmpty()) {
            QModelIndex rootIndex = m_fileModel->setRootPath(m_rootPath);
            m_treeView->setRootIndex(m_filterProxy->mapFromSource(rootIndex));
        }
    });
    connect(m_searchEdit, &QLineEdit::textChanged, this, &FileTreeWidget::onSearchTextChanged);
}

void FileTreeWidget::setupModel()
{
    m_fileModel = new QFileSystemModel(this);
    m_fileModel->setRootPath(QDir::rootPath());
    m_fileModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    m_fileModel->setResolveSymlinks(true);

    m_filterProxy = new FileTreeFilterModel(this);
    m_filterProxy->setSourceModel(m_fileModel);

    m_treeView->setModel(m_filterProxy);
    m_treeView->setRootIndex(m_filterProxy->mapFromSource(m_fileModel->index(QDir::homePath())));

    m_treeView->hideColumn(1);
    m_treeView->hideColumn(2);
    m_treeView->hideColumn(3);

    m_rootPath = QDir::homePath();
    updateRootLabel();
}

QStringList FileTreeWidget::nameFiltersForIndex(int index) const
{
    switch (index) {
    case 0: return QStringList{"*"};
    case 1: return QStringList{"*.cpp", "*.c", "*.h", "*.hpp", "*.hxx", "*.cxx",
                                      "*.cs", "*.java", "*.kt", "*.py", "*.lua",
                                      "*.js", "*.ts", "*.rs", "*.go", "*.swift",
                                      "*.glsl", "*.vert", "*.frag", "*.comp"};
    case 2: return QStringList{"*.txt", "*.md", "*.log", "*.ini", "*.cfg",
                                      "*.conf", "*.json", "*.xml", "*.yaml",
                                      "*.yml", "*.toml", "*.csv", "*.env"};
    case 3: return QStringList{"*.png", "*.jpg", "*.jpeg", "*.bmp", "*.tga",
                                      "*.dds", "*.tiff", "*.tif", "*.svg", "*.ico"};
    case 4: return QStringList{"*.fbx", "*.obj", "*.glb", "*.gltf", "*.dae",
                                      "*.stl", "*.kn5", "*.blend", "*.3ds", "*.dxf"};
    case 5: return QStringList{"*.wav", "*.mp3", "*.ogg", "*.flac", "*.aiff",
                                      "*.aif", "*.wma", "*.m4a", "*.aac"};
    default: return QStringList{"*"};
    }
}

void FileTreeWidget::setRootPath(const QString& path)
{
    if (path.isEmpty()) return;

    m_rootPath = path;
    QModelIndex rootIndex = m_fileModel->setRootPath(path);
    m_treeView->setRootIndex(m_filterProxy->mapFromSource(rootIndex));
    m_treeView->setColumnWidth(0, 220);

    updateRootLabel();
    emit rootPathChanged(path);
}

void FileTreeWidget::updateRootLabel()
{
    if (m_rootPath.isEmpty()) {
        m_rootLabel->setText("No project open");
    } else {
        QFileInfo fi(m_rootPath);
        m_rootLabel->setText(fi.absoluteFilePath());
    }
}

void FileTreeWidget::onDoubleClicked(const QModelIndex& index)
{
    QModelIndex sourceIndex = m_filterProxy->mapToSource(index);
    if (!sourceIndex.isValid()) return;

    if (m_fileModel->isDir(sourceIndex)) return;

    QString filePath = m_fileModel->filePath(sourceIndex);
    emit fileActivated(filePath);
}

void FileTreeWidget::onCustomContextMenu(const QPoint& pos)
{
    QModelIndex index = m_treeView->indexAt(pos);
    QModelIndex sourceIndex = index.isValid() ? m_filterProxy->mapToSource(index) : QModelIndex();

    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background: #2d2d2d; color: #ccc; border: 1px solid #555; }"
        "QMenu::item:selected { background: #094771; color: white; }"
        "QMenu::separator { height: 1px; background: #444; margin: 4px 8px; }"
    );

    if (sourceIndex.isValid()) {
        QString filePath = m_fileModel->filePath(sourceIndex);
        bool isDir = m_fileModel->isDir(sourceIndex);

        if (!isDir) {
            QAction* openAct = menu.addAction("Open in Editor");
            connect(openAct, &QAction::triggered, [this, filePath]() {
                emit fileActivated(filePath);
            });
        }

        QAction* openExternalAct = menu.addAction(isDir ? "Open in Explorer" : "Open with External Editor");
        connect(openExternalAct, &QAction::triggered, [filePath]() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
        });

        QAction* showInExplorerAct = menu.addAction("Show in Explorer");
        connect(showInExplorerAct, &QAction::triggered, [filePath]() {
#ifdef Q_OS_WIN
            QProcess::startDetached("explorer", {"/select,", QDir::toNativeSeparators(filePath)});
#else
            QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(filePath).absolutePath()));
#endif
        });

        menu.addSeparator();

        QAction* copyPathAct = menu.addAction("Copy Path");
        connect(copyPathAct, &QAction::triggered, [filePath]() {
            QApplication::clipboard()->setText(QDir::toNativeSeparators(filePath));
        });

        QAction* copyFileNameAct = menu.addAction("Copy File Name");
        connect(copyFileNameAct, &QAction::triggered, [filePath]() {
            QApplication::clipboard()->setText(QFileInfo(filePath).fileName());
        });

        menu.addSeparator();

        QAction* renameAct = menu.addAction("Rename...");
        connect(renameAct, &QAction::triggered, [this, sourceIndex]() {
            m_treeView->edit(m_filterProxy->mapFromSource(sourceIndex));
        });

        QAction* deleteAct = menu.addAction("Delete...");
        deleteAct->setEnabled(!isDir);
        connect(deleteAct, &QAction::triggered, [this, filePath, isDir]() {
            if (isDir) return;
            QString name = QFileInfo(filePath).fileName();
            auto reply = QMessageBox::question(this, "Delete File",
                QString("Are you sure you want to delete '%1'?").arg(name),
                QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                QFile::remove(filePath);
            }
        });

        menu.addSeparator();
    }

    QAction* newFileAct = menu.addAction("New File...");
    connect(newFileAct, &QAction::triggered, [this]() {
        QString dir = m_rootPath;
        QModelIndex current = m_treeView->currentIndex();
        if (current.isValid()) {
            QModelIndex src = m_filterProxy->mapToSource(current);
            QString path = m_fileModel->filePath(src);
            if (m_fileModel->isDir(src)) dir = path;
            else dir = QFileInfo(path).absolutePath();
        }
        bool ok;
        QString name = QInputDialog::getText(this, "New File", "File name:", QLineEdit::Normal, "", &ok);
        if (ok && !name.isEmpty()) {
            QFile file(dir + "/" + name);
            file.open(QIODevice::WriteOnly);
            file.close();
        }
    });

    QAction* newFolderAct = menu.addAction("New Folder...");
    connect(newFolderAct, &QAction::triggered, [this]() {
        QString dir = m_rootPath;
        QModelIndex current = m_treeView->currentIndex();
        if (current.isValid()) {
            QModelIndex src = m_filterProxy->mapToSource(current);
            QString path = m_fileModel->filePath(src);
            if (m_fileModel->isDir(src)) dir = path;
            else dir = QFileInfo(path).absolutePath();
        }
        bool ok;
        QString name = QInputDialog::getText(this, "New Folder", "Folder name:", QLineEdit::Normal, "", &ok);
        if (ok && !name.isEmpty()) {
            QDir().mkdir(dir + "/" + name);
        }
    });

    menu.exec(m_treeView->viewport()->mapToGlobal(pos));
}

void FileTreeWidget::onFilterChanged(int index)
{
    QStringList filters = nameFiltersForIndex(index);
    m_fileModel->setNameFilters(filters);
    m_fileModel->setNameFilterDisables(false);
}

void FileTreeWidget::onSearchTextChanged(const QString& text)
{
    m_filterProxy->setSearchText(text);
    if (!text.isEmpty()) {
        m_treeView->expandAll();
    }
}

void FileTreeWidget::onTreeCollapsed(const QModelIndex& index)
{
    QModelIndex srcIndex = m_filterProxy->mapToSource(index);
    if (srcIndex.isValid()) {
        QString path = m_fileModel->filePath(srcIndex);
        m_expandedPaths.removeAll(path);
    }
}

void FileTreeWidget::onTreeExpanded(const QModelIndex& index)
{
    QModelIndex srcIndex = m_filterProxy->mapToSource(index);
    if (srcIndex.isValid()) {
        QString path = m_fileModel->filePath(srcIndex);
        if (!m_expandedPaths.contains(path)) {
            m_expandedPaths.append(path);
        }
    }
}

void FileTreeWidget::refresh()
{
    if (!m_rootPath.isEmpty()) {
        m_fileModel->setRootPath(m_rootPath);
    }
}
