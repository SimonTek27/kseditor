#pragma once

#include "../../../core/editor/EditorModule.h"
#include <QDockWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QTabWidget>

namespace ks {

struct DriverSkin {
    QString name;
    QString suitPath;
    QString glovesPath;
    QString helmetPath;
    QString helmetBase;
    int helmetVariant = 0;
    QString brand;
};

class DriverEditorModule : public EditorModule {
    Q_OBJECT
public:
    explicit DriverEditorModule(QWidget* parent = nullptr);
    ~DriverEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "Driver Editor"; }
    QString moduleId() const override { return "driverEditor"; }
    int getModulePriority() const override { return 35; }
    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;

protected:
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onDriverSelected(int row);
    void onLoadDriver();
    void onSuitPathChanged(const QString& t);
    void onGlovesPathChanged(const QString& t);
    void onHelmetPathChanged(const QString& t);
    void onHelmetBaseChanged(const QString& t);
    void onHelmetVariantChanged(int v);
    void onBrandChanged(const QString& t);
    void onLoadSkinsDir();
    void onSaveSkinsDir();
    void onRefreshDrivers();

private:
    void setupUi();
    void loadSkinsIniToUI();
    void saveSkinsIniFromUI();

    QDockWidget* m_dockWidget = nullptr;
    QListWidget* m_driverList = nullptr;
    QLineEdit* m_suitPathEdit = nullptr;
    QLineEdit* m_glovesPathEdit = nullptr;
    QLineEdit* m_helmetPathEdit = nullptr;
    QLineEdit* m_helmetBaseEdit = nullptr;
    QSpinBox* m_helmetVariantSpin = nullptr;
    QLineEdit* m_brandEdit = nullptr;
    QPushButton* m_loadBtn = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QLabel* m_statusLabel = nullptr;

    QVector<DriverSkin> m_skins;
    int m_selectedIndex = -1;
    QString m_skinsDir;
};

} // namespace ks
