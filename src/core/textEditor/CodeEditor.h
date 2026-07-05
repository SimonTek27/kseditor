#pragma once

#include <QPlainTextEdit>
#include <QWidget>
#include <QSyntaxHighlighter>
#include <QSet>
#include <QList>
#include <QTextCursor>
#include <QCompleter>
#include <QStringListModel>
#include <QTimer>

namespace ks {
class MinimapWidget;

class LineNumberArea;
class FoldArea;

class CodeEditor : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit CodeEditor(QWidget* parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent* event);
    int lineNumberAreaWidth() const;
    int currentLineNumber() const { return textCursor().blockNumber() + 1; }

    void foldAreaPaintEvent(QPaintEvent* event);
    int foldAreaWidth() const;

    void setSyntaxHighlighter(QSyntaxHighlighter* highlighter);
    void gotoLine(int line);

    // Folding
    bool isBlockFolded(const QTextBlock& block) const;
    void toggleFold(const QTextBlock& block);
    void foldBlock(const QTextBlock& block);
    void unfoldBlock(const QTextBlock& block);
    void unfoldAll();
    void recalculateFoldLevels();

    // Multi-cursor
    void selectNextOccurrence();
    void clearExtraCursors();
    bool hasExtraCursors() const { return !m_extraCursors.isEmpty(); }
    QList<QTextCursor> extraCursors() const { return m_extraCursors; }

    // Selection helpers for multi-cursor
    QList<QTextCursor> allCursors() const;

    void addCompletions(const QStringList& words);

signals:
    void foldingChanged();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect& rect, int dy);
    void updateFoldArea(const QRect& rect, int dy);
    void highlightCurrentLine();
    void matchBrackets();

private:
    int computeFoldLevel(const QTextBlock& block) const;
    QTextBlock findFoldEnd(const QTextBlock& startBlock) const;
    void applyFolding();
    bool isAutoPair(QChar before, QChar after) const;
    void applyKeyToAllCursors(const QString& text);
    void applyDeleteToAllCursors();
    void applyBackspaceToAllCursors();

    QWidget* m_lineNumberArea;
    QWidget* m_foldArea;
    MinimapWidget* m_minimap;
    QSyntaxHighlighter* m_highlighter = nullptr;

    // Per-block fold deltas (how much this block changes nesting)
    QMap<int, int> m_foldDeltas;
    // Per-block cumulative level after processing the block
    QMap<int, int> m_foldLevels;
    // Block numbers that are currently folded (block number of the fold start)
    QSet<int> m_foldedBlocks;

    // Minimap
    void updateMinimap();
public:
    void scrollMinimapTo(int y);
    void setMinimapVisible(bool visible);
    bool isMinimapVisible() const;
    QTextBlock firstVisibleBlock() const { return QPlainTextEdit::firstVisibleBlock(); }
    QRectF blockBoundingGeometry(const QTextBlock& block) const { return QPlainTextEdit::blockBoundingGeometry(block); }
    QPointF contentOffset() const { return QPlainTextEdit::contentOffset(); }
    QRectF blockBoundingRect(const QTextBlock& block) const { return QPlainTextEdit::blockBoundingRect(block); }

private:

    // Auto-completion
    void buildWordIndex();
    void showCompletion();
    void insertCompletion(const QString& completion);

    // Snippets
    void initSnippets();
    bool tryExpandSnippet(QTextCursor& cursor);
    QMap<QString, QString> m_snippets;

    // Multi-cursor state
    QList<QTextCursor> m_extraCursors;

    QCompleter* m_completer;
    QStringListModel* m_completionModel;
    QTimer* m_completionTimer;
};

class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(CodeEditor* editor) : QWidget(editor), m_editor(editor) {}
    QSize sizeHint() const override { return QSize(m_editor->lineNumberAreaWidth(), 0); }
protected:
    void paintEvent(QPaintEvent* event) override { m_editor->lineNumberAreaPaintEvent(event); }
private:
    CodeEditor* m_editor;
};

class FoldArea : public QWidget {
public:
    explicit FoldArea(CodeEditor* editor) : QWidget(editor), m_editor(editor) {}
    QSize sizeHint() const override { return QSize(m_editor->foldAreaWidth(), 0); }
protected:
    void paintEvent(QPaintEvent* event) override { m_editor->foldAreaPaintEvent(event); }
    void mousePressEvent(QMouseEvent* event) override;
private:
    CodeEditor* m_editor;
};

class MinimapWidget : public QWidget {
    Q_OBJECT
public:
    explicit MinimapWidget(ks::CodeEditor* editor, QWidget* parent = nullptr);
    QSize sizeHint() const override { return QSize(100, 100); }

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    ks::CodeEditor* m_editor;
    qreal m_lineHeight = 3.0;
    bool m_dragging = false;
};

} // namespace ks
