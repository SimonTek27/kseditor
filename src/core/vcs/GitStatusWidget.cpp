#include "GitStatusWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QDir>
#include <QTimer>

GitStatusWidget::GitStatusWidget(QWidget* parent)
    : QWidget(parent)
{
    m_git = new GitManager(this);
    setupUI();

    connect(m_git, &GitManager::statusReady, this, &GitStatusWidget::onStatusReady);
    connect(m_git, &GitManager::diffReady, this, &GitStatusWidget::onDiffReady);
    connect(m_git, &GitManager::commitDone, this, &GitStatusWidget::onCommitDone);
}

void GitStatusWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    // Branch + refresh bar
    QHBoxLayout* topBar = new QHBoxLayout();
    topBar->setSpacing(4);

    m_branchCombo = new QComboBox(this);
    m_branchCombo->setMinimumWidth(120);
    m_branchCombo->setStyleSheet(
        "QComboBox { background: #2d2d2d; color: #d4d4d4; border: 1px solid #3c3c3c; "
        "padding: 3px 6px; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView { background: #2d2d2d; color: #d4d4d4; selection-background-color: #264f78; }"
    );
    topBar->addWidget(m_branchCombo);

    m_refreshBtn = new QPushButton("Refresh", this);
    m_refreshBtn->setStyleSheet(
        "QPushButton { background: #0e639c; color: white; border: none; padding: 4px 12px; }"
        "QPushButton:hover { background: #1177bb; }"
    );
    topBar->addWidget(m_refreshBtn);
    topBar->addStretch();

    m_statusLabel = new QLabel("No repo", this);
    m_statusLabel->setStyleSheet("QLabel { color: #888; }");
    topBar->addWidget(m_statusLabel);

    mainLayout->addLayout(topBar);

    // Splitter: file tree + diff view
    QSplitter* splitter = new QSplitter(Qt::Vertical, this);

    m_fileTree = new QTreeWidget(this);
    m_fileTree->setHeaderLabels({"Status", "File"});
    m_fileTree->setColumnWidth(0, 60);
    m_fileTree->header()->setStretchLastSection(true);
    m_fileTree->setRootIsDecorated(false);
    m_fileTree->setAlternatingRowColors(true);
    m_fileTree->setStyleSheet(
        "QTreeWidget { background: #1e1e1e; color: #d4d4d4; border: 1px solid #3c3c3c; font-family: Consolas, monospace; }"
        "QTreeWidget::item:selected { background: #264f78; }"
        "QTreeWidget::item { padding: 2px 4px; }"
        "QHeaderView::section { background: #2d2d2d; color: #d4d4d4; border: 1px solid #3c3c3c; padding: 4px; }"
    );
    splitter->addWidget(m_fileTree);

    m_diffView = new QTextEdit(this);
    m_diffView->setReadOnly(true);
    m_diffView->setStyleSheet(
        "QTextEdit { background: #1e1e1e; color: #d4d4d4; font-family: Consolas, monospace; font-size: 11px; border: 1px solid #3c3c3c; }"
    );
    // setMaximumBlockCount not available on QTextEdit in Qt6
    splitter->addWidget(m_diffView);

    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);
    mainLayout->addWidget(splitter, 1);

    // Action bar
    QHBoxLayout* actionBar = new QHBoxLayout();
    actionBar->setSpacing(4);

    m_stageBtn = new QPushButton("Stage", this);
    m_stageBtn->setStyleSheet(
        "QPushButton { background: #2d8a2d; color: white; border: none; padding: 4px 12px; }"
        "QPushButton:hover { background: #3aa33a; }"
        "QPushButton:disabled { background: #333; color: #666; }"
    );
    actionBar->addWidget(m_stageBtn);

    m_unstageBtn = new QPushButton("Unstage", this);
    m_unstageBtn->setStyleSheet(
        "QPushButton { background: #8a2d2d; color: white; border: none; padding: 4px 12px; }"
        "QPushButton:hover { background: #a33a3a; }"
        "QPushButton:disabled { background: #333; color: #666; }"
    );
    actionBar->addWidget(m_unstageBtn);

    actionBar->addStretch();

    m_commitMsg = new QLineEdit(this);
    m_commitMsg->setPlaceholderText("Commit message...");
    m_commitMsg->setStyleSheet(
        "QLineEdit { background: #2d2d2d; color: #d4d4d4; border: 1px solid #3c3c3c; padding: 4px 6px; }"
    );
    actionBar->addWidget(m_commitMsg, 1);

    m_commitBtn = new QPushButton("Commit", this);
    m_commitBtn->setStyleSheet(
        "QPushButton { background: #0e639c; color: white; border: none; padding: 4px 16px; }"
        "QPushButton:hover { background: #1177bb; }"
        "QPushButton:disabled { background: #333; color: #666; }"
    );
    actionBar->addWidget(m_commitBtn);

    mainLayout->addLayout(actionBar);

    // Connections
    connect(m_refreshBtn, &QPushButton::clicked, this, &GitStatusWidget::onRefreshClicked);
    connect(m_fileTree, &QTreeWidget::itemClicked, this, &GitStatusWidget::onItemClicked);
    connect(m_stageBtn, &QPushButton::clicked, this, &GitStatusWidget::onStageClicked);
    connect(m_unstageBtn, &QPushButton::clicked, this, &GitStatusWidget::onUnstageClicked);
    connect(m_commitBtn, &QPushButton::clicked, this, &GitStatusWidget::onCommitClicked);
    connect(m_branchCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GitStatusWidget::onBranchChanged);
}

void GitStatusWidget::setRepoPath(const QString& path)
{
    m_repoPath = path;
    m_git->setRepoPath(path);

    if (path.isEmpty() || !m_git->isValidRepo()) {
        m_statusLabel->setText("Not a git repo");
        return;
    }

    m_statusLabel->setText("Git: " + QDir(path).dirName());
    m_git->fetchBranches();
    refresh();
}

void GitStatusWidget::refresh()
{
    if (m_repoPath.isEmpty()) return;
    m_git->fetchStatus();
}

void GitStatusWidget::onStatusReady(const QList<GitFileStatus>& files)
{
    m_currentFiles = files;
    updateFileTree(files);
}

void GitStatusWidget::updateFileTree(const QList<GitFileStatus>& files)
{
    m_fileTree->clear();

    if (files.isEmpty()) {
        m_statusLabel->setText("No changes");
        return;
    }

    int staged = 0, unstaged = 0, untracked = 0;
    for (const auto& f : files) {
        if (f.isStaged()) ++staged;
        if (f.isUnstaged()) ++unstaged;
        if (f.isUntracked()) ++untracked;
    }

    m_statusLabel->setText(QString("%1 staged, %2 unstaged, %3 untracked")
        .arg(staged).arg(unstaged).arg(untracked));

    for (const auto& f : files) {
        QStringList cols;
        QString statusChar;
        QColor color;

        if (f.isStaged()) {
            statusChar = f.stagedStatus;
            color = QColor("#6a9955"); // green
        } else if (f.isUnstaged()) {
            statusChar = f.unstagedStatus;
            color = f.isUntracked() ? QColor("#888") : QColor("#ce9178"); // gray or orange
        }

        cols << statusChar << f.path;

        QTreeWidgetItem* item = new QTreeWidgetItem(cols);
        item->setData(0, Qt::ForegroundRole, color);
        item->setData(1, Qt::ForegroundRole, color);
        item->setData(0, Qt::UserRole, f.path);
        item->setToolTip(1, f.path);
        m_fileTree->addTopLevelItem(item);
    }
}

void GitStatusWidget::onDiffReady(const QString& filePath, const QString& diff)
{
    m_diffView->clear();

    if (diff.trimmed().isEmpty()) {
        m_diffView->setPlainText("(no changes)");
        return;
    }

    // Colorize diff output with HTML
    QString html;
    QStringList lines = diff.split('\n');
    for (const QString& line : lines) {
        if (line.startsWith("+++") || line.startsWith("---") || line.startsWith("@@")) {
            html += "<span style='color:#569cd6;'>" + line.toHtmlEscaped() + "</span>\n";
        } else if (line.startsWith("+")) {
            html += "<span style='color:#6a9955;'>" + line.toHtmlEscaped() + "</span>\n";
        } else if (line.startsWith("-")) {
            html += "<span style='color:#ce9178;'>" + line.toHtmlEscaped() + "</span>\n";
        } else {
            html += line.toHtmlEscaped() + "\n";
        }
    }
    m_diffView->setHtml("<pre style='margin:0; font-family: Consolas, monospace; font-size:11px; color:#d4d4d4;'>" + html + "</pre>");
}

void GitStatusWidget::onCommitDone(bool success, const QString& output)
{
    if (success) {
        m_commitMsg->clear();
        m_statusLabel->setText("Commit successful!");
        refresh();
    } else {
        m_statusLabel->setText("Commit failed: " + output);
    }
}

void GitStatusWidget::onItemClicked(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);
    if (!item) return;

    QString filePath = item->data(0, Qt::UserRole).toString();
    if (filePath.isEmpty()) return;

    // Fetch diff
    m_diffView->setPlainText("Loading diff...");
    m_git->fetchDiff(filePath);

    // If double-click, open file
    // (track single vs double click with a simple approach)
}

void GitStatusWidget::onStageClicked()
{
    QTreeWidgetItem* item = m_fileTree->currentItem();
    if (!item) return;

    QString filePath = item->data(0, Qt::UserRole).toString();
    if (filePath.isEmpty()) return;

    m_git->stageFile(filePath);
    m_statusLabel->setText("Staging " + filePath + "...");
    // Refresh after a short delay
    QTimer::singleShot(500, this, &GitStatusWidget::refresh);
}

void GitStatusWidget::onUnstageClicked()
{
    QTreeWidgetItem* item = m_fileTree->currentItem();
    if (!item) return;

    QString filePath = item->data(0, Qt::UserRole).toString();
    if (filePath.isEmpty()) return;

    m_git->unstageFile(filePath);
    m_statusLabel->setText("Unstaging " + filePath + "...");
    QTimer::singleShot(500, this, &GitStatusWidget::refresh);
}

void GitStatusWidget::onCommitClicked()
{
    QString msg = m_commitMsg->text().trimmed();
    if (msg.isEmpty()) {
        m_statusLabel->setText("Enter a commit message");
        return;
    }

    m_git->commit(msg);
    m_statusLabel->setText("Committing...");
}

void GitStatusWidget::onRefreshClicked()
{
    m_git->fetchBranches();
    refresh();
}

void GitStatusWidget::onBranchChanged(int index)
{
    Q_UNUSED(index);
    // Could implement branch checkout here
}
