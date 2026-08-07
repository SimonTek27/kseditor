#include "PaintEditor.h"
#include "PaintDocument.h"
#include "PaintCanvasWidget.h"
#include "PaintPainter.h"
#include <QToolButton>
#include <QMenuBar>
#include <QMenu>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QSlider>
#include <QLabel>
#include <QComboBox>
#include <QListWidget>
#include <QToolBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QColorDialog>
#include <QInputDialog>
#include <QFontDialog>
#include <QPlainTextEdit>
#include <QFontComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QImageReader>
#include <QImageWriter>
#include <QPainter>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include "../../modules/PaintEditor/PaintSystem.h"

namespace ks {
namespace paint {

// ────────────────────────────────────────────────────────────────────────────
// PaintEditor
// ────────────────────────────────────────────────────────────────────────────

PaintEditor::PaintEditor(QWidget* parent)
    : QWidget(parent)
    , m_document(new PaintDocument(this))
{
    setupUI();

    connect(m_document, &PaintDocument::documentChanged, this, &PaintEditor::onDocumentChanged);
    connect(m_document, &PaintDocument::layersChanged, this, &PaintEditor::onLayersChanged);
    connect(m_document, &PaintDocument::currentLayerChanged, this, &PaintEditor::onCurrentLayerChanged);
    connect(m_document, &PaintDocument::selectionChanged, this, &PaintEditor::onSelectionChanged);
    connect(m_document, &PaintDocument::historyChanged, this, &PaintEditor::updateActionState);
    // Forward historyChanged signal
    connect(m_document, &PaintDocument::historyChanged, this, [this]() { emit historyChanged(); });
}

PaintEditor::~PaintEditor()
{
}

bool PaintEditor::hasDocument() const
{
    return m_document && m_document->hasDocument();
}

void PaintEditor::setTexture(const QImage& texture)
{
    if (texture.isNull()) return;
    m_document->loadImage(texture, QStringLiteral("Background"));
    m_canvas->setDocument(m_document);
    m_canvas->zoomToFit();
    refreshLayerList();
    updateActionState();
    emit textureChanged(texture);
}

QImage PaintEditor::currentTexture() const
{
    if (!m_document) return QImage();
    return m_document->composite();
}

void PaintEditor::setCurrentTool(PaintTool tool)
{
    m_tool = tool;
    m_canvas->setTool(tool);
    if (m_toolButtons.contains(tool)) {
        m_toolButtons.value(tool)->setChecked(true);
    }
    updateToolOptions();
    emit toolChanged(tool);
}

PaintTool PaintEditor::currentTool() const
{
    return m_tool;
}

void PaintEditor::setPrimaryColor(const QColor& color)
{
    m_primaryColor = color;
    m_canvas->setPrimaryColor(color);
    if (m_fgSwatch) {
        m_fgSwatch->setStyleSheet(QString("QToolButton { background: %1; }").arg(color.name()));
    }
    emit primaryColorChanged(color);
}

void PaintEditor::swapColors()
{
    QColor temp = m_primaryColor;
    setPrimaryColor(m_secondaryColor);
    m_secondaryColor = temp;
    m_canvas->setSecondaryColor(m_secondaryColor);
    if (m_bgSwatch) {
        m_bgSwatch->setStyleSheet(QString("QToolButton { background: %1; }").arg(m_secondaryColor.name()));
    }
    emit primaryColorChanged(m_primaryColor);
    onStatusMessage(tr("Colors swapped"));
}

void PaintEditor::resetColors()
{
    setPrimaryColor(QColor(0, 0, 0));
    m_secondaryColor = QColor(255, 255, 255);
    m_canvas->setSecondaryColor(m_secondaryColor);
    if (m_fgSwatch) m_fgSwatch->setStyleSheet("QToolButton { background: #000000; }");
    if (m_bgSwatch) m_bgSwatch->setStyleSheet("QToolButton { background: #FFFFFF; }");
    onStatusMessage(tr("Colors reset to default"));
}

void PaintEditor::setBrushSize(int size)
{
    if (m_sizeSlider) m_sizeSlider->setValue(size);
    if (m_sizeLabel) m_sizeLabel->setText(QString::number(size));
    applyBrushSettingsToCanvas();
}

void PaintEditor::setBrushHardness(int hardness)
{
    if (m_hardnessSlider) m_hardnessSlider->setValue(hardness);
    if (m_hardnessLabel) m_hardnessLabel->setText(QString("%1%").arg(hardness));
    applyBrushSettingsToCanvas();
}

void PaintEditor::setBrushStrength(int strength)
{
    if (m_strengthSlider) m_strengthSlider->setValue(strength);
    if (m_strengthLabel) m_strengthLabel->setText(QString("%1%").arg(strength));
    applyBrushSettingsToCanvas();
}

void PaintEditor::setBrushFlow(int flow)
{
    if (m_flowSlider) m_flowSlider->setValue(flow);
    if (m_flowLabel) m_flowLabel->setText(QString("%1%").arg(flow));
    applyBrushSettingsToCanvas();
}

void PaintEditor::setTextToolContent(const QString& text, const QFont& font, const QColor& color)
{
    m_textToolContent = text;
    m_textFont = font;
    m_textColor = color;
    m_textReady = true;
}

void PaintEditor::applyTextTool()
{
    if (!m_textReady || !hasDocument() || !m_document->currentLayer()) return;
    if (m_textToolContent.isEmpty()) return;

    QImage layer = m_document->currentLayerImage();
    if (layer.isNull()) return;

    m_document->pushUndo();
    QPoint anchor = m_canvas->mapFromGlobal(QCursor::pos());
    QPoint imagePos = m_canvas->imagePosFromView(anchor);
    imagePos = QPoint(qBound(0, imagePos.x(), layer.width() - 1),
                       qBound(0, imagePos.y(), layer.height() - 1));

    QPainter p(&layer);
    p.setPen(m_textColor);
    p.setFont(m_textFont);
    p.drawText(imagePos, m_textToolContent);
    p.end();

    m_document->setCurrentLayerImage(layer);
    m_textReady = false;
    emit imageEdited();
    onStatusMessage(tr("Text drawn at %1, %2").arg(imagePos.x()).arg(imagePos.y()));
}

void PaintEditor::undo()
{
    if (m_document) m_document->undo();
}

void PaintEditor::redo()
{
    if (m_document) m_document->redo();
}

bool PaintEditor::canUndo() const
{
    return m_document && m_document->canUndo();
}

bool PaintEditor::canRedo() const
{
    return m_document && m_document->canRedo();
}

void PaintEditor::copySelection()
{
    if (!hasDocument()) return;
    QImage layer = m_document->currentLayerImage();
    if (layer.isNull()) return;
    QImage masked = m_document->applySelection(layer);
    QApplication::clipboard()->setImage(masked);
}

void PaintEditor::pasteSelection()
{
    if (!hasDocument()) return;
    QImage clip = QApplication::clipboard()->image();
    if (clip.isNull()) return;
    m_document->addLayerImage(tr("Pasted Layer"), clip);
    refreshLayerList();
    emit imageEdited();
}

void PaintEditor::cutSelection()
{
    if (!hasDocument()) return;
    copySelection();
    QImage layer = m_document->currentLayerImage();
    if (layer.isNull()) return;
    m_document->pushUndo();

    QImage mask = m_document->selectionMask();
    if (!mask.isNull()) {
        for (int y = 0; y < layer.height(); ++y) {
            QRgb* line = reinterpret_cast<QRgb*>(layer.scanLine(y));
            const QRgb* m = reinterpret_cast<const QRgb*>(mask.constScanLine(y));
            for (int x = 0; x < layer.width(); ++x) {
                float a = qAlpha(m[x]) / 255.0f;
                if (a > 0) line[x] = qRgba(qRed(line[x]), qGreen(line[x]), qBlue(line[x]),
                                            int(qAlpha(line[x]) * (1.0f - a)));
            }
        }
    }
    m_document->setCurrentLayerImage(layer);
    emit imageEdited();
}

// ── UI construction ──────────────────────────────────────────────────────────

void PaintEditor::setupUI()
{
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    setupMenuBar();
    root->addWidget(m_menuBar);

    // Main splitter: toolbox | canvas | docks
    QSplitter* mainSplit = new QSplitter(Qt::Horizontal, this);
    mainSplit->setChildrenCollapsible(false);
    mainSplit->setHandleWidth(1);

    mainSplit->addWidget(setupToolbox());

    QWidget* canvasArea = new QWidget(mainSplit);
    canvasArea->setObjectName("paintCanvasArea");
    QVBoxLayout* canvasLayout = new QVBoxLayout(canvasArea);
    canvasLayout->setContentsMargins(0, 0, 0, 0);
    canvasLayout->setSpacing(0);

    canvasLayout->addWidget(setupToolOptions());

    m_canvas = new PaintCanvasWidget(canvasArea);
    m_canvas->setObjectName("paintCanvas");
    canvasLayout->addWidget(m_canvas, 1);

    // Status bar
    QWidget* statusBar = new QWidget(canvasArea);
    statusBar->setObjectName("paintStatusBar");
    statusBar->setFixedHeight(26);
    QHBoxLayout* statusLayout = new QHBoxLayout(statusBar);
    statusLayout->setContentsMargins(8, 2, 8, 2);
    m_statusLabel = new QLabel(tr("Ready"), statusBar);
    statusLayout->addWidget(m_statusLabel, 1);
    m_zoomLabel = new QLabel("100%", statusBar);
    m_zoomLabel->setObjectName("zoomLabel");
    statusLayout->addWidget(m_zoomLabel);
    canvasLayout->addWidget(statusBar);

    mainSplit->addWidget(canvasArea);

    QWidget* rightDocks = new QWidget(mainSplit);
    rightDocks->setObjectName("paintDocks");
    QVBoxLayout* dockLayout = new QVBoxLayout(rightDocks);
    dockLayout->setContentsMargins(0, 0, 0, 0);
    dockLayout->setSpacing(0);

    QToolBox* toolBox = new QToolBox(rightDocks);
    toolBox->addItem(setupLayersDock(), tr("Layers"));
    toolBox->addItem(setupBrushesPanel(), tr("Brushes"));
    toolBox->addItem(setupColorsPanel(), tr("Colors"));
    dockLayout->addWidget(toolBox, 1);

    mainSplit->addWidget(rightDocks);

    mainSplit->setStretchFactor(0, 0);
    mainSplit->setStretchFactor(1, 1);
    mainSplit->setStretchFactor(2, 0);
    root->addWidget(mainSplit, 1);

    // Default tool: brush
    setCurrentTool(PaintTool::Brush);

    // Register keyboard shortcuts for tools
    auto registerToolShortcut = [this](PaintTool tool) {
        QString shortcut = paintToolShortcut(tool);
        if (shortcut.isEmpty()) return;
        QAction* act = new QAction(this);
        act->setShortcut(QKeySequence(shortcut));
        act->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(act, &QAction::triggered, this, [this, tool]() { setCurrentTool(tool); });
        addAction(act);
    };

    // Transform tools
    registerToolShortcut(PaintTool::Move);
    registerToolShortcut(PaintTool::Zoom);
    registerToolShortcut(PaintTool::Pan);
    // Selection tools
    registerToolShortcut(PaintTool::RectSelect);
    registerToolShortcut(PaintTool::EllipseSelect);
    registerToolShortcut(PaintTool::FreeSelect);
    registerToolShortcut(PaintTool::FuzzySelect);
    registerToolShortcut(PaintTool::ColorPicker);
    // Paint tools
    registerToolShortcut(PaintTool::Brush);
    registerToolShortcut(PaintTool::Pencil);
    registerToolShortcut(PaintTool::Eraser);
    registerToolShortcut(PaintTool::Airbrush);
    registerToolShortcut(PaintTool::Smudge);
    registerToolShortcut(PaintTool::Blur);
    registerToolShortcut(PaintTool::Sharpen);
    registerToolShortcut(PaintTool::Dodge);
    registerToolShortcut(PaintTool::Burn);
    registerToolShortcut(PaintTool::Clone);
    registerToolShortcut(PaintTool::Healing);
    // Fill/Gradient/Text
    registerToolShortcut(PaintTool::BucketFill);
    registerToolShortcut(PaintTool::Gradient);
    registerToolShortcut(PaintTool::Text);

    connect(m_canvas, &PaintCanvasWidget::imageEdited, this, &PaintEditor::onCanvasImageEdited);
    connect(m_canvas, &PaintCanvasWidget::statusMessage, this, &PaintEditor::onStatusMessage);
    connect(m_canvas, &PaintCanvasWidget::zoomChanged, this, &PaintEditor::onZoomChanged);
    connect(m_canvas, &PaintCanvasWidget::colorPicked, this, &PaintEditor::setPrimaryColor);
    connect(m_canvas, &PaintCanvasWidget::selectionChanged, this, &PaintEditor::onSelectionChanged);
    connect(m_canvas, &PaintCanvasWidget::textToolClicked, this, &PaintEditor::onTextToolClicked);
}

void PaintEditor::setupMenuBar()
{
    m_menuBar = new QMenuBar(this);

    QMenu* fileMenu = m_menuBar->addMenu(tr("&File"));
    fileMenu->addAction(tr("&New Image..."), this, &PaintEditor::onNewImage);
    fileMenu->addAction(tr("&Open..."), this, &PaintEditor::onOpenImage);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("&Save"), this, &PaintEditor::onSaveImage);
    fileMenu->addAction(tr("Export as &PNG..."), this, &PaintEditor::onExportPng);
    fileMenu->addAction(tr("Export as &DDS..."), this, &PaintEditor::onExportDds);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("&Close"), this, &QWidget::close);

    QMenu* editMenu = m_menuBar->addMenu(tr("&Edit"));
    editMenu->addAction(tr("&Undo"), this, &PaintEditor::undo, QKeySequence::Undo);
    editMenu->addAction(tr("&Redo"), this, &PaintEditor::redo, QKeySequence::Redo);
    editMenu->addSeparator();
    editMenu->addAction(tr("&Copy"), this, &PaintEditor::copySelection, QKeySequence::Copy);
    editMenu->addAction(tr("C&ut"), this, &PaintEditor::cutSelection, QKeySequence::Cut);
    editMenu->addAction(tr("&Paste"), this, &PaintEditor::pasteSelection, QKeySequence::Paste);

    QMenu* selectMenu = m_menuBar->addMenu(tr("&Select"));
    selectMenu->addAction(tr("&All"), this, &PaintEditor::onSelectAll, QKeySequence::SelectAll);
    selectMenu->addAction(tr("&None"), this, &PaintEditor::onSelectNone);
    selectMenu->addAction(tr("&Invert"), this, &PaintEditor::onSelectInvert);

    QMenu* layerMenu = m_menuBar->addMenu(tr("&Layer"));
    layerMenu->addAction(tr("&New Layer"), this, &PaintEditor::onNewLayer, QKeySequence::New);
    layerMenu->addAction(tr("&Duplicate"), this, &PaintEditor::onDuplicateLayer);
    layerMenu->addAction(tr("&Delete"), this, &PaintEditor::onRemoveLayer, QKeySequence::Delete);
    layerMenu->addSeparator();
    layerMenu->addAction(tr("&Raise"), this, &PaintEditor::onRaiseLayer);
    layerMenu->addAction(tr("&Lower"), this, &PaintEditor::onLowerLayer);
    layerMenu->addSeparator();
    layerMenu->addAction(tr("Add &Decal Layer..."), this, &PaintEditor::onAddDecalLayer);

    QMenu* filterMenu = m_menuBar->addMenu(tr("&Filters"));
    filterMenu->addAction(tr("&Blur..."), this, &PaintEditor::onBlur);
    filterMenu->addAction(tr("&Sharpen..."), this, &PaintEditor::onSharpen);
    filterMenu->addSeparator();
    filterMenu->addAction(tr("&Brightness/Contrast..."), this, &PaintEditor::onBrightnessContrast);
    filterMenu->addAction(tr("&Invert"), this, &PaintEditor::onInvert);
    filterMenu->addAction(tr("&Desaturate"), this, &PaintEditor::onGrayscale);
    filterMenu->addAction(tr("&Sepia"), this, &PaintEditor::onSepia);

    QMenu* viewMenu = m_menuBar->addMenu(tr("&View"));
    viewMenu->addAction(tr("Zoom &In"), this, &PaintEditor::onZoomIn, QKeySequence::ZoomIn);
    viewMenu->addAction(tr("Zoom &Out"), this, &PaintEditor::onZoomOut, QKeySequence::ZoomOut);
    viewMenu->addAction(tr("&Fit to Window"), this, &PaintEditor::onZoomFit);
    viewMenu->addAction(tr("Zoom &100%"), this, &PaintEditor::onZoom100);
}

QWidget* PaintEditor::setupToolbox()
{
    QWidget* box = new QWidget(this);
    box->setObjectName("paintToolbox");
    box->setFixedWidth(44);

    QVBoxLayout* layout = new QVBoxLayout(box);
    layout->setContentsMargins(4, 6, 4, 6);
    layout->setSpacing(3);

    layout->addWidget(addToolButton(box, PaintTool::Move));
    layout->addWidget(addToolButton(box, PaintTool::Pan));
    layout->addWidget(addToolButton(box, PaintTool::Zoom));
    layout->addSpacing(8);
    layout->addWidget(addToolButton(box, PaintTool::RectSelect));
    layout->addWidget(addToolButton(box, PaintTool::EllipseSelect));
    layout->addWidget(addToolButton(box, PaintTool::FreeSelect));
    layout->addWidget(addToolButton(box, PaintTool::FuzzySelect));
    layout->addSpacing(8);
    layout->addWidget(addToolButton(box, PaintTool::ColorPicker));
    layout->addWidget(addToolButton(box, PaintTool::Text));
    layout->addSpacing(8);
    layout->addWidget(addToolButton(box, PaintTool::Brush));
    layout->addWidget(addToolButton(box, PaintTool::Pencil));
    layout->addWidget(addToolButton(box, PaintTool::Eraser));
    layout->addWidget(addToolButton(box, PaintTool::Airbrush));
    layout->addSpacing(8);
    layout->addWidget(addToolButton(box, PaintTool::Smudge));
    layout->addWidget(addToolButton(box, PaintTool::Blur));
    layout->addWidget(addToolButton(box, PaintTool::Sharpen));
    layout->addWidget(addToolButton(box, PaintTool::Dodge));
    layout->addWidget(addToolButton(box, PaintTool::Burn));
    layout->addSpacing(8);
    layout->addWidget(addToolButton(box, PaintTool::Clone));
    layout->addWidget(addToolButton(box, PaintTool::Healing));
    layout->addSpacing(8);
    layout->addWidget(addToolButton(box, PaintTool::BucketFill));
    layout->addWidget(addToolButton(box, PaintTool::Gradient));
    layout->addStretch();

    return box;
}

QToolButton* PaintEditor::addToolButton(QWidget* parent, PaintTool tool)
{
    QToolButton* btn = new QToolButton(parent);
    btn->setCheckable(true);
    btn->setToolTip(QString("%1 (%2)").arg(paintToolDisplayName(tool)).arg(paintToolShortcut(tool)));
    btn->setText(paintToolShortcut(tool));
    btn->setFixedSize(36, 30);
    btn->setProperty("paintTool", int(tool));
    btn->setObjectName("paintToolButton");
    connect(btn, &QToolButton::clicked, this, &PaintEditor::onToolClicked);
    m_toolButtons.insert(tool, btn);
    return btn;
}

QWidget* PaintEditor::setupToolOptions()
{
    QWidget* opts = new QWidget(this);
    opts->setObjectName("paintToolOptions");
    opts->setFixedHeight(40);

    QHBoxLayout* layout = new QHBoxLayout(opts);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(6);

    layout->addWidget(new QLabel(tr("Size:"), opts));
    m_sizeSlider = new QSlider(Qt::Horizontal, opts);
    m_sizeSlider->setRange(1, 200);
    m_sizeSlider->setValue(20);
    m_sizeSlider->setFixedWidth(110);
    layout->addWidget(m_sizeSlider);
    m_sizeLabel = new QLabel("20", opts);
    m_sizeLabel->setFixedWidth(26);
    layout->addWidget(m_sizeLabel);

    layout->addWidget(new QLabel(tr("Hard:"), opts));
    m_hardnessSlider = new QSlider(Qt::Horizontal, opts);
    m_hardnessSlider->setRange(0, 100);
    m_hardnessSlider->setValue(50);
    m_hardnessSlider->setFixedWidth(70);
    layout->addWidget(m_hardnessSlider);
    m_hardnessLabel = new QLabel("50%", opts);
    m_hardnessLabel->setFixedWidth(34);
    layout->addWidget(m_hardnessLabel);

    layout->addWidget(new QLabel(tr("Opacity:"), opts));
    m_opacitySlider = new QSlider(Qt::Horizontal, opts);
    m_opacitySlider->setRange(1, 100);
    m_opacitySlider->setValue(100);
    m_opacitySlider->setFixedWidth(70);
    layout->addWidget(m_opacitySlider);
    m_opacityLabel = new QLabel("100%", opts);
    m_opacityLabel->setFixedWidth(34);
    layout->addWidget(m_opacityLabel);

    layout->addWidget(new QLabel(tr("Flow:"), opts));
    m_flowSlider = new QSlider(Qt::Horizontal, opts);
    m_flowSlider->setRange(1, 100);
    m_flowSlider->setValue(100);
    m_flowSlider->setFixedWidth(70);
    layout->addWidget(m_flowSlider);
    m_flowLabel = new QLabel("100%", opts);
    m_flowLabel->setFixedWidth(34);
    layout->addWidget(m_flowLabel);

    layout->addWidget(new QLabel(tr("Strength:"), opts));
    m_strengthSlider = new QSlider(Qt::Horizontal, opts);
    m_strengthSlider->setRange(1, 100);
    m_strengthSlider->setValue(100);
    m_strengthSlider->setFixedWidth(70);
    layout->addWidget(m_strengthSlider);
    m_strengthLabel = new QLabel("100%", opts);
    m_strengthLabel->setFixedWidth(34);
    layout->addWidget(m_strengthLabel);

    layout->addWidget(new QLabel(tr("Tolerance:"), opts));
    m_toleranceSlider = new QSlider(Qt::Horizontal, opts);
    m_toleranceSlider->setRange(0, 100);
    m_toleranceSlider->setValue(20);
    m_toleranceSlider->setFixedWidth(70);
    layout->addWidget(m_toleranceSlider);
    m_toleranceLabel = new QLabel("20%", opts);
    m_toleranceLabel->setFixedWidth(34);
    layout->addWidget(m_toleranceLabel);

    layout->addWidget(new QLabel(tr("Blend:"), opts));
    m_blendCombo = new QComboBox(opts);
    m_blendCombo->addItems({ tr("Normal"), tr("Multiply"), tr("Screen"), tr("Overlay"),
                             tr("Darken"), tr("Lighten"), tr("Dodge"), tr("Burn"),
                             tr("Hard Light"), tr("Soft Light"), tr("Difference"), tr("Exclusion") });
    m_blendCombo->setFixedWidth(100);
    layout->addWidget(m_blendCombo);

    layout->addStretch();

    // Brush parameter changes
    connect(m_sizeSlider, &QSlider::valueChanged, this, [this](int v) {
        if (m_sizeLabel) m_sizeLabel->setText(QString::number(v));
        applyBrushSettingsToCanvas();
    });
    connect(m_hardnessSlider, &QSlider::valueChanged, this, [this](int v) {
        if (m_hardnessLabel) m_hardnessLabel->setText(QString("%1%").arg(v));
        applyBrushSettingsToCanvas();
    });
    connect(m_opacitySlider, &QSlider::valueChanged, this, [this](int v) {
        if (m_opacityLabel) m_opacityLabel->setText(QString("%1%").arg(v));
        applyBrushSettingsToCanvas();
    });
    connect(m_flowSlider, &QSlider::valueChanged, this, [this](int v) {
        if (m_flowLabel) m_flowLabel->setText(QString("%1%").arg(v));
        applyBrushSettingsToCanvas();
    });
    connect(m_strengthSlider, &QSlider::valueChanged, this, [this](int v) {
        if (m_strengthLabel) m_strengthLabel->setText(QString("%1%").arg(v));
        applyBrushSettingsToCanvas();
    });
    connect(m_toleranceSlider, &QSlider::valueChanged, this, [this](int v) {
        if (m_toleranceLabel) m_toleranceLabel->setText(QString("%1%").arg(v));
        if (m_canvas) m_canvas->setFuzzyTolerance(v / 100.0f);
    });
    connect(m_blendCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (m_document && index >= 0)
            m_document->setLayerBlendMode(m_document->currentLayerIndex(), static_cast<PaintBlendMode>(index));
    });

    return opts;
}

QWidget* PaintEditor::setupLayersDock()
{
    QWidget* dock = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(dock);
    layout->setContentsMargins(4, 6, 4, 6);
    layout->setSpacing(4);

    m_layerList = new QListWidget(dock);
    m_layerList->setObjectName("paintLayerList");
    m_layerList->setMinimumHeight(180);
    layout->addWidget(m_layerList, 1);

    QWidget* btnRow = new QWidget(dock);
    QHBoxLayout* btnLayout = new QHBoxLayout(btnRow);
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(4);

    QToolButton* addBtn = new QToolButton(btnRow);
    addBtn->setText("+");
    addBtn->setToolTip(tr("New Layer"));
    connect(addBtn, &QToolButton::clicked, this, &PaintEditor::onNewLayer);
    btnLayout->addWidget(addBtn);

    QToolButton* dupBtn = new QToolButton(btnRow);
    dupBtn->setText("D");
    dupBtn->setToolTip(tr("Duplicate Layer"));
    connect(dupBtn, &QToolButton::clicked, this, &PaintEditor::onDuplicateLayer);
    btnLayout->addWidget(dupBtn);

    QToolButton* delBtn = new QToolButton(btnRow);
    delBtn->setText("-");
    delBtn->setToolTip(tr("Delete Layer"));
    connect(delBtn, &QToolButton::clicked, this, &PaintEditor::onRemoveLayer);
    btnLayout->addWidget(delBtn);

    QToolButton* upBtn = new QToolButton(btnRow);
    upBtn->setText(tr("^"));
    upBtn->setToolTip(tr("Raise Layer"));
    connect(upBtn, &QToolButton::clicked, this, &PaintEditor::onRaiseLayer);
    btnLayout->addWidget(upBtn);

    QToolButton* downBtn = new QToolButton(btnRow);
    downBtn->setText(tr("v"));
    downBtn->setToolTip(tr("Lower Layer"));
    connect(downBtn, &QToolButton::clicked, this, &PaintEditor::onLowerLayer);
    btnLayout->addWidget(downBtn);

    btnLayout->addStretch();
    layout->addWidget(btnRow);

    // Layer properties
    QWidget* props = new QWidget(dock);
    QHBoxLayout* propLayout = new QHBoxLayout(props);
    propLayout->setContentsMargins(0, 2, 0, 2);
    propLayout->addWidget(new QLabel(tr("Opacity:"), props));
    m_layerOpacitySlider = new QSlider(Qt::Horizontal, props);
    m_layerOpacitySlider->setRange(0, 100);
    m_layerOpacitySlider->setValue(100);
    propLayout->addWidget(m_layerOpacitySlider, 1);
    m_layerOpacityLabel = new QLabel("100%", props);
    m_layerOpacityLabel->setFixedWidth(36);
    propLayout->addWidget(m_layerOpacityLabel);
    layout->addWidget(props);

    connect(m_layerList, &QListWidget::currentRowChanged, this, &PaintEditor::onLayerListCurrentRowChanged);
    connect(m_layerOpacitySlider, &QSlider::valueChanged, this, &PaintEditor::onLayerOpacityChanged);
    connect(m_layerList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        if (!item || !m_document) return;
        int row = m_layerList->row(item);
        bool ok;
        QString name = QInputDialog::getText(this, tr("Rename Layer"), tr("Layer name:"),
                                             QLineEdit::Normal, m_document->layerAt(row).name, &ok);
        if (ok && !name.isEmpty()) m_document->setLayerName(row, name);
    });

    return dock;
}

QWidget* PaintEditor::setupBrushesPanel()
{
    QWidget* panel = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    layout->addWidget(new QLabel(tr("Preset Brush Sizes:"), panel));

    QGridLayout* grid = new QGridLayout;
    int sizes[] = { 5, 10, 20, 35, 50, 80, 120 };
    for (int i = 0; i < 7; ++i) {
        QToolButton* btn = new QToolButton(panel);
        btn->setText(QString::number(sizes[i]));
        btn->setFixedSize(48, 40);
        btn->setCheckable(true);
        btn->setProperty("size", sizes[i]);
        connect(btn, &QToolButton::clicked, this, [this, sizes, i]() {
            setBrushSize(sizes[i]);
            for (QToolButton* b : findChildren<QToolButton*>())
                if (b->property("size").isValid()) b->setChecked(b->property("size").toInt() == sizes[i]);
        });
        grid->addWidget(btn, i / 4, i % 4);
    }
    layout->addLayout(grid);
    layout->addStretch();
    return panel;
}

QWidget* PaintEditor::setupColorsPanel()
{
    QWidget* panel = new QWidget(this);
    panel->setObjectName("paintColorsPanel");
    QVBoxLayout* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    QWidget* swatches = new QWidget(panel);
    swatches->setObjectName("paintSwatchBox");
    QHBoxLayout* swLayout = new QHBoxLayout(swatches);
    swLayout->setContentsMargins(4, 4, 4, 4);

    QWidget* stack = new QWidget(swatches);
    stack->setFixedSize(64, 64);
    QVBoxLayout* stackLayout = new QVBoxLayout(stack);
    stackLayout->setContentsMargins(0, 0, 0, 0);

    m_fgSwatch = new QToolButton(stack);
    m_fgSwatch->setFixedSize(40, 40);
    m_fgSwatch->setStyleSheet("QToolButton { background: #000000; border: 2px solid #888; border-radius: 3px; }");
    m_fgSwatch->setToolTip(tr("Foreground color"));
    connect(m_fgSwatch, &QToolButton::clicked, this, &PaintEditor::onPrimaryColorSwatch);
    stackLayout->addWidget(m_fgSwatch, 0, Qt::AlignTop | Qt::AlignLeft);

    m_bgSwatch = new QToolButton(stack);
    m_bgSwatch->setFixedSize(40, 40);
    m_bgSwatch->setStyleSheet("QToolButton { background: #ffffff; border: 2px solid #888; border-radius: 3px; }");
    m_bgSwatch->setToolTip(tr("Background color"));
    connect(m_bgSwatch, &QToolButton::clicked, this, &PaintEditor::onSecondaryColorSwatch);
    stackLayout->addWidget(m_bgSwatch, 0, Qt::AlignBottom | Qt::AlignRight);

    swLayout->addWidget(stack);
    swLayout->addSpacing(4);

    QWidget* buttons = new QWidget(swatches);
    QVBoxLayout* btnLayout = new QVBoxLayout(buttons);
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(4);
    QToolButton* swapBtn = new QToolButton(buttons);
    swapBtn->setText(tr("Swap"));
    swapBtn->setToolTip(tr("Swap foreground/background"));
    connect(swapBtn, &QToolButton::clicked, this, &PaintEditor::onSwapColors);
    btnLayout->addWidget(swapBtn);
    QToolButton* resetBtn = new QToolButton(buttons);
    resetBtn->setText(tr("Default"));
    resetBtn->setToolTip(tr("Default colors (black/white)"));
    connect(resetBtn, &QToolButton::clicked, this, &PaintEditor::onResetColors);
    btnLayout->addWidget(resetBtn);
    swLayout->addWidget(buttons);
    swLayout->addStretch();

    layout->addWidget(swatches);
    layout->addStretch();

    setPrimaryColor(QColor(0, 0, 0));
    m_secondaryColor = QColor(255, 255, 255);
    if (m_bgSwatch) m_bgSwatch->setStyleSheet("QToolButton { background: #ffffff; border: 2px solid #888; border-radius: 3px; }");

    return panel;
}

// ── Slots ─────────────────────────────────────────────────────────────

void PaintEditor::onToolClicked()
{
    QToolButton* btn = qobject_cast<QToolButton*>(sender());
    if (!btn) return;
    PaintTool tool = static_cast<PaintTool>(btn->property("paintTool").toInt());
    setCurrentTool(tool);
}

void PaintEditor::onDocumentChanged()
{
    m_canvas->update();
    updateActionState();
}

void PaintEditor::onLayersChanged()
{
    refreshLayerList();
    updateLayerProperties();
    updateActionState();
}

void PaintEditor::onCurrentLayerChanged(int index)
{
    Q_UNUSED(index);
    updateLayerProperties();
    if (m_layerList && m_document) {
        int cur = m_document->currentLayerIndex();
        if (cur >= 0 && cur < m_layerList->count() && m_layerList->currentRow() != cur) {
            m_layerList->setCurrentRow(cur);
        }
    }
    updateActionState();
}

void PaintEditor::onSelectionChanged()
{
    updateActionState();
}

void PaintEditor::onCanvasImageEdited()
{
    QImage tex = currentTexture();
    emit imageEdited();
    emit textureChanged(tex);
}

void PaintEditor::onZoomChanged(float zoom)
{
    if (m_zoomLabel) m_zoomLabel->setText(QString("%1%").arg(int(zoom * 100)));
}

void PaintEditor::onStatusMessage(const QString& message)
{
    if (m_statusLabel) m_statusLabel->setText(message);
    emit statusMessage(message);
}

void PaintEditor::onTextToolClicked(const QPoint& imagePos)
{
    if (!hasDocument() || !m_document->currentLayer()) return;

    // Create text input dialog
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Text Tool"));
    dlg.setModal(true);
    dlg.resize(400, 300);

    QVBoxLayout* layout = new QVBoxLayout(&dlg);

    // Text input
    layout->addWidget(new QLabel(tr("Text:"), &dlg));
    QPlainTextEdit* textEdit = new QPlainTextEdit(&dlg);
    textEdit->setPlainText(m_textToolContent);
    textEdit->setMinimumHeight(80);
    layout->addWidget(textEdit);

    // Font options
    QGroupBox* fontGroup = new QGroupBox(tr("Font"), &dlg);
    QFormLayout* fontLayout = new QFormLayout(fontGroup);

    QFontComboBox* fontCombo = new QFontComboBox(&dlg);
    fontCombo->setCurrentFont(m_textFont);
    fontLayout->addRow(tr("Family:"), fontCombo);

    QSpinBox* sizeSpin = new QSpinBox(&dlg);
    sizeSpin->setRange(6, 200);
    sizeSpin->setValue(m_textFont.pointSize() > 0 ? m_textFont.pointSize() : 24);
    fontLayout->addRow(tr("Size:"), sizeSpin);

    QCheckBox* boldCheck = new QCheckBox(tr("Bold"), &dlg);
    boldCheck->setChecked(m_textFont.bold());
    fontLayout->addRow(boldCheck);

    QCheckBox* italicCheck = new QCheckBox(tr("Italic"), &dlg);
    italicCheck->setChecked(m_textFont.italic());
    fontLayout->addRow(italicCheck);

    layout->addWidget(fontGroup);

    // Color
    QHBoxLayout* colorLayout = new QHBoxLayout();
    colorLayout->addWidget(new QLabel(tr("Color:"), &dlg));
    QPushButton* colorBtn = new QPushButton(&dlg);
    colorBtn->setFixedSize(40, 24);
    colorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #333; border-radius: 3px;").arg(m_textColor.name()));
    connect(colorBtn, &QPushButton::clicked, [&]() {
        QColor c = QColorDialog::getColor(m_textColor, &dlg, tr("Text Color"));
        if (c.isValid()) {
            m_textColor = c;
            colorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #333; border-radius: 3px;").arg(c.name()));
        }
    });
    colorLayout->addWidget(colorBtn);
    colorLayout->addStretch();
    layout->addLayout(colorLayout);

    // Buttons
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(buttons);

    if (dlg.exec() == QDialog::Accepted) {
        QFont font = fontCombo->currentFont();
        font.setPointSize(sizeSpin->value());
        font.setBold(boldCheck->isChecked());
        font.setItalic(italicCheck->isChecked());

        setTextToolContent(textEdit->toPlainText(), font, m_textColor);
        applyTextTool();
    }
}

void PaintEditor::refreshLayerList()
{
    if (!m_layerList) return;
    m_updatingLayerUI = true;
    m_layerList->clear();
    if (!m_document) { m_updatingLayerUI = false; return; }

    for (int i = 0; i < m_document->layerCount(); ++i) {
        const PaintLayer& layer = m_document->layerAt(i);
        QString name = layer.name;
        if (!layer.visible) name = tr("(hidden) ") + name;
        QListWidgetItem* item = new QListWidgetItem(name, m_layerList);
        QImage thumb = layer.image.scaled(28, 28, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        if (!thumb.isNull()) {
            QPixmap pm = QPixmap::fromImage(thumb);
            QImage bg(28, 28, QImage::Format_ARGB32);
            bg.fill(QColor(60, 60, 66));
            QPainter p(&bg);
            p.drawImage((28 - pm.width()) / 2, (28 - pm.height()) / 2, pm.toImage());
            p.end();
            item->setIcon(QIcon(QPixmap::fromImage(bg)));
        }
        item->setData(Qt::UserRole, i);
    }

    int cur = m_document->currentLayerIndex();
    if (cur >= 0 && cur < m_layerList->count()) m_layerList->setCurrentRow(cur);
    m_updatingLayerUI = false;
}

void PaintEditor::updateLayerProperties()
{
    if (!m_document || m_updatingLayerUI) return;
    int cur = m_document->currentLayerIndex();
    if (cur < 0) return;
    const PaintLayer& layer = m_document->layerAt(cur);
    m_updatingLayerUI = true;
    if (m_layerOpacitySlider) {
        m_layerOpacitySlider->setValue(int(layer.opacity * 100));
    }
    if (m_layerOpacityLabel) m_layerOpacityLabel->setText(QString("%1%").arg(int(layer.opacity * 100)));
    if (m_blendCombo) m_blendCombo->setCurrentIndex(int(layer.blend));
    m_updatingLayerUI = false;
}

void PaintEditor::updateToolOptions()
{
    if (!m_canvas) return;
    bool isPaint = paintToolIsPaint(m_tool);
    bool isSelect = paintToolIsSelect(m_tool);

    if (m_sizeSlider) m_sizeSlider->setVisible(isPaint);
    if (m_sizeLabel) m_sizeLabel->setVisible(isPaint);
    if (m_hardnessSlider) m_hardnessSlider->setVisible(isPaint);
    if (m_hardnessLabel) m_hardnessLabel->setVisible(isPaint);
    if (m_opacitySlider) m_opacitySlider->setVisible(isPaint);
    if (m_opacityLabel) m_opacityLabel->setVisible(isPaint);
    if (m_flowSlider) m_flowSlider->setVisible(isPaint);
    if (m_flowLabel) m_flowLabel->setVisible(isPaint);
    if (m_strengthSlider) m_strengthSlider->setVisible(isPaint);
    if (m_strengthLabel) m_strengthLabel->setVisible(isPaint);

    bool needsTolerance = (m_tool == PaintTool::FuzzySelect || m_tool == PaintTool::BucketFill);
    if (m_toleranceSlider) m_toleranceSlider->setVisible(needsTolerance);
    if (m_toleranceLabel) m_toleranceLabel->setVisible(needsTolerance);

    bool blendVisible = (m_tool == PaintTool::Brush || m_tool == PaintTool::Pencil);
    if (m_blendCombo) m_blendCombo->setVisible(blendVisible);
}

void PaintEditor::updateActionState()
{
    // Called on history / doc changes; could update menu enable states.
    if (m_menuBar) {
        // Undo/Redo states could be toggled here
    }
}

void PaintEditor::applyBrushSettingsToCanvas()
{
    if (!m_canvas) return;
    m_canvas->setBrushSize(float(m_sizeSlider ? m_sizeSlider->value() : 20));
    m_canvas->setBrushHardness(float(m_hardnessSlider ? m_hardnessSlider->value() : 50) / 100.0f);
    m_canvas->setBrushOpacity(float(m_opacitySlider ? m_opacitySlider->value() : 100) / 100.0f);
    m_canvas->setBrushFlow(float(m_flowSlider ? m_flowSlider->value() : 100) / 100.0f);
    m_canvas->setBrushStrength(float(m_strengthSlider ? m_strengthSlider->value() : 100) / 100.0f);
}

void PaintEditor::onNewLayer()
{
    if (!hasDocument()) return;
    bool ok;
    QString name = QInputDialog::getText(this, tr("New Layer"), tr("Layer name:"),
                                         QLineEdit::Normal, tr("Layer %1").arg(m_document->layerCount() + 1), &ok);
    if (!ok) return;
    int idx = m_document->addLayer(name);
    refreshLayerList();
    if (m_layerList) m_layerList->setCurrentRow(idx);
    emit imageEdited();
}

void PaintEditor::onDuplicateLayer()
{
    if (!hasDocument()) return;
    int idx = m_document->currentLayerIndex();
    m_document->duplicateLayer(idx);
    refreshLayerList();
    emit imageEdited();
}

void PaintEditor::onRemoveLayer()
{
    if (!hasDocument()) return;
    if (m_document->layerCount() <= 1) {
        QMessageBox::information(this, tr("Remove Layer"), tr("Cannot remove the last layer."));
        return;
    }
    int idx = m_document->currentLayerIndex();
    m_document->removeLayer(idx);
    refreshLayerList();
    emit imageEdited();
}

void PaintEditor::onRaiseLayer()
{
    if (!hasDocument()) return;
    int idx = m_document->currentLayerIndex();
    if (idx < m_document->layerCount() - 1) {
        m_document->moveLayer(idx, idx + 1);
        refreshLayerList();
        emit imageEdited();
    }
}

void PaintEditor::onLowerLayer()
{
    if (!hasDocument()) return;
    int idx = m_document->currentLayerIndex();
    if (idx > 0) {
        m_document->moveLayer(idx, idx - 1);
        refreshLayerList();
        emit imageEdited();
    }
}

void PaintEditor::onLayerOpacityChanged(int value)
{
    if (m_updatingLayerUI || !m_document) return;
    m_document->setLayerOpacity(m_document->currentLayerIndex(), value / 100.0f);
    if (m_layerOpacityLabel) m_layerOpacityLabel->setText(QString("%1%").arg(value));
    refreshLayerList();
    emit imageEdited();
}

void PaintEditor::onLayerBlendChanged(int index)
{
    if (m_updatingLayerUI || !m_document) return;
    m_document->setLayerBlendMode(m_document->currentLayerIndex(), static_cast<PaintBlendMode>(index));
}

void PaintEditor::onLayerVisibilityClicked(int row)
{
    if (!m_document) return;
    if (row < 0 || row >= m_document->layerCount()) return;
    m_document->setLayerVisible(row, !m_document->layerAt(row).visible);
}

void PaintEditor::onLayerListCurrentRowChanged(int row)
{
    if (m_updatingLayerUI || !m_document) return;
    if (row >= 0 && row < m_document->layerCount()) {
        m_document->setCurrentLayer(row);
        updateLayerProperties();
    }
}

// ── Menu actions ─────────────────────────────────────────────────────────────

void PaintEditor::onFilterRequested()
{
    // Placeholder (subclass handlers used)
}

void PaintEditor::onNewImage()
{
    bool ok;
    int w = QInputDialog::getInt(this, tr("New Image"), tr("Width (px):"), 2048, 16, 8192, 1, &ok);
    if (!ok) return;
    int h = QInputDialog::getInt(this, tr("New Image"), tr("Height (px):"), 2048, 16, 8192, 1, &ok);
    if (!ok) return;

    QColor bg = QColorDialog::getColor(Qt::white, this, tr("Background color"));
    m_document->newDocument(w, h, bg.isValid() ? bg : QColor(Qt::white));
    m_canvas->setDocument(m_document);
    m_canvas->zoomToFit();
    refreshLayerList();
    emit imageEdited();
}

void PaintEditor::onOpenImage()
{
    QStringList formats;
    const QList<QByteArray> supported = QImageReader::supportedImageFormats();
    for (const QByteArray& fmt : supported) formats << "*." + QString::fromLatin1(fmt);
    QString file = QFileDialog::getOpenFileName(this, tr("Open Image"), m_carPath,
                                                tr("Images (%1)").arg(formats.join(" ")));
    if (file.isEmpty()) return;

    QImageReader reader(file);
    reader.setAutoTransform(true);
    QImage img = reader.read();
    if (img.isNull()) {
        QMessageBox::warning(this, tr("Open Image"), tr("Failed to load:\n%1").arg(reader.errorString()));
        return;
    }
    setTexture(img);
}

void PaintEditor::onSaveImage()
{
    if (!hasDocument()) return;
    onExportPng();
}

void PaintEditor::onExportPng()
{
    if (!hasDocument()) return;
    QString defaultPath = m_carPath.isEmpty() ? QStringLiteral("paint.png") : m_carPath + "/paint.png";
    QString file = QFileDialog::getSaveFileName(this, tr("Export PNG"),
                                                defaultPath,
                                                tr("PNG image (*.png)"));
    if (file.isEmpty()) return;
    QImage comp = currentTexture();
    if (comp.save(file, "PNG")) {
        onStatusMessage(tr("Saved: %1").arg(file));
    } else {
        QMessageBox::warning(this, tr("Export PNG"), tr("Failed to save image."));
    }
}

void PaintEditor::onExportDds()
{
    if (!hasDocument()) return;
    QString defaultPath = m_carPath.isEmpty() ? QStringLiteral("paint.dds") : m_carPath + "/paint.dds";
    QString file = QFileDialog::getSaveFileName(this, tr("Export DDS"),
                                                defaultPath,
                                                tr("DDS files (*.dds)"));
    if (file.isEmpty()) return;
    QImage comp = currentTexture();
    // Use PaintSystem to export as DDS with proper format
    if (PaintSystem::saveTextureAsDDS(comp, file)) {
        onStatusMessage(tr("Exported DDS: %1").arg(file));
    } else {
        QMessageBox::warning(this, tr("Export DDS"), tr("Failed to export DDS."));
    }
}

void PaintEditor::onSelectAll()
{
    if (!hasDocument()) return;
    QImage mask(m_document->size(), QImage::Format_ARGB32);
    mask.fill(QColor(255, 255, 255, 255));
    m_document->setSelectionMask(mask);
}

void PaintEditor::onSelectNone()
{
    if (m_document) m_document->clearSelection();
}

void PaintEditor::onSelectInvert()
{
    if (!m_document || !m_document->hasSelection()) return;
    QImage mask = m_document->selectionMask();
    mask.invertPixels();
    m_document->setSelectionMask(mask);
}

void PaintEditor::onZoomIn() { if (m_canvas) m_canvas->zoomIn(); }
void PaintEditor::onZoomOut() { if (m_canvas) m_canvas->zoomOut(); }
void PaintEditor::onZoomFit() { if (m_canvas) m_canvas->zoomToFit(); }
void PaintEditor::onZoom100() { if (m_canvas) m_canvas->zoom100(); }

void PaintEditor::onPrimaryColorSwatch()
{
    QColor color = QColorDialog::getColor(m_primaryColor, this, tr("Foreground color"));
    if (color.isValid()) setPrimaryColor(color);
}

void PaintEditor::onSecondaryColorSwatch()
{
    QColor color = QColorDialog::getColor(m_secondaryColor, this, tr("Background color"));
    if (color.isValid()) {
        m_secondaryColor = color;
        if (m_bgSwatch) m_bgSwatch->setStyleSheet(QString("QToolButton { background: %1; border: 2px solid #888; border-radius: 3px; }").arg(color.name()));
        if (m_canvas) m_canvas->setSecondaryColor(color);
    }
}

void PaintEditor::onSwapColors()
{
    QColor tmp = m_primaryColor;
    setPrimaryColor(m_secondaryColor);
    m_secondaryColor = tmp;
    if (m_bgSwatch) m_bgSwatch->setStyleSheet(QString("QToolButton { background: %1; border: 2px solid #888; border-radius: 3px; }").arg(tmp.name()));
    if (m_canvas) m_canvas->setSecondaryColor(tmp);
}

void PaintEditor::onResetColors()
{
    setPrimaryColor(QColor(0, 0, 0));
    m_secondaryColor = QColor(255, 255, 255);
    if (m_bgSwatch) m_bgSwatch->setStyleSheet("QToolButton { background: #ffffff; border: 2px solid #888; border-radius: 3px; }");
    if (m_canvas) m_canvas->setSecondaryColor(m_secondaryColor);
}

void PaintEditor::onAddDecalLayer()
{
    if (!hasDocument()) return;
    QString file = QFileDialog::getOpenFileName(this, tr("Add Decal Layer"), m_carPath,
                                                tr("Images (*.png *.jpg *.jpeg *.bmp *.webp *.tga)"));
    if (file.isEmpty()) return;
    QImage img(file);
    if (img.isNull()) {
        QMessageBox::warning(this, tr("Add Decal"), tr("Failed to load image."));
        return;
    }
    QFileInfo fi(file);
    m_document->addLayerImage(fi.completeBaseName(), img);
    refreshLayerList();
    emit imageEdited();
}

void PaintEditor::onInvert()
{
    if (!hasDocument() || !m_document->currentLayer()) return;
    m_document->pushUndo();
    QImage layer = m_document->currentLayerImage();
    QImage masked = m_document->applySelection(layer);
    QImage filtered = PaintPainter::applyFilter(masked, QStringLiteral("invert"));
    // Blend filtered back only in selection
    QImage result = layer;
    QImage mask = m_document->selectionMask();
    for (int y = 0; y < result.height(); ++y) {
        QRgb* line = reinterpret_cast<QRgb*>(result.scanLine(y));
        const QRgb* fl = reinterpret_cast<const QRgb*>(filtered.constScanLine(y));
        const QRgb* m = mask.isNull() ? nullptr : reinterpret_cast<const QRgb*>(mask.constScanLine(y));
        for (int x = 0; x < result.width(); ++x) {
            if (!m || qAlpha(m[x]) > 0) line[x] = fl[x];
        }
    }
    m_document->setCurrentLayerImage(result);
    emit imageEdited();
}

void PaintEditor::onGrayscale()
{
    if (!hasDocument() || !m_document->currentLayer()) return;
    m_document->pushUndo();
    QImage layer = m_document->currentLayerImage();
    QImage masked = m_document->applySelection(layer);
    QImage filtered = PaintPainter::applyFilter(masked, QStringLiteral("grayscale"));
    QImage result = layer;
    QImage mask = m_document->selectionMask();
    for (int y = 0; y < result.height(); ++y) {
        QRgb* line = reinterpret_cast<QRgb*>(result.scanLine(y));
        const QRgb* fl = reinterpret_cast<const QRgb*>(filtered.constScanLine(y));
        const QRgb* m = mask.isNull() ? nullptr : reinterpret_cast<const QRgb*>(mask.constScanLine(y));
        for (int x = 0; x < result.width(); ++x) {
            if (!m || qAlpha(m[x]) > 0) line[x] = fl[x];
        }
    }
    m_document->setCurrentLayerImage(result);
    emit imageEdited();
}

void PaintEditor::onSepia()
{
    if (!hasDocument() || !m_document->currentLayer()) return;
    m_document->pushUndo();
    QImage layer = m_document->currentLayerImage();
    QImage masked = m_document->applySelection(layer);
    QImage filtered = PaintPainter::applyFilter(masked, QStringLiteral("sepia"));
    QImage result = layer;
    QImage mask = m_document->selectionMask();
    for (int y = 0; y < result.height(); ++y) {
        QRgb* line = reinterpret_cast<QRgb*>(result.scanLine(y));
        const QRgb* fl = reinterpret_cast<const QRgb*>(filtered.constScanLine(y));
        const QRgb* m = mask.isNull() ? nullptr : reinterpret_cast<const QRgb*>(mask.constScanLine(y));
        for (int x = 0; x < result.width(); ++x) {
            if (!m || qAlpha(m[x]) > 0) line[x] = fl[x];
        }
    }
    m_document->setCurrentLayerImage(result);
    emit imageEdited();
}

void PaintEditor::onBrightnessContrast()
{
    if (!hasDocument() || !m_document->currentLayer()) return;
    bool ok;
    int brightness = QInputDialog::getInt(this, tr("Brightness"), tr("Brightness (-255..255):"), 0, -255, 255, 1, &ok);
    if (!ok) return;
    int contrast = QInputDialog::getInt(this, tr("Contrast"), tr("Contrast (-255..255):"), 0, -255, 255, 1, &ok);
    if (!ok) return;

    m_document->pushUndo();
    QImage layer = m_document->currentLayerImage();
    QImage masked = m_document->applySelection(layer);
    QVariantMap params;
    params.insert(QStringLiteral("delta"), brightness);
    QImage filtered = PaintPainter::applyFilter(masked, QStringLiteral("brightness"), params);
    params.clear();
    params.insert(QStringLiteral("delta"), contrast);
    filtered = PaintPainter::applyFilter(filtered, QStringLiteral("contrast"), params);

    QImage result = layer;
    QImage mask = m_document->selectionMask();
    for (int y = 0; y < result.height(); ++y) {
        QRgb* line = reinterpret_cast<QRgb*>(result.scanLine(y));
        const QRgb* fl = reinterpret_cast<const QRgb*>(filtered.constScanLine(y));
        const QRgb* m = mask.isNull() ? nullptr : reinterpret_cast<const QRgb*>(mask.constScanLine(y));
        for (int x = 0; x < result.width(); ++x) {
            if (!m || qAlpha(m[x]) > 0) line[x] = fl[x];
        }
    }
    m_document->setCurrentLayerImage(result);
    emit imageEdited();
}

void PaintEditor::onBlur()
{
    if (!hasDocument() || !m_document->currentLayer()) return;
    bool ok;
    int radius = QInputDialog::getInt(this, tr("Gaussian Blur"), tr("Blur radius (px):"), 3, 1, 64, 1, &ok);
    if (!ok) return;

    m_document->pushUndo();
    QImage layer = m_document->currentLayerImage();
    QImage masked = m_document->applySelection(layer);
    QVariantMap params;
    params.insert(QStringLiteral("radius"), radius);
    QImage filtered = PaintPainter::applyFilter(masked, QStringLiteral("blur"), params);
    filtered = filtered.convertToFormat(QImage::Format_ARGB32);
    if (filtered.size() != layer.size()) filtered = filtered.scaled(layer.size());

    QImage result = layer;
    QImage mask = m_document->selectionMask();
    for (int y = 0; y < result.height(); ++y) {
        QRgb* line = reinterpret_cast<QRgb*>(result.scanLine(y));
        const QRgb* fl = reinterpret_cast<const QRgb*>(filtered.constScanLine(y));
        const QRgb* m = mask.isNull() ? nullptr : reinterpret_cast<const QRgb*>(mask.constScanLine(y));
        for (int x = 0; x < result.width(); ++x) {
            if (!m || qAlpha(m[x]) > 0) line[x] = fl[x];
        }
    }
    m_document->setCurrentLayerImage(result);
    emit imageEdited();
}

void PaintEditor::onSharpen()
{
    if (!hasDocument() || !m_document->currentLayer()) return;
    bool ok;
    int radius = QInputDialog::getInt(this, tr("Sharpen"), tr("Sharpen radius (px):"), 3, 1, 64, 1, &ok);
    if (!ok) return;
    int amount = QInputDialog::getInt(this, tr("Sharpen"), tr("Sharpen amount (0-100):"), 50, 0, 100, 1, &ok);
    if (!ok) return;

    m_document->pushUndo();
    QImage layer = m_document->currentLayerImage();
    QImage masked = m_document->applySelection(layer);
    QVariantMap params;
    params.insert(QStringLiteral("amount"), amount);
    QImage filtered = PaintPainter::applyFilter(masked, QStringLiteral("sharpen"), params);
    filtered = filtered.convertToFormat(QImage::Format_ARGB32);
    if (filtered.size() != layer.size()) filtered = filtered.scaled(layer.size());

    QImage result = layer;
    QImage mask = m_document->selectionMask();
    for (int y = 0; y < result.height(); ++y) {
        QRgb* line = reinterpret_cast<QRgb*>(result.scanLine(y));
        const QRgb* fl = reinterpret_cast<const QRgb*>(filtered.constScanLine(y));
        const QRgb* m = mask.isNull() ? nullptr : reinterpret_cast<const QRgb*>(mask.constScanLine(y));
        for (int x = 0; x < result.width(); ++x) {
            if (!m || qAlpha(m[x]) > 0) line[x] = fl[x];
        }
    }
    m_document->setCurrentLayerImage(result);
    emit imageEdited();
}

void PaintEditor::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
}

} // namespace paint
} // namespace ks