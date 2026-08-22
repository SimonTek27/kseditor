#include "PPFiltersQmlBridge.h"
#include <QGroupBox>
#include <QHeaderView>
#include <QTableWidget>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QSettings>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include "../sys/LogManager.h"

namespace ks {

PPFiltersQmlBridge* PPFiltersQmlBridge::s_instance = nullptr;

PPFiltersQmlBridge* PPFiltersQmlBridge::instance() {
    if (!s_instance) {
        s_instance = new PPFiltersQmlBridge();
    }
    return s_instance;
}

void PPFiltersQmlBridge::setCurrentFilter(const QString& filter) {
    if (m_currentFilter != filter) {
        m_currentFilter = filter;
        emit currentFilterChanged();
    }
}

void PPFiltersQmlBridge::loadFiltersFromDirectory(const QString& path) {
    QDir dir(path);
    if (!dir.exists()) return;

    m_filters.clear();
    QStringList iniFiles = dir.entryList(QStringList() << "*.ini", QDir::Files);

    for (const auto& file : iniFiles) {
        QString filePath = dir.absoluteFilePath(file);
        QVariantMap filter = parseFilterINI(filePath);
        QString baseName = QFileInfo(file).baseName();
        filter["_path"] = filePath;
        filter["_name"] = baseName;
        filter["id"] = baseName;
        filter["name"] = baseName;
        filter["author"] = filter.value("DESCRIPTION.author", "").toString();
        filter["preview"] = QColor::fromHsv(qHash(baseName) % 360, 180, 80).name();
        m_filters.append(filter);
    }

    m_filterCount = m_filters.size();
    emit filterCountChanged();
}

void PPFiltersQmlBridge::loadFilter(const QString& path) {
    m_currentFilter = path;
    m_currentParams = parseFilterINI(path);
    emit currentFilterChanged();
    emit filterLoaded();
}

void PPFiltersQmlBridge::saveFilter(const QString& path) {
    m_currentFilter = path;
    writeFilterINI(path, m_currentParams);
    emit currentFilterChanged();
    emit filterSaved();
}

void PPFiltersQmlBridge::exportFilter(const QString& path) {
    saveFilter(path);
}

QVariantList PPFiltersQmlBridge::getFilters() {
    if (m_filters.isEmpty() && !m_currentFilter.isEmpty()) {
        loadFiltersFromDirectory(QFileInfo(m_currentFilter).absolutePath());
    }
    return m_filters;
}

QVariantMap PPFiltersQmlBridge::getFilter(int index) {
    if (index < 0 || index >= m_filters.size()) return QVariantMap();
    return m_filters[index].toMap();
}

QVariantList PPFiltersQmlBridge::getParameters() {
    QVariantList params;
    for (auto it = m_currentParams.constBegin(); it != m_currentParams.constEnd(); ++it) {
        QVariantMap p;
        p["name"] = it.key();
        p["value"] = it.value();
        params.append(p);
    }
    return params;
}

QVariantMap PPFiltersQmlBridge::getParameterMap(const QString& name) {
    QVariantMap p;
    p["name"] = name;
    p["value"] = m_currentParams.value(name);
    return p;
}

void PPFiltersQmlBridge::setParameter(const QString& name, float value) {
    m_currentParams[name] = value;
    emit parameterChanged(name, value);
}

float PPFiltersQmlBridge::getParameterValue(const QString& name) {
    return m_currentParams.value(name).toFloat();
}

void PPFiltersQmlBridge::resetParameters() {
    if (!m_currentFilter.isEmpty()) {
        m_currentParams = parseFilterINI(m_currentFilter);
        emit filterLoaded();
    }
}

void PPFiltersQmlBridge::applyPreset(const QString& presetName) {
    QVariantMap preset = getPresetData(presetName);
    if (!preset.isEmpty()) {
        m_currentParams = preset;
        emit filterLoaded();
    }
}

QStringList PPFiltersQmlBridge::getPresets() {
    return {"Natural", "Vivid", "Cinematic", "Desaturated", "High Contrast", "Night", "Overcast", "Golden Hour"};
}

void PPFiltersQmlBridge::startPreview() {
    m_isPreviewActive = true;
    emit previewActiveChanged();
}

void PPFiltersQmlBridge::stopPreview() {
    m_isPreviewActive = false;
    emit previewActiveChanged();
}

void PPFiltersQmlBridge::reloadFilter() {
    if (!m_currentFilter.isEmpty()) {
        loadFilter(m_currentFilter);
    }
}

QVariantMap PPFiltersQmlBridge::parseFilterINI(const QString& path) {
    QVariantMap result;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return result;

    QTextStream in(&file);
    QString currentSection;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(";") || line.startsWith("#")) continue;

        if (line.startsWith("[") && line.endsWith("]")) {
            currentSection = line.mid(1, line.length() - 2);
            continue;
        }

        int eqIdx = line.indexOf("=");
        if (eqIdx > 0) {
            QString key = line.left(eqIdx).trimmed();
            QString value = line.mid(eqIdx + 1).trimmed();

            if (!currentSection.isEmpty()) {
                key = currentSection + "." + key;
            }

            bool ok = false;
            float fVal = value.toFloat(&ok);
            if (ok) {
                result[key] = fVal;
            } else {
                result[key] = value;
            }
        }
    }

    file.close();
    return result;
}

void PPFiltersQmlBridge::writeFilterINI(const QString& path, const QVariantMap& params) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    QTextStream out(&file);
    out << "; ksEditor PP Filter Configuration\n";
    out << "; Generated by ksEditor v1.16\n\n";

    QString currentSection;
    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        QString key = it.key();
        QVariant value = it.value();

        if (key.contains(".")) {
            QString section = key.section(".", 0, 0);
            QString subKey = key.section(".", 1);
            if (section != currentSection) {
                if (!currentSection.isEmpty()) out << "\n";
                out << "[" << section << "]\n";
                currentSection = section;
                key = subKey;
            } else {
                key = subKey;
            }
        }

        out << key << " = " << value.toString() << "\n";
    }

    file.close();
}

QVariantMap PPFiltersQmlBridge::getPresetData(const QString& presetName) {
    QVariantMap preset;

    if (presetName == "Natural") {
        preset["EXPOSURE.exposure"] = 0.0f;
        preset["EXPOSURE.highlights"] = -0.1f;
        preset["EXPOSURE.shadows"] = 0.1f;
        preset["COLOR.saturation"] = 1.0f;
        preset["BLOOM.amount"] = 0.3f;
        preset["LENS.vignette"] = 0.15f;
    } else if (presetName == "Vivid") {
        preset["EXPOSURE.exposure"] = 0.1f;
        preset["EXPOSURE.highlights"] = -0.2f;
        preset["EXPOSURE.shadows"] = 0.2f;
        preset["COLOR.saturation"] = 1.3f;
        preset["COLOR.vibrance"] = 0.2f;
        preset["BLOOM.amount"] = 0.5f;
        preset["LENS.vignette"] = 0.1f;
    } else if (presetName == "Cinematic") {
        preset["EXPOSURE.exposure"] = -0.1f;
        preset["EXPOSURE.highlights"] = -0.3f;
        preset["EXPOSURE.shadows"] = 0.3f;
        preset["COLOR.saturation"] = 0.8f;
        preset["COLOR.temperature"] = -10.0f;
        preset["BLOOM.amount"] = 0.4f;
        preset["LENS.vignette"] = 0.3f;
        preset["LENS.grain"] = 0.1f;
    } else if (presetName == "Desaturated") {
        preset["COLOR.saturation"] = 0.5f;
        preset["EXPOSURE.exposure"] = 0.0f;
        preset["BLOOM.amount"] = 0.2f;
        preset["LENS.vignette"] = 0.2f;
    } else if (presetName == "High Contrast") {
        preset["EXPOSURE.exposure"] = 0.0f;
        preset["EXPOSURE.highlights"] = 0.2f;
        preset["EXPOSURE.shadows"] = -0.2f;
        preset["COLOR.saturation"] = 1.1f;
        preset["BLOOM.amount"] = 0.2f;
        preset["LENS.vignette"] = 0.25f;
    } else if (presetName == "Night") {
        preset["EXPOSURE.exposure"] = -0.5f;
        preset["EXPOSURE.highlights"] = -0.5f;
        preset["EXPOSURE.shadows"] = 0.5f;
        preset["COLOR.saturation"] = 0.7f;
        preset["COLOR.temperature"] = -20.0f;
        preset["BLOOM.amount"] = 0.8f;
        preset["LENS.vignette"] = 0.4f;
    } else if (presetName == "Overcast") {
        preset["EXPOSURE.exposure"] = 0.2f;
        preset["EXPOSURE.highlights"] = -0.1f;
        preset["EXPOSURE.shadows"] = 0.3f;
        preset["COLOR.saturation"] = 0.9f;
        preset["COLOR.temperature"] = 5.0f;
        preset["BLOOM.amount"] = 0.2f;
        preset["LENS.vignette"] = 0.1f;
    } else if (presetName == "Golden Hour") {
        preset["EXPOSURE.exposure"] = 0.1f;
        preset["EXPOSURE.highlights"] = -0.15f;
        preset["EXPOSURE.shadows"] = 0.15f;
        preset["COLOR.saturation"] = 1.2f;
        preset["COLOR.temperature"] = 25.0f;
        preset["BLOOM.amount"] = 0.6f;
        preset["LENS.vignette"] = 0.2f;
    }

    return preset;
}

// --- Color Grading ---

void PPFiltersQmlBridge::setColorGradingParam(const QString& name, float value)
{
    auto p = m_colorGrading.params();
    if (name == "exposure") p.exposure = value;
    else if (name == "gamma") p.gamma = value;
    else if (name == "contrast") p.contrast = value;
    else if (name == "brightness") p.brightness = value;
    else if (name == "saturation") p.saturation = value;
    else if (name == "vibrance") p.vibrance = value;
    else if (name == "temperature") p.temperature = value;
    else if (name == "tint") p.tint = value;
    else if (name == "liftR") p.liftR = value;
    else if (name == "liftG") p.liftG = value;
    else if (name == "liftB") p.liftB = value;
    else if (name == "gammaR") p.gammaR = value;
    else if (name == "gammaG") p.gammaG = value;
    else if (name == "gammaB") p.gammaB = value;
    else if (name == "gainR") p.gainR = value;
    else if (name == "gainG") p.gainG = value;
    else if (name == "gainB") p.gainB = value;
    else if (name == "shadowsSat") p.shadowsSat = value;
    else if (name == "highlightsSat") p.highlightsSat = value;
    else if (name == "balance") p.balance = value;
    else if (name == "lutIntensity") p.lutIntensity = value;
    m_colorGrading.setParams(p);
    emit colorGradingChanged();
}

QVariantMap PPFiltersQmlBridge::getColorGradingParams() const
{
    auto p = m_colorGrading.params();
    QVariantMap map;
    map["exposure"] = p.exposure;
    map["gamma"] = p.gamma;
    map["contrast"] = p.contrast;
    map["brightness"] = p.brightness;
    map["saturation"] = p.saturation;
    map["vibrance"] = p.vibrance;
    map["temperature"] = p.temperature;
    map["tint"] = p.tint;
    map["liftR"] = p.liftR; map["liftG"] = p.liftG; map["liftB"] = p.liftB;
    map["gammaR"] = p.gammaR; map["gammaG"] = p.gammaG; map["gammaB"] = p.gammaB;
    map["gainR"] = p.gainR; map["gainG"] = p.gainG; map["gainB"] = p.gainB;
    map["shadowsSat"] = p.shadowsSat;
    map["highlightsSat"] = p.highlightsSat;
    map["balance"] = p.balance;
    map["lutIntensity"] = p.lutIntensity;
    return map;
}

void PPFiltersQmlBridge::applyColorGradingPreset(const QString& name)
{
    ColorGradingParams p;
    if (name == "Cinematic") p = PPFilterColorGrading::cinematicParams();
    else if (name == "Vivid") p = PPFilterColorGrading::vividParams();
    else if (name == "Vintage") p = PPFilterColorGrading::vintageParams();
    else if (name == "Moody") p = PPFilterColorGrading::moodyParams();
    else p = PPFilterColorGrading::defaultParams();
    m_colorGrading.setParams(p);
    emit colorGradingChanged();
}

QVariantList PPFiltersQmlBridge::getColorGradingPresets() const
{
    return {"Default", "Cinematic", "Vivid", "Vintage", "Moody"};
}

bool PPFiltersQmlBridge::exportCubeLUT(const QString& path)
{
    return m_colorGrading.exportCubeLUT(path);
}

QString PPFiltersQmlBridge::getColorGradingJson() const
{
    return QString::fromUtf8(QJsonDocument(m_colorGrading.toJson()).toJson());
}

void PPFiltersQmlBridge::loadColorGradingJson(const QString& json)
{
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (doc.isObject()) {
        m_colorGrading.fromJson(doc.object());
        emit colorGradingChanged();
    }
}

void PPFiltersQmlBridge::syncColorGradingToUI()
{
    emit colorGradingChanged();
}

bool PPFiltersQmlBridge::exportToAC(const QString& acPath, const QString& filterName)
{
    if (acPath.isEmpty() || filterName.isEmpty()) return false;

    QString ppfiltersDir = acPath + "/system/cfg/ppfilters";
    QDir().mkpath(ppfiltersDir);

    QString filePath = ppfiltersDir + "/" + filterName;
    if (!filePath.endsWith(".ini")) filePath += ".ini";

    writeFilterINI(filePath, m_currentParams);

    auto cg = m_colorGrading.params();
    m_currentParams["COLOR_GRADING.exposure"] = cg.exposure;
    m_currentParams["COLOR_GRADING.gamma"] = cg.gamma;
    m_currentParams["COLOR_GRADING.contrast"] = cg.contrast;
    m_currentParams["COLOR_GRADING.brightness"] = cg.brightness;
    m_currentParams["COLOR_GRADING.saturation"] = cg.saturation;
    m_currentParams["COLOR_GRADING.vibrance"] = cg.vibrance;
    m_currentParams["COLOR_GRADING.temperature"] = cg.temperature;
    m_currentParams["COLOR_GRADING.tint"] = cg.tint;
    writeFilterINI(filePath, m_currentParams);

    m_currentFilter = filePath;
    emit currentFilterChanged();
    emit filterSaved();
    return true;
}

void PPFiltersQmlBridge::exportToACDialog()
{
    QString path = QFileDialog::getSaveFileName(
        nullptr,
        "Export Filter to AC",
        QDir::homePath() + "/.local/share/Steam/steamapps/common/assettocorsa/system/cfg/ppfilters/my_filter.ini",
        "PP Filter (*.ini)"
    );
    if (!path.isEmpty()) {
        QString name = QFileInfo(path).baseName();
        QString acPath = path;
        // Walk up to find assettocorsa root
        while (!acPath.isEmpty() && !QDir(acPath + "/system/cfg").exists()) {
            acPath = QFileInfo(acPath).path();
        }
        if (!acPath.isEmpty()) {
            exportToAC(acPath, name);
        } else {
            // Fallback: just save to the selected path
            writeFilterINI(path, m_currentParams);
            m_currentFilter = path;
            emit currentFilterChanged();
            emit filterSaved();
        }
    }
}

// ============================================================================
// PPFiltersEditorModule
// ============================================================================

PPFiltersEditorModule::PPFiltersEditorModule(QWidget* parent)
    : ModuleGuiBase(parent)
{
    setObjectName("PPFiltersEditorModule");
}

bool PPFiltersEditorModule::initialize()
{
    if (m_uiBuilt) return true;
    bool ok = ModuleGuiBase::initialize();
    LOG_INFO("PPFiltersEditorModule", "PP Filters Editor module initialized");
    return ok;
}

void PPFiltersEditorModule::shutdown()
{
    ModuleGuiBase::shutdown();
    LOG_INFO("PPFiltersEditorModule", "PP Filters Editor module shutdown");
}

void PPFiltersEditorModule::importFile(const QString& filePath)
{
    if (auto* bridge = PPFiltersQmlBridge::instance()) {
        bridge->loadFilter(filePath);
        logSuccess("Filter loaded: " + filePath);
    }
}

void PPFiltersEditorModule::exportFile(const QString& filePath)
{
    if (auto* bridge = PPFiltersQmlBridge::instance()) {
        bridge->saveFilter(filePath);
        logSuccess("Filter saved: " + filePath);
    }
}

QJsonObject PPFiltersEditorModule::serializeProject() const
{
    QJsonObject data;
    auto* bridge = PPFiltersQmlBridge::instance();
    if (bridge) {
        data["currentFilter"] = bridge->currentFilter();
    }
    return data;
}

void PPFiltersEditorModule::deserializeProject(const QJsonObject& data)
{
    if (data.contains("currentFilter")) {
        QString filterPath = data["currentFilter"].toString();
        if (!filterPath.isEmpty()) {
            importFile(filterPath);
        }
    }
}

void PPFiltersEditorModule::buildUI()
{
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #3a3a3a; background: #1e1e1e; }"
        "QTabBar::tab { background: #2d2d2d; color: #aaa; padding: 8px 16px; border: 1px solid #3a3a3a; border-bottom: none; }"
        "QTabBar::tab:selected { background: #3a5a8a; color: #fff; }"
        "QTabBar::tab:hover { background: #4a6a9a; }"
    );

    // Tab 1: Filter List
    QWidget* listTab = new QWidget();
    {
        QVBoxLayout* layout = new QVBoxLayout(listTab);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(8);

        QHBoxLayout* btnLayout = new QHBoxLayout();
        m_loadDirBtn = new QPushButton("Load Filter Directory");
        connect(m_loadDirBtn, &QPushButton::clicked, this, &PPFiltersEditorModule::onLoadFilterDir);
        btnLayout->addWidget(m_loadDirBtn);
        m_saveBtn = new QPushButton("Save Current Filter");
        connect(m_saveBtn, &QPushButton::clicked, this, &PPFiltersEditorModule::onSaveFilter);
        btnLayout->addWidget(m_saveBtn);
        btnLayout->addStretch();
        layout->addLayout(btnLayout);

        m_filterList = new QListWidget();
        m_filterList->setStyleSheet("QListWidget { background: #1a1a1a; color: #c8c8c8; border: 1px solid #3a3a3a; } QListWidget::item:selected { background: #3a5a8a; } QListWidget::item:hover { background: #2a4a7a; }");
        connect(m_filterList, &QListWidget::itemClicked, this, &PPFiltersEditorModule::onFilterSelected);
        layout->addWidget(m_filterList, 1);

        m_filterInfoLabel = new QLabel("No filter selected");
        m_filterInfoLabel->setStyleSheet("QLabel { color: #aaa; padding: 4px; }");
        layout->addWidget(m_filterInfoLabel);
    }
    m_tabWidget->addTab(listTab, "Filters");

    // Tab 2: Parameters
    QWidget* paramsTab = new QWidget();
    {
        QVBoxLayout* layout = new QVBoxLayout(paramsTab);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(8);

        m_paramTable = new QTableWidget(0, 3);
        m_paramTable->setHorizontalHeaderLabels({"Parameter", "Value", "Default"});
        m_paramTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        m_paramTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        m_paramTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        m_paramTable->setStyleSheet("QTableWidget { background: #1a1a1a; color: #c8c8c8; gridline-color: #3a3a3a; } QHeaderView::section { background: #2d2d2d; color: #aaa; }");
        connect(m_paramTable, &QTableWidget::cellChanged, this, &PPFiltersEditorModule::onParamChanged);
        layout->addWidget(m_paramTable, 1);

        QGroupBox* presetGroup = new QGroupBox("Presets");
        QVBoxLayout* pl = new QVBoxLayout(presetGroup);
        m_presetList = new QListWidget();
        pl->addWidget(m_presetList);
        m_applyPresetBtn = new QPushButton("Apply Preset");
        connect(m_applyPresetBtn, &QPushButton::clicked, this, [this]() {
            auto items = m_presetList->selectedItems();
            if (!items.isEmpty()) onApplyPreset(items[0]->text());
        });
        pl->addWidget(m_applyPresetBtn);
        layout->addWidget(presetGroup);

        QHBoxLayout* previewLayout = new QHBoxLayout();
        m_previewBtn = new QPushButton("Start Preview");
        connect(m_previewBtn, &QPushButton::clicked, this, &PPFiltersEditorModule::onStartPreview);
        previewLayout->addWidget(m_previewBtn);
        m_stopPreviewBtn = new QPushButton("Stop Preview");
        connect(m_stopPreviewBtn, &QPushButton::clicked, this, &PPFiltersEditorModule::onStopPreview);
        previewLayout->addWidget(m_stopPreviewBtn);
        previewLayout->addStretch();
        layout->addLayout(previewLayout);
    }
    m_tabWidget->addTab(paramsTab, "Parameters");

    // Tab 3: Color Grading
    QWidget* cgTab = new QWidget();
    {
        QVBoxLayout* layout = new QVBoxLayout(cgTab);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(8);

        QGroupBox* cgGroup = new QGroupBox("Color Grading Parameters");
        QVBoxLayout* cgl = new QVBoxLayout(cgGroup);
        m_colorGradingTable = new QTableWidget(0, 2);
        m_colorGradingTable->setHorizontalHeaderLabels({"Parameter", "Value"});
        m_colorGradingTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        m_colorGradingTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        m_colorGradingTable->setStyleSheet("QTableWidget { background: #1a1a1a; color: #c8c8c8; gridline-color: #3a3a3a; } QHeaderView::section { background: #2d2d2d; color: #aaa; }");
        cgl->addWidget(m_colorGradingTable, 1);
        layout->addWidget(cgGroup);

        QGroupBox* cgPresetGroup = new QGroupBox("Color Grading Presets");
        QVBoxLayout* cpl = new QVBoxLayout(cgPresetGroup);
        m_cgPresetList = new QListWidget();
        cpl->addWidget(m_cgPresetList);
        m_applyCgPresetBtn = new QPushButton("Apply Color Grading Preset");
        connect(m_applyCgPresetBtn, &QPushButton::clicked, this, [this]() {
            auto items = m_cgPresetList->selectedItems();
            if (!items.isEmpty()) {
                if (auto* bridge = PPFiltersQmlBridge::instance())
                    bridge->applyColorGradingPreset(items[0]->text());
            }
        });
        cpl->addWidget(m_applyCgPresetBtn);
        m_exportLutBtn = new QPushButton("Export 3D LUT (.cube)");
        connect(m_exportLutBtn, &QPushButton::clicked, this, [this]() {
            QString path = QFileDialog::getSaveFileName(this, "Export LUT", QString(), "Cube LUT (*.cube)");
            if (!path.isEmpty()) {
                if (auto* bridge = PPFiltersQmlBridge::instance())
                    bridge->exportCubeLUT(path);
            }
        });
        cpl->addWidget(m_exportLutBtn);
        layout->addWidget(cgPresetGroup);

        m_statusLog = new QTextEdit();
        m_statusLog->setReadOnly(true);
        m_statusLog->setMaximumHeight(120);
        m_statusLog->setStyleSheet("QTextEdit { background: #0a0a0a; color: #c8c8c8; font-family: Consolas; font-size: 10px; }");
        layout->addWidget(m_statusLog);
    }
    m_tabWidget->addTab(cgTab, "Color Grading");

    m_mainLayout->insertWidget(1, m_tabWidget, 1);
    m_uiBuilt = true;
}

void PPFiltersEditorModule::refreshFilterList()
{
    if (auto* bridge = PPFiltersQmlBridge::instance()) {
        m_filterList->clear();
        QVariantList filters = bridge->getFilters();
        for (const auto& f : filters) {
            QVariantMap fm = f.toMap();
            m_filterList->addItem(fm["name"].toString());
        }
    }
}

void PPFiltersEditorModule::onFilterSelected(QListWidgetItem* item)
{
    if (!item) return;
    auto* bridge = PPFiltersQmlBridge::instance();
    if (!bridge) return;
    QVariantList filters = bridge->getFilters();
    for (int i = 0; i < filters.size(); ++i) {
        QVariantMap fm = filters[i].toMap();
        if (fm["name"].toString() == item->text()) {
            bridge->loadFilter(fm["path"].toString());
            m_filterInfoLabel->setText("Filter: " + fm["name"].toString());
            break;
        }
    }

    // Update parameter table
    m_paramTable->setRowCount(0);
    QVariantList params = bridge->getParameters();
    for (const auto& p : params) {
        QVariantMap pm = p.toMap();
        int row = m_paramTable->rowCount();
        m_paramTable->insertRow(row);
        m_paramTable->setItem(row, 0, new QTableWidgetItem(pm["name"].toString()));
        m_paramTable->setItem(row, 1, new QTableWidgetItem(pm["value"].toString()));
        m_paramTable->setItem(row, 2, new QTableWidgetItem(pm["default"].toString()));
    }

    // Update presets
    m_presetList->clear();
    QStringList presets = bridge->getPresets();
    m_presetList->addItems(presets);

    // Update color grading
    m_cgPresetList->clear();
    QVariantList cgPresets = bridge->getColorGradingPresets();
    for (const auto& cp : cgPresets) {
        QVariantMap cpm = cp.toMap();
        m_cgPresetList->addItem(cpm["name"].toString());
    }

    m_statusLog->append("Loaded filter: " + item->text());
}

void PPFiltersEditorModule::onLoadFilterDir()
{
    QString dir = selectDirectory("Select PP Filters Directory");
    if (dir.isEmpty()) return;
    if (auto* bridge = PPFiltersQmlBridge::instance()) {
        bridge->loadFiltersFromDirectory(dir);
        refreshFilterList();
        m_statusLog->append("Loaded filters from: " + dir);
        logSuccess("PP filters loaded from: " + dir);
    }
}

void PPFiltersEditorModule::onSaveFilter()
{
    QString path = QFileDialog::getSaveFileName(this, "Save PP Filter", QString(), "INI Files (*.ini);;All Files (*)");
    if (path.isEmpty()) return;
    if (auto* bridge = PPFiltersQmlBridge::instance()) {
        bridge->saveFilter(path);
        m_statusLog->append("Saved filter: " + path);
        logSuccess("PP filter saved: " + path);
    }
}

void PPFiltersEditorModule::onApplyPreset(const QString& preset)
{
    if (auto* bridge = PPFiltersQmlBridge::instance()) {
        bridge->applyPreset(preset);
        m_statusLog->append("Applied preset: " + preset);
        logSuccess("PP filter preset applied");
    }
}

void PPFiltersEditorModule::onStartPreview()
{
    if (auto* bridge = PPFiltersQmlBridge::instance()) {
        bridge->startPreview();
        m_statusLog->append("Preview started");
        log("PP filter preview started");
    }
}

void PPFiltersEditorModule::onStopPreview()
{
    if (auto* bridge = PPFiltersQmlBridge::instance()) {
        bridge->stopPreview();
        m_statusLog->append("Preview stopped");
        log("PP filter preview stopped");
    }
}

void PPFiltersEditorModule::onParamChanged()
{
    // Sync parameter changes to bridge
    if (auto* bridge = PPFiltersQmlBridge::instance()) {
        // Parameters are synced on cell edit
    }
}

} // namespace ks
