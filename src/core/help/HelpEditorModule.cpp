#include "HelpEditorModule.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QTreeWidgetItem>
#include <QFileDialog>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QMenu>

namespace ks {
namespace help {

HelpEditorModule::HelpEditorModule(QWidget* parent)
    : ModuleGuiBase(parent)
    , m_tabWidget(nullptr)
    , m_browserTab(nullptr)
    , m_topicTree(nullptr)
    , m_contentBrowser(nullptr)
    , m_backBtn(nullptr)
    , m_forwardBtn(nullptr)
    , m_externalBtn(nullptr)
    , m_topicTitleLabel(nullptr)
    , m_searchTab(nullptr)
    , m_searchInput(nullptr)
    , m_searchResults(nullptr)
    , m_sectionCombo(nullptr)
    , m_historyIndex(-1)
{
    setObjectName("HelpEditorModule");
}

bool HelpEditorModule::initialize() {
    if (m_uiBuilt) return true;
    ModuleGuiBase::initialize();
    return true;
}

void HelpEditorModule::shutdown() {
    m_uiBuilt = false;
}

void HelpEditorModule::onActivation() {}
void HelpEditorModule::onDeactivation() {}

void HelpEditorModule::buildUI() {
    m_tabWidget = new QTabWidget();

    setupBrowserTab();
    setupSearchTab();

    m_mainLayout->addWidget(m_tabWidget);
    m_mainLayout->addWidget(m_logOutput);
}

void HelpEditorModule::setupBrowserTab() {
    m_browserTab = new QWidget();
    auto* layout = new QVBoxLayout(m_browserTab);

    auto* navBar = new QHBoxLayout();
    m_backBtn = createButton("< Back");
    m_forwardBtn = createButton("Forward >");
    m_externalBtn = createButton("Open in Browser");
    navBar->addWidget(m_backBtn);
    navBar->addWidget(m_forwardBtn);
    navBar->addStretch();
    navBar->addWidget(m_externalBtn);
    layout->addLayout(navBar);

    m_topicTitleLabel = createLabel("Welcome to ksEditor Help");
    m_topicTitleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #ffffff; padding: 8px;");
    layout->addWidget(m_topicTitleLabel);

    auto* splitter = createSplitter(Qt::Horizontal);
    m_topicTree = createTreeWidget({"Topics"});
    splitter->addWidget(m_topicTree);

    m_contentBrowser = new QTextBrowser();
    m_contentBrowser->setOpenExternalLinks(true);
    m_contentBrowser->setStyleSheet(
        "QTextBrowser { background: #1e1e1e; color: #dddddd; border: 1px solid #3a3a3a; padding: 12px; font-size: 12px; }"
    );
    m_contentBrowser->setHtml(
        "<h2>Welcome to ksEditor</h2>"
        "<p>Select a topic from the tree to view documentation.</p>"
        "<h3>Quick Start</h3>"
        "<ul>"
        "<li>Use <b>File &gt; New Project</b> to create a new mod project</li>"
        "<li>Use the <b>Module Manager</b> to enable/disable editor modules</li>"
        "<li>Each module has its own toolbar and tab-based interface</li>"
        "</ul>"
    );
    splitter->addWidget(m_contentBrowser);

    layout->addWidget(splitter);

    connect(m_backBtn, &QPushButton::clicked, this, &HelpEditorModule::onNavigateBack);
    connect(m_forwardBtn, &QPushButton::clicked, this, &HelpEditorModule::onNavigateForward);
    connect(m_externalBtn, &QPushButton::clicked, this, &HelpEditorModule::onOpenExternalDocs);
    connect(m_topicTree, &QTreeWidget::itemClicked, this, &HelpEditorModule::onTopicSelected);

    populateTopicTree();
    m_tabWidget->addTab(m_browserTab, "Documentation");
}

void HelpEditorModule::setupSearchTab() {
    m_searchTab = new QWidget();
    auto* layout = new QVBoxLayout(m_searchTab);

    auto* searchLayout = new QHBoxLayout();
    m_searchInput = new QLineEdit();
    m_searchInput->setPlaceholderText("Search documentation...");
    m_sectionCombo = createComboBox({"All Sections", "Getting Started", "Modules", "File Formats", "Tutorials", "API Reference"});
    searchLayout->addWidget(m_searchInput);
    searchLayout->addWidget(m_sectionCombo);
    layout->addLayout(searchLayout);

    m_searchResults = createTreeWidget({"Topic", "Section", "Relevance"});
    layout->addWidget(m_searchResults);

    layout->addStretch();

    connect(m_searchInput, &QLineEdit::textChanged, this, &HelpEditorModule::onSearchTextChanged);

    m_tabWidget->addTab(m_searchTab, "Search");
}

void HelpEditorModule::populateTopicTree() {
    m_topicTree->clear();

    auto* gettingStarted = new QTreeWidgetItem(m_topicTree, {"Getting Started"});
    gettingStarted->addChild(new QTreeWidgetItem({"Installation Guide"}));
    gettingStarted->addChild(new QTreeWidgetItem({"Quick Start Tutorial"}));
    gettingStarted->addChild(new QTreeWidgetItem({"User Interface Overview"}));
    gettingStarted->addChild(new QTreeWidgetItem({"Project Management"}));

    auto* modules = new QTreeWidgetItem(m_topicTree, {"Modules"});
    modules->addChild(new QTreeWidgetItem({"Asset Manager"}));
    modules->addChild(new QTreeWidgetItem({"Audio Editor"}));
    modules->addChild(new QTreeWidgetItem({"Graphics Viewport"}));
    modules->addChild(new QTreeWidgetItem({"Physics Editor"}));
    modules->addChild(new QTreeWidgetItem({"Livery Editor"}));
    modules->addChild(new QTreeWidgetItem({"Mod Manager"}));
    modules->addChild(new QTreeWidgetItem({"File Format Converter"}));
    modules->addChild(new QTreeWidgetItem({"3D Modeling"}));

    auto* fileFormats = new QTreeWidgetItem(m_topicTree, {"File Formats"});
    fileFormats->addChild(new QTreeWidgetItem({"glTF/GLB Import/Export"}));
    fileFormats->addChild(new QTreeWidgetItem({"FBX Format Support"}));
    fileFormats->addChild(new QTreeWidgetItem({"KN5 Format"}));
    fileFormats->addChild(new QTreeWidgetItem({"OBJ/STL/PLY"}));

    auto* tutorials = new QTreeWidgetItem(m_topicTree, {"Tutorials"});
    tutorials->addChild(new QTreeWidgetItem({"Creating a New Car Mod"}));
    tutorials->addChild(new QTreeWidgetItem({"Designing a Livery"}));
    tutorials->addChild(new QTreeWidgetItem({"Setting Up Physics"}));
    tutorials->addChild(new QTreeWidgetItem({"Exporting to Game"}));

    auto* reference = new QTreeWidgetItem(m_topicTree, {"API Reference"});
    reference->addChild(new QTreeWidgetItem({"EditorModule Base Class"}));
    reference->addChild(new QTreeWidgetItem({"Module System"}));
    reference->addChild(new QTreeWidgetItem({"File Format API"}));
    reference->addChild(new QTreeWidgetItem({"Scripting (Python/Lua)"}));

    m_topicTree->expandAll();
}

void HelpEditorModule::onTopicSelected(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    if (!item) return;

    QString topic = item->text(0);
    m_topicTitleLabel->setText(topic);
    m_contentBrowser->setHtml(QString(
        "<h2>%1</h2>"
        "<p>Documentation content for <b>%1</b> would be displayed here.</p>"
        "<p>This is a placeholder. In the full implementation, this would load "
        "rich documentation with code examples, screenshots, and cross-references.</p>"
        "<h3>Related Topics</h3>"
        "<ul>"
        "<li>See the <i>User Guide</i> for detailed walkthroughs</li>"
        "<li>Check the <i>API Reference</i> for programming interfaces</li>"
        "<li>Visit our <a href='https://kseditor.readthedocs.io'>online documentation</a></li>"
        "</ul>"
    ).arg(topic));

    m_history = m_history.mid(0, m_historyIndex + 1);
    m_history.append(topic);
    m_historyIndex = m_history.size() - 1;
}

void HelpEditorModule::onSearchTextChanged(const QString& text) {
    m_searchResults->clear();
    if (text.isEmpty()) return;

    auto* result = new QTreeWidgetItem(m_searchResults, {QString("Results for: %1").arg(text), "Search", "100%"});
    result->addChild(new QTreeWidgetItem({"Module Overview", "Getting Started", "85%"}));
    result->addChild(new QTreeWidgetItem({"File Format Reference", "File Formats", "70%"}));
    m_searchResults->expandAll();
}

void HelpEditorModule::onNavigateBack() {
    if (m_historyIndex > 0) {
        m_historyIndex--;
        QString topic = m_history[m_historyIndex];
        m_topicTitleLabel->setText(topic);
        log(QString("Navigated back to: %1").arg(topic));
    }
}

void HelpEditorModule::onNavigateForward() {
    if (m_historyIndex < m_history.size() - 1) {
        m_historyIndex++;
        QString topic = m_history[m_historyIndex];
        m_topicTitleLabel->setText(topic);
        log(QString("Navigated forward to: %1").arg(topic));
    }
}

void HelpEditorModule::onOpenExternalDocs() {
    QDesktopServices::openUrl(QUrl("https://kseditor.readthedocs.io"));
    log("Opening online documentation");
}

} // namespace help
} // namespace ks

#include "HelpEditorModule.moc"
