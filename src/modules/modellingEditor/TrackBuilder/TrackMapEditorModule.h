#pragma once

#include "../../../core/editor/EditorModule.h"
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QGraphicsView>
#include <QGraphicsScene>

namespace ks {

class TrackMapEditorModule : public EditorModule {
    Q_OBJECT
public:
    explicit TrackMapEditorModule(QWidget* parent = nullptr);
    ~TrackMapEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "Track Map Editor"; }
    QString moduleId() const override { return "trackMapEditor"; }
    int getModulePriority() const override { return 32; }
    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;

protected:
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onCenterXChanged(double v);
    void onCenterYChanged(double v);
    void onZoomChanged(double v);
    void onMapSizeChanged(double v);
    void onMapImageChanged(const QString& text);
    void onFlipXChanged(bool c);
    void onFlipYChanged(bool c);
    void onLoadFile();
    void onSaveFile();
    void onResetDefaults();

private:
    void setupUi();
    void loadFileToUI();
    void saveFileFromUI();

    QDockWidget* m_dockWidget = nullptr;

    QDoubleSpinBox* m_centerXSpin = nullptr;
    QDoubleSpinBox* m_centerYSpin = nullptr;
    QDoubleSpinBox* m_zoomSpin = nullptr;
    QDoubleSpinBox* m_mapSizeSpin = nullptr;
    QLineEdit* m_mapImageEdit = nullptr;
    QCheckBox* m_flipXCheck = nullptr;
    QCheckBox* m_flipYCheck = nullptr;

    QPushButton* m_loadBtn = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_resetBtn = nullptr;
    QLabel* m_statusLabel = nullptr;
    QGraphicsView* m_previewView = nullptr;
    QGraphicsScene* m_previewScene = nullptr;

    QString m_filePath;
    float m_centerX = 0, m_centerY = 0;
    float m_zoom = 1.0;
    float m_mapSize = 1.0;
    bool m_flipX = false, m_flipY = false;
};

} // namespace ks
