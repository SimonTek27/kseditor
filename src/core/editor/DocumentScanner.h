#pragma once

#include "EditorModule.h"
#include <QObject>
#include <QImage>
#include <QList>
#include <QString>
#include <QThread>
#include <QProcess>
#include <QStandardPaths>
#include <QFileDialog>
#include <QMessageBox>

namespace ks {

struct ScannerDevice {
    QString id;
    QString name;
    QString manufacturer;
    QString model;
    QString type;
    bool isDefault = false;
};

struct ScanSettings {
    int dpi = 300;
    QString colorMode = "Color"; // Color, Grayscale, BlackWhite
    QString paperSize = "A4";
    QString format = "PNG"; // PNG, JPEG, TIFF, PDF
    int brightness = 0;
    int contrast = 0;
    bool duplex = false;
    bool autoCrop = false;
    bool deskew = false;
    QString source = "Flatbed"; // Flatbed, ADF, ADF Duplex
};

class DocumentScanner : public QObject {
    Q_OBJECT
public:
    explicit DocumentScanner(QObject* parent = nullptr);
    ~DocumentScanner() override = default;

    QList<ScannerDevice> availableDevices() const;
    bool selectDevice(const QString& deviceId);
    ScannerDevice currentDevice() const;

    void setSettings(const ScanSettings& settings);
    ScanSettings settings() const { return m_settings; }

    void scan();
    void scanToFile(const QString& filePath);
    void scanMultiple();

    QImage lastScannedImage() const { return m_lastImage; }
    QStringList lastScannedFiles() const { return m_lastFiles; }

    bool isScanning() const { return m_scanning; }
    int scanProgress() const { return m_progress; }

    static bool isTwainAvailable();
    static bool isWiaAvailable();
    static QString recommendedBackend();

signals:
    void scanStarted();
    void scanProgressed(int progress);
    void scanCompleted(const QImage& image);
    void scanCompleted(const QStringList& files);
    void scanFailed(const QString& error);
    void deviceListChanged();

private:
    void scanTwain();
    void scanWia();
    void scanWithExternalTool();
    QString findScanTool() const;
    QString buildScanCommand(const QString& outputFile) const;

    QList<ScannerDevice> m_devices;
    QString m_currentDeviceId;
    ScanSettings m_settings;
    QImage m_lastImage;
    QStringList m_lastFiles;
    bool m_scanning = false;
    int m_progress = 0;
    QProcess* m_scanProcess = nullptr;
};

#if 0 // DocumentScannerModule: deeply broken (mismatched override signatures, undefined members)
class DocumentScannerModule : public EditorModule {
    Q_OBJECT
public:
    explicit DocumentScannerModule(QObject* parent = nullptr);
    ~DocumentScannerModule() override = default;

    QString moduleName() const override { return tr("Document Scanner"); }
    QString moduleId() const override { return "documentScanner"; }

    void initialize() override;
    void shutdown() override;

    DocumentScanner* scanner() { return m_scanner.get(); }

    bool canImportFile(const QString& filePath) const override;
    bool importFile(const QString& filePath) override;
    bool canExportFile(const QString& filePath) const override;
    bool exportFile(const QString& filePath) override;

signals:
    void scanRequested();
    void scanCompleted(const QImage& image);
    void scanCompleted(const QStringList& files);
    void scanFailed(const QString& error);

private:
    std::unique_ptr<DocumentScanner> m_scanner;
};
#endif

} // namespace ks