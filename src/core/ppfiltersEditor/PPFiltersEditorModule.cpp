#include "PPFiltersEditorModule.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QTreeWidgetItem>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QStandardPaths>

namespace ks {
namespace ppfilters {

PPFiltersEditorModule::PPFiltersEditorModule(QWidget* parent)
    : ModuleGuiBase(parent)
    , m_tabWidget(nullptr)
    , m_filterChainTab(nullptr)
    , m_filterChainList(nullptr)
    , m_resetFilterBtn(nullptr)
    , m_realtimePreviewCheck(nullptr)
    , m_presetCombo(nullptr)
    , m_loadPresetBtn(nullptr)
    , m_savePresetBtn(nullptr)
    , m_bloomTab(nullptr)
    , m_bloomCheck(nullptr)
    , m_bloomIntensitySpin(nullptr)
    , m_bloomThresholdSpin(nullptr)
    , m_bloomRadiusSpin(nullptr)
    , m_toneMappingTab(nullptr)
    , m_toneMappingCombo(nullptr)
    , m_exposureSpin(nullptr)
    , m_gammaSpin(nullptr)
    , m_colorGradingTab(nullptr)
    , m_colorTempSpin(nullptr)
    , m_saturationSpin(nullptr)
    , m_contrastSpin(nullptr)
    , m_liftSlider(nullptr)
    , m_gammaSlider(nullptr)
    , m_gainSlider(nullptr)
    , m_lensEffectsTab(nullptr)
    , m_vignetteCheck(nullptr)
    , m_vignetteIntensitySpin(nullptr)
    , m_vignetteRadiusSpin(nullptr)
    , m_chromaticAberrationCheck(nullptr)
    , m_chromaticAberrationSpin(nullptr)
    , m_dofCheck(nullptr)
    , m_motionBlurCheck(nullptr)
    , m_motionBlurSamplesSpin(nullptr)
{
    setObjectName("PPFiltersEditorModule");
}

bool PPFiltersEditorModule::initialize() {
    if (m_uiBuilt) return true;
    ModuleGuiBase::initialize();
    return true;
}

void PPFiltersEditorModule::shutdown() {
    m_uiBuilt = false;
}

void PPFiltersEditorModule::importFile(const QString& filePath) {
    if (filePath.isEmpty()) return;
    QFileInfo fi(filePath);
    if (fi.suffix().toLower() == "ini" || fi.suffix().toLower() == "lua") {
        log(QString("Loading PP filter preset: %1").arg(filePath));
    } else {
        logError(QString("Unsupported filter format: %1").arg(filePath));
    }
}

void PPFiltersEditorModule::exportFile(const QString& filePath) {
    if (filePath.isEmpty()) return;
    log(QString("Exporting PP filter to: %1").arg(filePath));
}

void PPFiltersEditorModule::onActivation() {}
void PPFiltersEditorModule::onDeactivation() {}

void PPFiltersEditorModule::buildUI() {
    m_tabWidget = new QTabWidget();

    setupFilterChainTab();
    setupBloomTab();
    setupToneMappingTab();
    setupColorGradingTab();
    setupLensEffectsTab();

    m_mainLayout->addWidget(m_tabWidget);
    m_mainLayout->addWidget(m_logOutput);
}

void PPFiltersEditorModule::setupFilterChainTab() {
    m_filterChainTab = new QWidget();
    auto* layout = new QVBoxLayout(m_filterChainTab);

    auto* presetBar = new QHBoxLayout();
    m_presetCombo = createComboBox({"Default (Natural)", "Vibrant", "Cinematic", "Moody", "Vintage", "B&W", "HDR", "Custom"});
    m_loadPresetBtn = createButton("Load");
    m_savePresetBtn = createButton("Save");
    presetBar->addWidget(new QLabel("Preset:"));
    presetBar->addWidget(m_presetCombo);
    presetBar->addWidget(m_loadPresetBtn);
    presetBar->addWidget(m_savePresetBtn);
    presetBar->addStretch();
    layout->addLayout(presetBar);

    auto* controlBar = new QHBoxLayout();
    m_resetFilterBtn = createButton("Reset All");
    m_realtimePreviewCheck = createCheckBox("Real-time Preview", true);
    controlBar->addWidget(m_resetFilterBtn);
    controlBar->addStretch();
    controlBar->addWidget(m_realtimePreviewCheck);
    layout->addLayout(controlBar);

    m_filterChainList = new QListWidget();
    m_filterChainList->addItem("Bloom");
    m_filterChainList->addItem("Tone Mapping");
    m_filterChainList->addItem("Color Grading");
    m_filterChainList->addItem("Vignette");
    m_filterChainList->addItem("Chromatic Aberration");
    m_filterChainList->addItem("Depth of Field");
    m_filterChainList->addItem("Motion Blur");
    layout->addWidget(m_filterChainList);

    connect(m_filterChainList, &QListWidget::itemClicked, this, &PPFiltersEditorModule::onFilterSelected);
    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PPFiltersEditorModule::onPresetSelected);
    connect(m_loadPresetBtn, &QPushButton::clicked, this, &PPFiltersEditorModule::onLoadPreset);
    connect(m_savePresetBtn, &QPushButton::clicked, this, &PPFiltersEditorModule::onSavePreset);
    connect(m_resetFilterBtn, &QPushButton::clicked, this, &PPFiltersEditorModule::onResetFilter);
    connect(m_realtimePreviewCheck, &QCheckBox::toggled, this, &PPFiltersEditorModule::onRealTimePreviewToggled);

    populatePresets();
    m_tabWidget->addTab(m_filterChainTab, "Filter Chain");
}

void PPFiltersEditorModule::setupBloomTab() {
    m_bloomTab = new QWidget();
    auto* layout = new QVBoxLayout(m_bloomTab);

    m_bloomCheck = createCheckBox("Enable Bloom", true);
    layout->addWidget(m_bloomCheck);

    auto* params = createGroupBox("Bloom Parameters");
    auto* form = new QFormLayout(params);
    m_bloomIntensitySpin = createDoubleSpinBox(0.0, 10.0, 1.5, 2, "");
    m_bloomThresholdSpin = createDoubleSpinBox(0.0, 5.0, 1.0, 2, "");
    m_bloomRadiusSpin = createDoubleSpinBox(0.0, 100.0, 30.0, 1, "");
    form->addRow("Intensity:", m_bloomIntensitySpin);
    form->addRow("Threshold:", m_bloomThresholdSpin);
    form->addRow("Radius:", m_bloomRadiusSpin);
    layout->addWidget(params);
    layout->addStretch();

    connect(m_bloomCheck, &QCheckBox::toggled, this, &PPFiltersEditorModule::onBloomToggled);
    connect(m_bloomIntensitySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PPFiltersEditorModule::onBloomIntensityChanged);

    m_tabWidget->addTab(m_bloomTab, "Bloom");
}

void PPFiltersEditorModule::setupToneMappingTab() {
    m_toneMappingTab = new QWidget();
    auto* layout = new QVBoxLayout(m_toneMappingTab);

    auto* params = createGroupBox("Tone Mapping");
    auto* form = new QFormLayout(params);
    m_toneMappingCombo = createComboBox({"ACES", "Reinhard", "Filmic", "Uncharted 2", "Hejl Richard", "Linear"});
    m_exposureSpin = createDoubleSpinBox(0.01, 10.0, 1.0, 2, "");
    m_gammaSpin = createDoubleSpinBox(0.1, 5.0, 2.2, 2, "");
    form->addRow("Operator:", m_toneMappingCombo);
    form->addRow("Exposure:", m_exposureSpin);
    form->addRow("Gamma:", m_gammaSpin);
    layout->addWidget(params);
    layout->addStretch();

    connect(m_toneMappingCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PPFiltersEditorModule::onToneMappingChanged);

    m_tabWidget->addTab(m_toneMappingTab, "Tone Mapping");
}

void PPFiltersEditorModule::setupColorGradingTab() {
    m_colorGradingTab = new QWidget();
    auto* layout = new QVBoxLayout(m_colorGradingTab);

    auto* params = createGroupBox("Color Grading");
    auto* form = new QFormLayout(params);
    m_colorTempSpin = createDoubleSpinBox(-100.0, 100.0, 0.0, 1, "");
    m_saturationSpin = createDoubleSpinBox(0.0, 200.0, 100.0, 1, "%");
    m_contrastSpin = createDoubleSpinBox(-100.0, 100.0, 0.0, 1, "");
    form->addRow("Temperature:", m_colorTempSpin);
    form->addRow("Saturation:", m_saturationSpin);
    form->addRow("Contrast:", m_contrastSpin);

    auto* liftGammaGain = createGroupBox("Lift / Gamma / Gain");
    auto* lggForm = new QFormLayout(liftGammaGain);
    m_liftSlider = new QSlider(Qt::Horizontal);
    m_liftSlider->setRange(0, 200);
    m_liftSlider->setValue(100);
    m_gammaSlider = new QSlider(Qt::Horizontal);
    m_gammaSlider->setRange(0, 200);
    m_gammaSlider->setValue(100);
    m_gainSlider = new QSlider(Qt::Horizontal);
    m_gainSlider->setRange(0, 200);
    m_gainSlider->setValue(100);
    lggForm->addRow("Lift:", m_liftSlider);
    lggForm->addRow("Gamma:", m_gammaSlider);
    lggForm->addRow("Gain:", m_gainSlider);
    layout->addWidget(params);
    layout->addWidget(liftGammaGain);
    layout->addStretch();

    connect(m_colorTempSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PPFiltersEditorModule::onColorTemperatureChanged);
    connect(m_saturationSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PPFiltersEditorModule::onSaturationChanged);
    connect(m_contrastSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PPFiltersEditorModule::onContrastChanged);

    m_tabWidget->addTab(m_colorGradingTab, "Color Grading");
}

void PPFiltersEditorModule::setupLensEffectsTab() {
    m_lensEffectsTab = new QWidget();
    auto* layout = new QVBoxLayout(m_lensEffectsTab);
    auto* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    auto* container = new QWidget();
    auto* form = new QFormLayout(container);

    m_vignetteCheck = createCheckBox("Enable Vignette", true);
    m_vignetteIntensitySpin = createDoubleSpinBox(0.0, 1.0, 0.5, 2, "");
    m_vignetteRadiusSpin = createDoubleSpinBox(0.0, 2.0, 1.0, 2, "");

    m_chromaticAberrationCheck = createCheckBox("Enable Chromatic Aberration", false);
    m_chromaticAberrationSpin = createDoubleSpinBox(0.0, 1.0, 0.1, 3, "");

    m_dofCheck = createCheckBox("Enable Depth of Field", false);

    m_motionBlurCheck = createCheckBox("Enable Motion Blur", false);
    m_motionBlurSamplesSpin = createSpinBox(2, 64, 16, " samples");

    form->addRow("", m_vignetteCheck);
    form->addRow("Vignette Intensity:", m_vignetteIntensitySpin);
    form->addRow("Vignette Radius:", m_vignetteRadiusSpin);
    form->addRow("", m_chromaticAberrationCheck);
    form->addRow("Aberration Amount:", m_chromaticAberrationSpin);
    form->addRow("", m_dofCheck);
    form->addRow("", m_motionBlurCheck);
    form->addRow("Motion Blur Samples:", m_motionBlurSamplesSpin);

    scrollArea->setWidget(container);
    layout->addWidget(scrollArea);

    connect(m_vignetteCheck, &QCheckBox::toggled, this, &PPFiltersEditorModule::onVignetteToggled);
    connect(m_vignetteIntensitySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PPFiltersEditorModule::onVignetteIntensityChanged);
    connect(m_chromaticAberrationCheck, &QCheckBox::toggled, this, &PPFiltersEditorModule::onChromaticAberrationToggled);
    connect(m_chromaticAberrationSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PPFiltersEditorModule::onChromaticAberrationChanged);
    connect(m_dofCheck, &QCheckBox::toggled, this, &PPFiltersEditorModule::onDepthOfFieldToggled);
    connect(m_motionBlurCheck, &QCheckBox::toggled, this, &PPFiltersEditorModule::onMotionBlurToggled);
    connect(m_motionBlurSamplesSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &PPFiltersEditorModule::onMotionBlurChanged);

    m_tabWidget->addTab(m_lensEffectsTab, "Lens Effects");
}

void PPFiltersEditorModule::populateFilterChain() {}
void PPFiltersEditorModule::populatePresets() {}

void PPFiltersEditorModule::onFilterSelected(QListWidgetItem* item) {
    if (item) {
        log(QString("Selected filter: %1").arg(item->text()));
        QString filter = item->text();
        if (filter == "Bloom") m_tabWidget->setCurrentWidget(m_bloomTab);
        else if (filter == "Tone Mapping") m_tabWidget->setCurrentWidget(m_toneMappingTab);
        else if (filter == "Color Grading") m_tabWidget->setCurrentWidget(m_colorGradingTab);
        else m_tabWidget->setCurrentWidget(m_lensEffectsTab);
    }
}

void PPFiltersEditorModule::onPresetSelected(int index) {
    Q_UNUSED(index);
}

void PPFiltersEditorModule::onBloomToggled(bool checked) {
    log(QString("Bloom %1").arg(checked ? "enabled" : "disabled"));
}

void PPFiltersEditorModule::onBloomIntensityChanged(double value) {
    log(QString("Bloom intensity: %1").arg(value, 0, 'f', 2));
}

void PPFiltersEditorModule::onToneMappingChanged(int index) {
    log(QString("Tone mapping operator: %1").arg(m_toneMappingCombo->currentText()));
}

void PPFiltersEditorModule::onColorTemperatureChanged(double value) {
    log(QString("Color temperature: %1").arg(value, 0, 'f', 1));
}

void PPFiltersEditorModule::onSaturationChanged(double value) {
    log(QString("Saturation: %1%").arg(value, 0, 'f', 1));
}

void PPFiltersEditorModule::onContrastChanged(double value) {
    log(QString("Contrast: %1").arg(value, 0, 'f', 1));
}

void PPFiltersEditorModule::onVignetteToggled(bool checked) {
    log(QString("Vignette %1").arg(checked ? "enabled" : "disabled"));
}

void PPFiltersEditorModule::onVignetteIntensityChanged(double value) {
    log(QString("Vignette intensity: %1").arg(value, 0, 'f', 2));
}

void PPFiltersEditorModule::onChromaticAberrationToggled(bool checked) {
    log(QString("Chromatic aberration %1").arg(checked ? "enabled" : "disabled"));
}

void PPFiltersEditorModule::onChromaticAberrationChanged(double value) {
    log(QString("Chromatic aberration amount: %1").arg(value, 0, 'f', 3));
}

void PPFiltersEditorModule::onDepthOfFieldToggled(bool checked) {
    log(QString("Depth of field %1").arg(checked ? "enabled" : "disabled"));
}

void PPFiltersEditorModule::onMotionBlurToggled(bool checked) {
    log(QString("Motion blur %1").arg(checked ? "enabled" : "disabled"));
}

void PPFiltersEditorModule::onMotionBlurChanged(int value) {
    log(QString("Motion blur samples: %1").arg(value));
}

void PPFiltersEditorModule::onLoadPreset() {
    QString path = selectFile("Load PP Filter Preset", "Filter Presets (*.ini *.lua);;All Files (*)");
    if (!path.isEmpty()) {
        importFile(path);
        logSuccess("Filter preset loaded");
    }
}

void PPFiltersEditorModule::onSavePreset() {
    QString path = selectFile("Save PP Filter Preset", "Filter Presets (*.ini)");
    if (!path.isEmpty()) {
        exportFile(path);
        logSuccess("Filter preset saved");
    }
}

void PPFiltersEditorModule::onResetFilter() {
    if (confirmAction("Reset Filters", "Reset all post-processing filters to defaults?")) {
        m_bloomCheck->setChecked(true);
        m_bloomIntensitySpin->setValue(1.5);
        m_bloomThresholdSpin->setValue(1.0);
        m_bloomRadiusSpin->setValue(30.0);
        m_toneMappingCombo->setCurrentIndex(0);
        m_exposureSpin->setValue(1.0);
        m_gammaSpin->setValue(2.2);
        m_colorTempSpin->setValue(0.0);
        m_saturationSpin->setValue(100.0);
        m_contrastSpin->setValue(0.0);
        m_vignetteCheck->setChecked(true);
        m_vignetteIntensitySpin->setValue(0.5);
        m_vignetteRadiusSpin->setValue(1.0);
        m_chromaticAberrationCheck->setChecked(false);
        m_chromaticAberrationSpin->setValue(0.1);
        m_dofCheck->setChecked(false);
        m_motionBlurCheck->setChecked(false);
        m_motionBlurSamplesSpin->setValue(16);
        logSuccess("Filters reset to defaults");
    }
}

void PPFiltersEditorModule::onRealTimePreviewToggled(bool checked) {
    log(QString("Real-time preview %1").arg(checked ? "enabled" : "disabled"));
}

} // namespace ppfilters
} // namespace ks

#include "PPFiltersEditorModule.moc"
