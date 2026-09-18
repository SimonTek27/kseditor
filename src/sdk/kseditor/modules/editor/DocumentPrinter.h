#pragma once

#include "EditorModule.h"

#if __has_include(<QPrinter>)
#include <QPrinter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPageSetupDialog>
#define HAS_QPRINTER 1
#endif

#include <QPdfWriter>
#include <QPainter>
#include <QTextDocument>
#include <QImage>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>

namespace ks {

#if HAS_QPRINTER

class DocumentPrinter : public QObject {
    Q_OBJECT
public:
    explicit DocumentPrinter(QObject* parent = nullptr);
    ~DocumentPrinter() override = default;

    void setDocument(QTextDocument* document);
    void setImage(const QImage& image);
    void setHtml(const QString& html);
    void setPlainText(const QString& text);

    void print();
    void printPreview();
    void pageSetup();
    void printToPdf(const QString& filePath = QString());

    QPrinter* printer() { return m_printer.get(); }
    const QPrinter* printer() const { return m_printer.get(); }

signals:
    void printRequested();
    void printCompleted(bool success, const QString& message);
    void pdfExportCompleted(bool success, const QString& filePath);

private slots:
    void onPreviewPaintRequested(QPrinter* printer);

private:
    void setupPrinter();
    void doPrint(QPrinter* printer);

    std::unique_ptr<QPrinter> m_printer;
    QTextDocument* m_document = nullptr;
    QImage m_image;
    QString m_html;
    QString m_plainText;
    bool m_hasImage = false;
};

class DocumentPrinterModule : public EditorModule {
    Q_OBJECT
public:
    explicit DocumentPrinterModule(QObject* parent = nullptr);
    ~DocumentPrinterModule() override = default;

    QString moduleName() const override { return tr("Document Printer"); }
    QString moduleId() const override { return "documentPrinter"; }

    void initialize() override;
    void shutdown() override;

    DocumentPrinter* printer() { return m_printer.get(); }

    bool canImportFile(const QString& filePath) const override;
    bool importFile(const QString& filePath) override;
    bool canExportFile(const QString& filePath) const override;
    bool exportFile(const QString& filePath) override;

signals:
    void printRequested();
    void printCompleted(bool success, const QString& message);

private:
    std::unique_ptr<DocumentPrinter> m_printer;
};

#endif // HAS_QPRINTER

} // namespace ks