#pragma once

#include "../../../core/editor/EditorModule.h"
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QGraphicsView>
#include <QGraphicsScene>

namespace ks {

class TrackLightingEditorModule : public EditorModule {
    Q_OBJECT
public:
    explicit TrackLightingEditorModule(QWidget* parent = nullptr);
    ~TrackLightingEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "Track Lighting Editor"; }
    QString moduleId() const override { return "trackLightingEditor"; }
    int getModulePriority() const override { return 38; }
    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;

protected:
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onSunPitchChanged(double v);
    void onSunHeadingChanged(double v);
    void onLoadFile();
    void onSaveFile();
    void onResetDefaults();

private:
    void setupUi();
    void loadFileToUI();
    void saveFileFromUI();

    QDockWidget* m_dockWidget = nullptr;
    QDoubleSpinBox* m_sunPitchSpin = nullptr;
    QDoubleSpinBox* m_sunHeadingSpin = nullptr;
    QPushButton* m_loadBtn = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_resetBtn = nullptr;
    QLabel* m_statusLabel = nullptr;
    QGraphicsView* m_previewView = nullptr;
    QGraphicsScene* m_previewScene = nullptr;
    QString m_filePath;
    float m_sunPitch = 45.0f, m_sunHeading = 180.0f;
};

} // namespace ks
