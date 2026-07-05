#pragma once

#include "../../core/editor/EditorModule.h"
#include "WeatherConfigParser.h"
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTabWidget>
#include <QTableWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QColorDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QPainter>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QPaintEvent>

namespace ks {

class WeatherEditorModule;

class WeatherPreviewWidget : public QWidget {
public:
    using QWidget::QWidget;

    WeatherEditorModule* parentModule = nullptr;

protected:
    void paintEvent(QPaintEvent* event) override;
};

class WeatherEditorModule : public EditorModule {
    Q_OBJECT
    friend class WeatherPreviewWidget;
public:
    explicit WeatherEditorModule(QWidget* parent = nullptr);
    ~WeatherEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "Weather Editor"; }
    QString moduleId() const override { return "weatherEditor"; }
    int getModulePriority() const override { return 25; }
    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;

protected:
    void onActivation() override;
    void onDeactivation() override;

private slots:
    // Cloud settings
    void onCloudCoverChanged(double value);
    void onCloudCutoffChanged(double value);
    void onCloudColorChanged(double value);
    void onCloudWidthChanged(double value);
    void onCloudHeightChanged(double value);
    void onCloudRadiusChanged(double value);
    void onCloudNumberChanged(int value);
    void onCloudSpeedChanged(double value);

    // Fog settings
    void onFogColorClicked();
    void onFogBlendChanged(double value);
    void onFogDistanceChanged(double value);

    // Launcher
    void onWeatherNameChanged(const QString& name);
    void onTemperatureCoeffChanged(double value);

    // Color curves
    void onHorizonLowColorClicked();
    void onHorizonHighColorClicked();
    void onSkyLowColorClicked();
    void onSkyHighColorClicked();
    void onSunLowColorClicked();
    void onSunHighColorClicked();
    void onAmbientLowColorClicked();
    void onAmbientHighColorClicked();

    // Preset management
    void onLoadPreset();
    void onSavePreset();
    void onPresetSelected(int index);
    void onNewPreset();
    void onDeletePreset();

    // File operations
    void onLoadWeatherIni();
    void onSaveWeatherIni();
    void onLoadColorCurves();
    void onSaveColorCurves();
    void onResetDefaults();

    // Preview
    void onUpdatePreview();

private:
    void setupUi();
    void loadWeatherIniToUI();
    void saveWeatherIniFromUI();
    void loadColorCurvesToUI();
    void saveColorCurvesFromUI();
    void updatePreview();
    QColor QColorVectorToQColor(const QVector<float>& c);
    QVector<float> QColorToVector(const QColor& c);

    QDockWidget* m_dockWidget = nullptr;
    QTabWidget* m_tabWidget = nullptr;

    // Cloud settings
    QDoubleSpinBox* m_cloudCoverSpin = nullptr;
    QDoubleSpinBox* m_cloudCutoffSpin = nullptr;
    QDoubleSpinBox* m_cloudColorSpin = nullptr;
    QDoubleSpinBox* m_cloudWidthSpin = nullptr;
    QDoubleSpinBox* m_cloudHeightSpin = nullptr;
    QDoubleSpinBox* m_cloudRadiusSpin = nullptr;
    QSpinBox* m_cloudNumberSpin = nullptr;
    QDoubleSpinBox* m_cloudSpeedSpin = nullptr;

    // Fog settings
    QPushButton* m_fogColorBtn = nullptr;
    QDoubleSpinBox* m_fogBlendSpin = nullptr;
    QDoubleSpinBox* m_fogDistanceSpin = nullptr;

    // Launcher
    QLineEdit* m_weatherNameEdit = nullptr;
    QDoubleSpinBox* m_temperatureCoeffSpin = nullptr;

    // Color curves
    QPushButton* m_horizonLowColorBtn = nullptr;
    QPushButton* m_horizonHighColorBtn = nullptr;
    QPushButton* m_skyLowColorBtn = nullptr;
    QPushButton* m_skyHighColorBtn = nullptr;
    QPushButton* m_sunLowColorBtn = nullptr;
    QPushButton* m_sunHighColorBtn = nullptr;
    QPushButton* m_ambientLowColorBtn = nullptr;
    QPushButton* m_ambientHighColorBtn = nullptr;

    // Color curve data
    QColor m_horizonLowColor;
    QColor m_horizonHighColor;
    QColor m_skyLowColor;
    QColor m_skyHighColor;
    QColor m_sunLowColor;
    QColor m_sunHighColor;
    QColor m_ambientLowColor;
    QColor m_ambientHighColor;

    // Preset list
    QListWidget* m_presetList = nullptr;
    QPushButton* m_loadPresetBtn = nullptr;
    QPushButton* m_savePresetBtn = nullptr;
    QPushButton* m_newPresetBtn = nullptr;
    QPushButton* m_deletePresetBtn = nullptr;

    // Actions
    QPushButton* m_loadIniBtn = nullptr;
    QPushButton* m_saveIniBtn = nullptr;
    QPushButton* m_loadCurvesBtn = nullptr;
    QPushButton* m_saveCurvesBtn = nullptr;
    QPushButton* m_resetBtn = nullptr;
    QPushButton* m_previewBtn = nullptr;
    QLabel* m_statusLabel = nullptr;

    // Preview
    WeatherPreviewWidget* m_previewWidget = nullptr;
    QImage m_previewBuffer;

    // Data
    WeatherConfigParser::WeatherPreset m_preset;
    QString m_weatherIniPath;
    QString m_colorCurvesPath;
    QVector<QString> m_presetNames;
};

} // namespace ks
