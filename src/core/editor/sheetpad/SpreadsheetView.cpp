#include "SpreadsheetView.h"
#include <QApplication>
#include <QClipboard>
#include <QHeaderView>
#include <QMimeData>
#include <QScrollBar>

namespace ks {

SpreadsheetView::SpreadsheetView(QWidget* parent)
    : QTableView(parent)
{
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectItems);


    // Spreadsheet behavior
    horizontalHeader()->setStretchLastSection(true);
    verticalHeader()->setDefaultSectionSize(24);
    horizontalHeader()->setDefaultSectionSize(80);

    // Enable editing on click
    setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::AnyKeyPressed | QAbstractItemView::SelectedClicked);

    connect(this, &QTableView::doubleClicked, this, &SpreadsheetView::onCellDoubleClicked);
    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &SpreadsheetView::onSelectionChanged);
}

void SpreadsheetView::setSpreadsheetModel(SpreadsheetModel* model)
{
    m_spreadsheetModel = model;
    setModel(model);
}

void SpreadsheetView::copyToClipboard()
{
    if (!m_spreadsheetModel) return;

    QModelIndexList selected = selectionModel()->selectedIndexes();
    if (selected.isEmpty()) return;

    // Find bounds of selection
    int minRow = INT_MAX, maxRow = INT_MIN, minCol = INT_MAX, maxCol = INT_MIN;
    for (const QModelIndex& idx : selected) {
        minRow = qMin(minRow, idx.row());
        maxRow = qMax(maxRow, idx.row());
        minCol = qMin(minCol, idx.column());
        maxCol = qMax(maxCol, idx.column());
    }

    SpreadsheetModel::CellRange range{minRow, minCol, maxRow, maxCol};
    QString data = m_spreadsheetModel->serializeRange(range);

    QMimeData* mimeData = new QMimeData();
    mimeData->setText(data);
    QApplication::clipboard()->setMimeData(mimeData);
}

void SpreadsheetView::cutToClipboard()
{
    copyToClipboard();

    if (!m_spreadsheetModel) return;

    QModelIndexList selected = selectionModel()->selectedIndexes();
    for (const QModelIndex& idx : selected) {
        m_spreadsheetModel->setData(idx, "", Qt::EditRole);
    }
}

void SpreadsheetView::pasteFromClipboard()
{
    if (!m_spreadsheetModel) return;

    const QMimeData* mimeData = QApplication::clipboard()->mimeData();
    if (!mimeData || !mimeData->hasText()) return;

    QString data = mimeData->text();
    QModelIndex current = currentIndex();
    if (!current.isValid()) return;

    m_spreadsheetModel->deserializeToPosition(data, current.row(), current.column());
}

QString SpreadsheetView::currentCellReference() const
{
    QModelIndex idx = currentIndex();
    if (!idx.isValid() || !m_spreadsheetModel) return QString();
    return m_spreadsheetModel->cellReference(idx.row(), idx.column());
}

SpreadsheetModel::CellRange SpreadsheetView::selectionRange() const
{
    QModelIndexList selected = selectionModel()->selectedIndexes();
    if (selected.isEmpty()) {
        QModelIndex idx = currentIndex();
        if (idx.isValid())
            return {idx.row(), idx.column(), idx.row(), idx.column()};
        return {-1, -1, -1, -1};
    }

    int minRow = INT_MAX, maxRow = INT_MIN, minCol = INT_MAX, maxCol = INT_MIN;
    for (const QModelIndex& idx : selected) {
        minRow = qMin(minRow, idx.row());
        maxRow = qMax(maxRow, idx.row());
        minCol = qMin(minCol, idx.column());
        maxCol = qMax(maxCol, idx.column());
    }
    return {minRow, minCol, maxRow, maxCol};
}

void SpreadsheetView::keyPressEvent(QKeyEvent* event)
{
    // Handle Tab navigation (Tab moves right, Shift+Tab moves left)
    if (event->key() == Qt::Key_Tab) {
        QModelIndex current = currentIndex();
        if (current.isValid()) {
            int nextCol = current.column() + 1;
            int nextRow = current.row();
            if (nextCol >= model()->columnCount()) {
                nextCol = 0;
                nextRow = qMin(nextRow + 1, model()->rowCount() - 1);
            }
            QModelIndex next = model()->index(nextRow, nextCol);
            setCurrentIndex(next);
            selectionModel()->select(next, QItemSelectionModel::ClearAndSelect);
        }
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Backtab) {
        QModelIndex current = currentIndex();
        if (current.isValid()) {
            int prevCol = current.column() - 1;
            int prevRow = current.row();
            if (prevCol < 0) {
                prevCol = model()->columnCount() - 1;
                prevRow = qMax(prevRow - 1, 0);
            }
            QModelIndex prev = model()->index(prevRow, prevCol);
            setCurrentIndex(prev);
            selectionModel()->select(prev, QItemSelectionModel::ClearAndSelect);
        }
        event->accept();
        return;
    }

    // Enter: move down
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        QModelIndex current = currentIndex();
        if (state() == QAbstractItemView::EditingState) {
            // Confirm edit and move down
            QModelIndex next = model()->index(qMin(current.row() + 1, model()->rowCount() - 1), current.column());
            setCurrentIndex(next);
            selectionModel()->select(next, QItemSelectionModel::ClearAndSelect);
        } else {
            // Start editing
            edit(current);
        }
        event->accept();
        return;
    }

    // Arrow keys: navigate (exit edit mode if editing)
    if (event->key() >= Qt::Key_Left && event->key() <= Qt::Key_Down) {
        if (state() == QAbstractItemView::EditingState) {
            // Finish editing first
            QModelIndex current = currentIndex();
            if (current.isValid()) {
                emit cellEditingFinished(current.row(), current.column());
            }
        }
    }

    // Ctrl+C, Ctrl+X, Ctrl+V
    if (event->modifiers() & Qt::ControlModifier) {
        switch (event->key()) {
        case Qt::Key_C:
            copyToClipboard();
            event->accept();
            return;
        case Qt::Key_X:
            cutToClipboard();
            event->accept();
            return;
        case Qt::Key_V:
            pasteFromClipboard();
            event->accept();
            return;
        case Qt::Key_B:
            // Bold toggle handled by SheetPad
            event->accept();
            return;
        default:
            break;
        }
    }

    // Delete: clear selected cells
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        QModelIndexList selected = selectionModel()->selectedIndexes();
        for (const QModelIndex& idx : selected) {
            m_spreadsheetModel->setData(idx, "", Qt::EditRole);
        }
        event->accept();
        return;
    }

    // F2: start editing
    if (event->key() == Qt::Key_F2) {
        startEditingCurrentCell();
        event->accept();
        return;
    }

    QTableView::keyPressEvent(event);
}

void SpreadsheetView::mousePressEvent(QMouseEvent* event)
{
    QTableView::mousePressEvent(event);

    if (event->button() == Qt::LeftButton) {
        QModelIndex idx = indexAt(event->pos());
        if (idx.isValid()) {
            emit cellSelected(idx.row(), idx.column());
        }
    }
}

void SpreadsheetView::currentChanged(const QModelIndex& current, const QModelIndex& previous)
{
    QTableView::currentChanged(current, previous);

    if (previous.isValid()) {
        emit cellEditingFinished(previous.row(), previous.column());
    }

    if (current.isValid()) {
        emit currentCellChanged(current.row(), current.column());
    }
}

void SpreadsheetView::onSelectionChanged()
{
    QModelIndex current = currentIndex();
    if (current.isValid()) {
        emit cellSelected(current.row(), current.column());
    }
}

void SpreadsheetView::onCellDoubleClicked(const QModelIndex& index)
{
    if (index.isValid()) {
        emit cellEditingStarted(index.row(), index.column());
    }
}

void SpreadsheetView::navigateToCell(int row, int col)
{
    QModelIndex idx = model()->index(row, col);
    if (idx.isValid()) {
        setCurrentIndex(idx);
        selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect);
        scrollTo(idx);
    }
}

void SpreadsheetView::startEditingCurrentCell()
{
    QModelIndex current = currentIndex();
    if (current.isValid()) {
        edit(current);
        emit cellEditingStarted(current.row(), current.column());
    }
}

bool SpreadsheetView::isEditing() const
{
    return state() == QAbstractItemView::EditingState;
}

} // namespace ks
