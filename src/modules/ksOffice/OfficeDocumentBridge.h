#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QVariantList>

namespace ks { class DocumentPad; class SheetPad; }
namespace ks::office {

class OfficeDocumentBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentDocument READ currentDocument NOTIFY currentDocumentChanged)
    Q_PROPERTY(bool documentModified READ documentModified NOTIFY documentModifiedChanged)
    Q_PROPERTY(QString currentSpreadsheet READ currentSpreadsheet NOTIFY currentSpreadsheetChanged)
    Q_PROPERTY(bool spreadsheetModified READ spreadsheetModified NOTIFY spreadsheetModifiedChanged)
    Q_PROPERTY(int activeTab READ activeTab WRITE setActiveTab NOTIFY activeTabChanged)

public:
    static OfficeDocumentBridge* instance();

    QString currentDocument() const;
    bool documentModified() const;
    QString currentSpreadsheet() const;
    bool spreadsheetModified() const;
    int activeTab() const;
    void setActiveTab(int tab);

    // Document operations
    Q_INVOKABLE void newDocument();
    Q_INVOKABLE void openDocument();
    Q_INVOKABLE void saveDocument();
    Q_INVOKABLE void saveDocumentAs();

    // Spreadsheet operations
    Q_INVOKABLE void newSpreadsheet();
    Q_INVOKABLE void openSpreadsheet();
    Q_INVOKABLE void saveSpreadsheet();
    Q_INVOKABLE void saveSpreadsheetAs();
    Q_INVOKABLE void exportCSV();
    Q_INVOKABLE void importCSV();

    // Document formatting
    Q_INVOKABLE void documentBold();
    Q_INVOKABLE void documentItalic();
    Q_INVOKABLE void documentUnderline();
    Q_INVOKABLE void documentStrikeThrough();
    Q_INVOKABLE void documentSetFontSize(int size);
    Q_INVOKABLE void documentSetFamily(const QString& family);
    Q_INVOKABLE void documentAlignLeft();
    Q_INVOKABLE void documentAlignCenter();
    Q_INVOKABLE void documentAlignRight();
    Q_INVOKABLE void documentAlignJustify();
    Q_INVOKABLE void documentInsertTable(int rows, int cols);
    Q_INVOKABLE void documentUndo();
    Q_INVOKABLE void documentRedo();

    // Spreadsheet formatting
    Q_INVOKABLE void spreadsheetBold();
    Q_INVOKABLE void spreadsheetItalic();
    Q_INVOKABLE void spreadsheetUnderline();
    Q_INVOKABLE void spreadsheetInsertRow();
    Q_INVOKABLE void spreadsheetInsertColumn();
    Q_INVOKABLE void spreadsheetDeleteRow();
    Q_INVOKABLE void spreadsheetDeleteColumn();
    Q_INVOKABLE void spreadsheetClearContent();
    Q_INVOKABLE QVariantMap spreadsheetGetCell(int row, int col) const;
    Q_INVOKABLE void spreadsheetSetCell(int row, int col, const QString& value);

signals:
    void currentDocumentChanged();
    void documentModifiedChanged();
    void currentSpreadsheetChanged();
    void spreadsheetModifiedChanged();
    void activeTabChanged(int tab);

private:
    explicit OfficeDocumentBridge(QObject* parent = nullptr);
    static OfficeDocumentBridge* s_instance;

    ks::DocumentPad* m_documentPad = nullptr;
    ks::SheetPad* m_sheetPad = nullptr;
    int m_activeTab = 0;
};

} // namespace ks::office
