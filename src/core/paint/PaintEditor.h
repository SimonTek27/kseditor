#pragma once

#include <QWidget>
#include <QImage>
#include <QColorDialog>
#include <QDialogButtonBox>
#include "PaintTypes.h"
#include "PaintDocument.h"

class QToolButton;
class QMenuBar;
class QStackedWidget;
class QLabel;
class QSlider;
class QComboBox;
class QListWidget;
class QToolBox;
class QDockWidget;
class QPlainTextEdit;
class QFontComboBox;
class QSpinBox;
class QCheckBox;
class QGroupBox;
class QFormLayout;

namespace ks {
namespace paint {

class PaintDocument;
class PaintCanvasWidget;

class PaintEditor : public QWidget {
    Q_OBJECT
public:
    explicit PaintEditor(QWidget* parent = nullptr);
    ~PaintEditor() override;

    PaintDocument* document() const { return m_document; }
    PaintCanvasWidget* canvas() const { return m_canvas; }

    void setTexture(const QImage& texture);          // load paint texture as background layer
    QImage currentTexture() const;                   // flattened composite

    void setCarPath(const QString& path) { m_carPath = path; }
    QString carPath() const { return m_carPath; }

    void setCurrentTool(PaintTool tool);
    PaintTool currentTool() const;

    // External color / brush control (from PaintEditorWidget)
    void setPrimaryColor(const QColor& color);
    void setBrushSize(int size);
    void setBrushHardness(int hardness);
    void setBrushStrength(int strength);
    void setBrushFlow(int flow);

    void setTextToolContent(const QString& text, const QFont& font, const QColor& color);
    void applyTextTool();

    void swapColors();
    void resetColors();

    bool hasDocument() const;

    void pushUndo() { if (m_document) m_document->pushUndo(); }
    void undo();
    void redo();
    bool canUndo() const;
    bool canRedo() const;

    void copySelection();
    void pasteSelection();
    void cutSelection();
    void onExportDds();

signals:
    void imageEdited();
    void textureChanged(const QImage& texture);
    void textureSaved(const QString& path);
    void statusMessage(const QString& message);
    void selectionChanged();
    void layerListChanged();
    void toolChanged(PaintTool tool);
    void primaryColorChanged(const QColor& color);
    void historyChanged();

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onToolClicked();
    void onDocumentChanged();
    void onLayersChanged();
    void onCurrentLayerChanged(int index);
    void onSelectionChanged();
    void onCanvasImageEdited();
    void onZoomChanged(float zoom);
    void onStatusMessage(const QString& message);
    void onTextToolClicked(const QPoint& imagePos);

    void onNewLayer();
    void onDuplicateLayer();
    void onRemoveLayer();
    void onRaiseLayer();
    void onLowerLayer();
    void onLayerOpacityChanged(int value);
    void onLayerBlendChanged(int index);
    void onLayerVisibilityClicked(int row);
    void onLayerListCurrentRowChanged(int row);

    void onFilterRequested();
    void onInvert();
    void onGrayscale();
    void onSepia();
    void onBrightnessContrast();
    void onBlur();
    void onSharpen();
    void onNewImage();
    void onOpenImage();
    void onSaveImage();
    void onExportPng();
    void onSelectAll();
    void onSelectNone();
    void onSelectInvert();
    void onZoomIn();
    void onZoomOut();
    void onZoomFit();
    void onZoom100();
    void onPrimaryColorSwatch();
    void onSecondaryColorSwatch();
    void onSwapColors();
    void onResetColors();
    void onAddDecalLayer();

private:
    void setupUI();
    void setupMenuBar();
    QWidget* setupToolbox();
    QWidget* setupToolOptions();
    QWidget* setupLayersDock();
    QWidget* setupColorsPanel();
    QWidget* setupBrushesPanel();
    void refreshLayerList();
    void updateLayerProperties();
    void updateToolOptions();
    void updateActionState();
    void applyBrushSettingsToCanvas();
    QToolButton* addToolButton(QWidget* parent, PaintTool tool);

    PaintDocument* m_document;
    PaintCanvasWidget* m_canvas;

    QString m_carPath;

    // Toolbox
    QHash<PaintTool, QToolButton*> m_toolButtons;
    PaintTool m_tool = PaintTool::Brush;

    // Tool options
    QSlider* m_sizeSlider = nullptr;
    QLabel* m_sizeLabel = nullptr;
    QSlider* m_hardnessSlider = nullptr;
    QLabel* m_hardnessLabel = nullptr;
    QSlider* m_opacitySlider = nullptr;
    QLabel* m_opacityLabel = nullptr;
    QSlider* m_flowSlider = nullptr;
    QLabel* m_flowLabel = nullptr;
    QSlider* m_strengthSlider = nullptr;
    QLabel* m_strengthLabel = nullptr;
    QSlider* m_toleranceSlider = nullptr;
    QLabel* m_toleranceLabel = nullptr;
    QComboBox* m_blendCombo = nullptr;

    // Layers
    QListWidget* m_layerList = nullptr;
    QSlider* m_layerOpacitySlider = nullptr;
    QLabel* m_layerOpacityLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_zoomLabel = nullptr;

    // Colors
    QColor m_primaryColor = QColor(0, 0, 0);
    QColor m_secondaryColor = QColor(255, 255, 255);
    QToolButton* m_fgSwatch = nullptr;
    QToolButton* m_bgSwatch = nullptr;

    // Text tool content
    QString m_textToolContent;
    QFont m_textFont;
    QColor m_textColor = QColor(255, 255, 255);
    bool m_textReady = false;

    QMenuBar* m_menuBar = nullptr;
    bool m_updatingLayerUI = false;
};

} // namespace paint
} // namespace ks