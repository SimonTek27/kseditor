#pragma once

#include "../editor/EditorModule.h"
#include "ServerConfigEditor.h"
#include <QDockWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextEdit>

namespace ks {

class ServerConfigEditorModule : public EditorModule {
    Q_OBJECT
public:
    explicit ServerConfigEditorModule(QWidget* parent = nullptr);
    ~ServerConfigEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "Server Config Editor"; }
    QString moduleId() const override { return "serverConfigEditor"; }
    int getModulePriority() const override { return 36; }
    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;

protected:
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onLoadConfig();
    void onSaveConfig();
    void onApplyPreset();
    void onAddEntry();
    void onRemoveEntry();
    void onServerPropChanged();

private:
    void setupUi();
    void loadConfigToUI();
    void saveConfigFromUI();
    void populateEntryTable();

    QDockWidget* m_dockWidget = nullptr;
    QTabWidget* m_tabWidget = nullptr;
    QLabel* m_statusLabel = nullptr;

    // Basic
    QLineEdit* m_nameEdit = nullptr;
    QLineEdit* m_descEdit = nullptr;
    QLineEdit* m_passwordEdit = nullptr;
    QLineEdit* m_adminPwEdit = nullptr;

    // Network
    QLineEdit* m_ipEdit = nullptr;
    QSpinBox* m_portSpin = nullptr;
    QSpinBox* m_httpPortSpin = nullptr;
    QSpinBox* m_maxClientsSpin = nullptr;
    QSpinBox* m_slotCountSpin = nullptr;

    // Session
    QComboBox* m_sessionTypeCombo = nullptr;
    QSpinBox* m_sessionDurationSpin = nullptr;
    QSpinBox* m_lapsCountSpin = nullptr;
    QSpinBox* m_waitTimeSpin = nullptr;

    // Weather
    QComboBox* m_weatherCombo = nullptr;
    QDoubleSpinBox* m_timeOfDaySpin = nullptr;
    QCheckBox* m_useRealWeatherCheck = nullptr;
    QSpinBox* m_timeMultSpin = nullptr;

    // Entry list
    QTableWidget* m_entryTable = nullptr;
    QPushButton* m_addEntryBtn = nullptr;
    QPushButton* m_removeEntryBtn = nullptr;

    // Actions
    QPushButton* m_loadBtn = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QComboBox* m_presetCombo = nullptr;
    QPushButton* m_applyPresetBtn = nullptr;

    QString m_serverPath;
    ServerConfigEditor::ServerConfig m_config;
    QVector<ServerConfigEditor::EntryInfo> m_entries;
    bool m_updating = false;
};

} // namespace ks