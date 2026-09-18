#include "SheetPad.h"
#include "SpreadsheetModel.h"
#include "SpreadsheetView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QFontDatabase>
#include <QColorDialog>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

namespace ks {

SheetPad::SheetPad(QWidget* parent)
    : QWidget(parent)
    , m_model(new SpreadsheetModel(this))
    , m_view(new SpreadsheetView(this))
    , m_formulaBar(new QLineEdit(this))
    , m_cellLabel(new QLabel(this))
    , m_fontSizeCombo(new QComboBox(this))
{
    setupUI();
    setupActions();
    setupToolbar();
    setupFormulaBar();
    setupConnects();

    m_view->setSpreadsheetModel(m_model);
    setWindowTitle("SheetPad");

    QList<int> sizes = QFontDatabase::standardSizes();
    for (int size : sizes) {
        m_fontSizeCombo->addItem(QString::number(size));
    }
    m_fontSizeCombo->setCurrentText("10");
}

SheetPad::~SheetPad() = default;

void SheetPad::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto* formulaBarLayout = new QHBoxLayout();
    formulaBarLayout->setContentsMargins(4, 2, 4, 2);
    formulaBarLayout->setSpacing(4);

    m_cellLabel->setMinimumWidth(60);
    m_cellLabel->setStyleSheet("QLabel { border: 1px solid #999; padding: 2px 4px; background: white; }");
    m_cellLabel->setText("A1");

    m_formulaBar->setPlaceholderText("Enter value or formula (start with =)");
    m_formulaBar->setStyleSheet("QLineEdit { border: 1px solid #999; padding: 2px 4px; }");

    formulaBarLayout->addWidget(m_cellLabel);
    formulaBarLayout->addWidget(m_formulaBar);

    auto* formulaBarWidget = new QWidget(this);
    formulaBarWidget->setLayout(formulaBarLayout);
    mainLayout->addWidget(formulaBarWidget);

    m_view->setAlternatingRowColors(true);
    m_view->setStyleSheet(
        "QTableView { gridline-color: #d0d0d0; }"
        "QTableView::item:selected { background: #3399ff; color: white; }"
        "QHeaderView::section { background: #f0f0f0; border: 1px solid #d0d0d0; padding: 4px; }"
    );

    mainLayout->addWidget(m_view);
}

void SheetPad::setupActions()
{
    m_actionNew = new QAction(QIcon(":/icons/document-new.svg"), tr("&New"), this);
    m_actionNew->setShortcut(QKeySequence::New);
    m_actionNew->setStatusTip(tr("Create a new spreadsheet"));

    m_actionOpen = new QAction(QIcon(":/icons/document-open.svg"), tr("&Open..."), this);
    m_actionOpen->setShortcut(QKeySequence::Open);
    m_actionOpen->setStatusTip(tr("Open a spreadsheet"));

    m_actionSave = new QAction(QIcon(":/icons/document-save.svg"), tr("&Save"), this);
    m_actionSave->setShortcut(QKeySequence::Save);
    m_actionSave->setStatusTip(tr("Save the spreadsheet"));

    m_actionSaveAs = new QAction(QIcon(":/icons/document-save-as.svg"), tr("Save &As..."), this);
    m_actionSaveAs->setShortcut(QKeySequence::SaveAs);
    m_actionSaveAs->setStatusTip(tr("Save the spreadsheet as a new file"));

    m_actionExportCSV = new QAction(tr("Export as &CSV..."), this);
    m_actionExportCSV->setStatusTip(tr("Export spreadsheet to CSV format"));

    m_actionImportCSV = new QAction(tr("&Import CSV..."), this);
    m_actionImportCSV->setStatusTip(tr("Import a CSV file"));

    m_actionCut = new QAction(QIcon(":/icons/edit-cut.svg"), tr("Cu&t"), this);
    m_actionCut->setShortcut(QKeySequence::Cut);
    m_actionCut->setStatusTip(tr("Cut selection"));

    m_actionCopy = new QAction(QIcon(":/icons/edit-copy.svg"), tr("&Copy"), this);
    m_actionCopy->setShortcut(QKeySequence::Copy);
    m_actionCopy->setStatusTip(tr("Copy selection"));

    m_actionPaste = new QAction(QIcon(":/icons/edit-paste.svg"), tr("&Paste"), this);
    m_actionPaste->setShortcut(QKeySequence::Paste);
    m_actionPaste->setStatusTip(tr("Paste from clipboard"));

    m_actionUndo = new QAction(QIcon(":/icons/edit-undo.svg"), tr("&Undo"), this);
    m_actionUndo->setShortcut(QKeySequence::Undo);
    m_actionUndo->setEnabled(false);

    m_actionRedo = new QAction(QIcon(":/icons/edit-redo.svg"), tr("&Redo"), this);
    m_actionRedo->setShortcut(QKeySequence::Redo);
    m_actionRedo->setEnabled(false);

    m_actionBold = new QAction(tr("&Bold"), this);
    m_actionBold->setShortcut(QKeySequence::Bold);
    m_actionBold->setCheckable(true);
    m_actionBold->setStatusTip(tr("Toggle bold"));

    m_actionItalic = new QAction(tr("&Italic"), this);
    m_actionItalic->setShortcut(QKeySequence::Italic);
    m_actionItalic->setCheckable(true);
    m_actionItalic->setStatusTip(tr("Toggle italic"));

    m_actionUnderline = new QAction(tr("&Underline"), this);
    m_actionUnderline->setShortcut(QKeySequence::Underline);
    m_actionUnderline->setCheckable(true);
    m_actionUnderline->setStatusTip(tr("Toggle underline"));

    m_actionFontColor = new QAction(tr("Font &Color..."), this);
    m_actionFontColor->setStatusTip(tr("Change font color"));

    m_actionBackgroundColor = new QAction(tr("Background &Color..."), this);
    m_actionBackgroundColor->setStatusTip(tr("Change cell background color"));

    m_actionInsertRow = new QAction(tr("Insert &Row"), this);
    m_actionInsertRow->setStatusTip(tr("Insert row at current position"));

    m_actionInsertColumn = new QAction(tr("Insert &Column"), this);
    m_actionInsertColumn->setStatusTip(tr("Insert column at current position"));

    m_actionDeleteRow = new QAction(tr("&Delete Row"), this);
    m_actionDeleteRow->setStatusTip(tr("Delete current row"));

    m_actionDeleteColumn = new QAction(tr("Delete C&olumn"), this);
    m_actionDeleteColumn->setStatusTip(tr("Delete current column"));

    m_actionClearContent = new QAction(tr("Clear &Content"), this);
    m_actionClearContent->setShortcut(QKeySequence::Delete);
    m_actionClearContent->setStatusTip(tr("Clear cell content"));

    m_actionClearAll = new QAction(tr("Clear &All"), this);
    m_actionClearAll->setStatusTip(tr("Clear all content and formatting"));
}

void SheetPad::setupToolbar()
{
    auto* toolbar = new QToolBar("SheetPad Toolbar", this);
    toolbar->setIconSize(QSize(20, 20));
    toolbar->setFloatable(false);
    toolbar->setMovable(false);

    toolbar->addAction(m_actionNew);
    toolbar->addAction(m_actionOpen);
    toolbar->addSeparator();
    toolbar->addAction(m_actionSave);
    toolbar->addAction(m_actionSaveAs);
    toolbar->addSeparator();

    toolbar->addAction(m_actionCut);
    toolbar->addAction(m_actionCopy);
    toolbar->addAction(m_actionPaste);
    toolbar->addSeparator();
    toolbar->addAction(m_actionUndo);
    toolbar->addAction(m_actionRedo);
    toolbar->addSeparator();

    toolbar->addAction(m_actionBold);
    toolbar->addAction(m_actionItalic);
    toolbar->addAction(m_actionUnderline);
    toolbar->addSeparator();
    toolbar->addAction(m_actionFontColor);
    toolbar->addAction(m_actionBackgroundColor);
    toolbar->addWidget(m_fontSizeCombo);
    toolbar->addSeparator();

    toolbar->addAction(m_actionInsertRow);
    toolbar->addAction(m_actionInsertColumn);
    toolbar->addAction(m_actionDeleteRow);
    toolbar->addAction(m_actionDeleteColumn);
    toolbar->addSeparator();
    toolbar->addAction(m_actionClearContent);

    auto* layout = qobject_cast<QVBoxLayout*>(this->layout());
    if (layout) {
        layout->insertWidget(0, toolbar);
    }
}

void SheetPad::setupFormulaBar()
{
}

void SheetPad::setupConnects()
{
    connect(m_actionNew, &QAction::triggered, this, &SheetPad::onNewFile);
    connect(m_actionOpen, &QAction::triggered, this, &SheetPad::onOpen);
    connect(m_actionSave, &QAction::triggered, this, &SheetPad::onSave);
    connect(m_actionSaveAs, &QAction::triggered, this, &SheetPad::onSaveAs);
    connect(m_actionExportCSV, &QAction::triggered, this, &SheetPad::onExportCSV);
    connect(m_actionImportCSV, &QAction::triggered, this, &SheetPad::onImportCSV);

    connect(m_actionCut, &QAction::triggered, m_view, &SpreadsheetView::cutToClipboard);
    connect(m_actionCopy, &QAction::triggered, m_view, &SpreadsheetView::copyToClipboard);
    connect(m_actionPaste, &QAction::triggered, m_view, &SpreadsheetView::pasteFromClipboard);

    connect(m_actionBold, &QAction::triggered, this, &SheetPad::onBoldToggled);
    connect(m_actionItalic, &QAction::triggered, this, &SheetPad::onItalicToggled);
    connect(m_actionUnderline, &QAction::triggered, this, &SheetPad::onUnderlineToggled);
    connect(m_actionFontColor, &QAction::triggered, this, &SheetPad::onFontColorChanged);
    connect(m_actionBackgroundColor, &QAction::triggered, this, &SheetPad::onBackgroundColorChanged);
    connect(m_fontSizeCombo, &QComboBox::currentTextChanged, this, [this](const QString& text) {
        bool ok;
        int size = text.toInt(&ok);
        if (!ok) return;
        QModelIndexList selected = m_view->selectionModel()->selectedIndexes();
        for (const QModelIndex& idx : selected) {
            QFont f = m_model->cellStyle(idx.row(), idx.column()).font;
            f.setPointSize(size);
            m_model->setCellFont(idx.row(), idx.column(), f);
        }
    });

    connect(m_actionInsertRow, &QAction::triggered, this, &SheetPad::onInsertRow);
    connect(m_actionInsertColumn, &QAction::triggered, this, &SheetPad::onInsertColumn);
    connect(m_actionDeleteRow, &QAction::triggered, this, &SheetPad::onDeleteRow);
    connect(m_actionDeleteColumn, &QAction::triggered, this, &SheetPad::onDeleteColumn);
    connect(m_actionClearContent, &QAction::triggered, this, &SheetPad::onClearContent);
    connect(m_actionClearAll, &QAction::triggered, this, &SheetPad::onClearAll);

    connect(m_formulaBar, &QLineEdit::returnPressed, this, &SheetPad::onFormulaBarReturnPressed);

    connect(m_view, &SpreadsheetView::cellSelected, this, &SheetPad::onCellSelected);
    connect(m_view, &SpreadsheetView::cellEditingFinished, this, &SheetPad::onCellEditingFinished);
    connect(m_view, &SpreadsheetView::currentCellChanged, this, &SheetPad::onCurrentCellChanged);
}

void SheetPad::updateFormulaBar()
{
    QModelIndex idx = m_view->currentIndex();
    if (!idx.isValid()) return;

    m_cellLabel->setText(m_model->cellReference(idx.row(), idx.column()));
    m_formulaBar->setText(m_model->cellValue(idx.row(), idx.column()));

    CellStyle style = m_model->cellStyle(idx.row(), idx.column());
    m_actionBold->setChecked(style.bold);
    m_actionItalic->setChecked(style.italic);
    m_actionUnderline->setChecked(style.underline);
    m_fontSizeCombo->setCurrentText(QString::number(style.font.pointSize() > 0 ? style.font.pointSize() : 10));
}

void SheetPad::updateTitle()
{
    QString title = "SheetPad";
    if (!m_currentFile.isEmpty()) {
        title += " - " + QFileInfo(m_currentFile).fileName();
    }
    if (m_modified) {
        title += " *";
    }
    setWindowTitle(title);
}

void SheetPad::newSpreadsheet()
{
    if (m_modified) {
        auto reply = QMessageBox::question(this, tr("Unsaved Changes"),
            tr("Do you want to save changes to the current spreadsheet?"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (reply == QMessageBox::Save) {
            saveSpreadsheet();
        } else if (reply == QMessageBox::Cancel) {
            return;
        }
    }

    m_model->clear();
    m_currentFile.clear();
    m_modified = false;
    updateTitle();
}

void SheetPad::openSpreadsheet()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Open Spreadsheet"),
        QString(), tr("Spreadsheet Files (*.csv *.json);;CSV Files (*.csv);;JSON Files (*.json);;All Files (*)"));

    if (path.isEmpty()) return;

    QString suffix = QFileInfo(path).suffix().toLower();
    bool success = false;

    if (suffix == "csv") {
        success = m_model->loadCSV(path);
    } else if (suffix == "json") {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            m_model->deserialize(doc.object());
            success = true;
        }
    }

    if (!success) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to open file: %1").arg(path));
        return;
    }

    m_currentFile = path;
    m_modified = false;
    updateTitle();
    emit fileOpened(path);
}

void SheetPad::saveSpreadsheet()
{
    if (m_currentFile.isEmpty()) {
        saveSpreadsheetAs();
        return;
    }

    QString suffix = QFileInfo(m_currentFile).suffix().toLower();
    bool success = false;

    if (suffix == "csv") {
        success = m_model->saveCSV(m_currentFile);
    } else if (suffix == "json") {
        QFile file(m_currentFile);
        if (file.open(QIODevice::WriteOnly)) {
            QJsonObject data = m_model->serialize();
            file.write(QJsonDocument(data).toJson());
            success = true;
        }
    }

    if (!success) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to save file: %1").arg(m_currentFile));
        return;
    }

    m_modified = false;
    updateTitle();
    emit fileSaved(m_currentFile);
}

void SheetPad::saveSpreadsheetAs()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save Spreadsheet As"),
        QString(), tr("JSON Files (*.json);;CSV Files (*.csv);;All Files (*)"));

    if (path.isEmpty()) return;

    m_currentFile = path;
    saveSpreadsheet();
}

void SheetPad::exportCSV()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Export as CSV"),
        QString(), tr("CSV Files (*.csv);;All Files (*)"));

    if (path.isEmpty()) return;

    if (!m_model->saveCSV(path)) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to export CSV: %1").arg(path));
    }
}

void SheetPad::importCSV()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Import CSV"),
        QString(), tr("CSV Files (*.csv);;All Files (*)"));

    if (path.isEmpty()) return;

    if (!m_model->loadCSV(path)) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to import CSV: %1").arg(path));
    } else {
        m_modified = true;
        updateTitle();
    }
}

void SheetPad::onCellSelected(int row, int col)
{
    updateFormulaBar();
    emit cellSelected(row, col);
}

void SheetPad::onCellEditingFinished(int row, int col)
{
    Q_UNUSED(row);
    Q_UNUSED(col);
}

void SheetPad::onCurrentCellChanged(int row, int col)
{
    updateFormulaBar();
}

void SheetPad::onFormulaBarReturnPressed()
{
    QModelIndex idx = m_view->currentIndex();
    if (!idx.isValid()) return;

    m_model->setData(idx, m_formulaBar->text(), Qt::EditRole);
    m_modified = true;
    updateTitle();
}

void SheetPad::onBoldToggled(bool checked)
{
    QModelIndexList selected = m_view->selectionModel()->selectedIndexes();
    for (const QModelIndex& idx : selected) {
        m_model->setCellBold(idx.row(), idx.column(), checked);
    }
}

void SheetPad::onItalicToggled(bool checked)
{
    QModelIndexList selected = m_view->selectionModel()->selectedIndexes();
    for (const QModelIndex& idx : selected) {
        m_model->setCellItalic(idx.row(), idx.column(), checked);
    }
}

void SheetPad::onUnderlineToggled(bool checked)
{
    QModelIndexList selected = m_view->selectionModel()->selectedIndexes();
    for (const QModelIndex& idx : selected) {
        m_model->setCellUnderline(idx.row(), idx.column(), checked);
    }
}

void SheetPad::onFontColorChanged()
{
    QModelIndex idx = m_view->currentIndex();
    if (!idx.isValid()) return;

    QColor color = QColorDialog::getColor(m_model->cellStyle(idx.row(), idx.column()).textColor,
        this, tr("Select Font Color"));
    if (!color.isValid()) return;

    QModelIndexList selected = m_view->selectionModel()->selectedIndexes();
    for (const QModelIndex& sidx : selected) {
        m_model->setCellTextColor(sidx.row(), sidx.column(), color);
    }
}

void SheetPad::onBackgroundColorChanged()
{
    QModelIndex idx = m_view->currentIndex();
    if (!idx.isValid()) return;

    QColor color = QColorDialog::getColor(m_model->cellStyle(idx.row(), idx.column()).backgroundColor,
        this, tr("Select Background Color"));
    if (!color.isValid()) return;

    QModelIndexList selected = m_view->selectionModel()->selectedIndexes();
    for (const QModelIndex& sidx : selected) {
        m_model->setCellBackground(sidx.row(), sidx.column(), color);
    }
}

void SheetPad::onInsertRow()
{
    QModelIndex idx = m_view->currentIndex();
    int row = idx.isValid() ? idx.row() : m_model->rowCount() - 1;
    m_model->insertRows(row + 1, 1);
    m_modified = true;
    updateTitle();
}

void SheetPad::onInsertColumn()
{
    QModelIndex idx = m_view->currentIndex();
    int col = idx.isValid() ? idx.column() : m_model->columnCount() - 1;
    m_model->insertColumns(col + 1, 1);
    m_modified = true;
    updateTitle();
}

void SheetPad::onDeleteRow()
{
    QModelIndex idx = m_view->currentIndex();
    if (!idx.isValid()) return;
    m_model->removeRows(idx.row(), 1);
    m_modified = true;
    updateTitle();
}

void SheetPad::onDeleteColumn()
{
    QModelIndex idx = m_view->currentIndex();
    if (!idx.isValid()) return;
    m_model->removeColumns(idx.column(), 1);
    m_modified = true;
    updateTitle();
}

void SheetPad::onClearContent()
{
    QModelIndexList selected = m_view->selectionModel()->selectedIndexes();
    for (const QModelIndex& idx : selected) {
        m_model->setData(idx, "", Qt::EditRole);
    }
    m_modified = true;
    updateTitle();
}

void SheetPad::onClearAll()
{
    m_model->clear();
    m_modified = true;
    updateTitle();
}

void SheetPad::onNewFile() { newSpreadsheet(); }
void SheetPad::onOpen() { openSpreadsheet(); }
void SheetPad::onSave() { saveSpreadsheet(); }
void SheetPad::onSaveAs() { saveSpreadsheetAs(); }
void SheetPad::onExportCSV() { exportCSV(); }
void SheetPad::onImportCSV() { importCSV(); }

QString SheetPad::currentFilePath() const { return m_currentFile; }
bool SheetPad::documentModified() const { return m_modified; }

void SheetPad::onFontSizeChanged(int index)
{
    Q_UNUSED(index);
}

} // namespace ks
