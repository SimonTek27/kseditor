#include "UIEditorModule.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QTreeWidgetItem>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QStandardPaths>
#include <QColorDialog>
#include <QMenu>
#include <QAction>

namespace ks {
namespace ui {

UIEditorModule::UIEditorModule(QWidget* parent)
    : ModuleGuiBase(parent)
    , m_tabWidget(nullptr)
    , m_themeTab(nullptr)
    , m_themeCombo(nullptr)
    , m_accentColorBtn(nullptr)
    , m_accentColorPreview(nullptr)
    , m_fontSizeSpin(nullptr)
    , m_animationsCheck(nullptr)
    , m_animationsReduceCheck(nullptr)
    , m_themePreviewLabel(nullptr)
    , m_layoutTab(nullptr)
    , m_layoutCombo(nullptr)
    , m_resetLayoutBtn(nullptr)
    , m_saveLayoutBtn(nullptr)
    , m_loadLayoutBtn(nullptr)
    , m_layoutTree(nullptr)
    , m_shortcutsTab(nullptr)
    , m_shortcutTree(nullptr)
    , m_configureShortcutsBtn(nullptr)
    , m_shortcutSearchInput(nullptr)
    , m_widgetsTab(nullptr)
    , m_widgetTree(nullptr)
    , m_openNodeGraphBtn(nullptr)
{
    setObjectName("UIEditorModule");
}

bool UIEditorModule::initialize() {
    if (m_uiBuilt) return true;
    ModuleGuiBase::initialize();
    return true;
}

void UIEditorModule::shutdown() {
    m_uiBuilt = false;
}

void UIEditorModule::onActivation() {}
void UIEditorModule::onDeactivation() {}

void UIEditorModule::buildUI() {
    m_tabWidget = new QTabWidget();

    setupThemeTab();
    setupLayoutTab();
    setupShortcutsTab();
    setupWidgetsTab();

    m_mainLayout->addWidget(m_tabWidget);
    m_mainLayout->addWidget(m_logOutput);
}

void UIEditorModule::setupThemeTab() {
    m_themeTab = new QWidget();
    auto* layout = new QVBoxLayout(m_themeTab);
    auto* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    auto* container = new QWidget();
    auto* containerLayout = new QVBoxLayout(container);

    auto* themeGroup = createGroupBox("Theme Selection");
    auto* themeForm = new QFormLayout(themeGroup);
    m_themeCombo = createComboBox({"ksEditor Dark (Default)", "ksEditor Light", "Monokai Pro", "Dracula", "Nord", "One Dark", "Solarized Dark", "Custom"});
    themeForm->addRow("Theme:", m_themeCombo);
    containerLayout->addWidget(themeGroup);

    auto* accentGroup = createGroupBox("Accent Color");
    auto* accentLayout = new QVBoxLayout(accentGroup);
    auto* accentRow = new QHBoxLayout();
    m_accentColorBtn = createButton("Choose Color...");
    m_accentColorPreview = new QLabel();
    m_accentColorPreview->setFixedSize(32, 32);
    m_accentColorPreview->setStyleSheet("QLabel { background: #3a5a8a; border: 1px solid #4a6a9a; border-radius: 4px; }");
    accentRow->addWidget(m_accentColorBtn);
    accentRow->addWidget(m_accentColorPreview);
    accentRow->addStretch();
    accentLayout->addLayout(accentRow);
    containerLayout->addWidget(accentGroup);

    auto* fontGroup = createGroupBox("Font Settings");
    auto* fontForm = new QFormLayout(fontGroup);
    m_fontSizeSpin = createSpinBox(8, 24, 11, " pt");
    fontForm->addRow("UI Font Size:", m_fontSizeSpin);
    containerLayout->addWidget(fontGroup);

    auto* animGroup = createGroupBox("Animations");
    auto* animLayout = new QVBoxLayout(animGroup);
    m_animationsCheck = createCheckBox("Enable UI animations", true);
    m_animationsReduceCheck = createCheckBox("Reduce motion", false);
    animLayout->addWidget(m_animationsCheck);
    animLayout->addWidget(m_animationsReduceCheck);
    containerLayout->addWidget(animGroup);

    m_themePreviewLabel = createLabel("Theme preview area");
    m_themePreviewLabel->setMinimumHeight(100);
    m_themePreviewLabel->setStyleSheet("QLabel { background: #1e1e1e; border: 1px solid #3a3a3a; border-radius: 4px; padding: 12px; color: #cccccc; }");
    containerLayout->addWidget(m_themePreviewLabel);

    containerLayout->addStretch();
    scrollArea->setWidget(container);
    layout->addWidget(scrollArea);

    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &UIEditorModule::onThemeSelected);
    connect(m_accentColorBtn, &QPushButton::clicked, this, &UIEditorModule::onAccentColorChanged);
    connect(m_fontSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &UIEditorModule::onFontSizeChanged);
    connect(m_animationsCheck, &QCheckBox::toggled, this, &UIEditorModule::onToggleAnimations);

    m_tabWidget->addTab(m_themeTab, "Theme");
}

void UIEditorModule::setupLayoutTab() {
    m_layoutTab = new QWidget();
    auto* layout = new QVBoxLayout(m_layoutTab);

    auto* btnLayout = new QHBoxLayout();
    m_layoutCombo = createComboBox({"Default Layout", "Modeling", "Physics Tuning", "Audio Mixing", "Livery Design", "Full Screen"});
    m_resetLayoutBtn = createButton("Reset Layout");
    m_saveLayoutBtn = createButton("Save Layout");
    m_loadLayoutBtn = createButton("Load Layout");
    btnLayout->addWidget(new QLabel("Layout:"));
    btnLayout->addWidget(m_layoutCombo);
    btnLayout->addWidget(m_saveLayoutBtn);
    btnLayout->addWidget(m_loadLayoutBtn);
    btnLayout->addWidget(m_resetLayoutBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    m_layoutTree = createTreeWidget({"Dock Widget", "Position", "Visible"});
    m_layoutTree->addTopLevelItem(new QTreeWidgetItem({"Asset Browser", "Left", "Yes"}));
    m_layoutTree->addTopLevelItem(new QTreeWidgetItem({"Properties", "Right", "Yes"}));
    m_layoutTree->addTopLevelItem(new QTreeWidgetItem({"Log Output", "Bottom", "Yes"}));
    m_layoutTree->addTopLevelItem(new QTreeWidgetItem({"Node Graph", "Right", "No"}));
    m_layoutTree->addTopLevelItem(new QTreeWidgetItem({"Timeline", "Bottom", "No"}));
    layout->addWidget(m_layoutTree);

    connect(m_layoutCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &UIEditorModule::onLayoutSelected);
    connect(m_resetLayoutBtn, &QPushButton::clicked, this, &UIEditorModule::onResetLayout);
    connect(m_saveLayoutBtn, &QPushButton::clicked, this, &UIEditorModule::onSaveLayout);
    connect(m_loadLayoutBtn, &QPushButton::clicked, this, &UIEditorModule::onLoadLayout);

    m_tabWidget->addTab(m_layoutTab, "Layout");
}

void UIEditorModule::setupShortcutsTab() {
    m_shortcutsTab = new QWidget();
    auto* layout = new QVBoxLayout(m_shortcutsTab);

    auto* searchLayout = new QHBoxLayout();
    m_shortcutSearchInput = new QLineEdit();
    m_shortcutSearchInput->setPlaceholderText("Search shortcuts...");
    m_configureShortcutsBtn = createButton("Configure Shortcuts");
    searchLayout->addWidget(m_shortcutSearchInput);
    searchLayout->addWidget(m_configureShortcutsBtn);
    layout->addLayout(searchLayout);

    m_shortcutTree = createTreeWidget({"Command", "Shortcut", "Category"});
    m_shortcutTree->addTopLevelItem(new QTreeWidgetItem({"New Project", "Ctrl+N", "File"}));
    m_shortcutTree->addTopLevelItem(new QTreeWidgetItem({"Open Project", "Ctrl+O", "File"}));
    m_shortcutTree->addTopLevelItem(new QTreeWidgetItem({"Save", "Ctrl+S", "File"}));
    m_shortcutTree->addTopLevelItem(new QTreeWidgetItem({"Undo", "Ctrl+Z", "Edit"}));
    m_shortcutTree->addTopLevelItem(new QTreeWidgetItem({"Redo", "Ctrl+Y", "Edit"}));
    m_shortcutTree->addTopLevelItem(new QTreeWidgetItem({"Cut", "Ctrl+X", "Edit"}));
    m_shortcutTree->addTopLevelItem(new QTreeWidgetItem({"Copy", "Ctrl+C", "Edit"}));
    m_shortcutTree->addTopLevelItem(new QTreeWidgetItem({"Paste", "Ctrl+V", "Edit"}));
    m_shortcutTree->addTopLevelItem(new QTreeWidgetItem({"Delete", "Del", "Edit"}));
    m_shortcutTree->addTopLevelItem(new QTreeWidgetItem({"Find", "Ctrl+F", "Edit"}));
    m_shortcutTree->addTopLevelItem(new QTreeWidgetItem({"Build Project", "F7", "Project"}));
    m_shortcutTree->addTopLevelItem(new QTreeWidgetItem({"Run", "F5", "Project"}));
    m_shortcutTree->addTopLevelItem(new QTreeWidgetItem({"Toggle Console", "Ctrl+`", "View"}));
    layout->addWidget(m_shortcutTree);

    connect(m_configureShortcutsBtn, &QPushButton::clicked, this, &UIEditorModule::onConfigureShortcuts);

    m_tabWidget->addTab(m_shortcutsTab, "Shortcuts");
}

void UIEditorModule::setupWidgetsTab() {
    m_widgetsTab = new QWidget();
    auto* layout = new QVBoxLayout(m_widgetsTab);

    m_widgetTree = createTreeWidget({"Widget", "Type", "Status"});
    m_widgetTree->addTopLevelItem(new QTreeWidgetItem({"Node Graph Editor", "Editor", "Active"}));
    m_widgetTree->addTopLevelItem(new QTreeWidgetItem({"File Tree", "Browser", "Active"}));
    m_widgetTree->addTopLevelItem(new QTreeWidgetItem({"Asset List", "Browser", "Active"}));
    m_widgetTree->addTopLevelItem(new QTreeWidgetItem({"Terminal", "Utility", "Active"}));
    m_widgetTree->addTopLevelItem(new QTreeWidgetItem({"Property Editor", "Editor", "Active"}));
    m_widgetTree->addTopLevelItem(new QTreeWidgetItem({"Sound Wizard", "Dialog", "Active"}));
    layout->addWidget(m_widgetTree);

    m_openNodeGraphBtn = createButton("Open Node Graph");
    layout->addWidget(m_openNodeGraphBtn);
    layout->addStretch();

    connect(m_openNodeGraphBtn, &QPushButton::clicked, this, &UIEditorModule::onNodeGraphSelected);

    m_tabWidget->addTab(m_widgetsTab, "Widgets");
}

void UIEditorModule::populateThemeList() {}

void UIEditorModule::onThemeSelected(int index) {
    Q_UNUSED(index);
    log(QString("Theme changed to: %1").arg(m_themeCombo->currentText()));
}

void UIEditorModule::onAccentColorChanged() {
    QColor color = QColorDialog::getColor(QColor("#3a5a8a"), this, "Select Accent Color");
    if (color.isValid()) {
        m_accentColorPreview->setStyleSheet(QString("QLabel { background: %1; border: 1px solid #4a6a9a; border-radius: 4px; }").arg(color.name()));
        log(QString("Accent color changed to: %1").arg(color.name()));
    }
}

void UIEditorModule::onFontSizeChanged(int value) {
    log(QString("UI font size set to: %1pt").arg(value));
}

void UIEditorModule::onLayoutSelected(int index) {
    Q_UNUSED(index);
}

void UIEditorModule::onResetLayout() {
    if (confirmAction("Reset Layout", "Reset UI layout to default?")) {
        m_layoutCombo->setCurrentIndex(0);
        logSuccess("Layout reset to default");
    }
}

void UIEditorModule::onSaveLayout() {
    QString name = QInputDialog::getText(this, "Save Layout", "Layout name:", QLineEdit::Normal, m_layoutCombo->currentText());
    if (!name.isEmpty()) {
        if (m_layoutCombo->findText(name) < 0) m_layoutCombo->addItem(name);
        m_layoutCombo->setCurrentText(name);
        logSuccess(QString("Layout saved: %1").arg(name));
    }
}

void UIEditorModule::onLoadLayout() {
    log(QString("Loading layout: %1").arg(m_layoutCombo->currentText()));
}

void UIEditorModule::onNodeGraphSelected() {
    log("Opening Node Graph Editor...");
}

void UIEditorModule::onConfigureShortcuts() {
    log("Opening shortcut configuration...");
}

void UIEditorModule::onToggleAnimations(bool checked) {
    log(QString("UI animations %1").arg(checked ? "enabled" : "disabled"));
}

} // namespace ui
} // namespace ks

#include "UIEditorModule.moc"
