#include "WeatherEditor.h"
#include "../WeatherConfig.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace ks {
namespace physics {

// ============================================================================
// Constructor/Destructor
// ============================================================================

WeatherEditor::WeatherEditor(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    updateFromConfig();
}

// ============================================================================
// Public Methods
// ============================================================================

void WeatherEditor::setWeatherConfig(const config::WeatherConfig& config) {
    m_config = config;
    updateFromConfig();
}

config::WeatherConfig WeatherEditor::weatherConfig() const {
    return m_config;
}

void WeatherEditor::setReadOnly(bool readOnly) {
    m_readOnly = readOnly;
    
    // Enable/disable all input widgets
    QList<QWidget*> widgets = findChildren<QWidget*>();
    for (QWidget* widget : widgets) {
        if (qobject_cast<QAbstractButton*>(widget) ||
            qobject_cast<QAbstractSpinBox*>(widget) ||
            qobject_cast<QSlider*>(widget) ||
            qobject_cast<QComboBox*>(widget)) {
            widget->setEnabled(!readOnly);
        }
    }
}

bool WeatherEditor::isReadOnly() const {
    return m_readOnly;
}

// ============================================================================
// Public Slots
// ============================================================================

void WeatherEditor::updateUI() {
    updateFromConfig();
}

void WeatherEditor::addSequence() {
    bool ok;
    QString name = QInputDialog::getText(this, tr("Add Sequence"),
        tr("Sequence name:"), QLineEdit::Normal, QString(), &ok);
    
    if (ok && !name.isEmpty()) {
        m_config.addSequence(name);
        m_sequenceCombo->addItem(name);
        m_sequenceCombo->setCurrentText(name);
        
        emit sequenceAdded(name);
        emit configChanged();
    }
}

void WeatherEditor::removeSequence() {
    QString name = m_sequenceCombo->currentText();
    if (name.isEmpty()) return;
    
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        tr("Remove Sequence"),
        tr("Are you sure you want to remove sequence '%1'?").arg(name),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        m_config.removeSequence(name);
        
        int index = m_sequenceCombo->findText(name);
        if (index >= 0) {
            m_sequenceCombo->removeItem(index);
        }
        
        emit sequenceRemoved(name);
        emit configChanged();
    }
}

void WeatherEditor::addKeyframe() {
    QString sequenceName = m_sequenceCombo->currentText();
    if (sequenceName.isEmpty()) return;
    
    double time = m_keyframeTimeSpin->value();
    m_config.addKeyframe(sequenceName, time);
    
    emit keyframeAdded(sequenceName, time);
    emit configChanged();
}

void WeatherEditor::removeKeyframe() {
    QString sequenceName = m_sequenceCombo->currentText();
    if (sequenceName.isEmpty()) return;
    
    double time = m_keyframeTimeSpin->value();
    m_config.removeKeyframe(sequenceName, time);
    
    emit keyframeRemoved(sequenceName, time);
    emit configChanged();
}

// ============================================================================
// Private Slots
// ============================================================================

void WeatherEditor::onWeatherTypeChanged(int index) {
    if (m_updating) return;
    
    WeatherConfig::WeatherType type = static_cast<WeatherConfig::WeatherType>(index);
    m_config.setWeatherType(type);
    
    updateFromConfig();
    emit configChanged();
}

void WeatherEditor::onAmbientTempChanged(double value) {
    if (m_updating) return;
    m_config.setAmbientTemp(value);
    emit configChanged();
}

void WeatherEditor::onTrackTempChanged(double value) {
    if (m_updating) return;
    m_config.setTrackTemp(value);
    emit configChanged();
}

void WeatherEditor::onWindSpeedChanged(double value) {
    if (m_updating) return;
    m_config.setWindSpeed(value);
    emit configChanged();
}

void WeatherEditor::onWindDirectionChanged(double value) {
    if (m_updating) return;
    m_config.setWindDirection(value);
    emit configChanged();
}

void WeatherEditor::onRainIntensityChanged(double value) {
    if (m_updating) return;
    m_config.setRainIntensity(value);
    emit configChanged();
}

void WeatherEditor::onCloudIntensityChanged(double value) {
    if (m_updating) return;
    m_config.setCloudIntensity(value);
    emit configChanged();
}

void WeatherEditor::onFogIntensityChanged(double value) {
    if (m_updating) return;
    m_config.setFogIntensity(value);
    emit configChanged();
}

void WeatherEditor::onHumidityChanged(double value) {
    if (m_updating) return;
    m_config.setHumidity(value);
    emit configChanged();
}

void WeatherEditor::onTimeOfDayChanged(double value) {
    if (m_updating) return;
    m_config.setTimeOfDay(value);
    updateFromConfig();
    emit configChanged();
}

void WeatherEditor::onSunDirectionChanged() {
    if (m_updating) return;
    
    float azimuth = static_cast<float>(m_sunAzimuthSpin->value());
    float elevation = static_cast<float>(m_sunElevationSpin->value());
    m_config.setSunDirection(azimuth, elevation);
    
    emit configChanged();
}

// ============================================================================
// Private Methods - UI Setup
// ============================================================================

void WeatherEditor::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    m_tabWidget = new QTabWidget(this);
    
    setupWeatherTypeGroup();
    setupTemperatureGroup();
    setupWindGroup();
    setupPrecipitationGroup();
    setupAtmosphereGroup();
    setupTimeGroup();
    setupSequenceControls();
    setupPreview();
    
    mainLayout->addWidget(m_tabWidget);
}

void WeatherEditor::setupWeatherTypeGroup() {
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    
    QGroupBox* group = new QGroupBox(tr("Weather Type"));
    QVBoxLayout* groupLayout = new QVBoxLayout(group);
    
    m_weatherTypeCombo = new QComboBox();
    m_weatherTypeCombo->addItem(tr("Clear"));
    m_weatherTypeCombo->addItem(tr("Cloudy"));
    m_weatherTypeCombo->addItem(tr("Overcast"));
    m_weatherTypeCombo->addItem(tr("Light Rain"));
    m_weatherTypeCombo->addItem(tr("Rain"));
    m_weatherTypeCombo->addItem(tr("Heavy Rain"));
    m_weatherTypeCombo->addItem(tr("Storm"));
    m_weatherTypeCombo->addItem(tr("Fog"));
    m_weatherTypeCombo->addItem(tr("Night"));
    m_weatherTypeCombo->addItem(tr("Custom"));
    
    connect(m_weatherTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &WeatherEditor::onWeatherTypeChanged);
    
    groupLayout->addWidget(m_weatherTypeCombo);
    layout->addWidget(group);
    layout->addStretch();
    
    m_tabWidget->addTab(widget, tr("Weather"));
}

void WeatherEditor::setupTemperatureGroup() {
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    
    QGroupBox* group = new QGroupBox(tr("Temperature"));
    QGridLayout* groupLayout = new QGridLayout(group);
    
    // Ambient temperature
    groupLayout->addWidget(new QLabel(tr("Ambient (°C):")), 0, 0);
    m_ambientTempSpin = new QDoubleSpinBox();
    m_ambientTempSpin->setRange(-50.0, 60.0);
    m_ambientTempSpin->setSingleStep(0.5);
    m_ambientTempSpin->setDecimals(1);
    connect(m_ambientTempSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &WeatherEditor::onAmbientTempChanged);
    groupLayout->addWidget(m_ambientTempSpin, 0, 1);
    
    // Track temperature
    groupLayout->addWidget(new QLabel(tr("Track (°C):")), 1, 0);
    m_trackTempSpin = new QDoubleSpinBox();
    m_trackTempSpin->setRange(-40.0, 70.0);
    m_trackTempSpin->setSingleStep(0.5);
    m_trackTempSpin->setDecimals(1);
    connect(m_trackTempSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &WeatherEditor::onTrackTempChanged);
    groupLayout->addWidget(m_trackTempSpin, 1, 1);
    
    layout->addWidget(group);
    layout->addStretch();
    
    m_tabWidget->addTab(widget, tr("Temperature"));
}

void WeatherEditor::setupWindGroup() {
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    
    QGroupBox* group = new QGroupBox(tr("Wind"));
    QGridLayout* groupLayout = new QGridLayout(group);
    
    // Wind speed
    groupLayout->addWidget(new QLabel(tr("Speed (km/h):")), 0, 0);
    m_windSpeedSpin = new QDoubleSpinBox();
    m_windSpeedSpin->setRange(0.0, 200.0);
    m_windSpeedSpin->setSingleStep(1.0);
    m_windSpeedSpin->setDecimals(1);
    connect(m_windSpeedSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &WeatherEditor::onWindSpeedChanged);
    groupLayout->addWidget(m_windSpeedSpin, 0, 1);
    
    m_windSpeedSlider = new QSlider(Qt::Horizontal);
    m_windSpeedSlider->setRange(0, 200);
    connect(m_windSpeedSlider, &QSlider::valueChanged, this, [this](int value) {
        m_windSpeedSpin->setValue(value);
    });
    groupLayout->addWidget(m_windSpeedSlider, 0, 2);
    
    // Wind direction
    groupLayout->addWidget(new QLabel(tr("Direction (°):")), 1, 0);
    m_windDirectionSpin = new QDoubleSpinBox();
    m_windDirectionSpin->setRange(0.0, 360.0);
    m_windDirectionSpin->setSingleStep(1.0);
    m_windDirectionSpin->setDecimals(1);
    connect(m_windDirectionSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &WeatherEditor::onWindDirectionChanged);
    groupLayout->addWidget(m_windDirectionSpin, 1, 1);
    
    layout->addWidget(group);
    layout->addStretch();
    
    m_tabWidget->addTab(widget, tr("Wind"));
}

void WeatherEditor::setupPrecipitationGroup() {
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    
    QGroupBox* group = new QGroupBox(tr("Precipitation"));
    QGridLayout* groupLayout = new QGridLayout(group);
    
    // Rain intensity
    groupLayout->addWidget(new QLabel(tr("Rain Intensity:")), 0, 0);
    m_rainIntensitySpin = new QDoubleSpinBox();
    m_rainIntensitySpin->setRange(0.0, 1.0);
    m_rainIntensitySpin->setSingleStep(0.05);
    m_rainIntensitySpin->setDecimals(2);
    connect(m_rainIntensitySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &WeatherEditor::onRainIntensityChanged);
    groupLayout->addWidget(m_rainIntensitySpin, 0, 1);
    
    m_rainIntensitySlider = new QSlider(Qt::Horizontal);
    m_rainIntensitySlider->setRange(0, 100);
    connect(m_rainIntensitySlider, &QSlider::valueChanged, this, [this](int value) {
        m_rainIntensitySpin->setValue(value / 100.0);
    });
    groupLayout->addWidget(m_rainIntensitySlider, 0, 2);
    
    // Enable rain checkbox
    m_enableRainCheck = new QCheckBox(tr("Enable Rain"));
    connect(m_enableRainCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_rainIntensitySpin->setEnabled(checked);
        m_rainIntensitySlider->setEnabled(checked);
        if (!checked) {
            m_rainIntensitySpin->setValue(0.0);
        }
    });
    groupLayout->addWidget(m_enableRainCheck, 1, 0, 1, 3);
    
    layout->addWidget(group);
    layout->addStretch();
    
    m_tabWidget->addTab(widget, tr("Precipitation"));
}

void WeatherEditor::setupAtmosphereGroup() {
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    
    QGroupBox* group = new QGroupBox(tr("Atmosphere"));
    QGridLayout* groupLayout = new QGridLayout(group);
    
    // Cloud intensity
    groupLayout->addWidget(new QLabel(tr("Clouds:")), 0, 0);
    m_cloudIntensitySpin = new QDoubleSpinBox();
    m_cloudIntensitySpin->setRange(0.0, 1.0);
    m_cloudIntensitySpin->setSingleStep(0.05);
    m_cloudIntensitySpin->setDecimals(2);
    connect(m_cloudIntensitySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &WeatherEditor::onCloudIntensityChanged);
    groupLayout->addWidget(m_cloudIntensitySpin, 0, 1);
    
    m_cloudIntensitySlider = new QSlider(Qt::Horizontal);
    m_cloudIntensitySlider->setRange(0, 100);
    connect(m_cloudIntensitySlider, &QSlider::valueChanged, this, [this](int value) {
        m_cloudIntensitySpin->setValue(value / 100.0);
    });
    groupLayout->addWidget(m_cloudIntensitySlider, 0, 2);
    
    // Fog intensity
    groupLayout->addWidget(new QLabel(tr("Fog:")), 1, 0);
    m_fogIntensitySpin = new QDoubleSpinBox();
    m_fogIntensitySpin->setRange(0.0, 1.0);
    m_fogIntensitySpin->setSingleStep(0.05);
    m_fogIntensitySpin->setDecimals(2);
    connect(m_fogIntensitySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &WeatherEditor::onFogIntensityChanged);
    groupLayout->addWidget(m_fogIntensitySpin, 1, 1);
    
    m_fogIntensitySlider = new QSlider(Qt::Horizontal);
    m_fogIntensitySlider->setRange(0, 100);
    connect(m_fogIntensitySlider, &QSlider::valueChanged, this, [this](int value) {
        m_fogIntensitySpin->setValue(value / 100.0);
    });
    groupLayout->addWidget(m_fogIntensitySlider, 1, 2);
    
    // Humidity
    groupLayout->addWidget(new QLabel(tr("Humidity (%):")), 2, 0);
    m_humiditySpin = new QDoubleSpinBox();
    m_humiditySpin->setRange(0.0, 100.0);
    m_humiditySpin->setSingleStep(1.0);
    m_humiditySpin->setDecimals(1);
    connect(m_humiditySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &WeatherEditor::onHumidityChanged);
    groupLayout->addWidget(m_humiditySpin, 2, 1);
    
    layout->addWidget(group);
    layout->addStretch();
    
    m_tabWidget->addTab(widget, tr("Atmosphere"));
}

void WeatherEditor::setupTimeGroup() {
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    
    QGroupBox* timeGroup = new QGroupBox(tr("Time of Day"));
    QGridLayout* timeLayout = new QGridLayout(timeGroup);
    
    // Time of day
    timeLayout->addWidget(new QLabel(tr("Time (hours):")), 0, 0);
    m_timeOfDaySpin = new QDoubleSpinBox();
    m_timeOfDaySpin->setRange(0.0, 24.0);
    m_timeOfDaySpin->setSingleStep(0.25);
    m_timeOfDaySpin->setDecimals(2);
    connect(m_timeOfDaySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &WeatherEditor::onTimeOfDayChanged);
    timeLayout->addWidget(m_timeOfDaySpin, 0, 1);
    
    m_timeOfDaySlider = new QSlider(Qt::Horizontal);
    m_timeOfDaySlider->setRange(0, 2400);
    connect(m_timeOfDaySlider, &QSlider::valueChanged, this, [this](int value) {
        m_timeOfDaySpin->setValue(value / 100.0);
    });
    timeLayout->addWidget(m_timeOfDaySlider, 0, 2);
    
    layout->addWidget(timeGroup);
    
    // Sun direction
    QGroupBox* sunGroup = new QGroupBox(tr("Sun Direction"));
    QGridLayout* sunLayout = new QGridLayout(sunGroup);
    
    sunLayout->addWidget(new QLabel(tr("Azimuth (°):")), 0, 0);
    m_sunAzimuthSpin = new QDoubleSpinBox();
    m_sunAzimuthSpin->setRange(0.0, 360.0);
    m_sunAzimuthSpin->setSingleStep(1.0);
    m_sunAzimuthSpin->setDecimals(1);
    connect(m_sunAzimuthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &WeatherEditor::onSunDirectionChanged);
    sunLayout->addWidget(m_sunAzimuthSpin, 0, 1);
    
    sunLayout->addWidget(new QLabel(tr("Elevation (°):")), 1, 0);
    m_sunElevationSpin = new QDoubleSpinBox();
    m_sunElevationSpin->setRange(-90.0, 90.0);
    m_sunElevationSpin->setSingleStep(1.0);
    m_sunElevationSpin->setDecimals(1);
    connect(m_sunElevationSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &WeatherEditor::onSunDirectionChanged);
    sunLayout->addWidget(m_sunElevationSpin, 1, 1);
    
    layout->addWidget(sunGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(widget, tr("Time"));
}

void WeatherEditor::setupSequenceControls() {
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    
    QGroupBox* group = new QGroupBox(tr("Sequence Controls"));
    QVBoxLayout* groupLayout = new QVBoxLayout(group);
    
    // Sequence selection
    QHBoxLayout* seqLayout = new QHBoxLayout();
    m_sequenceCombo = new QComboBox();
    m_sequenceCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    seqLayout->addWidget(m_sequenceCombo);
    
    m_addSequenceBtn = new QPushButton(tr("Add"));
    connect(m_addSequenceBtn, &QPushButton::clicked, this, &WeatherEditor::addSequence);
    seqLayout->addWidget(m_addSequenceBtn);
    
    m_removeSequenceBtn = new QPushButton(tr("Remove"));
    connect(m_removeSequenceBtn, &QPushButton::clicked, this, &WeatherEditor::removeSequence);
    seqLayout->addWidget(m_removeSequenceBtn);
    
    groupLayout->addLayout(seqLayout);
    
    // Keyframe controls
    QHBoxLayout* kfLayout = new QHBoxLayout();
    kfLayout->addWidget(new QLabel(tr("Time:")));
    
    m_keyframeTimeSpin = new QDoubleSpinBox();
    m_keyframeTimeSpin->setRange(0.0, 3600.0);
    m_keyframeTimeSpin->setSingleStep(1.0);
    m_keyframeTimeSpin->setDecimals(2);
    kfLayout->addWidget(m_keyframeTimeSpin);
    
    m_addKeyframeBtn = new QPushButton(tr("Add Keyframe"));
    connect(m_addKeyframeBtn, &QPushButton::clicked, this, &WeatherEditor::addKeyframe);
    kfLayout->addWidget(m_addKeyframeBtn);
    
    m_removeKeyframeBtn = new QPushButton(tr("Remove Keyframe"));
    connect(m_removeKeyframeBtn, &QPushButton::clicked, this, &WeatherEditor::removeKeyframe);
    kfLayout->addWidget(m_removeKeyframeBtn);
    
    groupLayout->addLayout(kfLayout);
    
    layout->addWidget(group);
    layout->addStretch();
    
    m_tabWidget->addTab(widget, tr("Sequences"));
}

void WeatherEditor::setupPreview() {
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    
    QGroupBox* group = new QGroupBox(tr("Preview"));
    QVBoxLayout* groupLayout = new QVBoxLayout(group);
    
    m_previewScene = new QGraphicsScene(this);
    m_previewView = new QGraphicsView(m_previewScene, this);
    m_previewView->setMinimumHeight(150);
    m_previewView->setMaximumHeight(200);
    
    // Create sky gradient background
    m_previewScene->addRect(0, 0, 300, 100, QPen(Qt::NoPen), QBrush(QColor(135, 206, 235)));
    m_previewScene->addRect(0, 100, 300, 50, QPen(Qt::NoPen), QBrush(QColor(34, 139, 34)));
    
    // Sun indicator
    m_sunIndicator = m_previewScene->addEllipse(0, 0, 20, 20, QPen(Qt::yellow), QBrush(QColor(255, 255, 0)));
    m_sunIndicator->setPos(140, 30);
    
    groupLayout->addWidget(m_previewView);
    
    layout->addWidget(group);
    layout->addStretch();
    
    m_tabWidget->addTab(widget, tr("Preview"));
}

// ============================================================================
// Private Methods - Update
// ============================================================================

void WeatherEditor::updateFromConfig() {
    m_updating = true;
    
    // Weather type
    m_weatherTypeCombo->setCurrentIndex(static_cast<int>(m_config.weatherType()));
    
    // Temperature
    m_ambientTempSpin->setValue(m_config.ambientTemp());
    m_trackTempSpin->setValue(m_config.trackTemp());
    
    // Wind
    m_windSpeedSpin->setValue(m_config.windSpeed());
    m_windSpeedSlider->setValue(static_cast<int>(m_config.windSpeed()));
    m_windDirectionSpin->setValue(m_config.windDirection());
    
    // Precipitation
    m_rainIntensitySpin->setValue(m_config.rainIntensity());
    m_rainIntensitySlider->setValue(static_cast<int>(m_config.rainIntensity() * 100));
    m_enableRainCheck->setChecked(m_config.rainIntensity() > 0.0);
    
    // Atmosphere
    m_cloudIntensitySpin->setValue(m_config.cloudIntensity());
    m_cloudIntensitySlider->setValue(static_cast<int>(m_config.cloudIntensity() * 100));
    m_fogIntensitySpin->setValue(m_config.fogIntensity());
    m_fogIntensitySlider->setValue(static_cast<int>(m_config.fogIntensity() * 100));
    m_humiditySpin->setValue(m_config.humidity() * 100.0);
    
    // Time
    m_timeOfDaySpin->setValue(m_config.timeOfDay());
    m_timeOfDaySlider->setValue(static_cast<int>(m_config.timeOfDay() * 100));
    
    // Sun direction
    m_sunAzimuthSpin->setValue(m_config.sunAzimuth());
    m_sunElevationSpin->setValue(m_config.sunElevation());
    
    // Sequences
    m_sequenceCombo->clear();
    for (const QString& seqName : m_config.sequenceNames()) {
        m_sequenceCombo->addItem(seqName);
    }
    
    // Update preview
    float hour = m_config.timeOfDay();
    QColor skyColor = WeatherConfigParser::getSkyColor(hour, m_config.cloudIntensity());
    m_previewScene->setBackgroundBrush(skyColor);
    
    // Update sun position in preview
    float azimuth = m_config.sunAzimuth() * M_PI / 180.0f;
    float elevation = m_config.sunElevation() * M_PI / 180.0f;
    float x = 150.0f + 100.0f * std::cos(azimuth) * std::cos(elevation);
    float y = 50.0f - 50.0f * std::sin(elevation);
    m_sunIndicator->setPos(x, y);
    
    m_updating = false;
}

} // namespace physics
} // namespace ks