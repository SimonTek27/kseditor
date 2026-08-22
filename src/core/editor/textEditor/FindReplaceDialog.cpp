#include "FindReplaceDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QTextDocument>

namespace ks {

FindReplaceDialog::FindReplaceDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Find / Replace");
    setMinimumWidth(450);
    setModal(false);

    auto* layout = new QVBoxLayout(this);

    auto* findGroup = new QGroupBox("Find", this);
    auto* findLayout = new QVBoxLayout(findGroup);
    m_findInput = new QLineEdit(this);
    m_findInput->setPlaceholderText("Search text...");
    findLayout->addWidget(m_findInput);

    auto* replaceGroup = new QGroupBox("Replace", this);
    auto* replaceLayout = new QVBoxLayout(replaceGroup);
    m_replaceInput = new QLineEdit(this);
    m_replaceInput->setPlaceholderText("Replace with...");
    replaceLayout->addWidget(m_replaceInput);

    m_caseSensitive = new QCheckBox("Case sensitive", this);
    findLayout->addWidget(m_caseSensitive);

    auto* btnLayout = new QHBoxLayout();
    auto* findBtn = new QPushButton("Find Next", this);
    findBtn->setStyleSheet("QPushButton { background: #3a5a8a; color: #fff; border: 1px solid #4a6a9a; padding: 5px 12px; }");
    auto* replaceBtn = new QPushButton("Replace", this);
    replaceBtn->setStyleSheet("QPushButton { background: #5a6a3a; color: #fff; border: 1px solid #6a7a4a; padding: 5px 12px; }");
    auto* replaceAllBtn = new QPushButton("Replace All", this);
    replaceAllBtn->setStyleSheet("QPushButton { background: #7a4a3a; color: #fff; border: 1px solid #8a5a4a; padding: 5px 12px; }");
    btnLayout->addWidget(findBtn);
    btnLayout->addWidget(replaceBtn);
    btnLayout->addWidget(replaceAllBtn);
    layout->addWidget(findGroup);
    layout->addWidget(replaceGroup);
    layout->addLayout(btnLayout);

    m_resultLabel = new QLabel(this);
    m_resultLabel->setStyleSheet("color: #888;");
    layout->addWidget(m_resultLabel);

    connect(findBtn, &QPushButton::clicked, this, &FindReplaceDialog::onFindNext);
    connect(replaceBtn, &QPushButton::clicked, this, &FindReplaceDialog::onReplace);
    connect(replaceAllBtn, &QPushButton::clicked, this, &FindReplaceDialog::onReplaceAll);
    connect(m_findInput, &QLineEdit::returnPressed, this, &FindReplaceDialog::onFindNext);
}

void FindReplaceDialog::onFindNext()
{
    if (!m_editor) return;
    QString text = m_findInput->text();
    if (text.isEmpty()) return;

    QTextDocument::FindFlags flags;
    if (m_caseSensitive->isChecked()) flags |= QTextDocument::FindCaseSensitively;

    bool found = m_editor->find(text, flags);
    if (!found) {
        QTextCursor cursor = m_editor->textCursor();
        cursor.movePosition(QTextCursor::Start);
        m_editor->setTextCursor(cursor);
        found = m_editor->find(text, flags);
    }

    m_resultLabel->setText(found ? "" : "No more results found");
}

void FindReplaceDialog::onReplace()
{
    if (!m_editor) return;
    QString text = m_findInput->text();
    QString replacement = m_replaceInput->text();
    if (text.isEmpty()) return;

    QTextCursor cursor = m_editor->textCursor();
    if (cursor.hasSelection() && cursor.selectedText() == text) {
        cursor.insertText(replacement);
    }
    onFindNext();
}

void FindReplaceDialog::onReplaceAll()
{
    if (!m_editor) return;
    QString text = m_findInput->text();
    QString replacement = m_replaceInput->text();
    if (text.isEmpty()) return;

    QTextDocument::FindFlags flags;
    if (m_caseSensitive->isChecked()) flags |= QTextDocument::FindCaseSensitively;

    QTextCursor cursor = m_editor->textCursor();
    cursor.movePosition(QTextCursor::Start);
    m_editor->setTextCursor(cursor);

    int count = 0;
    while (m_editor->find(text, flags)) {
        m_editor->textCursor().insertText(replacement);
        ++count;
    }

    m_resultLabel->setText(QString("Replaced %1 occurrence(s)").arg(count));
}

} // namespace ks
