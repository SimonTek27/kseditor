#include "FontCreatorQmlBridge.h"
#include "core/sys/LogManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QFileDialog>
#include <QMessageBox>
#include <QFontDatabase>

namespace ks {

FontCreatorEditorModule::FontCreatorEditorModule(QWidget* parent)
    : ModuleGuiBase(parent)
{
    setObjectName("FontCreatorEditorModule");
}

bool FontCreatorEditorModule::initialize()
{
    if (m_uiBuilt) return true;
    bool ok = ModuleGuiBase::initialize();
    refreshFonts();
    LOG_INFO("FontCreatorEditorModule", "Initialized");
    return ok;
}

void FontCreatorEditorModule::shutdown()
{
    ModuleGuiBase::shutdown();
}

void FontCreatorEditorModule::importFile(const QString& filePath)
{
    if (auto* bridge = FontCreatorQmlBridge::instance()) {
        bridge->loadPreset(filePath);
        logSuccess("Loaded preset: " + filePath);
    }
}

void FontCreatorEditorModule::exportFile(const QString& filePath)
{
    if (auto* bridge = FontCreatorQmlBridge::instance()) {
        bridge->savePreset(filePath);
        logSuccess("Saved preset: " + filePath);
    }
}

QJsonObject FontCreatorEditorModule::serializeProject() const
{
    return QJsonObject();
}

void FontCreatorEditorModule::deserializeProject(const QJsonObject& data)
{
    Q_UNUSED(data);
}

void FontCreatorEditorModule::buildUI()
{
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #3a3a3a; background: #1e1e1e; }"
        "QTabBar::tab { background: #2d2d2d; color: #aaa; padding: 8px 16px; border: 1px solid #3a3a3a; border-bottom: none; }"
        "QTabBar::tab:selected { background: #3a5a8a; color: #fff; }"
        "QTabBar::tab:hover { background: #4a6a9a; }"
    );

    // Tab 1: Font Selection
    QWidget* fontTab = new QWidget();
    {
        QVBoxLayout* layout = new QVBoxLayout(fontTab);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(8);

        QGroupBox* fontGroup = new QGroupBox("Font Settings");
        QFormLayout* form = new QFormLayout(fontGroup);

        m_fontCombo = new QComboBox();
        m_fontCombo->setMinimumWidth(250);
        connect(m_fontCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FontCreatorEditorModule::onFontChanged);
        form->addRow("Font Family:", m_fontCombo);

        m_sizeSpin = new QSpinBox();
        m_sizeSpin->setRange(8, 256);
        m_sizeSpin->setValue(48);
        connect(m_sizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &FontCreatorEditorModule::onSizeChanged);
        form->addRow("Font Size:", m_sizeSpin);

        m_weightCombo = new QComboBox();
        m_weightCombo->addItems({"Thin (100)", "Light (200)", "Normal (400)", "Medium (500)", "Semi-Bold (600)", "Bold (700)", "Black (900)"});
        m_weightCombo->setCurrentIndex(2);
        form->addRow("Weight:", m_weightCombo);

        m_italicCheck = new QCheckBox("Italic");
        form->addRow("Style:", m_italicCheck);

        m_modeCombo = new QComboBox();
        m_modeCombo->addItems({"Standard Bitmap", "SDF (Signed Distance Field)", "MSDF (Multi-channel SDF)"});
        form->addRow("Atlas Mode:", m_modeCombo);
        connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FontCreatorEditorModule::onModeChanged);

        layout->addWidget(fontGroup);
        layout->addStretch();
    }
    m_tabWidget->addTab(fontTab, "Font");

    // Tab 2: Atlas Settings
    QWidget* atlasTab = new QWidget();
    {
        QVBoxLayout* layout = new QVBoxLayout(atlasTab);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(8);

        QGroupBox* atlasGroup = new QGroupBox("Atlas Dimensions");
        QFormLayout* form = new QFormLayout(atlasGroup);

        m_atlasWidthSpin = new QSpinBox();
        m_atlasWidthSpin->setRange(64, 8192);
        m_atlasWidthSpin->setValue(512);
        m_atlasWidthSpin->setSingleStep(64);
        connect(m_atlasWidthSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &FontCreatorEditorModule::onAtlasWidthChanged);
        form->addRow("Width:", m_atlasWidthSpin);

        m_atlasHeightSpin = new QSpinBox();
        m_atlasHeightSpin->setRange(64, 8192);
        m_atlasHeightSpin->setValue(512);
        m_atlasHeightSpin->setSingleStep(64);
        connect(m_atlasHeightSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &FontCreatorEditorModule::onAtlasHeightChanged);
        form->addRow("Height:", m_atlasHeightSpin);

        m_globalHeightSpin = new QSpinBox();
        m_globalHeightSpin->setRange(10, 200);
        m_globalHeightSpin->setValue(85);
        form->addRow("Global Height (%):", m_globalHeightSpin);

        m_globalVPadSpin = new QSpinBox();
        m_globalVPadSpin->setRange(0, 50);
        m_globalVPadSpin->setValue(13);
        form->addRow("Vertical Padding:", m_globalVPadSpin);

        layout->addWidget(atlasGroup);

        QGroupBox* hintGroup = new QGroupBox("Hinting & Anti-Aliasing");
        QFormLayout* hform = new QFormLayout(hintGroup);

        m_hintingCheck = new QCheckBox("Enable Hinting");
        m_hintingCheck->setChecked(true);
        hform->addRow("", m_hintingCheck);

        m_hintingLevelCombo = new QComboBox();
        m_hintingLevelCombo->addItems({"None", "Light", "Normal", "Full"});
        m_hintingLevelCombo->setCurrentIndex(1);
        hform->addRow("Hinting Level:", m_hintingLevelCombo);

        m_gridFitCheck = new QCheckBox("Grid Fitting");
        m_gridFitCheck->setChecked(true);
        hform->addRow("", m_gridFitCheck);

        m_subpixelCheck = new QCheckBox("Subpixel Hinting");
        hform->addRow("", m_subpixelCheck);

        m_aaModeCombo = new QComboBox();
        m_aaModeCombo->addItems({"None", "Standard", "LCD (subpixel)"});
        m_aaModeCombo->setCurrentIndex(1);
        hform->addRow("Anti-Alias Mode:", m_aaModeCombo);

        layout->addWidget(hintGroup);

        QGroupBox* charsetGroup = new QGroupBox("Character Set");
        QVBoxLayout* clayout = new QVBoxLayout(charsetGroup);
        m_charsetEdit = new QLineEdit();
        m_charsetEdit->setPlaceholderText("Custom characters (leave empty for full charset)");
        clayout->addWidget(m_charsetEdit);
        layout->addWidget(charsetGroup);

        layout->addStretch();
    }
    m_tabWidget->addTab(atlasTab, "Atlas");

    // Tab 3: Glyph Atlas
    QWidget* glyphTab = new QWidget();
    {
        QVBoxLayout* layout = new QVBoxLayout(glyphTab);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(8);

        m_glyphTable = new QTableWidget(0, 4);
        m_glyphTable->setHorizontalHeaderLabels({"Codepoint", "Char", "Width", "Height"});
        m_glyphTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        m_glyphTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        m_glyphTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
        m_glyphTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
        m_glyphTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_glyphTable->setStyleSheet("QTableWidget { background: #1a1a1a; color: #c8c8c8; gridline-color: #3a3a3a; } QHeaderView::section { background: #2d2d2d; color: #aaa; }");
        layout->addWidget(m_glyphTable);

        m_statsText = new QTextEdit();
        m_statsText->setReadOnly(true);
        m_statsText->setMaximumHeight(80);
        m_statsText->setStyleSheet("QTextEdit { background: #1a1a1a; color: #c8c8c8; font-family: Consolas; font-size: 10px; }");
        layout->addWidget(m_statsText);
    }
    m_tabWidget->addTab(glyphTab, "Glyphs");

    // Tab 4: Preview
    QWidget* previewTab = new QWidget();
    {
        QVBoxLayout* layout = new QVBoxLayout(previewTab);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(8);

        QGroupBox* previewGroup = new QGroupBox("Text Preview");
        QVBoxLayout* pl = new QVBoxLayout(previewGroup);
        m_previewEdit = new QTextEdit();
        m_previewEdit->setPlaceholderText("Type preview text here...");
        m_previewEdit->setMaximumHeight(100);
        pl->addWidget(m_previewEdit);

        m_previewLabel = new QLabel("Preview will appear here after generation");
        m_previewLabel->setMinimumHeight(200);
        m_previewLabel->setAlignment(Qt::AlignCenter);
        m_previewLabel->setStyleSheet("QLabel { background: #0a0a0a; border: 1px solid #3a3a3a; color: #fff; font-size: 24px; }");
        pl->addWidget(m_previewLabel);

        layout->addWidget(previewGroup);
    }
    m_tabWidget->addTab(previewTab, "Preview");

    // Tab 5: Export
    QWidget* exportTab = new QWidget();
    {
        QVBoxLayout* layout = new QVBoxLayout(exportTab);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(8);

        QGroupBox* outputGroup = new QGroupBox("Output Settings");
        QHBoxLayout* ol = new QHBoxLayout(outputGroup);
        m_outputPathEdit = new QLineEdit();
        m_outputPathEdit->setPlaceholderText("Output directory...");
        ol->addWidget(m_outputPathEdit);
        m_browseBtn = new QPushButton("Browse");
        connect(m_browseBtn, &QPushButton::clicked, this, &FontCreatorEditorModule::onBrowseOutput);
        ol->addWidget(m_browseBtn);
        layout->addWidget(outputGroup);

        QGroupBox* actionsGroup = new QGroupBox("Actions");
        QVBoxLayout* al = new QVBoxLayout(actionsGroup);

        QHBoxLayout* btnLayout = new QHBoxLayout();
        m_generateBtn = new QPushButton("Generate Atlas");
        m_generateBtn->setStyleSheet("QPushButton { background: #2d6b2d; color: white; padding: 8px 16px; border: none; border-radius: 4px; } QPushButton:hover { background: #3a8a3a; }");
        connect(m_generateBtn, &QPushButton::clicked, this, &FontCreatorEditorModule::onGenerateAtlas);
        btnLayout->addWidget(m_generateBtn);

        m_exportACFBtn = new QPushButton("Export ACF");
        m_exportACFBtn->setStyleSheet("QPushButton { background: #3a5a8a; color: white; padding: 8px 16px; border: none; border-radius: 4px; } QPushButton:hover { background: #4a6a9a; }");
        connect(m_exportACFBtn, &QPushButton::clicked, this, &FontCreatorEditorModule::onExportACF);
        btnLayout->addWidget(m_exportACFBtn);

        m_loadPresetBtn = new QPushButton("Load Preset");
        connect(m_loadPresetBtn, &QPushButton::clicked, this, &FontCreatorEditorModule::onLoadPreset);
        btnLayout->addWidget(m_loadPresetBtn);

        m_savePresetBtn = new QPushButton("Save Preset");
        connect(m_savePresetBtn, &QPushButton::clicked, this, &FontCreatorEditorModule::onSavePreset);
        btnLayout->addWidget(m_savePresetBtn);

        al->addLayout(btnLayout);
        layout->addWidget(actionsGroup);

        m_logOutput = new QTextEdit();
        m_logOutput->setReadOnly(true);
        m_logOutput->setMaximumHeight(150);
        m_logOutput->setStyleSheet("QTextEdit { background: #0a0a0a; color: #c8c8c8; font-family: Consolas; font-size: 10px; }");
        layout->addWidget(m_logOutput);

        layout->addStretch();
    }
    m_tabWidget->addTab(exportTab, "Export");

    m_mainLayout->insertWidget(1, m_tabWidget, 1);
    m_uiBuilt = true;
}

void FontCreatorEditorModule::refreshFonts()
{
    m_fontCombo->clear();
    QFontDatabase db;
    QStringList families = db.families();
    families.sort();
    m_fontCombo->addItems(families);
    if (auto* bridge = FontCreatorQmlBridge::instance()) {
        int idx = m_fontCombo->findText(bridge->currentFont());
        if (idx >= 0) m_fontCombo->setCurrentIndex(idx);
    }
}

void FontCreatorEditorModule::updatePreview()
{
    QString text = m_previewEdit->toPlainText();
    if (text.isEmpty()) {
        text = "The quick brown fox jumps over the lazy dog\n0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    }
    m_previewLabel->setText(text);
    m_previewLabel->setStyleSheet(QString(
        "QLabel { background: #0a0a0a; border: 1px solid #3a3a3a; color: #fff;"
        " font-family: '%1'; font-size: %2px; padding: 10px; }")
        .arg(m_fontCombo->currentText())
        .arg(m_sizeSpin->value()));
}

// Slots
void FontCreatorEditorModule::onFontChanged(int) { updatePreview(); }
void FontCreatorEditorModule::onSizeChanged(int) { updatePreview(); }
void FontCreatorEditorModule::onAtlasWidthChanged(int width) {
    Q_UNUSED(width);
}
void FontCreatorEditorModule::onAtlasHeightChanged(int height) {
    Q_UNUSED(height);
}
void FontCreatorEditorModule::onModeChanged(int index) {
    static const QStringList modes = {"Regular", "SDF", "MSDF"};
    log(QString("Font render mode: %1").arg(modes.value(index, "Unknown")));
}

void FontCreatorEditorModule::onGenerateAtlas()
{
    auto* bridge = FontCreatorQmlBridge::instance();
    if (!bridge) return;

    QString outputDir = m_outputPathEdit->text();
    if (outputDir.isEmpty()) {
        outputDir = QFileDialog::getExistingDirectory(this, "Select Output Directory");
        if (outputDir.isEmpty()) return;
        m_outputPathEdit->setText(outputDir);
    }

    QString pngPath = outputDir + "/" + m_fontCombo->currentText().replace(' ', '_') + ".png";

    bridge->setFontFamily(m_fontCombo->currentText());
    bridge->setFontSize(m_sizeSpin->value());
    bridge->setFontWeight((m_weightCombo->currentIndex() + 1) * 100);
    bridge->setItalic(m_italicCheck->isChecked());
    bridge->setAtlasWidth(m_atlasWidthSpin->value());
    bridge->setAtlasHeight(m_atlasHeightSpin->value());
    bridge->setGlobalHeight(m_globalHeightSpin->value());
    bridge->setGlobalVPad(m_globalVPadSpin->value());

    bool ok = false;
    switch (m_modeCombo->currentIndex()) {
        case 0: ok = bridge->generateAtlas(pngPath); break;
        case 1: ok = bridge->generateSDFAtlas(pngPath, 4); break;
        case 2: ok = bridge->generateMSDFAtlas(pngPath, 4); break;
    }

    if (ok) {
        m_logOutput->append("Atlas generated: " + pngPath);
        updatePreview();
    } else {
        m_logOutput->append("Failed to generate atlas");
    }
}

void FontCreatorEditorModule::onLoadPreset()
{
    QString path = QFileDialog::getOpenFileName(this, "Load Font Preset", "", "AC Font Preset (*.acf);;JSON (*.json)");
    if (!path.isEmpty()) {
        if (auto* bridge = FontCreatorQmlBridge::instance()) {
            bridge->loadPreset(path);
            m_logOutput->append("Loaded preset: " + path);
        }
    }
}

void FontCreatorEditorModule::onSavePreset()
{
    QString path = QFileDialog::getSaveFileName(this, "Save Font Preset", "", "AC Font Preset (*.acf);;JSON (*.json)");
    if (!path.isEmpty()) {
        if (auto* bridge = FontCreatorQmlBridge::instance()) {
            bridge->savePreset(path);
            m_logOutput->append("Saved preset: " + path);
        }
    }
}

void FontCreatorEditorModule::onExportACF()
{
    QString path = QFileDialog::getSaveFileName(this, "Export ACF", "", "AC Font Config (*.acf)");
    if (!path.isEmpty()) {
        if (auto* bridge = FontCreatorQmlBridge::instance()) {
            bridge->saveAtlas(m_outputPathEdit->text() + "/font.png", path);
            m_logOutput->append("Exported ACF: " + path);
        }
    }
}

void FontCreatorEditorModule::onBrowseOutput()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Output Directory");
    if (!dir.isEmpty()) m_outputPathEdit->setText(dir);
}

} // namespace ks
