#include "OfficeDocumentBridge.h"
#include "core/editor/documentpad/DocumentPad.h"
#include "core/editor/sheetpad/SheetPad.h"
#include <QDebug>

namespace ks::office {

OfficeDocumentBridge* OfficeDocumentBridge::s_instance = nullptr;

OfficeDocumentBridge* OfficeDocumentBridge::instance() {
    if (!s_instance) {
        s_instance = new OfficeDocumentBridge();
    }
    return s_instance;
}

OfficeDocumentBridge::OfficeDocumentBridge(QObject* parent)
    : QObject(parent)
{
    m_documentPad = new ks::DocumentPad(nullptr);
    m_sheetPad = new ks::SheetPad(nullptr);

    connect(m_documentPad, &ks::DocumentPad::fileOpened, this, [this](const QString&) {
        emit currentDocumentChanged();
    });
    connect(m_documentPad, &ks::DocumentPad::documentModifiedChanged, this, [this](bool) {
        emit documentModifiedChanged();
    });
    connect(m_documentPad, &ks::DocumentPad::fileSaved, this, [this](const QString&) {
        emit currentDocumentChanged();
    });

    connect(m_sheetPad, &ks::SheetPad::fileOpened, this, [this](const QString&) {
        emit currentSpreadsheetChanged();
    });
    connect(m_sheetPad, &ks::SheetPad::documentModifiedChanged, this, [this](bool) {
        emit spreadsheetModifiedChanged();
    });
    connect(m_sheetPad, &ks::SheetPad::fileSaved, this, [this](const QString&) {
        emit currentSpreadsheetChanged();
    });
}

int OfficeDocumentBridge::activeTab() const { return m_activeTab; }

void OfficeDocumentBridge::setActiveTab(int tab) {
    if (m_activeTab != tab) {
        m_activeTab = tab;
        emit activeTabChanged(tab);
    }
}

QString OfficeDocumentBridge::currentDocument() const {
    return m_documentPad ? m_documentPad->currentFilePath() : QString();
}

bool OfficeDocumentBridge::documentModified() const {
    return m_documentPad ? m_documentPad->documentModified() : false;
}

QString OfficeDocumentBridge::currentSpreadsheet() const {
    return m_sheetPad ? m_sheetPad->currentFilePath() : QString();
}

bool OfficeDocumentBridge::spreadsheetModified() const {
    return m_sheetPad ? m_sheetPad->documentModified() : false;
}

void OfficeDocumentBridge::newDocument() { if (m_documentPad) m_documentPad->newDocument(); }
void OfficeDocumentBridge::openDocument() { if (m_documentPad) m_documentPad->openDocument(); }
void OfficeDocumentBridge::saveDocument() { if (m_documentPad) m_documentPad->saveDocument(); }
void OfficeDocumentBridge::saveDocumentAs() { if (m_documentPad) m_documentPad->saveDocumentAs(); }

void OfficeDocumentBridge::newSpreadsheet() { if (m_sheetPad) m_sheetPad->newSpreadsheet(); }
void OfficeDocumentBridge::openSpreadsheet() { if (m_sheetPad) m_sheetPad->openSpreadsheet(); }
void OfficeDocumentBridge::saveSpreadsheet() { if (m_sheetPad) m_sheetPad->saveSpreadsheet(); }
void OfficeDocumentBridge::saveSpreadsheetAs() { if (m_sheetPad) m_sheetPad->saveSpreadsheetAs(); }
void OfficeDocumentBridge::exportCSV() { if (m_sheetPad) m_sheetPad->exportCSV(); }
void OfficeDocumentBridge::importCSV() { if (m_sheetPad) m_sheetPad->importCSV(); }

void OfficeDocumentBridge::documentBold() { if (m_documentPad) m_documentPad->setBold(); }
void OfficeDocumentBridge::documentItalic() { if (m_documentPad) m_documentPad->setItalic(); }
void OfficeDocumentBridge::documentUnderline() { if (m_documentPad) m_documentPad->setUnderline(); }
void OfficeDocumentBridge::documentStrikeThrough() { if (m_documentPad) m_documentPad->setStrikeThrough(); }
void OfficeDocumentBridge::documentSetFontSize(int size) { if (m_documentPad) m_documentPad->setFontSize(size); }
void OfficeDocumentBridge::documentSetFamily(const QString& family) { if (m_documentPad) m_documentPad->setFontFamily(family); }
void OfficeDocumentBridge::documentAlignLeft() {}
void OfficeDocumentBridge::documentAlignCenter() {}
void OfficeDocumentBridge::documentAlignRight() {}
void OfficeDocumentBridge::documentAlignJustify() {}
void OfficeDocumentBridge::documentInsertTable(int, int) {}
void OfficeDocumentBridge::documentUndo() { if (m_documentPad) m_documentPad->undo(); }
void OfficeDocumentBridge::documentRedo() { if (m_documentPad) m_documentPad->redo(); }

void OfficeDocumentBridge::spreadsheetBold() {}
void OfficeDocumentBridge::spreadsheetItalic() {}
void OfficeDocumentBridge::spreadsheetUnderline() {}
void OfficeDocumentBridge::spreadsheetInsertRow() {}
void OfficeDocumentBridge::spreadsheetInsertColumn() {}
void OfficeDocumentBridge::spreadsheetDeleteRow() {}
void OfficeDocumentBridge::spreadsheetDeleteColumn() {}
void OfficeDocumentBridge::spreadsheetClearContent() {}

QVariantMap OfficeDocumentBridge::spreadsheetGetCell(int row, int col) const {
    QVariantMap m;
    m["row"] = row;
    m["col"] = col;
    return m;
}

void OfficeDocumentBridge::spreadsheetSetCell(int, int, const QString&) {}

} // namespace ks::office
