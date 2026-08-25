#include "SpreadsheetModel.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>
#include <algorithm>
#include <limits>

namespace ks {

SpreadsheetModel::SpreadsheetModel(QObject* parent)
    : QAbstractTableModel(parent)
    , m_rowCount(DEFAULT_ROWS)
    , m_colCount(DEFAULT_COLS)
    , m_cells(DEFAULT_ROWS, QVector<CellData>(DEFAULT_COLS))
{
}

int SpreadsheetModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return m_rowCount;
}

int SpreadsheetModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return m_colCount;
}

QVariant SpreadsheetModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || !isValidIndex(index.row(), index.column()))
        return QVariant();

    const CellData& cell = m_cells[index.row()][index.column()];

    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        if (cell.isFormula)
            return cell.computedValue.isValid() ? cell.computedValue.toString() : cell.rawValue;
        return cell.rawValue;

    case Qt::ForegroundRole:
        return cell.style.textColor;

    case Qt::BackgroundRole:
        return cell.style.backgroundColor;

    case Qt::FontRole:
        return cell.style.font;

    case Qt::TextAlignmentRole:
        return static_cast<int>(cell.style.alignment);

    case static_cast<int>(SpreadsheetRole::FormulaRole):
        return cell.formula;

    case static_cast<int>(SpreadsheetRole::RawValueRole):
        return cell.rawValue;

    default:
        return QVariant();
    }
}

bool SpreadsheetModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || !isValidIndex(index.row(), index.column()))
        return false;

    if (role != Qt::EditRole && role != Qt::DisplayRole)
        return false;

    CellData& cell = m_cells[index.row()][index.column()];
    QString newValue = value.toString();

    if (cell.rawValue == newValue)
        return false;

    cell.rawValue = newValue;
    cell.isFormula = newValue.startsWith('=');
    cell.formula = cell.isFormula ? newValue.mid(1) : QString();

    if (cell.isFormula) {
        evaluateCell(index.row(), index.column());
    } else {
        // Try to interpret as number
        bool ok;
        double numVal = newValue.toDouble(&ok);
        cell.computedValue = ok ? numVal : QVariant(newValue);
    }

    emit dataChanged(index, index, {role});
    emit cellChanged(index.row(), index.column());
    return true;
}

QVariant SpreadsheetModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        return columnLabel(section);
    } else {
        return section + 1;
    }
}

Qt::ItemFlags SpreadsheetModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QString SpreadsheetModel::cellReference(int row, int col) const
{
    return columnLabel(col) + QString::number(row + 1);
}

QString SpreadsheetModel::columnLabel(int col)
{
    if (col < 0) return QString();

    QString label;
    int remaining = col;
    while (remaining >= 0) {
        label.prepend(QChar('A' + remaining % 26));
        remaining = remaining / 26 - 1;
    }
    return label;
}

int SpreadsheetModel::columnFromLabel(const QString& label)
{
    int col = 0;
    for (int i = 0; i < label.size(); ++i) {
        col = col * 26 + (label[i].toUpper().unicode() - 'A' + 1);
    }
    return col - 1;
}

void SpreadsheetModel::setCellValue(int row, int col, const QString& value)
{
    if (!isValidIndex(row, col)) return;
    QModelIndex idx = index(row, col);
    setData(idx, value, Qt::EditRole);
}

QString SpreadsheetModel::cellValue(int row, int col) const
{
    if (!isValidIndex(row, col)) return QString();
    return m_cells[row][col].rawValue;
}

QString SpreadsheetModel::cellFormula(int row, int col) const
{
    if (!isValidIndex(row, col)) return QString();
    return m_cells[row][col].formula;
}

QVariant SpreadsheetModel::cellDisplayValue(int row, int col) const
{
    if (!isValidIndex(row, col)) return QVariant();
    const CellData& cell = m_cells[row][col];
    if (cell.isFormula)
        return cell.computedValue;
    return cell.computedValue;
}

void SpreadsheetModel::setCellFont(int row, int col, const QFont& font)
{
    if (!isValidIndex(row, col)) return;
    m_cells[row][col].style.font = font;
    emit dataChanged(index(row, col), index(row, col), {Qt::FontRole});
}

void SpreadsheetModel::setCellTextColor(int row, int col, const QColor& color)
{
    if (!isValidIndex(row, col)) return;
    m_cells[row][col].style.textColor = color;
    emit dataChanged(index(row, col), index(row, col), {Qt::ForegroundRole});
}

void SpreadsheetModel::setCellBackground(int row, int col, const QColor& color)
{
    if (!isValidIndex(row, col)) return;
    m_cells[row][col].style.backgroundColor = color;
    emit dataChanged(index(row, col), index(row, col), {Qt::BackgroundRole});
}

void SpreadsheetModel::setCellAlignment(int row, int col, Qt::Alignment alignment)
{
    if (!isValidIndex(row, col)) return;
    m_cells[row][col].style.alignment = alignment;
    emit dataChanged(index(row, col), index(row, col), {Qt::TextAlignmentRole});
}

void SpreadsheetModel::setCellBold(int row, int col, bool bold)
{
    if (!isValidIndex(row, col)) return;
    m_cells[row][col].style.bold = bold;
    QFont f = m_cells[row][col].style.font;
    f.setBold(bold);
    m_cells[row][col].style.font = f;
    emit dataChanged(index(row, col), index(row, col), {Qt::FontRole});
}

void SpreadsheetModel::setCellItalic(int row, int col, bool italic)
{
    if (!isValidIndex(row, col)) return;
    m_cells[row][col].style.italic = italic;
    QFont f = m_cells[row][col].style.font;
    f.setItalic(italic);
    m_cells[row][col].style.font = f;
    emit dataChanged(index(row, col), index(row, col), {Qt::FontRole});
}

void SpreadsheetModel::setCellUnderline(int row, int col, bool underline)
{
    if (!isValidIndex(row, col)) return;
    m_cells[row][col].style.underline = underline;
    QFont f = m_cells[row][col].style.font;
    f.setUnderline(underline);
    m_cells[row][col].style.font = f;
    emit dataChanged(index(row, col), index(row, col), {Qt::FontRole});
}

CellStyle SpreadsheetModel::cellStyle(int row, int col) const
{
    if (!isValidIndex(row, col)) return CellStyle();
    return m_cells[row][col].style;
}

void SpreadsheetModel::clear()
{
    beginResetModel();
    m_cells.clear();
    m_cells.resize(m_rowCount);
    for (auto& row : m_cells) {
        row.resize(m_colCount);
        row.fill(CellData(), m_colCount);
    }
    endResetModel();
}

void SpreadsheetModel::clearContent()
{
    for (int r = 0; r < m_rowCount; ++r) {
        for (int c = 0; c < m_colCount; ++c) {
            m_cells[r][c].rawValue.clear();
            m_cells[r][c].formula.clear();
            m_cells[r][c].computedValue.clear();
            m_cells[r][c].isFormula = false;
        }
    }
    emit dataChanged(index(0, 0), index(m_rowCount - 1, m_colCount - 1));
}

void SpreadsheetModel::clearStyles()
{
    for (int r = 0; r < m_rowCount; ++r) {
        for (int c = 0; c < m_colCount; ++c) {
            m_cells[r][c].style = CellStyle();
        }
    }
    emit dataChanged(index(0, 0), index(m_rowCount - 1, m_colCount - 1));
}

void SpreadsheetModel::insertRows(int row, int count)
{
    if (row < 0 || row > m_rowCount || count <= 0) return;
    if (m_rowCount + count > MAX_ROWS) return;

    beginInsertRows(QModelIndex(), row, row + count - 1);
    m_rowCount += count;
    for (int i = 0; i < count; ++i) {
        m_cells.insert(row, QVector<CellData>(m_colCount));
    }
    endInsertRows();
}

void SpreadsheetModel::insertColumns(int col, int count)
{
    if (col < 0 || col > m_colCount || count <= 0) return;
    if (m_colCount + count > MAX_COLS) return;

    beginInsertColumns(QModelIndex(), col, col + count - 1);
    m_colCount += count;
    for (int r = 0; r < m_rowCount; ++r) {
        for (int i = 0; i < count; ++i) {
            m_cells[r].insert(col, CellData());
        }
    }
    endInsertColumns();
}

void SpreadsheetModel::removeRows(int row, int count)
{
    if (row < 0 || row + count > m_rowCount || count <= 0) return;

    beginRemoveRows(QModelIndex(), row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        m_cells.removeAt(row);
    }
    m_rowCount -= count;
    endRemoveRows();
}

void SpreadsheetModel::removeColumns(int col, int count)
{
    if (col < 0 || col + count > m_colCount || count <= 0) return;

    beginRemoveColumns(QModelIndex(), col, col + count - 1);
    for (int r = 0; r < m_rowCount; ++r) {
        for (int i = 0; i < count; ++i) {
            m_cells[r].removeAt(col);
        }
    }
    m_colCount -= count;
    endRemoveColumns();
}

QString SpreadsheetModel::serializeRange(const CellRange& range) const
{
    QStringList rows;
    for (int r = range.startRow; r <= range.endRow; ++r) {
        QStringList cells;
        for (int c = range.startCol; c <= range.endCol; ++c) {
            cells.append(cellValue(r, c));
        }
        rows.append(cells.join('\t'));
    }
    return rows.join('\n');
}

void SpreadsheetModel::deserializeToPosition(const QString& data, int targetRow, int targetCol)
{
    QStringList rows = data.split('\n', Qt::SkipEmptyParts);
    for (int r = 0; r < rows.size(); ++r) {
        QStringList cells = rows[r].split('\t');
        for (int c = 0; c < cells.size(); ++c) {
            int targetR = targetRow + r;
            int targetC = targetCol + c;
            if (targetR < m_rowCount && targetC < m_colCount) {
                setCellValue(targetR, targetC, cells[c]);
            }
        }
    }
}

void SpreadsheetModel::evaluateAll()
{
    for (int r = 0; r < m_rowCount; ++r) {
        for (int c = 0; c < m_colCount; ++c) {
            if (m_cells[r][c].isFormula) {
                evaluateCell(r, c);
            }
        }
    }
}

void SpreadsheetModel::evaluateCell(int row, int col)
{
    if (!isValidIndex(row, col)) return;
    CellData& cell = m_cells[row][col];
    if (!cell.isFormula) return;

    cell.computedValue = evaluateFormula(cell.formula, row, col);
}

QVariant SpreadsheetModel::evaluateFormula(const QString& formula, int sourceRow, int sourceCol) const
{
    // Simple formula evaluation supporting:
    // =A1, =A1+B2, =SUM(A1:B3), =AVG(A1:B3), =MIN(A1:B3), =MAX(A1:B3)
    // =A1*2, =A1+B1*3, basic arithmetic

    QString expr = formula.trimmed().toUpper();

    // Check for function calls: FUNC(range)
    static QRegularExpression funcRegex(R"((SUM|AVG|AVERAGE|MIN|MAX|COUNT|COUNTA|IF)\(([A-Z]+\d+):([A-Z]+\d+)\))");
    QRegularExpressionMatch funcMatch = funcRegex.match(expr);
    if (funcMatch.hasMatch()) {
        QString funcName = funcMatch.captured(1);
        QString rangeStr = funcMatch.captured(2) + ":" + funcMatch.captured(3);
        return evaluateFunction(funcName, rangeStr, sourceRow, sourceCol);
    }

    // Simple cell reference
    static QRegularExpression cellRef(R"(^([A-Z]+)(\d+)$)");
    QRegularExpressionMatch cellMatch = cellRef.match(expr);
    if (cellMatch.hasMatch()) {
        int col = columnFromLabel(cellMatch.captured(1));
        int row = cellMatch.captured(2).toInt() - 1;
        if (isValidIndex(row, col)) {
            const CellData& refCell = m_cells[row][col];
            if (refCell.isFormula) {
                // Recursively evaluate (with cycle detection via simple depth limit)
                return refCell.computedValue;
            }
            bool ok;
            double val = refCell.rawValue.toDouble(&ok);
            return ok ? val : QVariant(refCell.rawValue);
        }
        return QVariant();
    }

    // Simple arithmetic: A1+B1, A1*2, etc.
    static QRegularExpression arithRegex(R"(([A-Z]+\d+)([\+\-\*/])([A-Z]+\d+|\d+\.?\d*))");
    QRegularExpressionMatch arithMatch = arithRegex.match(expr);
    if (arithMatch.hasMatch()) {
        auto resolveRef = [&](const QString& ref) -> double {
            QRegularExpressionMatch m = cellRef.match(ref);
            if (m.hasMatch()) {
                int c = columnFromLabel(m.captured(1));
                int r = m.captured(2).toInt() - 1;
                if (isValidIndex(r, c)) {
                    bool ok;
                    double v = m_cells[r][c].computedValue.toDouble(&ok);
                    if (ok) return v;
                    v = m_cells[r][c].rawValue.toDouble(&ok);
                    if (ok) return v;
                }
            } else {
                bool ok;
                double v = ref.toDouble(&ok);
                if (ok) return v;
            }
            return 0.0;
        };

        double left = resolveRef(arithMatch.captured(1));
        QChar op = arithMatch.captured(2)[0];
        double right = resolveRef(arithMatch.captured(3));

        switch (op.unicode()) {
        case '+': return left + right;
        case '-': return left - right;
        case '*': return left * right;
        case '/': return right != 0.0 ? left / right : QVariant("#DIV/0!");
        }
    }

    // Try as plain number
    bool ok;
    double numVal = formula.toDouble(&ok);
    if (ok) return numVal;

    return QVariant(formula);
}

QVariant SpreadsheetModel::evaluateFunction(const QString& funcName, const QString& args, int sourceRow, int sourceCol) const
{
    Q_UNUSED(sourceRow);
    Q_UNUSED(sourceCol);

    CellRange range = parseCellRange(args);
    if (range.startRow < 0) return QVariant();

    QVector<double> values;
    int count = 0;

    for (int r = range.startRow; r <= range.endRow && r < m_rowCount; ++r) {
        for (int c = range.startCol; c <= range.endCol && c < m_colCount; ++c) {
            ++count;
            bool ok;
            double val = m_cells[r][c].computedValue.toDouble(&ok);
            if (ok) {
                values.append(val);
            } else {
                val = m_cells[r][c].rawValue.toDouble(&ok);
                if (ok) values.append(val);
            }
        }
    }

    if (funcName == "SUM") {
        double sum = 0;
        for (double v : values) sum += v;
        return sum;
    } else if (funcName == "AVG" || funcName == "AVERAGE") {
        if (values.isEmpty()) return 0.0;
        double sum = 0;
        for (double v : values) sum += v;
        return sum / values.size();
    } else if (funcName == "MIN") {
        if (values.isEmpty()) return 0.0;
        return *std::min_element(values.begin(), values.end());
    } else if (funcName == "MAX") {
        if (values.isEmpty()) return 0.0;
        return *std::max_element(values.begin(), values.end());
    } else if (funcName == "COUNT" || funcName == "COUNTA") {
        return count;
    }

    return QVariant();
}

SpreadsheetModel::CellRange SpreadsheetModel::parseCellRange(const QString& rangeStr) const
{
    // Parse "A1:B3" format
    static QRegularExpression rangeRegex(R"(([A-Z]+)(\d+):([A-Z]+)(\d+))");
    QRegularExpressionMatch match = rangeRegex.match(rangeStr.trimmed().toUpper());
    if (!match.hasMatch()) return {-1, -1, -1, -1};

    CellRange range;
    range.startCol = columnFromLabel(match.captured(1));
    range.startRow = match.captured(2).toInt() - 1;
    range.endCol = columnFromLabel(match.captured(3));
    range.endRow = match.captured(4).toInt() - 1;
    return range;
}

bool SpreadsheetModel::dependsOnCell(const QString& formula, int row, int col) const
{
    QString cellRef = columnLabel(col) + QString::number(row + 1);
    return formula.contains(cellRef, Qt::CaseInsensitive);
}

QJsonObject SpreadsheetModel::serialize() const
{
    QJsonObject root;
    root["rowCount"] = m_rowCount;
    root["colCount"] = m_colCount;

    QJsonArray cellsArray;
    for (int r = 0; r < m_rowCount; ++r) {
        for (int c = 0; c < m_colCount; ++c) {
            const CellData& cell = m_cells[r][c];
            if (!cell.rawValue.isEmpty() || cell.style.bold || cell.style.italic) {
                QJsonObject cellObj;
                cellObj["r"] = r;
                cellObj["c"] = c;
                cellObj["v"] = cell.rawValue;
                if (!cell.style.textColor.isValid() || cell.style.textColor != Qt::black)
                    cellObj["tc"] = cell.style.textColor.name();
                if (!cell.style.backgroundColor.isValid() || cell.style.backgroundColor != Qt::white)
                    cellObj["bc"] = cell.style.backgroundColor.name();
                if (cell.style.bold) cellObj["b"] = true;
                if (cell.style.italic) cellObj["i"] = true;
                if (cell.style.underline) cellObj["u"] = true;
                cellsArray.append(cellObj);
            }
        }
    }
    root["cells"] = cellsArray;
    return root;
}

void SpreadsheetModel::deserialize(const QJsonObject& data)
{
    beginResetModel();

    m_rowCount = data["rowCount"].toInt(DEFAULT_ROWS);
    m_colCount = data["colCount"].toInt(DEFAULT_COLS);
    m_cells.clear();
    m_cells.resize(m_rowCount);
    for (auto& row : m_cells) {
        row.resize(m_colCount);
    }

    QJsonArray cellsArray = data["cells"].toArray();
    for (const auto& cellVal : cellsArray) {
        QJsonObject cellObj = cellVal.toObject();
        int r = cellObj["r"].toInt();
        int c = cellObj["c"].toInt();
        if (isValidIndex(r, c)) {
            m_cells[r][c].rawValue = cellObj["v"].toString();
            m_cells[r][c].isFormula = m_cells[r][c].rawValue.startsWith('=');
            m_cells[r][c].formula = m_cells[r][c].isFormula ? m_cells[r][c].rawValue.mid(1) : QString();
            if (cellObj.contains("tc")) m_cells[r][c].style.textColor = QColor(cellObj["tc"].toString());
            if (cellObj.contains("bc")) m_cells[r][c].style.backgroundColor = QColor(cellObj["bc"].toString());
            if (cellObj["b"].toBool()) m_cells[r][c].style.bold = true;
            if (cellObj["i"].toBool()) m_cells[r][c].style.italic = true;
            if (cellObj["u"].toBool()) m_cells[r][c].style.underline = true;

            QFont f;
            f.setBold(m_cells[r][c].style.bold);
            f.setItalic(m_cells[r][c].style.italic);
            f.setUnderline(m_cells[r][c].style.underline);
            m_cells[r][c].style.font = f;
        }
    }

    evaluateAll();
    endResetModel();
}

bool SpreadsheetModel::loadCSV(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    beginResetModel();
    m_cells.clear();

    QTextStream in(&file);
    int maxCols = 0;
    int row = 0;

    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList fields = line.split(',');
        int colCount = fields.size();
        if (colCount > maxCols) maxCols = colCount;

        QVector<CellData> rowData;
        for (const QString& field : fields) {
            CellData cell;
            cell.rawValue = field.trimmed();
            cell.isFormula = cell.rawValue.startsWith('=');
            cell.formula = cell.isFormula ? cell.rawValue.mid(1) : QString();
            bool ok;
            double val = cell.rawValue.toDouble(&ok);
            cell.computedValue = ok ? QVariant(val) : QVariant(cell.rawValue);
            rowData.append(cell);
        }
        m_cells.append(rowData);
        ++row;
    }
    file.close();

    m_rowCount = m_cells.size();
    m_colCount = maxCols > 0 ? maxCols : DEFAULT_COLS;

    // Pad rows to have consistent column count
    for (auto& rowData : m_cells) {
        while (rowData.size() < m_colCount) {
            rowData.append(CellData());
        }
    }

    evaluateAll();
    endResetModel();
    return true;
}

bool SpreadsheetModel::saveCSV(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    for (int r = 0; r < m_rowCount; ++r) {
        QStringList fields;
        for (int c = 0; c < m_colCount; ++c) {
            fields.append(m_cells[r][c].rawValue);
        }
        out << fields.join(',') << '\n';
    }
    file.close();
    return true;
}

bool SpreadsheetModel::isValidIndex(int row, int col) const
{
    return row >= 0 && row < m_rowCount && col >= 0 && col < m_colCount;
}

} // namespace ks
