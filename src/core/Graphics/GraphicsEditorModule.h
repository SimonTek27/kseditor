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
#include <QSlider>
#include <QLabel>
#include <QSplitter>
#include <QGroupBox>
#include <QFormLayout>
#include <QScrollArea>
#include <QListWidget>

namespace ks {

class GraphicsEditorModule : public ModuleGuiBase {
    Q_OBJECT
public:
    explicit GraphicsEditorModule(QWidget* parent = nullptr);
    ~GraphicsEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;

    QString moduleName() const override { return "Graphics Viewport"; }
    QString moduleId() const override { return "graphics"; }
    int getModulePriority() const override { return 40; }

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

protected:
    void buildUI() override;
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onSceneNodeSelected(QTreeWidgetItem* item, int column);
    void onPassSelected(QTreeWidgetItem* item, int column);
    void onShaderSelected(QTreeWidgetItem* item, int column);
    void onEffectToggled(QTreeWidgetItem* item, int column);
    void onResolutionChanged(int index);
    void onVSyncToggled(bool checked);
    void onMSAAChanged(int index);
    void onShadowQualityChanged(int index);
    void onTextureQualityChanged(int index);
    void onAnisotropyChanged(int value);
    void onGammaChanged(double value);
    void onExposureChanged(double value);
    void onBloomToggled(bool checked);
    void onSSAOToggled(bool checked);
    void onSSRToggled(bool checked);
    void onDOFToggled(bool checked);
    void onDrawDistanceChanged(int value);
    void onFOVChanged(int value);
    void onAddShader();
    void onRemoveShader();
    void onAddPass();
    void onRemovePass();
    void onLoadScene();
    void onExportScreenshot();
    void onResetDefaults();

private:
    void setupSceneGraphTab();
    void setupRenderGraphTab();
    void setupShadersTab();
    void setupPostProcessingTab();
    void setupSettingsTab();
    void populateSceneGraph();
    void populateRenderGraph();
    void populateShaders();

    QTabWidget* m_tabWidget = nullptr;

    QWidget* m_sceneGraphTab = nullptr;
    QTreeWidget* m_sceneTree = nullptr;
    QLabel* m_sceneInfoLabel = nullptr;
    QPushButton* m_loadSceneBtn = nullptr;
    QPushButton* m_exportScreenshotBtn = nullptr;

    QWidget* m_renderGraphTab = nullptr;
    QTreeWidget* m_passTree = nullptr;
    QPushButton* m_addPassBtn = nullptr;
    QPushButton* m_removePassBtn = nullptr;
    QComboBox* m_passTypeCombo = nullptr;

    QWidget* m_shadersTab = nullptr;
    QTreeWidget* m_shaderTree = nullptr;
    QTextEdit* m_shaderCodeEdit = nullptr;
    QPushButton* m_addShaderBtn = nullptr;
    QPushButton* m_removeShaderBtn = nullptr;
    QComboBox* m_shaderTypeCombo = nullptr;
    QComboBox* m_shaderLanguageCombo = nullptr;

    QWidget* m_postFxTab = nullptr;
    QTreeWidget* m_effectTree = nullptr;
    QDoubleSpinBox* m_gammaSpin = nullptr;
    QDoubleSpinBox* m_exposureSpin = nullptr;
    QSlider* m_drawDistanceSlider = nullptr;
    QSlider* m_fovSlider = nullptr;
    QLabel* m_drawDistanceLabel = nullptr;
    QLabel* m_fovLabel = nullptr;

    QWidget* m_settingsTab = nullptr;
    QComboBox* m_resolutionCombo = nullptr;
    QCheckBox* m_vsyncCheck = nullptr;
    QComboBox* m_msaaCombo = nullptr;
    QComboBox* m_shadowQualityCombo = nullptr;
    QComboBox* m_textureQualityCombo = nullptr;
    QSpinBox* m_anisotropySpin = nullptr;
    QPushButton* m_resetDefaultsBtn = nullptr;
};

} // namespace ks
