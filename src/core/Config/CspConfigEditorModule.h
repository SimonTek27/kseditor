#pragma once

#include "../editor/EditorModule.h"
#include "CspConfigParser.h"
#include <QDockWidget>
#include <QTabWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QTextEdit>
#include <QPlainTextEdit>

namespace ks {

class CspConfigEditorModule : public EditorModule {
    Q_OBJECT
public:
    explicit CspConfigEditorModule(QWidget* parent = nullptr);
    ~CspConfigEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "CSP Config Editor"; }
    QString moduleId() const override { return "cspConfigEditor"; }
    int getModulePriority() const override { return 50; }
    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;

protected:
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onLoad();
    void onSave();
    void onReset();
    void onBrowseAcPath();
    void onRefreshStatus();
    void onExtSelectionChanged();

private:
    void loadFromManager();
    void saveToManager();
    void refreshExtList();

    CspConfigManager* m_cspManager = nullptr;
    QDockWidget* m_dockWidget = nullptr;
    QTabWidget* m_tabWidget = nullptr;

    // General
    QLineEdit* m_acPathEdit = nullptr;
    QLabel* m_cspStatusLabel = nullptr;
    QLabel* m_cspVersionLabel = nullptr;

    // WeatherFX
    QCheckBox* m_weEnabled = nullptr;
    QLineEdit* m_weScript = nullptr;
    QDoubleSpinBox* m_weTimeMult = nullptr;
    QCheckBox* m_weRealWeather = nullptr;

    // LightingFX
    QCheckBox* m_lfEnabled = nullptr;
    QCheckBox* m_lfDynamicLights = nullptr;
    QCheckBox* m_lfOcclusion = nullptr;
    QDoubleSpinBox* m_lfAmbient = nullptr;
    QDoubleSpinBox* m_lfSun = nullptr;

    // ParticlesFX
    QCheckBox* m_pfEnabled = nullptr;
    QCheckBox* m_pfSmoke = nullptr;
    QCheckBox* m_pfSparks = nullptr;
    QCheckBox* m_pfGrass = nullptr;
    QDoubleSpinBox* m_pfSmokeIntensity = nullptr;
    QDoubleSpinBox* m_pfSparkIntensity = nullptr;

    // Physics Extensions
    QCheckBox* m_phEnabled = nullptr;
    QCheckBox* m_phAero = nullptr;
    QCheckBox* m_phSuspension = nullptr;
    QCheckBox* m_phTires = nullptr;
    QDoubleSpinBox* m_phAeroMult = nullptr;

    // Car Extensions
    QCheckBox* m_ceEnabled = nullptr;
    QCheckBox* m_ceReverseLights = nullptr;
    QCheckBox* m_ceTurnSignals = nullptr;
    QCheckBox* m_ceOdometer = nullptr;
    QCheckBox* m_ceWipers = nullptr;

    // Track Extensions
    QCheckBox* m_teEnabled = nullptr;
    QCheckBox* m_teGrassFx = nullptr;
    QCheckBox* m_teParticles = nullptr;
    QDoubleSpinBox* m_teGrassDistance = nullptr;

    // Extensions Browser
    QListWidget* m_extList = nullptr;
    QTextEdit* m_extConfigView = nullptr;

    // Shader Compiler
    QPlainTextEdit* m_shaderSourceEdit = nullptr;
    QTextEdit* m_shaderResultView = nullptr;
    QLabel* m_compilerStatusLabel = nullptr;

    QPushButton* m_browseBtn = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QPushButton* m_loadBtn = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_resetBtn = nullptr;
    QLabel* m_statusLabel = nullptr;
};

} // namespace ks
