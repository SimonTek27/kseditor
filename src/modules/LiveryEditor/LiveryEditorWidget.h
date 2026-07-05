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
#include "LiveryEditorModule.h"

namespace ks {

class LiveryPainterWidget;

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
    void onBrushSizeChanged(int size);
    void onBrushHardnessChanged(int value);
    void onColorSelected();
    void onLicensePlateGenerate();
    void onSaveSkin();
    void onRefreshSkins();
    void updateLayerUI();
    void clearLayerUI();
    void refreshSkinList();
    void refreshLayerList();

private:
    void setupUI();
    void setupSkinPanel(QWidget* parent, QFormLayout* layout);
    void setupLayerPanel(QWidget* parent, QFormLayout* layout);
    void setupPaintPanel(QWidget* parent, QFormLayout* layout);
    void setupLicensePlatePanel(QWidget* parent, QFormLayout* layout);

    LiveryEditor* m_editor;
    LiveryPainterWidget* m_painterWidget;

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
    QPushButton* m_colorBtn;
    QSlider* m_brushSizeSlider;
    QLabel* m_brushSizeLabel;
    QSlider* m_brushHardnessSlider;
    QLabel* m_brushHardnessLabel;

    QGroupBox* m_plateGroup;
    QLineEdit* m_plateText;
    QComboBox* m_plateCountry;
    QPushButton* m_generatePlateBtn;

    QPushButton* m_saveSkinBtn;
    QPushButton* m_refreshBtn;

    bool m_updatingUI;
};

} // namespace ks
