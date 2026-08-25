#pragma once

#include <QAbstractTableModel>
#include <QVector>
#include <QVariant>
#include <QColor>
#include <QFont>
#include <QJsonObject>
#include <QRegularExpression>

namespace ks {

struct CellStyle {
    QFont font;
    QColor textColor = Qt::black;
    QColor backgroundColor = Qt::white;
    Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignVCenter;
    bool bold = false;
    bool italic = false;
    bool underline = false;
};

struct CellData {
    QString rawValue;
    QString formula;
    QVariant computedValue;
    CellStyle style;
    bool isFormula = false;
};

enum class SpreadsheetRole {
    DisplayRole = Qt::DisplayRole,
    EditRole = Qt::EditRole,
    ForegroundRole = Qt::ForegroundRole,
    BackgroundRole = Qt::BackgroundRole,
    FontRole = Qt::FontRole,
    TextAlignmentRole = Qt::TextAlignmentRole,
    FormulaRole = Qt::UserRole,
    RawValueRole = Qt::UserRole + 1,
};

class SpreadsheetModel : public QAbstractTableModel {
    Q_OBJECT

public:
    static constexpr int DEFAULT_ROWS = 100;
    static constexpr int DEFAULT_COLS = 26;
    static constexpr int MAX_ROWS = 10000;
    static constexpr int MAX_COLS = 702; // AA-ZZ

    explicit SpreadsheetModel(QObject* parent = nullptr);
    ~SpreadsheetModel() override = default;

    // QAbstractTableModel interface
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    // Cell access
    QString cellReference(int row, int col) const;
    static QString columnLabel(int col);
    static int columnFromLabel(const QString& label);

    // Data operations
    void setCellValue(int row, int col, const QString& value);
    QString cellValue(int row, int col) const;
    QString cellFormula(int row, int col) const;
    QVariant cellDisplayValue(int row, int col) const;

    // Style operations
    void setCellFont(int row, int col, const QFont& font);
    void setCellTextColor(int row, int col, const QColor& color);
    void setCellBackground(int row, int col, const QColor& color);
    void setCellAlignment(int row, int col, Qt::Alignment alignment);
    void setCellBold(int row, int col, bool bold);
    void setCellItalic(int row, int col, bool italic);
    void setCellUnderline(int row, int col, bool underline);
    CellStyle cellStyle(int row, int col) const;

    // Bulk operations
    void clear();
    void clearContent();
    void clearStyles();
    void insertRows(int row, int count);
    void insertColumns(int col, int count);
    void removeRows(int row, int count);
    void removeColumns(int col, int count);

    // Selection data for copy/paste
    struct CellRange {
        int startRow, startCol, endRow, endCol;
    };
    QString serializeRange(const CellRange& range) const;
    void deserializeToPosition(const QString& data, int targetRow, int targetCol);

    // Formula evaluation
    void evaluateAll();
    void evaluateCell(int row, int col);

    // Serialization
    QJsonObject serialize() const;
    void deserialize(const QJsonObject& data);

    // CSV support
    bool loadCSV(const QString& filePath);
    bool saveCSV(const QString& filePath) const;

signals:
    void cellChanged(int row, int col);

private:
    bool isValidIndex(int row, int col) const;
    QVariant evaluateFormula(const QString& formula, int sourceRow, int sourceCol) const;
    QVariant evaluateFunction(const QString& funcName, const QString& args, int sourceRow, int sourceCol) const;
    CellRange parseCellRange(const QString& rangeStr) const;
    bool dependsOnCell(const QString& formula, int row, int col) const;

    int m_rowCount;
    int m_colCount;
    QVector<QVector<CellData>> m_cells;
};

} // namespace ks
