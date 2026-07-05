#include "CodeEditor.h"

#include <QPainter>
#include <QTextBlock>
#include <QTextCursor>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QSyntaxHighlighter>
#include <QTextLayout>
#include <QTextDocument>
#include <QScrollBar>
#include <QAbstractItemView>
#include <QRegularExpression>
#include <QApplication>
#include <QPainterPath>

namespace ks {

CodeEditor::CodeEditor(QWidget* parent)
    : QPlainTextEdit(parent)
{
    m_lineNumberArea = new LineNumberArea(this);
    m_foldArea = new FoldArea(this);
    m_minimap = new MinimapWidget(this, this);
    m_minimap->show(); // shown by default

    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateFoldArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::matchBrackets);
    connect(this, &CodeEditor::textChanged, this, &CodeEditor::recalculateFoldLevels);
    connect(this, &CodeEditor::cursorPositionChanged, this, [this]() {
        if (hasExtraCursors() && !textCursor().hasSelection()) {
            clearExtraCursors();
        }
    });

    // Auto-completion
    m_completionModel = new QStringListModel(this);
    m_completer = new QCompleter(m_completionModel, this);
    m_completer->setWidget(this);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setFilterMode(Qt::MatchStartsWith);
    m_completer->setMaxVisibleItems(12);
    m_completer->popup()->setStyleSheet(
        "QAbstractItemView { background: #2d2d2d; color: #d4d4d4; border: 1px solid #555; "
        "  font-family: Consolas; font-size: 12px; }"
        "QAbstractItemView::item { padding: 2px 6px; }"
        "QAbstractItemView::item:selected { background: #094771; color: white; }"
    );
    connect(m_completer, QOverload<const QString&>::of(&QCompleter::activated),
            this, &CodeEditor::insertCompletion);

    m_completionTimer = new QTimer(this);
    m_completionTimer->setSingleShot(true);
    m_completionTimer->setInterval(400);
    connect(m_completionTimer, &QTimer::timeout, this, &CodeEditor::showCompletion);
    connect(this, &CodeEditor::textChanged, this, [this]() {
        m_completionTimer->start();
    });

    // Initialize built-in snippets
    initSnippets();

    // Minimap updates
    connect(this, &CodeEditor::textChanged, this, &CodeEditor::updateMinimap);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::updateMinimap);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &CodeEditor::updateMinimap);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();

    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 4);
    setCursorWidth(2);

    setStyleSheet(
        "QPlainTextEdit {"
        "  background-color: #1e1e1e;"
        "  color: #d4d4d4;"
        "  font-family: 'Consolas', 'Courier New', monospace;"
        "  font-size: 13px;"
        "  selection-background-color: #264f78;"
        "}"
    );
}

// ── Line numbers ──

int CodeEditor::lineNumberAreaWidth() const
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) { max /= 10; ++digits; }
    return 10 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void CodeEditor::updateLineNumberAreaWidth(int)
{
    int mmWidth = m_minimap && m_minimap->isVisible() ? m_minimap->width() : 0;
    setViewportMargins(foldAreaWidth() + lineNumberAreaWidth(), 0, mmWidth, 0);
}

void CodeEditor::updateLineNumberArea(const QRect& rect, int dy)
{
    if (dy) m_lineNumberArea->scroll(0, dy);
    else m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
    if (rect.contains(viewport()->rect())) updateLineNumberAreaWidth(0);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent* event)
{
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), QColor(0x2d, 0x2d, 0x2d));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            bool isCurrent = (blockNumber == textCursor().blockNumber());
            painter.setPen(isCurrent ? QColor(0xae, 0xaf, 0xad) : QColor(0x85, 0x85, 0x85));
            painter.drawText(0, top, m_lineNumberArea->width() - 4, fontMetrics().height(),
                             Qt::AlignRight, QString::number(blockNumber + 1));
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void CodeEditor::resizeEvent(QResizeEvent* event)
{
    QPlainTextEdit::resizeEvent(event);
    QRect cr = contentsRect();
    int faWidth = foldAreaWidth();
    int lnWidth = lineNumberAreaWidth();
    const int mmWidth = 100;

    m_foldArea->setGeometry(QRect(cr.left(), cr.top(), faWidth, cr.height()));
    m_lineNumberArea->setGeometry(QRect(cr.left() + faWidth, cr.top(), lnWidth, cr.height()));

    if (m_minimap && m_minimap->isVisible()) {
        m_minimap->setGeometry(QRect(cr.right() - mmWidth, cr.top(), mmWidth, cr.height()));
    }
}

// ── Folding ──

int CodeEditor::foldAreaWidth() const
{
    return 14;
}

void CodeEditor::updateFoldArea(const QRect& rect, int dy)
{
    if (dy) m_foldArea->scroll(0, dy);
    else m_foldArea->update(0, rect.y(), m_foldArea->width(), rect.height());
}

void CodeEditor::foldAreaPaintEvent(QPaintEvent* event)
{
    QPainter painter(m_foldArea);
    painter.fillRect(event->rect(), QColor(0x25, 0x25, 0x28));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            int delta = m_foldDeltas.value(blockNumber, 0);
            bool isFoldStart = (delta > 0);
            bool isFolded = m_foldedBlocks.contains(blockNumber);

            if (isFoldStart) {
                int xCenter = m_foldArea->width() / 2;
                int yCenter = top + fontMetrics().height() / 2;
                int size = 10;

                painter.setPen(QColor(0x99, 0x99, 0x99));
                painter.setBrush(isFolded ? QColor(0x66, 0x66, 0x66) : QColor(0x3a, 0x3a, 0x3a));

                QRect rect(xCenter - size / 2, yCenter - size / 2, size, size);
                painter.drawRect(rect);

                painter.setPen(QColor(0xcc, 0xcc, 0xcc));
                painter.setFont(QFont("Consolas", 8, QFont::Bold));
                painter.drawText(rect, Qt::AlignCenter, isFolded ? "+" : "-");
            }
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

int CodeEditor::computeFoldLevel(const QTextBlock& block) const
{
    if (!block.isValid()) return 0;
    QString text = block.text().trimmed();

    if (text.isEmpty()) return 0;

    int commentIdx = text.indexOf("//");
    if (commentIdx >= 0) text = text.left(commentIdx).trimmed();

    int openers = text.count('{') + text.count('(') + text.count('[');
    int closers = text.count('}') + text.count(')') + text.count(']');

    if (text.startsWith("#if") || text.startsWith("#ifdef") || text.startsWith("#ifndef")
        || text.startsWith("#elif") || text.startsWith("#else") || text.startsWith("#pragma")
        || text.startsWith("#region"))
        openers++;
    if (text.startsWith("#endif") || text.startsWith("#elif") || text.startsWith("#else")
        || text.startsWith("#endregion"))
        closers++;

    return openers - closers;
}

QTextBlock CodeEditor::findFoldEnd(const QTextBlock& startBlock) const
{
    if (!startBlock.isValid()) return QTextBlock();

    int targetLevel = m_foldLevels.value(startBlock.blockNumber(), 0);

    QTextBlock block = startBlock.next();
    while (block.isValid()) {
        int level = m_foldLevels.value(block.blockNumber(), 0);
        if (level < targetLevel) return block;
        block = block.next();
    }
    return QTextBlock();
}

void CodeEditor::recalculateFoldLevels()
{
    m_foldDeltas.clear();
    m_foldLevels.clear();
    QTextBlock block = document()->begin();
    int runningLevel = 0;

    while (block.isValid()) {
        int delta = computeFoldLevel(block);
        m_foldDeltas[block.blockNumber()] = delta;
        runningLevel += delta;
        m_foldLevels[block.blockNumber()] = runningLevel;
        block = block.next();
    }

    m_foldedBlocks.clear();
    QTextBlock b = document()->begin();
    while (b.isValid()) {
        b.setVisible(true);
        b = b.next();
    }
    applyFolding();
    emit foldingChanged();
}

bool CodeEditor::isBlockFolded(const QTextBlock& block) const
{
    return m_foldedBlocks.contains(block.blockNumber());
}

void CodeEditor::toggleFold(const QTextBlock& block)
{
    if (!block.isValid()) return;

    if (m_foldedBlocks.contains(block.blockNumber())) {
        unfoldBlock(block);
    } else {
        foldBlock(block);
    }
}

void CodeEditor::foldBlock(const QTextBlock& block)
{
    if (!block.isValid()) return;

    QTextBlock end = findFoldEnd(block);
    if (!end.isValid() || end == block) return;

    m_foldedBlocks.insert(block.blockNumber());

    QTextBlock b = block.next();
    while (b.isValid() && b != end.next()) {
        b.setVisible(false);
        b = b.next();
    }

    applyFolding();
    emit foldingChanged();
}

void CodeEditor::unfoldBlock(const QTextBlock& block)
{
    if (!block.isValid()) return;

    m_foldedBlocks.remove(block.blockNumber());

    QTextBlock end = findFoldEnd(block);
    if (!end.isValid()) return;

    QTextBlock b = block.next();
    while (b.isValid() && b != end.next()) {
        b.setVisible(true);
        if (m_foldedBlocks.contains(b.blockNumber())) {
            QTextBlock childEnd = findFoldEnd(b);
            if (childEnd.isValid()) {
                QTextBlock child = b.next();
                while (child.isValid() && child != childEnd.next()) {
                    child.setVisible(false);
                    child = child.next();
                }
            }
        }
        b = b.next();
    }

    applyFolding();
    emit foldingChanged();
}

void CodeEditor::unfoldAll()
{
    QTextBlock block = document()->begin();
    while (block.isValid()) {
        block.setVisible(true);
        block = block.next();
    }
    m_foldedBlocks.clear();
    applyFolding();
    emit foldingChanged();
}

void CodeEditor::applyFolding()
{
    document()->documentLayout()->update();
    updateLineNumberAreaWidth(0);
    viewport()->update();
}

// ── Multi-cursor ──

QList<QTextCursor> CodeEditor::allCursors() const
{
    QList<QTextCursor> cursors;
    cursors.append(textCursor());
    cursors.append(m_extraCursors);
    return cursors;
}

void CodeEditor::selectNextOccurrence()
{
    QTextCursor cursor = textCursor();
    QString selected = cursor.selectedText();
    if (selected.isEmpty()) {
        // Select the current word
        cursor.select(QTextCursor::WordUnderCursor);
        if (cursor.selectedText().isEmpty()) return;
        setTextCursor(cursor);
        return;
    }

    // Find the next occurrence
    QTextDocument::FindFlags flags;
    QTextCursor found = document()->find(selected, cursor, flags);
    if (found.isNull() || found == cursor) {
        // Wrap around from the start
        QTextCursor start(document()->begin());
        found = document()->find(selected, start, flags);
        if (found.isNull() || found == cursor) return;
    }

    // Add to extra cursors
    m_extraCursors.append(found);

    // Build combined selections for display
    QList<QTextEdit::ExtraSelection> selections;
    for (const QTextCursor& ec : m_extraCursors) {
        QTextEdit::ExtraSelection sel;
        sel.format.setBackground(QColor(0x26, 0x4f, 0x78));
        sel.cursor = ec;
        selections.append(sel);
    }
    // Also highlight the primary selection
    QTextEdit::ExtraSelection primarySel;
    primarySel.format.setBackground(QColor(0x26, 0x4f, 0x78));
    primarySel.cursor = cursor;
    selections.append(primarySel);

    setExtraSelections(selections);
}

void CodeEditor::clearExtraCursors()
{
    if (m_extraCursors.isEmpty()) return;
    m_extraCursors.clear();

    // Restore normal selections
    QList<QTextEdit::ExtraSelection> selections;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection lineSel;
        QColor lineColor(0x2a, 0x2d, 0x2e);
        lineSel.format.setBackground(lineColor);
        lineSel.format.setProperty(QTextFormat::FullWidthSelection, true);
        lineSel.cursor = textCursor();
        lineSel.cursor.clearSelection();
        selections.append(lineSel);
    }
    setExtraSelections(selections);
    viewport()->update();
}

void CodeEditor::applyKeyToAllCursors(const QString& text)
{
    if (text.isEmpty()) return;

    QTextCursor primary = textCursor();

    // Sort cursors in reverse position order to avoid invalidation
    QList<QTextCursor> sorted;
    sorted.append(primary);
    sorted.append(m_extraCursors);
    std::sort(sorted.begin(), sorted.end(), [](const QTextCursor& a, const QTextCursor& b) {
        return a.position() > b.position();
    });

    QTextCursor editBlockCursor(document());
    editBlockCursor.beginEditBlock();

    for (const QTextCursor& c : sorted) {
        QTextCursor editCursor = c;
        if (editCursor.hasSelection()) {
            editCursor.insertText(text);
        } else {
            editCursor.insertText(text);
        }
    }

    editBlockCursor.endEditBlock();

    // Update primary cursor to the last position
    QTextCursor newCursor = textCursor();
    setTextCursor(newCursor);

    // Clear extra cursors since positions are now invalid
    m_extraCursors.clear();
}

void CodeEditor::applyDeleteToAllCursors()
{
    QList<QTextCursor> sorted;
    sorted.append(textCursor());
    sorted.append(m_extraCursors);
    std::sort(sorted.begin(), sorted.end(), [](const QTextCursor& a, const QTextCursor& b) {
        return a.position() > b.position();
    });

    QTextCursor editBlockCursor(document());
    editBlockCursor.beginEditBlock();

    for (const QTextCursor& c : sorted) {
        QTextCursor ec = c;
        if (ec.hasSelection()) {
            ec.removeSelectedText();
        } else {
            ec.deleteChar();
        }
    }

    editBlockCursor.endEditBlock();
    m_extraCursors.clear();
}

void CodeEditor::applyBackspaceToAllCursors()
{
    QList<QTextCursor> sorted;
    sorted.append(textCursor());
    sorted.append(m_extraCursors);
    std::sort(sorted.begin(), sorted.end(), [](const QTextCursor& a, const QTextCursor& b) {
        return a.position() > b.position();
    });

    QTextCursor editBlockCursor(document());
    editBlockCursor.beginEditBlock();

    for (const QTextCursor& c : sorted) {
        QTextCursor ec = c;
        if (ec.hasSelection()) {
            ec.removeSelectedText();
        } else if (ec.position() > 0) {
            ec.setPosition(ec.position() - 1);
            ec.setPosition(ec.position() + 1, QTextCursor::KeepAnchor);
            ec.removeSelectedText();
        }
    }

    editBlockCursor.endEditBlock();
    m_extraCursors.clear();
}

// ── Minimap ──

void CodeEditor::updateMinimap()
{
    if (m_minimap && m_minimap->isVisible()) {
        m_minimap->update();
    }
}

void CodeEditor::scrollMinimapTo(int y)
{
    qreal ratio = qreal(y) / qreal(m_minimap->height());
    int maxScroll = verticalScrollBar()->maximum();
    verticalScrollBar()->setValue(qRound(ratio * maxScroll));
}

void CodeEditor::setMinimapVisible(bool visible)
{
    if (m_minimap) {
        m_minimap->setVisible(visible);
        updateLineNumberAreaWidth(0);
        if (visible) {
            QRect cr = contentsRect();
            m_minimap->setGeometry(QRect(cr.right() - 100, cr.top(), 100, cr.height()));
        }
        viewport()->update();
    }
}

bool CodeEditor::isMinimapVisible() const
{
    return m_minimap && m_minimap->isVisible();
}

// ── Snippets ──

void CodeEditor::addCompletions(const QStringList& words)
{
    QStringList existing = m_completionModel->stringList();
    existing.append(words);
    existing.removeDuplicates();
    m_completionModel->setStringList(existing);
}

void CodeEditor::initSnippets()
{
    // C++ snippets
    m_snippets["for"] =
        "for (int $1 = 0; $1 < $2; ++$1) {\n    $3\n}";
    m_snippets["foreach"] =
        "for (const auto& $1 : $2) {\n    $3\n}";
    m_snippets["if"] =
        "if ($1) {\n    $2\n}";
    m_snippets["else"] =
        "} else {\n    $1\n}";
    m_snippets["elif"] =
        "} else if ($1) {\n    $2\n}";
    m_snippets["while"] =
        "while ($1) {\n    $2\n}";
    m_snippets["class"] =
        "class $1 {\npublic:\n    $1();\n    ~$1();\n\nprivate:\n    $2\n};";
    m_snippets["struct"] =
        "struct $1 {\n    $1() = default;\n    $2\n};";
    m_snippets["main"] =
        "int main(int argc, char* argv[]) {\n    $1\n    return 0;\n}";
    m_snippets["ifndef"] =
        "#ifndef $1\n#define $1\n\n$2\n\n#endif // $1";
    m_snippets["ns"] =
        "namespace $1 {\n\n$2\n\n} // namespace $1";
    m_snippets["fn"] =
        "void $1($2) {\n    $3\n}";

    // Lua snippets
    m_snippets["function"] =
        "function $1($2)\n    $3\nend";
    m_snippets["forl"] =
        "for $1 = $2, $3 do\n    $4\nend";
    m_snippets["ifl"] =
        "if $1 then\n    $2\nend";
    m_snippets["whilel"] =
        "while $1 do\n    $2\nend";

    // Python snippets
    m_snippets["def"] =
        "def $1($2):\n    $3";
    m_snippets["classpy"] =
        "class $1:\n    def __init__(self, $2):\n        $3";
    m_snippets["forpy"] =
        "for $1 in $2:\n    $3";
    m_snippets["ifpy"] =
        "if $1:\n    $2";
    m_snippets["withpy"] =
        "with $1 as $2:\n    $3";

    // INI snippets
    m_snippets["section"] =
        "[$1]\n$2 = $3";
    m_snippets["header"] =
        "; $1\n; Created with ksEditor\n[$2]\n$3 = $4";

    // JSON snippets
    m_snippets["jsonobj"] =
        "{\n    \"$1\": $2\n}";
    m_snippets["jsonarr"] =
        "[\n    $1\n]";

    // HTML snippets
    m_snippets["html"] =
        "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n    <meta charset=\"UTF-8\">\n"
        "    <title>$1</title>\n</head>\n<body>\n    $2\n</body>\n</html>";
    m_snippets["div"] =
        "<div>$1</div>";
}

bool CodeEditor::tryExpandSnippet(QTextCursor& cursor)
{
    // Get the word before cursor
    int pos = cursor.position();
    QTextBlock block = cursor.block();
    QString line = block.text();
    int posInBlock = cursor.positionInBlock();

    // Find start of word before cursor
    int wordStart = posInBlock;
    while (wordStart > 0 && (line[wordStart - 1].isLetterOrNumber() || line[wordStart - 1] == '_')) {
        --wordStart;
    }

    QString prefix = line.mid(wordStart, posInBlock - wordStart);
    if (prefix.isEmpty()) return false;

    if (!m_snippets.contains(prefix)) return false;

    // Remove the prefix
    cursor.setPosition(block.position() + wordStart);
    cursor.setPosition(block.position() + posInBlock, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();

    // Insert the snippet template
    QString template_ = m_snippets[prefix];

    // Simple indent: prepend current line's indent to each new line
    QString indent;
    for (const QChar& c : line) {
        if (c == ' ' || c == '\t') indent += c;
        else break;
    }
    QStringList lines = template_.split('\n');
    for (int i = 1; i < lines.size(); ++i) {
        lines[i] = indent + lines[i];
    }
    QString result = lines.join('\n');

    cursor.insertText(result);
    setTextCursor(cursor);
    return true;
}

// ── Auto-completion ──

void CodeEditor::buildWordIndex()
{
    QSet<QString> words;
    QString text = toPlainText();

    QRegularExpression wordRegex(R"(\b[A-Za-z_]\w*\b)");
    QRegularExpressionMatchIterator it = wordRegex.globalMatch(text);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString word = match.captured();
        if (word.length() >= 2) {
            words.insert(word);
        }
    }

    QStringList sorted = words.values();
    sorted.sort();
    m_completionModel->setStringList(sorted);
}

void CodeEditor::showCompletion()
{
    if (hasExtraCursors()) return;
    if (m_completer->popup()->isVisible()) return;

    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::StartOfWord, QTextCursor::MoveAnchor);
    int startPos = cursor.position();
    QString prefix = textCursor().block().text().mid(
        cursor.positionInBlock(),
        textCursor().positionInBlock() - cursor.positionInBlock());

    if (prefix.length() < 2) return;

    // Only complete at word boundaries
    if (!prefix[0].isLetter() && prefix[0] != '_') return;

    // Rebuild word index on-demand for fresh results
    buildWordIndex();

    m_completer->setCompletionPrefix(prefix);
    if (m_completer->completionCount() == 0) return;

    QRect cr = cursorRect();
    cr.setWidth(m_completer->popup()->sizeHintForColumn(0)
                + m_completer->popup()->verticalScrollBar()->sizeHint().width() + 20);
    m_completer->complete(cr);
}

void CodeEditor::insertCompletion(const QString& completion)
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::StartOfWord, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    cursor.insertText(completion);
    setTextCursor(cursor);
}

// ── Syntax highlighting ──

void CodeEditor::setSyntaxHighlighter(QSyntaxHighlighter* highlighter)
{
    if (m_highlighter) delete m_highlighter;
    m_highlighter = highlighter;
    if (m_highlighter) m_highlighter->setDocument(document());
}

// ── Line highlighting ──

void CodeEditor::highlightCurrentLine()
{
    // Don't override multi-cursor selections
    if (hasExtraCursors()) return;

    QList<QTextEdit::ExtraSelection> extraSelections;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor(0x2a, 0x2d, 0x2e);
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }
    setExtraSelections(extraSelections);
}

// ── Bracket matching ──

void CodeEditor::matchBrackets()
{
    if (hasExtraCursors()) return;

    QTextCursor cursor = textCursor();
    int pos = cursor.position();
    QTextDocument* doc = document();
    if (pos <= 0 || pos > doc->characterCount()) return;

    QChar c = doc->characterAt(pos - 1);
    QChar openChar, closeChar;
    int direction = 0;

    if (c == '(') { openChar = '('; closeChar = ')'; direction = 1; }
    else if (c == ')') { openChar = ')'; closeChar = '('; direction = -1; }
    else if (c == '{') { openChar = '{'; closeChar = '}'; direction = 1; }
    else if (c == '}') { openChar = '}'; closeChar = '{'; direction = -1; }
    else if (c == '[') { openChar = '['; closeChar = ']'; direction = 1; }
    else if (c == ']') { openChar = ']'; closeChar = '['; direction = -1; }
    else return;

    int depth = 0;
    int searchPos = pos - 1;
    while (searchPos >= 0 && searchPos < doc->characterCount()) {
        QChar ch = doc->characterAt(searchPos);
        if (ch == openChar) depth += direction;
        else if (ch == closeChar) depth -= direction;
        if (depth == 0) break;
        searchPos += direction;
    }

    QList<QTextEdit::ExtraSelection> selections = extraSelections();
    if (depth == 0 && searchPos >= 0 && searchPos < doc->characterCount()) {
        QTextEdit::ExtraSelection match;
        match.format.setForeground(QColor(0xd4, 0xd4, 0xd4));
        match.format.setBackground(QColor(0x3a, 0x3a, 0x3a));
        match.cursor = textCursor();
        match.cursor.setPosition(searchPos);
        match.cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
        selections.append(match);

        match.cursor = textCursor();
        match.cursor.setPosition(pos - 1);
        match.cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
        selections.append(match);
    }
    setExtraSelections(selections);
}

// ── Painting ──

void CodeEditor::paintEvent(QPaintEvent* event)
{
    QPlainTextEdit::paintEvent(event);

    // Draw extra cursor lines
    if (!m_extraCursors.isEmpty()) {
        QPainter painter(viewport());
        painter.setPen(QPen(QColor(0xae, 0xaf, 0xad), 2));

        for (const QTextCursor& cursor : m_extraCursors) {
            if (!cursor.isNull()) {
                QRect cr = cursorRect(cursor);
                cr.setWidth(2);
                painter.fillRect(cr, QColor(0xae, 0xaf, 0xad));
            }
        }
    }
}

// ── Keyboard ──

void CodeEditor::keyPressEvent(QKeyEvent* event)
{
    QTextCursor cursor = textCursor();
    bool multiCursor = hasExtraCursors();
    bool completionActive = m_completer->popup()->isVisible();

    // Ctrl+Space: force show completion
    if (event->key() == Qt::Key_Space && (event->modifiers() & Qt::ControlModifier)) {
        buildWordIndex();
        showCompletion();
        return;
    }

    // Completion popup handling
    if (completionActive) {
        // Esc: dismiss
        if (event->key() == Qt::Key_Escape) {
            m_completer->popup()->hide();
            return;
        }
        // Enter/Tab: accept selected completion
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter ||
            event->key() == Qt::Key_Tab) {
            QString current = m_completer->popup()->currentIndex().data().toString();
            if (!current.isEmpty()) {
                m_completer->popup()->hide();
                insertCompletion(current);
                return;
            }
        }
        // Up/Down: navigate popup
        if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down) {
            QKeyEvent forward(event->type(), event->key(), event->modifiers());
            QApplication::sendEvent(m_completer->popup(), &forward);
            return;
        }
    }

    // Ctrl+D: Select next occurrence
    if (event->key() == Qt::Key_D && (event->modifiers() & Qt::ControlModifier)) {
        selectNextOccurrence();
        return;
    }

    // Esc: Clear extra cursors or dismiss completion
    if (event->key() == Qt::Key_Escape) {
        if (hasExtraCursors()) {
            clearExtraCursors();
            return;
        }
    }

    // Mouse/arrow keys clear multi-cursor
    if (multiCursor && (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down ||
                        event->key() == Qt::Key_Left || event->key() == Qt::Key_Right ||
                        event->key() == Qt::Key_Home || event->key() == Qt::Key_End ||
                        event->key() == Qt::Key_PageUp || event->key() == Qt::Key_PageDown)) {
        clearExtraCursors();
    }

    // Multi-cursor text input: apply to all cursors
    if (multiCursor && !event->text().isEmpty()) {
        QChar ch = event->text().at(0);
        if (ch.isPrint() || ch == '\t') {
            applyKeyToAllCursors(event->text());
            return;
        }
    }

    // Multi-cursor deletion
    if (multiCursor && (event->key() == Qt::Key_Delete)) {
        applyDeleteToAllCursors();
        return;
    }
    if (multiCursor && (event->key() == Qt::Key_Backspace)) {
        applyBackspaceToAllCursors();
        return;
    }

    // Tab: snippet expansion or 4 spaces
    if (event->key() == Qt::Key_Tab && !completionActive) {
        if (!multiCursor) {
            QTextCursor c = textCursor();
            if (tryExpandSnippet(c)) {
                return;
            }
        }
        if (multiCursor) {
            applyKeyToAllCursors("    ");
        } else {
            insertPlainText("    ");
        }
        return;
    }

    // Auto-pair brackets and quotes (only for single cursor)
    if (!multiCursor && !event->text().isEmpty()) {
        QChar ch = event->text().at(0);

        if (ch == '(' || ch == '[' || ch == '{') {
            QChar close;
            if (ch == '(') close = ')';
            else if (ch == '[') close = ']';
            else if (ch == '{') close = '}';

            if (!cursor.hasSelection()) {
                int pos = cursor.position();
                QTextDocument* doc = document();
                if (pos > 0 && doc->characterAt(pos - 1).isLetterOrNumber()) {
                    QPlainTextEdit::keyPressEvent(event);
                    return;
                }
            }

            cursor.beginEditBlock();
            cursor.insertText(ch);
            cursor.insertText(close);
            cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 1);
            setTextCursor(cursor);
            cursor.endEditBlock();
            return;
        }

        if (ch == ')' || ch == ']' || ch == '}') {
            QString nextChar = cursor.block().text().mid(cursor.positionInBlock(), 1);
            if (nextChar == QString(ch)) {
                cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, 1);
                setTextCursor(cursor);
                return;
            }
        }

        if (ch == '"' || ch == '\'') {
            if (cursor.hasSelection()) {
                QString selected = cursor.selectedText();
                cursor.beginEditBlock();
                cursor.insertText(ch + selected + ch);
                cursor.endEditBlock();
                return;
            }

            int pos = cursor.position();
            QTextDocument* doc = document();
            QChar prev = (pos > 0) ? doc->characterAt(pos - 1) : QChar(' ');
            QChar next = (pos < doc->characterCount()) ? doc->characterAt(pos) : QChar(' ');

            bool prevIsWord = prev.isLetterOrNumber() || prev == '_';
            bool nextIsQuote = (next == ch);

            if (nextIsQuote) {
                cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, 1);
                setTextCursor(cursor);
                return;
            }

            if (!prevIsWord) {
                cursor.beginEditBlock();
                cursor.insertText(ch);
                cursor.insertText(ch);
                cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 1);
                setTextCursor(cursor);
                cursor.endEditBlock();
                return;
            }
        }
    }

    // Smart backspace for empty bracket pairs
    if (event->key() == Qt::Key_Backspace && !multiCursor) {
        int pos = cursor.position();
        if (pos > 0 && pos < document()->characterCount()) {
            QChar before = document()->characterAt(pos - 1);
            QChar after = document()->characterAt(pos);
            if (isAutoPair(before, after)) {
                cursor.beginEditBlock();
                cursor.deleteChar();
                cursor.deleteChar();
                setTextCursor(cursor);
                cursor.endEditBlock();
                return;
            }
        }
    }

    // Enter between braces: smart indent
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) && !multiCursor) {
        if (cursor.position() > 0 && cursor.position() < document()->characterCount()) {
            QChar before = document()->characterAt(cursor.position() - 1);
            QChar after = document()->characterAt(cursor.position());
            if ((before == '{' && after == '}') || (before == '[' && after == ']') ||
                (before == '(' && after == ')')) {
                QString indent;
                QString line = cursor.block().text();
                for (const QChar& c : line) {
                    if (c == ' ' || c == '\t') indent += c;
                    else break;
                }
                cursor.beginEditBlock();
                cursor.insertText("\n" + indent + "    ");
                cursor.insertText("\n" + indent);
                cursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor, 1);
                cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::MoveAnchor);
                setTextCursor(cursor);
                cursor.endEditBlock();
                return;
            }
        }

        // Normal enter with auto-indent
        QString indent;
        QString line = cursor.block().text();
        for (const QChar& c : line) {
            if (c == ' ' || c == '\t') indent += c;
            else break;
        }
        QPlainTextEdit::keyPressEvent(event);
        cursor = textCursor();
        cursor.insertText(indent);
        setTextCursor(cursor);
        return;
    }

    QPlainTextEdit::keyPressEvent(event);
}

void CodeEditor::mousePressEvent(QMouseEvent* event)
{
    if (hasExtraCursors()) {
        clearExtraCursors();
    }
    QPlainTextEdit::mousePressEvent(event);
}

bool CodeEditor::isAutoPair(QChar before, QChar after) const
{
    return (before == '(' && after == ')') ||
           (before == '[' && after == ']') ||
           (before == '{' && after == '}') ||
           (before == '"' && after == '"') ||
           (before == '\'' && after == '\'');
}

// ── Navigation ──

void CodeEditor::gotoLine(int line)
{
    QTextCursor cursor(document()->findBlockByNumber(qMax(0, line - 1)));
    setTextCursor(cursor);
    centerCursor();
}

// ── MinimapWidget ──

MinimapWidget::MinimapWidget(ks::CodeEditor* editor, QWidget* parent)
    : QWidget(parent), m_editor(editor)
{
    setFixedWidth(100);
    setCursor(Qt::PointingHandCursor);
}

void MinimapWidget::paintEvent(QPaintEvent*)
{
    if (!m_editor || !m_editor->document()) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    // Background
    painter.fillRect(rect(), QColor(0x22, 0x22, 0x25));

    // Draw left border
    painter.setPen(QColor(0x3a, 0x3a, 0x3a));
    painter.drawLine(0, 0, 0, height());

    QTextDocument* doc = m_editor->document();
    int totalBlocks = doc->blockCount();
    if (totalBlocks == 0) return;

    qreal mmHeight = height() - 4;
    qreal lineH = qMax(2.0, mmHeight / totalBlocks);
    m_lineHeight = lineH;

    QTextBlock block = doc->begin();
    int y = 2;

    while (block.isValid()) {
        if (block.isVisible() && y + lineH <= height() - 2) {
            QString text = block.text();
            if (!text.isEmpty()) {
                // Draw line as a tiny colored strip
                // Use a simple hash of the first word for color variety
                QColor lineColor(0x55, 0x55, 0x58);
                if (text.trimmed().startsWith("//") || text.trimmed().startsWith("#") ||
                    text.trimmed().startsWith("--") || text.trimmed().startsWith(";")) {
                    lineColor = QColor(0x3a, 0x5a, 0x3a);
                } else if (text.contains('{') || text.contains('}')) {
                    lineColor = QColor(0x4a, 0x4a, 0x6a);
                } else if (text.contains('"')) {
                    lineColor = QColor(0x5a, 0x4a, 0x3a);
                }

                painter.fillRect(2, y, width() - 4, qMax(1, (int)lineH - 1), lineColor);
            }
        }
        block = block.next();
        y += lineH;
    }

    // Viewport overlay
    QWidget* viewport = m_editor->viewport();
    qreal visibleRatio = qreal(viewport->height()) / qreal(m_editor->document()->size().height());
    qreal scrollRatio = qreal(m_editor->verticalScrollBar()->value())
                        / qreal(m_editor->verticalScrollBar()->maximum());

    int overlayHeight = qMax(20, qRound(mmHeight * qMin(1.0, visibleRatio)));
    int overlayTop = 2 + qRound((mmHeight - overlayHeight) * scrollRatio);

    painter.fillRect(2, overlayTop, width() - 4, overlayHeight, QColor(0x88, 0xaa, 0xdd, 0x30));
    painter.setPen(QPen(QColor(0x88, 0xaa, 0xdd, 0x60), 1));
    painter.drawRect(2, overlayTop, width() - 4, overlayHeight);
}

void MinimapWidget::mousePressEvent(QMouseEvent* event)
{
    if (!m_editor) return;
    m_dragging = true;
    m_editor->scrollMinimapTo(event->pos().y());
}

void MinimapWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && m_editor) {
        m_editor->scrollMinimapTo(event->pos().y());
    }
}

// ── FoldArea mouse handling ──

void FoldArea::mousePressEvent(QMouseEvent* event)
{
    if (!m_editor) return;

    int y = event->pos().y();
    QTextBlock block = m_editor->firstVisibleBlock();
    int top = qRound(m_editor->blockBoundingGeometry(block).translated(m_editor->contentOffset()).top());

    while (block.isValid()) {
        if (block.isVisible()) {
            QRectF blockRect = m_editor->blockBoundingRect(block);
            int blockTop = top;
            int blockBottom = top + qRound(blockRect.height());

            if (y >= blockTop && y < blockBottom) {
                m_editor->toggleFold(block);
                return;
            }
            top = blockBottom;
        }
        block = block.next();
    }

    QWidget::mousePressEvent(event);
}

} // namespace ks
