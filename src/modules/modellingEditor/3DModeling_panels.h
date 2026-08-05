#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QSlider>
#include <QLineEdit>
#include <QListWidget>
#include <QButtonGroup>
#include <QList>

#include "3DModeling.h"
#include "TrackBuilder/TrackEditorWidget.h"

namespace ks {
class SceneObject;

// ============================================================================
// Properties Panel
// ============================================================================

class PropertiesPanel : public QWidget {
    Q_OBJECT
public:
    explicit PropertiesPanel(QWidget* parent = nullptr);
    ~PropertiesPanel();

    void setObject(SceneObject* obj);
    void clear();

 signals:
    void propertyChanged(const QString& name, const QVariant& value);
    void transformChanged(const QString& axis, double value);

private:
    void buildTransformGroup();
    void buildInfoGroup();
    void onNameChanged(const QString& text);
    void onVisibleChanged(bool checked);
    void onPosXChanged(double v); void onPosYChanged(double v); void onPosZChanged(double v);
    void onRotXChanged(double v); void onRotYChanged(double v); void onRotZChanged(double v);
    void onScaleXChanged(double v); void onScaleYChanged(double v); void onScaleZChanged(double v);

    SceneObject* m_object = nullptr;
    QTreeWidget* m_tree = nullptr;
    QLineEdit* m_nameEdit = nullptr;
    QCheckBox* m_visibleCheck = nullptr;
    QDoubleSpinBox *m_posX = nullptr, *m_posY = nullptr, *m_posZ = nullptr;
    QDoubleSpinBox *m_rotX = nullptr, *m_rotY = nullptr, *m_rotZ = nullptr;
    QDoubleSpinBox *m_scaleX = nullptr, *m_scaleY = nullptr, *m_scaleZ = nullptr;
    QGroupBox* m_transformGroup = nullptr;
    QGroupBox* m_infoGroup = nullptr;
};

// ============================================================================
// Material Editor Panel
// ============================================================================

class MaterialEditorPanel : public QWidget {
    Q_OBJECT
public:
    explicit MaterialEditorPanel(QWidget* parent = nullptr);
    ~MaterialEditorPanel();

    void setMaterial(void* material);
    void clear();

signals:
    void materialChanged();

private:
    void buildSurfaceGroup();
    void buildPBRGroup();
    void onBaseColorClicked();
    void onEmissiveClicked();

    void* m_material = nullptr;
    QColor m_baseColor;
    QColor m_emissiveColor;
    QPushButton* m_baseColorBtn = nullptr;
    QPushButton* m_emissiveBtn = nullptr;
    QComboBox* m_blendModeCombo = nullptr;
    QCheckBox* m_twoSidedCheck = nullptr;
    QSlider* m_roughnessSlider = nullptr;
    QLabel* m_roughnessLabel = nullptr;
    QSlider* m_metallicSlider = nullptr;
    QLabel* m_metallicLabel = nullptr;
    QSlider* m_opacitySlider = nullptr;
    QLabel* m_opacityLabel = nullptr;
    QSlider* m_emissiveIntensitySlider = nullptr;
    QLabel* m_emissiveIntensityLabel = nullptr;
    QGroupBox* m_surfaceGroup = nullptr;
    QGroupBox* m_pbrGroup = nullptr;
};

// ============================================================================
// Tool Palette Widget
// ============================================================================

class ToolPaletteWidget : public QWidget {
    Q_OBJECT
public:
    explicit ToolPaletteWidget(QWidget* parent = nullptr);
    ~ToolPaletteWidget();

    void addTool(const QString& name, const QString& icon, const QString& tooltip);
    void removeTool(const QString& name);
    void selectTool(const QString& name);
    QString currentTool() const { return m_currentTool; }

 signals:
    void toolSelected(const QString& name);

private:
    void updateToolButtons();

    QString m_currentTool;
    QMap<QString, QWidget*> m_toolWidgets;
    QVBoxLayout* m_layout = nullptr;
    QButtonGroup* m_buttonGroup = nullptr;
};

// ============================================================================
// Object List Widget
// ============================================================================

class ObjectListWidget : public QWidget {
    Q_OBJECT
public:
    explicit ObjectListWidget(QWidget* parent = nullptr);
    ~ObjectListWidget();

    void addObject(const QString& id, const QString& name, const QString& type);
    void removeObject(const QString& id);
    void updateObject(const QString& id, const QString& name);

    void setSelection(const QStringList& ids);
    QStringList selection() const;

 signals:
    void objectSelected(const QString& id);
    void objectDoubleClicked(const QString& id);

private:
    QListWidget* m_list = nullptr;
    QMap<QString, QString> m_objectIds;
};

// ============================================================================
// Layer Panel Widget
// ============================================================================

class LayerPanelWidget : public QWidget {
    Q_OBJECT
public:
    explicit LayerPanelWidget(QWidget* parent = nullptr);
    ~LayerPanelWidget();

    void addLayer(const QString& name, bool visible = true, bool locked = false);
    void removeLayer(const QString& name);
    void setLayerVisible(const QString& name, bool visible);
    void setLayerLocked(const QString& name, bool locked);

    QStringList layers() const { return m_layers.keys(); }

 signals:
    void layerChanged(const QString& name);
    void layerVisibilityChanged(const QString& name, bool visible);
    void layerSelectionChanged(const QString& name);

private:
    void onItemChanged(QTreeWidgetItem* item, int column);
    void onItemClicked(QTreeWidgetItem* item, int column);

    struct LayerInfo {
        bool visible = true;
        bool locked = false;
    };
    QMap<QString, QTreeWidgetItem*> m_layers;
    QMap<QString, LayerInfo> m_layerData;
    QTreeWidget* m_tree = nullptr;
    QString m_selectedLayer;
};

} // namespace ks
