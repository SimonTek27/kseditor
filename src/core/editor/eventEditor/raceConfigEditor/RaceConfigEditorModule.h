#pragma once

#include "../../editor/EditorModule.h"
#include <QDockWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QTabWidget>

namespace ks {

class RaceConfigEditorModule : public EditorModule {
    Q_OBJECT
public:
    explicit RaceConfigEditorModule(QWidget* parent = nullptr);
    ~RaceConfigEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "Race Config Editor"; }
    QString moduleId() const override { return "raceConfigEditor"; }
    int getModulePriority() const override { return 40; }
    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;

protected:
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onLoadFile();
    void onSaveFile();
    void onResetDefaults();

private:
    void setupUi();
    void loadFileToUI();
    void saveFileFromUI();

    QDockWidget* m_dockWidget = nullptr;
    QTabWidget* m_tabWidget = nullptr;

    // Track/Car
    QLineEdit* m_trackEdit = nullptr;
    QLineEdit* m_trackLayoutEdit = nullptr;
    QLineEdit* m_carEdit = nullptr;

    // Sessions
    QSpinBox* m_qualifyMinutesSpin = nullptr;
    QSpinBox* m_raceLapsSpin = nullptr;
    QSpinBox* m_raceMinutesSpin = nullptr;

    // Grid
    QSpinBox* m_gridSizeSpin = nullptr;
    QComboBox* m_gridSortCombo = nullptr;

    // Rules
    QCheckBox* m_tcCheck = nullptr;
    QCheckBox* m_absCheck = nullptr;
    QCheckBox* m_stabilityCheck = nullptr;
    QCheckBox* m_autoClutchCheck = nullptr;
    QSpinBox* m_mgukLapsSpin = nullptr;

    // Weather
    QLineEdit* m_weatherEdit = nullptr;
    QDoubleSpinBox* m_weatherTempSpin = nullptr;

    QPushButton* m_loadBtn = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_resetBtn = nullptr;
    QLabel* m_statusLabel = nullptr;
    QString m_filePath;
};

} // namespace ks
