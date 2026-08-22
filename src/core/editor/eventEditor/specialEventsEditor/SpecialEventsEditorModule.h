#pragma once

#include "../../editor/EditorModule.h"
#include <QDockWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QTableWidget>

namespace ks {

struct SpecialEvent {
    QString name;
    QString iniPath;
    QString previewPath;
    QMap<QString, QString> settings;
};

class SpecialEventsEditorModule : public EditorModule {
    Q_OBJECT
public:
    explicit SpecialEventsEditorModule(QWidget* parent = nullptr);
    ~SpecialEventsEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "Special Events Editor"; }
    QString moduleId() const override { return "specialEventsEditor"; }
    int getModulePriority() const override { return 41; }
    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;

protected:
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onEventSelected(int row);
    void onAddEvent();
    void onRemoveEvent();
    void onLoadDir();
    void onSaveDir();
    void onEventNameChanged(const QString& t);
    void onAddSetting();
    void onRemoveSetting();
    void onSettingChanged(int row, int column);

private:
    void setupUi();
    void loadDirToUI();

    QDockWidget* m_dockWidget = nullptr;
    QListWidget* m_eventList = nullptr;
    QLineEdit* m_eventNameEdit = nullptr;
    QPushButton* m_addBtn = nullptr;
    QPushButton* m_removeBtn = nullptr;
    QPushButton* m_loadBtn = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QLabel* m_statusLabel = nullptr;
    QTableWidget* m_settingsTable = nullptr;
    QPushButton* m_addSettingBtn = nullptr;
    QPushButton* m_removeSettingBtn = nullptr;
    bool m_updatingSettings = false;
    QVector<SpecialEvent> m_events;
    int m_selectedIndex = -1;
    QString m_dir;
};

} // namespace ks
