#include "TextEditorModule.h"
#include "CodeEditor.h"
#include "SyntaxHighlighter.h"
#include "FindReplaceDialog.h"
#include "../../sys/LogManager.h"

#include <functional>
#include <QShortcut>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QFileDialog>
#include <QJsonArray>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QApplication>
#include <QTextCursor>
#include <QFile>

namespace ks {

TextEditorModule::TextEditorModule(QWidget* parent)
    : EditorModule(parent)
    , m_dockWidget(nullptr)
    , m_tabWidget(nullptr)
    , m_toolbar(nullptr)
    , m_findDialog(nullptr)
{
    setAcceptDrops(true);
}

TextEditorModule::~TextEditorModule()
{
    delete m_findDialog;
}

bool TextEditorModule::initialize()
{
    LOG_INFO("TextEditorModule", "Initializing Text Editor module");
    m_findDialog = new FindReplaceDialog(nullptr);
    return true;
}

void TextEditorModule::shutdown()
{
    LOG_INFO("TextEditorModule", "Shutting down Text Editor module");
    if (m_findDialog) m_findDialog->close();
}

QDockWidget* TextEditorModule::getOrCreateDockWidget(QMainWindow* mainWindow)
{
    if (m_dockWidget) return m_dockWidget;

    m_dockWidget = new QDockWidget("Text Editor", mainWindow);
    m_dockWidget->setObjectName("TextEditorDock");
    m_dockWidget->setAllowedAreas(Qt::AllDockWidgetAreas);

    setupUI();

    m_dockWidget->setWidget(this);
    return m_dockWidget;
}

void TextEditorModule::setupUI()
{
    if (m_tabWidget) return;

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    setupToolbar(mainLayout);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane { border: 0; }"
        "QTabBar::tab {"
        "  background: #2d2d2d; color: #aaa; border: 1px solid #3a3a3a;"
        "  padding: 4px 12px; margin: 0; min-width: 80px;"
        "}"
        "QTabBar::tab:selected {"
        "  background: #1e1e1e; color: #fff; border-bottom: 2px solid #569cd6;"
        "}"
        "QTabBar::tab:hover { background: #3a3a3a; }"
    );

    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &TextEditorModule::onCloseTab);
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &TextEditorModule::onTabChanged);

    mainLayout->addWidget(m_tabWidget);
}

void TextEditorModule::setupToolbar(QVBoxLayout* mainLayout)
{
    m_toolbar = new QToolBar("Text Editor Toolbar", this);
    m_toolbar->setIconSize(QSize(16, 16));
    m_toolbar->setStyleSheet(
        "QToolBar { background: #2d2d2d; border: none; border-bottom: 1px solid #3a3a3a; padding: 2px; spacing: 2px; }"
        "QToolButton { padding: 3px 8px; border-radius: 3px; color: #ccc; }"
        "QToolButton:hover { background: #3a3a3a; }"
        "QToolButton:pressed { background: #4a4a4a; }"
    );

    auto addBtn = [&](const QString& text, std::function<void()> slot) {
        auto* btn = new QPushButton(text, m_toolbar);
        btn->setStyleSheet(
            "QPushButton { background: #3a5a8a; color: #fff; border: 1px solid #4a6a9a; padding: 3px 10px; }"
            "QPushButton:hover { background: #4a6a9a; }"
        );
        connect(btn, &QPushButton::clicked, this, slot);
        m_toolbar->addWidget(btn);
        return btn;
    };

    m_newBtn = addBtn("New", [this](){ onNewFile(); });
    m_openBtn = addBtn("Open", [this](){ onOpenFile(); });
    m_saveBtn = addBtn("Save", [this](){ onSaveFile(); });
    m_saveAsBtn = addBtn("Save As", [this](){ onSaveAsFile(); });
    m_toolbar->addSeparator();
    m_undoBtn = addBtn("Undo", [this](){ onUndo(); });
    m_redoBtn = addBtn("Redo", [this](){ onRedo(); });
    m_toolbar->addSeparator();
    m_findBtn = addBtn("Find", [this](){ onFind(); });
    m_replaceBtn = addBtn("Replace", [this](){ onReplace(); });
    m_gotoBtn = addBtn("Go To Line", [this](){ onGotoLine(); });
    m_toolbar->addSeparator();
    auto zoomInBtn = addBtn("Zoom +", [this](){ onZoomIn(); });
    auto zoomOutBtn = addBtn("Zoom -", [this](){ onZoomOut(); });

    m_toolbar->addSeparator();
    // Editor keyboard shortcuts
    QShortcut* findShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F), this);
    connect(findShortcut, &QShortcut::activated, this, &TextEditorModule::onFind);
    QShortcut* replaceShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_H), this);
    connect(replaceShortcut, &QShortcut::activated, this, &TextEditorModule::onReplace);
    QShortcut* gotoShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_G), this);
    connect(gotoShortcut, &QShortcut::activated, this, &TextEditorModule::onGotoLine);

    addBtn("Fold All", [this](){ onFoldAll(); });
    addBtn("Unfold All", [this](){ onUnfoldAll(); });

    m_toolbar->addSeparator();
    m_statusLabel = new QLabel("No file open", m_toolbar);
    m_statusLabel->setStyleSheet("color: #888; padding: 0 8px;");
    m_toolbar->addWidget(m_statusLabel);

    mainLayout->addWidget(m_toolbar);
}

void TextEditorModule::onNewFile()
{
    if (!m_tabWidget) setupUI();

    auto* editor = new CodeEditor(this);
    int index = m_tabWidget->addTab(editor, "untitled");
    m_tabPaths[index] = QString();
    m_tabModified[index] = false;
    m_tabWidget->setCurrentIndex(index);
    editor->setFocus();

    connect(editor, &CodeEditor::textChanged, this, &TextEditorModule::onDocumentModified);

    updateTitle(index);
    m_findDialog->setEditor(editor);
}

void TextEditorModule::onOpenFile()
{
    QStringList files = QFileDialog::getOpenFileNames(this, "Open File(s)",
        QString(), "All Files (*)");
    for (const QString& path : files) {
        importFile(path);
    }
}

void TextEditorModule::importFile(const QString& filePath)
{
    if (filePath.isEmpty()) return;

    int existingTab = findTabByPath(filePath);
    if (existingTab >= 0) {
        m_tabWidget->setCurrentIndex(existingTab);
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_ERROR("TextEditorModule", "Failed to open file: " + filePath);
        return;
    }

    if (!m_tabWidget) setupUI();

    auto* editor = new CodeEditor(this);
    QTextStream in(&file);
    editor->setPlainText(in.readAll());
    file.close();

    SyntaxHighlighter::Language lang = SyntaxHighlighter::detectLanguage(filePath);
    if (lang != SyntaxHighlighter::None) {
        auto* highlighter = new SyntaxHighlighter();
        highlighter->setLanguage(lang);
        editor->setSyntaxHighlighter(highlighter);
    }

    QString fileName = QFileInfo(filePath).fileName();
    int index = m_tabWidget->addTab(editor, fileName);
    m_tabPaths[index] = filePath;
    m_tabModified[index] = false;
    m_tabWidget->setCurrentIndex(index);

    connect(editor, &CodeEditor::textChanged, this, &TextEditorModule::onDocumentModified);
    m_findDialog->setEditor(editor);

    updateTitle(index);
    emit fileOpened(filePath);
    LOG_INFO("TextEditorModule", "Opened file: " + filePath);
}

void TextEditorModule::onSaveFile()
{
    int index = m_tabWidget->currentIndex();
    if (index < 0) return;

    QString path = m_tabPaths.value(index);
    if (path.isEmpty()) {
        onSaveAsFile();
        return;
    }

    auto* editor = qobject_cast<CodeEditor*>(m_tabWidget->widget(index));
    if (!editor) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_ERROR("TextEditorModule", "Failed to save file: " + path);
        return;
    }

    file.write(editor->toPlainText().toUtf8());
    file.close();

    setEditorModified(index, false);
    updateTitle(index);
    emit fileSaved(path);
    LOG_INFO("TextEditorModule", "Saved file: " + path);
}

void TextEditorModule::onSaveAsFile()
{
    int index = m_tabWidget->currentIndex();
    if (index < 0) return;

    auto* editor = qobject_cast<CodeEditor*>(m_tabWidget->widget(index));
    if (!editor) return;

    QString path = QFileDialog::getSaveFileName(this, "Save File As",
        m_tabPaths.value(index), "All Files (*)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    file.write(editor->toPlainText().toUtf8());
    file.close();

    m_tabPaths[index] = path;
    setEditorModified(index, false);
    m_tabWidget->setTabText(index, QFileInfo(path).fileName());
    updateTitle(index);

    SyntaxHighlighter::Language lang = SyntaxHighlighter::detectLanguage(path);
    if (lang != SyntaxHighlighter::None) {
        auto* highlighter = new SyntaxHighlighter();
        highlighter->setLanguage(lang);
        editor->setSyntaxHighlighter(highlighter);
    }

    emit fileSaved(path);
}

void TextEditorModule::onCloseTab(int index)
{
    if (index < 0 || index >= m_tabWidget->count()) return;

    if (m_tabModified.value(index, false)) {
        QString name = m_tabWidget->tabText(index);
        auto reply = QMessageBox::question(this, "Unsaved Changes",
            QString("Save changes to '%1'?").arg(name),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (reply == QMessageBox::Save) {
            m_tabWidget->setCurrentIndex(index);
            onSaveFile();
        } else if (reply == QMessageBox::Cancel) {
            return;
        }
    }

    QString path = m_tabPaths.value(index);
    m_tabPaths.remove(index);
    m_tabModified.remove(index);
    m_tabWidget->removeTab(index);

    if (!path.isEmpty()) emit fileClosed(path);
}

void TextEditorModule::onCloseCurrentTab()
{
    onCloseTab(m_tabWidget->currentIndex());
}

void TextEditorModule::onTabChanged(int index)
{
    if (index >= 0) {
        auto* editor = qobject_cast<CodeEditor*>(m_tabWidget->widget(index));
        if (editor) m_findDialog->setEditor(editor);
        updateTitle(index);
    } else {
        m_statusLabel->setText("No file open");
    }
}

void TextEditorModule::onDocumentModified()
{
    auto* editor = qobject_cast<CodeEditor*>(sender());
    if (!editor) return;

    int index = m_tabWidget->indexOf(editor);
    if (index < 0) return;

    setEditorModified(index, true);
    updateTitle(index);
}

void TextEditorModule::setEditorModified(int index, bool modified)
{
    bool wasModified = m_tabModified.value(index, false);
    if (wasModified == modified) return;

    m_tabModified[index] = modified;
    QString text = m_tabWidget->tabText(index);
    if (modified && !text.endsWith(" *")) {
        m_tabWidget->setTabText(index, text + " *");
    } else if (!modified && text.endsWith(" *")) {
        m_tabWidget->setTabText(index, text.left(text.length() - 2));
    }
}

void TextEditorModule::updateTitle(int index)
{
    if (index < 0) {
        m_statusLabel->setText("No file open");
        return;
    }

    QString path = m_tabPaths.value(index);
    QString lineInfo;
    auto* editor = qobject_cast<CodeEditor*>(m_tabWidget->widget(index));
    if (editor) {
        lineInfo = QString("  |  Line %1").arg(editor->currentLineNumber());
    }

    if (path.isEmpty()) {
        m_statusLabel->setText(QString("untitled%1").arg(lineInfo));
    } else {
        m_statusLabel->setText(QString("%1%2").arg(path, lineInfo));
    }

    SyntaxHighlighter::Language lang = SyntaxHighlighter::detectLanguage(path);
    if (lang != SyntaxHighlighter::None) {
        m_statusLabel->setText(m_statusLabel->text() +
            QString("  [%1]").arg(SyntaxHighlighter::languageName(lang)));
    }
}

CodeEditor* TextEditorModule::currentEditor() const
{
    return qobject_cast<CodeEditor*>(m_tabWidget->currentWidget());
}

int TextEditorModule::findTabByPath(const QString& path) const
{
    for (auto it = m_tabPaths.begin(); it != m_tabPaths.end(); ++it) {
        if (it.value() == path) return it.key();
    }
    return -1;
}

void TextEditorModule::onUndo()
{
    auto* editor = currentEditor();
    if (editor) editor->undo();
}

void TextEditorModule::onRedo()
{
    auto* editor = currentEditor();
    if (editor) editor->redo();
}

void TextEditorModule::onFind()
{
    if (!m_findDialog) return;
    if (m_findDialog->isHidden()) {
        m_findDialog->show();
        m_findDialog->raise();
    }
    m_findDialog->activateWindow();
}

void TextEditorModule::onReplace()
{
    onFind();
}

void TextEditorModule::onGotoLine()
{
    auto* editor = currentEditor();
    if (!editor) return;

    bool ok;
    int line = QInputDialog::getInt(this, "Go to Line", "Line number:",
        editor->currentLineNumber(), 1, 1000000, 1, &ok);
    if (ok) editor->gotoLine(line);
}

void TextEditorModule::onZoomIn() { applyZoom(kZoomStep); }
void TextEditorModule::onZoomOut() { applyZoom(-kZoomStep); }
void TextEditorModule::onResetZoom() { applyZoom(kBaseFontSize - m_currentFontSize); }

void TextEditorModule::onFoldAll()
{
    if (auto* editor = currentEditor()) {
        QTextDocument* doc = editor->document();
        QTextBlock block = doc->begin();
        // Fold all blocks that have children at higher level
        while (block.isValid()) {
            if (!editor->isBlockFolded(block)) {
                QTextBlock next = block.next();
                if (next.isValid() && !next.text().trimmed().isEmpty()) {
                    editor->foldBlock(block);
                }
            }
            block = block.next();
        }
    }
}

void TextEditorModule::onUnfoldAll()
{
    if (auto* editor = currentEditor()) {
        editor->unfoldAll();
    }
}

void TextEditorModule::applyZoom(int delta)
{
    m_currentFontSize = qBound(kMinZoom, m_currentFontSize + delta, kMaxZoom);
    QFont font("Consolas", m_currentFontSize);
    font.setStyleHint(QFont::Monospace);

    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto* editor = qobject_cast<CodeEditor*>(m_tabWidget->widget(i));
        if (editor) {
            editor->setFont(font);
            editor->setTabStopDistance(QFontMetrics(font).horizontalAdvance(' ') * 4);
        }
    }
}

void TextEditorModule::openFileAtLine(const QString& filePath, int line)
{
    importFile(filePath);
    auto* editor = currentEditor();
    if (editor) {
        editor->gotoLine(line);
        editor->setFocus();
    }
}

void TextEditorModule::exportFile(const QString& filePath)
{
    onSaveAsFile();
}

bool TextEditorModule::canCut() const { return currentEditor() != nullptr; }
bool TextEditorModule::canCopy() const { return currentEditor() != nullptr; }
bool TextEditorModule::canPaste() const { return currentEditor() != nullptr; }
bool TextEditorModule::canDelete() const { return currentEditor() != nullptr; }

void TextEditorModule::cut()
{
    auto* editor = currentEditor();
    if (editor) editor->cut();
}

void TextEditorModule::copy()
{
    auto* editor = currentEditor();
    if (editor) editor->copy();
}

void TextEditorModule::paste()
{
    auto* editor = currentEditor();
    if (editor) editor->paste();
}

void TextEditorModule::deleteSelected()
{
    auto* editor = currentEditor();
    if (editor) {
        QTextCursor cursor = editor->textCursor();
        if (cursor.hasSelection()) {
            cursor.removeSelectedText();
        }
    }
}

void TextEditorModule::onActivation()
{
    if (!m_tabWidget) {
        setupUI();
    }
    if (m_statusLabel) m_statusLabel->setText("Active");
}

void TextEditorModule::onDeactivation()
{
    if (m_statusLabel) m_statusLabel->setText("Inactive");
}

QJsonObject TextEditorModule::serializeProject() const
{
    QJsonObject data;
    QJsonArray tabsArray;
    for (auto it = m_tabPaths.constBegin(); it != m_tabPaths.constEnd(); ++it) {
        QJsonObject tabObj;
        tabObj["index"] = it.key();
        tabObj["path"] = it.value();
        tabObj["modified"] = m_tabModified.value(it.key(), false);
        tabsArray.append(tabObj);
    }
    data["tabs"] = tabsArray;
    data["fontSize"] = m_currentFontSize;
    return data;
}

void TextEditorModule::deserializeProject(const QJsonObject& data)
{
    m_currentFontSize = data["fontSize"].toInt(kBaseFontSize);
    for (const auto& v : data["tabs"].toArray()) {
        QJsonObject tabObj = v.toObject();
        int idx = tabObj["index"].toInt();
        QString path = tabObj["path"].toString();
        if (!path.isEmpty()) {
            m_tabPaths[idx] = path;
            m_tabModified[idx] = tabObj["modified"].toBool();
        }
    }
}

} // namespace ks
