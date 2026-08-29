#include "DocumentPad.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QFontDatabase>
#include <QSyntaxHighlighter>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QFontInfo>
#include <QFont>
#include <QTextDocumentWriter>
#include <QTextDocumentFragment>
#include <QGuiApplication>
#include <QFontDialog>

namespace ks {

DocumentPad::DocumentPad(QWidget* parent)
    : QWidget(parent)
    , m_textEdit(new QTextEdit(this))
    , m_toolbar(new QToolBar("Document Toolbar", this))
    , m_currentFile()
    , m_modified(false)
{
    setupActions();
    setupToolbar();
    setupMenu();
    setupConnects();

    // Set up the text edit
    m_textEdit->setAcceptRichText(true);
    m_textEdit->setUndoRedoEnabled(true);

    // Set up a simple syntax highlighter for rich text (basic HTML-like handling)
    // We'll use QTextDocument's built-in rich text support

    // Set central widget layout
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_textEdit);
    setLayout(layout);

    // Setup toolbar
    setupToolbar();

    // Set window properties
    setWindowTitle("Document - UltraPad");
}

DocumentPad::~DocumentPad() = default;

void DocumentPad::setupActions()
{
    // File actions
m_actionNew = new QAction(QIcon(":/icons/document-new.svg"), tr("&New"), this);
    m_actionNew->setShortcut(QKeySequence::New);
    m_actionNew->setStatusTip(tr("Create a new document"));

    m_actionOpen = new QAction(QIcon(":/icons/document-open.svg"), tr("&Open..."), this);
    m_actionOpen->setShortcut(QKeySequence::Open);
    m_actionOpen->setStatusTip(tr("Open an existing document"));

    m_actionSave = new QAction(QIcon(":/icons/document-save.svg"), tr("&Save"), this);
    m_actionSave->setShortcut(QKeySequence::Save);
    m_actionSave->setStatusTip(tr("Save the document"));

    m_actionSaveAs = new QAction(QIcon(":/icons/document-save-as.svg"), tr("Save &As..."), this);
    m_actionSaveAs->setShortcut(QKeySequence::SaveAs);
    m_actionSaveAs->setStatusTip(tr("Save the document as a new file"));

    // Edit toolbar buttons
    m_actionCut = new QAction(QIcon(":/icons/edit-cut.svg"), tr("Cu&t"), this);
    m_actionCut->setShortcut(QKeySequence::Cut);
    m_actionCut->setStatusTip(tr("Cut the current selection"));

    mactionCopy = new QAction(QIcon(":/icons/edit-copy.svg"), tr("&Copy"), this);
    mactionCopy->setShortcut(QKeySequence::Copy);
    mactionCopy->setStatusTip(tr("Copy the current selection"));

    mactionPaste = new QAction(QIcon(":/icons/edit-paste.svg"), tr("&Paste"), this);
    mactionPaste->setShortcut(QKeySequence::Paste);
    mactionPaste->setStatusTip(tr("Paste from clipboard"));

    m_actionUndo = new QAction(QIcon(":/icons/edit-undo.svg"), tr("&Undo"), this);
    m_actionUndo->setShortcut(QKeySequence::Undo);
    m_actionUndo->setStatusTip(tr("Undo the last action"));
    m_actionUndo->setEnabled(false);

    m_actionRedo = new QAction(QIcon(":/icons/edit-redo.svg"), tr("&Redo"), this);
    m_actionRedo->setShortcut(QKeySequence::Redo);
    m_actionRedo->setStatusTip(tr("Redo the last undone action"));
    m_actionRedo->setEnabled(false);

    // Format actions - DocumentPad specific
    mactionBold = new QAction(QIcon(":/icons/documentpad-bold.svg"), tr("&Bold"), this);
    mactionBold->setStatusTip(tr("Make selection bold"));
    connect(mactionBold, &QAction::triggered, this, [this]() { setBold(); });

    mactionItalic = new QAction(QIcon(":/icons/documentpad-italic.svg"), tr("&Italic"), this);
    mactionItalic->setStatusTip(tr("Make selection italic"));
    connect(mactionItalic, &QAction::triggered, this, [this]() { setItalic(); });

    mactionUnderline = new QAction(QIcon(":/icons/documentpad-underline.svg"), tr("&Underline"), this);
    mactionUnderline->setStatusTip(tr("Underline selection"));
    connect(mactionUnderline, &QAction::triggered, this, [this]() { setUnderline(); });

    mactionStrikeThrough = new QAction(QIcon(":/icons/documentpad-strike.svg"), tr("Strikethrough"), this);
    mactionStrikeThrough->setStatusTip(tr("Strike through selection"));
    connect(mactionStrikeThrough, &QAction::triggered, this, [this]() { setStrikeThrough(); });

    mactionFontColor = new QAction(QIcon(":/icons/documentpad-font-color.svg"), tr("Font Color..."), this);
    mactionFontColor->setStatusTip(tr("Change font color"));
    connect(mactionFontColor, &QAction::triggered, this, [this]() { setFontColor(); });

    mactionFontSize = new QAction(QIcon(":/icons/documentpad-font-size.svg"), tr("Font Size..."), this);
    mactionFontSize->setStatusTip(tr("Change font size"));

    mactionFontFamily = new QAction(QIcon(":/icons/documentpad-font.svg"), tr("Font..."), this);
    mactionFontFamily->setStatusTip(tr("Change font family"));
    connect(mactionFontFamily, &QAction::triggered, this, [this]() {
        bool ok;
        QString family = QFontDialog::getFont(&ok, m_textEdit->font(), m_textEdit, tr("Select Font")).family();
        if (ok) setFontFamily(family);
    });

    mactionAbout = new QAction(QIcon(":/icons/documentpad-about.svg"), tr("About"), this);
    mactionAbout->setStatusTip(tr("About UltraPad"));
    connect(mactionAbout, &QAction::triggered, this, [this]() { onAbout(); });

    mactionMarginRuler = new QAction(QIcon(":/icons/documentpad-margin-ruler.svg"), tr("Margin Ruler"), this);
    mactionMarginRuler->setCheckable(true);
    mactionMarginRuler->setStatusTip(tr("Show/hide margin ruler"));
}

void DocumentPad::setupToolbar()
{
    m_toolbar->setIconSize(QSize(20, 20));
    m_toolbar->setFloatable(false);
    m_toolbar->setMovable(false);

    // File toolbar buttons
    m_toolbar->addAction(m_actionNew);
    m_toolbar->addAction(m_actionOpen);
    m_toolbar->addSeparator();
    m_toolbar->addAction(m_actionSave);
    m_toolbar->addAction(m_actionSaveAs);
    m_toolbar->addSeparator();

    // Edit toolbar buttons
    m_toolbar->addAction(m_actionCut);
    m_toolbar->addAction(mactionCopy);
    m_toolbar->addAction(mactionPaste);
    m_toolbar->addSeparator();
    m_toolbar->addAction(m_actionUndo);
    m_toolbar->addAction(m_actionRedo);
    m_toolbar->addSeparator();

    // Format toolbar
    m_toolbar->addSeparator();
    m_toolbar->addAction(mactionBold);
    m_toolbar->addAction(mactionItalic);
    m_toolbar->addAction(mactionUnderline);
    m_toolbar->addAction(mactionStrikeThrough);
    m_toolbar->addSeparator();
    m_toolbar->addAction(mactionFontColor);
    m_toolbar->addAction(mactionFontSize);
    m_toolbar->addAction(mactionFontFamily);
    m_toolbar->addSeparator();
    m_toolbar->addAction(mactionMarginRuler);

    auto* mainLayout = qobject_cast<QVBoxLayout*>(layout());
    if (mainLayout) {
        mainLayout->setMenuBar(nullptr); // toolbar is separate
        mainLayout->addWidget(m_toolbar);
    }
}

void DocumentPad::setupMenu()
{
    // Create a menu bar for the widget (since QWidget doesn't have menubar by default)
    // We'll add actions to a context menu or parent widget's menubar
    // For now, setup context menu actions
}

void DocumentPad::setupConnects()
{
    connect(m_textEdit, &QTextEdit::textChanged, this, &DocumentPad::onTextChanged);
    connect(m_textEdit, &QTextEdit::selectionChanged, this, &DocumentPad::onSelectionChanged);
    connect(m_textEdit, &QTextEdit::undoAvailable, this, &DocumentPad::onUndoAvailable);
    connect(m_textEdit, &QTextEdit::redoAvailable, this, &DocumentPad::onRedoAvailable);

    connect(m_actionNew, &QAction::triggered, this, [this]() { onNewFile(); });
    connect(m_actionOpen, &QAction::triggered, this, [this]() { onOpen(); });
    connect(m_actionSave, &QAction::triggered, this, [this]() { onSave(); });
    connect(m_actionSaveAs, &QAction::triggered, this, [this]() { onSaveAs(); });
    connect(m_actionCut, &QAction::triggered, this, [this]() { onCut(); });
    connect(mactionCopy, &QAction::triggered, this, [this]() { onCopy(); });
    connect(mactionPaste, &QAction::triggered, this, [this]() { onPaste(); });
    connect(mactionBold, &QAction::triggered, this, [this]() { onBold(); });
    connect(mactionItalic, &QAction::triggered, this, [this]() { onItalic(); });
    connect(mactionUnderline, &QAction::triggered, this, [this]() { onUnderline(); });
    connect(mactionStrikeThrough, &QAction::triggered, this, [this]() { onStrikeThrough(); });
    connect(mactionFontColor, &QAction::triggered, this, [this]() { setFontColor(); });
    connect(mactionFontFamily, &QAction::triggered, this, [this]() {
        bool ok;
        QString family = QFontDialog::getFont(&ok, m_textEdit->font(), m_textEdit, tr("Select Font")).family();
        if (ok) setFontFamily(family);
    });
    connect(mactionAbout, &QAction::triggered, this, [this]() { onAbout(); });
}

void DocumentPad::newDocument()
{
    if (m_modified) {
        auto reply = QMessageBox::question(this, "Unsaved Changes",
            "Do you want to save changes to the current document?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (reply == QMessageBox::Save) {
            saveDocument();
        } else if (reply == QMessageBox::Cancel) {
            return;
        }
    }

    m_textEdit->clear();
    m_currentFile.clear();
    m_modified = false;
    setWindowTitle("UltraPad");
    m_actionUndo->setEnabled(m_textEdit->document()->isUndoAvailable());
    m_actionRedo->setEnabled(m_textEdit->document()->isRedoAvailable());
}

void DocumentPad::openDocument()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Open Document"),
        QString(), tr("Text Documents (*.txt *.rtf *.doc);;Word Documents (*.doc *.docx);;All Files (*)"));

    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to open file: %1").arg(path));
        return;
    }

    QString suffix = QFileInfo(path).suffix().toLower();
    if (suffix == "txt" || suffix == "text") {
        QTextStream in(&file);
        m_textEdit->setPlainText(in.readAll());
    } else if (suffix == "rtf") {
        QByteArray rtfData = file.readAll();
        m_textEdit->setHtml(QString::fromUtf8(rtfData));
    } else if (suffix == "doc") {
        // Basic .doc handling - try to read as RTF or plain text
        QByteArray data = file.readAll();
        QString text = QString::fromUtf8(data);
        // Try to detect if it's RTF format
        if (text.startsWith("{")) {
            m_textEdit->setHtml(text);
        } else {
            m_textEdit->setPlainText(text);
        }
    } else {
        // Try to read as plain text for unknown formats
        QTextStream in(&file);
        m_textEdit->setPlainText(in.readAll());
    }
    file.close();

    m_currentFile = path;
    m_modified = false;
    setWindowTitle(QString("UltraPad - %1").arg(QFileInfo(path).fileName()));
    m_actionUndo->setEnabled(m_textEdit->document()->isUndoAvailable());
    m_actionRedo->setEnabled(m_textEdit->document()->isRedoAvailable());
}

void DocumentPad::saveDocument()
{
    if (m_currentFile.isEmpty()) {
        saveDocumentAs();
        return;
    }

    QString suffix = QFileInfo(m_currentFile).suffix().toLower();
    QFile file(m_currentFile);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to save file: %1").arg(m_currentFile));
        return;
    }

    if (suffix == "txt" || suffix == "text") {
        QTextStream out(&file);
        out << m_textEdit->toPlainText();
    } else if (suffix == "rtf") {
        QTextDocumentWriter writer(&file, "rtf");
        writer.write(m_textEdit->document());
    } else {
        // Default: save as RTF for .doc/.docx compatibility
        QTextDocumentWriter writer(&file, "rtf");
        writer.write(m_textEdit->document());
    }

    file.close();

    m_modified = false;
    setWindowTitle(QString("UltraPad - %1").arg(QFileInfo(m_currentFile).fileName()));
    m_actionUndo->setEnabled(m_textEdit->document()->isUndoAvailable());
    m_actionRedo->setEnabled(m_textEdit->document()->isRedoAvailable());
}

void DocumentPad::saveDocumentAs()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save Document As"),
        QString(), tr("Text Documents (*.txt *.rtf);;Word Documents (*.doc *.docx);;All Files (*)"));

    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to save file: %1").arg(path));
        return;
    }

    QString suffix = QFileInfo(path).suffix().toLower();
    m_currentFile = path;

    if (suffix == "txt" || suffix == "text") {
        QTextStream out(&file);
        out << m_textEdit->toPlainText();
    } else if (suffix == "rtf") {
        QTextDocumentWriter writer(&file, "rtf");
        writer.write(m_textEdit->document());
    } else {
        // Save as RTF for .doc/.docx compatibility
        QTextDocumentWriter writer(&file, "rtf");
        writer.write(m_textEdit->document());
    }

    file.close();

    m_modified = false;
    setWindowTitle(QString("UltraPad - %1").arg(QFileInfo(path).fileName()));
    m_actionUndo->setEnabled(m_textEdit->document()->isUndoAvailable());
    m_actionRedo->setEnabled(m_textEdit->document()->isRedoAvailable());
}

QString DocumentPad::currentFilePath() const
{
    return m_currentFile;
}

bool DocumentPad::documentModified() const
{
    return m_modified;
}

void DocumentPad::cut()
{
    m_textEdit->cut();
}

void DocumentPad::copy()
{
    m_textEdit->copy();
}

void DocumentPad::paste()
{
    m_textEdit->paste();
}

void DocumentPad::selectAll()
{
    m_textEdit->selectAll();
}

void DocumentPad::undo()
{
    m_textEdit->undo();
}

void DocumentPad::redo()
{
    m_textEdit->redo();
}

void DocumentPad::setBold()
{
    QTextCharFormat fmt;
    fmt.setFontWeight(m_textEdit->currentCharFormat().fontWeight() == QFont::Bold ? QFont::Normal : QFont::Bold);
    mergeFormatOnSelection(fmt);
}

void DocumentPad::setItalic()
{
    QTextCharFormat fmt;
    fmt.setFontItalic(!m_textEdit->currentCharFormat().fontItalic());
    mergeFormatOnSelection(fmt);
}

void DocumentPad::setUnderline()
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(!m_textEdit->currentCharFormat().fontUnderline());
    mergeFormatOnSelection(fmt);
}

void DocumentPad::setStrikeThrough()
{
    QTextCharFormat fmt;
    fmt.setFontStrikeOut(!m_textEdit->currentCharFormat().fontStrikeOut());
    mergeFormatOnSelection(fmt);
}

void DocumentPad::setFontColor()
{
    QColor color = QColorDialog::getColor(m_textEdit->currentCharFormat().foreground().color(), this, tr("Select Text Color"));
    if (color.isValid()) {
        QTextCharFormat fmt;
        fmt.setForeground(color);
        mergeFormatOnSelection(fmt);
    }
}

void DocumentPad::setFontSize(qreal size)
{
    QTextCharFormat fmt;
    fmt.setFontPointSize(size);
    mergeFormatOnSelection(fmt);
}

void DocumentPad::setFontFamily(const QString& family)
{
    QTextCharFormat fmt;
    fmt.setFontFamily(family);
    mergeFormatOnSelection(fmt);
}

void DocumentPad::mergeFormatOnSelection(const QTextCharFormat& format)
{
    QTextCursor cursor = m_textEdit->textCursor();
    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::WordUnderCursor);
    }
    cursor.mergeCharFormat(format);
}

void DocumentPad::onTextChanged()
{
    m_modified = true;
    emit documentModifiedChanged(true);
    m_actionUndo->setEnabled(m_textEdit->document()->isUndoAvailable());
    m_actionRedo->setEnabled(m_textEdit->document()->isRedoAvailable());
}

void DocumentPad::onUndoAvailable(bool available)
{
    m_actionUndo->setEnabled(available);
}

void DocumentPad::onRedoAvailable(bool available)
{
    m_actionRedo->setEnabled(available);
}

void DocumentPad::onSelectionChanged()
{
    emit selectionChanged();
}

void DocumentPad::onFormatTriggered(QAction* action)
{
    if (action == mactionBold) setBold();
    else if (action == mactionItalic) setItalic();
    else if (action == mactionUnderline) setUnderline();
    else if (action == mactionStrikeThrough) setStrikeThrough();
}

void DocumentPad::onNewFile()
{
    newDocument();
}

void DocumentPad::onOpen()
{
    openDocument();
}

void DocumentPad::onSave()
{
    saveDocument();
}

void DocumentPad::onSaveAs()
{
    saveDocumentAs();
}

void DocumentPad::onCut()
{
    cut();
}

void DocumentPad::onCopy()
{
    copy();
}

void DocumentPad::onPaste()
{
    paste();
}

void DocumentPad::onBold()
{
    setBold();
}

void DocumentPad::onItalic()
{
    setItalic();
}

void DocumentPad::onUnderline()
{
    setUnderline();
}

void DocumentPad::onStrikeThrough()
{
    setStrikeThrough();
}

void DocumentPad::onAbout()
{
    QMessageBox::about(this, tr("About UltraPad"),
        tr("UltraPad\n\nA modern rich text editor for Windows.\n\nBased on the UltraPad project by lixkote.\n\nCopyright (c) 2024-2026"));
}

void DocumentPad::onFontColor()
{
    QColor color = QColorDialog::getColor(m_textEdit->textColor(), this, tr("Select Font Color"));
    if (color.isValid()) {
        QTextCharFormat fmt;
        fmt.setForeground(color);
        mergeFormatOnSelection(fmt);
    }
}

void DocumentPad::onFontSizeChanged(int index)
{
    Q_UNUSED(index);
}

void DocumentPad::onFontFamilyChanged(const QString& text)
{
    QTextCharFormat fmt;
    fmt.setFontFamily(text);
    mergeFormatOnSelection(fmt);
}

} // namespace ks