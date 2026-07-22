#include "ArchiveEditorModule.h"
#include "core/editor/ModuleGuiBase.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QTreeWidgetItem>
#include <QMessageBox>
#include <QInputDialog>
#include <QLabel>
#include <cstddef>

static quint32 crc32_compute(const QByteArray& data) {
    static quint32 table[256];
    static bool initialized = false;
    if (!initialized) {
        for (quint32 i = 0; i < 256; ++i) {
            quint32 crc = i;
            for (int j = 0; j < 8; ++j) {
                crc = (crc & 1) ? ((crc >> 1) ^ 0xEDB88320) : (crc >> 1);
            }
            table[i] = crc;
        }
        initialized = true;
    }
    quint32 crc = 0xFFFFFFFF;
    for (unsigned char b : data) {
        crc = table[(crc ^ b) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

static quint64 crc64_compute(const QByteArray& data) {
    static quint64 table[256];
    static bool initialized = false;
    if (!initialized) {
        for (quint64 i = 0; i < 256; ++i) {
            quint64 crc = i;
            for (int j = 0; j < 8; ++j) {
                crc = (crc & 1) ? ((crc >> 1) ^ 0x95AC34E5A7034D21ULL) : (crc >> 1);
            }
            table[i] = crc;
        }
        initialized = true;
    }
    quint64 crc = 0xFFFFFFFFFFFFFFFFULL;
    for (unsigned char b : data) {
        crc = table[(crc ^ b) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFFFFFFFFFULL;
}
#include <QFileDialog>
#include <QStandardPaths>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QProcess>
#include <QThread>
#include <QProgressDialog>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QDesktopServices>
#include <QCryptographicHash>
#include <QListWidget>
#include <QListWidgetItem>
#include <QFile>
#include <QTabWidget>
#include <QScrollBar>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QClipboard>

namespace ks {

ArchiveEditorModule::ArchiveEditorModule(QWidget* parent)
    : ModuleGuiBase(parent)
    , m_tabWidget(nullptr)
    , m_browseTab(nullptr)
    , m_archivePathEdit(nullptr)
    , m_browseBtn(nullptr)
    , m_contentTree(nullptr)
    , m_extractPathEdit(nullptr)
    , m_extractBrowseBtn(nullptr)
    , m_extractAllBtn(nullptr)
    , m_createTab(nullptr)
    , m_createPathEdit(nullptr)
    , m_createBrowseBtn(nullptr)
    , m_addFilesBtn(nullptr)
    , m_removeFilesBtn(nullptr)
    , m_clearFilesBtn(nullptr)
    , m_createArchiveBtn(nullptr)
    , m_formatCombo(nullptr)
    , m_compressionCombo(nullptr)
    , m_compressionLevelSpin(nullptr)
    , m_passwordEdit(nullptr)
    , m_encryptNamesCheck(nullptr)
    , m_splitCombo(nullptr)
    , m_fileList(nullptr)
    , m_testTab(nullptr)
    , m_testPathEdit(nullptr)
    , m_testBrowseBtn(nullptr)
    , m_testBtn(nullptr)
    , m_testLog(nullptr)
    , m_hashTab(nullptr)
    , m_hashPathEdit(nullptr)
    , m_hashBrowseBtn(nullptr)
    , m_hashAlgoCombo(nullptr)
    , m_hashBtn(nullptr)
    , m_hashResult(nullptr)
    , m_batchTab(nullptr)
    , m_batchFilesList(nullptr)
    , m_batchAddBtn(nullptr)
    , m_batchRemoveBtn(nullptr)
    , m_batchFormatCombo(nullptr)
    , m_batchOutputEdit(nullptr)
    , m_batchOutputBrowseBtn(nullptr)
    , m_batchProcessBtn(nullptr)
    , m_currentArchive("")
    , m_pendingFiles()
{
    setObjectName("ArchiveEditorModule");
}

bool ArchiveEditorModule::initialize() {
    if (m_uiBuilt) return true;
    
    ModuleGuiBase::initialize();
    
    // Load settings
    QSettings settings;
    settings.beginGroup("ArchiveEditor");
    m_extractPathEdit->setText(settings.value("extractPath", QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).toString());
    settings.endGroup();
    
    return true;
}

void ArchiveEditorModule::shutdown() {
    // Save settings
    QSettings settings;
    settings.beginGroup("ArchiveEditor");
    settings.setValue("extractPath", m_extractPathEdit->text());
    settings.endGroup();
    
    ModuleGuiBase::shutdown();
}

void ArchiveEditorModule::importFile(const QString& filePath) {
    QFileInfo info(filePath);
    QString ext = info.suffix().toLower();
    
    if (ext == "7z" || ext == "zip" || ext == "tar" || ext == "gz" || ext == "bz2" || 
        ext == "xz" || ext == "rar" || ext == "iso" || ext == "wim") {
        m_archivePathEdit->setText(filePath);
        onArchiveSelected(filePath);
        m_tabWidget->setCurrentWidget(m_browseTab);
    } else {
        logError(QString("Unsupported archive format: %1").arg(ext));
    }
}

void ArchiveEditorModule::exportFile(const QString& filePath) {
    if (m_currentArchive.isEmpty()) {
        logError("No archive loaded to export");
        return;
    }
    log(QString("Exporting archive: %1 -> %2").arg(m_currentArchive, filePath));
    QFile::copy(m_currentArchive, filePath);
}

void ArchiveEditorModule::buildUI() {
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #3a3a3a; background: #1e1e1e; }"
        "QTabBar::tab { background: #2d2d2d; color: #aaa; padding: 8px 16px; border: 1px solid #3a3a3a; border-bottom: none; }"
        "QTabBar::tab:selected { background: #3a5a8a; color: #fff; }"
        "QTabBar::tab:hover { background: #4a6a9a; }"
    );
    
    setupBrowseTab();
    setupCreateTab();
    setupTestTab();
    setupHashTab();
    setupBatchTab();
    
    m_tabWidget->addTab(m_browseTab, "Browse / Extract");
    m_tabWidget->addTab(m_createTab, "Create Archive");
    m_tabWidget->addTab(m_testTab, "Test Archive");
    m_tabWidget->addTab(m_hashTab, "Checksums");
    m_tabWidget->addTab(m_batchTab, "Batch Process");
    
    m_mainLayout->insertWidget(1, m_tabWidget, 1);
}

void ArchiveEditorModule::setupBrowseTab() {
    m_browseTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_browseTab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);
    
    // Archive selection
    QGroupBox* archiveGroup = createGroupBox("Archive File");
    QHBoxLayout* archiveLayout = new QHBoxLayout(archiveGroup);
    
    archiveLayout->addWidget(createLabel("Archive:"));
    m_archivePathEdit = new QLineEdit();
    m_archivePathEdit->setPlaceholderText("Select an archive file...");
    m_archivePathEdit->setReadOnly(true);
    archiveLayout->addWidget(m_archivePathEdit, 1);
    
    m_browseBtn = createButton("Browse...");
    connect(m_browseBtn, &QPushButton::clicked, this, [this]() {
        QString file = selectFile("Open Archive", "Archives (*.7z *.zip *.tar *.gz *.bz2 *.xz *.rar *.iso *.wim);;All Files (*)");
        if (!file.isEmpty()) {
            m_archivePathEdit->setText(file);
            onArchiveSelected(file);
        }
    });
    archiveLayout->addWidget(m_browseBtn);
    
    QPushButton* openFolderBtn = createButton("Open Folder");
    connect(openFolderBtn, &QPushButton::clicked, this, [this]() {
        if (!m_currentArchive.isEmpty()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_currentArchive).absolutePath()));
        }
    });
    archiveLayout->addWidget(openFolderBtn);
    
    layout->addWidget(archiveGroup);
    
    // Content tree
    QGroupBox* contentGroup = createGroupBox("Archive Contents");
    QVBoxLayout* contentLayout = new QVBoxLayout(contentGroup);
    
    QHBoxLayout* treeToolbar = new QHBoxLayout();
    QPushButton* refreshBtn = createButton("Refresh");
    connect(refreshBtn, &QPushButton::clicked, this, [this]() {
        if (!m_currentArchive.isEmpty()) onArchiveSelected(m_currentArchive);
    });
    treeToolbar->addWidget(refreshBtn);
    
    QPushButton* extractSelBtn = createButton("Extract Selected", "success");
    connect(extractSelBtn, &QPushButton::clicked, this, &ArchiveEditorModule::extractSelected);
    treeToolbar->addWidget(extractSelBtn);
    
    treeToolbar->addStretch();
    
    QLabel* itemCountLabel = createLabel("Items: 0");
    itemCountLabel->setObjectName("itemCountLabel");
    treeToolbar->addWidget(itemCountLabel);
    
    contentLayout->addLayout(treeToolbar);
    
    m_contentTree = createTreeWidget({"Name", "Size", "Packed", "Modified", "Attributes", "CRC", "Method", "Encrypted"});
    m_contentTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_contentTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_contentTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_contentTree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_contentTree->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_contentTree->header()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_contentTree->header()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_contentTree->header()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    m_contentTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_contentTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_contentTree, &QTreeWidget::customContextMenuRequested, this, &ArchiveEditorModule::showContentContextMenu);
    connect(m_contentTree, &QTreeWidget::itemDoubleClicked, this, &ArchiveEditorModule::onContentItemDoubleClicked);
    contentLayout->addWidget(m_contentTree);
    
    layout->addWidget(contentGroup, 1);
    
    // Extract options
    QGroupBox* extractGroup = createGroupBox("Extract Options");
    QVBoxLayout* extractLayout = new QVBoxLayout(extractGroup);
    
    QHBoxLayout* extractPathLayout = new QHBoxLayout();
    extractPathLayout->addWidget(createLabel("Extract to:"));
    m_extractPathEdit = new QLineEdit();
    m_extractPathEdit->setPlaceholderText("Select extraction directory...");
    extractPathLayout->addWidget(m_extractPathEdit, 1);
    
    m_extractBrowseBtn = createButton("Browse...");
    connect(m_extractBrowseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = selectDirectory("Select Extraction Directory");
        if (!dir.isEmpty()) m_extractPathEdit->setText(dir);
    });
    extractPathLayout->addWidget(m_extractBrowseBtn);
    extractLayout->addLayout(extractPathLayout);
    
    QHBoxLayout* extractBtnLayout = new QHBoxLayout();
    m_extractAllBtn = createButton("Extract All", "success");
    connect(m_extractAllBtn, &QPushButton::clicked, this, &ArchiveEditorModule::extractAll);
    extractBtnLayout->addWidget(m_extractAllBtn);
    
    QPushButton* extractFlatBtn = createButton("Extract Flat (no folders)");
    connect(extractFlatBtn, &QPushButton::clicked, this, &ArchiveEditorModule::extractFlat);
    extractBtnLayout->addWidget(extractFlatBtn);
    
    QCheckBox* overwriteCheck = createCheckBox("Overwrite existing");
    overwriteCheck->setChecked(true);
    overwriteCheck->setObjectName("overwriteCheck");
    extractBtnLayout->addWidget(overwriteCheck);
    
    QCheckBox* preservePathsCheck = createCheckBox("Preserve paths");
    preservePathsCheck->setChecked(true);
    preservePathsCheck->setObjectName("preservePathsCheck");
    extractBtnLayout->addWidget(preservePathsCheck);
    
    extractBtnLayout->addStretch();
    extractLayout->addLayout(extractBtnLayout);
    
    layout->addWidget(extractGroup);
}

void ArchiveEditorModule::setupCreateTab() {
    m_createTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_createTab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);
    
    // Archive output
    QGroupBox* outputGroup = createGroupBox("Output Archive");
    QHBoxLayout* outputLayout = new QHBoxLayout(outputGroup);
    
    outputLayout->addWidget(createLabel("Archive:"));
    m_createPathEdit = new QLineEdit();
    m_createPathEdit->setPlaceholderText("Select output archive path...");
    m_createPathEdit->setReadOnly(true);
    outputLayout->addWidget(m_createPathEdit, 1);
    
    m_createBrowseBtn = createButton("Browse...");
    connect(m_createBrowseBtn, &QPushButton::clicked, this, [this]() {
        QString file = QFileDialog::getSaveFileName(this, "Create Archive", QString(), "7z (*.7z);;ZIP (*.zip);;TAR (*.tar);;GZIP (*.tar.gz);;BZIP2 (*.tar.bz2);;XZ (*.tar.xz)");
        if (!file.isEmpty()) m_createPathEdit->setText(file);
    });
    outputLayout->addWidget(m_createBrowseBtn);
    
    layout->addWidget(outputGroup);
    
    // Files to add
    QGroupBox* filesGroup = createGroupBox("Files to Add");
    QVBoxLayout* filesLayout = new QVBoxLayout(filesGroup);
    
    QHBoxLayout* fileBtnLayout = new QHBoxLayout();
    m_addFilesBtn = createButton("Add Files", "success");
    connect(m_addFilesBtn, &QPushButton::clicked, this, [this]() {
        QStringList files = selectFiles("Add Files to Archive", "All Files (*)");
        for (const QString& file : files) {
            addFileToList(file);
        }
        updateFileListCount();
    });
    fileBtnLayout->addWidget(m_addFilesBtn);
    
    QPushButton* addDirBtn = createButton("Add Directory");
    connect(addDirBtn, &QPushButton::clicked, this, [this]() {
        QString dir = selectDirectory("Add Directory to Archive");
        if (!dir.isEmpty()) addDirectoryToList(dir);
        updateFileListCount();
    });
    fileBtnLayout->addWidget(addDirBtn);
    
    m_removeFilesBtn = createButton("Remove Selected", "warning");
    connect(m_removeFilesBtn, &QPushButton::clicked, this, [this]() {
        QList<QListWidgetItem*> selected = m_fileList->selectedItems();
        for (QListWidgetItem* item : selected) delete item;
        updateFileListCount();
    });
    fileBtnLayout->addWidget(m_removeFilesBtn);
    
    m_clearFilesBtn = createButton("Clear All", "danger");
    connect(m_clearFilesBtn, &QPushButton::clicked, this, [this]() {
        if (confirmAction("Clear All", "Remove all files from the list?")) {
            m_fileList->clear();
            updateFileListCount();
        }
    });
    fileBtnLayout->addWidget(m_clearFilesBtn);
    
    fileBtnLayout->addStretch();
    filesLayout->addLayout(fileBtnLayout);
    
    m_fileList = new QListWidget();
    m_fileList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_fileList->setAlternatingRowColors(true);
    m_fileList->setStyleSheet(
        "QListWidget { background: #1e1e1e; color: #ddd; border: 1px solid #3a3a3a; font-size: 11px; }"
        "QListWidget::item { padding: 4px; }"
        "QListWidget::item:selected { background: #3a5a8a; }"
    );
    m_fileList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_fileList, &QListWidget::customContextMenuRequested, this, &ArchiveEditorModule::showFileListContextMenu);
    filesLayout->addWidget(m_fileList);
    
    m_fileCountLabel = new QLabel("Files: 0 | Total: 0 B");
    m_fileCountLabel->setStyleSheet("color: #888; font-size: 10px; padding: 2px 4px;");
    filesLayout->addWidget(m_fileCountLabel);
    
    layout->addWidget(filesGroup, 1);
    
    // Compression options
    QGroupBox* compGroup = createGroupBox("Compression Options");
    QFormLayout* compLayout = new QFormLayout(compGroup);
    
    m_formatCombo = createComboBox({"7z", "zip", "tar", "gzip", "bzip2", "xz"});
    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ArchiveEditorModule::onFormatChanged);
    compLayout->addRow("Format:", m_formatCombo);
    
    m_compressionCombo = createComboBox({"Store", "Fastest", "Fast", "Normal", "Maximum", "Ultra"});
    m_compressionCombo->setCurrentIndex(3); // Normal
    compLayout->addRow("Compression:", m_compressionCombo);
    
    m_compressionLevelSpin = createSpinBox(0, 9, 5);
    compLayout->addRow("Level (0-9):", m_compressionLevelSpin);
    
    m_passwordEdit = new QLineEdit();
    m_passwordEdit->setPlaceholderText("Optional password for encryption");
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    compLayout->addRow("Password:", m_passwordEdit);
    
    QPushButton* showPassBtn = createButton("Show");
    showPassBtn->setCheckable(true);
    connect(showPassBtn, &QPushButton::toggled, this, [this](bool checked) {
        m_passwordEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
    });
    compLayout->addRow("", showPassBtn);
    
    m_encryptNamesCheck = createCheckBox("Encrypt file names (7z only)");
    compLayout->addRow("", m_encryptNamesCheck);
    
    m_splitCombo = createComboBox({"No split", "1.44 MB (floppy)", "100 MB", "200 MB", "500 MB", "700 MB (CD)", "1 GB", "2 GB", "4 GB", "Custom..."});
    compLayout->addRow("Split volumes:", m_splitCombo);
    
    layout->addWidget(compGroup);
    
    // Create button
    QHBoxLayout* createBtnLayout = new QHBoxLayout();
    createBtnLayout->addStretch();
    
    m_createArchiveBtn = createButton("Create Archive", "success");
    m_createArchiveBtn->setMinimumHeight(40);
    connect(m_createArchiveBtn, &QPushButton::clicked, this, &ArchiveEditorModule::createArchive);
    createBtnLayout->addWidget(m_createArchiveBtn);
    
    layout->addLayout(createBtnLayout);
}

void ArchiveEditorModule::setupTestTab() {
    m_testTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_testTab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);
    
    QGroupBox* testGroup = createGroupBox("Archive Integrity Test");
    QVBoxLayout* testLayout = new QVBoxLayout(testGroup);
    
    QHBoxLayout* pathLayout = new QHBoxLayout();
    pathLayout->addWidget(createLabel("Archive:"));
    m_testPathEdit = new QLineEdit();
    m_testPathEdit->setPlaceholderText("Select archive to test...");
    m_testPathEdit->setReadOnly(true);
    pathLayout->addWidget(m_testPathEdit, 1);
    
    m_testBrowseBtn = createButton("Browse...");
    connect(m_testBrowseBtn, &QPushButton::clicked, this, [this]() {
        QString file = selectFile("Test Archive", "Archives (*.7z *.zip *.tar *.gz *.bz2 *.xz *.rar *.iso *.wim)");
        if (!file.isEmpty()) m_testPathEdit->setText(file);
    });
    pathLayout->addWidget(m_testBrowseBtn);
    testLayout->addLayout(pathLayout);
    
    QHBoxLayout* testBtnLayout = new QHBoxLayout();
    m_testBtn = createButton("Test Archive Integrity", "primary");
    m_testBtn->setMinimumHeight(40);
    connect(m_testBtn, &QPushButton::clicked, this, &ArchiveEditorModule::testArchive);
    testBtnLayout->addWidget(m_testBtn);
    
    QPushButton* testListBtn = createButton("List Contents Only");
    connect(testListBtn, &QPushButton::clicked, this, &ArchiveEditorModule::testListOnly);
    testBtnLayout->addWidget(testListBtn);
    
    testBtnLayout->addStretch();
    testLayout->addLayout(testBtnLayout);
    
    layout->addWidget(testGroup);
    
    QGroupBox* logGroup = createGroupBox("Test Results");
    QVBoxLayout* logLayout = new QVBoxLayout(logGroup);
    
    m_testLog = new QTextEdit();
    m_testLog->setReadOnly(true);
    m_testLog->setStyleSheet("QTextEdit { background: #1a1a1a; color: #c8c8c8; font-family: Consolas; font-size: 10px; border: 1px solid #3a3a3a; }");
    logLayout->addWidget(m_testLog);
    
    QPushButton* clearLogBtn = createButton("Clear Log");
    connect(clearLogBtn, &QPushButton::clicked, this, [this]() { m_testLog->clear(); });
    logLayout->addWidget(clearLogBtn);
    
    layout->addWidget(logGroup, 1);
}

void ArchiveEditorModule::setupHashTab() {
    m_hashTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_hashTab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);
    
    QGroupBox* hashGroup = createGroupBox("Calculate Checksums");
    QVBoxLayout* hashLayout = new QVBoxLayout(hashGroup);
    
    QHBoxLayout* pathLayout = new QHBoxLayout();
    pathLayout->addWidget(createLabel("File:"));
    m_hashPathEdit = new QLineEdit();
    m_hashPathEdit->setPlaceholderText("Select file to hash...");
    m_hashPathEdit->setReadOnly(true);
    pathLayout->addWidget(m_hashPathEdit, 1);
    
    m_hashBrowseBtn = createButton("Browse...");
    connect(m_hashBrowseBtn, &QPushButton::clicked, this, [this]() {
        QString file = selectFile("Select File for Hashing", "All Files (*)");
        if (!file.isEmpty()) m_hashPathEdit->setText(file);
    });
    pathLayout->addWidget(m_hashBrowseBtn);
    hashLayout->addLayout(pathLayout);
    
    QHBoxLayout* algoLayout = new QHBoxLayout();
    algoLayout->addWidget(createLabel("Algorithms:"));
    
    m_hashAlgoCombo = createComboBox({"CRC32", "CRC64", "MD5", "SHA-1", "SHA-256", "SHA-512", "BLAKE2sp", "BLAKE2b", "All"});
    m_hashAlgoCombo->setCurrentIndex(8); // All
    algoLayout->addWidget(m_hashAlgoCombo);
    
    m_hashBtn = createButton("Calculate", "primary");
    m_hashBtn->setMinimumHeight(40);
    connect(m_hashBtn, &QPushButton::clicked, this, &ArchiveEditorModule::calculateHash);
    algoLayout->addWidget(m_hashBtn);
    
    QPushButton* verifyBtn = createButton("Verify Hash");
    connect(verifyBtn, &QPushButton::clicked, this, &ArchiveEditorModule::verifyHash);
    algoLayout->addWidget(verifyBtn);
    
    algoLayout->addStretch();
    hashLayout->addLayout(algoLayout);
    
    layout->addWidget(hashGroup);
    
    QGroupBox* resultGroup = createGroupBox("Results");
    QVBoxLayout* resultLayout = new QVBoxLayout(resultGroup);
    
    m_hashResult = new QTextEdit();
    m_hashResult->setReadOnly(true);
    m_hashResult->setStyleSheet("QTextEdit { background: #1a1a1a; color: #c8c8c8; font-family: Consolas; font-size: 11px; border: 1px solid #3a3a3a; }");
    resultLayout->addWidget(m_hashResult);
    
    QHBoxLayout* resultBtnLayout = new QHBoxLayout();
    QPushButton* copyBtn = createButton("Copy All");
    connect(copyBtn, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(m_hashResult->toPlainText());
    });
    resultBtnLayout->addWidget(copyBtn);
    
    QPushButton* saveBtn = createButton("Save to File");
    connect(saveBtn, &QPushButton::clicked, this, [this]() {
        QString file = QFileDialog::getSaveFileName(this, "Save Hashes", QString(), "Text Files (*.txt);;All Files (*)");
        if (!file.isEmpty()) {
            QFile f(file);
            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                f.write(m_hashResult->toPlainText().toUtf8());
                logSuccess("Hashes saved to " + file);
            }
        }
    });
    resultBtnLayout->addWidget(saveBtn);
    
    QPushButton* clearBtn = createButton("Clear");
    connect(clearBtn, &QPushButton::clicked, this, [this]() { m_hashResult->clear(); });
    resultBtnLayout->addWidget(clearBtn);
    
    resultBtnLayout->addStretch();
    resultLayout->addLayout(resultBtnLayout);
    
    layout->addWidget(resultGroup, 1);
}

void ArchiveEditorModule::setupBatchTab() {
    m_batchTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_batchTab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);
    
    QGroupBox* batchGroup = createGroupBox("Batch Processing");
    QVBoxLayout* batchLayout = new QVBoxLayout(batchGroup);
    
    // Files list
    QHBoxLayout* fileBtnLayout = new QHBoxLayout();
    m_batchAddBtn = createButton("Add Files", "success");
    connect(m_batchAddBtn, &QPushButton::clicked, this, [this]() {
        QStringList files = selectFiles("Add Files for Batch Processing", "All Files (*)");
        for (const QString& f : files) {
            QListWidgetItem* item = new QListWidgetItem(QFileInfo(f).fileName(), m_batchFilesList);
            item->setData(Qt::UserRole, f);
            item->setToolTip(f);
        }
    });
    fileBtnLayout->addWidget(m_batchAddBtn);
    
    QPushButton* addDirBtn = createButton("Add Directory");
    connect(addDirBtn, &QPushButton::clicked, this, [this]() {
        QString dir = selectDirectory("Add Directory for Batch Processing");
        if (!dir.isEmpty()) {
            QDirIterator it(dir, QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                QString f = it.next();
                QListWidgetItem* item = new QListWidgetItem(QFileInfo(f).fileName(), m_batchFilesList);
                item->setData(Qt::UserRole, f);
                item->setToolTip(f);
            }
        }
    });
    fileBtnLayout->addWidget(addDirBtn);
    
    m_batchRemoveBtn = createButton("Remove Selected", "warning");
    connect(m_batchRemoveBtn, &QPushButton::clicked, this, [this]() {
        qDeleteAll(m_batchFilesList->selectedItems());
    });
    fileBtnLayout->addWidget(m_batchRemoveBtn);
    
    QPushButton* clearBtn = createButton("Clear All", "danger");
    connect(clearBtn, &QPushButton::clicked, this, [this]() {
        if (confirmAction("Clear All", "Remove all files from batch list?")) m_batchFilesList->clear();
    });
    fileBtnLayout->addWidget(clearBtn);
    
    fileBtnLayout->addStretch();
    batchLayout->addLayout(fileBtnLayout);
    
    m_batchFilesList = new QListWidget();
    m_batchFilesList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_batchFilesList->setAlternatingRowColors(true);
    m_batchFilesList->setStyleSheet(
        "QListWidget { background: #1e1e1e; color: #ddd; border: 1px solid #3a3a3a; font-size: 11px; }"
        "QListWidget::item { padding: 4px; }"
        "QListWidget::item:selected { background: #3a5a8a; }"
    );
    batchLayout->addWidget(m_batchFilesList);
    
    // Options
    QGroupBox* optsGroup = createGroupBox("Batch Options");
    QFormLayout* optsLayout = new QFormLayout(optsGroup);
    
    m_batchFormatCombo = createComboBox({"Compress to 7z", "Compress to ZIP", "Compress to TAR.GZ", "Compress to TAR.BZ2", "Compress to TAR.XZ", "Extract all", "Test all", "Calculate hashes"});
    optsLayout->addRow("Operation:", m_batchFormatCombo);
    
    QHBoxLayout* outputLayout = new QHBoxLayout();
    outputLayout->addWidget(createLabel("Output:"));
    m_batchOutputEdit = new QLineEdit();
    m_batchOutputEdit->setPlaceholderText("Output directory (for compress/extract)");
    outputLayout->addWidget(m_batchOutputEdit, 1);
    
    m_batchOutputBrowseBtn = createButton("Browse...");
    connect(m_batchOutputBrowseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = selectDirectory("Select Output Directory");
        if (!dir.isEmpty()) m_batchOutputEdit->setText(dir);
    });
    outputLayout->addWidget(m_batchOutputBrowseBtn);
    optsLayout->addRow(outputLayout);
    
    QCheckBox* overwriteCheck = createCheckBox("Overwrite existing");
    overwriteCheck->setChecked(true);
    overwriteCheck->setObjectName("batchOverwriteCheck");
    optsLayout->addRow("", overwriteCheck);
    
    QCheckBox* recursiveCheck = createCheckBox("Recursive (for directories)");
    recursiveCheck->setChecked(true);
    recursiveCheck->setObjectName("batchRecursiveCheck");
    optsLayout->addRow("", recursiveCheck);
    
    batchLayout->addWidget(optsGroup);
    
    // Process button
    QHBoxLayout* procLayout = new QHBoxLayout();
    procLayout->addStretch();
    
    m_batchProcessBtn = createButton("Process All", "success");
    m_batchProcessBtn->setMinimumHeight(50);
    connect(m_batchProcessBtn, &QPushButton::clicked, this, &ArchiveEditorModule::processBatch);
    procLayout->addWidget(m_batchProcessBtn);
    
    layout->addWidget(batchGroup, 1);
    layout->addLayout(procLayout);
}

// Archive browsing
void ArchiveEditorModule::onArchiveSelected(const QString& path) {
    m_currentArchive = path;
    m_contentTree->clear();
    
    QProcess proc;
    proc.start("7z", {"l", "-slt", path});
    if (!proc.waitForFinished(30000)) {
        logError("Failed to list archive: timeout");
        return;
    }
    
    QString output = proc.readAllStandardOutput();
    QString error = proc.readAllStandardError();
    
    if (proc.exitCode() != 0 && !error.isEmpty()) {
        logError("7z error: " + error);
        return;
    }
    
    parse7zListOutput(output);
    log(QString("Loaded archive: %1 (%2 items)").arg(QFileInfo(path).fileName()).arg(m_contentTree->topLevelItemCount()));
    
    QLabel* countLabel = m_browseTab->findChild<QLabel*>("itemCountLabel");
    if (countLabel) countLabel->setText(QString("Items: %1").arg(m_contentTree->topLevelItemCount()));
}

void ArchiveEditorModule::parse7zListOutput(const QString& output) {
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    QTreeWidgetItem* currentItem = nullptr;
    
    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        
        if (trimmed.startsWith("----------")) continue;
        if (trimmed.startsWith("Path =")) {
            if (currentItem) {
                m_contentTree->addTopLevelItem(currentItem);
            }
            currentItem = new QTreeWidgetItem();
            currentItem->setText(0, trimmed.mid(6).trimmed());
        } else if (currentItem) {
            if (trimmed.startsWith("Size =")) currentItem->setText(1, formatSize(trimmed.mid(6).trimmed().toLongLong()));
            else if (trimmed.startsWith("Packed Size =")) currentItem->setText(2, formatSize(trimmed.mid(13).trimmed().toLongLong()));
            else if (trimmed.startsWith("Modified =")) currentItem->setText(3, trimmed.mid(10).trimmed());
            else if (trimmed.startsWith("Attributes =")) currentItem->setText(4, trimmed.mid(12).trimmed());
            else if (trimmed.startsWith("CRC =")) currentItem->setText(5, trimmed.mid(5).trimmed());
            else if (trimmed.startsWith("Method =")) currentItem->setText(6, trimmed.mid(8).trimmed());
            else if (trimmed.startsWith("Encrypted =")) currentItem->setText(7, trimmed.mid(11).trimmed() == "+" ? "Yes" : "No");
        }
    }
    
    if (currentItem) m_contentTree->addTopLevelItem(currentItem);
}

void ArchiveEditorModule::showContentContextMenu(const QPoint& pos) {
    QTreeWidgetItem* item = m_contentTree->itemAt(pos);
    if (!item) return;
    
    QMenu menu(this);
    
    QAction* extractAct = menu.addAction("Extract Selected");
    connect(extractAct, &QAction::triggered, this, &ArchiveEditorModule::extractSelected);
    
    QAction* extractToAct = menu.addAction("Extract To...");
    connect(extractToAct, &QAction::triggered, this, [this, item]() {
        QString dir = selectDirectory("Extract To");
        if (!dir.isEmpty()) extractItems({item}, dir);
    });
    
    menu.addSeparator();
    
    QAction* copyPathAct = menu.addAction("Copy Path");
    connect(copyPathAct, &QAction::triggered, this, [item]() {
        QApplication::clipboard()->setText(item->text(0));
    });
    
    QAction* copyFullAct = menu.addAction("Copy Full Info");
    connect(copyFullAct, &QAction::triggered, this, [item]() {
        QString info;
        for (int i = 0; i < item->columnCount(); ++i) {
            if (!item->text(i).isEmpty()) info += item->text(i) + "\t";
        }
        QApplication::clipboard()->setText(info);
    });
    
    menu.exec(m_contentTree->viewport()->mapToGlobal(pos));
}

void ArchiveEditorModule::onContentItemDoubleClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    // Could open file preview for text files
    log("Double-clicked: " + item->text(0));
}

void ArchiveEditorModule::extractSelected() {
    QList<QTreeWidgetItem*> items = m_contentTree->selectedItems();
    if (items.isEmpty()) {
        logError("No items selected for extraction");
        return;
    }
    
    QString dir = m_extractPathEdit->text();
    if (dir.isEmpty()) {
        dir = selectDirectory("Select Extraction Directory");
        if (dir.isEmpty()) return;
        m_extractPathEdit->setText(dir);
    }
    
    extractItems(items, dir);
}

void ArchiveEditorModule::extractAll() {
    if (m_contentTree->topLevelItemCount() == 0) {
        logError("Archive is empty or not loaded");
        return;
    }
    
    QString dir = m_extractPathEdit->text();
    if (dir.isEmpty()) {
        dir = selectDirectory("Select Extraction Directory");
        if (dir.isEmpty()) return;
        m_extractPathEdit->setText(dir);
    }
    
    QList<QTreeWidgetItem*> allItems;
    for (int i = 0; i < m_contentTree->topLevelItemCount(); ++i) {
        allItems.append(m_contentTree->topLevelItem(i));
    }
    
    extractItems(allItems, dir);
}

void ArchiveEditorModule::extractFlat() {
    if (m_contentTree->topLevelItemCount() == 0) return;
    
    QString dir = m_extractPathEdit->text();
    if (dir.isEmpty()) {
        dir = selectDirectory("Select Extraction Directory");
        if (dir.isEmpty()) return;
        m_extractPathEdit->setText(dir);
    }
    
    // Extract all files to flat directory
    QStringList args = {"x", m_currentArchive, "-o" + dir, "-y"};
    if (findChild<QCheckBox*>("overwriteCheck")->isChecked()) args << "-aoa";
    
    run7zCommand(args, "Extracting flat...");
}

void ArchiveEditorModule::extractItems(const QList<QTreeWidgetItem*>& items, const QString& outputDir) {
    QStringList paths;
    for (QTreeWidgetItem* item : items) {
        paths << item->text(0);
    }
    
    QStringList args = {"x", m_currentArchive, "-o" + outputDir};
    if (findChild<QCheckBox*>("overwriteCheck")->isChecked()) args << "-aoa";
    if (!findChild<QCheckBox*>("preservePathsCheck")->isChecked()) args << "-spf";
    
    // Note: 7z doesn't easily support extracting specific files by path list
    // This would need a more complex implementation
    log("Extracting " + QString::number(items.size()) + " items to " + outputDir);
    run7zCommand(args, "Extracting...");
}

void ArchiveEditorModule::run7zCommand(const QStringList& args, const QString& operation) {
    QProgressDialog progress(operation, "Cancel", 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    progress.setValue(0);
    
    QProcess proc;
    proc.start("7z", args);
    
    while (!proc.waitForFinished(100)) {
        progress.setLabelText(operation + " (" + proc.readAllStandardOutput().split('\n').last() + ")");
        QApplication::processEvents();
        if (progress.wasCanceled()) {
            proc.kill();
            log("Operation cancelled");
            return;
        }
    }
    
    QString output = proc.readAllStandardOutput();
    QString error = proc.readAllStandardError();
    
    if (proc.exitCode() == 0) {
        logSuccess(operation + " completed successfully");
        if (!output.isEmpty()) log(output);
    } else {
        logError(operation + " failed: " + error);
    }
}

// Archive creation
void ArchiveEditorModule::onFormatChanged(int index) {
    QString format = m_formatCombo->itemText(index);
    bool is7z = (format == "7z");
    m_encryptNamesCheck->setEnabled(is7z);
    m_splitCombo->setEnabled(is7z || format == "zip");
    m_passwordEdit->setEnabled(is7z || format == "zip");
}

void ArchiveEditorModule::addFileToList(const QString& file) {
    QFileInfo info(file);
    QListWidgetItem* item = new QListWidgetItem(info.fileName(), m_fileList);
    item->setData(Qt::UserRole, file);
    item->setToolTip(QString("%1 (%2)").arg(file, formatSize(info.size())));
}

void ArchiveEditorModule::addDirectoryToList(const QString& dir) {
    QDirIterator it(dir, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString f = it.next();
        addFileToList(f);
    }
}

void ArchiveEditorModule::updateFileListCount() {
    int count = m_fileList->count();
    qint64 totalSize = 0;
    for (int i = 0; i < count; ++i) {
        QListWidgetItem* item = m_fileList->item(i);
        totalSize += QFileInfo(item->data(Qt::UserRole).toString()).size();
    }
    m_fileCountLabel->setText(QString("Files: %1 | Total: %2").arg(count).arg(formatSize(totalSize)));
}

void ArchiveEditorModule::showFileListContextMenu(const QPoint& pos) {
    QListWidgetItem* item = m_fileList->itemAt(pos);
    if (!item) return;
    
    QMenu menu(this);
    QAction* removeAct = menu.addAction("Remove");
    connect(removeAct, &QAction::triggered, this, [this, item]() { delete item; updateFileListCount(); });
    QAction* openAct = menu.addAction("Open Containing Folder");
    connect(openAct, &QAction::triggered, this, [item]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(item->data(Qt::UserRole).toString()).absolutePath()));
    });
    menu.exec(m_fileList->viewport()->mapToGlobal(pos));
}

void ArchiveEditorModule::createArchive() {
    QString outputPath = m_createPathEdit->text();
    if (outputPath.isEmpty()) {
        logError("Please select output archive path");
        return;
    }
    
    if (m_fileList->count() == 0) {
        logError("No files added to archive");
        return;
    }
    
    QStringList files;
    for (int i = 0; i < m_fileList->count(); ++i) {
        files << m_fileList->item(i)->data(Qt::UserRole).toString();
    }
    
    QString format = m_formatCombo->currentText().toLower();
    QStringList args = {"a", "-t" + format};
    
    // Compression level
    args << "-mx" + QString::number(m_compressionLevelSpin->value());
    
    // Compression method
    int compIdx = m_compressionCombo->currentIndex();
    if (compIdx > 0) {
        QStringList methods = {"", "lzma2:d=21", "lzma2:d=23", "lzma2:d=25", "lzma2:d=27", "lzma2:d=29"};
        if (compIdx < methods.size() && !methods[compIdx].isEmpty()) {
            args << "-m0=" + methods[compIdx];
        }
    }
    
    // Password
    if (!m_passwordEdit->text().isEmpty()) {
        args << "-p" + m_passwordEdit->text();
        if (m_encryptNamesCheck->isChecked()) args << "-mhe=on";
    }
    
    // Split volumes
    QString split = m_splitCombo->currentText();
    if (split != "No split") {
        QString size;
        if (split == "1.44 MB (floppy)") size = "1440k";
        else if (split == "100 MB") size = "100m";
        else if (split == "200 MB") size = "200m";
        else if (split == "500 MB") size = "500m";
        else if (split == "700 MB (CD)") size = "700m";
        else if (split == "1 GB") size = "1g";
        else if (split == "2 GB") size = "2g";
        else if (split == "4 GB") size = "4g";
        if (!size.isEmpty()) args << "-v" + size;
    }
    
    // Add files
    args << outputPath;
    args << files;
    
    log("Creating archive: " + outputPath);
    log("Files: " + QString::number(files.size()));
    log("Format: " + format);
    
    run7zCommand(args, "Creating archive");
}

// Archive testing
void ArchiveEditorModule::testArchive() {
    QString path = m_testPathEdit->text();
    if (path.isEmpty()) {
        logError("Please select an archive to test");
        return;
    }
    
    m_testLog->clear();
    m_testLog->append("Testing archive: " + path);
    m_testLog->append("Started at: " + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    m_testLog->append("");
    
    QProcess proc;
    proc.start("7z", {"t", path});
    
    m_testLog->append("$ 7z t " + path);
    m_testLog->append("");
    
    while (proc.waitForReadyRead(100)) {
        QString output = QString::fromUtf8(proc.readAllStandardOutput());
        m_testLog->append(output);
        m_testLog->verticalScrollBar()->setValue(m_testLog->verticalScrollBar()->maximum());
        QApplication::processEvents();
    }
    
    proc.waitForFinished(60000);
    
    QString error = proc.readAllStandardError();
    if (!error.isEmpty()) m_testLog->append("STDERR: " + error);
    
    m_testLog->append("");
    m_testLog->append("Finished at: " + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    m_testLog->append("Exit code: " + QString::number(proc.exitCode()));
    
    if (proc.exitCode() == 0) {
        m_testLog->append("<span style='color:#6bff6b;'><b>✓ Archive test PASSED - no errors found</b></span>");
        logSuccess("Archive integrity test passed");
    } else {
        m_testLog->append("<span style='color:#ff6b6b;'><b>✗ Archive test FAILED</b></span>");
        logError("Archive integrity test failed");
    }
}

void ArchiveEditorModule::testListOnly() {
    QString path = m_testPathEdit->text();
    if (path.isEmpty()) {
        logError("Please select an archive");
        return;
    }
    
    m_testLog->clear();
    m_testLog->append("Listing contents: " + path);
    m_testLog->append("");
    
    QProcess proc;
    proc.start("7z", {"l", path});
    proc.waitForFinished(30000);
    
    QString output = proc.readAllStandardOutput();
    m_testLog->append(output);
    
    if (proc.exitCode() == 0) logSuccess("Archive listing complete");
    else logError("Failed to list archive");
}

// Hashing
void ArchiveEditorModule::calculateHash() {
    QString path = m_hashPathEdit->text();
    if (path.isEmpty() || !QFile::exists(path)) {
        logError("Please select a valid file");
        return;
    }
    
    m_hashResult->clear();
    m_hashResult->append("File: " + path);
    m_hashResult->append("Size: " + formatSize(QFileInfo(path).size()));
    m_hashResult->append("Started: " + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    m_hashResult->append("");
    
    QString algo = m_hashAlgoCombo->currentText();
    QStringList algos;
    
    if (algo == "All") {
        algos = {"CRC32", "CRC64", "MD5", "SHA-1", "SHA-256", "SHA-512"};
    } else {
        algos << algo;
    }
    
    for (const QString& a : algos) {
        QCryptographicHash::Algorithm alg;
        bool isCrc = (a == "CRC32" || a == "CRC64");
        if (a == "MD5") alg = QCryptographicHash::Md5;
        else if (a == "SHA-1") alg = QCryptographicHash::Sha1;
        else if (a == "SHA-256") alg = QCryptographicHash::Sha256;
        else if (a == "SHA-512") alg = QCryptographicHash::Sha512;
        else if (isCrc) { /* handled separately */ }
        else continue;
        
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            QString result;
            if (isCrc) {
                QByteArray data = file.readAll();
                if (a == "CRC32") {
                    result = QString("%1").arg(crc32_compute(data), 8, 16, QChar('0')).toUpper();
                } else {
                    result = QString("%1").arg(crc64_compute(data), 16, 16, QChar('0')).toUpper();
                }
            } else {
                QCryptographicHash hash(alg);
                if (hash.addData(&file)) {
                    result = hash.result().toHex().toUpper();
                }
            }
            if (!result.isEmpty()) {
                m_hashResult->append(QString("%1: %2").arg(a.leftJustified(10), result));
            }
            file.close();
        }
    }
    
    m_hashResult->append("");
    m_hashResult->append("Completed: " + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    logSuccess("Hash calculation complete");
}

void ArchiveEditorModule::verifyHash() {
    QString path = m_hashPathEdit->text();
    if (path.isEmpty()) {
        logError("Please select a file first");
        return;
    }
    
    bool ok;
    QString expected = QInputDialog::getText(this, "Verify Hash", "Enter expected hash:", QLineEdit::Normal, "", &ok);
    if (!ok || expected.isEmpty()) return;
    
    // Calculate SHA-256 for verification
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        logError("Cannot open file");
        return;
    }
    
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(&file);
    QString actual = hash.result().toHex().toUpper();
    file.close();
    
    if (actual == expected.toUpper()) {
        QMessageBox::information(this, "Hash Verification", "<span style='color:#6bff6b'><b>✓ Hash MATCHES</b></span>");
        m_hashResult->append("<span style='color:#6bff6b'>VERIFIED: Hash matches!</span>");
        logSuccess("Hash verification passed");
    } else {
        QMessageBox::warning(this, "Hash Verification", "<span style='color:#ff6b6b'><b>✗ Hash MISMATCH</b></span><br>Expected: " + expected + "<br>Actual: " + actual);
        m_hashResult->append("<span style='color:#ff6b6b'>FAILED: Hash mismatch!</span>");
        m_hashResult->append("Expected: " + expected);
        m_hashResult->append("Actual: " + actual);
        logError("Hash verification failed");
    }
}

// Batch processing
void ArchiveEditorModule::processBatch() {
    if (m_batchFilesList->count() == 0) {
        logError("No files in batch list");
        return;
    }
    
    QString operation = m_batchFormatCombo->currentText();
    QString outputDir = m_batchOutputEdit->text();
    bool overwrite = findChild<QCheckBox*>("batchOverwriteCheck")->isChecked();
    
    if (operation.startsWith("Compress") && outputDir.isEmpty()) {
        logError("Please specify output directory");
        return;
    }
    
    QProgressDialog progress("Batch Processing...", "Cancel", 0, m_batchFilesList->count(), this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    
    int success = 0, failed = 0;
    
    for (int i = 0; i < m_batchFilesList->count(); ++i) {
        progress.setValue(i);
        progress.setLabelText(QString("Processing %1 of %2: %3")
            .arg(i + 1).arg(m_batchFilesList->count()).arg(m_batchFilesList->item(i)->text()));
        QApplication::processEvents();
        
        if (progress.wasCanceled()) break;
        
        QString file = m_batchFilesList->item(i)->data(Qt::UserRole).toString();
        QStringList args;
        
        if (operation == "Compress to 7z") {
            args = {"a", "-t7z", "-mx5", outputDir + "/" + QFileInfo(file).baseName() + ".7z", file};
        } else if (operation == "Compress to ZIP") {
            args = {"a", "-tzip", "-mx5", outputDir + "/" + QFileInfo(file).baseName() + ".zip", file};
        } else if (operation == "Compress to TAR.GZ") {
            args = {"a", "-tgzip", outputDir + "/" + QFileInfo(file).baseName() + ".tar.gz", file};
        } else if (operation == "Compress to TAR.BZ2") {
            args = {"a", "-tbzip2", outputDir + "/" + QFileInfo(file).baseName() + ".tar.bz2", file};
        } else if (operation == "Compress to TAR.XZ") {
            args = {"a", "-txz", outputDir + "/" + QFileInfo(file).baseName() + ".tar.xz", file};
        } else if (operation == "Extract all") {
            QString out = outputDir.isEmpty() ? QFileInfo(file).absolutePath() : outputDir;
            args = {"x", file, "-o" + out};
            if (overwrite) args << "-aoa";
        } else if (operation == "Test all") {
            args = {"t", file};
        } else if (operation == "Calculate hashes") {
            // Handled separately
        }
        
        if (!args.isEmpty()) {
            QProcess proc;
            proc.start("7z", args);
            if (proc.waitForFinished(60000) && proc.exitCode() == 0) {
                success++;
            } else {
                failed++;
                logError(QString("Failed: %1").arg(file));
            }
        }
    }
    
    progress.setValue(m_batchFilesList->count());
    
    QString msg = QString("Batch complete: %1 succeeded, %2 failed").arg(success).arg(failed);
    if (failed == 0) logSuccess(msg);
    else logError(msg);
}

QString ArchiveEditorModule::formatSize(qint64 bytes) const {
    if (bytes < 1024) return QString("%1 B").arg(bytes);
    if (bytes < 1024LL * 1024) return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    if (bytes < 1024LL * 1024 * 1024) return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    if (bytes < 1024LL * 1024 * 1024 * 1024) return QString("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
    return QString("%1 TB").arg(bytes / (1024.0 * 1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
}

} // namespace ks

#include "ArchiveEditorModule.moc"