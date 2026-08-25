#pragma once

#include <QTableView>
#include <QKeyEvent>
#include <QMouseEvent>

namespace ks {

class SpreadsheetModel;

class SpreadsheetView : public QTableView {
    Q_OBJECT

public:
    explicit SpreadsheetView(QWidget* parent = nullptr);
    ~SpreadsheetView() override = default;

    void setSpreadsheetModel(SpreadsheetModel* model);

    // Clipboard operations
    void copyToClipboard();
    void cutToClipboard();
    void pasteFromClipboard();

    // Selection helpers
    QString currentCellReference() const;
    SpreadsheetModel::CellRange selectionRange() const;

signals:
    void cellSelected(int row, int col);
    void cellEditingStarted(int row, int col);
    void cellEditingFinished(int row, int col);
    void currentCellChanged(int row, int col);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void currentChanged(const QModelIndex& current, const QModelIndex& previous) override;

private slots:
    void onSelectionChanged();
    void onCellDoubleClicked(const QModelIndex& index);

private:
    void navigateToCell(int row, int col);
    void startEditingCurrentCell();
    bool isEditing() const;

    SpreadsheetModel* m_spreadsheetModel = nullptr;
};

} // namespace ks
