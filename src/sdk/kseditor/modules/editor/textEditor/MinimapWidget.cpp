#include "CodeEditor.h"
#include <QPainter>
#include <QMouseEvent>
#include <QTextBlock>
#include <QAbstractScrollArea>
#include <QScrollBar>

namespace ks {

void FoldArea::mousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    if (m_editor) {
        int y = event->pos().y();
        QTextBlock block = m_editor->firstVisibleBlock();
        int blockNumber = block.blockNumber();
        int top = qRound(m_editor->blockBoundingGeometry(block).translated(m_editor->contentOffset()).top());
        int bottom = top + qRound(m_editor->blockBoundingRect(block).height());

        while (block.isValid() && top <= event->pos().y()) {
            if (y >= top && y < bottom) {
                if (m_editor->isBlockFolded(block)) {
                    m_editor->unfoldBlock(block);
                } else if (block.text().trimmed().endsWith("{") || block.text().trimmed().endsWith(":")) {
                    m_editor->foldBlock(block);
                }
                break;
            }
            block = block.next();
            top = bottom;
            bottom = top + qRound(m_editor->blockBoundingRect(block).height());
            blockNumber++;
        }
    }
}

MinimapWidget::MinimapWidget(ks::CodeEditor* editor, QWidget* parent)
    : QWidget(parent)
    , m_editor(editor)
{
    setMouseTracking(true);
    setFixedWidth(100);
    setMinimumHeight(100);
}

void MinimapWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    if (!m_editor) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    // Background
    painter.fillRect(rect(), QColor(35, 35, 45));

    // Draw viewport indicator
    QAbstractScrollArea* scrollArea = qobject_cast<QAbstractScrollArea*>(m_editor->parent());
    if (scrollArea) {
        int viewportHeight = m_editor->height();
        int documentHeight = m_editor->document()->size().height();
        if (documentHeight > 0) {
            float scale = static_cast<float>(height()) / documentHeight;
            int vbarValue = m_editor->verticalScrollBar()->value();
            int viewportTop = static_cast<int>(vbarValue * scale);
            int viewportH = static_cast<int>(viewportHeight * scale);

            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(60, 60, 80, 100));
            painter.drawRect(0, viewportTop, width(), viewportH);
        }
    }

    // Draw text lines as thin colored bars
    QTextBlock block = m_editor->firstVisibleBlock();
    int blockNumber = block.blockNumber();
    qreal top = m_editor->blockBoundingGeometry(block).translated(m_editor->contentOffset()).top();
    qreal bottom = top + m_editor->blockBoundingRect(block).height();

    float lineScale = static_cast<float>(height()) / m_editor->document()->size().height();

    while (block.isValid() && top < height()) {
        int scaledTop = static_cast<int>(top * lineScale);
        int scaledBottom = static_cast<int>(bottom * lineScale);
        int lineHeight = qMax(1, scaledBottom - scaledTop);

        // Determine line color based on content
        QString text = block.text().trimmed();
        QColor lineColor(100, 100, 120);

        if (text.isEmpty()) {
            block = block.next();
            top = bottom;
            bottom = top + m_editor->blockBoundingRect(block).height();
            blockNumber++;
            continue;
        }

        if (text.startsWith("//") || text.startsWith("#") || text.startsWith("--")) {
            lineColor = QColor(80, 80, 100); // Comment
        } else if (text.startsWith("function") || text.startsWith("def ") || text.startsWith("void ") ||
                   text.startsWith("int ") || text.startsWith("float ") || text.startsWith("class ") ||
                   text.startsWith("struct ") || text.startsWith("namespace ") || text.startsWith("import ") ||
                   text.startsWith("using ")) {
            lineColor = QColor(120, 160, 200); // Declaration
        } else if (text.contains('{') || text.contains('}')) {
            lineColor = QColor(140, 130, 160); // Braces
        } else if (text.startsWith("if") || text.startsWith("else") || text.startsWith("for") ||
                   text.startsWith("while") || text.startsWith("switch") || text.startsWith("return")) {
            lineColor = QColor(160, 130, 100); // Control flow
        }

        int lineWidth = qMin(static_cast<int>(text.length() * 1.5), width() - 4);
        painter.setPen(Qt::NoPen);
        painter.setBrush(lineColor);
        painter.drawRect(2, scaledTop, qMax(2, lineWidth), lineHeight);

        block = block.next();
        top = bottom;
        bottom = top + m_editor->blockBoundingRect(block).height();
        blockNumber++;
    }
}

void MinimapWidget::mousePressEvent(QMouseEvent* event)
{
    m_dragging = true;
    scrollToPosition(event->pos().y());
}

void MinimapWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging) {
        scrollToPosition(event->pos().y());
    }
}

void MinimapWidget::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    m_dragging = false;
}

void MinimapWidget::scrollToPosition(int mouseY)
{
    if (!m_editor) return;

    float lineScale = static_cast<float>(m_editor->document()->size().height()) / height();
    int targetY = static_cast<int>(mouseY * lineScale);

    // Find the block at this position
    QTextBlock block = m_editor->document()->begin();
    while (block.isValid()) {
        QRectF blockRect = m_editor->blockBoundingGeometry(block).translated(m_editor->contentOffset());
        if (blockRect.top() <= targetY && blockRect.bottom() >= targetY) {
            QTextCursor cursor(block);
            cursor.setPosition(block.position());
            m_editor->setTextCursor(cursor);
            m_editor->centerCursor();
            break;
        }
        block = block.next();
    }
}

} // namespace ks