#pragma once

#include "../../../core/editor/EditorModule.h"
#include <QDockWidget>
#include <QTableWidget>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>

namespace ks {

struct DRSZone {
    int id = 0;
    float startLine[3] = {0,0,0};
    float endLine[3] = {0,0,0};
    float detectionPoint = 0.0f;
    float activationPoint = 0.0f;
};

class DRSZoneEditorModule : public EditorModule {
    Q_OBJECT
public:
    explicit DRSZoneEditorModule(QWidget* parent = nullptr);
    ~DRSZoneEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "DRS Zone Editor"; }
    QString moduleId() const override { return "drsZoneEditor"; }
    int getModulePriority() const override { return 39; }
    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;

protected:
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onZoneSelected(int row);
    void onAddZone();
    void onRemoveZone();
    void onStartLineXChanged(double v);
    void onStartLineYChanged(double v);
    void onStartLineZChanged(double v);
    void onEndLineXChanged(double v);
    void onEndLineYChanged(double v);
    void onEndLineZChanged(double v);
    void onDetectionPointChanged(double v);
    void onActivationPointChanged(double v);
    void onLoadFile();
    void onSaveFile();
    void onResetDefaults();

private:
    void setupUi();
    void loadFileToUI();
    void saveFileFromUI();
    void updateTable();
    void selectZone(int idx);

    QDockWidget* m_dockWidget = nullptr;
    QTableWidget* m_zoneTable = nullptr;
    QPushButton* m_addBtn = nullptr;
    QPushButton* m_removeBtn = nullptr;
    QDoubleSpinBox* m_startXSpin = nullptr;
    QDoubleSpinBox* m_startYSpin = nullptr;
    QDoubleSpinBox* m_startZSpin = nullptr;
    QDoubleSpinBox* m_endXSpin = nullptr;
    QDoubleSpinBox* m_endYSpin = nullptr;
    QDoubleSpinBox* m_endZSpin = nullptr;
    QDoubleSpinBox* m_detectionSpin = nullptr;
    QDoubleSpinBox* m_activationSpin = nullptr;
    QPushButton* m_loadBtn = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_resetBtn = nullptr;
    QLabel* m_statusLabel = nullptr;
    QVector<DRSZone> m_zones;
    int m_selectedIndex = -1;
    QString m_filePath;
};

} // namespace ks
