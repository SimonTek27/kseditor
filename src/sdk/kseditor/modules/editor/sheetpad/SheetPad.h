#pragma once

#include <QWidget>
#include <QAction>
#include <QToolBar>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>

namespace ks {

class SpreadsheetModel;
class SpreadsheetView;

class SheetPad : public QWidget {
    Q_OBJECT

public:
    explicit SheetPad(QWidget* parent = nullptr);
    ~SheetPad() override;

    void newSpreadsheet();
    void openSpreadsheet();
    void saveSpreadsheet();
    void saveSpreadsheetAs();
    void exportCSV();
    void importCSV();
    QString currentFilePath() const;
    bool documentModified() const;

    SpreadsheetModel* model() const { return m_model; }
    SpreadsheetView* view() const { return m_view; }

signals:
    void fileOpened(const QString& path);
    void fileClosed();
    void fileSaved(const QString& path);
    void documentModifiedChanged(bool modified);
    void cellSelected(int row, int col);

private slots:
    void onCellSelected(int row, int col);
    void onCellEditingFinished(int row, int col);
    void onCurrentCellChanged(int row, int col);
    void onFormulaBarReturnPressed();
    void onBoldToggled(bool checked);
    void onItalicToggled(bool checked);
    void onUnderlineToggled(bool checked);
    void onFontColorChanged();
    void onBackgroundColorChanged();
    void onFontSizeChanged(int index);
    void onNewFile();
    void onOpen();
    void onSave();
    void onSaveAs();
    void onExportCSV();
    void onImportCSV();
    void onInsertRow();
    void onInsertColumn();
    void onDeleteRow();
    void onDeleteColumn();
    void onClearContent();
    void onClearAll();

private:
    void setupUI();
    void setupActions();
    void setupToolbar();
    void setupFormulaBar();
    void setupConnects();
    void updateFormulaBar();
    void updateTitle();

    SpreadsheetModel* m_model;
    SpreadsheetView* m_view;
    QLineEdit* m_formulaBar;
    QLabel* m_cellLabel;
    QComboBox* m_fontSizeCombo;
    QString m_currentFile;
    bool m_modified = false;

    QAction* m_actionNew;
    QAction* m_actionOpen;
    QAction* m_actionSave;
    QAction* m_actionSaveAs;
    QAction* m_actionExportCSV;
    QAction* m_actionImportCSV;

    QAction* m_actionCut;
    QAction* m_actionCopy;
    QAction* m_actionPaste;
    QAction* m_actionUndo;
    QAction* m_actionRedo;

    QAction* m_actionBold;
    QAction* m_actionItalic;
    QAction* m_actionUnderline;
    QAction* m_actionFontColor;
    QAction* m_actionBackgroundColor;

    QAction* m_actionInsertRow;
    QAction* m_actionInsertColumn;
    QAction* m_actionDeleteRow;
    QAction* m_actionDeleteColumn;
    QAction* m_actionClearContent;
    QAction* m_actionClearAll;
};

} // namespace ks
