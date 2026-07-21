#pragma once

#include "core/editor/ModuleGuiBase.h"
#include <QTabWidget>
#include <QTreeWidget>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLabel>
#include <QSplitter>
#include <QGroupBox>
#include <QFormLayout>
#include <QScrollArea>
#include <QSlider>
#include <QListWidget>

namespace ks {
namespace ppfilters {

class PPFiltersEditorModule : public ModuleGuiBase {
    Q_OBJECT
public:
    explicit PPFiltersEditorModule(QWidget* parent = nullptr);
    ~PPFiltersEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;

    QString moduleName() const override { return "Post-Processing Filters"; }
    QString moduleId() const override { return "ppFilters"; }
    int getModulePriority() const override { return 45; }

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

protected:
    void buildUI() override;
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onFilterSelected(QListWidgetItem* item);
    void onPresetSelected(int index);
    void onBloomToggled(bool checked);
    void onBloomIntensityChanged(double value);
    void onToneMappingChanged(int index);
    void onColorTemperatureChanged(double value);
    void onSaturationChanged(double value);
    void onContrastChanged(double value);
    void onVignetteToggled(bool checked);
    void onVignetteIntensityChanged(double value);
    void onChromaticAberrationToggled(bool checked);
    void onChromaticAberrationChanged(double value);
    void onDepthOfFieldToggled(bool checked);
    void onMotionBlurToggled(bool checked);
    void onMotionBlurChanged(int value);
    void onLoadPreset();
    void onSavePreset();
    void onResetFilter();
    void onRealTimePreviewToggled(bool checked);

private:
    void setupFilterChainTab();
    void setupBloomTab();
    void setupToneMappingTab();
    void setupColorGradingTab();
    void setupLensEffectsTab();
    void populateFilterChain();
    void populatePresets();

    QTabWidget* m_tabWidget = nullptr;

    QWidget* m_filterChainTab = nullptr;
    QListWidget* m_filterChainList = nullptr;
    QPushButton* m_resetFilterBtn = nullptr;
    QCheckBox* m_realtimePreviewCheck = nullptr;
    QComboBox* m_presetCombo = nullptr;
    QPushButton* m_loadPresetBtn = nullptr;
    QPushButton* m_savePresetBtn = nullptr;

    QWidget* m_bloomTab = nullptr;
    QCheckBox* m_bloomCheck = nullptr;
    QDoubleSpinBox* m_bloomIntensitySpin = nullptr;
    QDoubleSpinBox* m_bloomThresholdSpin = nullptr;
    QDoubleSpinBox* m_bloomRadiusSpin = nullptr;

    QWidget* m_toneMappingTab = nullptr;
    QComboBox* m_toneMappingCombo = nullptr;
    QDoubleSpinBox* m_exposureSpin = nullptr;
    QDoubleSpinBox* m_gammaSpin = nullptr;

    QWidget* m_colorGradingTab = nullptr;
    QDoubleSpinBox* m_colorTempSpin = nullptr;
    QDoubleSpinBox* m_saturationSpin = nullptr;
    QDoubleSpinBox* m_contrastSpin = nullptr;
    QSlider* m_liftSlider = nullptr;
    QSlider* m_gammaSlider = nullptr;
    QSlider* m_gainSlider = nullptr;

    QWidget* m_lensEffectsTab = nullptr;
    QCheckBox* m_vignetteCheck = nullptr;
    QDoubleSpinBox* m_vignetteIntensitySpin = nullptr;
    QDoubleSpinBox* m_vignetteRadiusSpin = nullptr;
    QCheckBox* m_chromaticAberrationCheck = nullptr;
    QDoubleSpinBox* m_chromaticAberrationSpin = nullptr;
    QCheckBox* m_dofCheck = nullptr;
    QCheckBox* m_motionBlurCheck = nullptr;
    QSpinBox* m_motionBlurSamplesSpin = nullptr;
};

} // namespace ppfilters
} // namespace ks
