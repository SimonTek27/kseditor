#include "CodeEditor.h"
#include "SyntaxHighlighter.h"
#include "TreeSitterHighlighter.h"
#include "LSPClient.h"
#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QTextCursor>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QDebug>
#include <QTimer>
#include <QToolTip>

namespace ks {

CodeEditor::CodeEditor(QWidget* parent)
    : QPlainTextEdit(parent)
    , m_completer(new QCompleter(this))
    , m_completionModel(new QStringListModel(this))
    , m_completionTimer(new QTimer(this))
{
    setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setTabStopDistance(4 * fontMetrics().horizontalAdvance(' '));
    
    // Line number area
    m_lineNumberArea = new LineNumberArea(this);
    
    // Fold area
    m_foldArea = new FoldArea(this);
    
    // Minimap
    m_minimap = new MinimapWidget(this);
    m_minimap->setVisible(false);
    
    // Completer setup
    m_completer->setModel(m_completionModel);
    m_completer->setWidget(this);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    
    connect(m_completer, QOverload<const QString&>::of(&QCompleter::activated),
            this, &CodeEditor::insertCompletion);
    
    // Completion timer
    m_completionTimer->setSingleShot(true);
    m_completionTimer->setInterval(300);
    connect(m_completionTimer, &QTimer::timeout, this, &CodeEditor::showCompletion);
    
    // Connections
    connect(this, &QPlainTextEdit::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::updateRequest, this, &CodeEditor::updateFoldArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);
    connect(this, &QPlainTextEdit::textChanged, this, &CodeEditor::matchBrackets);
    
    // Document connections
    connect(document(), &QTextDocument::contentsChanged, this, &CodeEditor::recalculateFoldLevels);
    connect(document(), &QTextDocument::contentsChanged, this, &CodeEditor::buildWordIndex);
    connect(document(), &QTextDocument::contentsChanged, this, &CodeEditor::updateMinimap);
    
    // LSP Client
    m_lspClient = new LSPClient(this);
    connect(m_lspClient, &LSPClient::diagnosticsReceived, this, &CodeEditor::onDiagnosticsReceived);
    connect(m_lspClient, &LSPClient::completionReceived, this, &CodeEditor::onLSPCompletionReceived);
    connect(m_lspClient, &LSPClient::hoverReceived, this, &CodeEditor::onLSPHoverReceived);
    
    // Highlighter
    m_highlighter = new TreeSitterHighlighter(document());
    
    // Initialize
    initSnippets();
    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
    
    // Context menu
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        QMenu menu(this);
        menu.addAction(tr("Undo"), this, &CodeEditor::undo, QKeySequence::Undo);
        menu.addAction(tr("Redo"), this, &CodeEditor::redo, QKeySequence::Redo);
        menu.addSeparator();
        menu.addAction(tr("Cut"), this, &CodeEditor::cut, QKeySequence::Cut);
        menu.addAction(tr("Copy"), this, &CodeEditor::copy, QKeySequence::Copy);
        menu.addAction(tr("Paste"), this, &CodeEditor::paste, QKeySequence::Paste);
        menu.addSeparator();
        menu.addAction(tr("Select All"), this, &CodeEditor::selectAll, QKeySequence::SelectAll);
        menu.addSeparator();
        menu.addAction(tr("Fold"), this, [this]() { 
            QTextCursor cursor = textCursor();
            if (cursor.hasSelection()) {
                // Fold selected blocks
            } else {
                toggleFold(cursor.block());
            }
        });
        menu.addAction(tr("Unfold"), this, [this]() {
            QTextCursor cursor = textCursor();
            unfoldBlock(cursor.block());
        });
        menu.addAction(tr("Unfold All"), this, &CodeEditor::unfoldAll);
        menu.addSeparator();
        menu.addAction(tr("Toggle Comment"), this, [this]() { toggleComment(); }, QKeySequence(Qt::CTRL | Qt::Key_Slash));
        menu.exec(mapToGlobal(pos));
    });
}

CodeEditor::~CodeEditor() {}

void CodeEditor::setSyntaxHighlighter(QSyntaxHighlighter* highlighter) {
    if (m_highlighter) {
        delete m_highlighter;
    }
    m_highlighter = highlighter;
    if (m_highlighter) {
        m_highlighter->setDocument(document());
    }
}

void CodeEditor::setLanguage(const QString& language) {
    m_language = language;
    if (auto* tsHighlighter = qobject_cast<TreeSitterHighlighter*>(m_highlighter)) {
        auto langEnum = TreeSitterHighlighter::Language::Unknown;
        if (language.compare("cpp", Qt::CaseInsensitive) == 0 || language.compare("c++", Qt::CaseInsensitive) == 0)
            langEnum = TreeSitterHighlighter::Language::Cpp;
        else if (language.compare("c", Qt::CaseInsensitive) == 0)
            langEnum = TreeSitterHighlighter::Language::C;
        else if (language.compare("python", Qt::CaseInsensitive) == 0)
            langEnum = TreeSitterHighlighter::Language::Python;
        else if (language.compare("lua", Qt::CaseInsensitive) == 0)
            langEnum = TreeSitterHighlighter::Language::Lua;
        else if (language.compare("glsl", Qt::CaseInsensitive) == 0)
            langEnum = TreeSitterHighlighter::Language::GLSL;
        else if (language.compare("hlsl", Qt::CaseInsensitive) == 0)
            langEnum = TreeSitterHighlighter::Language::HLSL;
        else if (language.compare("json", Qt::CaseInsensitive) == 0)
            langEnum = TreeSitterHighlighter::Language::JSON;
        else if (language.compare("xml", Qt::CaseInsensitive) == 0)
            langEnum = TreeSitterHighlighter::Language::XML;
        else if (language.compare("yaml", Qt::CaseInsensitive) == 0 || language.compare("yml", Qt::CaseInsensitive) == 0)
            langEnum = TreeSitterHighlighter::Language::YAML;
        else if (language.compare("markdown", Qt::CaseInsensitive) == 0 || language.compare("md", Qt::CaseInsensitive) == 0)
            langEnum = TreeSitterHighlighter::Language::Markdown;
        else if (language.compare("ini", Qt::CaseInsensitive) == 0)
            langEnum = TreeSitterHighlighter::Language::INI;
        tsHighlighter->setLanguage(langEnum);
    }
    rebuildWordIndex();
}

void CodeEditor::gotoLine(int line) {
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, line - 1);
    setTextCursor(cursor);
    ensureCursorVisible();
}

void CodeEditor::resizeEvent(QResizeEvent* event) {
    QPlainTextEdit::resizeEvent(event);
    
    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
    m_foldArea->setGeometry(QRect(cr.left() + lineNumberAreaWidth(), cr.top(), foldAreaWidth(), cr.height()));
    
    // Position minimap on right
    if (m_minimap->isVisible()) {
        m_minimap->setGeometry(QRect(cr.right() - 120, cr.top(), 120, cr.height()));
    }
}

void CodeEditor::paintEvent(QPaintEvent* event) {
    QPlainTextEdit::paintEvent(event);
    
    // Draw bracket match highlighting
    if (m_bracketMatchStart.isValid() && m_bracketMatchEnd.isValid()) {
        QPainter painter(viewport());
        QColor matchColor = palette().color(QPalette::Highlight).lighter(120);
        painter.fillRect(blockBoundingGeometry(m_bracketMatchStart).translated(contentOffset()), matchColor);
        painter.fillRect(blockBoundingGeometry(m_bracketMatchEnd).translated(contentOffset()), matchColor);
    }
}

void CodeEditor::keyPressEvent(QKeyEvent* event) {
    // Handle multi-cursor operations
    if (hasExtraCursors()) {
        if (event->matches(QKeySequence::Copy)) {
            applyKeyToAllCursors(textCursor().selectedText());
            return;
        }
        if (event->matches(QKeySequence::Paste)) {
            applyKeyToAllCursors(QApplication::clipboard()->text());
            return;
        }
        if (event->key() == Qt::Key_Delete) {
            applyDeleteToAllCursors();
            return;
        }
        if (event->key() == Qt::Key_Backspace) {
            applyBackspaceToAllCursors();
            return;
        }
        if (event->key() == Qt::Key_Escape) {
            clearExtraCursors();
            return;
        }
        
        // Allow typing for multi-cursor
        if (event->text().length() > 0 && !event->modifiers().testFlag(Qt::ControlModifier)) {
            applyKeyToAllCursors(event->text());
            return;
        }
    }
    
    // Completion trigger
    if (event->key() == Qt::Key_Space && event->modifiers().testFlag(Qt::ControlModifier)) {
        showCompletion();
        return;
    }
    
    // Snippet expansion
    if (event->key() == Qt::Key_Tab) {
        QTextCursor cursor = textCursor();
        if (tryExpandSnippet(cursor)) {
            return;
        }
    }
    
    // Multi-cursor: Alt+Click or Ctrl+Alt+Up/Down
    if (event->key() == Qt::Key_Up && event->modifiers() == (Qt::ControlModifier | Qt::AltModifier)) {
        addCursorAbove();
        return;
    }
    if (event->key() == Qt::Key_Down && event->modifiers() == (Qt::ControlModifier | Qt::AltModifier)) {
        addCursorBelow();
        return;
    }
    
    // Auto-pair brackets/quotes
    if (event->text().length() == 1) {
        QChar ch = event->text().at(0);
        if (ch == '"' || ch == '\'' || ch == '(' || ch == '[' || ch == '{') {
            QTextCursor cursor = textCursor();
            if (cursor.hasSelection()) {
                // Surround selection
                cursor.insertText(ch + cursor.selectedText() + matchingChar(ch));
            } else {
                // Auto-pair
                cursor.insertText(QString(ch) + matchingChar(ch));
                cursor.movePosition(QTextCursor::Left);
                setTextCursor(cursor);
            }
            return;
        }
        // Auto-close on typing closing bracket
        if (ch == ')' || ch == ']' || ch == '}' || ch == '"' || ch == '\'') {
            QTextCursor cursor = textCursor();
            if (!cursor.atBlockEnd() && cursor.document()->characterAt(cursor.position()) == ch) {
                cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
                cursor.removeSelectedText();
                setTextCursor(cursor);
                return;
            }
        }
    }
    
    // Auto-indent on Enter
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        QTextCursor cursor = textCursor();
        QString currentLine = cursor.block().text();
        int indent = 0;
        for (int i = 0; i < currentLine.length(); ++i) {
            if (currentLine[i].isSpace()) indent++;
            else break;
        }
        if (indent > 0) {
            QString indentStr = currentLine.left(indent);
            // Extra indent after {
            if (currentLine.trimmed().endsWith('{') || currentLine.trimmed().endsWith(':')) {
                indentStr += "    ";
            }
            cursor.insertText("\n" + indentStr);
            return;
        }
    }
    
    QPlainTextEdit::keyPressEvent(event);
}

void CodeEditor::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && event->modifiers() == Qt::AltModifier) {
        // Add multi-cursor at click position
        QTextCursor cursor = cursorForPosition(event->pos());
        addExtraCursor(cursor);
        return;
    }
    
    if (event->button() == Qt::LeftButton && event->modifiers() == Qt::ControlModifier) {
        // Ctrl+Click for go to definition
        if (m_lspClient) {
            QTextCursor cursor = cursorForPosition(event->pos());
            m_lspClient->gotoDefinition(document()->baseUrl().toLocalFile(), cursor.blockNumber(), cursor.positionInBlock());
        }
        return;
    }
    
    QPlainTextEdit::mousePressEvent(event);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent* event) {
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), palette().alternateBase());
    
    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    qreal top = blockBoundingGeometry(block).translated(contentOffset()).top();
    qreal bottom = top + blockBoundingRect(block).height();
    
    QFont font = painter.font();
    font.setPointSizeF(font.pointSizeF() * 0.85);
    painter.setFont(font);
    
    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(palette().color(QPalette::WindowText).lighter(120));
            painter.drawText(0, top, m_lineNumberArea->width() - 4, 
                           fontMetrics().height(), Qt::AlignRight, number);
        }
        
        block = block.next();
        top = bottom;
        bottom = top + blockBoundingRect(block).height();
        ++blockNumber;
    }
}

int CodeEditor::lineNumberAreaWidth() const {
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    int space = 8 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void CodeEditor::updateLineNumberAreaWidth(int) {
    setViewportMargins(lineNumberAreaWidth() + foldAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect& rect, int dy) {
    if (dy) m_lineNumberArea->scroll(0, dy);
    else m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
    
    if (rect.contains(viewport()->rect())) updateLineNumberAreaWidth(0);
}

void CodeEditor::foldAreaPaintEvent(QPaintEvent* event) {
    QPainter painter(m_foldArea);
    painter.fillRect(event->rect(), palette().alternateBase());
    
    QTextBlock block = firstVisibleBlock();
    qreal top = blockBoundingGeometry(block).translated(contentOffset()).top();
    qreal bottom = top + blockBoundingRect(block).height();
    
    QFont font = painter.font();
    font.setPointSizeF(font.pointSizeF() * 0.7);
    painter.setFont(font);
    
    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            int blockNumber = block.blockNumber();
            if (m_foldLevels.value(blockNumber, 0) > 0 && m_foldDeltas.value(blockNumber, 0) > 0) {
                // Draw fold marker
                QRectF rect(2, top + 2, foldAreaWidth() - 4, blockBoundingRect(block).height() - 4);
                if (m_foldedBlocks.contains(blockNumber)) {
                    // Folded - draw +
                    painter.setPen(Qt::black);
                    painter.drawText(rect, Qt::AlignCenter, "+");
                } else {
                    // Unfolded - draw -
                    painter.setPen(Qt::black);
                    painter.drawText(rect, Qt::AlignCenter, "-");
                }
            }
        }
        
        block = block.next();
        top = bottom;
        bottom = top + blockBoundingRect(block).height();
    }
}

int CodeEditor::foldAreaWidth() const {
    return 16;
}

void CodeEditor::updateFoldArea(const QRect& rect, int dy) {
    if (dy) m_foldArea->scroll(0, dy);
    else m_foldArea->update(0, rect.y(), m_foldArea->width(), rect.height());
}

void CodeEditor::recalculateFoldLevels() {
    m_foldDeltas.clear();
    m_foldLevels.clear();
    
    QTextBlock block = document()->begin();
    int level = 0;
    
    while (block.isValid()) {
        QString text = block.text().trimmed();
        int blockNum = block.blockNumber();
        
        // Calculate fold delta for this block
        int delta = 0;
        if (text.endsWith('{') || text.endsWith(':')) {
            delta = 1;
        } else if (text.startsWith('}') || text.startsWith("elif") || text.startsWith("else:") || text.startsWith("except:") || text.startsWith("finally:")) {
            delta = -1;
        }
        
        m_foldDeltas[blockNum] = delta;
        
        // Apply previous level to this block
        m_foldLevels[blockNum] = level;
        
        level += delta;
        if (level < 0) level = 0;
        
        block = block.next();
    }
    
    applyFolding();
    m_foldArea->update();
}

void CodeEditor::applyFolding() {
    QTextCursor cursor = textCursor();
    int oldPos = cursor.position();
    
    for (int blockNum : m_foldedBlocks) {
        QTextBlock block = document()->findBlockByNumber(blockNum);
        if (!block.isValid()) continue;
        
        int level = m_foldLevels.value(blockNum, 0);
        int nextBlockNum = blockNum + 1;
        
        while (nextBlockNum < blockCount()) {
            QTextBlock nextBlock = document()->findBlockByNumber(nextBlockNum);
            if (!nextBlock.isValid()) break;
            
            int nextLevel = m_foldLevels.value(nextBlockNum, 0);
            if (nextLevel <= m_foldLevels.value(blockNum, 0)) break;
            
            nextBlock.setVisible(false);
            nextBlockNum++;
        }
    }
    
    // Unfold blocks not in folded set
    QTextBlock block = document()->begin();
    while (block.isValid()) {
        if (!m_foldedBlocks.contains(block.blockNumber())) {
            block.setVisible(true);
        }
        block = block.next();
    }
    
    QTextCursor restoreCursor = textCursor();
    restoreCursor.setPosition(oldPos);
    setTextCursor(restoreCursor);
}

int CodeEditor::computeFoldLevel(const QTextBlock& block) const {
    return m_foldLevels.value(block.blockNumber(), 0);
}

QTextBlock CodeEditor::findFoldEnd(const QTextBlock& startBlock) const {
    int startLevel = m_foldLevels.value(startBlock.blockNumber(), 0);
    QTextBlock block = startBlock.next();
    
    while (block.isValid()) {
        int level = m_foldLevels.value(block.blockNumber(), 0);
        if (level <= startLevel) return block;
        block = block.next();
    }
    return QTextBlock();
}

bool CodeEditor::isBlockFolded(const QTextBlock& block) const {
    return m_foldedBlocks.contains(block.blockNumber());
}

void CodeEditor::toggleFold(const QTextBlock& block) {
    int blockNum = block.blockNumber();
    if (m_foldedBlocks.contains(blockNum)) {
        unfoldBlock(block);
    } else {
        foldBlock(block);
    }
    m_foldArea->update();
}

void CodeEditor::foldBlock(const QTextBlock& block) {
    int blockNum = block.blockNumber();
    if (m_foldDeltas.value(blockNum, 0) <= 0) return; // Not foldable
    
    m_foldedBlocks.insert(blockNum);
    applyFolding();
}

void CodeEditor::unfoldBlock(const QTextBlock& block) {
    int blockNum = block.blockNumber();
    m_foldedBlocks.remove(blockNum);
    applyFolding();
}

void CodeEditor::unfoldAll() {
    m_foldedBlocks.clear();
    applyFolding();
    m_foldArea->update();
}

void CodeEditor::selectNextOccurrence() {
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::WordUnderCursor);
        setTextCursor(cursor);
        return;
    }
    
    QString selectedText = cursor.selectedText();
    if (selectedText.isEmpty()) return;
    
    QTextDocument::FindFlags flags = QTextDocument::FindCaseSensitively;
    QTextCursor found = document()->find(selectedText, textCursor(), flags);
    if (!found.isNull()) {
        addExtraCursor(found);
        setTextCursor(found);
    } else {
        // Wrap around
        QTextCursor startCursor = textCursor();
        startCursor.movePosition(QTextCursor::Start);
        QTextCursor found = document()->find(selectedText, startCursor, flags);
        if (!found.isNull()) {
            addExtraCursor(found);
            setTextCursor(found);
        }
    }
}

void CodeEditor::addExtraCursor(const QTextCursor& cursor) {
    QTextCursor extra = cursor;
    extra.setCharFormat(QTextCharFormat());
    m_extraCursors.append(extra);
    highlightCurrentLine();
}

void CodeEditor::clearExtraCursors() {
    m_extraCursors.clear();
    setExtraSelections({});
}

void CodeEditor::addCursorAbove() {
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::Up);
    addExtraCursor(cursor);
}

void CodeEditor::addCursorBelow() {
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::Down);
    addExtraCursor(cursor);
}

void CodeEditor::addMultiCursorSelection(const QTextCursor& cursor) {
    m_extraCursors.append(cursor);
    highlightCurrentLine();
}

void CodeEditor::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> extraSelections;
    
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = palette().color(QPalette::Highlight).lighter(160);
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }
    
    // Also add extra cursors
    for (const auto& cursor : m_extraCursors) {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(QColor(255, 255, 0, 80));
        selection.cursor = cursor;
        extraSelections.append(selection);
    }
    
    setExtraSelections(extraSelections);
}

void CodeEditor::matchBrackets() {
    m_bracketMatchStart = QTextBlock();
    m_bracketMatchEnd = QTextBlock();
    
    QTextCursor cursor = textCursor();
    int pos = cursor.position();
    QTextDocument* doc = document();
    
    if (pos > 0) {
        QChar prevChar = doc->characterAt(pos - 1);
        if (isBracket(prevChar)) {
            int matchPos = findMatchingBracket(pos - 1, false);
            if (matchPos != -1) {
                m_bracketMatchStart = doc->findBlock(pos - 1);
                m_bracketMatchEnd = doc->findBlock(matchPos);
            }
        }
    }
    
    if (pos < doc->characterCount()) {
        QChar nextChar = doc->characterAt(pos);
        if (isBracket(nextChar)) {
            int matchPos = findMatchingBracket(pos, true);
            if (matchPos != -1) {
                m_bracketMatchStart = doc->findBlock(pos);
                m_bracketMatchEnd = doc->findBlock(matchPos);
            }
        }
    }
    
    viewport()->update();
}

bool CodeEditor::isBracket(QChar ch) const {
    return ch == '(' || ch == ')' || ch == '[' || ch == ']' || ch == '{' || ch == '}';
}

QChar CodeEditor::matchingChar(QChar ch) const {
    switch (ch.unicode()) {
        case '(': return ')';
        case ')': return '(';
        case '[': return ']';
        case ']': return '[';
        case '{': return '}';
        case '}': return '{';
        case '"': return '"';
        case '\'': return '\'';
        default: return QChar();
    }
}

int CodeEditor::findMatchingBracket(int pos, bool forward) const {
    QTextDocument* doc = document();
    QChar startChar = doc->characterAt(pos);
    QChar endChar = matchingChar(startChar);
    if (endChar.isNull()) return -1;
    
    int count = 1;
    int step = forward ? 1 : -1;
    int limit = forward ? doc->characterCount() : 0;
    
    for (int i = pos + step; forward ? i < limit : i >= limit; i += step) {
        QChar ch = doc->characterAt(i);
        if (ch == startChar) count++;
        else if (ch == endChar) count--;
        if (count == 0) return i;
    }
    return -1;
}

void CodeEditor::toggleComment() {
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::LineUnderCursor);
    }
    
    QString selectedText = cursor.selectedText();
    QStringList lines = selectedText.split('\n');
    
    bool allCommented = true;
    for (const QString& line : lines) {
        if (!line.trimmed().startsWith("//") && !line.trimmed().startsWith("#")) {
            allCommented = false;
            break;
        }
    }
    
    QString result;
    if (allCommented) {
        for (const QString& line : lines) {
            result += line.trimmed().startsWith("//") ? line.mid(line.indexOf("//") + 2) : 
                     (line.trimmed().startsWith("#") ? line.mid(line.indexOf("#") + 1) : line);
            result += '\n';
        }
    } else {
        for (const QString& line : lines) {
            result += "// " + line + '\n';
        }
    }
    
    cursor.insertText(result);
}

void CodeEditor::initSnippets() {
    m_snippets["for"] = "for (int i = 0; i < count; ++i) {\n    \n}";
    m_snippets["fori"] = "for (int i = 0; i < ${1:count}; ++i) {\n    \n}";
    m_snippets["fore"] = "for (auto& ${1:item} : ${2:container}) {\n    \n}";
    m_snippets["while"] = "while (${1:condition}) {\n    \n}";
    m_snippets["if"] = "if (${1:condition}) {\n    \n}";
    m_snippets["ife"] = "if (${1:condition}) {\n    \n} else {\n    \n}";
    m_snippets["switch"] = "switch (${1:expr}) {\n    case ${2:value}:\n        \n        break;\n    default:\n        \n}";
    m_snippets["class"] = "class ${1:Name} {\npublic:\n    ${1:Name}();\n    ~${1:Name}();\n\nprivate:\n    \n};";
    m_snippets["func"] = "${1:void} ${2:name}(${3:params}) {\n    \n}";
    m_snippets["lambda"] = "[${1:capture}](${2:params}) -> ${3:return} {\n    \n}";
    m_snippets["fori"] = "for (int ${1:i} = 0; ${1:i} < ${2:count}; ++${1:i}) {\n    \n}";
    m_snippets["try"] = "try {\n    \n} catch (const ${1:exception}& e) {\n    \n}";
    m_snippets["singleton"] = "static ${1:Class}& instance() {\n    static ${1:Class} instance;\n    return instance;\n}";
    m_snippets["property"] = "Q_PROPERTY(${1:type} ${2:name} READ ${3:getter} WRITE ${4:setter} NOTIFY ${5:name}Changed)";
    m_snippets["signal"] = "signals:\n    void ${1:name}(${2:params});";
    m_snippets["slot"] = "public slots:\n    void ${1:name}(${2:params});";
}

bool CodeEditor::tryExpandSnippet(QTextCursor& cursor) {
    cursor.select(QTextCursor::WordUnderCursor);
    QString word = cursor.selectedText();
    
    if (m_snippets.contains(word)) {
        QString snippet = m_snippets[word];
        cursor.insertText(snippet);
        
        // Position cursor at first placeholder
        QRegularExpression re("\\$\\{(\\d+):([^}]+)\\}");
        QRegularExpressionMatch match = re.match(snippet);
        int pos = match.capturedStart();
        if (pos >= 0) {
            cursor.setPosition(cursor.position() - (snippet.length() - pos));
            cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, match.capturedLength() - 3);
        }
        return true;
    }
    return false;
}

void CodeEditor::buildWordIndex() {
    m_wordIndex.clear();
    
    QTextBlock block = document()->begin();
    while (block.isValid()) {
        QString text = block.text();
        QStringList words = text.split(QRegularExpression("\\W+"));
        for (const QString& word : words) {
            if (word.length() >= 3) {
                m_wordIndex.insert(word.toLower());
            }
        }
        block = block.next();
    }
}

void CodeEditor::rebuildWordIndex() {
    buildWordIndex();
    m_completionModel->setStringList(m_wordIndex.values().toList());
}

void CodeEditor::showCompletion() {
    QTextCursor cursor = textCursor();
    cursor.select(QTextCursor::WordUnderCursor);
    QString prefix = cursor.selectedText().toLower();
    
    if (prefix.length() < 2) return;
    
    QStringList completions;
    for (const QString& word : m_wordIndex) {
        if (word.startsWith(prefix)) {
            completions << word;
        }
    }
    
    // Add LSP completions if available
    if (m_lspClient && m_lspClient->isConnected()) {
        // Would request from LSP
    }
    
    if (completions.size() > 1) {
        m_completionModel->setStringList(completions);
        m_completer->setCompletionPrefix(prefix);
        m_completer->complete();
    }
}

void CodeEditor::insertCompletion(const QString& completion) {
    QTextCursor cursor = textCursor();
    int extra = completion.length() - m_completer->completionPrefix().length();
    cursor.movePosition(QTextCursor::Left);
    cursor.movePosition(QTextCursor::EndOfWord);
    cursor.insertText(completion.right(extra));
    setTextCursor(cursor);
}

void CodeEditor::onDiagnosticsReceived(const QString& file, const QVector<LSPDiagnostic>& diagnostics) {
    // Show diagnostics as squiggly lines or markers
    QList<QTextEdit::ExtraSelection> selections;
    for (const auto& diag : diagnostics) {
        QTextEdit::ExtraSelection selection;
        QTextBlock block = document()->findBlockByLineNumber(diag.range.start.line);
        selection.cursor = QTextCursor(block);
        selection.cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, diag.range.end.character - diag.range.start.character);
        
        QColor color;
        switch (diag.severity) {
            case 1: color = Qt::red; break;    // Error
            case 2: color = QColor(255, 165, 0); break; // Warning
            case 3: color = Qt::blue; break;   // Info
            case 4: color = Qt::gray; break;   // Hint
            default: color = Qt::gray;
        }
        selection.format.setUnderlineColor(color);
        selection.format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        selection.cursor = QTextCursor(document()->findBlockByLineNumber(diag.range.start.line));
        selection.cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 
                                    diag.range.end.character - diag.range.start.character);
        selections.append(selection);
    }
    setExtraSelections(selections);
}

void CodeEditor::onLSPCompletionReceived(const QStringList& completions) {
    m_completionModel->setStringList(completions);
    m_completer->complete();
}

void CodeEditor::onLSPHoverReceived(const QString& file, int line, const QString& content) {
    // Show hover tooltip
    QToolTip::showText(mapToGlobal(QPoint(0, 0)), content, this);
}

void CodeEditor::addCompletions(const QStringList& words) {
    for (const QString& word : words) {
        if (word.length() >= 3) {
            m_wordIndex.insert(word.toLower());
        }
    }
    rebuildWordIndex();
}

void CodeEditor::addSnippet(const QString& trigger, const QString& expansion) {
    m_snippets[trigger] = expansion;
}

void CodeEditor::updateMinimap() {
    if (m_minimap && m_minimap->isVisible()) {
        m_minimap->update();
    }
}

void CodeEditor::scrollMinimapTo(int y) {
    QScrollBar* vbar = verticalScrollBar();
    int maxY = document()->size().height() - viewport()->height();
    if (maxY > 0) {
        vbar->setValue(qMax(0, y * maxY / 100));
    }
}

void CodeEditor::setMinimapVisible(bool visible) {
    m_minimap->setVisible(visible);
    resizeEvent(new QResizeEvent(size(), size()));
}

bool CodeEditor::isMinimapVisible() const {
    return m_minimap && m_minimap->isVisible();
}

void CodeEditor::removeExtraCursor(int index) {
    if (index >= 0 && index < m_extraCursors.size()) {
        m_extraCursors.removeAt(index);
        highlightCurrentLine();
    }
}

void CodeEditor::applyKeyToAllCursors(const QString& text) {
    QTextCursor mainCursor = textCursor();
    mainCursor.insertText(text);
    
    for (auto& cursor : m_extraCursors) {
        cursor.insertText(text);
    }
}

void CodeEditor::applyDeleteToAllCursors() {
    QTextCursor mainCursor = textCursor();
    if (mainCursor.hasSelection()) {
        mainCursor.removeSelectedText();
    } else {
        mainCursor.deleteChar();
    }
    
    for (auto& cursor : m_extraCursors) {
        if (cursor.hasSelection()) {
            cursor.removeSelectedText();
        } else {
            cursor.deleteChar();
        }
    }
}

void CodeEditor::applyBackspaceToAllCursors() {
    QTextCursor mainCursor = textCursor();
    if (mainCursor.hasSelection()) {
        mainCursor.removeSelectedText();
    } else {
        mainCursor.deletePreviousChar();
    }
    
    for (auto& cursor : m_extraCursors) {
        if (cursor.hasSelection()) {
            cursor.removeSelectedText();
        } else {
            cursor.deletePreviousChar();
        }
    }
}

} // namespace ks