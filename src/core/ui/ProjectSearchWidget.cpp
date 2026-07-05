#include "ProjectSearchWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QDirIterator>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QHeaderView>
#include <QFileInfo>
#include <QDir>

// ── ProjectSearchWorker ──

ProjectSearchWorker::ProjectSearchWorker(const QString& rootPath, const QString& searchText,
                                         const QStringList& filePatterns, bool caseSensitive, bool wholeWord)
    : m_rootPath(rootPath)
    , m_searchText(searchText)
    , m_filePatterns(filePatterns)
    , m_caseSensitive(caseSensitive)
    , m_wholeWord(wholeWord)
{
}

void ProjectSearchWorker::run()
{
    if (m_rootPath.isEmpty() || m_searchText.isEmpty()) {
        emit searchError("Search root or search text is empty");
        return;
    }

    QDir rootDir(m_rootPath);
    if (!rootDir.exists()) {
        emit searchError("Search root does not exist: " + m_rootPath);
        return;
    }

    // Build name filters from patterns
    QStringList nameFilters;
    for (const QString& pattern : m_filePatterns) {
        QString trimmed = pattern.trimmed();
        if (!trimmed.isEmpty()) {
            if (!trimmed.startsWith("*.") && !trimmed.contains('*')) {
                nameFilters << ("*." + trimmed);
            } else {
                nameFilters << trimmed;
            }
        }
    }
    if (nameFilters.isEmpty()) nameFilters << "*";

    QDirIterator it(m_rootPath, nameFilters, QDir::Files, QDirIterator::Subdirectories);
    int totalFiles = 0;
    int totalMatches = 0;

    while (it.hasNext()) {
        if (m_cancelled.loadRelaxed()) return;

        it.next();
        ++totalFiles;
        searchFile(it.filePath(), totalMatches);

        // Update every 50 files to avoid UI lock
        if (totalFiles % 50 == 0) {
            QThread::yieldCurrentThread();
        }
    }

    emit searchCompleted(totalFiles, totalMatches);
}

void ProjectSearchWorker::searchFile(const QString& filePath, int& totalMatches)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QTextStream in(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    in.setCodec("UTF-8");
#endif

    Qt::CaseSensitivity cs = m_caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    int lineNumber = 0;
    int localMatches = 0;

    while (!in.atEnd()) {
        if (m_cancelled.loadRelaxed()) return;

        QString line = in.readLine();
        ++lineNumber;

        int idx = -1;
        if (m_wholeWord) {
            QRegularExpression regex(
                "\\b" + QRegularExpression::escape(m_searchText) + "\\b",
                m_caseSensitive ? QRegularExpression::NoPatternOption
                                : QRegularExpression::CaseInsensitiveOption);
            QRegularExpressionMatch match = regex.match(line);
            if (match.hasMatch()) {
                idx = match.capturedStart();
            }
        } else {
            idx = line.indexOf(m_searchText, 0, cs);
        }

        if (idx >= 0) {
            SearchResultItem item;
            item.filePath = filePath;
            item.lineNumber = lineNumber;
            item.lineText = line.trimmed();
            item.matchStart = idx;
            item.matchLength = m_searchText.length();
            emit resultFound(item);
            ++localMatches;
            ++totalMatches;
        }
    }

    file.close();
}

// ── ProjectSearchWidget ──

ProjectSearchWidget::ProjectSearchWidget(QWidget* parent)
    : QWidget(parent)
    , m_searchInput(nullptr)
    , m_replaceInput(nullptr)
    , m_patternInput(nullptr)
    , m_caseSensitive(nullptr)
    , m_wholeWord(nullptr)
    , m_searchBtn(nullptr)
    , m_stopBtn(nullptr)
    , m_replaceAllBtn(nullptr)
    , m_clearBtn(nullptr)
    , m_statusLabel(nullptr)
    , m_resultTree(nullptr)
    , m_workerThread(nullptr)
    , m_worker(nullptr)
    , m_matchCount(0)
{
    setupUI();
}

ProjectSearchWidget::~ProjectSearchWidget()
{
    stopSearch();
}

void ProjectSearchWidget::setupUI()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    // Search row
    auto* searchRow = new QHBoxLayout();
    m_searchInput = new QLineEdit(this);
    m_searchInput->setPlaceholderText("Search text...");
    searchRow->addWidget(m_searchInput, 1);

    m_searchBtn = new QPushButton("Search", this);
    m_searchBtn->setStyleSheet(
        "QPushButton { background: #3a5a8a; color: #fff; border: 1px solid #4a6a9a;"
        "  padding: 4px 12px; border-radius: 3px; }"
        "QPushButton:hover { background: #4a6a9a; }");
    searchRow->addWidget(m_searchBtn);

    m_stopBtn = new QPushButton("Stop", this);
    m_stopBtn->setEnabled(false);
    m_stopBtn->setStyleSheet(
        "QPushButton { background: #8a3a3a; color: #fff; border: 1px solid #9a4a4a;"
        "  padding: 4px 12px; border-radius: 3px; }"
        "QPushButton:hover { background: #9a4a4a; }");
    searchRow->addWidget(m_stopBtn);

    layout->addLayout(searchRow);

    // Options row
    auto* optionsRow = new QHBoxLayout();
    m_caseSensitive = new QCheckBox("Case sensitive", this);
    optionsRow->addWidget(m_caseSensitive);
    m_wholeWord = new QCheckBox("Whole word", this);
    optionsRow->addWidget(m_wholeWord);
    optionsRow->addStretch();
    layout->addLayout(optionsRow);

    // File pattern row
    auto* patternRow = new QHBoxLayout();
    auto* patternLabel = new QLabel("Files:", this);
    patternLabel->setStyleSheet("color: #aaa;");
    patternRow->addWidget(patternLabel);
    m_patternInput = new QLineEdit(this);
    m_patternInput->setPlaceholderText("e.g. *.cpp *.h or * for all");
    m_patternInput->setText("*");
    patternRow->addWidget(m_patternInput, 1);
    layout->addLayout(patternRow);

    // Replace row
    auto* replaceRow = new QHBoxLayout();
    m_replaceInput = new QLineEdit(this);
    m_replaceInput->setPlaceholderText("Replace with...");
    replaceRow->addWidget(m_replaceInput, 1);
    m_replaceAllBtn = new QPushButton("Replace All", this);
    m_replaceAllBtn->setEnabled(false);
    m_replaceAllBtn->setStyleSheet(
        "QPushButton { background: #7a4a3a; color: #fff; border: 1px solid #8a5a4a;"
        "  padding: 4px 12px; border-radius: 3px; }"
        "QPushButton:hover { background: #8a5a4a; }");
    replaceRow->addWidget(m_replaceAllBtn);
    layout->addLayout(replaceRow);

    // Result tree
    m_resultTree = new QTreeWidget(this);
    m_resultTree->setHeaderLabels({"File / Match", "Line", "Path"});
    m_resultTree->setColumnWidth(0, 400);
    m_resultTree->setColumnWidth(1, 60);
    m_resultTree->setColumnWidth(2, 200);
    m_resultTree->setRootIsDecorated(true);
    m_resultTree->setAlternatingRowColors(false);
    m_resultTree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_resultTree->setStyleSheet(
        "QTreeWidget { background: #1e1e1e; color: #d4d4d4; border: 1px solid #333; font: 12px Consolas; }"
        "QTreeWidget::item { padding: 2px 0; }"
        "QTreeWidget::item:selected { background: #094771; color: #fff; }"
        "QHeaderView::section { background: #2d2d2d; color: #aaa; border: 1px solid #3a3a3a; padding: 3px; }"
    );
    layout->addWidget(m_resultTree, 1);

    // Status bar
    m_statusLabel = new QLabel("Ready", this);
    m_statusLabel->setStyleSheet("color: #888; padding: 2px 0; font-size: 11px;");
    layout->addWidget(m_statusLabel);

    m_clearBtn = new QPushButton("Clear", this);
    m_clearBtn->setStyleSheet(
        "QPushButton { background: #3a3a3a; color: #ccc; border: 1px solid #555;"
        "  padding: 2px 8px; border-radius: 3px; }"
        "QPushButton:hover { background: #4a4a4a; }");

    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();
    btnRow->addWidget(m_clearBtn);
    layout->addLayout(btnRow);

    // Connections
    connect(m_searchBtn, &QPushButton::clicked, this, &ProjectSearchWidget::startSearch);
    connect(m_stopBtn, &QPushButton::clicked, this, &ProjectSearchWidget::stopSearch);
    connect(m_clearBtn, &QPushButton::clicked, this, &ProjectSearchWidget::clearResults);
    connect(m_replaceAllBtn, &QPushButton::clicked, this, &ProjectSearchWidget::onReplaceAll);
    connect(m_searchInput, &QLineEdit::returnPressed, this, &ProjectSearchWidget::startSearch);
    connect(m_resultTree, &QTreeWidget::itemActivated, this, &ProjectSearchWidget::onItemActivated);
    connect(m_resultTree, &QTreeWidget::customContextMenuRequested, this, &ProjectSearchWidget::onContextMenu);
}

void ProjectSearchWidget::setSearchRoot(const QString& path)
{
    m_searchRoot = path;
}

void ProjectSearchWidget::focusSearchInput()
{
    m_searchInput->setFocus();
    m_searchInput->selectAll();
}

void ProjectSearchWidget::startSearch()
{
    if (m_searchRoot.isEmpty()) {
        m_statusLabel->setText("No search root set (open a project first)");
        return;
    }

    QString text = m_searchInput->text().trimmed();
    if (text.isEmpty()) return;

    clearResults();

    // Parse file patterns
    QStringList patterns = m_patternInput->text().split(' ', Qt::SkipEmptyParts);
    if (patterns.isEmpty()) patterns << "*";

    m_searchBtn->setEnabled(false);
    m_stopBtn->setEnabled(true);
    m_replaceAllBtn->setEnabled(false);
    m_statusLabel->setText("Searching...");
    m_matchCount = 0;

    emit searchStarted();

    m_worker = new ProjectSearchWorker(
        m_searchRoot, text, patterns,
        m_caseSensitive->isChecked(), m_wholeWord->isChecked()
    );

    m_workerThread = new QThread(this);
    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::started, m_worker, &ProjectSearchWorker::run);
    connect(m_worker, &ProjectSearchWorker::resultFound, this, &ProjectSearchWidget::onResultFound);
    connect(m_worker, &ProjectSearchWorker::searchCompleted, this, &ProjectSearchWidget::onSearchCompleted);
    connect(m_worker, &ProjectSearchWorker::searchError, this, &ProjectSearchWidget::onSearchError);
    connect(m_worker, &ProjectSearchWorker::searchCompleted, m_workerThread, &QThread::quit);
    connect(m_worker, &ProjectSearchWorker::searchError, m_workerThread, &QThread::quit);
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_workerThread, &QThread::finished, m_workerThread, &QObject::deleteLater);

    m_workerThread->start();
}

void ProjectSearchWidget::stopSearch()
{
    if (m_worker) {
        m_worker->stop();
        m_worker->disconnect();
    }
    if (m_workerThread && m_workerThread->isRunning()) {
        m_workerThread->disconnect();
        m_workerThread->quit();
        m_workerThread->wait(2000);
    }
    m_worker = nullptr;
    m_workerThread = nullptr;

    m_searchBtn->setEnabled(true);
    m_stopBtn->setEnabled(false);
    m_statusLabel->setText("Search cancelled");
}

void ProjectSearchWidget::clearResults()
{
    m_resultTree->clear();
    m_fileGroups.clear();
    m_matchCount = 0;
    m_statusLabel->setText("Ready");
    m_replaceAllBtn->setEnabled(false);
}

void ProjectSearchWidget::onResultFound(const SearchResultItem& item)
{
    QTreeWidgetItem* group = getOrCreateFileGroup(item.filePath);
    if (!group) return;

    QString display = QString("  %1  %2")
        .arg(item.lineNumber, 4)
        .arg(item.lineText.left(200));

    auto* child = new QTreeWidgetItem();
    child->setText(0, display);
    child->setText(1, QString::number(item.lineNumber));
    child->setText(2, QFileInfo(item.filePath).absolutePath());
    child->setData(0, Qt::UserRole, item.filePath);
    child->setData(0, Qt::UserRole + 1, item.lineNumber);
    child->setForeground(0, QColor(0xd4, 0xd4, 0xd4));
    child->setForeground(1, QColor(0x6a, 0x9a, 0xdb));

    group->addChild(child);
    ++m_matchCount;
}

void ProjectSearchWidget::onSearchCompleted(int totalFiles, int totalMatches)
{
    m_searchBtn->setEnabled(true);
    m_stopBtn->setEnabled(false);
    m_replaceAllBtn->setEnabled(m_matchCount > 0);

    m_statusLabel->setText(
        QString("Found %1 matches in %2 files (%3 files searched)")
            .arg(m_matchCount).arg(m_fileGroups.size()).arg(totalFiles));

    if (m_resultTree->topLevelItemCount() > 0) {
        m_resultTree->expandAll();
    }

    emit searchFinished(totalFiles, m_matchCount);
}

void ProjectSearchWidget::onSearchError(const QString& error)
{
    m_searchBtn->setEnabled(true);
    m_stopBtn->setEnabled(false);
    m_statusLabel->setText("Error: " + error);
}

void ProjectSearchWidget::onItemActivated(QTreeWidgetItem* item, int)
{
    // Only handle leaf items (matches), not file group headers
    if (item->childCount() > 0) return;
    if (!item->data(0, Qt::UserRole).isValid()) return;

    QString filePath = item->data(0, Qt::UserRole).toString();
    int line = item->data(0, Qt::UserRole + 1).toInt();

    emit resultActivated(filePath, line);
}

void ProjectSearchWidget::onContextMenu(const QPoint& pos)
{
    QTreeWidgetItem* item = m_resultTree->itemAt(pos);

    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background: #2d2d2d; color: #ccc; border: 1px solid #555; }"
        "QMenu::item:selected { background: #094771; color: white; }"
        "QMenu::separator { height: 1px; background: #444; margin: 4px 8px; }"
    );

    if (item) {
        QAction* openAct = menu.addAction("Open File");
        connect(openAct, &QAction::triggered, [this, item]() {
            onItemActivated(item, 0);
        });

        QAction* copyPathAct = menu.addAction("Copy File Path");
        connect(copyPathAct, &QAction::triggered, [item]() {
            QString filePath = item->data(0, Qt::UserRole).toString();
            if (filePath.isEmpty()) {
                // It's a group header, get path from text
                filePath = item->text(2) + "/" + item->text(0);
            }
            QApplication::clipboard()->setText(filePath);
        });

        menu.addSeparator();
    }

    QAction* copyAllAct = menu.addAction("Copy All Results");
    connect(copyAllAct, &QAction::triggered, [this]() {
        QString text;
        for (int i = 0; i < m_resultTree->topLevelItemCount(); ++i) {
            auto* group = m_resultTree->topLevelItem(i);
            text += group->text(0) + "\n";
            for (int j = 0; j < group->childCount(); ++j) {
                auto* child = group->child(j);
                text += QString("  %1: %2\n")
                    .arg(child->text(1))
                    .arg(child->text(0).mid(6));
            }
        }
        QApplication::clipboard()->setText(text);
    });

    QAction* clearAct = menu.addAction("Clear Results");
    connect(clearAct, &QAction::triggered, this, &ProjectSearchWidget::clearResults);

    menu.exec(m_resultTree->viewport()->mapToGlobal(pos));
}

void ProjectSearchWidget::onReplaceAll()
{
    QString findText = m_searchInput->text().trimmed();
    QString replaceText = m_replaceInput->text();
    if (findText.isEmpty()) return;

    if (m_matchCount == 0) {
        m_statusLabel->setText("No matches to replace");
        return;
    }

    auto reply = QMessageBox::question(this, "Replace All",
        QString("Replace all %1 occurrences of '%2' with '%3'?")
            .arg(m_matchCount).arg(findText).arg(replaceText),
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    Qt::CaseSensitivity cs = m_caseSensitive->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;
    int replaced = 0;

    for (int i = 0; i < m_resultTree->topLevelItemCount(); ++i) {
        auto* group = m_resultTree->topLevelItem(i);
        for (int j = 0; j < group->childCount(); ++j) {
            auto* child = group->child(j);
            QString filePath = child->data(0, Qt::UserRole).toString();
            if (filePath.isEmpty()) continue;

            // Read the whole file
            QFile file(filePath);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
            QString content = file.readAll();
            file.close();

            // Perform replacement
            QString newContent;
            if (m_wholeWord->isChecked()) {
                QRegularExpression regex(
                    "\\b" + QRegularExpression::escape(findText) + "\\b",
                    m_caseSensitive->isChecked() ? QRegularExpression::NoPatternOption
                                                  : QRegularExpression::CaseInsensitiveOption);
                newContent = content.replace(regex, replaceText);
            } else {
                newContent = content.replace(findText, replaceText, cs);
            }

            if (newContent != content) {
                QFile writeFile(filePath);
                if (writeFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    writeFile.write(newContent.toUtf8());
                    writeFile.close();
                    ++replaced;
                }
            }
        }
    }

    m_statusLabel->setText(QString("Replaced in %1 files").arg(replaced));
    clearResults();
}

QTreeWidgetItem* ProjectSearchWidget::getOrCreateFileGroup(const QString& filePath)
{
    if (m_fileGroups.contains(filePath)) {
        return m_fileGroups[filePath];
    }

    QFileInfo fi(filePath);
    auto* group = new QTreeWidgetItem();
    group->setText(0, fi.fileName());
    group->setText(2, fi.absolutePath());
    group->setForeground(0, QColor(0x6a, 0x9a, 0xdb));
    group->setData(0, Qt::UserRole, filePath);

    // File icon indicator
    QFont font = group->font(0);
    font.setBold(true);
    group->setFont(0, font);

    m_resultTree->addTopLevelItem(group);
    m_fileGroups[filePath] = group;

    return group;
}
