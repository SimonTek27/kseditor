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
#include "LiveryEditorModule.h"
#include "LiveryViewport.h"

namespace ks {

class LiveryPainterWidget;
class VectorDesignCanvas;

class LiveryEditorWidget : public QWidget {
    Q_OBJECT
public:
    explicit LiveryEditorWidget(QWidget* parent = nullptr);

    void setCarPath(const QString& path);

signals:
    void skinSelected(const QString& skinName);
    void liveryModified();

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
    void onColorSelected();
    void onSecondaryColorSelected();
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
    void setupSkinPanel(QWidget* parent, QFormLayout* layout);
    void setupLayerPanel(QWidget* parent, QFormLayout* layout);
    void setupPaintPanel(QWidget* parent, QFormLayout* layout);
    void setupLicensePlatePanel(QWidget* parent, QFormLayout* layout);
    void setupVectorPanel(QWidget* parent, QFormLayout* layout);

    LiveryEditor* m_editor;
    LiveryPainterWidget* m_painterWidget;
    VectorDesignCanvas* m_vectorCanvas;
    LiveryViewport* m_viewport3D;

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

    bool m_updatingUI;
};

} // namespace ks
