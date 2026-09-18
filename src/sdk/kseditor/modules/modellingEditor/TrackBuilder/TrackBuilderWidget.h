#pragma once
// ============================================================================
// TrackBuilderWidget.h
// Qt Widgets UI panel — mirrors TreCorsa's left toolbar layout.
// Tabs: Terrain | Roads | Kerbs | Walls | Surfaces | Props | Lights |
//       Start/Pit | AI Line | Physics | Export
// ============================================================================

#include "TrackBuilderModule.h"
#include <QWidget>
#include <QTabWidget>
#include <QToolButton>
#include <QSlider>
#include <QComboBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QLineEdit>
#include <QCheckBox>
#include <QProgressBar>

namespace ks { namespace track {

class TrackBuilderWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TrackBuilderWidget(QWidget* parent = nullptr);
    void setModule(TrackBuilderModule* module);

signals:
    void requestNewProject();
    void requestOpenProject();
    void requestSaveProject();
    void requestExport();

private slots:
    void onNewProject();
    void onOpenProject();
    void onSaveProject();
    void onExport();
    void onTerrainBrushChanged(int idx);
    void onBrushRadiusChanged(int val);
    void onBrushStrengthChanged(int val);
    void onAddRoad();
    void onRemoveRoad();
    void onAddWall();
    void onAddStartPos();
    void onAddPitPos();
    void onAutoAILine();
    void onSmoothAILine();
    void onImportTerrain();
    void onImportSRTM();
    void onValidate();
    void onModuleProjectChanged();
    void onExportProgress(int pct, const QString& stage);

private:
    QWidget* buildTerrainTab();
    QWidget* buildRoadsTab();
    QWidget* buildKerbsTab();
    QWidget* buildWallsTab();
    QWidget* buildPropsTab();
    QWidget* buildLightsTab();
    QWidget* buildStartPitTab();
    QWidget* buildAILineTab();
    QWidget* buildPhysicsTab();
    QWidget* buildExportTab();

    QToolButton* makeToolBtn(const QString& icon, const QString& tip);
    QGroupBox*   makeGroup(const QString& title, QLayout* layout);

    TrackBuilderModule* m_module = nullptr;

    // Top bar
    QLabel*       m_trackNameLabel = nullptr;
    QLabel*       m_statusLabel    = nullptr;

    // Terrain tab
    QSlider*      m_brushRadiusSlider   = nullptr;
    QSlider*      m_brushStrengthSlider = nullptr;
    QLabel*       m_brushRadiusLabel    = nullptr;
    QLabel*       m_brushStrengthLabel  = nullptr;

    // Roads tab
    QListWidget*  m_roadList       = nullptr;
    QLineEdit*    m_roadNameEdit   = nullptr;
    QComboBox*    m_roadSurfaceCombo = nullptr;
    QCheckBox*    m_roadBridgeCheck  = nullptr;
    QDoubleSpinBox* m_bridgeHeightSpin = nullptr;

    // Export tab
    QProgressBar* m_exportProgress = nullptr;
    QLabel*       m_exportStatusLabel = nullptr;
    QLineEdit*    m_exportDirEdit   = nullptr;
    QCheckBox*    m_exportZipCheck  = nullptr;
    QListWidget*  m_validationList  = nullptr;

    // Main tab widget
    QTabWidget*   m_tabs = nullptr;
};

}} // namespace ks::track
