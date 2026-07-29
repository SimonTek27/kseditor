#pragma once

// ============================================================================
// PhysicsEditor — Assetto Corsa car editor (INI files + ACD)
// Inspired by AssettoTools: side menu, file tree, syntax-highlighted editor
// ============================================================================

#include <QWidget>
#include <QString>
#include <QStackedWidget>
#include <QLabel>

#include "../../core/editor/EditorModule.h"
#include "../../plugins/simulators/kunos/assettocorsa/ksAssettoCorsaConfig.h"
#include "../../plugins/simulators/kunos/assettocorsa/ksAssettoCorsaSetup.h"
#include "../../plugins/simulators/kunos/assettocorsa/ksAssettoCorsaIni.h"
#include "../../core/sys/UndoStack.h"
#include "widgets/IniEditorWidget.h"
#include "widgets/FileTreeWidget.h"
#include "widgets/CarBrowserWidget.h"
#include "widgets/TyresTableWidget.h"
#include "widgets/AcdManagerWidget.h"
#include "widgets/LutCurveWidget.h"
#include "widgets/EngineCurveWidget.h"
#include "widgets/TelemetryWidget.h"
#include "widgets/CarSetupCompareWidget.h"
#include "widgets/AcdBrowserWidget.h"
#include "widgets/SuspGeometryWidget.h"
#include "widgets/FfbPreviewWidget.h"
#include "widgets/CarValidatorWidget.h"
#include "widgets/TyreTempModelWidget.h"
#include "../../plugins/simulators/kunos/assettocorsa/physics/TireCurveEditor.h"

namespace ks {
using KsSetupData     = ::ks::kunos::KsSetupData;
using KsSetupManager  = ::ks::kunos::KsSetupManager;
using KsGameSettings  = ::ks::kunos::KsGameSettings;
using KsIniDocument   = ::ks::plugins::kunos::ks::KsIniDocument;
using KsIniSection    = ::ks::plugins::kunos::ks::KsIniSection;

// ─────────────────────────────────────────────────────────────────────────────
// PhysicsEditorModule — main module, side menu + stacked content
// ─────────────────────────────────────────────────────────────────────────────
class PhysicsEditorModule : public EditorModule {
    Q_OBJECT
public:
    explicit PhysicsEditorModule(QWidget* parent = nullptr);

    QString moduleName() const override { return "Physics Editor"; }
    QString moduleId()   const override { return "ks.physics_editor"; }
    bool    initialize() override;
    void    shutdown() override;

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;

public slots:
    void onCarSelected(const QString& carFolder);
    void onFileSelected(const QString& path);
    void onOpenCarsFolder();
    void onSaveCurrentFile();
    void onExportCar();
    void onImportCar();
    void onShowLutEditor();
    void onShowEngineCurve();
    void onShowTelemetry();
    void onShowSetupCompare();
    void onShowAcdBrowser();
    void onShowTireCurveEditor();
    void onShowSuspGeometry();
    void onShowFfbPreview();
    void onShowCarValidator();
    void onShowTyreTempModel();
    double estimateLapTime(double trackLengthM, double avgCornerSpeedKmh,
                           double avgStraightSpeedKmh, double accelMs2,
                           double brakeDecelMs2, int cornerCount);

private:
    void buildUI();
    void updateWindowTitle();
    bool loadCarIniFiles(const QString& carFolder);
    void populateFileTree(const QString& carFolder);
    bool saveCurrentIni();

    QString                        m_carsPath;
    QString                        m_currentCar;
    QString                        m_currentFile;

    CarBrowserWidget*              m_carBrowser;
    FileTreeWidget*                m_fileTree;
    QStackedWidget*               m_contentStack;
    IniEditorWidget*              m_iniEditor;
    TyresTableWidget*             m_tyresEditor;
    AcdManagerWidget*             m_acdManager;
    LutCurveWidget*               m_lutEditor;
    EngineCurveWidget*            m_engineEditor;
    TelemetryWidget*              m_telemetryWidget;
    CarSetupCompareWidget*        m_setupCompare;
    AcdBrowserWidget*             m_acdBrowser;
    SuspGeometryWidget*           m_suspGeometry;
    FfbPreviewWidget*             m_ffbPreview;
    CarValidatorWidget*           m_validator;
    TyreTempModelWidget*          m_tyreTempModel;
    TireCurveEditor*              m_tireCurveEditor;
    QLabel*                       m_statusLabel;
    QLabel*                       m_carLabel;
};

} // namespace ks