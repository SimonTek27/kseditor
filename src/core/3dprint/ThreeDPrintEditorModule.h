#pragma once

#include "core/editor/ModuleGuiBase.h"
#include <QTabWidget>
#include <QTreeWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QSplitter>
#include <QGroupBox>
#include <QFormLayout>
#include <QScrollArea>
#include <QListWidget>
#include <QProgressBar>
#include <QTextEdit>

namespace ks {
namespace printing {

class ThreeDPrintEditorModule : public ModuleGuiBase {
    Q_OBJECT
public:
    explicit ThreeDPrintEditorModule(QWidget* parent = nullptr);
    ~ThreeDPrintEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;

    QString moduleName() const override { return "3D Printing"; }
    QString moduleId() const override { return "3dprint"; }
    int getModulePriority() const override { return 55; }

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

protected:
    void buildUI() override;
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onOpenModel();
    void onSlice();
    void onSlicePresetChanged(int index);
    void onLayerHeightChanged(double value);
    void onInfillChanged(int value);
    void onInfillPatternChanged(int index);
    void onSupportToggled(bool checked);
    void onAdhesionTypeChanged(int index);
    void onFilamentDiameterChanged(double value);
    void onNozzleTempChanged(int value);
    void onBedTempChanged(int value);
    void onFanSpeedChanged(int value);
    void onPrintSpeedChanged(int value);
    void onGenerateGCode();
    void onPreviewGCode();
    void onExportGCode();
    void onAddPrinterProfile();
    void onRemovePrinterProfile();
    void onProfileSelected(QTreeWidgetItem* item, int column);
    void onDuplicateProfile();
    void onImportProfile();
    void onExportProfile();
    void onLoadFilament();
    void onUnloadFilament();
    void onCalibrateBed();
    void onHomeAll();
    void onShowGCodeContextMenu(const QPoint& pos);

private:
    void setupSliceTab();
    void setupGCodeTab();
    void setupPrinterProfilesTab();
    void setupPreviewTab();
    void setupSettingsTab();
    void refreshProfiles();
    void populateSlicePresets();
    void updateSlicePreview();
    void applyProfile(const QString& name);

    QTabWidget* m_tabWidget = nullptr;

    QWidget* m_sliceTab = nullptr;
    QPushButton* m_openModelBtn = nullptr;
    QLabel* m_modelInfoLabel = nullptr;
    QPushButton* m_sliceBtn = nullptr;
    QProgressBar* m_sliceProgress = nullptr;
    QComboBox* m_slicePresetCombo = nullptr;
    QDoubleSpinBox* m_layerHeightSpin = nullptr;
    QSpinBox* m_infillSpin = nullptr;
    QComboBox* m_infillPatternCombo = nullptr;
    QCheckBox* m_supportCheck = nullptr;
    QDoubleSpinBox* m_supportOverhangSpin = nullptr;
    QComboBox* m_adhesionTypeCombo = nullptr;
    QDoubleSpinBox* m_filamentDiameterSpin = nullptr;
    QSpinBox* m_nozzleTempSpin = nullptr;
    QSpinBox* m_bedTempSpin = nullptr;
    QSpinBox* m_fanSpeedSpin = nullptr;
    QSpinBox* m_printSpeedSpin = nullptr;
    QCheckBox* m_raftCheck = nullptr;
    QCheckBox* m_brimCheck = nullptr;
    QLabel* m_estimatedTimeLabel = nullptr;
    QLabel* m_filamentUsageLabel = nullptr;

    QWidget* m_gCodeTab = nullptr;
    QTextEdit* m_gCodeOutput = nullptr;
    QPushButton* m_generateGCodeBtn = nullptr;
    QPushButton* m_previewGCodeBtn = nullptr;
    QPushButton* m_exportGCodeBtn = nullptr;
    QLabel* m_gCodeInfoLabel = nullptr;
    QTreeWidget* m_gCodeCommandTree = nullptr;

    QWidget* m_profilesTab = nullptr;
    QTreeWidget* m_profileTree = nullptr;
    QPushButton* m_addProfileBtn = nullptr;
    QPushButton* m_removeProfileBtn = nullptr;
    QPushButton* m_duplicateProfileBtn = nullptr;
    QPushButton* m_importProfileBtn = nullptr;
    QPushButton* m_exportProfileBtn = nullptr;
    QGroupBox* m_profileDetailsGroup = nullptr;
    QFormLayout* m_profileDetailsLayout = nullptr;
    QDoubleSpinBox* m_profileNozzleSizeSpin = nullptr;
    QDoubleSpinBox* m_profileMaxVolSpeedSpin = nullptr;
    QDoubleSpinBox* m_profileRetractionDistSpin = nullptr;
    QDoubleSpinBox* m_profileRetractionSpeedSpin = nullptr;

    QWidget* m_previewTab = nullptr;
    QLabel* m_previewLabel = nullptr;
    QComboBox* m_viewModeCombo = nullptr;
    QCheckBox* m_showSupportCheck = nullptr;
    QCheckBox* m_showInfillCheck = nullptr;
    QCheckBox* m_showShellsCheck = nullptr;
    QCheckBox* m_showTravelCheck = nullptr;
    QSlider* m_layerSlider = nullptr;
    QLabel* m_layerLabel = nullptr;
    QLabel* m_previewInfoLabel = nullptr;

    QWidget* m_settingsTab = nullptr;
    QComboBox* m_printerTypeCombo = nullptr;
    QDoubleSpinBox* m_bedSizeXSpin = nullptr;
    QDoubleSpinBox* m_bedSizeYSpin = nullptr;
    QDoubleSpinBox* m_bedSizeZSpin = nullptr;
    QPushButton* m_loadFilamentBtn = nullptr;
    QPushButton* m_unloadFilamentBtn = nullptr;
    QPushButton* m_calibrateBedBtn = nullptr;
    QPushButton* m_homeAllBtn = nullptr;
    QLabel* m_printerStatusLabel = nullptr;
    QSpinBox* m_shellCountSpin = nullptr;
    QDoubleSpinBox* m_shellThicknessSpin = nullptr;
    QDoubleSpinBox* m_topBottomThicknessSpin = nullptr;
};

} // namespace printing
} // namespace ks
