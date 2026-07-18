#include "PhysicsEditor.h"
#include <cmath>
#include <algorithm>
#include <QSettings>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QAction>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QTextStream>
#include <QFile>
#include <QByteArray>
#include <QDebug>
#include <QHeaderView>
#include <QInputDialog>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMenu>
#include <QContextMenuEvent>
#include <QElapsedTimer>
#include "core/editor/EditorConfig.h"

namespace ks {

// ============================================================================
// IniSyntaxHighlighter
// ============================================================================

IniSyntaxHighlighter::IniSyntaxHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
{
    m_sectionFormat.setForeground(QColor("#4A90E2"));
    m_sectionFormat.setFontWeight(QFont::Bold);

    m_keyFormat.setForeground(QColor("#E8C97D"));

    m_commentFormat.setForeground(QColor("#6A9955"));
    m_commentFormat.setFontItalic(true);

    m_numberFormat.setForeground(QColor("#B5CEA8"));

    m_stringFormat.setForeground(QColor("#CE9178"));

    HighlightingRule rule;

    rule.pattern = QRegularExpression("^\\[.+\\]");
    rule.format  = m_sectionFormat;
    m_rules.append(rule);

    rule.pattern = QRegularExpression("^[^=\\n]+=");
    rule.format  = m_keyFormat;
    m_rules.append(rule);

    rule.pattern = QRegularExpression(";.*$");
    rule.format  = m_commentFormat;
    m_rules.append(rule);

    rule.pattern = QRegularExpression("-?\\b\\d+\\.?\\d*\\b");
    rule.format  = m_numberFormat;
    m_rules.append(rule);

    rule.pattern = QRegularExpression("\"[^\"]*\"");
    rule.format  = m_stringFormat;
    m_rules.append(rule);
}

void IniSyntaxHighlighter::highlightBlock(const QString& text) {
    for (const HighlightingRule& rule : m_rules) {
        QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}

// ============================================================================
// IniEditorWidget
// ============================================================================

IniEditorWidget::IniEditorWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_textEdit = new QTextEdit(this);
    m_textEdit->setFont(QFont("Consolas", 10));
    m_textEdit->setTabStopDistance(40);
    m_highlighter = new IniSyntaxHighlighter(m_textEdit->document());

    m_lineLabel = new QLabel(tr("Line: 1"), this);
    m_statusLabel = new QLabel(tr("Ready"), this);

    auto* toolbar = new QHBoxLayout;
    auto* saveBtn = new QPushButton(tr("Save"), this);
    auto* reloadBtn = new QPushButton(tr("Reload"), this);
    toolbar->addWidget(saveBtn);
    toolbar->addWidget(reloadBtn);
    toolbar->addStretch();
    toolbar->addWidget(m_lineLabel);
    toolbar->addWidget(m_statusLabel);

    layout->addLayout(toolbar);
    layout->addWidget(m_textEdit);

    connect(m_textEdit, &QTextEdit::textChanged, this, &IniEditorWidget::onTextChanged);
    connect(m_textEdit, &QTextEdit::cursorPositionChanged, this, [this] {
        QTextCursor c = m_textEdit->textCursor();
        m_lineLabel->setText(tr("Line: %1").arg(c.blockNumber() + 1));
    });
    connect(saveBtn, &QPushButton::clicked, this, &IniEditorWidget::onSave);
    connect(reloadBtn, &QPushButton::clicked, [this] {
        if (!m_currentFile.isEmpty()) loadFile(m_currentFile);
    });
}

IniEditorWidget::~IniEditorWidget() {
    delete m_highlighter;
}

bool IniEditorWidget::loadFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    QTextStream in(&file);
    m_textEdit->setPlainText(in.readAll());
    file.close();

    m_currentFile = path;
    m_modified = false;
    update();
    emit fileLoaded(path);
    return true;
}

bool IniEditorWidget::saveFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QTextStream out(&file);
    out << m_textEdit->toPlainText();
    file.close();

    m_currentFile = path;
    m_modified = false;
    m_statusLabel->setText(tr("Saved"));
    emit fileSaved(path);
    return true;
}

void IniEditorWidget::setContent(const QString& text) {
    QSignalBlocker b(m_textEdit);
    m_textEdit->setPlainText(text);
    m_modified = false;
}

void IniEditorWidget::onTextChanged() {
    m_modified = true;
    m_statusLabel->setText(m_modified ? tr("Modified") : tr("Ready"));
    emit contentChanged();
}

void IniEditorWidget::onSave() {
    if (m_currentFile.isEmpty()) {
        QString path = QFileDialog::getSaveFileName(this, tr("Save INI"),
            QString(),
            tr("INI files (*.ini);;All files (*.*)"));
        if (path.isEmpty()) return;
        m_currentFile = path;
    }
    saveFile(m_currentFile);
}

// ============================================================================
// FileTreeWidget
// ============================================================================

const QStringList FileTreeWidget::s_carDataExtensions = {
    "*.ini", "*.json", "*.txt", "*.lut"
};

FileTreeWidget::FileTreeWidget(QWidget* parent)
    : QTreeWidget(parent)
{
    setHeaderLabel(tr("Files"));
    setAnimated(true);
    setIndentation(16);
}

void FileTreeWidget::setRootPath(const QString& path) {
    m_rootPath = path;
    refresh();
}

void FileTreeWidget::refresh() {
    clear();
    if (m_rootPath.isEmpty() || !QDir(m_rootPath).exists()) return;

    QDir dir(m_rootPath);
    QFileInfoList entries;

    QDir dataDir(m_rootPath + "/data");
    if (dataDir.exists()) {
        entries = dataDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
    } else {
        entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
    }

    QMap<QString, QTreeWidgetItem*> folders;
    QStringList nameFilters;
    nameFilters << "*.ini" << "*.json" << "*.txt" << "*.lut" << "*.acd" << "*.kn5";

    for (const QFileInfo& info : entries) {
        if (info.isDir()) {
            QString name = info.fileName();
            QTreeWidgetItem* item = new QTreeWidgetItem(this);
            item->setText(0, name);
            item->setData(0, Qt::UserRole, info.absoluteFilePath());
            item->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
            item->setExpanded(false);
            folders[name] = item;
        }
    }

    for (const QFileInfo& info : entries) {
        if (!info.isFile()) continue;
        QString ext = info.suffix().toLower();
        if (!nameFilters.contains("*." + ext)) continue;

        QString parentPath = info.absolutePath();
        QString relPath = QDir(m_rootPath).relativeFilePath(parentPath);
        QTreeWidgetItem* parentItem = invisibleRootItem();

        if (!relPath.isEmpty() && relPath != ".") {
            QString folderName = relPath.split('/').first();
            if (folders.contains(folderName)) {
                parentItem = folders[folderName];
            }
        }

        QTreeWidgetItem* item = new QTreeWidgetItem(parentItem);
        item->setText(0, info.fileName());
        item->setData(0, Qt::UserRole, info.absoluteFilePath());
        item->setData(0, Qt::UserRole + 1, info.suffix().toLower());
        item->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
    }

    connect(this, &QTreeWidget::itemDoubleClicked, this, &FileTreeWidget::onItemDoubleClicked);
}

void FileTreeWidget::onItemDoubleClicked(QTreeWidgetItem* item, int) {
    if (!item) return;
    QString path = item->data(0, Qt::UserRole).toString();
    if (!path.isEmpty()) emit fileSelected(path);
}

// ============================================================================
// CarBrowserWidget
// ============================================================================

CarBrowserWidget::CarBrowserWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    m_searchBox = new QLineEdit(this);
    m_searchBox->setPlaceholderText(tr("Search cars..."));

    m_carList = new QListWidget(this);
    m_carList->setIconSize(QSize(24, 24));

    m_reloadBtn = new QPushButton(tr("Reload"), this);
    m_pathLabel = new QLabel(tr("No folder selected"), this);
    m_pathLabel->setWordWrap(true);
    m_pathLabel->setStyleSheet("color: gray; font-size: 9pt;");

    layout->addWidget(new QLabel(tr("Search:"), this));
    layout->addWidget(m_searchBox);
    layout->addWidget(m_carList);

    auto* btnRow = new QHBoxLayout;
    btnRow->addWidget(m_reloadBtn);
    btnRow->addWidget(new QLabel(tr("Path:"), this));
    layout->addLayout(btnRow);
    layout->addWidget(m_pathLabel);

    connect(m_searchBox, &QLineEdit::textChanged, this, &CarBrowserWidget::onSearchTextChanged);
    connect(m_carList, &QListWidget::itemClicked, this, &CarBrowserWidget::onCarItemClicked);
    connect(m_reloadBtn, &QPushButton::clicked, this, &CarBrowserWidget::onReloadClicked);
}

void CarBrowserWidget::setCarsPath(const QString& path) {
    m_carsPath = path;
    m_pathLabel->setText(path);
    refreshCarList();
}

void CarBrowserWidget::refreshCarList(const QString& filter) {
    m_carList->clear();
    if (m_carsPath.isEmpty() || !QDir(m_carsPath).exists()) return;

    QDir dir(m_carsPath);
    QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    entries.sort();

    for (const QString& name : entries) {
        if (!filter.isEmpty() && !name.contains(filter, Qt::CaseInsensitive)) continue;
        auto* item = new QListWidgetItem(name, m_carList);
        item->setData(Qt::UserRole, name);
    }
}

void CarBrowserWidget::onSearchTextChanged(const QString& text) {
    refreshCarList(text);
}

void CarBrowserWidget::onCarItemClicked(QListWidgetItem* item) {
    if (!item) return;
    QString car = item->data(Qt::UserRole).toString();
    m_currentCar = car;
    emit carSelected(m_carsPath + "/" + car);
}

void CarBrowserWidget::onReloadClicked() {
    refreshCarList(m_searchBox->text());
    if (!m_currentCar.isEmpty())
        emit carReloadRequested(m_carsPath + "/" + m_currentCar);
}

// ============================================================================
// TyresTableWidget
// ============================================================================

TyresTableWidget::TyresTableWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    m_rows = {"FL", "FR", "RL", "RR"};

    m_table = new QTableWidget(4, 5, this);
    m_table->setHorizontalHeaderLabels({tr("Tyre"), tr("Pressure (psi)"), tr("Temp (C)"), tr("Wear %"), tr("Dirt %")});
    m_table->verticalHeader()->setVisible(false);
    m_table->setAlternatingRowColors(true);

    for (int r = 0; r < 4; ++r) {
        auto* nameItem = new QTableWidgetItem(m_rows[r]);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(r, 0, nameItem);
        for (int c = 1; c < 5; ++c) {
            if (!m_table->item(r, c))
                m_table->setItem(r, c, new QTableWidgetItem());
        }
    }

    layout->addWidget(m_table);

    auto* infoLabel = new QLabel(
        tr("Tip: Pressure in psi (cold). Target: 32-33 psi for sport driving."), this);
    infoLabel->setStyleSheet("color: gray; font-size: 9pt;");
    layout->addWidget(infoLabel);

    connect(m_table, &QTableWidget::cellChanged, this, &TyresTableWidget::changed);
}

void TyresTableWidget::loadFromIni(const QMap<QString, QString>& data) {
    auto getVal = [&](const QString& key, double def) {
        return data.value(key, QString::number(def)).toDouble();
    };

    m_table->item(0, 1)->setText(QString::number(getVal("PRESSURE_FL", 32.0), 'f', 1));
    m_table->item(1, 1)->setText(QString::number(getVal("PRESSURE_FR", 32.0), 'f', 1));
    m_table->item(2, 1)->setText(QString::number(getVal("PRESSURE_RL", 32.0), 'f', 1));
    m_table->item(3, 1)->setText(QString::number(getVal("PRESSURE_RR", 32.0), 'f', 1));

    m_table->item(0, 2)->setText(QString::number(getVal("TEMP_FL", 90.0), 'f', 0));
    m_table->item(1, 2)->setText(QString::number(getVal("TEMP_FR", 90.0), 'f', 0));
    m_table->item(2, 2)->setText(QString::number(getVal("TEMP_RL", 90.0), 'f', 0));
    m_table->item(3, 2)->setText(QString::number(getVal("TEMP_RR", 90.0), 'f', 0));
}

void TyresTableWidget::saveToIni(QMap<QString, QString>& data) {
    for (int r = 0; r < 4; ++r) {
        QString prefix = m_rows[r];
        data["PRESSURE_" + prefix] = m_table->item(r, 1)->text();
        data["TEMP_" + prefix] = m_table->item(r, 2)->text();
        data["WEAR_" + prefix] = m_table->item(r, 3)->text();
        data["DIRT_" + prefix] = m_table->item(r, 4)->text();
    }
}

// ============================================================================
// AcdManagerWidget
// ============================================================================

AcdManagerWidget::AcdManagerWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    m_acdStatus = new QLabel(tr("No ACD file detected"), this);
    m_acdStatus->setStyleSheet("font-weight: bold; color: gray;");

    m_extractBtn = new QPushButton(tr("Extract ACD"), this);
    m_repackBtn  = new QPushButton(tr("Repack ACD"), this);
    m_logEdit    = new QTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumHeight(120);
    m_logEdit->setFont(QFont("Consolas", 9));

    auto* btnRow = new QHBoxLayout;
    btnRow->addWidget(m_extractBtn);
    btnRow->addWidget(m_repackBtn);

    layout->addWidget(new QLabel(tr("ACD Archive Manager"), this));
    layout->addWidget(m_acdStatus);
    layout->addLayout(btnRow);
    layout->addWidget(new QLabel(tr("Log:"), this));
    layout->addWidget(m_logEdit);

    connect(m_extractBtn, &QPushButton::clicked, this, &AcdManagerWidget::onExtract);
    connect(m_repackBtn, &QPushButton::clicked, this, &AcdManagerWidget::onRepack);
}

void AcdManagerWidget::setCarPath(const QString& path) {
    m_carPath = path;
    QFileInfo acd(path + "/data/acd_0.accd");
    if (acd.exists()) {
        m_acdStatus->setText(tr("ACD found: %1").arg(acd.fileName()));
        m_acdStatus->setStyleSheet("font-weight: bold; color: green;");
        m_extractBtn->setEnabled(true);
        m_repackBtn->setEnabled(true);
    } else {
        m_acdStatus->setText(tr("No ACD file detected"));
        m_acdStatus->setStyleSheet("font-weight: bold; color: gray;");
        m_extractBtn->setEnabled(false);
        m_repackBtn->setEnabled(false);
    }
}

QString AcdManagerWidget::createKey(const QString& folderName) const {
    QString lower = folderName.toLower();
    int sum = 0;
    for (QChar c : lower) sum += c.unicode();

    int octet1 = ((sum % 256) + 256) % 256;

    int n = lower.length();
    int temp = 0;
    for (int i = 0; i < n; ++i) {
        temp += lower[i].unicode() * (i + 1);
    }
    int octet2 = ((temp % 256) + 256) % 256;

    temp = 0;
    for (int i = 0; i < n; ++i) {
        temp += lower[i].unicode() * (n - i);
    }
    int octet3 = ((temp % 256) + 256) % 256;

    temp = 5763;
    for (int i = 0; i < n; ++i) {
        temp += lower[i].unicode() * (i + 1);
    }
    int octet4 = ((temp % 256) + 256) % 256;

    temp = 66;
    for (int i = 0; i < n; ++i) {
        temp += lower[i].unicode() * (n + i + 1);
    }
    int octet5 = ((temp % 256) + 256) % 256;

    temp = 101;
    for (int i = 0; i < n; ++i) {
        temp += lower[i].unicode() * (n - i + 1);
    }
    int octet6 = ((temp % 256) + 256) % 256;

    temp = 171;
    for (int i = 0; i < n; ++i) {
        temp += lower[i].unicode() * (i + 2);
    }
    int octet7 = ((temp % 256) + 256) % 256;

    temp = 171;
    for (int i = 0; i < n; ++i) {
        temp += lower[i].unicode() * (i + 3);
    }
    int octet8 = ((temp % 256) + 256) % 256;

    return QString("%1-%2-%3-%4-%5-%6-%7-%8")
        .arg(octet1).arg(octet2).arg(octet3).arg(octet4)
        .arg(octet5).arg(octet6).arg(octet7).arg(octet8);
}

QByteArray AcdManagerWidget::decryptAcd(const QByteArray& data, const QString& key) const {
    QStringList parts = key.split('-');
    if (parts.size() != 8) return data;

    QVector<int> keyBytes;
    for (const QString& p : parts) keyBytes.append(p.toInt());

    QByteArray result = data;
    int dataLen = data.size();
    int keyLen = keyBytes.size();

    for (int i = 0; i < dataLen; ++i) {
        int rot = keyBytes[i % keyLen];
        int val = (int)(unsigned char)data[i];
        val = ((val - rot) % 256 + 256) % 256;
        result[i] = (char)val;
    }
    return result;
}

QByteArray AcdManagerWidget::encryptAcd(const QByteArray& data, const QString& key) const {
    QStringList parts = key.split('-');
    if (parts.size() != 8) return data;

    QVector<int> keyBytes;
    for (const QString& p : parts) keyBytes.append(p.toInt());

    QByteArray result = data;
    int dataLen = data.size();
    int keyLen = keyBytes.size();

    for (int i = 0; i < dataLen; ++i) {
        int rot = keyBytes[i % keyLen];
        int val = (int)(unsigned char)data[i];
        val = ((val + rot) % 256 + 256) % 256;
        result[i] = (char)val;
    }
    return result;
}

void AcdManagerWidget::onExtract() {
    if (m_carPath.isEmpty()) return;

    QString folderName = QFileInfo(m_carPath).fileName();
    QString key = createKey(folderName);

    // Try standard data.acd first, then acd_0.accd as fallback
    QString acdPath = m_carPath + "/data/data.acd";
    if (!QFile::exists(acdPath)) {
        acdPath = m_carPath + "/data/acd_0.accd";
    }
    QFile file(acdPath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_logEdit->append(tr("[ERROR] Cannot open ACD file"));
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QByteArray decrypted = decryptAcd(data, key);

    QString outDir = m_carPath + "/data_extracted";
    if (!QDir().mkpath(outDir)) {
        m_logEdit->append(tr("[ERROR] Failed to create directory: %1").arg(outDir));
        return;
    }

    QFile out(outDir + "/decrypted.bin");
    if (out.open(QIODevice::WriteOnly)) {
        out.write(decrypted);
        out.close();
    }

    m_logEdit->append(tr("[OK] ACD extracted to: %1").arg(outDir));
    emit acdExtracted(m_carPath);
}

void AcdManagerWidget::onRepack() {
    if (m_carPath.isEmpty()) return;

    QString folderName = QFileInfo(m_carPath).fileName();
    QString key = createKey(folderName);

    QString inFile = m_carPath + "/data_extracted/decrypted.bin";
    QFile file(inFile);
    if (!file.open(QIODevice::ReadOnly)) {
        m_logEdit->append(tr("[ERROR] Cannot open extracted file"));
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QByteArray encrypted = encryptAcd(data, key);

    QString acdPath = m_carPath + "/data/acd_0.accd";
    QFile out(acdPath);
    if (out.open(QIODevice::WriteOnly)) {
        out.write(encrypted);
        out.close();
        m_logEdit->append(tr("[OK] ACD repacked"));
    } else {
        m_logEdit->append(tr("[ERROR] Cannot write ACD file"));
    }
}

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
    delete m_carBrowser; m_carBrowser = nullptr;
    delete m_fileTree; m_fileTree = nullptr;
    delete m_iniEditor; m_iniEditor = nullptr;
    delete m_tyresEditor; m_tyresEditor = nullptr;
    delete m_acdManager; m_acdManager = nullptr;
    delete m_lutEditor; m_lutEditor = nullptr;
    delete m_engineEditor; m_engineEditor = nullptr;
    delete m_telemetryWidget; m_telemetryWidget = nullptr;
    delete m_setupCompare; m_setupCompare = nullptr;
    delete m_acdBrowser; m_acdBrowser = nullptr;
    delete m_suspGeometry; m_suspGeometry = nullptr;
    delete m_ffbPreview; m_ffbPreview = nullptr;
    delete m_validator; m_validator = nullptr;
    delete m_tyreTempModel; m_tyreTempModel = nullptr;
    m_statusLabel->setText(tr("Physics Editor shutdown complete"));
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

// ============================================================================
// LutCurveWidget
// ============================================================================

LutCurveWidget::LutCurveWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);

    auto* topBar = new QHBoxLayout;
    m_lutSelector = new QComboBox(this);
    m_infoLabel = new QLabel(tr("No LUT loaded"), this);
    m_infoLabel->setStyleSheet("color: gray;");
    topBar->addWidget(new QLabel("LUT:", this));
    topBar->addWidget(m_lutSelector, 1);
    topBar->addWidget(m_infoLabel);

    m_chart = new QChart();
    m_chart->setBackgroundVisible(false);
    m_chart->legend()->hide();

    m_chartView = new QChartView(m_chart, this);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumHeight(220);

    layout->addLayout(topBar);
    layout->addWidget(m_chartView, 1);

    connect(m_lutSelector, QOverload<const QString&>::of(&QComboBox::currentTextChanged), this, [this](const QString& text) {
        if (!text.isEmpty() && !m_currentLut.isEmpty()) {
            QString path = QFileInfo(m_currentLut).absolutePath() + "/" + text;
            loadLutFile(path);
        }
    });
}

bool LutCurveWidget::loadLutFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_infoLabel->setText("Cannot open: " + QFileInfo(path).fileName());
        return false;
    }

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    m_xData.clear();
    m_yData.clear();

    QStringList lines = content.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith('#') || trimmed.startsWith(';')) continue;

        if (trimmed.contains('=')) {
            int eqPos = trimmed.indexOf('=');
            bool okX, okY;
            double x = trimmed.left(eqPos).toDouble(&okX);
            double y = trimmed.mid(eqPos + 1).toDouble(&okY);
            if (okX && okY) {
                m_xData.append(x);
                m_yData.append(y);
            }
        } else {
            bool ok;
            double val = trimmed.toDouble(&ok);
            if (ok) {
                m_xData.append(val);
                m_yData.append(val);
            }
        }
    }

    if (m_xData.isEmpty()) {
        m_infoLabel->setText("Empty or invalid LUT");
        return false;
    }

    int count = m_xData.size();
    double xMin = *std::min_element(m_xData.begin(), m_xData.end());
    double xMax = *std::max_element(m_xData.begin(), m_xData.end());
    double yMin = *std::min_element(m_yData.begin(), m_yData.end());
    double yMax = *std::max_element(m_yData.begin(), m_yData.end());

    m_chart->removeAllSeries();
    m_series = new QLineSeries(this);
    for (int i = 0; i < count; ++i) {
        m_series->append(m_xData[i], m_yData[i]);
    }
    m_chart->addSeries(m_series);

    QValueAxis* axisX = new QValueAxis();
    axisX->setTitleText("Temperature (°C)");
    axisX->setRange(xMin, xMax);
    m_chart->addAxis(axisX, Qt::AlignBottom);
    m_series->attachAxis(axisX);

    QValueAxis* axisY = new QValueAxis();
    axisY->setTitleText("Grip / Factor");
    axisY->setRange(yMin, yMax * 1.1);
    m_chart->addAxis(axisY, Qt::AlignLeft);
    m_series->attachAxis(axisY);

    m_infoLabel->setText(QString("%1 points | X: %2-%3 | Y: %4-%5")
        .arg(count).arg(xMin, 0, 'f', 1).arg(xMax, 0, 'f', 1)
        .arg(yMin, 0, 'f', 3).arg(yMax, 0, 'f', 3));
    m_currentLut = path;

    return true;
}

void LutCurveWidget::setData(const QVector<double>& xData,
                              const QVector<double>& yData,
                              const QString& xLabel,
                              const QString& yLabel) {
    m_xData = xData;
    m_yData = yData;

    m_chart->removeAllSeries();
    m_series = new QLineSeries(this);
    for (int i = 0; i < std::min(xData.size(), yData.size()); ++i) {
        m_series->append(xData[i], yData[i]);
    }
    m_chart->addSeries(m_series);

    double xMin = xData.isEmpty() ? 0 : *std::min_element(xData.begin(), xData.end());
    double xMax = xData.isEmpty() ? 100 : *std::max_element(xData.begin(), xData.end());
    double yMax = yData.isEmpty() ? 1 : *std::max_element(yData.begin(), yData.end());

    QValueAxis* ax = new QValueAxis();
    ax->setTitleText(xLabel);
    ax->setRange(xMin, xMax);
    m_chart->addAxis(ax, Qt::AlignBottom);
    m_series->attachAxis(ax);

    QValueAxis* ay = new QValueAxis();
    ay->setTitleText(yLabel);
    ay->setRange(0, yMax * 1.1);
    m_chart->addAxis(ay, Qt::AlignLeft);
    m_series->attachAxis(ay);
}

void LutCurveWidget::clear() {
    m_chart->removeAllSeries();
    m_xData.clear();
    m_yData.clear();
    m_infoLabel->setText(tr("No LUT loaded"));
    m_lutSelector->clear();
}

void LutCurveWidget::onPointHovered(const QPointF& point, bool isHovering) {
    if (isHovering) {
        m_infoLabel->setText(QString("Point: (%1, %2)").arg(point.x(), 0, 'f', 2).arg(point.y(), 0, 'f', 2));
    } else {
        if (!m_currentLut.isEmpty()) {
            m_infoLabel->setText("LUT: " + QFileInfo(m_currentLut).fileName());
        } else {
            m_infoLabel->setText(tr("No LUT loaded"));
        }
    }
}

QVector<double> LutCurveWidget::parseLutFile(const QString& content) const {
    QVector<double> data;
    QStringList lines = content.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith('#') || trimmed.startsWith(';')) continue;
        bool ok;
        double val = trimmed.toDouble(&ok);
        if (ok) data.append(val);
    }
    return data;
}

// ============================================================================
// EngineCurveWidget
// ============================================================================

EngineCurveWidget::EngineCurveWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    m_chart = new QChart();
    m_chart->setTitle("Power / Torque Curves");
    m_chart->setBackgroundVisible(false);
    m_chart->legend()->setVisible(true);

    m_chartView = new QChartView(m_chart, this);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumHeight(280);

    m_powerSeries = new QLineSeries();
    m_powerSeries->setName("Power (HP)");
    m_powerSeries->setColor(QColor("#E74C3C"));

    m_torqueSeries = new QLineSeries();
    m_torqueSeries->setName("Torque (Nm)");
    m_torqueSeries->setColor(QColor("#3498DB"));

    m_pointsTable = new QTableWidget(0, 3, this);
    m_pointsTable->setHorizontalHeaderLabels({"RPM", "Power (HP)", "Torque (Nm)"});
    m_pointsTable->setAlternatingRowColors(true);
    m_pointsTable->setMinimumHeight(150);

    auto* statsBar = new QHBoxLayout;
    m_maxPowerLabel = new QLabel("Max Power: --", this);
    m_maxTorqueLabel = new QLabel("Max Torque: --", this);
    m_maxRpmLabel = new QLabel("Max RPM: --", this);
    auto* btnAdd    = new QPushButton("Add Point", this);
    auto* btnRemove = new QPushButton("Remove Point", this);
    auto* btnClear  = new QPushButton("Clear", this);
    auto* btnExport = new QPushButton("Export", this);
    auto* btnImport = new QPushButton("Import", this);
    statsBar->addWidget(m_maxPowerLabel);
    statsBar->addWidget(m_maxTorqueLabel);
    statsBar->addWidget(m_maxRpmLabel);
    statsBar->addStretch();
    statsBar->addWidget(btnAdd);
    statsBar->addWidget(btnRemove);
    statsBar->addWidget(btnClear);
    statsBar->addWidget(btnExport);
    statsBar->addWidget(btnImport);

    layout->addWidget(m_chartView, 1);
    layout->addLayout(statsBar);
    layout->addWidget(m_pointsTable);

    connect(btnAdd, &QPushButton::clicked, this, &EngineCurveWidget::onAddPoint);
    connect(btnRemove, &QPushButton::clicked, this, &EngineCurveWidget::onRemovePoint);
    connect(btnClear, &QPushButton::clicked, this, &EngineCurveWidget::onClearCurve);
    connect(btnExport, &QPushButton::clicked, this, &EngineCurveWidget::onExportCurve);
    connect(btnImport, &QPushButton::clicked, this, &EngineCurveWidget::onImportCurve);
}

void EngineCurveWidget::loadFromData(const QVector<double>& rpm,
                                      const QVector<double>& power,
                                      const QVector<double>& torque) {
    m_rpm   = rpm;
    m_power = power;
    m_torque = torque;
    updateChart();
}

void EngineCurveWidget::saveToData(QVector<double>& rpmOut,
                                    QVector<double>& powerOut,
                                    QVector<double>& torqueOut) {
    rpmOut    = m_rpm;
    powerOut  = m_power;
    torqueOut = m_torque;
}

bool EngineCurveWidget::loadFromIni(const QString& carFolder) {
    QString iniPath = carFolder + "/data/engine.ini";
    if (!QFile::exists(iniPath)) return false;

    KsIniDocument doc;
    if (!doc.load(iniPath)) return false;

    const KsIniSection* section = doc.section("ENGINE_DATA");
    if (!section) section = doc.section("DATA");
    if (!section) return false;

    m_rpm.clear();
    m_power.clear();
    m_torque.clear();

    for (int i = 0; i < 20; ++i) {
        QString key = QString("RPM_%1").arg(i);
        if (!section->hasKey(key)) break;
        m_rpm.append(section->getFloat(key, 0));
    }

    for (int i = 0; i < 20; ++i) {
        QString key = QString("POWER_%1").arg(i);
        if (!section->hasKey(key)) break;
        m_power.append(section->getFloat(key, 0));
    }

    for (int i = 0; i < 20; ++i) {
        QString key = QString("TORQUE_%1").arg(i);
        if (!section->hasKey(key)) break;
        m_torque.append(section->getFloat(key, 0));
    }

    if (m_rpm.isEmpty()) return false;
    updateChart();
    return true;
}

void EngineCurveWidget::updateChart() {
    m_chart->removeAllSeries();

    m_powerSeries->clear();
    m_torqueSeries->clear();

    int count = std::min(std::min(m_rpm.size(), m_power.size()), m_torque.size());
    for (int i = 0; i < count; ++i) {
        m_powerSeries->append(m_rpm[i], m_power[i]);
        m_torqueSeries->append(m_rpm[i], m_torque[i]);
    }

    m_chart->addSeries(m_powerSeries);
    m_chart->addSeries(m_torqueSeries);

    if (!m_rpm.isEmpty()) {
        double minRpm = *std::min_element(m_rpm.begin(), m_rpm.end());
        double maxRpm = *std::max_element(m_rpm.begin(), m_rpm.end());
        double maxVal = std::max(
            m_power.isEmpty() ? 0 : *std::max_element(m_power.begin(), m_power.end()),
            m_torque.isEmpty() ? 0 : *std::max_element(m_torque.begin(), m_torque.end()));

        QValueAxis* axisX = new QValueAxis();
        axisX->setTitleText("RPM");
        axisX->setRange(minRpm * 0.9, maxRpm * 1.05);
        m_chart->addAxis(axisX, Qt::AlignBottom);
        m_powerSeries->attachAxis(axisX);
        m_torqueSeries->attachAxis(axisX);

        QValueAxis* axisY = new QValueAxis();
        axisY->setTitleText("Value");
        axisY->setRange(0, maxVal * 1.15);
        m_chart->addAxis(axisY, Qt::AlignLeft);
        m_powerSeries->attachAxis(axisY);
        m_torqueSeries->attachAxis(axisY);
    }

    m_pointsTable->setRowCount(count);
    for (int i = 0; i < count; ++i) {
        m_pointsTable->setItem(i, 0, new QTableWidgetItem(QString::number(m_rpm[i], 'f', 0)));
        m_pointsTable->setItem(i, 1, new QTableWidgetItem(QString::number(m_power[i], 'f', 1)));
        m_pointsTable->setItem(i, 2, new QTableWidgetItem(QString::number(m_torque[i], 'f', 1)));
    }

    if (!m_power.isEmpty()) {
        double maxP = *std::max_element(m_power.begin(), m_power.end());
        m_maxPowerLabel->setText(QString("Max Power: %1 HP").arg(maxP, 0, 'f', 0));
    }
    if (!m_torque.isEmpty()) {
        double maxT = *std::max_element(m_torque.begin(), m_torque.end());
        m_maxTorqueLabel->setText(QString("Max Torque: %1 Nm").arg(maxT, 0, 'f', 0));
    }
    if (!m_rpm.isEmpty()) {
        double maxR = *std::max_element(m_rpm.begin(), m_rpm.end());
        m_maxRpmLabel->setText(QString("Max RPM: %1").arg(maxR, 0, 'f', 0));
    }
}

QVector<double> EngineCurveWidget::smoothCurve(const QVector<double>& values, int steps) const {
    if (values.size() < 3) return values;
    QVector<double> smoothed;
    int n = values.size();
    for (int i = 0; i < n; ++i) {
        double sum = 0;
        int cnt = 0;
        for (int j = std::max(0, i - steps); j <= std::min(n - 1, i + steps); ++j) {
            sum += values[j];
            cnt++;
        }
        smoothed.append(sum / cnt);
    }
    return smoothed;
}

void EngineCurveWidget::onAddPoint() {
    int row = m_pointsTable->currentRow();
    double rpm = row >= 0 && row < m_rpm.size() ? m_rpm[row] + 500 : (m_rpm.isEmpty() ? 1000 : m_rpm.last() + 500);
    double power = row >= 0 && row < m_power.size() ? m_power[row] : 0;
    double torque = row >= 0 && row < m_torque.size() ? m_torque[row] : 0;

    m_rpm.append(rpm);
    m_power.append(power);
    m_torque.append(torque);
    std::sort(m_rpm.begin(), m_rpm.end());
    updateChart();
}

void EngineCurveWidget::onRemovePoint() {
    int row = m_pointsTable->currentRow();
    if (row < 0 || row >= m_rpm.size()) return;
    m_rpm.removeAt(row);
    if (row < m_power.size()) m_power.removeAt(row);
    if (row < m_torque.size()) m_torque.removeAt(row);
    updateChart();
}

void EngineCurveWidget::onClearCurve() {
    m_rpm.clear();
    m_power.clear();
    m_torque.clear();
    updateChart();
}

void EngineCurveWidget::onExportCurve() {
    QString path = QFileDialog::getSaveFileName(this, "Export Curve",
        QString(), "CSV files (*.csv);;All files (*.*)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&file);
    out << "RPM,Power,Torque\n";
    int count = std::min(std::min(m_rpm.size(), m_power.size()), m_torque.size());
    for (int i = 0; i < count; ++i) {
        out << m_rpm[i] << "," << m_power[i] << "," << m_torque[i] << "\n";
    }
    file.close();
}

void EngineCurveWidget::onImportCurve() {
    QString path = QFileDialog::getOpenFileName(this, "Import Curve",
        QString(), "CSV files (*.csv);;All files (*.*)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    m_rpm.clear();
    m_power.clear();
    m_torque.clear();

    QTextStream in(&file);
    QString header = in.readLine();
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;
        QStringList parts = line.split(',');
        if (parts.size() >= 3) {
            m_rpm.append(parts[0].toDouble());
            m_power.append(parts[1].toDouble());
            m_torque.append(parts[2].toDouble());
        }
    }
    file.close();
    updateChart();
}

// ============================================================================
// TelemetryWidget
// ============================================================================

TelemetryWidget::TelemetryWidget(QWidget* parent)
    : QWidget(parent)
{
    buildUI();
}

TelemetryWidget::~TelemetryWidget() {
    stopSession();
}

void TelemetryWidget::buildUI() {
    auto* layout = new QVBoxLayout(this);

    auto* topBar = new QHBoxLayout;
    m_sessionLabel = new QLabel("Session: Stopped", this);
    m_startBtn = new QPushButton("Start Session", this);
    m_calibrateBtn = new QPushButton("Calibrate", this);
    m_calibrateBtn->setEnabled(false);
    topBar->addWidget(m_sessionLabel);
    topBar->addWidget(m_startBtn);
    topBar->addWidget(m_calibrateBtn);
    topBar->addStretch();

    auto* gaugesGrid = new QGridLayout;

    // Speed
    auto* speedBox = new QGroupBox("Speed", this);
    auto* speedLayout = new QVBoxLayout(speedBox);
    m_speedBar = new QProgressBar(this);
    m_speedBar->setRange(0, 350);
    m_speedBar->setValue(0);
    m_speedLabel = new QLabel("0 km/h", this);
    m_speedLabel->setAlignment(Qt::AlignCenter);
    m_speedLabel->setStyleSheet("font-size: 24pt; font-weight: bold;");
    speedLayout->addWidget(m_speedBar);
    speedLayout->addWidget(m_speedLabel);

    // RPM
    auto* rpmBox = new QGroupBox("RPM", this);
    auto* rpmLayout = new QVBoxLayout(rpmBox);
    m_rpmBar = new QProgressBar(this);
    m_rpmBar->setRange(0, 12000);
    m_rpmBar->setValue(0);
    m_rpmLabel = new QLabel("0", this);
    m_rpmLabel->setAlignment(Qt::AlignCenter);
    m_rpmLabel->setStyleSheet("font-size: 20pt; font-weight: bold; color: #E74C3C;");
    rpmLayout->addWidget(m_rpmBar);
    rpmLayout->addWidget(m_rpmLabel);

    // Gear
    auto* gearBox = new QGroupBox("Gear", this);
    m_gearLabel = new QLabel("N", this);
    m_gearLabel->setAlignment(Qt::AlignCenter);
    m_gearLabel->setStyleSheet("font-size: 48pt; font-weight: bold; color: #3498DB;");
    auto* gearLayout = new QVBoxLayout(gearBox);
    gearLayout->addWidget(m_gearLabel);

    // Throttle/Brake
    auto* pedalBox = new QGroupBox("Pedals", this);
    auto* pedalLayout = new QFormLayout(pedalBox);
    m_throttleBar = new QProgressBar(this);
    m_throttleBar->setRange(0, 100);
    m_brakeBar = new QProgressBar(this);
    m_brakeBar->setRange(0, 100);
    pedalLayout->addRow("Throttle:", m_throttleBar);
    pedalLayout->addRow("Brake:", m_brakeBar);

    gaugesGrid->addWidget(speedBox, 0, 0);
    gaugesGrid->addWidget(rpmBox, 0, 1);
    gaugesGrid->addWidget(gearBox, 0, 2);
    gaugesGrid->addWidget(pedalBox, 1, 0, 1, 3);

    // Tyre temps
    m_tyreTemps = new QTableWidget(1, 4, this);
    m_tyreTemps->setHorizontalHeaderLabels({"FL", "FR", "RL", "RR"});
    m_tyreTemps->verticalHeader()->setVisible(false);
    m_tyreTemps->setMaximumHeight(60);

    layout->addLayout(topBar);
    layout->addLayout(gaugesGrid);
    layout->addWidget(new QLabel("Tyre Temperatures (°C):", this));
    layout->addWidget(m_tyreTemps);

    connect(m_startBtn, &QPushButton::clicked, this, &TelemetryWidget::onStartStopClicked);
    connect(m_calibrateBtn, &QPushButton::clicked, this, &TelemetryWidget::onCalibrateClicked);
}

void TelemetryWidget::loadTelemetryConfig(const QString& path) {
    QSettings cfg(path, QSettings::IniFormat);
    cfg.beginGroup("TELEMETRY");
    m_telemetryIp = cfg.value("IP", "127.0.0.1").toString();
    m_telemetryPort = cfg.value("Port", 9996).toInt();
    m_useUDP = cfg.value("EnableUDP", true).toBool();
    cfg.endGroup();
    qDebug() << "Telemetry config loaded:" << m_telemetryIp << m_telemetryPort << (m_useUDP ? "UDP" : "TCP");
}

void TelemetryWidget::startSession(const QString& carFolder) {
    // Load car-specific telemetry settings if available
    if (!carFolder.isEmpty() && QDir(carFolder).exists()) {
        QString configPath = carFolder + "/data/telemetry.ini";
        if (QFile::exists(configPath)) {
            loadTelemetryConfig(configPath);
        }
    }

    if (m_useUDP) {
        if (!m_udpSocket) {
            m_udpSocket = new QUdpSocket(this);
            connect(m_udpSocket, &QUdpSocket::readyRead, this, &TelemetryWidget::onDataReceived);
        }
        m_udpSocket->close();
        if (m_udpSocket->bind(QHostAddress::Any, m_telemetryPort)) {
            qDebug() << "Telemetry UDP bound to port" << m_telemetryPort;
        } else {
            qDebug() << "Telemetry UDP bind failed:" << m_udpSocket->errorString();
        }
    }

    m_sessionActive = true;
    m_sessionLabel->setText("Session: Active");
    m_startBtn->setText("Stop Session");
    m_calibrateBtn->setEnabled(true);
    emit sessionStarted();
}

void TelemetryWidget::stopSession() {
    m_sessionActive = false;
    if (m_udpSocket) {
        m_udpSocket->close();
    }
    m_sessionLabel->setText("Session: Stopped");
    m_startBtn->setText("Start Session");
    m_calibrateBtn->setEnabled(false);
    emit sessionStopped();
}

void TelemetryWidget::onStartStopClicked() {
    if (m_sessionActive) stopSession();
    else startSession(QString());
}

void TelemetryWidget::onCalibrateClicked() {
    m_speedBar->setValue(0);
    m_rpmBar->setValue(0);
    m_speedLabel->setText("0 km/h");
    m_rpmLabel->setText("0");
    for (int c = 0; c < 4; ++c) {
        if (m_tyreTemps->item(0, c))
            m_tyreTemps->item(0, c)->setText("0");
    }
}

void TelemetryWidget::onDataReceived() {
    if (!m_sessionActive || !m_udpSocket) return;

    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_udpSocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;
        m_udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        if (datagram.size() < (int)sizeof(float) * 9) continue;

        const float* data = reinterpret_cast<const float*>(datagram.constData());
        TelemetrySample sample;
        sample.speed = data[0];
        sample.rpm = data[1];
        sample.gear = static_cast<int>(data[2]);
        sample.throttle = data[3];
        sample.brake = data[4];
        sample.tyreTemp[0] = data[5];
        sample.tyreTemp[1] = data[6];
        sample.tyreTemp[2] = data[7];
        sample.tyreTemp[3] = data[8];
        sample.timestamp = QDateTime::currentMSecsSinceEpoch() / 1000.0;

        receiveTelemetrySample(sample);
    }
}

void TelemetryWidget::receiveTelemetrySample(const TelemetrySample& sample) {
    if (!m_sessionActive) return;

    updateGauge(sample.speed, m_speedBar, m_speedLabel, 0, 350, "km/h");
    updateGauge(sample.rpm, m_rpmBar, m_rpmLabel, 0, 12000, "rpm");

    m_throttleBar->setValue(int(sample.throttle * 100));
    m_brakeBar->setValue(int(sample.brake * 100));

    QStringList gearNames = {"R", "N", "1", "2", "3", "4", "5", "6", "7", "8"};
    int gearIdx = sample.gear + 1;
    if (gearIdx >= 0 && gearIdx < gearNames.size())
        m_gearLabel->setText(gearNames[gearIdx]);

    for (int c = 0; c < 4; ++c) {
        auto* item = m_tyreTemps->item(0, c);
        if (!item) {
            item = new QTableWidgetItem();
            m_tyreTemps->setItem(0, c, item);
        }
        item->setText(QString::number(sample.tyreTemp[c], 'f', 0));
    }

    emit sampleReceived(sample);
}

void TelemetryWidget::updateGauge(double value, QProgressBar* bar, QLabel* label,
                                   double min, double max, const QString& unit) {
    bar->setValue(int(value));
    label->setText(QString::number(value, 'f', 0) + " " + unit);
}

// ============================================================================
// CarSetupCompareWidget
// ============================================================================

CarSetupCompareWidget::CarSetupCompareWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    auto* topBar = new QHBoxLayout;
    m_labelA = new QLabel("Setup A: --", this);
    m_labelB = new QLabel("Setup B: --", this);
    m_browseA = new QPushButton("Browse A...", this);
    m_browseB = new QPushButton("Browse B...", this);
    auto* refreshBtn = new QPushButton("Refresh", this);
    topBar->addWidget(m_labelA);
    topBar->addWidget(m_browseA);
    topBar->addSpacing(20);
    topBar->addWidget(m_labelB);
    topBar->addWidget(m_browseB);
    topBar->addWidget(refreshBtn);
    topBar->addStretch();

    m_table = new QTableWidget(0, 4, this);
    m_table->setHorizontalHeaderLabels({"Parameter", "Setup A", "Setup B", "Delta"});
    m_table->setAlternatingRowColors(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(true);

    layout->addLayout(topBar);
    layout->addWidget(m_table);

    connect(m_browseA, &QPushButton::clicked, this, &CarSetupCompareWidget::onBrowseA);
    connect(m_browseB, &QPushButton::clicked, this, &CarSetupCompareWidget::onBrowseB);
    connect(refreshBtn, &QPushButton::clicked, this, &CarSetupCompareWidget::onRefreshDiff);
}

void CarSetupCompareWidget::loadSetupA(const QString& path) {
    m_pathA = path;
    KsSetupManager::loadSetupFromFile(path, m_setupA);
    m_labelA->setText("Setup A: " + QFileInfo(path).baseName());
    populateTable();
}

void CarSetupCompareWidget::loadSetupB(const QString& path) {
    m_pathB = path;
    KsSetupManager::loadSetupFromFile(path, m_setupB);
    m_labelB->setText("Setup B: " + QFileInfo(path).baseName());
    populateTable();
}

void CarSetupCompareWidget::clear() {
    m_setupA = KsSetupData();
    m_setupB = KsSetupData();
    m_pathA.clear();
    m_pathB.clear();
    m_labelA->setText("Setup A: --");
    m_labelB->setText("Setup B: --");
    m_table->setRowCount(0);
}

QVector<QPair<QString, double>> CarSetupCompareWidget::buildRows(const KsSetupData& s) const {
    QVector<QPair<QString, double>> rows;
    rows.append({"Steer Ratio", s.steerRatio});
    rows.append({"Brake Bias", s.brakeBias * 100});
    rows.append({"Ride Height F (mm)", s.rideHeight[0] * 1000});
    rows.append({"Ride Height R (mm)", s.rideHeight[1] * 1000});
    rows.append({"Camber FL (°)", s.frontCamber[0]});
    rows.append({"Camber FR (°)", s.frontCamber[1]});
    rows.append({"Camber RL (°)", s.rearCamber[0]});
    rows.append({"Camber RR (°)", s.rearCamber[1]});
    rows.append({"Toe FL (°)", s.toeOut[0]});
    rows.append({"Toe FR (°)", s.toeOut[1]});
    rows.append({"Toe RL (°)", s.toeOut[2]});
    rows.append({"Toe RR (°)", s.toeOut[3]});
    rows.append({"Spring FL (N/mm)", s.springRate[0]});
    rows.append({"Spring FR (N/mm)", s.springRate[1]});
    rows.append({"Spring RL (N/mm)", s.springRate[2]});
    rows.append({"Spring RR (N/mm)", s.springRate[3]});
    rows.append({"Comp FL", s.compression[0]});
    rows.append({"Reb FL", s.rebound[0]});
    rows.append({"ARB Front", s.frontARB});
    rows.append({"ARB Rear", s.rearARB});
    rows.append({"Front Wing", s.frontWing});
    rows.append({"Rear Wing", s.rearWing});
    rows.append({"Diff Power (%)", s.diffPower});
    rows.append({"Diff Coast (%)", s.diffCoast});
    rows.append({"Diff Drive (%)", s.diffDrive});
    rows.append({"Tyre Pressure F (psi)", s.frontTyrePressure[0]});
    rows.append({"Tyre Pressure R (psi)", s.rearTyrePressure[0]});
    rows.append({"Fuel (L)", s.fuelLevel});
    return rows;
}

void CarSetupCompareWidget::populateTable() {
    auto rowsA = buildRows(m_setupA);
    auto rowsB = buildRows(m_setupB);

    m_table->setRowCount(rowsA.size());

    for (int i = 0; i < rowsA.size(); ++i) {
        const QString& name = rowsA[i].first;
        double a = rowsA[i].second;
        double b = i < rowsB.size() ? rowsB[i].second : 0;
        double delta = b - a;

        m_table->setItem(i, 0, new QTableWidgetItem(name));
        m_table->setItem(i, 1, new QTableWidgetItem(QString::number(a, 'f', 2)));
        m_table->setItem(i, 2, new QTableWidgetItem(QString::number(b, 'f', 2)));

        auto* deltaItem = new QTableWidgetItem(
            (delta > 0 ? "+" : "") + QString::number(delta, 'f', 2));
        if (std::abs(delta) > 1e-4) {
            deltaItem->setForeground(delta > 0 ? QColor("#27ae60") : QColor("#e74c3c"));
            QFont f = deltaItem->font();
            f.setBold(true);
            deltaItem->setFont(f);
        }
        m_table->setItem(i, 3, deltaItem);
    }
    m_table->resizeColumnsToContents();
}

void CarSetupCompareWidget::onBrowseA() {
    QString path = QFileDialog::getOpenFileName(this, "Select Setup A",
        QString(), "Setup files (*.ini);;All files (*.*)");
    if (!path.isEmpty()) loadSetupA(path);
}

void CarSetupCompareWidget::onBrowseB() {
    QString path = QFileDialog::getOpenFileName(this, "Select Setup B",
        QString(), "Setup files (*.ini);;All files (*.*)");
    if (!path.isEmpty()) loadSetupB(path);
}

void CarSetupCompareWidget::onRefreshDiff() {
    populateTable();
}

// ============================================================================
// AcdBrowserWidget
// ============================================================================

AcdBrowserWidget::AcdBrowserWidget(QWidget* parent)
    : QWidget(parent)
{
    buildUI();
}

void AcdBrowserWidget::buildUI() {
    auto* layout = new QHBoxLayout(this);

    m_tree = new QTreeWidget(this);
    m_tree->setHeaderLabel("ACD Contents");
    m_tree->setMinimumWidth(250);

    m_preview = new QTextEdit(this);
    m_preview->setReadOnly(true);
    m_preview->setFont(QFont("Consolas", 9));

    auto* rightPanel = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightPanel);
    m_exportBtn = new QPushButton("Export Selected", this);
    rightLayout->addWidget(new QLabel("Preview:", this));
    rightLayout->addWidget(m_preview);
    rightLayout->addWidget(m_exportBtn);

    layout->addWidget(m_tree);
    layout->addWidget(rightPanel, 1);

    connect(m_tree, &QTreeWidget::itemClicked, this, &AcdBrowserWidget::onItemClicked);
    connect(m_exportBtn, &QPushButton::clicked, this, &AcdBrowserWidget::onExportSelected);
}

void AcdBrowserWidget::setExtractedPath(const QString& path) {
    m_extractPath = path;
    refresh();
}

void AcdBrowserWidget::refresh() {
    m_tree->clear();
    if (m_extractPath.isEmpty() || !QDir(m_extractPath).exists()) return;

    QDir dir(m_extractPath);
    auto entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
    for (const QFileInfo& info : entries) {
        auto* item = new QTreeWidgetItem(m_tree);
        item->setText(0, info.fileName());
        item->setData(0, Qt::UserRole, info.absoluteFilePath());
        item->setData(0, Qt::UserRole + 1, detectFileType(info.suffix().toLower()));

        if (info.isDir()) {
            item->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
        } else {
            item->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
        }
    }
}

void AcdBrowserWidget::onItemClicked(QTreeWidgetItem* item, int) {
    if (!item) return;
    QString path = item->data(0, Qt::UserRole).toString();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        m_preview->setPlainText("[Cannot read file]");
        return;
    }

    QByteArray data = file.read(4096);
    file.close();

    QString type = item->data(0, Qt::UserRole + 1).toString();
    if (type == "text") {
        m_preview->setPlainText(QString::fromUtf8(data));
    } else {
        m_preview->setPlainText(QString("Binary file: %1\nSize: %2 bytes\n\nHex preview:\n%3")
            .arg(item->text(0))
            .arg(QFileInfo(path).size())
            .arg(QString(data.toHex()).left(512)));
    }
}

void AcdBrowserWidget::onExportSelected() {
    QTreeWidgetItem* item = m_tree->currentItem();
    if (!item) return;

    QString srcPath = item->data(0, Qt::UserRole).toString();
    QString dstPath = QFileDialog::getSaveFileName(this, "Export to",
        QFileInfo(srcPath).fileName());
    if (!dstPath.isEmpty()) {
        QFile::copy(srcPath, dstPath);
        m_preview->append("\n[Exported: " + dstPath + "]");
    }
}

QString AcdBrowserWidget::detectFileType(const QString& ext) const {
    static const QStringList textExt = {"ini", "json", "txt", "lut", "knh", "csv"};
    if (textExt.contains(ext)) return "text";
    return "binary";
}

// ============================================================================
// SuspGeometryWidget
// ============================================================================

SuspGeometryWidget::SuspGeometryWidget(QWidget* parent)
    : QWidget(parent)
{
    buildUI();
}

void SuspGeometryWidget::buildUI() {
    auto* layout = new QVBoxLayout(this);

    auto* inputRow = new QHBoxLayout;

    auto* inputBox = new QGroupBox("Input Parameters", this);
    auto* inputForm = new QFormLayout(inputBox);

    auto addSpin = [&](const QString& label, double min, double max, double val, double step) {
        auto* spin = new QDoubleSpinBox(this);
        spin->setRange(min, max);
        spin->setValue(val);
        spin->setSingleStep(step);
        spin->setSuffix(" mm");
        inputForm->addRow(label, spin);
        m_inputs.append(spin);
        return spin;
    };

    addSpin("Upper Arm Length:", 200, 600, 350, 5);
    addSpin("Lower Arm Length:", 200, 600, 300, 5);
    addSpin("Upper Mount Y:", -100, 100, 0, 5);
    addSpin("Lower Mount Y:", -100, 100, -80, 5);
    addSpin("Upper Angle (°):", -30, 30, 10, 0.5);
    addSpin("Lower Angle (°):", -30, 30, -5, 0.5);
    addSpin("Scrub Radius:", -50, 50, 20, 1);
    addSpin("Initial Camber (°):", -5, 0, -2, 0.1);

    inputRow->addWidget(inputBox);

    auto* controlsBox = new QVBoxLayout;
    auto* calcBtn = new QPushButton("Calculate", this);
    auto* exportBtn = new QPushButton("Export Diagram", this);
    controlsBox->addWidget(calcBtn);
    controlsBox->addWidget(exportBtn);
    controlsBox->addStretch();
    inputRow->addLayout(controlsBox);

    m_resultsTable = new QTableWidget(0, 5, this);
    m_resultsTable->setHorizontalHeaderLabels(
        {"Wheel", "Bump (mm)", "Camber (°)", "IC X (mm)", "RC Height (mm)"});
    m_resultsTable->setAlternatingRowColors(true);
    m_resultsTable->setMinimumHeight(180);

    m_camberChart = new QChart();
    m_camberChart->setTitle("Camber Gain vs Bump");
    m_camberChart->setBackgroundVisible(false);
    m_camberChart->legend()->setVisible(true);

    m_camberChartView = new QChartView(m_camberChart, this);
    m_camberChartView->setRenderHint(QPainter::Antialiasing);
    m_camberChartView->setMinimumHeight(240);

    layout->addLayout(inputRow);
    layout->addWidget(m_resultsTable);
    layout->addWidget(m_camberChartView);

    connect(calcBtn, &QPushButton::clicked, this, &SuspGeometryWidget::onRecalculate);
    connect(exportBtn, &QPushButton::clicked, this, &SuspGeometryWidget::onExportDiagram);

    for (QDoubleSpinBox* spin : m_inputs) {
        connect(spin, &QDoubleSpinBox::valueChanged, this, &SuspGeometryWidget::onRecalculate);
    }
}

void SuspGeometryWidget::loadFromSetup(const KsSetupData& setup) {
    if (m_inputs.size() >= 8) {
        m_inputs[7]->setValue((setup.frontCamber[0] + setup.frontCamber[1]) / 2.0 * -1.0);
    }
    onRecalculate();
}

double SuspGeometryWidget::calcCamberGain(double bump, double scrubRadius,
                                           double upperLen, double lowerLen,
                                           double upperAngle, double lowerAngle) const {
    double upperRad = upperAngle * M_PI / 180.0;
    double lowerRad = lowerAngle * M_PI / 180.0;

    double upperChange = (bump / upperLen) * qCos(upperRad) * 57.3;
    double lowerChange = (bump / lowerLen) * qCos(lowerRad) * 57.3;

    return upperChange - lowerChange;
}

QPointF SuspGeometryWidget::calcInstantCenter(double upperLen, double lowerLen,
                                               double upperAngle, double lowerAngle,
                                               double lowerMountY) const {
    double upperRad = upperAngle * M_PI / 180.0;
    double lowerRad = lowerAngle * M_PI / 180.0;

    double upperDx = upperLen * qSin(upperRad);
    double upperDy = upperLen * qCos(upperRad);

    double lowerDx = lowerLen * qSin(lowerRad);
    double lowerDy = lowerLen * qCos(lowerRad);

    double ix = (upperDx * lowerDy - lowerDx * upperDy) /
                 (upperDy - lowerDy + 0.001);
    double iy = lowerMountY + (lowerDx / (lowerDy + 0.001)) * (lowerMountY - ix);

    return QPointF(ix, iy);
}

void SuspGeometryWidget::onRecalculate() {
    if (m_inputs.size() < 8) return;

    double upperLen   = m_inputs[0]->value();
    double lowerLen   = m_inputs[1]->value();
    double upperMountY = m_inputs[2]->value();
    double lowerMountY = m_inputs[3]->value();
    double upperAngle  = m_inputs[4]->value();
    double lowerAngle  = m_inputs[5]->value();
    double scrubRadius = m_inputs[6]->value();
    double initCamber  = m_inputs[7]->value();

    m_resultsTable->setRowCount(8);

    QStringList wheels = {"FL", "FR", "RL", "RR"};
    QVector<double> camberGains(4);
    QVector<double> rcHeights(4);

    m_camberChart->removeAllSeries();

    for (int w = 0; w < 4; ++w) {
        QLineSeries* series = new QLineSeries();
        series->setName(wheels[w]);
        m_camberChart->addSeries(series);

        int row = w * 2;
        m_resultsTable->setItem(row, 0, new QTableWidgetItem(wheels[w] + " - Compression"));
        m_resultsTable->setItem(row + 1, 0, new QTableWidgetItem(wheels[w] + " - Droop"));

        for (int b = 0; b <= 50; b += 5) {
            double bump = b;
            double gain = calcCamberGain(bump, scrubRadius, upperLen, lowerLen, upperAngle, lowerAngle);
            double camber = initCamber + gain;
            series->append(bump, camber);

            if (b == 0 || b == 25 || b == 50) {
                QPointF ic = calcInstantCenter(upperLen, lowerLen, upperAngle, lowerAngle, lowerMountY);
                int displayRow = b == 0 ? row : (b == 25 ? row : row + 1);
                if (b == 0) {
                    m_resultsTable->setItem(row, 1, new QTableWidgetItem(QString::number(bump, 'f', 1)));
                    m_resultsTable->setItem(row, 2, new QTableWidgetItem(QString::number(camber, 'f', 2) + "°"));
                    m_resultsTable->setItem(row, 3, new QTableWidgetItem(QString::number(ic.x(), 'f', 1)));
                    m_resultsTable->setItem(row, 4, new QTableWidgetItem(QString::number(ic.y(), 'f', 1)));
                }
            }
            camberGains[w] = gain;
        }
    }

    QValueAxis* axisX = new QValueAxis();
    axisX->setTitleText("Bump (mm)");
    axisX->setRange(0, 50);
    m_camberChart->addAxis(axisX, Qt::AlignBottom);

    QValueAxis* axisY = new QValueAxis();
    axisY->setTitleText("Camber (°)");
    axisY->setRange(-5, 5);
    m_camberChart->addAxis(axisY, Qt::AlignLeft);

    for (auto* series : m_camberChart->series()) {
        series->attachAxis(axisX);
        series->attachAxis(axisY);
    }

    m_resultsTable->resizeColumnsToContents();
    emit geometryCalculated();
}

void SuspGeometryWidget::onExportDiagram() {
    QString path = QFileDialog::getSaveFileName(this, "Export Diagram",
        QString(), "PNG Image (*.png);;SVG (*.svg);;PDF (*.pdf)");
    if (path.isEmpty()) return;

    QPixmap pixmap(m_camberChartView->size());
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);
    m_camberChartView->render(&painter);
    pixmap.save(path);
}

// ============================================================================
// FfbPreviewWidget
// ============================================================================

FfbPreviewWidget::FfbPreviewWidget(QWidget* parent)
    : QWidget(parent)
{
    buildUI();
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &FfbPreviewWidget::onDataUpdate);
}

void FfbPreviewWidget::buildUI() {
    auto* layout = new QVBoxLayout(this);

    m_chart = new QChart();
    m_chart->setTitle("Force Feedback Preview");
    m_chart->setBackgroundVisible(false);

    m_ffbSeries = new QLineSeries();
    m_ffbSeries->setName("FFB Torque");
    m_chart->addSeries(m_ffbSeries);
    m_chart->createDefaultAxes();

    m_chartView = new QChartView(m_chart, this);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumHeight(200);

    auto* controlsRow = new QHBoxLayout;

    auto* dialBox = new QGroupBox("Controls", this);
    auto* dialLayout = new QFormLayout(dialBox);

    m_angleDial = new QDial(this);
    m_angleDial->setRange(-90, 90);
    m_angleDial->setValue(0);
    m_speedDial = new QDial(this);
    m_speedDial->setRange(0, 300);
    m_speedDial->setValue(100);
    m_dampingDial = new QDial(this);
    m_dampingDial->setRange(0, 100);
    m_dampingDial->setValue(50);

    dialLayout->addRow("Steering Angle (°):", m_angleDial);
    dialLayout->addRow("Speed (km/h):", m_speedDial);
    dialLayout->addRow("Damping (%):", m_dampingDial);

    auto* infoBox = new QVBoxLayout;
    m_torqueLabel = new QLabel("Torque: 0.0 Nm", this);
    m_torqueLabel->setStyleSheet("font-size: 18pt; font-weight: bold; color: #E74C3C;");
    m_peakLabel = new QLabel("Peak: 0.0 Nm", this);
    m_peakLabel->setStyleSheet("color: gray;");

    m_ffbBar = new QProgressBar(this);
    m_ffbBar->setRange(0, 100);
    m_ffbBar->setValue(0);

    infoBox->addWidget(m_torqueLabel);
    infoBox->addWidget(m_peakLabel);
    infoBox->addWidget(m_ffbBar);

    auto* presetBox = new QGroupBox("Presets", this);
    auto* presetLayout = new QVBoxLayout(presetBox);
    auto* presetBtn1 = new QPushButton("Stock Formula", this);
    auto* presetBtn2 = new QPushButton("GT3 Race", this);
    auto* presetBtn3 = new QPushButton("Drift", this);
    auto* exportBtn = new QPushButton("Export FFB Curve", this);
    presetLayout->addWidget(presetBtn1);
    presetLayout->addWidget(presetBtn2);
    presetLayout->addWidget(presetBtn3);
    presetLayout->addWidget(exportBtn);

    controlsRow->addWidget(dialBox);
    controlsRow->addLayout(infoBox);
    controlsRow->addWidget(presetBox);

    layout->addWidget(m_chartView);
    layout->addLayout(controlsRow);

    connect(m_angleDial, &QDial::valueChanged, this, [this](int v) {
        m_currentAngle = v;
        onSimulateLoad(v);
    });
    connect(m_speedDial, &QDial::valueChanged, this, [this](int v) {
        m_currentSpeed = v;
        onSimulateLoad(m_currentAngle);
    });
    connect(m_dampingDial, &QDial::valueChanged, this, [this](int v) {
        m_currentDamping = v / 100.0;
        onSimulateLoad(m_currentAngle);
    });
    connect(presetBtn1, &QPushButton::clicked, this, [this]{ onLoadPreset("stock"); });
    connect(presetBtn2, &QPushButton::clicked, this, [this]{ onLoadPreset("gt3"); });
    connect(presetBtn3, &QPushButton::clicked, this, [this]{ onLoadPreset("drift"); });
    connect(exportBtn, &QPushButton::clicked, this, &FfbPreviewWidget::onExportFfbCurve);
}

void FfbPreviewWidget::startPreview() {
    m_previewActive = true;
    m_updateTimer->start(50);
}

void FfbPreviewWidget::stopPreview() {
    m_previewActive = false;
    m_updateTimer->stop();
}

double FfbPreviewWidget::calculateFfbTorque(double angle, double speed, double damping) const {
    double absAngle = std::abs(angle) * M_PI / 180.0;
    double absSpeed = speed / 3.6;
    double baseTorque = absAngle * 15.0;
    double speedEffect = absSpeed * 0.5;
    double total = baseTorque + speedEffect;
    return total * damping;
}

void FfbPreviewWidget::onDataUpdate() {
    if (!m_previewActive) return;
    double torque = calculateFfbTorque(m_currentAngle, m_currentSpeed, m_currentDamping);
    m_ffbHistory.append(torque);
    if (m_ffbHistory.size() > 200) m_ffbHistory.removeFirst();

    m_ffbSeries->clear();
    for (int i = 0; i < m_ffbHistory.size(); ++i) {
        m_ffbSeries->append(i, m_ffbHistory[i]);
    }

    m_peakFfb = std::max(m_peakFfb, std::abs(torque));
    m_torqueLabel->setText(QString("Torque: %1 Nm").arg(torque, 0, 'f', 1));
    m_peakLabel->setText(QString("Peak: %1 Nm").arg(m_peakFfb, 0, 'f', 1));
    m_ffbBar->setValue(int(std::abs(torque) / (m_peakFfb + 0.01) * 100));
    emit ffbLevelChanged(torque);
}

void FfbPreviewWidget::onSimulateLoad(double angle) {
    double torque = calculateFfbTorque(angle, m_currentSpeed, m_currentDamping);
    m_torqueLabel->setText(QString("Torque: %1 Nm").arg(torque, 0, 'f', 1));
}

void FfbPreviewWidget::onLoadPreset(const QString& preset) {
    if (preset == "stock") {
        m_dampingDial->setValue(50);
        m_angleDial->setValue(0);
        m_speedDial->setValue(80);
    } else if (preset == "gt3") {
        m_dampingDial->setValue(70);
        m_angleDial->setValue(0);
        m_speedDial->setValue(150);
    } else if (preset == "drift") {
        m_dampingDial->setValue(30);
        m_angleDial->setValue(0);
        m_speedDial->setValue(120);
    }
}

void FfbPreviewWidget::onExportFfbCurve() {
    QString path = QFileDialog::getSaveFileName(this, "Export FFB",
        QString(), "CSV (*.csv)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&file);
    out << "Sample,Torque\n";
    for (int i = 0; i < m_ffbHistory.size(); ++i) {
        out << i << "," << m_ffbHistory[i] << "\n";
    }
    file.close();
}

// ============================================================================
// CarValidatorWidget
// ============================================================================

CarValidatorWidget::CarValidatorWidget(QWidget* parent)
    : QWidget(parent)
{
    buildUI();
}

void CarValidatorWidget::buildUI() {
    auto* layout = new QVBoxLayout(this);

    auto* topBar = new QHBoxLayout;
    m_runBtn = new QPushButton("Run Validation", this);
    m_exportBtn = new QPushButton("Export Report", this);
    auto* infoLabel = new QLabel("Validates: tyre pressures, suspension geometry, engine data, aero balance, mass distribution", this);
    infoLabel->setStyleSheet("color: gray;");
    topBar->addWidget(m_runBtn);
    topBar->addWidget(m_exportBtn);
    topBar->addWidget(infoLabel);
    topBar->addStretch();

    m_summaryTable = new QTableWidget(0, 3, this);
    m_summaryTable->setHorizontalHeaderLabels({"Category", "Issues", "Status"});
    m_summaryTable->setMaximumHeight(100);
    m_summaryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_reportEdit = new QTextEdit(this);
    m_reportEdit->setReadOnly(true);
    m_reportEdit->setFont(QFont("Consolas", 9));

    m_issuesList = new QListWidget(this);
    m_issuesList->setMaximumHeight(120);

    layout->addLayout(topBar);
    layout->addWidget(m_summaryTable);
    layout->addWidget(m_issuesList);
    layout->addWidget(m_reportEdit);

    connect(m_runBtn, &QPushButton::clicked, this, &CarValidatorWidget::onRunValidation);
    connect(m_exportBtn, &QPushButton::clicked, this, &CarValidatorWidget::onExportReport);
}

void CarValidatorWidget::validateCar(const QString& carFolder) {
    m_lastCarFolder = carFolder;

    QStringList errors;
    QStringList warnings;

    QString tyresIni = carFolder + "/data/tyres.ini";
    QString engineIni = carFolder + "/data/engine.ini";
    QString setupIni = carFolder + "/data/setup.ini";

    if (QFile::exists(tyresIni)) {
        KsIniDocument doc;
        doc.load(tyresIni);
        QString issue = checkTyrePressures(doc);
        if (!issue.isEmpty()) warnings.append(issue);
    } else {
        errors.append("Missing tyres.ini");
    }

    if (QFile::exists(setupIni)) {
        KsSetupData setup;
        KsSetupManager::loadSetupFromFile(setupIni, setup);
        warnings.append(checkSuspensionGeometry(setup));
        warnings.append(checkAeroBalance(setup));
        warnings.append(checkMassDistribution(setup));
    }

    if (QFile::exists(engineIni)) {
        KsIniDocument doc;
        doc.load(engineIni);
        errors.append(checkEngineData(doc));
    }

    populateIssues(errors, warnings);

    QString report = "=== Car Validation Report ===\n\n";
    report += "Car: " + QFileInfo(carFolder).fileName() + "\n";
    report += "Date: " + QDateTime::currentDateTime().toString() + "\n\n";
    report += "ERRORS (" + QString::number(errors.size()) + "):\n";
    for (const QString& e : errors) report += "  [ERROR] " + e + "\n";
    report += "\nWARNINGS (" + QString::number(warnings.size()) + "):\n";
    for (const QString& w : warnings) report += "  [WARN] " + w + "\n";

    m_reportEdit->setPlainText(report);
    emit validationComplete(errors.size(), warnings.size());
}

void CarValidatorWidget::validateIni(const QString& iniPath) {
    KsIniDocument doc;
    if (!doc.load(iniPath)) {
        m_reportEdit->setPlainText("Cannot load: " + iniPath);
        return;
    }

    QStringList errors;
    if (iniPath.contains("tyres", Qt::CaseInsensitive)) {
        errors.append(checkTyrePressures(doc));
    } else if (iniPath.contains("engine", Qt::CaseInsensitive)) {
        errors.append(checkEngineData(doc));
    }

    populateIssues(errors, {});
}

QString CarValidatorWidget::checkTyrePressures(const KsIniDocument& doc) const {
    const KsIniSection* section = doc.section("TYRES");
    if (!section) return "No TYRES section found";

    QStringList pressures;
    for (const QString& key : {"PRESSURE_FL", "PRESSURE_FR", "PRESSURE_RL", "PRESSURE_RR"}) {
        double val = section->getFloat(key, 32.0);
        if (val < 28 || val > 38) {
            pressures.append(key + "=" + QString::number(val) + " (out of range 28-38 psi)");
        }
    }
    return pressures.isEmpty() ? "" : "Tyre pressures: " + pressures.join("; ");
}

QString CarValidatorWidget::checkSuspensionGeometry(const KsSetupData& setup) const {
    QStringList warnings;
    double avgCamber = (std::abs(setup.frontCamber[0]) + std::abs(setup.frontCamber[1])) / 2.0;
    if (avgCamber > 4.0) warnings.append("Front camber > 4° may cause excessive wear");
    if (setup.rideHeight[0] < 0.08) warnings.append("Front ride height very low (<80mm)");
    if (setup.rideHeight[0] > 0.25) warnings.append("Front ride height very high (>250mm)");
    if (std::abs(setup.toeOut[0] - setup.toeOut[1]) > 0.5) warnings.append("Toe asymmetry > 0.5° front");
    return warnings.isEmpty() ? "" : warnings.join(" | ");
}

QString CarValidatorWidget::checkEngineData(const KsIniDocument& doc) const {
    const KsIniSection* section = doc.section("ENGINE_DATA");
    if (!section) section = doc.section("DATA");
    if (!section) return "No ENGINE_DATA or DATA section found";

    if (!section->hasKey("MAX_POWER")) return "MAX_POWER not defined";
    if (!section->hasKey("MAX_TORQUE")) return "MAX_TORQUE not defined";

    double maxPower = section->getFloat("MAX_POWER", 0);
    double maxTorque = section->getFloat("MAX_TORQUE", 0);

    if (maxPower < 50) return "MAX_POWER seems too low (<50 HP)";
    if (maxPower > 1000) return "MAX_POWER seems unrealistically high (>1000 HP)";
    if (maxTorque < 50) return "MAX_TORQUE seems too low (<50 Nm)";

    return "";
}

QString CarValidatorWidget::checkAeroBalance(const KsSetupData& setup) const {
    QStringList warnings;
    double totalDownforce = setup.frontWing + setup.rearWing;
    if (totalDownforce > 15 && setup.frontWing > setup.rearWing * 1.5) {
        warnings.append("High front aero load relative to rear");
    }
    if (setup.rearWing > 8 && setup.diffusers > 6) {
        warnings.append("High rear aero - check stability at high speed");
    }
    return warnings.isEmpty() ? "" : warnings.join(" | ");
}

QString CarValidatorWidget::checkMassDistribution(const KsSetupData& setup) const {
    if (setup.fuelLevel > 100) {
        return "Fuel level > 100L may affect weight distribution significantly";
    }
    return "";
}

void CarValidatorWidget::populateIssues(const QStringList& errors, const QStringList& warnings) {
    m_issues.clear();
    m_issuesList->clear();

    for (const QString& e : errors) {
        auto* item = new QListWidgetItem("[ERROR] " + e, m_issuesList);
        item->setBackground(QColor("#ffcccc"));
    }
    for (const QString& w : warnings) {
        auto* item = new QListWidgetItem("[WARN] " + w, m_issuesList);
        item->setBackground(QColor("#fff3cd"));
    }

    m_summaryTable->setRowCount(3);
    m_summaryTable->setItem(0, 0, new QTableWidgetItem("Physics"));
    m_summaryTable->setItem(0, 1, new QTableWidgetItem(QString::number(errors.size())));
    m_summaryTable->setItem(0, 2, new QTableWidgetItem(errors.isEmpty() ? "OK" : "FAIL"));
    m_summaryTable->item(0, 2)->setForeground(errors.isEmpty() ? Qt::darkGreen : Qt::red);

    m_summaryTable->setItem(1, 0, new QTableWidgetItem("Setup"));
    m_summaryTable->setItem(1, 1, new QTableWidgetItem(QString::number(warnings.size())));
    m_summaryTable->setItem(1, 2, new QTableWidgetItem(warnings.isEmpty() ? "OK" : "Review"));

    m_summaryTable->setItem(2, 0, new QTableWidgetItem("Total"));
    m_summaryTable->setItem(2, 1, new QTableWidgetItem(
        QString::number(errors.size() + warnings.size())));
    m_summaryTable->setItem(2, 2, new QTableWidgetItem(
        errors.isEmpty() ? "PASS" : "FAIL"));

    m_summaryTable->resizeColumnsToContents();
}

void CarValidatorWidget::onRunValidation() {
    if (!m_lastCarFolder.isEmpty()) {
        validateCar(m_lastCarFolder);
    }
}

void CarValidatorWidget::onExportReport() {
    QString path = QFileDialog::getSaveFileName(this, "Export Report",
        QString(), "Text files (*.txt);;HTML (*.html)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&file);
    if (path.endsWith(".html", Qt::CaseInsensitive)) {
        out << "<html><body><pre>" << m_reportEdit->toPlainText() << "</pre></body></html>";
    } else {
        out << m_reportEdit->toPlainText();
    }
    file.close();
}

void CarValidatorWidget::onFixSuggested(int row) {
    if (row < 0 || row >= m_issuesList->count()) return;

    QListWidgetItem* item = m_issuesList->item(row);
    if (!item) return;

    QString issueText = item->text();
    QString fixMessage;

    if (issueText.contains("tyre pressure", Qt::CaseInsensitive) ||
        issueText.contains("PRESSURE", Qt::CaseInsensitive)) {
        fixMessage = "Reset tyre pressures to default (32 psi)";
    } else if (issueText.contains("camber", Qt::CaseInsensitive)) {
        fixMessage = "Reset camber to -3.5 front / -2.0 rear";
    } else if (issueText.contains("ride height", Qt::CaseInsensitive)) {
        fixMessage = "Reset ride height to 55mm front / 70mm rear";
    } else if (issueText.contains("MAX_POWER", Qt::CaseInsensitive)) {
        fixMessage = "Cannot auto-fix: requires manual engine data entry";
    } else if (issueText.contains("fuel", Qt::CaseInsensitive)) {
        fixMessage = "Reduced fuel to 80L";
    } else {
        fixMessage = "Auto-fix not available for this issue";
    }

    item->setText(issueText + " [FIXED: " + fixMessage + "]");
    item->setBackground(QColor("#d4edda"));
}

// ============================================================================
// TyreTempModelWidget
// ============================================================================

TyreTempModelWidget::TyreTempModelWidget(QWidget* parent)
    : QWidget(parent)
{
    buildUI();
}

void TyreTempModelWidget::buildUI() {
    auto* layout = new QVBoxLayout(this);

    auto* inputRow = new QHBoxLayout;

    auto* paramsBox = new QGroupBox("Simulation Parameters", this);
    auto* paramsForm = new QFormLayout(paramsBox);

    m_ambientInput = new QDoubleSpinBox(this);
    m_ambientInput->setRange(-20, 50);
    m_ambientInput->setValue(25);
    m_ambientInput->setSuffix(" °C");

    m_trackTempInput = new QDoubleSpinBox(this);
    m_trackTempInput->setRange(-20, 80);
    m_trackTempInput->setValue(35);
    m_trackTempInput->setSuffix(" °C");

    m_avgSpeedInput = new QDoubleSpinBox(this);
    m_avgSpeedInput->setRange(50, 350);
    m_avgSpeedInput->setValue(120);
    m_avgSpeedInput->setSuffix(" km/h");

    m_lapsInput = new QSpinBox(this);
    m_lapsInput->setRange(1, 50);
    m_lapsInput->setValue(10);

    paramsForm->addRow("Ambient Temp:", m_ambientInput);
    paramsForm->addRow("Track Temp:", m_trackTempInput);
    paramsForm->addRow("Avg Speed:", m_avgSpeedInput);
    paramsForm->addRow("Laps:", m_lapsInput);

    auto* btnBox = new QVBoxLayout;
    m_simBtn = new QPushButton("Start Simulation", this);
    auto* resetBtn = new QPushButton("Reset", this);
    auto* presetBtn = new QPushButton("Load Track Preset", this);
    btnBox->addWidget(m_simBtn);
    btnBox->addWidget(resetBtn);
    btnBox->addWidget(presetBtn);
    btnBox->addStretch();

    inputRow->addWidget(paramsBox);
    inputRow->addLayout(btnBox);

    m_tempChart = new QChart();
    m_tempChart->setTitle("Tyre Temperature Over Laps");
    m_tempChart->setBackgroundVisible(false);
    m_tempChart->legend()->setVisible(true);

    m_tempFL = new QLineSeries();
    m_tempFL->setName("FL");
    m_tempFR = new QLineSeries();
    m_tempFR->setName("FR");
    m_tempRL = new QLineSeries();
    m_tempRL->setName("RL");
    m_tempRR = new QLineSeries();
    m_tempRR->setName("RR");

    m_tempChart->addSeries(m_tempFL);
    m_tempChart->addSeries(m_tempFR);
    m_tempChart->addSeries(m_tempRL);
    m_tempChart->addSeries(m_tempRR);
    m_tempChart->createDefaultAxes();

    m_tempChartView = new QChartView(m_tempChart, this);
    m_tempChartView->setRenderHint(QPainter::Antialiasing);
    m_tempChartView->setMinimumHeight(240);

    m_tempTable = new QTableWidget(4, 5, this);
    m_tempTable->setHorizontalHeaderLabels({"Lap", "FL", "FR", "RL", "RR"});
    m_tempTable->verticalHeader()->setVisible(false);
    m_tempTable->setAlternatingRowColors(true);
    m_tempTable->setMinimumHeight(100);

    m_statusLabel = new QLabel(tr("Ready"), this);
    m_statusLabel->setStyleSheet("color: gray;");

    layout->addLayout(inputRow);
    layout->addWidget(m_tempChartView);
    layout->addWidget(m_tempTable);
    layout->addWidget(m_statusLabel);

    connect(m_simBtn, &QPushButton::clicked, this, &TyreTempModelWidget::onStartSim);
    connect(resetBtn, &QPushButton::clicked, this, &TyreTempModelWidget::onResetSim);
    connect(presetBtn, &QPushButton::clicked, this, [this] {
        onLoadTrackPreset("monza");
    });
}

void TyreTempModelWidget::loadFromIni(const QString& carFolder) {
    QString tyresIni = carFolder + "/data/tyres.ini";
    if (!QFile::exists(tyresIni)) return;

    KsIniDocument doc;
    doc.load(tyresIni);
    const KsIniSection* section = doc.section("TYRES");
    if (!section) return;

    double baseTemp = section->getFloat("BASE_TEMP", 90);
    m_lastPressure = section->getFloat("PRESSURE_FL", 32.0);

    m_tempsFL.fill(baseTemp, 50);
    m_tempsFR.fill(baseTemp, 50);
    m_tempsRL.fill(baseTemp - 5, 50);
    m_tempsRR.fill(baseTemp - 5, 50);

    m_statusLabel->setText("Loaded from: " + QFileInfo(tyresIni).fileName());
}

void TyreTempModelWidget::simulateLap(double ambient, double trackTemp,
                                       double avgSpeed, int laps) {
    QElapsedTimer timer;
    timer.start();

    double baseTemp = 90;
    double frictionCoeff = 1.0;
    double loadFactor = 1.2;

    int stepCount = 0;
    for (int lap = 0; lap < laps; ++lap) {
        double riseFL = estimateTempRise(avgSpeed, loadFactor, frictionCoeff);
        double riseFR = estimateTempRise(avgSpeed, loadFactor * 0.98, frictionCoeff);
        double riseRL = estimateTempRise(avgSpeed, loadFactor * 1.05, frictionCoeff);
        double riseRR = estimateTempRise(avgSpeed, loadFactor * 1.03, frictionCoeff);

        if (!m_tempsFL.isEmpty()) {
            double lastFL = m_tempsFL.last();
            double lastFR = m_tempsFR.isEmpty() ? lastFL : m_tempsFR.last();
            double lastRL = m_tempsRL.isEmpty() ? lastFL - 5 : m_tempsRL.last();
            double lastRR = m_tempsRR.isEmpty() ? lastFL - 5 : m_tempsRR.last();

            m_tempsFL.append(lastFL + riseFL - estimateCooling(lastFL, ambient));
            m_tempsFR.append(lastFR + riseFR - estimateCooling(lastFR, ambient));
            m_tempsRL.append(lastRL + riseRL - estimateCooling(lastRL, ambient));
            m_tempsRR.append(lastRR + riseRR - estimateCooling(lastRR, ambient));
        }
        ++stepCount;
    }

    m_lastSimDurationMs = timer.elapsed();
    m_lastSimStepCount = stepCount;
    m_avgStepMs = stepCount > 0 ? m_lastSimDurationMs / stepCount : 0;

    updateChart();
    updateTable(laps);
}

double TyreTempModelWidget::estimateTempRise(double speed, double load, double friction) const {
    double speedFactor = speed / 100.0;
    double loadFactor = load / 1.0;
    return (speedFactor * loadFactor * friction * 0.8);
}

double TyreTempModelWidget::estimateCooling(double tyreTemp, double ambient) const {
    return (tyreTemp - ambient) * 0.05;
}

void TyreTempModelWidget::onStartSim() {
    double ambient = m_ambientInput->value();
    double trackTemp = m_trackTempInput->value();
    double avgSpeed = m_avgSpeedInput->value();
    int laps = m_lapsInput->value();

    simulateLap(ambient, trackTemp, avgSpeed, laps);

    m_statusLabel->setText(QString("Simulated %1 laps at %2 km/h | %3 ms total, %4 ms/step")
        .arg(laps).arg(avgSpeed)
        .arg(m_lastSimDurationMs, 0, 'f', 1)
        .arg(m_avgStepMs, 0, 'f', 2));
    m_simBtn->setText("Re-run");

    QVector<double> finalTemps = {
        m_tempsFL.isEmpty() ? 0 : m_tempsFL.last(),
        m_tempsFR.isEmpty() ? 0 : m_tempsFR.last(),
        m_tempsRL.isEmpty() ? 0 : m_tempsRL.last(),
        m_tempsRR.isEmpty() ? 0 : m_tempsRR.last()
    };
    emit simulationComplete(finalTemps);
}

void TyreTempModelWidget::onResetSim() {
    m_tempsFL.clear();
    m_tempsFR.clear();
    m_tempsRL.clear();
    m_tempsRR.clear();
    m_tempFL->clear();
    m_tempFR->clear();
    m_tempRL->clear();
    m_tempRR->clear();
    m_tempTable->setRowCount(0);
    m_statusLabel->setText("Reset");
    m_simBtn->setText("Start Simulation");
}

void TyreTempModelWidget::onLoadTrackPreset(const QString& track) {
    if (track == "monza") {
        m_ambientInput->setValue(28);
        m_trackTempInput->setValue(40);
        m_avgSpeedInput->setValue(220);
    } else if (track == "nurburgring") {
        m_ambientInput->setValue(18);
        m_trackTempInput->setValue(25);
        m_avgSpeedInput->setValue(160);
    } else if (track == "suzuka") {
        m_ambientInput->setValue(25);
        m_trackTempInput->setValue(35);
        m_avgSpeedInput->setValue(180);
    }
}

void TyreTempModelWidget::updateChart() {
    m_tempFL->clear();
    m_tempFR->clear();
    m_tempRL->clear();
    m_tempRR->clear();

    for (int i = 0; i < std::min<qsizetype>(m_tempsFL.size(), 200); ++i) {
        m_tempFL->append(i, i < m_tempsFL.size() ? m_tempsFL[i] : 0);
    }
    for (int i = 0; i < std::min<qsizetype>(m_tempsFR.size(), 200); ++i) {
        m_tempFR->append(i, i < m_tempsFR.size() ? m_tempsFR[i] : 0);
    }
    for (int i = 0; i < std::min<qsizetype>(m_tempsRL.size(), 200); ++i) {
        m_tempRL->append(i, i < m_tempsRL.size() ? m_tempsRL[i] : 0);
    }
    for (int i = 0; i < std::min<qsizetype>(m_tempsRR.size(), 200); ++i) {
        m_tempRR->append(i, i < m_tempsRR.size() ? m_tempsRR[i] : 0);
    }
}

void TyreTempModelWidget::updateTable(int laps) {
    m_tempTable->setRowCount(std::min<int>(laps, 20));
    for (int i = 0; i < std::min<int>(laps, 20); ++i) {
        m_tempTable->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
        auto setTemp = [&](int col, const QVector<double>& temps) {
            if (i < temps.size())
                m_tempTable->setItem(i, col, new QTableWidgetItem(QString::number(temps[i], 'f', 1)));
            else
                m_tempTable->setItem(i, col, new QTableWidgetItem("--"));
        };
        setTemp(1, m_tempsFL);
        setTemp(2, m_tempsFR);
        setTemp(3, m_tempsRL);
        setTemp(4, m_tempsRR);
    }
    m_tempTable->resizeColumnsToContents();
}

// ============================================================================
// PhysicsEditorModule — new toolbar and content additions
// ============================================================================

void PhysicsEditorModule::onShowSuspGeometry() {
    m_contentStack->setCurrentWidget(m_suspGeometry);
}

void PhysicsEditorModule::onShowFfbPreview() {
    m_contentStack->setCurrentWidget(m_ffbPreview);
}

void PhysicsEditorModule::onShowCarValidator() {
    if (!m_currentCar.isEmpty()) {
        m_validator->validateCar(m_currentCar);
    }
    m_contentStack->setCurrentWidget(m_validator);
}

void PhysicsEditorModule::onShowTyreTempModel() {
    if (!m_currentCar.isEmpty()) {
        m_tyreTempModel->loadFromIni(m_currentCar);
    }
    m_contentStack->setCurrentWidget(m_tyreTempModel);
}

void PhysicsEditorModule::onShowTireCurveEditor() {
    if (!m_currentCar.isEmpty()) {
        m_tireCurveEditor->loadFromIni(m_currentCar);
    }
    m_contentStack->setCurrentWidget(m_tireCurveEditor);
}

QJsonObject PhysicsEditorModule::serializeProject() const
{
    QJsonObject data;
    data["currentCar"] = m_currentCar;
    data["currentFile"] = m_currentFile;
    data["carsPath"] = m_carsPath;
    return data;
}

void PhysicsEditorModule::deserializeProject(const QJsonObject& data)
{
    m_carsPath = data["carsPath"].toString();
    m_currentCar = data["currentCar"].toString();
    m_currentFile = data["currentFile"].toString();

    if (!m_currentCar.isEmpty()) {
        onCarSelected(m_currentCar);
    }
    if (!m_currentFile.isEmpty()) {
        onFileSelected(m_currentFile);
    }
}

}