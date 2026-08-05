#pragma once

#include <QWidget>
#include <QListWidget>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QSlider>
#include <QCheckBox>
#include <QButtonGroup>
#include <QRadioButton>
#include <QStackedWidget>
#include "PaintEditorModule.h"
#include "PaintViewport.h"
#include "../../core/paint/PaintEditor.h"
#include "../../core/paint/PaintTypes.h"

namespace ks {

class PaintPainterWidget;
class VectorDesignCanvas;

class PaintEditorWidget : public QWidget {
    Q_OBJECT
public:
    explicit PaintEditorWidget(QWidget* parent = nullptr);

    void setCarPath(const QString& path);

signals:
    void skinSelected(const QString& skinName);
    void paintModified();

private slots:
    void onSkinSelected(QListWidgetItem* current);
    void onCreateSkin();
    void onDeleteSkin();
    void onDuplicateSkin();
    void onLayerSelected(QListWidgetItem* current);
    void onAddLayer();
    void onRemoveLayer();
    void onMoveLayerUp();
    void onMoveLayerDown();
    void onLayerOpacityChanged(int value);
    void onLayerVisibilityChanged(int state);
    void onBrushTypeChanged(int index);
    void onBrushSizeChanged(int size);
    void onBrushHardnessChanged(int value);
    void onBrushStrengthChanged(int value);
    void onBrushFlowChanged(int value);
    void onColorSelected(bool = false);
    void onSecondaryColorSelected(bool = false);
    void onLicensePlateGenerate();
    void onSaveSkin();
    void onRefreshSkins();
    void onExportDDS();
    void onImportDecal();
    void onCreateFromTemplate();
    void onUndo();
    void onRedo();
    void onPaletteColorSelected(const QColor& color);
    void updateLayerUI();
    void clearLayerUI();

    // Wrapper slots for ribbon button connections (accept bool from clicked signal)
    void onMergeLayerDown(bool);
    void onFlattenImage(bool);
    void onSwapColors(bool);
    void onResetColors(bool);
    void onSelectNone(bool);
    void onSelectInvert(bool);
    void onZoomIn(bool);
    void onZoomOut(bool);
    void onZoomFit(bool);
    void onZoomTool(bool);
    void refreshSkinList();
    void refreshLayerList();

    // Vector design tool slots
    void onVectorToolChanged(int index);
    void onVectorFillColor();
    void onVectorStrokeColor();
    void onVectorStrokeWidthChanged(int value);
    void onVectorDrawFilledChanged(int state);
    void onEditModeChanged(int mode);
    void onVectorDeleteSelected();
    void onVectorClearAll();
    void onVectorShapesChanged();
    void syncVectorLayerData();

private:
    void setupUI();
    void setupSkinPanel(QWidget* parent, QVBoxLayout* layout);
    void setupLayerPanel(QWidget* parent, QVBoxLayout* layout);
    void setupPaintPanel(QWidget* parent, QVBoxLayout* layout);
    void setupLicensePlatePanel(QWidget* parent, QVBoxLayout* layout);
    void setupVectorPanel(QWidget* parent, QVBoxLayout* layout);
    void setupPalettePanel(QWidget* parent, QVBoxLayout* layout);
    void setupToolBar();

    PaintEditor* m_editor;
    PaintPainterWidget* m_painterWidget;
    VectorDesignCanvas* m_vectorCanvas;
    ks::paint::PaintEditor* m_paintEditor;
    PaintViewport* m_viewport3D;

    QGroupBox* m_skinsGroup;
    QListWidget* m_skinList;
    QPushButton* m_createSkinBtn;
    QPushButton* m_deleteSkinBtn;
    QPushButton* m_duplicateSkinBtn;

    QGroupBox* m_layersGroup;
    QListWidget* m_layerList;
    QPushButton* m_addLayerBtn;
    QPushButton* m_removeLayerBtn;
    QPushButton* m_moveUpBtn;
    QPushButton* m_moveDownBtn;

    QGroupBox* m_layerPropsGroup;
    QLineEdit* m_layerNameEdit;
    QComboBox* m_layerTypeCombo;
    QSlider* m_opacitySlider;
    QLabel* m_opacityLabel;
    QCheckBox* m_visibleCheck;

    QGroupBox* m_paintGroup;
    QComboBox* m_brushTypeCombo;
    QPushButton* m_colorBtn;
    QPushButton* m_secondaryColorBtn;
    QSlider* m_brushSizeSlider;
    QLabel* m_brushSizeLabel;
    QSlider* m_brushHardnessSlider;
    QLabel* m_brushHardnessLabel;
    QSlider* m_brushStrengthSlider;
    QLabel* m_brushStrengthLabel;
    QSlider* m_brushFlowSlider;
    QLabel* m_brushFlowLabel;

    QGroupBox* m_plateGroup;
    QLineEdit* m_plateText;
    QComboBox* m_plateCountry;
    QPushButton* m_generatePlateBtn;

    QPushButton* m_saveSkinBtn;
    QPushButton* m_refreshBtn;
    QPushButton* m_exportDdsBtn;
    QPushButton* m_importDecalBtn;
    QPushButton* m_templateBtn;
    QPushButton* m_undoBtn;
    QPushButton* m_redoBtn;
    QPushButton* m_paletteBtns[10];
    int m_paletteCount = 0;

    // Vector design tool UI
    QButtonGroup* m_editModeGroup;
    QRadioButton* m_rasterModeRadio;
    QRadioButton* m_vectorModeRadio;
    QGroupBox* m_vectorToolsGroup;
    QComboBox* m_vectorToolCombo;
    QPushButton* m_vectorFillColorBtn;
    QPushButton* m_vectorStrokeColorBtn;
    QSlider* m_vectorStrokeWidthSlider;
    QLabel* m_vectorStrokeWidthLabel;
    QCheckBox* m_vectorFilledCheck;
    QPushButton* m_vectorDeleteBtn;
    QPushButton* m_vectorClearBtn;

    QWidget* m_toolBar = nullptr;
    QStackedWidget* m_canvasStack = nullptr;

    bool m_updatingUI;
};

} // namespace ks