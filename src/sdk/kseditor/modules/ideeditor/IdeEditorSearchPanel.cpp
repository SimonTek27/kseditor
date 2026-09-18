#include "IdeEditorSearchPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QTextStream>
#include <QDirIterator>
#include <QFileInfo>
#include <QApplication>

namespace ks {

IdeEditorSearchPanel::IdeEditorSearchPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void IdeEditorSearchPanel::setupUI()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    // Search row
    auto* searchRow = new QHBoxLayout();
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Search in files...");
    m_searchEdit->setStyleSheet(
        "QLineEdit { background: #252526; border: 1px solid #3a3a3a; color: #d4d4d4; padding: 2px 6px; }");
    searchRow->addWidget(m_searchEdit, 1);

    m_searchBtn = new QPushButton("Go", this);
    m_searchBtn->setFixedWidth(32);
    m_searchBtn->setStyleSheet(
        "QPushButton { background: #3a5a8a; color: #fff; border: 1px solid #4a6a9a; padding: 2px 6px; }"
        "QPushButton:hover { background: #4a6a9a; }");
    searchRow->addWidget(m_searchBtn);

    m_clearBtn = new QPushButton("X", this);
    m_clearBtn->setFixedWidth(24);
    m_clearBtn->setToolTip("Clear results");
    m_clearBtn->setStyleSheet(
        "QPushButton { background: #5a3a3a; color: #fff; border: 1px solid #6a4a4a; padding: 2px; }"
        "QPushButton:hover { background: #6a4a4a; }");
    searchRow->addWidget(m_clearBtn);

    layout->addLayout(searchRow);

    // Options row
    auto* optionsRow = new QHBoxLayout();
    m_caseSensitiveChk = new QCheckBox("Match case", this);
    m_caseSensitiveChk->setStyleSheet("color: #d4d4d4;");
    optionsRow->addWidget(m_caseSensitiveChk);

    m_filePatternCombo = new QComboBox(this);
    m_filePatternCombo->setEditable(true);
    m_filePatternCombo->addItems({"*.*", "*.cpp *.h", "*.qml", "*.lua", "*.json", "*.ini"});
    m_filePatternCombo->setCurrentText("*.*");
    m_filePatternCombo->setStyleSheet(
        "QComboBox { background: #252526; border: 1px solid #3a3a3a; color: #d4d4d4; padding: 2px; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView { background: #252526; color: #d4d4d4; selection-background-color: #094771; }");
    optionsRow->addWidget(m_filePatternCombo);

    layout->addLayout(optionsRow);

    // Results
    m_statusLabel = new QLabel("Ready", this);
    m_statusLabel->setStyleSheet("color: #888; font-size: 11px; padding: 2px 0;");
    layout->addWidget(m_statusLabel);

    m_resultsList = new QListWidget(this);
    m_resultsList->setStyleSheet(
        "QListWidget { background: #1e1e1e; color: #d4d4d4; border: 1px solid #3a3a3a; }"
        "QListWidget::item { padding: 2px 4px; }"
        "QListWidget::item:selected { background: #094771; }"
        "QListWidget::item:hover { background: #2a2d2e; }");
    layout->addWidget(m_resultsList, 1);

    connect(m_searchBtn, &QPushButton::clicked, this, &IdeEditorSearchPanel::onSearch);
    connect(m_clearBtn, &QPushButton::clicked, this, &IdeEditorSearchPanel::onClear);
    connect(m_resultsList, &QListWidget::itemClicked, this, &IdeEditorSearchPanel::onResultClicked);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &IdeEditorSearchPanel::onSearch);
}

void IdeEditorSearchPanel::setRootPath(const QString& path)
{
    m_rootPath = path;
}

void IdeEditorSearchPanel::onSearch()
{
    m_resultsList->clear();

    QString term = m_searchEdit->text();
    if (term.isEmpty() || m_rootPath.isEmpty()) return;

    bool caseSensitive = m_caseSensitiveChk->isChecked();
    QStringList patterns = m_filePatternCombo->currentText().split(' ', Qt::SkipEmptyParts);
    if (patterns.isEmpty()) patterns << "*.*";

    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_statusLabel->setText("Searching...");

    int totalMatches = 0;
    int fileCount = 0;

    for (const QString& pattern : patterns) {
        QDirIterator it(m_rootPath, QStringList() << pattern.trimmed(),
            QDir::Files, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);

        while (it.hasNext()) {
            QString filePath = it.next();
            if (filePath.contains(".git/") || filePath.contains("node_modules/"))
                continue;

            QStringList matches = searchFile(filePath, term, caseSensitive);
            if (!matches.isEmpty()) {
                fileCount++;
                for (const QString& match : matches) {
                    auto* item = new QListWidgetItem(match);
                    item->setData(Qt::UserRole, filePath);
                    int lineStart = match.indexOf(":") + 1;
                    int lineEnd = match.indexOf(":", lineStart);
                    if (lineEnd > lineStart) {
                        item->setData(Qt::UserRole + 1, match.mid(lineStart, lineEnd - lineStart).toInt());
                    }
                    m_resultsList->addItem(item);
                    totalMatches++;
                }
            }
        }
    }

    m_statusLabel->setText(QString("Found %1 matches in %2 files").arg(totalMatches).arg(fileCount));
    QApplication::restoreOverrideCursor();
}

QStringList IdeEditorSearchPanel::searchFile(const QString& filePath, const QString& term, bool caseSensitive)
{
    QStringList results;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return results;

    QTextStream in(&file);
    QString fileName = QFileInfo(filePath).fileName();
    int lineNum = 0;

    Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;

    while (!in.atEnd()) {
        QString line = in.readLine();
        lineNum++;
        if (line.contains(term, cs)) {
            results.append(QString("%1:%2: %3").arg(fileName).arg(lineNum).arg(line.trimmed().left(120)));
        }
    }

    file.close();
    return results;
}

void IdeEditorSearchPanel::onResultClicked(QListWidgetItem* item)
{
    if (!item) return;
    QString filePath = item->data(Qt::UserRole).toString();
    int line = item->data(Qt::UserRole + 1).toInt();
    emit navigateToResult(filePath, line);
}

void IdeEditorSearchPanel::onClear()
{
    m_resultsList->clear();
    m_searchEdit->clear();
    m_statusLabel->setText("Ready");
}

} // namespace ks
