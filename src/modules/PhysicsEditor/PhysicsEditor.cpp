#include "PhysicsEditor.h"
#include <QSettings>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QTextStream>
#include <QFile>
#include <QDebug>
#include <QHeaderView>
#include <QInputDialog>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMenu>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QMessageBox>
#include "core/editor/EditorConfig.h"

namespace ks {

// ============================================================================
// PhysicsEditorModule
// ============================================================================

PhysicsEditorModule::PhysicsEditorModule(QWidget* parent)
    : EditorModule(parent)
{
    buildUI();
}

bool PhysicsEditorModule::initialize() {
    QString defaultPath = KsGameSettings::getValue("CARS_PATH",
        EditorConfig::instance().simContentCarsPath().isEmpty()
            ? QFileInfo(QDir::homePath() + "/../Documents/Assetto Corsa/content/cars").absolutePath()
            : EditorConfig::instance().simContentCarsPath()).toString();
    if (QDir(defaultPath).exists()) {
        m_carsPath = defaultPath;
    }
    if (!m_carsPath.isEmpty()) {
        m_carBrowser->setCarsPath(m_carsPath);
    }
    return true;
}

void PhysicsEditorModule::buildUI() {
    auto* root = new QHBoxLayout(this);

    m_carBrowser = new CarBrowserWidget(this);
    m_carBrowser->setMinimumWidth(220);
    m_carBrowser->setMaximumWidth(300);

    auto* contentWidget = new QWidget(this);
    auto* contentLayout = new QVBoxLayout(contentWidget);

    auto* toolbar = new QHBoxLayout;
    auto* openBtn   = new QPushButton(tr("Open Cars Folder"), this);
    auto* saveBtn   = new QPushButton(tr("Save"), this);
    auto* exportBtn = new QPushButton(tr("Export..."), this);
    auto* lutBtn    = new QPushButton(tr("LUT Editor"), this);
    auto* engBtn    = new QPushButton(tr("Engine Curve"), this);
    auto* telBtn    = new QPushButton(tr("Telemetry"), this);
    auto* cmpBtn    = new QPushButton(tr("Compare Setups"), this);
    auto* acdBtn    = new QPushButton(tr("ACD Browser"), this);
    auto* suspBtn   = new QPushButton(tr("Susp Geometry"), this);
    auto* ffbBtn    = new QPushButton(tr("FFB Preview"), this);
    auto* validBtn  = new QPushButton(tr("Validator"), this);
    auto* tyreCurveBtn = new QPushButton(tr("Tire Curves"), this);
    auto* tyreTempBtn = new QPushButton(tr("Tyre Temp Sim"), this);
    m_carLabel = new QLabel(tr("No car selected"), this);
    m_carLabel->setStyleSheet("font-weight: bold;");

    toolbar->addWidget(openBtn);
    toolbar->addWidget(saveBtn);
    toolbar->addWidget(exportBtn);
    toolbar->addSpacing(8);
    toolbar->addWidget(lutBtn);
    toolbar->addWidget(engBtn);
    toolbar->addWidget(telBtn);
    toolbar->addWidget(cmpBtn);
    toolbar->addWidget(acdBtn);
    toolbar->addWidget(suspBtn);
    toolbar->addWidget(ffbBtn);
    auto* cfmBtn = new QPushButton(tr("CFD"), this);
    toolbar->addWidget(cfmBtn);
    auto* thBtn = new QPushButton(tr("Tire Thermal"), this);
    toolbar->addWidget(thBtn);
    toolbar->addWidget(tyreCurveBtn);
    toolbar->addWidget(validBtn);
    toolbar->addWidget(tyreTempBtn);
    toolbar->addStretch();
    toolbar->addWidget(m_carLabel);
    contentLayout->addLayout(toolbar);

    m_contentStack = new QStackedWidget(this);

    auto* welcome = new QWidget(this);
    auto* welcomeLayout = new QVBoxLayout(welcome);
    auto* welcomeLabel = new QLabel(tr(
        "<h2>Physics Editor</h2>"
        "<p>Select a car from the sidebar or open a cars folder.</p>"
        "<p>Edit car INI files with syntax highlighting, manage ACD archives, "
        "visualize tyre LUT curves, tune engine power/torque, compare setups, "
        "and browse ACD contents.</p>"), this);
    welcomeLabel->setAlignment(Qt::AlignCenter);
    welcomeLayout->addWidget(welcomeLabel);
    welcomeLayout->addStretch();
    m_contentStack->addWidget(welcome);

    m_iniEditor = new IniEditorWidget(this);
    m_contentStack->addWidget(m_iniEditor);

    m_tyresEditor = new TyresTableWidget(this);
    m_contentStack->addWidget(m_tyresEditor);

    m_acdManager = new AcdManagerWidget(this);
    m_contentStack->addWidget(m_acdManager);

    m_lutEditor = new LutCurveWidget(this);
    m_contentStack->addWidget(m_lutEditor);

    m_engineEditor = new EngineCurveWidget(this);
    m_contentStack->addWidget(m_engineEditor);

    m_telemetryWidget = new TelemetryWidget(this);
    m_contentStack->addWidget(m_telemetryWidget);

    m_setupCompare = new CarSetupCompareWidget(this);
    m_contentStack->addWidget(m_setupCompare);

    m_acdBrowser = new AcdBrowserWidget(this);
    m_contentStack->addWidget(m_acdBrowser);

    m_suspGeometry = new SuspGeometryWidget(this);
    m_contentStack->addWidget(m_suspGeometry);

    m_ffbPreview = new FfbPreviewWidget(this);
    m_contentStack->addWidget(m_ffbPreview);

    m_cfdWidget = new CfdWidget(this);
    m_contentStack->addWidget(m_cfdWidget);

    m_tireThermalWidget = new TireThermalWidget(this);
    m_contentStack->addWidget(m_tireThermalWidget);

    m_validator = new CarValidatorWidget(this);
    m_contentStack->addWidget(m_validator);

    m_tyreTempModel = new TyreTempModelWidget(this);
    m_contentStack->addWidget(m_tyreTempModel);

    m_tireCurveEditor = new TireCurveEditor(this);
    m_contentStack->addWidget(m_tireCurveEditor);

    m_fileTree = new FileTreeWidget(this);
    m_fileTree->setMinimumWidth(200);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_fileTree);
    splitter->addWidget(m_contentStack);
    splitter->setSizes({200, 600});
    contentLayout->addWidget(splitter);

    m_statusLabel = new QLabel(tr("Ready"), this);
    contentLayout->addWidget(m_statusLabel);

    root->addWidget(m_carBrowser);
    root->addWidget(contentWidget, 1);

    connect(m_carBrowser, &CarBrowserWidget::carSelected, this, &PhysicsEditorModule::onCarSelected);
    connect(m_fileTree, &FileTreeWidget::fileSelected, this, &PhysicsEditorModule::onFileSelected);
    connect(openBtn, &QPushButton::clicked, this, &PhysicsEditorModule::onOpenCarsFolder);
    connect(saveBtn, &QPushButton::clicked, this, &PhysicsEditorModule::onSaveCurrentFile);
    connect(exportBtn, &QPushButton::clicked, this, &PhysicsEditorModule::onExportCar);
    connect(lutBtn, &QPushButton::clicked, this, &PhysicsEditorModule::onShowLutEditor);
    connect(engBtn, &QPushButton::clicked, this, &PhysicsEditorModule::onShowEngineCurve);
    connect(telBtn, &QPushButton::clicked, this, &PhysicsEditorModule::onShowTelemetry);
    connect(cmpBtn, &QPushButton::clicked, this, &PhysicsEditorModule::onShowSetupCompare);
    connect(acdBtn, &QPushButton::clicked, this, &PhysicsEditorModule::onShowAcdBrowser);
    connect(suspBtn, &QPushButton::clicked, this, &PhysicsEditorModule::onShowSuspGeometry);
    connect(ffbBtn, &QPushButton::clicked, this, &PhysicsEditorModule::onShowFfbPreview);
    connect(cfmBtn, &QPushButton::clicked, this, &PhysicsEditorModule::onShowCfdWidget);
    connect(thBtn, &QPushButton::clicked, this, &PhysicsEditorModule::onShowTireThermalWidget);
    connect(tyreCurveBtn, &QPushButton::clicked, this, &PhysicsEditorModule::onShowTireCurveEditor);
    connect(validBtn, &QPushButton::clicked, this, &PhysicsEditorModule::onShowCarValidator);
    connect(tyreTempBtn, &QPushButton::clicked, this, &PhysicsEditorModule::onShowTyreTempModel);

    connect(m_acdManager, &AcdManagerWidget::acdExtracted, this, [this](const QString& path) {
        m_acdBrowser->setExtractedPath(path + "/data_extracted");
    });
}

void PhysicsEditorModule::onShowLutEditor() {
    if (!m_currentCar.isEmpty()) {
        QDir dataDir(m_currentCar + "/data");
        if (dataDir.exists()) {
            QStringList lutFiles = dataDir.entryList(QStringList() << "*.lut" << "*LUT*", QDir::Files);
            if (!lutFiles.isEmpty()) {
                m_lutEditor->loadLutFile(dataDir.absoluteFilePath(lutFiles.first()));
            }
        }
    }
    m_contentStack->setCurrentWidget(m_lutEditor);
}

void PhysicsEditorModule::onShowEngineCurve() {
    if (!m_currentCar.isEmpty()) {
        m_engineEditor->loadFromIni(m_currentCar);
    }
    m_contentStack->setCurrentWidget(m_engineEditor);
}

void PhysicsEditorModule::onShowTelemetry() {
    if (!m_currentCar.isEmpty()) {
        m_telemetryWidget->startSession(m_currentCar);
    }
    m_contentStack->setCurrentWidget(m_telemetryWidget);
}

void PhysicsEditorModule::onShowSetupCompare() {
    m_contentStack->setCurrentWidget(m_setupCompare);
}

void PhysicsEditorModule::onShowAcdBrowser() {
    if (!m_currentCar.isEmpty()) {
        m_acdBrowser->setExtractedPath(m_currentCar + "/data_extracted");
        m_acdBrowser->refresh();
    }
    m_contentStack->setCurrentWidget(m_acdBrowser);
}

void PhysicsEditorModule::onShowSuspGeometry() {
    m_contentStack->setCurrentWidget(m_suspGeometry);
}

void PhysicsEditorModule::onShowFfbPreview() {
    m_contentStack->setCurrentWidget(m_ffbPreview);
}

void PhysicsEditorModule::onShowCfdWidget() {
    m_contentStack->setCurrentWidget(m_cfdWidget);
}

void PhysicsEditorModule::onShowTireThermalWidget() {
    m_contentStack->setCurrentWidget(m_tireThermalWidget);
}

void PhysicsEditorModule::onShowTireCurveEditor() {
    m_contentStack->setCurrentWidget(m_tireCurveEditor);
}

void PhysicsEditorModule::onShowCarValidator() {
    if (!m_currentCar.isEmpty()) {
        m_validator->validateCar(m_currentCar);
    }
    m_contentStack->setCurrentWidget(m_validator);
}

void PhysicsEditorModule::onShowTyreTempModel() {
    m_contentStack->setCurrentWidget(m_tyreTempModel);
}

QJsonObject PhysicsEditorModule::serializeProject() const {
    QJsonObject data;
    data["carsPath"]     = m_carsPath;
    data["currentCar"]   = m_currentCar;
    data["currentFile"]  = m_currentFile;
    return data;
}

void PhysicsEditorModule::deserializeProject(const QJsonObject& data) {
    m_carsPath   = data["carsPath"].toString();
    m_currentCar = data["currentCar"].toString();
    m_currentFile = data["currentFile"].toString();

    if (!m_carsPath.isEmpty() && QDir(m_carsPath).exists()) {
        m_carBrowser->setCarsPath(m_carsPath);
    }
    if (!m_currentCar.isEmpty() && QDir(m_currentCar).exists()) {
        onCarSelected(m_currentCar);
    }
}

void PhysicsEditorModule::onCarSelected(const QString& carFolder) {
    m_currentCar = carFolder;
    QString carName = QFileInfo(carFolder).fileName();
    m_carLabel->setText(tr("Car: %1").arg(carName));
    populateFileTree(carFolder);
    m_acdManager->setCarPath(carFolder);
    loadCarIniFiles(carFolder);
    m_statusLabel->setText(tr("Loaded: %1").arg(carName));
    updateWindowTitle();
}

void PhysicsEditorModule::onFileSelected(const QString& path) {
    if (path.endsWith(".ini", Qt::CaseInsensitive)) {
        m_iniEditor->loadFile(path);
        m_contentStack->setCurrentWidget(m_iniEditor);
        m_currentFile = path;
    }
}

void PhysicsEditorModule::onOpenCarsFolder() {
    QString path = QFileDialog::getExistingDirectory(this,
        tr("Select Cars Folder"),
        m_carsPath.isEmpty() ? QDir::homePath() : m_carsPath);
    if (!path.isEmpty()) {
        m_carsPath = path;
        m_carBrowser->setCarsPath(path);
        KsGameSettings::setValue("CARS_PATH", path);
    }
}

void PhysicsEditorModule::onSaveCurrentFile() {
    if (!m_currentFile.isEmpty()) {
        m_iniEditor->saveFile(m_currentFile);
        m_statusLabel->setText(tr("Saved: %1").arg(QFileInfo(m_currentFile).fileName()));
    }
}

void PhysicsEditorModule::onExportCar() {
    if (m_currentCar.isEmpty()) {
        m_statusLabel->setText(tr("No car selected"));
        return;
    }
    QString exportPath = QFileDialog::getExistingDirectory(this, tr("Export Car"), m_currentCar);
    if (exportPath.isEmpty()) return;

    QString carName = QFileInfo(m_currentCar).fileName();
    QString destDir = exportPath + "/" + carName;

    if (QFileInfo::exists(destDir)) {
        QMessageBox::StandardButton reply = QMessageBox::question(this,
            tr("Export Car"), tr("Destination already exists. Overwrite?"),
            QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) return;
        QDir(destDir).removeRecursively();
    }

    QDir srcDir(m_currentCar);
    int copied = 0;
    QDirIterator it(m_currentCar, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QString relPath = srcDir.relativeFilePath(it.filePath());
        QString destFile = destDir + "/" + relPath;
        QDir().mkpath(QFileInfo(destFile).absolutePath());
        if (QFile::copy(it.filePath(), destFile)) copied++;
    }

    m_statusLabel->setText(tr("Exported %1 files to: %2").arg(copied).arg(destDir));
}

void PhysicsEditorModule::onImportCar() {
    QString carPath = QFileDialog::getExistingDirectory(this, tr("Select Car Folder"));
    if (!carPath.isEmpty()) {
        onCarSelected(carPath);
        m_statusLabel->setText(tr("Imported: %1").arg(QFileInfo(carPath).fileName()));
    }
}

bool PhysicsEditorModule::saveCurrentIni() {
    if (m_currentFile.isEmpty()) {
        m_statusLabel->setText(tr("No file to save"));
        return false;
    }

    if (!m_iniEditor) {
        m_statusLabel->setText(tr("No editor available"));
        return false;
    }

    bool ok = m_iniEditor->saveFile(m_currentFile);
    if (ok) {
        m_statusLabel->setText(tr("Saved: %1").arg(QFileInfo(m_currentFile).fileName()));
    } else {
        m_statusLabel->setText(tr("Failed to save: %1").arg(QFileInfo(m_currentFile).fileName()));
    }
    return ok;
}

void PhysicsEditorModule::shutdown() {
    if (m_contentStack) {
        m_contentStack->setCurrentIndex(0);
    }
}

void PhysicsEditorModule::populateFileTree(const QString& carFolder) {
    m_fileTree->setRootPath(carFolder + "/data");
}

bool PhysicsEditorModule::loadCarIniFiles(const QString& carFolder) {
    QDir dataDir(carFolder + "/data");
    if (!dataDir.exists()) {
        m_statusLabel->setText(tr("No data folder found"));
        return false;
    }

    QStringList iniFiles = dataDir.entryList(QStringList() << "*.ini", QDir::Files);
    if (iniFiles.isEmpty()) {
        m_statusLabel->setText(tr("No INI files found in data folder"));
        return false;
    }

    QString mainIni = carFolder + "/data/car.ini";
    if (QFile::exists(mainIni)) {
        m_iniEditor->loadFile(mainIni);
        m_statusLabel->setText(tr("Loaded: %1").arg(QFileInfo(mainIni).fileName()));
    } else if (!iniFiles.isEmpty()) {
        QString firstIni = dataDir.absoluteFilePath(iniFiles.first());
        m_iniEditor->loadFile(firstIni);
        m_statusLabel->setText(tr("Loaded: %1").arg(iniFiles.first()));
    }

    return true;
}

void PhysicsEditorModule::updateWindowTitle() {
    QString title = tr("Physics Editor");
    if (!m_currentCar.isEmpty()) {
        title += tr(" - %1").arg(QFileInfo(m_currentCar).fileName());
    }
    setWindowTitle(title);
}

double PhysicsEditorModule::estimateLapTime(double trackLengthM, double avgCornerSpeedKmh,
                                             double avgStraightSpeedKmh, double accelMs2,
                                             double brakeDecelMs2, int cornerCount) {
    if (trackLengthM <= 0 || cornerCount <= 0) return 0;

    double avgCornerSpeedMs = avgCornerSpeedKmh / 3.6;
    double avgStraightSpeedMs = avgStraightSpeedKmh / 3.6;

    double straightLengthM = trackLengthM * 0.6;
    double cornerLengthM = trackLengthM * 0.4;

    double straightTime = straightLengthM / ((avgCornerSpeedMs + avgStraightSpeedMs) / 2.0);

    double cornerTime = cornerLengthM / (cornerCount * avgCornerSpeedMs);

    double accelTime = (avgStraightSpeedMs - avgCornerSpeedMs) / accelMs2;
    double brakeTime = (avgStraightSpeedMs - avgCornerSpeedMs) / brakeDecelMs2;
    double transitionTime = (accelTime + brakeTime) * cornerCount;

    return straightTime + cornerTime + transitionTime;
}

} // namespace ks