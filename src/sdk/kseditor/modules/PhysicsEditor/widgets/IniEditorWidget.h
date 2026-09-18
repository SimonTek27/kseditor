#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QVector>
#include <QTextCharFormat>
#include <QTextDocument>
#include "IniSyntaxHighlighter.h"

namespace ks {

// ─────────────────────────────────────────────────────────────────────────────
// IniEditorWidget — text editor with line numbers and highlighting
// ─────────────────────────────────────────────────────────────────────────────
class IniEditorWidget : public QWidget {
    Q_OBJECT
public:
    explicit IniEditorWidget(QWidget* parent = nullptr);
    ~IniEditorWidget();

    bool loadFile(const QString& path);
    bool saveFile(const QString& path);
    QString content() const { return m_textEdit->toPlainText(); }
    void setContent(const QString& text);
    bool isModified() const { return m_modified; }

signals:
    void contentChanged();
    void fileSaved(const QString& path);
    void fileLoaded(const QString& path);

private slots:
    void onTextChanged();
    void onSave();

private:
    void setupHighlighter();

    QTextEdit*       m_textEdit  = nullptr;
    QLabel*         m_lineLabel = nullptr;
    QLabel*         m_statusLabel = nullptr;
    QString         m_currentFile;
    bool            m_modified  = false;
    IniSyntaxHighlighter* m_highlighter = nullptr;
};

} // namespace ks