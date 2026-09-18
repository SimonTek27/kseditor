#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>

class QPlainTextEdit;

namespace ks {

class FindReplaceDialog : public QDialog {
    Q_OBJECT
public:
    explicit FindReplaceDialog(QWidget* parent = nullptr);

    void setEditor(QPlainTextEdit* editor) { m_editor = editor; }

signals:
    void findNext(const QString& text, bool caseSensitive);
    void replace(const QString& find, const QString& replace, bool caseSensitive);
    void replaceAll(const QString& find, const QString& replace, bool caseSensitive);

private slots:
    void onFindNext();
    void onReplace();
    void onReplaceAll();

private:
    QPlainTextEdit* m_editor = nullptr;
    QLineEdit* m_findInput;
    QLineEdit* m_replaceInput;
    QCheckBox* m_caseSensitive;
    QLabel* m_resultLabel;
};

} // namespace ks
