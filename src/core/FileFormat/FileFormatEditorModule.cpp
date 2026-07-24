#include "FileFormatEditorModule.h"
#include "FormatConverter.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QTreeWidgetItem>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QStandardPaths>
#include <QMenu>
#include <QApplication>
#include <QThread>

namespace ks {
namespace fileformat {

FileFormatEditorModule::FileFormatEditorModule(QWidget* parent)
    : ModuleGuiBase(parent)
    , m_tabWidget(nullptr)
    , m_converterTab(nullptr)
    , m_sourceFormatCombo(nullptr)
    , m_targetFormatCombo(nullptr)
    , m_convertBtn(nullptr)
    , m_detectBtn(nullptr)
    , m_sourcePathEdit(nullptr)
    , m_targetPathEdit(nullptr)
    , m_preserveStructureCheck(nullptr)
    , m_generateLODCheck(nullptr)
    , m_validateOutputCheck(nullptr)
    , m_conversionProgress(nullptr)
    , m_conversionStatusLabel(nullptr)
    , m_validatorTab(nullptr)
    , m_validateBtn(nullptr)
    , m_validationOutput(nullptr)
    , m_validationResultLabel(nullptr)
    , m_validationProgress(nullptr)
    , m_batchTab(nullptr)
    , m_batchConvertBtn(nullptr)
    , m_batchTable(nullptr)
    , m_batchProgress(nullptr)
    , m_batchStatusLabel(nullptr)
    , m_formatBrowserTab(nullptr)
    , m_formatTree(nullptr)
    , m_formatInfoLabel(nullptr)
{
    setObjectName("FileFormatEditorModule");
}

bool FileFormatEditorModule::initialize() {
    if (m_uiBuilt) return true;
    ModuleGuiBase::initialize();
    return true;
}

void FileFormatEditorModule::shutdown() {
    m_uiBuilt = false;
}

void FileFormatEditorModule::importFile(const QString& filePath) {
    if (filePath.isEmpty()) return;
    QFileInfo fi(filePath);
    QString suffix = fi.suffix().toLower();
    log(QString("Importing format file: %1 (.%2)").arg(filePath, suffix));
    m_sourcePathEdit->setText(filePath);
    onDetectFormat();
}

void FileFormatEditorModule::exportFile(const QString& filePath) {
    if (filePath.isEmpty()) return;
    QFileInfo fi(filePath);
    log(QString("Exporting to format: %1").arg(filePath));
    m_targetPathEdit->setText(filePath);
}

void FileFormatEditorModule::onActivation() {}
void FileFormatEditorModule::onDeactivation() {}

void FileFormatEditorModule::buildUI() {
    m_tabWidget = new QTabWidget();

    setupConverterTab();
    setupValidatorTab();
    setupBatchTab();
    setupFormatBrowserTab();

    m_mainLayout->addWidget(m_tabWidget);
    m_mainLayout->addWidget(m_logOutput);
}

void FileFormatEditorModule::setupConverterTab() {
    m_converterTab = new QWidget();
    auto* layout = new QVBoxLayout(m_converterTab);

    auto* inputGroup = createGroupBox("Input");
    auto* inputLayout = new QFormLayout(inputGroup);
    m_sourcePathEdit = new QLineEdit();
    m_sourcePathEdit->setPlaceholderText("Select source file...");
    auto* browseBtn = createButton("Browse...");
    auto* inputRow = new QHBoxLayout();
    inputRow->addWidget(m_sourcePathEdit);
    inputRow->addWidget(browseBtn);
    inputLayout->addRow("Source:", inputRow);
    m_sourceFormatCombo = createComboBox({"Auto-Detect", "glTF 2.0 (.gltf/.glb)", "FBX (.fbx)", "OBJ (.obj)", "Collada (.dae)", "STL (.stl)", "PLY (.ply)", "KN5 (.kn5)", "USD (.usd)", "Alembic (.abc)"});
    inputLayout->addRow("Source Format:", m_sourceFormatCombo);
    layout->addWidget(inputGroup);

    auto* outputGroup = createGroupBox("Output");
    auto* outputLayout = new QFormLayout(outputGroup);
    m_targetPathEdit = new QLineEdit();
    m_targetPathEdit->setPlaceholderText("Select output path...");
    auto* browseOutBtn = createButton("Browse...");
    auto* outputRow = new QHBoxLayout();
    outputRow->addWidget(m_targetPathEdit);
    outputRow->addWidget(browseOutBtn);
    outputLayout->addRow("Target:", outputRow);
    m_targetFormatCombo = createComboBox({"glTF 2.0 (.gltf/.glb)", "FBX (.fbx)", "OBJ (.obj)", "Collada (.dae)", "STL (.stl)", "PLY (.ply)", "KN5 (.kn5)", "USD (.usd)"});
    outputLayout->addRow("Target Format:", m_targetFormatCombo);
    layout->addWidget(outputGroup);

    auto* optionsGroup = createGroupBox("Options");
    auto* optionsLayout = new QVBoxLayout(optionsGroup);
    m_preserveStructureCheck = createCheckBox("Preserve scene structure", true);
    m_generateLODCheck = createCheckBox("Generate LODs");
    m_validateOutputCheck = createCheckBox("Validate output", true);
    optionsLayout->addWidget(m_preserveStructureCheck);
    optionsLayout->addWidget(m_generateLODCheck);
    optionsLayout->addWidget(m_validateOutputCheck);
    layout->addWidget(optionsGroup);

    auto* btnLayout = new QHBoxLayout();
    m_detectBtn = createButton("Detect Format");
    m_convertBtn = createButton("Convert");
    btnLayout->addWidget(m_detectBtn);
    btnLayout->addWidget(m_convertBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    m_conversionProgress = new QProgressBar();
    m_conversionProgress->setVisible(false);
    layout->addWidget(m_conversionProgress);

    m_conversionStatusLabel = createLabel("");
    layout->addWidget(m_conversionStatusLabel);

    layout->addStretch();

    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString path = selectFile("Select Source File", "All Supported (*.gltf *.glb *.fbx *.obj *.dae *.stl *.ply *.kn5 *.usd);;All Files (*)");
        if (!path.isEmpty()) m_sourcePathEdit->setText(path);
    });
    connect(browseOutBtn, &QPushButton::clicked, this, [this]() {
        QString path = selectFile("Select Output File", "All Files (*)");
        if (!path.isEmpty()) m_targetPathEdit->setText(path);
    });
    connect(m_convertBtn, &QPushButton::clicked, this, &FileFormatEditorModule::onConvertFormat);
    connect(m_detectBtn, &QPushButton::clicked, this, &FileFormatEditorModule::onDetectFormat);
    connect(m_preserveStructureCheck, &QCheckBox::toggled, this, &FileFormatEditorModule::onConversionOptionsChanged);
    connect(m_generateLODCheck, &QCheckBox::toggled, this, &FileFormatEditorModule::onConversionOptionsChanged);
    connect(m_validateOutputCheck, &QCheckBox::toggled, this, &FileFormatEditorModule::onConversionOptionsChanged);

    m_tabWidget->addTab(m_converterTab, "Converter");
}

void FileFormatEditorModule::setupValidatorTab() {
    m_validatorTab = new QWidget();
    auto* layout = new QVBoxLayout(m_validatorTab);

    auto* btnLayout = new QHBoxLayout();
    m_validateBtn = createButton("Validate File");
    btnLayout->addWidget(m_validateBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    m_validationProgress = new QProgressBar();
    m_validationProgress->setVisible(false);
    layout->addWidget(m_validationProgress);

    m_validationResultLabel = createLabel("Select a file to validate");
    layout->addWidget(m_validationResultLabel);

    m_validationOutput = new QTextEdit();
    m_validationOutput->setReadOnly(true);
    m_validationOutput->setPlaceholderText("Validation results...");
    layout->addWidget(m_validationOutput);

    connect(m_validateBtn, &QPushButton::clicked, this, &FileFormatEditorModule::onValidateFile);

    m_tabWidget->addTab(m_validatorTab, "Validator");
}

void FileFormatEditorModule::setupBatchTab() {
    m_batchTab = new QWidget();
    auto* layout = new QVBoxLayout(m_batchTab);

    auto* btnLayout = new QHBoxLayout();
    m_batchConvertBtn = createButton("Batch Convert");
    auto* addBtn = createButton("Add Files");
    auto* removeBtn = createButton("Remove");
    btnLayout->addWidget(m_batchConvertBtn);
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(removeBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    m_batchTable = new QTableWidget(0, 4);
    m_batchTable->setHorizontalHeaderLabels({"File", "Source Format", "Target Format", "Status"});
    m_batchTable->horizontalHeader()->setStretchLastSection(true);
    m_batchTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_batchTable->setAlternatingRowColors(true);
    layout->addWidget(m_batchTable);

    m_batchProgress = new QProgressBar();
    m_batchProgress->setVisible(false);
    layout->addWidget(m_batchProgress);

    m_batchStatusLabel = createLabel("");
    layout->addWidget(m_batchStatusLabel);

    connect(m_batchConvertBtn, &QPushButton::clicked, this, &FileFormatEditorModule::onBatchConvert);
    connect(addBtn, &QPushButton::clicked, this, [this]() {
        QStringList files = selectFiles("Add Files for Batch Conversion", "All Supported (*.gltf *.glb *.fbx *.obj *.dae *.stl *.ply *.kn5)");
        for (const auto& f : files) {
            int row = m_batchTable->rowCount();
            m_batchTable->insertRow(row);
            m_batchTable->setItem(row, 0, new QTableWidgetItem(QFileInfo(f).fileName()));
            m_batchTable->setItem(row, 1, new QTableWidgetItem("Auto"));
            m_batchTable->setItem(row, 2, new QTableWidgetItem(m_targetFormatCombo->currentText()));
            m_batchTable->setItem(row, 3, new QTableWidgetItem("Pending"));
        }
    });
    connect(removeBtn, &QPushButton::clicked, this, [this]() {
        int row = m_batchTable->currentRow();
        if (row >= 0) m_batchTable->removeRow(row);
    });

    m_tabWidget->addTab(m_batchTab, "Batch Convert");
}

void FileFormatEditorModule::setupFormatBrowserTab() {
    m_formatBrowserTab = new QWidget();
    auto* layout = new QVBoxLayout(m_formatBrowserTab);

    auto* splitter = createSplitter(Qt::Horizontal);
    m_formatTree = createTreeWidget({"Format", "Extension", "Version", "Status"});
    splitter->addWidget(m_formatTree);

    m_formatInfoLabel = createLabel("Select a format to view details");
    splitter->addWidget(m_formatInfoLabel);

    layout->addWidget(splitter);

    connect(m_formatTree, &QTreeWidget::itemClicked, this, &FileFormatEditorModule::onFormatSelected);

    populateFormatTree();
    m_tabWidget->addTab(m_formatBrowserTab, "Format Browser");
}

void FileFormatEditorModule::populateFormatTree() {
    m_formatTree->clear();
    auto* geometry = new QTreeWidgetItem(m_formatTree, {"Geometry Formats", "", "", ""});
    geometry->addChild(new QTreeWidgetItem({"glTF 2.0", ".gltf/.glb", "2.0", "Supported"}));
    geometry->addChild(new QTreeWidgetItem({"FBX", ".fbx", "2020", "Supported"}));
    geometry->addChild(new QTreeWidgetItem({"OBJ", ".obj", "", "Supported"}));
    geometry->addChild(new QTreeWidgetItem({"Collada", ".dae", "1.5", "Supported"}));
    geometry->addChild(new QTreeWidgetItem({"STL", ".stl", "", "Supported"}));
    geometry->addChild(new QTreeWidgetItem({"PLY", ".ply", "", "Supported"}));
    geometry->addChild(new QTreeWidgetItem({"USD", ".usd", "0.8", "Beta"}));

    auto* game = new QTreeWidgetItem(m_formatTree, {"Game Formats", "", "", ""});
    game->addChild(new QTreeWidgetItem({"KN5", ".kn5", "", "Supported"}));
    game->addChild(new QTreeWidgetItem({"CAR", ".car", "", "Supported"}));
    game->addChild(new QTreeWidgetItem({"TRACK", ".track", "", "Supported"}));

    auto* data = new QTreeWidgetItem(m_formatTree, {"Data Formats", "", "", ""});
    data->addChild(new QTreeWidgetItem({"JSON", ".json", "", "Supported"}));
    data->addChild(new QTreeWidgetItem({"INI", ".ini", "", "Supported"}));
    data->addChild(new QTreeWidgetItem({"Alembic", ".abc", "", "Beta"}));

    m_formatTree->expandAll();
}

void FileFormatEditorModule::onFormatSelected(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    if (item && item->childCount() == 0) {
        m_formatInfoLabel->setText(QString("Format: %1\nExtension: %2\nStatus: %3").arg(item->text(0), item->text(1), item->text(3)));
    }
}

void FileFormatEditorModule::onConvertFormat() {
    QString sourcePath = m_sourcePathEdit->text();
    if (sourcePath.isEmpty()) {
        logError("No source file selected");
        return;
    }
    QString targetPath = m_targetPathEdit->text();
    if (targetPath.isEmpty()) {
        QFileInfo fi(sourcePath);
        QString targetExt = m_targetFormatCombo->currentText().section("(", 1, 1).chopped(1);
        if (targetExt.isEmpty()) targetExt = ".glb";
        targetPath = fi.absolutePath() + "/" + fi.completeBaseName() + targetExt;
        m_targetPathEdit->setText(targetPath);
    }
    log(QString("Converting %1 to %2").arg(sourcePath, m_targetFormatCombo->currentText()));
    m_conversionProgress->setVisible(true);
    m_conversionProgress->setRange(0, 100);
    m_conversionProgress->setValue(0);
    m_conversionStatusLabel->setText("Converting...");
    QApplication::processEvents();

    if (QFile::exists(sourcePath)) {
        FormatConverter converter;
        QObject::connect(&converter, &FormatConverter::progressChanged, this, [this](float p) {
            m_conversionProgress->setValue(static_cast<int>(p * 100.0f));
            QApplication::processEvents();
        });
        ConversionOptions opts;
        opts.preserveMaterials = m_preserveStructureCheck->isChecked();
        opts.preserveUVs = true;
        if (converter.convert(sourcePath, targetPath, opts)) {
            m_conversionProgress->setValue(100);
            m_conversionStatusLabel->setText("Conversion complete");
            logSuccess(QString("Converted to: %1").arg(targetPath));
        } else {
            m_conversionStatusLabel->setText("Conversion failed");
            logError("Failed to convert: " + sourcePath + " to " + targetPath);
        }
    } else {
        logError("Source file not found: " + sourcePath);
    }
    m_conversionProgress->setVisible(false);
}

void FileFormatEditorModule::onValidateFile() {
    QString path = selectFile("Select File to Validate", "All Supported (*.gltf *.glb *.fbx *.obj *.dae *.stl *.ply *.kn5 *.json *.ini);;All Files (*)");
    if (!path.isEmpty()) {
        log(QString("Validating file: %1").arg(path));
        m_validationProgress->setVisible(true);
        m_validationProgress->setRange(0, 100);
        m_validationProgress->setValue(0);
        m_validationResultLabel->setText("Validating...");
        QApplication::processEvents();

        QFileInfo fi(path);
        QString suffix = fi.suffix().toLower();
        QString result;

        if (fi.exists()) {
            result = QString("File: %1\nSize: %2 bytes\nFormat: .%3\n").arg(fi.fileName()).arg(fi.size()).arg(suffix);
            m_validationProgress->setValue(30);

            if (suffix == "kn5") {
                QFile f(path);
                if (f.open(QIODevice::ReadOnly)) {
                    QByteArray magic = f.read(4);
                    f.close();
                    if (magic.size() == 4) {
                        quint32 magicVal;
                        memcpy(&magicVal, magic.constData(), 4);
                        result += QString("KN5 Magic: 0x%1\n").arg(magicVal, 8, 16, QChar('0'));
                        result += magicVal == 0x354E4B ? "Status: Valid KN5 header\n" : "Status: Invalid KN5 header\n";
                    }
                }
            } else if (suffix == "glb") {
                QFile f(path);
                if (f.open(QIODevice::ReadOnly)) {
                    QByteArray magic = f.read(4);
                    f.close();
                    result += magic == QByteArray("glTF") ? "Status: Valid glTF Binary header\n" : "Status: Invalid glTF header\n";
                }
            } else if (suffix == "obj" || suffix == "stl" || suffix == "ply") {
                result += QString("Status: %1 format detected, file size %2 bytes\n").arg(suffix.toUpper()).arg(fi.size());
            } else if (suffix == "json" || suffix == "ini") {
                result += QString("Status: %1 configuration file\n").arg(suffix.toUpper());
            } else {
                result += "Status: Unknown format - basic file check passed\n";
            }

            m_validationProgress->setValue(100);
            m_validationResultLabel->setText("Validation passed");
            logSuccess("File validation passed");
        } else {
            result = "Error: File not found\n";
            m_validationResultLabel->setText("Validation failed");
            logError("File validation failed: file not found");
        }
        m_validationOutput->setPlainText(result);
        m_validationProgress->setVisible(false);
    }
}

void FileFormatEditorModule::onDetectFormat() {
    QString path = m_sourcePathEdit->text();
    if (!path.isEmpty()) {
        QFileInfo fi(path);
        QString suffix = fi.suffix().toLower();
        for (int i = 1; i < m_sourceFormatCombo->count(); i++) {
            if (m_sourceFormatCombo->itemText(i).contains(suffix, Qt::CaseInsensitive)) {
                m_sourceFormatCombo->setCurrentIndex(i);
                log(QString("Detected format: %1").arg(m_sourceFormatCombo->currentText()));
                return;
            }
        }
        logWarning(QString("Could not detect format for: %1").arg(suffix));
    }
}

void FileFormatEditorModule::onBatchConvert() {
    if (m_batchTable->rowCount() == 0) {
        logError("No files in batch queue");
        return;
    }
    log(QString("Starting batch conversion of %1 files").arg(m_batchTable->rowCount()));
    m_batchProgress->setVisible(true);
    m_batchProgress->setRange(0, m_batchTable->rowCount());
    m_batchStatusLabel->setText("Processing...");
    QApplication::processEvents();

    FormatConverter converter;
    ConversionOptions opts;
    opts.preserveMaterials = m_preserveStructureCheck->isChecked();

    for (int i = 0; i < m_batchTable->rowCount(); ++i) {
        QString fileName = m_batchTable->item(i, 0) ? m_batchTable->item(i, 0)->text() : QString();
        if (fileName.isEmpty()) continue;
        QString sourceDir = QFileInfo(m_sourcePathEdit->text()).absolutePath();
        QString sourcePath = sourceDir + "/" + fileName;
        QString targetDir = QFileInfo(m_targetPathEdit->text()).absolutePath();
        QString targetExt = m_targetFormatCombo->currentText().section("(", 1, 1).chopped(1);
        QString targetPath = targetDir + "/" + QFileInfo(fileName).completeBaseName() + targetExt;

        m_batchTable->setItem(i, 3, new QTableWidgetItem("Converting..."));
        QApplication::processEvents();

        if (converter.convert(sourcePath, targetPath, opts)) {
            m_batchTable->setItem(i, 3, new QTableWidgetItem("Done"));
        } else {
            m_batchTable->setItem(i, 3, new QTableWidgetItem("Failed"));
            logError(QString("Failed to convert: %1").arg(fileName));
        }
        m_batchProgress->setValue(i + 1);
        QApplication::processEvents();
    }
    m_batchStatusLabel->setText("Batch conversion complete");
    m_batchProgress->setVisible(false);
    logSuccess("Batch conversion completed");
}

void FileFormatEditorModule::onConversionOptionsChanged() {
    log(QString("Options: preserve=%1, LOD=%2, validate=%3")
        .arg(m_preserveStructureCheck->isChecked() ? "yes" : "no")
        .arg(m_generateLODCheck->isChecked() ? "yes" : "no")
        .arg(m_validateOutputCheck->isChecked() ? "yes" : "no"));
}

} // namespace fileformat
} // namespace ks

#include "FileFormatEditorModule.moc"
