#include "IniEditorWidget.h"
#include "IniSyntaxHighlighter.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QTextStream>
#include <QFile>
#include <QTextCursor>
#include <QSignalBlocker>

namespace ks {

IniEditorWidget::IniEditorWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_textEdit = new QTextEdit(this);
    m_textEdit->setFont(QFont("Consolas", 10));
    m_textEdit->setTabStopDistance(40);
    m_highlighter = new IniSyntaxHighlighter(m_textEdit->document());

    m_lineLabel = new QLabel(tr("Line: 1"), this);
    m_statusLabel = new QLabel(tr("Ready"), this);

    auto* toolbar = new QHBoxLayout;
    auto* saveBtn = new QPushButton(tr("Save"), this);
    auto* reloadBtn = new QPushButton(tr("Reload"), this);
    toolbar->addWidget(saveBtn);
    toolbar->addWidget(reloadBtn);
    toolbar->addStretch();
    toolbar->addWidget(m_lineLabel);
    toolbar->addWidget(m_statusLabel);

    layout->addLayout(toolbar);
    layout->addWidget(m_textEdit);

    connect(m_textEdit, &QTextEdit::textChanged, this, &IniEditorWidget::onTextChanged);
    connect(m_textEdit, &QTextEdit::cursorPositionChanged, this, [this] {
        QTextCursor c = m_textEdit->textCursor();
        m_lineLabel->setText(tr("Line: %1").arg(c.blockNumber() + 1));
    });
    connect(saveBtn, &QPushButton::clicked, this, &IniEditorWidget::onSave);
    connect(reloadBtn, &QPushButton::clicked, [this] {
        if (!m_currentFile.isEmpty()) loadFile(m_currentFile);
    });
}

IniEditorWidget::~IniEditorWidget() {
    delete m_highlighter;
}

bool IniEditorWidget::loadFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    QTextStream in(&file);
    m_textEdit->setPlainText(in.readAll());
    file.close();

    m_currentFile = path;
    m_modified = false;
    update();
    emit fileLoaded(path);
    return true;
}

bool IniEditorWidget::saveFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QTextStream out(&file);
    out << m_textEdit->toPlainText();
    file.close();

    m_currentFile = path;
    m_modified = false;
    m_statusLabel->setText(tr("Saved"));
    emit fileSaved(path);
    return true;
}

void IniEditorWidget::setContent(const QString& text) {
    QSignalBlocker b(m_textEdit);
    m_textEdit->setPlainText(text);
    m_modified = false;
}

void IniEditorWidget::onTextChanged() {
    m_modified = true;
    m_statusLabel->setText(m_modified ? tr("Modified") : tr("Ready"));
    emit contentChanged();
}

void IniEditorWidget::onSave() {
    if (m_currentFile.isEmpty()) {
        QString path = QFileDialog::getSaveFileName(this, tr("Save INI"),
            QString(),
            tr("INI files (*.ini);;All files (*.*)"));
        if (path.isEmpty()) return;
        m_currentFile = path;
    }
    saveFile(m_currentFile);
}

} // namespace ks