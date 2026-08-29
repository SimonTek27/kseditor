#include "DocumentPrinter.h"
#include <QPainter>
#include <QTextDocument>
#include <QApplication>
#include <QFileInfo>
#include <QDir>

#if HAS_QPRINTER

namespace ks {

DocumentPrinter::DocumentPrinter(QObject* parent)
    : QObject(parent)
    , m_printer(std::make_unique<QPrinter>(QPrinter::HighResolution))
{
    setupPrinter();
}

void DocumentPrinter::setupPrinter() {
    m_printer->setPageSize(QPageSize(QPageSize::A4));
    m_printer->setPageOrientation(QPageLayout::Portrait);
    m_printer->setFullPage(false);
    m_printer->setColorMode(QPrinter::Color);
    m_printer->setResolution(300);
}

void DocumentPrinter::setDocument(QTextDocument* document) {
    m_document = document;
    m_hasImage = false;
    m_html.clear();
    m_plainText.clear();
}

void DocumentPrinter::setImage(const QImage& image) {
    m_image = image;
    m_hasImage = true;
    m_document = nullptr;
    m_html.clear();
    m_plainText.clear();
}

void DocumentPrinter::setHtml(const QString& html) {
    m_html = html;
    m_document = nullptr;
    m_hasImage = false;
    m_plainText.clear();
}

void DocumentPrinter::setPlainText(const QString& text) {
    m_plainText = text;
    m_document = nullptr;
    m_hasImage = false;
    m_html.clear();
}

void DocumentPrinter::print() {
    QPrintDialog dialog(m_printer.get(), nullptr);
    dialog.setWindowTitle(tr("Print Document"));
    dialog.setOption(QPrintDialog::PrintPageRange, true);
    dialog.setOption(QPrintDialog::PrintShowPageSize, true);
    dialog.setOption(QPrintDialog::PrintCollateCopies, true);

    if (dialog.exec() == QDialog::Accepted) {
        doPrint(m_printer.get());
    }
}

void DocumentPrinter::printPreview() {
    auto preview = std::make_unique<QPrintPreviewDialog>(m_printer.get(), nullptr);
    preview->setWindowTitle(tr("Print Preview"));
    preview->setMinimumSize(800, 600);
    connect(preview.get(), &QPrintPreviewDialog::paintRequested,
            this, &DocumentPrinter::onPreviewPaintRequested);
    preview->exec();
}

void DocumentPrinter::pageSetup() {
    QPageSetupDialog dialog(m_printer.get(), nullptr);
    dialog.setWindowTitle(tr("Page Setup"));
    dialog.exec();
}

void DocumentPrinter::printToPdf(const QString& filePath) {
    QString path = filePath;
    if (path.isEmpty()) {
        path = QFileDialog::getSaveFileName(nullptr, tr("Export to PDF"),
                                            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/document.pdf",
                                            tr("PDF Files (*.pdf)"));
    }
    if (path.isEmpty()) {
        emit pdfExportCompleted(false, tr("No file selected"));
        return;
    }

    if (!path.endsWith(".pdf", Qt::CaseInsensitive)) {
        path += ".pdf";
    }

    QPdfWriter pdfWriter(path);
    pdfWriter.setPageSize(m_printer->pageSize());
    pdfWriter.setPageOrientation(m_printer->pageOrientation());
    pdfWriter.setResolution(m_printer->resolution());
    pdfWriter.setPageMargins(m_printer->pageLayout().paintRectPixels(m_printer->resolution()).margins(QMarginsF()));

    doPrint(&pdfWriter);
    emit pdfExportCompleted(true, path);
}

void DocumentPrinter::onPreviewPaintRequested(QPrinter* printer) {
    doPrint(printer);
}

void DocumentPrinter::doPrint(QPrinter* printer) {
    QPainter painter(printer);
    if (!painter.isActive()) {
        emit printCompleted(false, tr("Failed to start painting"));
        return;
    }

    bool success = false;
    QString message;

    if (m_hasImage && !m_image.isNull()) {
        QRectF pageRect = printer->pageLayout().paintRectPixels(printer->resolution());
        QSize imgSize = m_image.size();
        qreal scale = qMin(pageRect.width() / imgSize.width(), pageRect.height() / imgSize.height());
        QSize scaledSize = (imgSize * scale).toSize();
        QPointF pos((pageRect.width() - scaledSize.width()) / 2, (pageRect.height() - scaledSize.height()) / 2);
        painter.drawImage(QRectF(pos, scaledSize), m_image);
        success = true;
        message = tr("Image printed successfully");
    } else if (m_document) {
        m_document->print(printer);
        success = true;
        message = tr("Document printed successfully");
    } else if (!m_html.isEmpty()) {
        QTextDocument doc;
        doc.setHtml(m_html);
        doc.print(printer);
        success = true;
        message = tr("HTML document printed successfully");
    } else if (!m_plainText.isEmpty()) {
        QTextDocument doc;
        doc.setPlainText(m_plainText);
        doc.print(printer);
        success = true;
        message = tr("Text document printed successfully");
    } else {
        message = tr("No content to print");
    }

    emit printCompleted(success, message);
    emit printRequested();
}

DocumentPrinterModule::DocumentPrinterModule(QObject* parent)
    : EditorModule(parent)
    , m_printer(std::make_unique<DocumentPrinter>(this))
{
}

void DocumentPrinterModule::initialize() {
    connect(m_printer.get(), &DocumentPrinter::printCompleted,
            this, &DocumentPrinterModule::printCompleted);
}

void DocumentPrinterModule::shutdown() {
}

bool DocumentPrinterModule::canImportFile(const QString& filePath) const {
    QFileInfo info(filePath);
    QString suffix = info.suffix().toLower();
    return suffix == "pdf" || suffix == "txt" || suffix == "html" || suffix == "htm";
}

bool DocumentPrinterModule::importFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    QFileInfo info(filePath);
    QString suffix = info.suffix().toLower();
    if (suffix == "html" || suffix == "htm") {
        m_printer->setHtml(file.readAll());
    } else {
        m_printer->setPlainText(file.readAll());
    }
    return true;
}

bool DocumentPrinterModule::canExportFile(const QString& filePath) const {
    QFileInfo info(filePath);
    QString suffix = info.suffix().toLower();
    return suffix == "pdf";
}

bool DocumentPrinterModule::exportFile(const QString& filePath) {
    m_printer->printToPdf(filePath);
    return true;
}

} // namespace ks

#endif // HAS_QPRINTER

#include "DocumentPrinter.moc"