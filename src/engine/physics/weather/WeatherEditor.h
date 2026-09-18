#pragma once

/**
 * @file WeatherEditor.h
 * @brief Weather editor widget for weather configuration
 * @copyright KS Physics Engine
 */

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include <QScrollArea>
#include <QColorDialog>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QLineEdit>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QTabWidget>

#include "../WeatherConfig.h"

namespace ks {
namespace physics {

/**
 * @class WeatherEditor
 * @brief Widget for editing weather configuration parameters
 */
class WeatherEditor : public QWidget {
    Q_OBJECT
    
public:
    explicit WeatherEditor(QWidget* parent = nullptr);
    ~WeatherEditor() override = default;
    
    void setWeatherConfig(const config::WeatherConfig& config);
    config::WeatherConfig weatherConfig() const;
    
    void setReadOnly(bool readOnly);
    bool isReadOnly() const;
    
signals:
    void configChanged();
    void sequenceAdded(const QString& name);
    void sequenceRemoved(const QString& name);
    void keyframeAdded(const QString& sequenceName, double time);
    void keyframeRemoved(const QString& sequenceName, double time);
    
public slots:
    void updateUI();
    void addSequence();
    void removeSequence();
    void addKeyframe();
    void removeKeyframe();
    
private slots:
    void onWeatherTypeChanged(int index);
    void onAmbientTempChanged(double value);
    void onTrackTempChanged(double value);
    void onWindSpeedChanged(double value);
    void onWindDirectionChanged(double value);
    void onRainIntensityChanged(double value);
    void onCloudIntensityChanged(double value);
    void onFogIntensityChanged(double value);
    void onHumidityChanged(double value);
    void onTimeOfDayChanged(double value);
    void onSunDirectionChanged();
    
private:
    void setupUI();
    void setupWeatherTypeGroup();
    void setupTemperatureGroup();
    void setupWindGroup();
    void setupPrecipitationGroup();
    void setupAtmosphereGroup();
    void setupTimeGroup();
    void setupSequenceControls();
    void setupPreview();
    
    void updateFromConfig();
    void updateToConfig();
    
    config::WeatherConfig m_config;
    bool m_readOnly = false;
    bool m_updating = false;
    
    // UI elements
    QTabWidget* m_tabWidget = nullptr;
    
    // Weather type
    QComboBox* m_weatherTypeCombo = nullptr;
    
    // Temperature
    QDoubleSpinBox* m_ambientTempSpin = nullptr;
    QDoubleSpinBox* m_trackTempSpin = nullptr;
    
    // Wind
    QDoubleSpinBox* m_windSpeedSpin = nullptr;
    QDoubleSpinBox* m_windDirectionSpin = nullptr;
    QSlider* m_windSpeedSlider = nullptr;
    
    // Precipitation
    QDoubleSpinBox* m_rainIntensitySpin = nullptr;
    QSlider* m_rainIntensitySlider = nullptr;
    QCheckBox* m_enableRainCheck = nullptr;
    
    // Atmosphere
    QDoubleSpinBox* m_cloudIntensitySpin = nullptr;
    QSlider* m_cloudIntensitySlider = nullptr;
    QDoubleSpinBox* m_fogIntensitySpin = nullptr;
    QSlider* m_fogIntensitySlider = nullptr;
    QDoubleSpinBox* m_humiditySpin = nullptr;
    
    // Time
    QDoubleSpinBox* m_timeOfDaySpin = nullptr;
    QSlider* m_timeOfDaySlider = nullptr;
    
    // Sun direction
    QDoubleSpinBox* m_sunAzimuthSpin = nullptr;
    QDoubleSpinBox* m_sunElevationSpin = nullptr;
    
    // Sequence controls
    QComboBox* m_sequenceCombo = nullptr;
    QPushButton* m_addSequenceBtn = nullptr;
    QPushButton* m_removeSequenceBtn = nullptr;
    QDoubleSpinBox* m_keyframeTimeSpin = nullptr;
    QPushButton* m_addKeyframeBtn = nullptr;
    QPushButton* m_removeKeyframeBtn = nullptr;
    
    // Preview
    QGraphicsView* m_previewView = nullptr;
    QGraphicsScene* m_previewScene = nullptr;
    QGraphicsEllipseItem* m_sunIndicator = nullptr;
};

} // namespace physics
} // namespace ks