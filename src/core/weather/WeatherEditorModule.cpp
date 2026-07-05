#include "WeatherEditorModule.h"
#include "../sys/LogManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
#include <QSplitter>

namespace ks {

WeatherEditorModule::WeatherEditorModule(QWidget* parent)
    : EditorModule(parent)
{
}

bool WeatherEditorModule::initialize()
{
    LOG_INFO("WeatherEditorModule", "Initialized");
    return true;
}

void WeatherEditorModule::shutdown()
{
    if (m_statusLabel) m_statusLabel->setText("Shut down");
}

QDockWidget* WeatherEditorModule::getOrCreateDockWidget(QMainWindow* mainWindow)
{
    if (m_dockWidget) return m_dockWidget;

    m_dockWidget = new QDockWidget("Weather Editor", mainWindow);
    m_dockWidget->setObjectName("WeatherEditorDock");

    auto* centralWidget = new QWidget();
    auto* mainLayout = new QVBoxLayout(centralWidget);

    m_tabWidget = new QTabWidget();

    // === Clouds Tab ===
    auto* cloudsWidget = new QWidget();
    auto* cloudsLayout = new QGridLayout(cloudsWidget);

    m_cloudCoverSpin = new QDoubleSpinBox();
    m_cloudCoverSpin->setRange(0.0, 1.0);
    m_cloudCoverSpin->setSingleStep(0.05);
    m_cloudCoverSpin->setToolTip("Cloud transparency (0-1)");
    cloudsLayout->addWidget(new QLabel("Cover:"), 0, 0);
    cloudsLayout->addWidget(m_cloudCoverSpin, 0, 1);

    m_cloudCutoffSpin = new QDoubleSpinBox();
    m_cloudCutoffSpin->setRange(0.0, 1.0);
    m_cloudCutoffSpin->setSingleStep(0.05);
    cloudsLayout->addWidget(new QLabel("Cutoff:"), 1, 0);
    cloudsLayout->addWidget(m_cloudCutoffSpin, 1, 1);

    m_cloudColorSpin = new QDoubleSpinBox();
    m_cloudColorSpin->setRange(0.0, 1.0);
    m_cloudColorSpin->setSingleStep(0.05);
    cloudsLayout->addWidget(new QLabel("Color:"), 2, 0);
    cloudsLayout->addWidget(m_cloudColorSpin, 2, 1);

    m_cloudWidthSpin = new QDoubleSpinBox();
    m_cloudWidthSpin->setRange(0.1, 100.0);
    m_cloudWidthSpin->setSingleStep(0.5);
    cloudsLayout->addWidget(new QLabel("Width:"), 3, 0);
    cloudsLayout->addWidget(m_cloudWidthSpin, 3, 1);

    m_cloudHeightSpin = new QDoubleSpinBox();
    m_cloudHeightSpin->setRange(0.1, 100.0);
    m_cloudHeightSpin->setSingleStep(0.5);
    cloudsLayout->addWidget(new QLabel("Height:"), 4, 0);
    cloudsLayout->addWidget(m_cloudHeightSpin, 4, 1);

    m_cloudRadiusSpin = new QDoubleSpinBox();
    m_cloudRadiusSpin->setRange(0.1, 10000.0);
    m_cloudRadiusSpin->setSingleStep(10.0);
    cloudsLayout->addWidget(new QLabel("Radius:"), 5, 0);
    cloudsLayout->addWidget(m_cloudRadiusSpin, 5, 1);

    m_cloudNumberSpin = new QSpinBox();
    m_cloudNumberSpin->setRange(0, 100);
    cloudsLayout->addWidget(new QLabel("Number:"), 6, 0);
    cloudsLayout->addWidget(m_cloudNumberSpin, 6, 1);

    m_cloudSpeedSpin = new QDoubleSpinBox();
    m_cloudSpeedSpin->setRange(0.0, 10.0);
    m_cloudSpeedSpin->setSingleStep(0.1);
    cloudsLayout->addWidget(new QLabel("Speed:"), 7, 0);
    cloudsLayout->addWidget(m_cloudSpeedSpin, 7, 1);

    m_tabWidget->addTab(cloudsWidget, "Clouds");

    // === Fog Tab ===
    auto* fogWidget = new QWidget();
    auto* fogLayout = new QGridLayout(fogWidget);

    m_fogColorBtn = new QPushButton("Pick Color");
    m_fogColorBtn->setStyleSheet("background-color: rgb(200,200,220)");
    fogLayout->addWidget(new QLabel("Color:"), 0, 0);
    fogLayout->addWidget(m_fogColorBtn, 0, 1);

    m_fogBlendSpin = new QDoubleSpinBox();
    m_fogBlendSpin->setRange(0.0, 1.0);
    m_fogBlendSpin->setSingleStep(0.05);
    fogLayout->addWidget(new QLabel("Blend:"), 1, 0);
    fogLayout->addWidget(m_fogBlendSpin, 1, 1);

    m_fogDistanceSpin = new QDoubleSpinBox();
    m_fogDistanceSpin->setRange(100.0, 100000.0);
    m_fogDistanceSpin->setSingleStep(100.0);
    fogLayout->addWidget(new QLabel("Distance:"), 2, 0);
    fogLayout->addWidget(m_fogDistanceSpin, 2, 1);

    m_tabWidget->addTab(fogWidget, "Fog");

    // === Launcher Tab ===
    auto* launcherWidget = new QWidget();
    auto* launcherLayout = new QGridLayout(launcherWidget);

    m_weatherNameEdit = new QLineEdit();
    launcherLayout->addWidget(new QLabel("Name:"), 0, 0);
    launcherLayout->addWidget(m_weatherNameEdit, 0, 1);

    m_temperatureCoeffSpin = new QDoubleSpinBox();
    m_temperatureCoeffSpin->setRange(-1.0, 1.0);
    m_temperatureCoeffSpin->setSingleStep(0.1);
    launcherLayout->addWidget(new QLabel("Temp Coeff:"), 1, 0);
    launcherLayout->addWidget(m_temperatureCoeffSpin, 1, 1);

    m_tabWidget->addTab(launcherWidget, "Launcher");

    // === Color Curves Tab ===
    auto* curvesWidget = new QWidget();
    auto* curvesLayout = new QGridLayout(curvesWidget);

    // Horizon
    auto* horizonGroup = new QGroupBox("Horizon");
    auto* horizonLayout = new QGridLayout(horizonGroup);
    m_horizonLowColorBtn = new QPushButton("Low");
    m_horizonLowColorBtn->setStyleSheet("background-color: rgb(255,138,34)");
    m_horizonHighColorBtn = new QPushButton("High");
    m_horizonHighColorBtn->setStyleSheet("background-color: rgb(150,170,220)");
    horizonLayout->addWidget(new QLabel("Low:"), 0, 0);
    horizonLayout->addWidget(m_horizonLowColorBtn, 0, 1);
    horizonLayout->addWidget(new QLabel("High:"), 1, 0);
    horizonLayout->addWidget(m_horizonHighColorBtn, 1, 1);
    curvesLayout->addWidget(horizonGroup, 0, 0);

    // Sky
    auto* skyGroup = new QGroupBox("Sky");
    auto* skyLayout = new QGridLayout(skyGroup);
    m_skyLowColorBtn = new QPushButton("Low");
    m_skyLowColorBtn->setStyleSheet("background-color: rgb(30,73,167)");
    m_skyHighColorBtn = new QPushButton("High");
    m_skyHighColorBtn->setStyleSheet("background-color: rgb(30,73,167)");
    skyLayout->addWidget(new QLabel("Low:"), 0, 0);
    skyLayout->addWidget(m_skyLowColorBtn, 0, 1);
    skyLayout->addWidget(new QLabel("High:"), 1, 0);
    skyLayout->addWidget(m_skyHighColorBtn, 1, 1);
    curvesLayout->addWidget(skyGroup, 0, 1);

    // Sun
    auto* sunGroup = new QGroupBox("Sun");
    auto* sunLayout = new QGridLayout(sunGroup);
    m_sunLowColorBtn = new QPushButton("Low");
    m_sunLowColorBtn->setStyleSheet("background-color: rgb(229,140,70)");
    m_sunHighColorBtn = new QPushButton("High");
    m_sunHighColorBtn->setStyleSheet("background-color: rgb(170,160,140)");
    sunLayout->addWidget(new QLabel("Low:"), 0, 0);
    sunLayout->addWidget(m_sunLowColorBtn, 0, 1);
    sunLayout->addWidget(new QLabel("High:"), 1, 0);
    sunLayout->addWidget(m_sunHighColorBtn, 1, 1);
    curvesLayout->addWidget(sunGroup, 1, 0);

    // Ambient
    auto* ambientGroup = new QGroupBox("Ambient");
    auto* ambientLayout = new QGridLayout(ambientGroup);
    m_ambientLowColorBtn = new QPushButton("Low");
    m_ambientLowColorBtn->setStyleSheet("background-color: rgb(124,124,124)");
    m_ambientHighColorBtn = new QPushButton("High");
    m_ambientHighColorBtn->setStyleSheet("background-color: rgb(105,105,105)");
    ambientLayout->addWidget(new QLabel("Low:"), 0, 0);
    ambientLayout->addWidget(m_ambientLowColorBtn, 0, 1);
    ambientLayout->addWidget(new QLabel("High:"), 1, 0);
    ambientLayout->addWidget(m_ambientHighColorBtn, 1, 1);
    curvesLayout->addWidget(ambientGroup, 1, 1);

    m_tabWidget->addTab(curvesWidget, "Color Curves");

    mainLayout->addWidget(m_tabWidget);

    // === Preset Management ===
    auto* presetGroup = new QGroupBox("Presets");
    auto* presetLayout = new QHBoxLayout(presetGroup);
    m_presetList = new QListWidget();
    m_presetList->setMaximumHeight(100);
    presetLayout->addWidget(m_presetList);

    auto* presetBtnLayout = new QVBoxLayout();
    m_newPresetBtn = new QPushButton("New");
    m_deletePresetBtn = new QPushButton("Delete");
    m_loadPresetBtn = new QPushButton("Load");
    m_savePresetBtn = new QPushButton("Save");
    presetBtnLayout->addWidget(m_newPresetBtn);
    presetBtnLayout->addWidget(m_deletePresetBtn);
    presetBtnLayout->addWidget(m_loadPresetBtn);
    presetBtnLayout->addWidget(m_savePresetBtn);
    presetBtnLayout->addStretch();
    presetLayout->addLayout(presetBtnLayout);

    mainLayout->addWidget(presetGroup);

    // === Action Buttons ===
    auto* actionLayout = new QHBoxLayout();
    m_loadIniBtn = new QPushButton("Load weather.ini");
    m_saveIniBtn = new QPushButton("Save weather.ini");
    m_loadCurvesBtn = new QPushButton("Load colorCurves.ini");
    m_saveCurvesBtn = new QPushButton("Save colorCurves.ini");
    m_resetBtn = new QPushButton("Reset Defaults");
    m_previewBtn = new QPushButton("Update Preview");
    actionLayout->addWidget(m_loadIniBtn);
    actionLayout->addWidget(m_saveIniBtn);
    actionLayout->addWidget(m_loadCurvesBtn);
    actionLayout->addWidget(m_saveCurvesBtn);
    actionLayout->addWidget(m_resetBtn);
    actionLayout->addWidget(m_previewBtn);
    mainLayout->addLayout(actionLayout);

    m_previewWidget = new WeatherPreviewWidget();
    m_previewWidget->parentModule = this;
    m_previewWidget->setMinimumHeight(160);
    m_previewWidget->setMaximumHeight(240);
    m_previewWidget->setStyleSheet("background: #1a1a1e; border: 1px solid #3f3f46; border-radius: 4px;");
    m_previewBuffer = QImage(640, 240, QImage::Format_ARGB32);
    m_previewBuffer.fill(Qt::black);
    mainLayout->addWidget(m_previewWidget);

    m_statusLabel = new QLabel("Ready");
    mainLayout->addWidget(m_statusLabel);

    connect(m_presetList, &QListWidget::currentRowChanged, this, &WeatherEditorModule::onPresetSelected);

    // Connections
    connect(m_cloudCoverSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WeatherEditorModule::onCloudCoverChanged);
    connect(m_cloudCutoffSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WeatherEditorModule::onCloudCutoffChanged);
    connect(m_cloudColorSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WeatherEditorModule::onCloudColorChanged);
    connect(m_cloudWidthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WeatherEditorModule::onCloudWidthChanged);
    connect(m_cloudHeightSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WeatherEditorModule::onCloudHeightChanged);
    connect(m_cloudRadiusSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WeatherEditorModule::onCloudRadiusChanged);
    connect(m_cloudNumberSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &WeatherEditorModule::onCloudNumberChanged);
    connect(m_cloudSpeedSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WeatherEditorModule::onCloudSpeedChanged);
    connect(m_fogColorBtn, &QPushButton::clicked, this, &WeatherEditorModule::onFogColorClicked);
    connect(m_fogBlendSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WeatherEditorModule::onFogBlendChanged);
    connect(m_fogDistanceSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WeatherEditorModule::onFogDistanceChanged);
    connect(m_weatherNameEdit, &QLineEdit::textChanged, this, &WeatherEditorModule::onWeatherNameChanged);
    connect(m_temperatureCoeffSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WeatherEditorModule::onTemperatureCoeffChanged);
    connect(m_horizonLowColorBtn, &QPushButton::clicked, this, &WeatherEditorModule::onHorizonLowColorClicked);
    connect(m_horizonHighColorBtn, &QPushButton::clicked, this, &WeatherEditorModule::onHorizonHighColorClicked);
    connect(m_skyLowColorBtn, &QPushButton::clicked, this, &WeatherEditorModule::onSkyLowColorClicked);
    connect(m_skyHighColorBtn, &QPushButton::clicked, this, &WeatherEditorModule::onSkyHighColorClicked);
    connect(m_sunLowColorBtn, &QPushButton::clicked, this, &WeatherEditorModule::onSunLowColorClicked);
    connect(m_sunHighColorBtn, &QPushButton::clicked, this, &WeatherEditorModule::onSunHighColorClicked);
    connect(m_ambientLowColorBtn, &QPushButton::clicked, this, &WeatherEditorModule::onAmbientLowColorClicked);
    connect(m_ambientHighColorBtn, &QPushButton::clicked, this, &WeatherEditorModule::onAmbientHighColorClicked);
    connect(m_loadPresetBtn, &QPushButton::clicked, this, &WeatherEditorModule::onLoadPreset);
    connect(m_savePresetBtn, &QPushButton::clicked, this, &WeatherEditorModule::onSavePreset);
    connect(m_newPresetBtn, &QPushButton::clicked, this, &WeatherEditorModule::onNewPreset);
    connect(m_deletePresetBtn, &QPushButton::clicked, this, &WeatherEditorModule::onDeletePreset);
    connect(m_loadIniBtn, &QPushButton::clicked, this, &WeatherEditorModule::onLoadWeatherIni);
    connect(m_saveIniBtn, &QPushButton::clicked, this, &WeatherEditorModule::onSaveWeatherIni);
    connect(m_loadCurvesBtn, &QPushButton::clicked, this, &WeatherEditorModule::onLoadColorCurves);
    connect(m_saveCurvesBtn, &QPushButton::clicked, this, &WeatherEditorModule::onSaveColorCurves);
    connect(m_resetBtn, &QPushButton::clicked, this, &WeatherEditorModule::onResetDefaults);
    connect(m_previewBtn, &QPushButton::clicked, this, &WeatherEditorModule::onUpdatePreview);

    m_dockWidget->setWidget(centralWidget);
    return m_dockWidget;
}

void WeatherEditorModule::importFile(const QString& filePath)
{
    if (filePath.endsWith("weather.ini", Qt::CaseInsensitive)) {
        m_weatherIniPath = filePath;
        loadWeatherIniToUI();
    } else if (filePath.endsWith("colorCurves.ini", Qt::CaseInsensitive)) {
        m_colorCurvesPath = filePath;
        loadColorCurvesToUI();
    }
}

void WeatherEditorModule::exportFile(const QString& filePath)
{
    if (filePath.endsWith("weather.ini", Qt::CaseInsensitive)) {
        m_weatherIniPath = filePath;
        saveWeatherIniFromUI();
    } else if (filePath.endsWith("colorCurves.ini", Qt::CaseInsensitive)) {
        m_colorCurvesPath = filePath;
        saveColorCurvesFromUI();
    }
}

void WeatherEditorModule::onActivation() { if (m_statusLabel) m_statusLabel->setText("Active"); }
void WeatherEditorModule::onDeactivation() { if (m_statusLabel) m_statusLabel->setText("Inactive"); }

// Cloud slots
void WeatherEditorModule::onCloudCoverChanged(double v) { m_preset.cloudIntensity = v; }
void WeatherEditorModule::onCloudCutoffChanged(double v) { m_preset.cloudCutoff = v; m_statusLabel->setText("Modified (Cloud Cutoff)"); }
void WeatherEditorModule::onCloudColorChanged(double v) { m_preset.cloudColor = v; m_statusLabel->setText("Modified (Cloud Color)"); }
void WeatherEditorModule::onCloudWidthChanged(double v) { m_preset.cloudWidth = v; m_statusLabel->setText("Modified (Cloud Width)"); }
void WeatherEditorModule::onCloudHeightChanged(double v) { m_preset.cloudHeight = v; m_statusLabel->setText("Modified (Cloud Height)"); }
void WeatherEditorModule::onCloudRadiusChanged(double v) { m_preset.cloudRadius = v; m_statusLabel->setText("Modified (Cloud Radius)"); }
void WeatherEditorModule::onCloudNumberChanged(int v) { m_preset.cloudNumber = v; m_statusLabel->setText("Modified (Cloud Number)"); }
void WeatherEditorModule::onCloudSpeedChanged(double v) { m_preset.cloudSpeed = v; m_statusLabel->setText("Modified (Cloud Speed)"); }

// Fog slots
void WeatherEditorModule::onFogColorClicked()
{
    QColor c = QColorDialog::getColor(m_preset.fogColor, this, "Fog Color");
    if (c.isValid()) {
        m_preset.fogColor = c;
        m_fogColorBtn->setStyleSheet(QString("background-color: rgb(%1,%2,%3)").arg(c.red()).arg(c.green()).arg(c.blue()));
    }
}

void WeatherEditorModule::onFogBlendChanged(double v) { m_preset.fogDensity = v; }
void WeatherEditorModule::onFogDistanceChanged(double v) { m_preset.fogDistance = v; m_statusLabel->setText("Modified (Fog Distance)"); }

// Launcher slots
void WeatherEditorModule::onWeatherNameChanged(const QString& n) { m_preset.name = n; m_statusLabel->setText("Modified (Weather Name)"); }
void WeatherEditorModule::onTemperatureCoeffChanged(double v) { m_preset.temperatureCoeff = v; m_statusLabel->setText("Modified (Temperature Coeff)"); }

// Color curve slots
void WeatherEditorModule::onHorizonLowColorClicked()
{
    QColor c = QColorDialog::getColor(Qt::white, this, "Horizon Low Color");
    if (c.isValid()) m_horizonLowColorBtn->setStyleSheet(QString("background-color: rgb(%1,%2,%3)").arg(c.red()).arg(c.green()).arg(c.blue()));
}

void WeatherEditorModule::onHorizonHighColorClicked()
{
    QColor c = QColorDialog::getColor(Qt::white, this, "Horizon High Color");
    if (c.isValid()) m_horizonHighColorBtn->setStyleSheet(QString("background-color: rgb(%1,%2,%3)").arg(c.red()).arg(c.green()).arg(c.blue()));
}

void WeatherEditorModule::onSkyLowColorClicked()
{
    QColor c = QColorDialog::getColor(Qt::white, this, "Sky Low Color");
    if (c.isValid()) m_skyLowColorBtn->setStyleSheet(QString("background-color: rgb(%1,%2,%3)").arg(c.red()).arg(c.green()).arg(c.blue()));
}

void WeatherEditorModule::onSkyHighColorClicked()
{
    QColor c = QColorDialog::getColor(Qt::white, this, "Sky High Color");
    if (c.isValid()) m_skyHighColorBtn->setStyleSheet(QString("background-color: rgb(%1,%2,%3)").arg(c.red()).arg(c.green()).arg(c.blue()));
}

void WeatherEditorModule::onSunLowColorClicked()
{
    QColor c = QColorDialog::getColor(Qt::white, this, "Sun Low Color");
    if (c.isValid()) m_sunLowColorBtn->setStyleSheet(QString("background-color: rgb(%1,%2,%3)").arg(c.red()).arg(c.green()).arg(c.blue()));
}

void WeatherEditorModule::onSunHighColorClicked()
{
    QColor c = QColorDialog::getColor(Qt::white, this, "Sun High Color");
    if (c.isValid()) m_sunHighColorBtn->setStyleSheet(QString("background-color: rgb(%1,%2,%3)").arg(c.red()).arg(c.green()).arg(c.blue()));
}

void WeatherEditorModule::onAmbientLowColorClicked()
{
    QColor c = QColorDialog::getColor(Qt::white, this, "Ambient Low Color");
    if (c.isValid()) m_ambientLowColorBtn->setStyleSheet(QString("background-color: rgb(%1,%2,%3)").arg(c.red()).arg(c.green()).arg(c.blue()));
}

void WeatherEditorModule::onAmbientHighColorClicked()
{
    QColor c = QColorDialog::getColor(Qt::white, this, "Ambient High Color");
    if (c.isValid()) m_ambientHighColorBtn->setStyleSheet(QString("background-color: rgb(%1,%2,%3)").arg(c.red()).arg(c.green()).arg(c.blue()));
}

// Preset management
void WeatherEditorModule::onLoadPreset()
{
    int idx = m_presetList->currentRow();
    if (idx < 0 || idx >= m_presetNames.size()) return;
    m_preset = WeatherConfigParser::getPreset(m_presetNames[idx]);
    loadWeatherIniToUI();
    m_statusLabel->setText("Loaded preset: " + m_presetNames[idx]);
}

void WeatherEditorModule::onSavePreset()
{
    QString name = m_weatherNameEdit->text();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Error", "Enter a weather name first");
        return;
    }
    m_preset.name = name;
    saveWeatherIniFromUI();
    WeatherConfigParser::savePreset(m_preset, QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/ksEditor/weather_presets");
    m_statusLabel->setText("Saved preset: " + name);
}

void WeatherEditorModule::onNewPreset()
{
    m_preset = WeatherConfigParser::getDefaultClear();
    m_weatherNameEdit->clear();
    loadWeatherIniToUI();
    m_statusLabel->setText("New preset created");
}

void WeatherEditorModule::onDeletePreset()
{
    int idx = m_presetList->currentRow();
    if (idx < 0 || idx >= m_presetNames.size()) return;
    QString name = m_presetNames[idx];
    QString path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/ksEditor/weather_presets/" + name + ".json";
    QFile::remove(path);
    m_presetNames.removeAt(idx);
    m_presetList->takeItem(idx);
    m_statusLabel->setText("Deleted preset: " + name);
}

void WeatherEditorModule::onPresetSelected(int index)
{
    if (index < 0 || index >= m_presetNames.size()) return;
    QString name = m_presetNames[index];
    QString path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/ksEditor/weather_presets/" + name + ".json";
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) return;

    QJsonObject obj = doc.object();
    m_preset.name = obj["name"].toString(name);
    m_preset.cloudIntensity = obj["cloudIntensity"].toDouble(0.5);
    m_preset.fogDensity = obj["fogDensity"].toDouble(0.01);
    m_preset.fogColor = QColor(obj["fogColor"].toString("#ffffff"));
    m_preset.ambientColor = QColor(obj["ambientColor"].toString("#ffffff"));
    m_preset.sunColor = QColor(obj["sunColor"].toString("#ffffff"));
    m_preset.skyColor = QColor(obj["skyColor"].toString("#87CEEB"));
    m_preset.horizonColor = QColor(obj["horizonColor"].toString("#FF7F50"));

    loadWeatherIniToUI();
    updatePreview();
    m_statusLabel->setText("Loaded preset: " + name);
}

// File operations
void WeatherEditorModule::onLoadWeatherIni()
{
    QString path = QFileDialog::getOpenFileName(this, "Open weather.ini", QString(), "Weather INI (*.ini);;All Files (*)");
    if (!path.isEmpty()) {
        m_weatherIniPath = path;
        loadWeatherIniToUI();
        m_statusLabel->setText("Loaded: " + path);
    }
}

void WeatherEditorModule::onSaveWeatherIni()
{
    QString path = m_weatherIniPath.isEmpty() ?
        QFileDialog::getSaveFileName(this, "Save weather.ini", QString(), "Weather INI (*.ini)") : m_weatherIniPath;
    if (!path.isEmpty()) {
        m_weatherIniPath = path;
        saveWeatherIniFromUI();
        m_statusLabel->setText("Saved: " + path);
    }
}

void WeatherEditorModule::onLoadColorCurves()
{
    QString path = QFileDialog::getOpenFileName(this, "Open colorCurves.ini", QString(), "Color Curves INI (*.ini);;All Files (*)");
    if (!path.isEmpty()) {
        m_colorCurvesPath = path;
        loadColorCurvesToUI();
        m_statusLabel->setText("Loaded: " + path);
    }
}

void WeatherEditorModule::onSaveColorCurves()
{
    QString path = m_colorCurvesPath.isEmpty() ?
        QFileDialog::getSaveFileName(this, "Save colorCurves.ini", QString(), "Color Curves INI (*.ini)") : m_colorCurvesPath;
    if (!path.isEmpty()) {
        m_colorCurvesPath = path;
        saveColorCurvesFromUI();
        m_statusLabel->setText("Saved: " + path);
    }
}

void WeatherEditorModule::onResetDefaults()
{
    m_preset = WeatherConfigParser::getDefaultClear();
    loadWeatherIniToUI();
    loadColorCurvesToUI();
    m_statusLabel->setText("Reset to defaults");
}

void WeatherEditorModule::onUpdatePreview()
{
    saveWeatherIniFromUI();
    saveColorCurvesFromUI();
    updatePreview();
}

void WeatherPreviewWidget::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);
    if (parentModule) {
        QPainter painter(this);
        painter.drawImage(rect(), parentModule->m_previewBuffer, parentModule->m_previewBuffer.rect());
    }
}

void WeatherEditorModule::setupUi()
{
    loadWeatherIniToUI();
    loadColorCurvesToUI();
    updatePreview();
}
void WeatherEditorModule::loadWeatherIniToUI()
{
    m_cloudCoverSpin->setValue(m_preset.cloudIntensity);
    m_cloudCutoffSpin->setValue(m_preset.cloudCutoff);
    m_cloudColorSpin->setValue(m_preset.cloudColor);
    m_cloudWidthSpin->setValue(m_preset.cloudWidth);
    m_cloudHeightSpin->setValue(m_preset.cloudHeight);
    m_cloudRadiusSpin->setValue(m_preset.cloudRadius);
    m_cloudNumberSpin->setValue(m_preset.cloudNumber);
    m_cloudSpeedSpin->setValue(m_preset.cloudSpeed);
    m_fogBlendSpin->setValue(m_preset.fogDensity);
    m_fogDistanceSpin->setValue(m_preset.fogDistance);
    m_fogColorBtn->setStyleSheet(QString("background-color: rgb(%1,%2,%3)")
        .arg(m_preset.fogColor.red()).arg(m_preset.fogColor.green()).arg(m_preset.fogColor.blue()));
    m_weatherNameEdit->setText(m_preset.name);
    m_temperatureCoeffSpin->setValue(m_preset.temperatureCoeff);
}

void WeatherEditorModule::saveWeatherIniFromUI()
{
    m_preset.cloudIntensity = m_cloudCoverSpin->value();
    m_preset.cloudCutoff = m_cloudCutoffSpin->value();
    m_preset.cloudColor = m_cloudColorSpin->value();
    m_preset.cloudWidth = m_cloudWidthSpin->value();
    m_preset.cloudHeight = m_cloudHeightSpin->value();
    m_preset.cloudRadius = m_cloudRadiusSpin->value();
    m_preset.cloudNumber = m_cloudNumberSpin->value();
    m_preset.cloudSpeed = m_cloudSpeedSpin->value();
    m_preset.fogDensity = m_fogBlendSpin->value();
    m_preset.fogDistance = m_fogDistanceSpin->value();
    m_preset.name = m_weatherNameEdit->text();
    m_preset.temperatureCoeff = m_temperatureCoeffSpin->value();
}

void WeatherEditorModule::loadColorCurvesToUI()
{
    if (m_colorCurvesPath.isEmpty()) return;
    QFile file(m_colorCurvesPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QTextStream in(&file);
    QString content = file.readAll();
    file.close();

    auto getVal = [&](const QString& key, float def) -> float {
        int idx = content.indexOf(key + "=");
        if (idx < 0) return def;
        int end = content.indexOf("\n", idx);
        if (end < 0) end = content.length();
        return content.mid(idx + key.length() + 1, end - idx - key.length() - 1).toFloat();
    };

    m_horizonLowColor = QColor::fromRgbF(getVal("HORIZON_LOW_R", 1.0f), getVal("HORIZON_LOW_G", 0.5f), getVal("HORIZON_LOW_B", 0.2f));
    m_horizonHighColor = QColor::fromRgbF(getVal("HORIZON_HIGH_R", 1.0f), getVal("HORIZON_HIGH_G", 0.8f), getVal("HORIZON_HIGH_B", 0.5f));
    m_skyLowColor = QColor::fromRgbF(getVal("SKY_LOW_R", 0.3f), getVal("SKY_LOW_G", 0.5f), getVal("SKY_LOW_B", 1.0f));
    m_skyHighColor = QColor::fromRgbF(getVal("SKY_HIGH_R", 0.1f), getVal("SKY_HIGH_G", 0.3f), getVal("SKY_HIGH_B", 0.8f));
    m_sunLowColor = QColor::fromRgbF(getVal("SUN_LOW_R", 1.0f), getVal("SUN_LOW_G", 0.9f), getVal("SUN_LOW_B", 0.7f));
    m_sunHighColor = QColor::fromRgbF(getVal("SUN_HIGH_R", 1.0f), getVal("SUN_HIGH_G", 1.0f), getVal("SUN_HIGH_B", 0.9f));
    m_ambientLowColor = QColor::fromRgbF(getVal("AMBIENT_LOW_R", 0.5f), getVal("AMBIENT_LOW_G", 0.5f), getVal("AMBIENT_LOW_B", 0.6f));
    m_ambientHighColor = QColor::fromRgbF(getVal("AMBIENT_HIGH_R", 0.3f), getVal("AMBIENT_HIGH_G", 0.3f), getVal("AMBIENT_HIGH_B", 0.4f));

    // Update button colors
    if (m_horizonLowColorBtn) m_horizonLowColorBtn->setStyleSheet(QString("background-color: rgb(%1,%2,%3)").arg(m_horizonLowColor.red()).arg(m_horizonLowColor.green()).arg(m_horizonLowColor.blue()));
    if (m_horizonHighColorBtn) m_horizonHighColorBtn->setStyleSheet(QString("background-color: rgb(%1,%2,%3)").arg(m_horizonHighColor.red()).arg(m_horizonHighColor.green()).arg(m_horizonHighColor.blue()));
    if (m_skyLowColorBtn) m_skyLowColorBtn->setStyleSheet(QString("background-color: rgb(%1,%2,%3)").arg(m_skyLowColor.red()).arg(m_skyLowColor.green()).arg(m_skyLowColor.blue()));
    if (m_skyHighColorBtn) m_skyHighColorBtn->setStyleSheet(QString("background-color: rgb(%1,%2,%3)").arg(m_skyHighColor.red()).arg(m_skyHighColor.green()).arg(m_skyHighColor.blue()));
    if (m_sunLowColorBtn) m_sunLowColorBtn->setStyleSheet(QString("background-color: rgb(%1,%2,%3)").arg(m_sunLowColor.red()).arg(m_sunLowColor.green()).arg(m_sunLowColor.blue()));
    if (m_sunHighColorBtn) m_sunHighColorBtn->setStyleSheet(QString("background-color: rgb(%1,%2,%3)").arg(m_sunHighColor.red()).arg(m_sunHighColor.green()).arg(m_sunHighColor.blue()));
    if (m_ambientLowColorBtn) m_ambientLowColorBtn->setStyleSheet(QString("background-color: rgb(%1,%2,%3)").arg(m_ambientLowColor.red()).arg(m_ambientLowColor.green()).arg(m_ambientLowColor.blue()));
    if (m_ambientHighColorBtn) m_ambientHighColorBtn->setStyleSheet(QString("background-color: rgb(%1,%2,%3)").arg(m_ambientHighColor.red()).arg(m_ambientHighColor.green()).arg(m_ambientHighColor.blue()));
}

void WeatherEditorModule::saveColorCurvesFromUI()
{
    if (m_colorCurvesPath.isEmpty()) return;
    QFile file(m_colorCurvesPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&file);
    out << "HORIZON_LOW_R=" << m_horizonLowColor.redF() << "\n";
    out << "HORIZON_LOW_G=" << m_horizonLowColor.greenF() << "\n";
    out << "HORIZON_LOW_B=" << m_horizonLowColor.blueF() << "\n";
    out << "HORIZON_HIGH_R=" << m_horizonHighColor.redF() << "\n";
    out << "HORIZON_HIGH_G=" << m_horizonHighColor.greenF() << "\n";
    out << "HORIZON_HIGH_B=" << m_horizonHighColor.blueF() << "\n";
    out << "SKY_LOW_R=" << m_skyLowColor.redF() << "\n";
    out << "SKY_LOW_G=" << m_skyLowColor.greenF() << "\n";
    out << "SKY_LOW_B=" << m_skyLowColor.blueF() << "\n";
    out << "SKY_HIGH_R=" << m_skyHighColor.redF() << "\n";
    out << "SKY_HIGH_G=" << m_skyHighColor.greenF() << "\n";
    out << "SKY_HIGH_B=" << m_skyHighColor.blueF() << "\n";
    out << "SUN_LOW_R=" << m_sunLowColor.redF() << "\n";
    out << "SUN_LOW_G=" << m_sunLowColor.greenF() << "\n";
    out << "SUN_LOW_B=" << m_sunLowColor.blueF() << "\n";
    out << "SUN_HIGH_R=" << m_sunHighColor.redF() << "\n";
    out << "SUN_HIGH_G=" << m_sunHighColor.greenF() << "\n";
    out << "SUN_HIGH_B=" << m_sunHighColor.blueF() << "\n";
    out << "AMBIENT_LOW_R=" << m_ambientLowColor.redF() << "\n";
    out << "AMBIENT_LOW_G=" << m_ambientLowColor.greenF() << "\n";
    out << "AMBIENT_LOW_B=" << m_ambientLowColor.blueF() << "\n";
    out << "AMBIENT_HIGH_R=" << m_ambientHighColor.redF() << "\n";
    out << "AMBIENT_HIGH_G=" << m_ambientHighColor.greenF() << "\n";
    out << "AMBIENT_HIGH_B=" << m_ambientHighColor.blueF() << "\n";
    file.close();
}
void WeatherEditorModule::updatePreview()
{
    if (!m_previewWidget) return;

    int w = m_previewBuffer.width();
    int h = m_previewBuffer.height();

    m_previewBuffer.fill(Qt::black);
    QPainter painter(&m_previewBuffer);

    // Sky gradient
    QLinearGradient skyGrad(0, 0, 0, h * 0.6f);
    skyGrad.setColorAt(0.0, m_skyHighColor);
    skyGrad.setColorAt(0.5, m_skyLowColor);
    skyGrad.setColorAt(1.0, m_horizonHighColor);
    painter.fillRect(0, 0, w, h * 0.6f, skyGrad);

    // Horizon band
    QLinearGradient horizGrad(0, h * 0.55f, 0, h * 0.65f);
    horizGrad.setColorAt(0.0, m_horizonHighColor);
    horizGrad.setColorAt(0.5, m_horizonLowColor);
    horizGrad.setColorAt(1.0, m_sunLowColor);
    painter.fillRect(0, h * 0.55f, w, h * 0.1f, horizGrad);

    // Ground
    painter.fillRect(0, h * 0.65f, w, h * 0.35f, QColor(80, 80, 80));

    // Sun
    int sunX = w * 0.5f;
    int sunY = h * 0.45f;
    QRadialGradient sunGrad(sunX, sunY, 40);
    sunGrad.setColorAt(0.0, m_sunHighColor);
    sunGrad.setColorAt(0.5, m_sunLowColor);
    sunGrad.setColorAt(1.0, Qt::transparent);
    painter.setBrush(sunGrad);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPoint(sunX, sunY), 40, 40);

    // Cloud overlay
    if (m_preset.cloudIntensity > 0.1) {
        painter.setOpacity(m_preset.cloudIntensity * 0.4);
        painter.fillRect(0, 0, w, h * 0.5f, QColor(200, 200, 210));
        painter.setOpacity(1.0);
    }

    // Fog overlay
    if (m_preset.fogDensity > 0.01) {
        painter.setOpacity(m_preset.fogDensity);
        painter.fillRect(0, h * 0.5f, w, h * 0.5f, m_preset.fogColor);
        painter.setOpacity(1.0);
    }

    painter.end();
    m_previewWidget->update();
}

QVector<float> WeatherEditorModule::QColorToVector(const QColor& c)
{
    return {c.redF(), c.greenF(), c.blueF(), c.alphaF()};
}

QColor WeatherEditorModule::QColorVectorToQColor(const QVector<float>& c)
{
    if (c.size() < 3) return Qt::white;
    return QColor::fromRgbF(c[0], c[1], c[2], c.size() > 3 ? c[3] : 1.0f);
}

QJsonObject WeatherEditorModule::serializeProject() const
{
    QJsonObject data;

    data["weatherIniPath"] = m_weatherIniPath;
    data["colorCurvesPath"] = m_colorCurvesPath;
    data["weatherName"] = m_weatherNameEdit ? m_weatherNameEdit->text() : QString();
    data["temperatureCoeff"] = m_temperatureCoeffSpin ? m_temperatureCoeffSpin->value() : 0.0;

    QJsonObject cloud;
    cloud["cover"] = m_cloudCoverSpin ? m_cloudCoverSpin->value() : 0.0;
    cloud["cutoff"] = m_cloudCutoffSpin ? m_cloudCutoffSpin->value() : 0.0;
    cloud["color"] = m_cloudColorSpin ? m_cloudColorSpin->value() : 0.0;
    cloud["width"] = m_cloudWidthSpin ? m_cloudWidthSpin->value() : 0.0;
    cloud["height"] = m_cloudHeightSpin ? m_cloudHeightSpin->value() : 0.0;
    cloud["radius"] = m_cloudRadiusSpin ? m_cloudRadiusSpin->value() : 0.0;
    cloud["number"] = m_cloudNumberSpin ? m_cloudNumberSpin->value() : 0;
    cloud["speed"] = m_cloudSpeedSpin ? m_cloudSpeedSpin->value() : 0.0;
    data["cloud"] = cloud;

    QJsonObject fog;
    fog["color"] = m_fogColorBtn ? m_fogColorBtn->palette().color(QPalette::Button).name() : "#FFFFFF";
    fog["blend"] = m_fogBlendSpin ? m_fogBlendSpin->value() : 0.0;
    fog["distance"] = m_fogDistanceSpin ? m_fogDistanceSpin->value() : 0.0;
    data["fog"] = fog;

    QJsonObject colors;
    colors["horizonLow"] = m_horizonLowColor.name();
    colors["horizonHigh"] = m_horizonHighColor.name();
    colors["skyLow"] = m_skyLowColor.name();
    colors["skyHigh"] = m_skyHighColor.name();
    colors["sunLow"] = m_sunLowColor.name();
    colors["sunHigh"] = m_sunHighColor.name();
    colors["ambientLow"] = m_ambientLowColor.name();
    colors["ambientHigh"] = m_ambientHighColor.name();
    data["colorCurves"] = colors;

    return data;
}

void WeatherEditorModule::deserializeProject(const QJsonObject& data)
{
    m_weatherIniPath = data["weatherIniPath"].toString();
    m_colorCurvesPath = data["colorCurvesPath"].toString();

    if (m_weatherNameEdit) m_weatherNameEdit->setText(data["weatherName"].toString());
    if (m_temperatureCoeffSpin) m_temperatureCoeffSpin->setValue(data["temperatureCoeff"].toDouble());

    QJsonObject cloud = data["cloud"].toObject();
    if (m_cloudCoverSpin) m_cloudCoverSpin->setValue(cloud["cover"].toDouble());
    if (m_cloudCutoffSpin) m_cloudCutoffSpin->setValue(cloud["cutoff"].toDouble());
    if (m_cloudColorSpin) m_cloudColorSpin->setValue(cloud["color"].toDouble());
    if (m_cloudWidthSpin) m_cloudWidthSpin->setValue(cloud["width"].toDouble());
    if (m_cloudHeightSpin) m_cloudHeightSpin->setValue(cloud["height"].toDouble());
    if (m_cloudRadiusSpin) m_cloudRadiusSpin->setValue(cloud["radius"].toDouble());
    if (m_cloudNumberSpin) m_cloudNumberSpin->setValue(cloud["number"].toInt());
    if (m_cloudSpeedSpin) m_cloudSpeedSpin->setValue(cloud["speed"].toDouble());

    QJsonObject fog = data["fog"].toObject();
    if (m_fogColorBtn) {
        QColor fc(fog["color"].toString("#FFFFFF"));
        m_fogColorBtn->setStyleSheet(QString("background-color: %1").arg(fc.name()));
    }
    if (m_fogBlendSpin) m_fogBlendSpin->setValue(fog["blend"].toDouble());
    if (m_fogDistanceSpin) m_fogDistanceSpin->setValue(fog["distance"].toDouble());

    QJsonObject colors = data["colorCurves"].toObject();
    m_horizonLowColor = QColor(colors["horizonLow"].toString("#87CEEB"));
    m_horizonHighColor = QColor(colors["horizonHigh"].toString("#4A90D9"));
    m_skyLowColor = QColor(colors["skyLow"].toString("#87CEEB"));
    m_skyHighColor = QColor(colors["skyHigh"].toString("#1E3A5F"));
    m_sunLowColor = QColor(colors["sunLow"].toString("#FFA500"));
    m_sunHighColor = QColor(colors["sunHigh"].toString("#FF6347"));
    m_ambientLowColor = QColor(colors["ambientLow"].toString("#B0C4DE"));
    m_ambientHighColor = QColor(colors["ambientHigh"].toString("#708090"));

    updatePreview();
}

} // namespace ks
