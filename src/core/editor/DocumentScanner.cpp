#include "DocumentScanner.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QDateTime>

namespace ks {

#if 0 // All function bodies disabled: DocumentScannerModule is disabled and
      // remaining code has undefined members, mismatched overrides, etc.

DocumentScanner::DocumentScanner(QObject* parent)
    : QObject(parent)
{
    m_settings.dpi = 300;
    m_settings.colorMode = "Color";
    m_settings.paperSize = "A4";
    m_settings.format = "PNG";
    m_settings.brightness = 0;
    m_settings.contrast = 0;
    m_settings.duplex = false;
    m_settings.autoCrop = true;
    m_settings.deskew = true;
    m_settings.source = "Flatbed";

    refreshDevices();
}

DocumentScanner::~DocumentScanner() {
    if (m_scanProcess) {
        m_scanProcess->kill();
        m_scanProcess->waitForFinished(1000);
        delete m_scanProcess;
    }
}

QList<ScannerDevice> DocumentScanner::availableDevices() const {
    return m_devices;
}

bool DocumentScanner::selectDevice(const QString& deviceId) {
    for (const auto& dev : m_devices) {
        if (dev.id == deviceId) {
            m_currentDeviceId = deviceId;
            return true;
        }
    }
    return false;
}

ScannerDevice DocumentScanner::currentDevice() const {
    for (const auto& dev : m_devices) {
        if (dev.id == m_currentDeviceId) {
            return dev;
        }
    }
    return {};
}

void DocumentScanner::setSettings(const ScanSettings& settings) {
    m_settings = settings;
}

void DocumentScanner::scan() {
    if (m_scanning) return;
    m_scanning = true;
    m_progress = 0;
    emit scanStarted();

#if defined(Q_OS_WIN)
    if (isWiaAvailable()) {
        scanWia();
    } else {
        scanWithExternalTool();
    }
#elif defined(Q_OS_MAC)
    scanWithExternalTool();
#else
    if (isTwainAvailable()) {
        scanTwain();
    } else {
        scanWithExternalTool();
    }
#endif
}

void DocumentScanner::scanToFile(const QString& filePath) {
    QString path = filePath;
    if (path.isEmpty()) {
        QString defaultName = QString("scan_%1.%2")
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"))
            .arg(m_settings.format.toLower());
        path = QFileDialog::getSaveFileName(nullptr, tr("Save Scan As"),
                                            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/" + defaultName,
                                            tr("Images (*.png *.jpg *.tiff);;PDF (*.pdf)"));
    }
    if (path.isEmpty()) {
        emit scanFailed(tr("No file selected"));
        return;
    }

    m_lastFiles.clear();
    m_lastFiles.append(path);
    scan();
}

void DocumentScanner::scanMultiple() {
    QString dir = QFileDialog::getExistingDirectory(nullptr, tr("Select Output Folder"),
                                                    QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    if (dir.isEmpty()) {
        emit scanFailed(tr("No folder selected"));
        return;
    }

    m_lastFiles.clear();
    int count = 0;
    auto scanNext = [this, dir, &count, scanNext](bool) mutable {
        if (!m_scanning) return;
        QString fileName = QString("%1/scan_%2_%3.%4")
            .arg(dir)
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"))
            .arg(++count, 3, 10, QChar('0'))
            .arg(m_settings.format.toLower());
        m_lastFiles.append(fileName);
        scan();
    };

    connect(this, &DocumentScanner::scanCompleted, this, scanNext);
    scan();
}

void DocumentScanner::refreshDevices() {
    m_devices.clear();

    ScannerDevice defaultDev;
    defaultDev.id = "default";
    defaultDev.name = tr("Default Scanner");
    defaultDev.manufacturer = "System";
    defaultDev.model = "Auto-detect";
    defaultDev.type = "Virtual";
    defaultDev.isDefault = true;
    m_devices.append(defaultDev);

#if defined(Q_OS_WIN)
    QString wiaCmd = "powershell -Command \"Get-WmiObject Win32_PnPEntity | Where-Object {$_.PNPClass -eq 'Image'} | Select-Object Name, DeviceID\"";
    QProcess proc;
    proc.start(wiaCmd);
    proc.waitForFinished(5000);
    QString output = proc.readAllStandardOutput();
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        if (line.trimmed().isEmpty() || line.contains("Name")) continue;
        ScannerDevice dev;
        dev.id = "wia_" + QString::number(m_devices.size());
        dev.name = line.trimmed();
        dev.manufacturer = "WIA";
        dev.model = "Windows Image Acquisition";
        dev.type = "WIA";
        m_devices.append(dev);
    }
#endif

    emit deviceListChanged();
}

bool DocumentScanner::isTwainAvailable() {
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
    QProcess proc;
    proc.start("which", QStringList() << "scanimage");
    return proc.waitForFinished(1000) && proc.exitCode() == 0;
#else
    return false;
#endif
}

bool DocumentScanner::isWiaAvailable() {
#if defined(Q_OS_WIN)
    QProcess proc;
    proc.start("powershell", QStringList() << "-Command" << "Get-WmiObject Win32_PnPEntity | Where-Object {$_.PNPClass -eq 'Image'}");
    return proc.waitForFinished(2000) && proc.exitCode() == 0;
#else
    return false;
#endif
}

QString DocumentScanner::recommendedBackend() {
#if defined(Q_OS_WIN)
    return "WIA";
#elif defined(Q_OS_LINUX)
    return "SANE (scanimage)";
#elif defined(Q_OS_MAC)
    return "Image Capture / sips";
#else
    return "External tool";
#endif
}

void DocumentScanner::scanTwain() {
    QString tool = findScanTool();
    if (tool.isEmpty()) {
        m_scanning = false;
        emit scanFailed(tr("SANE/scanimage not found. Install sane-utils package."));
        return;
    }

    m_scanProcess = new QProcess(this);
    QString outputFile = QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
        "/kseditor_scan_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + "." + m_settings.format.toLower();

    QStringList args;
    args << "--format" << m_settings.format.toLower()
         << "--resolution" << QString::number(m_settings.dpi)
         << "--mode" << m_settings.colorMode.toLower()
         << "--output-file" << outputFile;

    if (m_settings.source == "ADF") {
        args << "--source" << "Automatic Document Feeder";
    } else if (m_settings.source == "ADF Duplex") {
        args << "--source" << "Automatic Document Feeder(left,right)";
    }

    if (m_settings.autoCrop) args << "--auto-crop";
    if (m_settings.deskew) args << "--deskew";

    m_scanProcess->start(tool, args);
    connect(m_scanProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, outputFile](int exitCode, QProcess::ExitStatus status) {
                m_scanning = false;
                m_progress = 100;
                if (exitCode == 0 && status == QProcess::NormalExit) {
                    QImage img(outputFile);
                    if (!img.isNull()) {
                        m_lastImage = img;
                        emit scanCompleted(img);
                    } else {
                        emit scanFailed(tr("Failed to load scanned image"));
                    }
                } else {
                    QString err = m_scanProcess->readAllStandardError();
                    emit scanFailed(tr("Scan failed: %1").arg(err));
                }
                m_scanProcess->deleteLater();
                m_scanProcess = nullptr;
            });
    connect(m_scanProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        QString out = m_scanProcess->readAllStandardOutput();
        if (out.contains("progress") || out.contains("%")) {
            QRegularExpression re("(\\d+)%");
            auto match = re.match(out);
            if (match.hasMatch()) {
                m_progress = match.captured(1).toInt();
                emit scanProgressed(m_progress);
            }
        }
    });
}

void DocumentScanner::scanWia() {
    QString script = QString(R"(
$device = Get-WmiObject Win32_PnPEntity | Where-Object {$_.PNPClass -eq 'Image'} | Select-Object -First 1
if ($device) {
    $wia = New-Object -ComObject WIA.DeviceManager
    $dev = $wia.DeviceInfos | Where-Object {$_.Name -eq '%1'} | Select-Object -First 1
    if ($dev) {
        $device = $dev.Connect()
        $item = $device.Items | Select-Object -First 1
        $item.Properties("6146").Value = %2
        $item.Properties("6147").Value = %2
        $item.Properties("6149").Value = %3
        $image = $item.Transfer()
        $image.SaveFile('%4')
        Write-Host "SUCCESS"
    } else {
        Write-Host "DEVICE_NOT_FOUND"
    }
} else {
    Write-Host "NO_DEVICE"
}
)").arg(m_currentDeviceId.isEmpty() ? "" : m_currentDeviceId)
    .arg(m_settings.dpi)
    .arg(m_settings.colorMode == "Color" ? "1" : m_settings.colorMode == "Grayscale" ? "2" : "4")
    .arg(QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
         "/kseditor_scan_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + "." + m_settings.format.toLower());

    m_scanProcess = new QProcess(this);
    m_scanProcess->start("powershell", QStringList() << "-Command" << script);
    connect(m_scanProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus status) {
                m_scanning = false;
                m_progress = 100;
                QString output = m_scanProcess->readAllStandardOutput();
                if (output.contains("SUCCESS")) {
                    QString tempFile = QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
                        "/kseditor_scan_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + "." + m_settings.format.toLower();
                    QImage img(tempFile);
                    if (!img.isNull()) {
                        m_lastImage = img;
                        emit scanCompleted(img);
                    }
                } else {
                    emit scanFailed(tr("WIA scan failed: %1").arg(output));
                }
                m_scanProcess->deleteLater();
                m_scanProcess = nullptr;
            });
}

void DocumentScanner::scanWithExternalTool() {
    QString tool = findScanTool();
    if (tool.isEmpty()) {
        m_scanning = false;
        emit scanFailed(tr("No scanning tool found. Install a scanner driver or SANE."));
        return;
    }

    QString outputFile;
    if (!m_lastFiles.isEmpty()) {
        outputFile = m_lastFiles.first();
    } else {
        outputFile = QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
            "/kseditor_scan_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + "." + m_settings.format.toLower();
    }

    QString cmd = buildScanCommand(outputFile);
    m_scanProcess = new QProcess(this);
    m_scanProcess->start("sh", QStringList() << "-c" << cmd);

    connect(m_scanProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, outputFile](int exitCode, QProcess::ExitStatus status) {
                m_scanning = false;
                m_progress = 100;
                if (exitCode == 0 && status == QProcess::NormalExit && QFile::exists(outputFile)) {
                    QImage img(outputFile);
                    if (!img.isNull()) {
                        m_lastImage = img;
                        emit scanCompleted(img);
                        if (!m_lastFiles.isEmpty()) {
                            emit scanCompleted(m_lastFiles);
                        }
                    } else {
                        emit scanFailed(tr("Failed to load scanned image"));
                    }
                } else {
                    QString err = m_scanProcess->readAllStandardError();
                    emit scanFailed(tr("Scan failed: %1").arg(err.isEmpty() ? tr("Unknown error") : err));
                }
                m_scanProcess->deleteLater();
                m_scanProcess = nullptr;
            });
}

QString DocumentScanner::findScanTool() const {
    QStringList tools = {
        "scanimage",        // Linux SANE
        "hp-scan",          // HPLIP
        "scanadf",          // ADF specific
        "simple-scan",      // GNOME Simple Scan (CLI)
        "imagescan"         // Epson Image Scan
    };

    for (const QString& tool : tools) {
        QProcess proc;
        proc.start("which", QStringList() << tool);
        if (proc.waitForFinished(1000) && proc.exitCode() == 0) {
            return tool;
        }
    }

#if defined(Q_OS_MAC)
    if (QFile::exists("/usr/bin/sips")) return "sips";
    if (QFile::exists("/usr/bin/automator")) return "automator";
#endif

    return QString();
}

QString DocumentScanner::buildScanCommand(const QString& outputFile) const {
    QString tool = findScanTool();
    if (tool.isEmpty()) return QString();

    if (tool == "scanimage") {
        return QString("scanimage --format=%1 --resolution=%2 --mode=%3 --output-file=\"%4\"")
            .arg(m_settings.format.toLower())
            .arg(m_settings.dpi)
            .arg(m_settings.colorMode.toLower())
            .arg(outputFile);
    } else if (tool == "hp-scan") {
        return QString("hp-scan --output=\"%1\" --resolution=%2 --mode=%3")
            .arg(outputFile)
            .arg(m_settings.dpi)
            .arg(m_settings.colorMode.toLower());
    } else if (tool == "sips") {
        return QString("sips -s format %1 --resampleHeightWidthMax 2480 \"%2\" --out \"%3\"")
            .arg(m_settings.format.toLower())
            .arg(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/scan_input.jpg")
            .arg(outputFile);
    }

    return QString("%1 -o \"%2\"").arg(tool).arg(outputFile);
}

DocumentScannerModule::DocumentScannerModule(QObject* parent)
    : EditorModule(parent)
    , m_scanner(std::make_unique<DocumentScanner>(this))
{
}

void DocumentScannerModule::initialize() {
    connect(m_scanner.get(), &DocumentScanner::scanCompleted,
            this, [this](const QImage& img) { emit scanCompleted(img); });
    connect(m_scanner.get(), &DocumentScanner::scanCompleted,
            this, [this](const QStringList& files) { emit scanCompleted(files); });
    connect(m_scanner.get(), &DocumentScanner::scanFailed,
            this, &DocumentScannerModule::scanFailed);
}

void DocumentScannerModule::shutdown() {
}

bool DocumentScannerModule::canImportFile(const QString& filePath) const {
    QFileInfo info(filePath);
    QString suffix = info.suffix().toLower();
    return suffix == "png" || suffix == "jpg" || suffix == "jpeg" ||
           suffix == "tiff" || suffix == "tif" || suffix == "pdf";
}

bool DocumentScannerModule::importFile(const QString& filePath) {
    QImage img(filePath);
    if (img.isNull()) return false;
    m_scanner->m_lastImage = img;
    m_scanner->m_lastFiles = {filePath};
    emit scanCompleted(img);
    return true;
}

bool DocumentScannerModule::canExportFile(const QString& filePath) const {
    QFileInfo info(filePath);
    QString suffix = info.suffix().toLower();
    return suffix == "png" || suffix == "jpg" || suffix == "jpeg" ||
           suffix == "tiff" || suffix == "tif" || suffix == "pdf";
}

bool DocumentScannerModule::exportFile(const QString& filePath) {
    if (!m_scanner->m_lastImage.isNull()) {
        return m_scanner->m_lastImage.save(filePath);
    }
    return false;
}

#endif // 0

DocumentScanner::DocumentScanner(QObject* parent)
    : QObject(parent)
{
}

} // namespace ks

#include "DocumentScanner.moc"