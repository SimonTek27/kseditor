#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QAction>
#include <QToolBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QFontDialog>
#include <QColorDialog>

namespace ks {

class DocumentPad : public QWidget {
    Q_OBJECT

public:
    explicit DocumentPad(QWidget* parent = nullptr);
    ~DocumentPad() override;

    // Document operations
    void newDocument();
    void openDocument();
    void saveDocument();
    void saveDocumentAs();
    QString currentFilePath() const;
    bool documentModified() const;

    // Text formatting actions
    void cut();
    void copy();
    void paste();
    void selectAll();
    void undo();
    void redo();

    // Formatting actions
    void setBold();
    void setItalic();
    void setUnderline();
    void setStrikeThrough();
    void setFontColor();
    void setFontSize(qreal size);
    void setFontFamily(const QString& family);

    // Get the text edit widget
    QTextEdit* textEdit() const { return m_textEdit; }

signals:
    void fileOpened(const QString& path);
    void fileClosed();
    void fileSaved(const QString& path);
    void documentModifiedChanged(bool modified);
    void selectionChanged();

private slots:
    void onTextChanged();
    void onUndoAvailable(bool available);
    void onRedoAvailable(bool available);
    void onSelectionChanged();
    void onFormatTriggered(QAction* action);
    void onNewFile();
    void onOpen();
    void onSave();
    void onSaveAs();
    void onCut();
    void onCopy();
    void onPaste();
    void onBold();
    void onItalic();
    void onUnderline();
    void onStrikeThrough();
    void onFontColor();
    void onFontSizeChanged(int index);
    void onFontFamilyChanged(const QString& text);
    void onAbout();

private:
    void setupActions();
    void setupToolbar();
    void setupMenu();
    void setupConnects();
    void mergeFormatOnSelection(const QTextCharFormat& format);
    QByteArray loadSettings();
    void saveSettings();

    QTextEdit* m_textEdit;
    QToolBar* m_toolbar;
    QString m_currentFile;
    bool m_modified = false;

    // Actions
    QAction* m_actionNew;
    QAction* m_actionOpen;
    QAction* m_actionSave;
    QAction* m_actionSaveAs;
    QAction* m_actionCut;
    QAction* m_actionUndo;
    QAction* m_actionRedo;
    QAction* mactionCopy;
    QAction* mactionPaste;
    QAction* mactionBold;
    QAction* mactionItalic;
    QAction* mactionUnderline;
    QAction* mactionStrikeThrough;
    QAction* mactionFontColor;
    QAction* mactionFontSize;
    QAction* mactionFontFamily;
    QAction* mactionAbout;
    QAction* mactionMarginRuler; // WordPad feature: margin ruler
};

} // namespace ks