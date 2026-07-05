#pragma once

#include "../../core/editor/EditorModule.h"
#include <QDockWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>

namespace ks {

class SkinIniEditorModule : public EditorModule {
    Q_OBJECT
public:
    explicit SkinIniEditorModule(QWidget* parent = nullptr);
    ~SkinIniEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "Skin INI Editor"; }
    QString moduleId() const override { return "skinIniEditor"; }
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
    void onLoadFile();
    void onSaveFile();

private:
    void setupUi();
    void loadFileToUI();
    void saveFileFromUI();

    QDockWidget* m_dockWidget = nullptr;
    QTabWidget* m_tabWidget = nullptr;
    QLineEdit* m_suitEdit = nullptr;
    QLineEdit* m_glovesEdit = nullptr;
    QLineEdit* m_helmetEdit = nullptr;
    QLineEdit* m_brandEdit = nullptr;
    QLineEdit* m_crewSuitEdit = nullptr;
    QLineEdit* m_crewHelmetEdit = nullptr;
    QPushButton* m_loadBtn = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QLabel* m_statusLabel = nullptr;
    QString m_filePath;
};

} // namespace ks
