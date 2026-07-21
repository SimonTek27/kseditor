#include "FfbEditorQmlBridge.h"
#include "FfbConfigTool.h"
#include "../sys/LogManager.h"
#include <QGroupBox>
#include <QFormLayout>
#include <QSlider>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

namespace ks {

FfbEditorQmlBridge* FfbEditorQmlBridge::s_instance = nullptr;

FfbEditorQmlBridge* FfbEditorQmlBridge::instance() {
    if (!s_instance) {
        s_instance = new FfbEditorQmlBridge();
    }
    return s_instance;
}

void FfbEditorQmlBridge::setGain(float v) {
    if (qFuzzyCompare(m_settings.gain, v)) return;
    m_settings.gain = v;
    markUnsaved();
    emit settingsChanged();
}

void FfbEditorQmlBridge::setFilter(float v) {
    if (qFuzzyCompare(m_settings.filter, v)) return;
    m_settings.filter = v;
    markUnsaved();
    emit settingsChanged();
}

void FfbEditorQmlBridge::setMinimumForce(float v) {
    if (qFuzzyCompare(m_settings.minimumForce, v)) return;
    m_settings.minimumForce = v;
    markUnsaved();
    emit settingsChanged();
}

void FfbEditorQmlBridge::setKerbEffect(float v) {
    if (qFuzzyCompare(m_settings.kerbEffect, v)) return;
    m_settings.kerbEffect = v;
    markUnsaved();
    emit settingsChanged();
}

void FfbEditorQmlBridge::setRoadEffect(float v) {
    if (qFuzzyCompare(m_settings.roadEffect, v)) return;
    m_settings.roadEffect = v;
    markUnsaved();
    emit settingsChanged();
}

void FfbEditorQmlBridge::setSlipEffect(float v) {
    if (qFuzzyCompare(m_settings.slipEffect, v)) return;
    m_settings.slipEffect = v;
    markUnsaved();
    emit settingsChanged();
}

void FfbEditorQmlBridge::setAbsEffect(float v) {
    if (qFuzzyCompare(m_settings.absEffect, v)) return;
    m_settings.absEffect = v;
    markUnsaved();
    emit settingsChanged();
}

void FfbEditorQmlBridge::setEnhUndersteer(float v) {
    if (qFuzzyCompare(m_settings.enhUndersteer, v)) return;
    m_settings.enhUndersteer = v;
    markUnsaved();
    emit settingsChanged();
}

void FfbEditorQmlBridge::setCentreBoostGain(float v) {
    if (qFuzzyCompare(m_settings.centreBoostGain, v)) return;
    m_settings.centreBoostGain = v;
    markUnsaved();
    emit settingsChanged();
}

void FfbEditorQmlBridge::setCentreBoostRange(float v) {
    if (qFuzzyCompare(m_settings.centreBoostRange, v)) return;
    m_settings.centreBoostRange = v;
    markUnsaved();
    emit settingsChanged();
}

void FfbEditorQmlBridge::setEnableGyro(bool v) {
    if (m_settings.enableGyro == v) return;
    m_settings.enableGyro = v;
    markUnsaved();
    emit settingsChanged();
}

void FfbEditorQmlBridge::setGyroStrength(float v) {
    if (qFuzzyCompare(m_settings.gyroStrength, v)) return;
    m_settings.gyroStrength = v;
    markUnsaved();
    emit settingsChanged();
}

bool FfbEditorQmlBridge::loadSettings(const QString& path) {
    m_settings = FfbConfigTool::loadSettings(path);
    m_currentFile = path;
    m_hasUnsavedChanges = false;
    emit currentFileChanged();
    emit unsavedChangesChanged();
    emit settingsChanged();
    emit settingsLoaded(path);
    LOG_INFO("FfbEditorQmlBridge", QString("Loaded settings from: %1").arg(path));
    return true;
}

bool FfbEditorQmlBridge::saveSettings(const QString& path) {
    QString savePath = path.isEmpty() ? m_currentFile : path;
    if (savePath.isEmpty()) {
        LOG_ERROR("FfbEditorQmlBridge", "No path specified for save");
        return false;
    }

    if (!FfbConfigTool::saveSettings(m_settings, savePath)) {
        return false;
    }

    m_currentFile = savePath;
    m_hasUnsavedChanges = false;
    emit currentFileChanged();
    emit unsavedChangesChanged();
    emit settingsSaved(savePath);
    return true;
}

bool FfbEditorQmlBridge::loadSettingsFromAc(const QString& acPath) {
    QString settingsPath = acPath + "/cfg/cfg.ini";
    return loadSettings(settingsPath);
}

bool FfbEditorQmlBridge::saveSettingsToAc(const QString& acPath) {
    QString settingsPath = acPath + "/cfg/cfg.ini";
    return saveSettings(settingsPath);
}

QVariantList FfbEditorQmlBridge::getPresets() {
    QVariantList result;
    auto presets = FfbConfigTool::getPresets();
    for (const auto& preset : presets) {
        QVariantMap m;
        m["name"] = preset.name;
        m["wheelModel"] = preset.wheelModel;
        m["manufacturer"] = preset.manufacturer;
        m["description"] = preset.description;
        m["isBuiltIn"] = preset.isBuiltIn;
        result.append(m);
    }
    return result;
}

bool FfbEditorQmlBridge::applyPreset(const QString& presetName) {
    FfbConfigTool::WheelPreset preset = FfbConfigTool::getPreset(presetName);
    if (preset.name.isEmpty()) {
        emit validationFailed(QString("Preset not found: %1").arg(presetName));
        return false;
    }

    FfbConfigTool::applyPreset(m_settings, preset);
    markUnsaved();
    emit settingsChanged();
    emit presetApplied(presetName);
    return true;
}

bool FfbEditorQmlBridge::saveCustomPreset(const QString& name, const QString& wheelModel, const QString& manufacturer, const QString& description) {
    FfbConfigTool::WheelPreset preset;
    preset.name = name;
    preset.wheelModel = wheelModel;
    preset.manufacturer = manufacturer;
    preset.description = description;
    preset.isBuiltIn = false;
    preset.settings = m_settings;

    return FfbConfigTool::savePreset(preset);
}

QVariantMap FfbEditorQmlBridge::getPresetSettings(const QString& presetName) {
    FfbConfigTool::WheelPreset preset = FfbConfigTool::getPreset(presetName);
    QVariantMap m;
    m["gain"] = static_cast<double>(preset.settings.gain);
    m["filter"] = static_cast<double>(preset.settings.filter);
    m["minimumForce"] = static_cast<double>(preset.settings.minimumForce);
    m["kerbEffect"] = static_cast<double>(preset.settings.kerbEffect);
    m["roadEffect"] = static_cast<double>(preset.settings.roadEffect);
    m["slipEffect"] = static_cast<double>(preset.settings.slipEffect);
    m["absEffect"] = static_cast<double>(preset.settings.absEffect);
    m["enhUndersteer"] = static_cast<double>(preset.settings.enhUndersteer);
    m["centreBoostGain"] = static_cast<double>(preset.settings.centreBoostGain);
    m["centreBoostRange"] = static_cast<double>(preset.settings.centreBoostRange);
    m["enableGyro"] = preset.settings.enableGyro;
    m["gyroStrength"] = static_cast<double>(preset.settings.gyroStrength);
    return m;
}

bool FfbEditorQmlBridge::optimizeForWheel(const QString& wheelModel) {
    FfbConfigTool::optimizeForWheel(m_settings, wheelModel);
    markUnsaved();
    emit settingsChanged();
    return true;
}

QStringList FfbEditorQmlBridge::getSupportedWheels() {
    return FfbConfigTool::getSupportedWheels();
}

QString FfbEditorQmlBridge::getWheelManufacturer(const QString& wheelModel) {
    return FfbConfigTool::getWheelManufacturer(wheelModel);
}

QVariantMap FfbEditorQmlBridge::validateSettings() {
    QVariantMap result;
    QString error;
    bool valid = FfbConfigTool::validateSettings(m_settings, &error);
    result["valid"] = valid;
    result["error"] = error;
    if (!valid) {
        emit validationFailed(error);
    }
    return result;
}

void FfbEditorQmlBridge::resetToDefaults() {
    m_settings = FfbConfigTool::FfbSettings();
    markUnsaved();
    emit settingsChanged();
}

void FfbEditorQmlBridge::markSaved() {
    m_hasUnsavedChanges = false;
    emit unsavedChangesChanged();
}

void FfbEditorQmlBridge::markUnsaved() {
    if (!m_hasUnsavedChanges) {
        m_hasUnsavedChanges = true;
        emit unsavedChangesChanged();
    }
}

// ============================================================================
// FfbEditorModule
// ============================================================================

FfbEditorModule::FfbEditorModule(QWidget* parent)
    : ModuleGuiBase(parent)
{
    setObjectName("FfbEditorModule");
}

bool FfbEditorModule::initialize()
{
    if (m_uiBuilt) return true;
    bool ok = ModuleGuiBase::initialize();
    syncFromBridge();
    LOG_INFO("FfbEditorModule", "Initializing FFB Editor module");
    return ok;
}

void FfbEditorModule::shutdown()
{
    ModuleGuiBase::shutdown();
    LOG_INFO("FfbEditorModule", "Shutting down FFB Editor module");
}

void FfbEditorModule::importFile(const QString& filePath)
{
    if (auto* bridge = FfbEditorQmlBridge::instance()) {
        bridge->loadSettings(filePath);
    }
}

void FfbEditorModule::exportFile(const QString& filePath)
{
    if (auto* bridge = FfbEditorQmlBridge::instance()) {
        bridge->saveSettings(filePath);
    }
}

QJsonObject FfbEditorModule::serializeProject() const
{
    QJsonObject data;
    auto* bridge = FfbEditorQmlBridge::instance();
    if (bridge) {
        data["currentFile"] = bridge->currentFile();
    }
    return data;
}

void FfbEditorModule::deserializeProject(const QJsonObject& data)
{
    if (data.contains("currentFile")) {
        QString filePath = data["currentFile"].toString();
        if (!filePath.isEmpty()) {
            importFile(filePath);
        }
    }
}

void FfbEditorModule::buildUI()
{
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #3a3a3a; background: #1e1e1e; }"
        "QTabBar::tab { background: #2d2d2d; color: #aaa; padding: 8px 16px; border: 1px solid #3a3a3a; border-bottom: none; }"
        "QTabBar::tab:selected { background: #3a5a8a; color: #fff; }"
        "QTabBar::tab:hover { background: #4a6a9a; }"
    );

    // Tab 1: Settings
    QWidget* settingsTab = new QWidget();
    {
        QVBoxLayout* layout = new QVBoxLayout(settingsTab);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(6);

        auto addSlider = [&](const QString& name, QSlider*& slider, QLabel*& label, int min, int max, int def) {
            QHBoxLayout* hl = new QHBoxLayout();
            hl->addWidget(new QLabel(name));
            slider = new QSlider(Qt::Horizontal);
            slider->setRange(min, max);
            slider->setValue(def);
            hl->addWidget(slider, 1);
            label = new QLabel(QString::number(def));
            label->setMinimumWidth(40);
            label->setAlignment(Qt::AlignRight);
            hl->addWidget(label);
            layout->addLayout(hl);
            return slider;
        };

        QGroupBox* gainGroup = new QGroupBox("Force Feedback");
        QVBoxLayout* gl = new QVBoxLayout(gainGroup);
        addSlider("Gain:", m_gainSlider, m_gainLabel, 0, 200, 100);
        addSlider("Filter:", m_filterSlider, m_filterLabel, 0, 100, 50);
        addSlider("Min Force:", m_minForceSlider, m_minForceLabel, 0, 100, 20);
        addSlider("Kerb Effect:", m_kerbSlider, m_kerbLabel, 0, 200, 100);
        addSlider("Road Effect:", m_roadSlider, m_roadLabel, 0, 200, 100);
        addSlider("Slip Effect:", m_slipSlider, m_slipLabel, 0, 200, 100);
        addSlider("ABS Effect:", m_absSlider, m_absLabel, 0, 200, 100);
        addSlider("Enh. Understeer:", m_understeerSlider, m_understeerLabel, 0, 200, 100);
        addSlider("Center Boost Gain:", m_cenGainSlider, m_cenGainLabel, 0, 200, 100);
        addSlider("Center Boost Range:", m_cenRangeSlider, m_cenRangeLabel, 0, 100, 50);
        layout->addWidget(gainGroup);

        QGroupBox* gyroGroup = new QGroupBox("Gyroscope");
        QVBoxLayout* gyl = new QVBoxLayout(gyroGroup);
        m_gyroCheck = new QCheckBox("Enable Gyroscope");
        gyl->addWidget(m_gyroCheck);
        addSlider("Gyro Strength:", m_gyroSlider, m_gyroLabel, 0, 200, 100);
        layout->addWidget(gyroGroup);

        QHBoxLayout* btnLayout = new QHBoxLayout();
        m_loadBtn = new QPushButton("Load from File");
        connect(m_loadBtn, &QPushButton::clicked, this, &FfbEditorModule::onLoadFromFile);
        btnLayout->addWidget(m_loadBtn);
        m_saveBtn = new QPushButton("Save to File");
        connect(m_saveBtn, &QPushButton::clicked, this, &FfbEditorModule::onSaveToFile);
        btnLayout->addWidget(m_saveBtn);
        m_resetBtn = new QPushButton("Reset to Defaults");
        connect(m_resetBtn, &QPushButton::clicked, this, &FfbEditorModule::onResetDefaults);
        btnLayout->addWidget(m_resetBtn);
        btnLayout->addStretch();
        layout->addLayout(btnLayout);

        connect(m_gainSlider, &QSlider::valueChanged, this, &FfbEditorModule::onGainChanged);
        connect(m_filterSlider, &QSlider::valueChanged, this, &FfbEditorModule::onFilterChanged);
        connect(m_minForceSlider, &QSlider::valueChanged, this, &FfbEditorModule::onMinForceChanged);
        connect(m_kerbSlider, &QSlider::valueChanged, this, &FfbEditorModule::onKerbEffectChanged);
        connect(m_roadSlider, &QSlider::valueChanged, this, &FfbEditorModule::onRoadEffectChanged);
        connect(m_slipSlider, &QSlider::valueChanged, this, &FfbEditorModule::onSlipEffectChanged);
        connect(m_absSlider, &QSlider::valueChanged, this, &FfbEditorModule::onAbsEffectChanged);
        connect(m_understeerSlider, &QSlider::valueChanged, this, &FfbEditorModule::onUndersteerChanged);
        connect(m_cenGainSlider, &QSlider::valueChanged, this, &FfbEditorModule::onCenterBoostGainChanged);
        connect(m_cenRangeSlider, &QSlider::valueChanged, this, &FfbEditorModule::onCenterBoostRangeChanged);
        connect(m_gyroCheck, &QCheckBox::toggled, this, &FfbEditorModule::onGyroToggled);
        connect(m_gyroSlider, &QSlider::valueChanged, this, &FfbEditorModule::onGyroStrengthChanged);

        layout->addStretch();
    }
    m_tabWidget->addTab(settingsTab, "Settings");

    // Tab 2: Presets
    QWidget* presetsTab = new QWidget();
    {
        QVBoxLayout* layout = new QVBoxLayout(presetsTab);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(8);

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

        m_statusLog = new QTextEdit();
        m_statusLog->setReadOnly(true);
        m_statusLog->setMaximumHeight(150);
        m_statusLog->setStyleSheet("QTextEdit { background: #0a0a0a; color: #c8c8c8; font-family: Consolas; font-size: 10px; }");
        layout->addWidget(m_statusLog);

        if (auto* bridge = FfbEditorQmlBridge::instance()) {
            QVariantList presets = bridge->getPresets();
            for (const auto& p : presets) {
                m_presetList->addItem(p.toString());
            }
        }

        layout->addStretch();
    }
    m_tabWidget->addTab(presetsTab, "Presets");

    m_mainLayout->insertWidget(1, m_tabWidget, 1);
    m_uiBuilt = true;
}

void FfbEditorModule::syncFromBridge()
{
    auto* bridge = FfbEditorQmlBridge::instance();
    if (!bridge) return;
    m_gainSlider->setValue((int)(bridge->gain() * 100));
    m_filterSlider->setValue((int)(bridge->filter() * 100));
    m_minForceSlider->setValue((int)(bridge->minimumForce() * 100));
    m_kerbSlider->setValue((int)(bridge->kerbEffect() * 100));
    m_roadSlider->setValue((int)(bridge->roadEffect() * 100));
    m_slipSlider->setValue((int)(bridge->slipEffect() * 100));
    m_absSlider->setValue((int)(bridge->absEffect() * 100));
    m_understeerSlider->setValue((int)(bridge->enhUndersteer() * 100));
    m_cenGainSlider->setValue((int)(bridge->centreBoostGain() * 100));
    m_cenRangeSlider->setValue((int)(bridge->centreBoostRange() * 100));
    m_gyroCheck->setChecked(bridge->enableGyro());
    m_gyroSlider->setValue((int)(bridge->gyroStrength() * 100));
}

void FfbEditorModule::syncToBridge()
{
    auto* bridge = FfbEditorQmlBridge::instance();
    if (!bridge) return;
    bridge->setGain(m_gainSlider->value() / 100.0f);
    bridge->setFilter(m_filterSlider->value() / 100.0f);
    bridge->setMinimumForce(m_minForceSlider->value() / 100.0f);
    bridge->setKerbEffect(m_kerbSlider->value() / 100.0f);
    bridge->setRoadEffect(m_roadSlider->value() / 100.0f);
    bridge->setSlipEffect(m_slipSlider->value() / 100.0f);
    bridge->setAbsEffect(m_absSlider->value() / 100.0f);
    bridge->setEnhUndersteer(m_understeerSlider->value() / 100.0f);
    bridge->setCentreBoostGain(m_cenGainSlider->value() / 100.0f);
    bridge->setCentreBoostRange(m_cenRangeSlider->value() / 100.0f);
    bridge->setEnableGyro(m_gyroCheck->isChecked());
    bridge->setGyroStrength(m_gyroSlider->value() / 100.0f);
}

void FfbEditorModule::onGainChanged(int v) { m_gainLabel->setText(QString::number(v)); syncToBridge(); }
void FfbEditorModule::onFilterChanged(int v) { m_filterLabel->setText(QString::number(v)); syncToBridge(); }
void FfbEditorModule::onMinForceChanged(int v) { m_minForceLabel->setText(QString::number(v)); syncToBridge(); }
void FfbEditorModule::onKerbEffectChanged(int v) { m_kerbLabel->setText(QString::number(v)); syncToBridge(); }
void FfbEditorModule::onRoadEffectChanged(int v) { m_roadLabel->setText(QString::number(v)); syncToBridge(); }
void FfbEditorModule::onSlipEffectChanged(int v) { m_slipLabel->setText(QString::number(v)); syncToBridge(); }
void FfbEditorModule::onAbsEffectChanged(int v) { m_absLabel->setText(QString::number(v)); syncToBridge(); }
void FfbEditorModule::onUndersteerChanged(int v) { m_understeerLabel->setText(QString::number(v)); syncToBridge(); }
void FfbEditorModule::onCenterBoostGainChanged(int v) { m_cenGainLabel->setText(QString::number(v)); syncToBridge(); }
void FfbEditorModule::onCenterBoostRangeChanged(int v) { m_cenRangeLabel->setText(QString::number(v)); syncToBridge(); }
void FfbEditorModule::onGyroToggled(bool on) { m_gyroSlider->setEnabled(on); syncToBridge(); }
void FfbEditorModule::onGyroStrengthChanged(int v) { m_gyroLabel->setText(QString::number(v)); syncToBridge(); }

void FfbEditorModule::onApplyPreset(const QString& preset)
{
    if (auto* bridge = FfbEditorQmlBridge::instance()) {
        bridge->applyPreset(preset);
        syncFromBridge();
        m_statusLog->append("Applied preset: " + preset);
        logSuccess("FFB preset applied: " + preset);
    }
}

void FfbEditorModule::onLoadFromFile()
{
    QString path = selectFile("Load FFB Settings", "INI Files (*.ini);;All Files (*)");
    if (path.isEmpty()) return;
    if (auto* bridge = FfbEditorQmlBridge::instance()) {
        bridge->loadSettings(path);
        syncFromBridge();
        m_statusLog->append("Loaded: " + path);
        logSuccess("FFB settings loaded: " + path);
    }
}

void FfbEditorModule::onSaveToFile()
{
    QString path = QFileDialog::getSaveFileName(this, "Save FFB Settings", QString(), "INI Files (*.ini);;All Files (*)");
    if (path.isEmpty()) return;
    if (auto* bridge = FfbEditorQmlBridge::instance()) {
        bridge->saveSettings(path);
        m_statusLog->append("Saved: " + path);
        logSuccess("FFB settings saved: " + path);
    }
}

void FfbEditorModule::onResetDefaults()
{
    if (auto* bridge = FfbEditorQmlBridge::instance()) {
        bridge->resetToDefaults();
        syncFromBridge();
        m_statusLog->append("Reset to defaults");
        log("FFB settings reset to defaults");
    }
}

} // namespace ks
