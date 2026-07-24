#pragma once

#include "core/editor/ModuleGuiBase.h"
#include "ShaderGraphWidget.h"
#include <QPainter>
#include <QImage>
#include <QMap>

namespace ks {
namespace material {

class MaterialEditorModule : public ModuleGuiBase {
    Q_OBJECT
public:
    explicit MaterialEditorModule(QWidget* parent = nullptr);
    ~MaterialEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;

    QString moduleName() const override { return "Material Editor"; }
    QString moduleId() const override { return "material"; }
    int getModulePriority() const override { return 70; }

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

protected:
    void buildUI() override;
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onNewMaterial();
    void onMaterialTypeChanged(int index);
    void onShaderChanged(int index);
    void onTextureSelected(const QString& slot);
    void onParameterChanged(const QString& name, const QVariant& value);
    void onSaveMaterial();
    void onLoadMaterial();
    void onExportShader();
    void onPreviewToggle();

private:
    void setupMaterialTabs();
    void setupPbrTab();
    void setupShaderGraphTab();
    void setupTexturePaintTab();
    void setupPresetsTab();
    void setupParameterPanel(const QString& title, const QStringList& params);
    void refreshPresetList();
    void updateMaterialPreview();
    void loadPresets();
    void savePresets();
    void loadMaterial(const QString& path);
    void saveMaterial(const QString& path);
    void applyPreset(const QString& name);
    void saveCurrentAsPreset(const QString& name);
    void deletePreset(const QString& name);
    void exportShader(const QString& path);
    void setParameter(const QString& name, const QVariant& value);

    QTabWidget* m_tabWidget = nullptr;
    
    // PBR Tab
    QWidget* m_pbrTab = nullptr;
    QComboBox* m_materialTypeCombo = nullptr;
    QComboBox* m_shaderCombo = nullptr;
    QTreeWidget* m_parameterTree = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_loadBtn = nullptr;
    QPushButton* m_exportBtn = nullptr;
    QPushButton* m_previewBtn = nullptr;
    QCheckBox* m_livePreviewCheck = nullptr;
    
    // Shader Graph Tab
    QWidget* m_shaderGraphTab = nullptr;
    QSplitter* m_shaderGraphSplitter = nullptr;
    QTreeWidget* m_nodePalette = nullptr;
    ShaderGraphWidget* m_shaderGraphWidget = nullptr;
    QWidget* m_graphProps = nullptr;
    
    // Texture Paint Tab
    QWidget* m_texturePaintTab = nullptr;
    QComboBox* m_brushToolCombo = nullptr;
    QComboBox* m_textureSlotCombo = nullptr;
    QSpinBox* m_brushSizeSpin = nullptr;
    QDoubleSpinBox* m_brushStrengthSpin = nullptr;
    QPushButton* m_paintBtn = nullptr;
    QPushButton* m_eraseBtn = nullptr;
    QPushButton* m_fillBtn = nullptr;
    QPushButton* m_bakeBtn = nullptr;
    
    // Presets Tab
    QWidget* m_presetsTab = nullptr;
    QListWidget* m_presetList = nullptr;
    QPushButton* m_applyPresetBtn = nullptr;
    QPushButton* m_saveAsPresetBtn = nullptr;
    QPushButton* m_deletePresetBtn = nullptr;
    QTextEdit* m_presetDesc = nullptr;

    struct MaterialParameter {
        QString name;
        QString type;  // float, vec2, vec3, vec4, texture, bool
        QVariant value;
        QString texturePath;
    };
    QVector<MaterialParameter> m_parameters;
    
    QString m_currentMaterialPath;
    QString m_currentPreset;
    bool m_livePreview = false;

    // Texture paint canvas
    QMap<QString, QImage> m_textureImages;
    QLabel* m_paintPreview = nullptr;
    QString m_currentTexturePaintSlot;
};

} // namespace material
} // namespace ks