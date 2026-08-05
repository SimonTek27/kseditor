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
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QApplication>

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

static QString topicContent(const QString& topic) {
    static const QMap<QString, QString> docs = {
        {"Welcome", R"(<h2>Welcome to ksEditor</h2>
<p>ksEditor is a modding toolset for Assetto Corsa, Assetto Corsa Competizione, and Assetto Corsa EVO.</p>
<h3>Key Features</h3>
<ul>
<li>3D mesh viewer and editor (KN5, FBX, glTF, OBJ, STL, PLY)</li>
<li>Livery editor with vector-based design canvas</li>
<li>Audio editor with multi-track recording, effects, and sound bank generation</li>
<li>Full node-based material editor</li>
<li>CSP (Custom Shaders Patch) config editor</li>
<li>File format converter (KN5, FBX, glTF, JSON, INI, CSV)</li>
<li>Steam Workshop browser and download manager</li>
<li>Mod manager with conflict resolution and dependency tracking</li>
<li>AC shared memory telemetry viewer</li>
<li>VR preview (OpenXR + Vulkan)</li>
<li>Network collaboration via WebSocket</li>
<li>Cloud sync for assets</li>
</ul>
<p>Use the topic tree on the left to browse documentation.</p>)"},
        {"Installation Guide", R"(<h2>Installation Guide</h2>
<h3>System Requirements</h3>
<ul>
<li>Windows 10/11 64-bit</li>
<li>Assetto Corsa (Steam)</li>
<li>Custom Shaders Patch (CSP) recommended</li>
<li>Graphics card with OpenGL 4.3+ support</li>
</ul>
<h3>Installation Steps</h3>
<ol>
<li>Download the latest release from the repository</li>
<li>Extract to a folder of your choice</li>
<li>Run <code>ksEditor.exe</code></li>
<li>The editor will auto-detect your AC installation path</li>
<li>Optionally configure cloud sync and CSP integration in Settings</li>
</ol>
<h3>First Launch</h3>
<p>On first launch, ksEditor scans for installed Assetto Corsa paths.
If not found automatically, set the path manually in <b>Settings &gt; AC Path</b>.</p>)"},
        {"Quick Start Tutorial", R"(<h2>Quick Start Tutorial</h2>
<h3>Step 1: Open a KN5 Model</h3>
<p>Go to <b>File &gt; Open</b> and select a <code>.kn5</code> file from your AC content folder.</p>
<h3>Step 2: Browse Materials</h3>
<p>Switch to the <b>Material Editor</b> tab to view and edit shader properties.
You can modify texture maps, adjust colours, and tweak shader parameters.</p>
<h3>Step 3: Convert Format</h3>
<p>Use <b>Format Tools &gt; Single Convert</b> to convert between KN5, FBX, glTF, and other formats.</p>
<h3>Step 4: Export</h3>
<p>Export your modified model using <b>File &gt; Export</b> or the format tools panel.</p>
<p>See the Tutorials section for more detailed walkthroughs.</p>)"},
        {"User Interface Overview", R"(<h2>User Interface Overview</h2>
<h3>Main Window</h3>
<ul>
<li><b>Menu Bar</b>: File, Edit, View, Tools, Help</li>
<li><b>Module Tabs</b>: Switch between editor modules (Mesh, Material, Audio, etc.)</li>
<li><b>Dock Widgets</b>: Context-sensitive panels for the active module</li>
<li><b>Status Bar</b>: Shows module status, sync state, and log messages</li>
</ul>
<h3>Module System</h3>
<p>Each editor feature is a pluggable module with its own UI, toolbars,
and dock widgets. Modules can be enabled/disabled via the Module Manager.</p>
<h3>QML Integration</h3>
<p>ksEditor uses Qt Quick (QML) for many UI components. QML bridges
provide seamless communication between C++ and QML layers.</p>)"},
        {"Project Management", R"(<h2>Project Management</h2>
<h3>Creating a Project</h3>
<p>Use <b>File &gt; New Project</b> to create a structured mod project.
Projects bundle assets, metadata, and configuration files.</p>
<h3>Project Structure</h3>
<pre>
project/
  content/
    cars/
    tracks/
    skins/
  data/
    manifest.json
    preview.png
</pre>
<h3>Version Control</h3>
<p>ksEditor integrates with Git via the VCS module. You can commit,
push, pull, and view history directly from the editor.</p>)"},
        {"3D Modeling", R"(<h2>3D Modeling</h2>
<h3>Mesh Editor</h3>
<p>The mesh editor provides tools for:</p>
<ul>
<li><b>Boolean Operations</b>: Union, Intersection, Difference</li>
<li><b>UV Unwrapping</b>: 6 methods including planar, spherical, cylindrical, and LSCM</li>
<li><b>Sculpting</b>: 13 brush types with remesh, decimation, and smoothing</li>
<li><b>Rigging</b>: Bone weight painting for skinned meshes</li>
<li><b>LOD Generation</b>: Automatic LOD creation with configurable reduction</li>
</ul>
<h3>Import/Export</h3>
<p>Supports: KN5, FBX, OBJ, glTF, GLB, STL, PLY, DAE</p>)"},
        {"Audio Editor", R"(<h2>Audio Editor</h2>
<h3>Features</h3>
<ul>
<li>Multi-track recording studio</li>
<li>15 effect processors (EQ, reverb, delay, chorus, distortion, etc.)</li>
<li>Pitch shifting and formant filtering</li>
<li>Time stretching</li>
<li>Sound bank generation for AC car audio</li>
<li>Import: WAV, MP3, OGG, FLAC, AIFF, M4A</li>
<li>Export: WAV, OGG, MP3, BANK</li>
</ul>
<h3>Car Audio Engine</h3>
<p>Configure engine sound profiles with RPM-based samples,
transient processing, and dynamic mixing for Assetto Corsa.</p>)"},
        {"Graphics Viewport", R"(<h2>Graphics Viewport</h2>
<h3>Render Pipeline</h3>
<ul>
<li>7 configurable render passes</li>
<li>Deferred and forward rendering paths</li>
<li>PBR material system (Cook-Torrance BRDF)</li>
<li>Dynamic lighting with shadow mapping</li>
<li>Post-processing: Bloom, SSAO, SSR, DoF, Tone Mapping</li>
</ul>
<h3>Color Grading</h3>
<p>Full LUT-based color grading with:</p>
<ul>
<li>3D LUT generation (8-64 cube size)</li>
<li>.cube file import/export</li>
<li>Tone curves (Master, R, G, B channels)</li>
<li>Lift/Gamma/Gain per channel</li>
<li>Color temperature, vibrance, split-toning</li>
<li>Film grain and sharpness</li>
<li>5 preset styles</li>
</ul>
<h3>Shader Editor</h3>
<p>Edit GLSL, HLSL, SPIR-V, and Metal shaders in real-time.</p>)"},
        {"Physics Editor", R"(<h2>Physics Editor</h2>
<h3>AC Car Physics</h3>
<ul>
<li>Edit car setup files (aero, drivetrain, suspension, tyres)</li>
<li>Compare setups side-by-side with 25+ field diff view</li>
<li>Power and torque curve visualization</li>
<li>AI line editor</li>
</ul>
<h3>Collision Meshes</h3>
<p>Generate and edit collision geometry for AC car and track models.</p>)"},
        {"Livery Editor", R"(<h2>Livery Editor</h2>
<h3>Vector Design Canvas</h3>
<ul>
<li>Full vector-based painting with layers</li>
<li>Pen tool, shape tools, text tool</li>
<li>Layer management with grouping and blending</li>
<li>Export directly to AC DDS format</li>
</ul>
<h3>Templates</h3>
<p>Start from pre-made livery templates for popular AC cars.</p>)"},
        {"Mod Manager", R"(<h2>Mod Manager</h2>
<h3>Features</h3>
<ul>
<li>List all installed mods with version info</li>
<li>Enable/disable mods individually</li>
<li>Conflict detection and dependency resolution</li>
<li>Content verification and repair</li>
<li>Profile management (save/load mod sets)</li>
<li>Import from <code>.ksmod</code> and <code>.zip</code> packages</li>
</ul>
<h3>Dependency Resolution</h3>
<p>The mod manager reads manifest files to check required dependencies
and conflicting mods before installation.</p>)"},
        {"File Format Converter", R"(<h2>File Format Converter</h2>
<h3>Supported Conversions</h3>
<ul>
<li>KN5 &lt;--&gt; FBX</li>
<li>KN5 &lt;--&gt; glTF</li>
<li>JSON &lt;--&gt; INI</li>
<li>CSV &lt;--&gt; AI Line files</li>
</ul>
<h3>Batch Conversion</h3>
<p>Convert multiple files at once with progress tracking.</p>
<h3>KN5 Extraction</h3>
<p>Extract textures and materials from KN5 archives.</p>
<h3>Validation</h3>
<p>Validate JSON, INI, and Lua files with detailed error reporting.</p>)"},
        {"glTF/GLB Import/Export", R"(<h2>glTF/GLB Import/Export</h2>
<h3>Import</h3>
<ul>
<li>Load glTF 2.0 files (.gltf, .glb)</li>
<li>Mesh data with vertex positions, normals, UVs, tangents</li>
<li>Material system with PBR metallic/roughness workflow</li>
<li>Texture references</li>
<li>Animation data (skinning, bone hierarchies)</li>
</ul>
<h3>Export</h3>
<ul>
<li>Export meshes to glTF 2.0 binary (.glb) or JSON (.gltf)</li>
<li>Embedded or separate buffer files</li>
<li>Material conversion from KN5 to glTF PBR</li>
</ul>)"},
        {"FBX Format Support", R"(<h2>FBX Format Support</h2>
<h3>FBX Export (ASCII)</h3>
<p>The FBX exporter writes text-format FBX files compatible with
Blender, Maya, 3ds Max, and other DCC tools.</p>
<ul>
<li>Mesh geometry with normals, UVs, tangents</li>
<li>Hierarchical node structure</li>
<li>Basic material properties</li>
<li>ASCII FBX 2020 format</li>
</ul>)"},
        {"KN5 Format", R"(<h2>KN5 Format</h2>
<h3>Reading</h3>
<p>Full parser for KN5 version 5 files:</p>
<ul>
<li>Mesh data with vertex positions, normals, UVs, tangents, bone data</li>
<li>Material definitions with shader properties and texture mappings</li>
<li>Embedded textures in DDS format</li>
<li>LOD groups (A/B/C/D)</li>
<li>Bone hierarchy for skinned meshes</li>
<li>World transformation matrices</li>
</ul>
<h3>Writing</h3>
<p>Full writer support for creating and modifying KN5 files.</p>
<h3>CSP Decryption</h3>
<p>Detect and decrypt CSP-encrypted KN5 files using the
<code>__AC_SHADERS_PATCH_KN5ENC_v1__</code> envelope format.</p>)"},
        {"OBJ/STL/PLY", R"(<h2>OBJ/STL/PLY Support</h2>
<ul>
<li><b>OBJ</b>: Wavefront OBJ with MTL material references</li>
<li><b>STL</b>: Binary and ASCII stereolithography format</li>
<li><b>PLY</b>: Stanford PLY with vertex colours and normals</li>
</ul>
<p>All three formats support both import and export.</p>)"},
        {"Creating a New Car Mod", R"(<h2>Creating a New Car Mod</h2>
<h3>Prerequisites</h3>
<ul>
<li>A 3D model in FBX, OBJ, or glTF format</li>
<li>Texture maps (albedo, normal, AO, etc.)</li>
<li>AC car setup data (physics, audio)</li>
</ul>
<h3>Steps</h3>
<ol>
<li>Create a new project: <b>File &gt; New Project &gt; Car Mod</b></li>
<li>Import your 3D model via <b>File &gt; Import</b></li>
<li>Set up materials in the Material Editor</li>
<li>Configure physics in the car setup editor</li>
<li>Add engine audio in the Audio Editor</li>
<li>Export to KN5: <b>Format Tools &gt; Convert to KN5</b></li>
<li>Test in Assetto Corsa</li>
</ol>)"},
        {"Designing a Livery", R"(<h2>Designing a Livery</h2>
<ol>
<li>Open the Livery Editor from the modules toolbar</li>
<li>Select a template or start with a blank canvas</li>
<li>Use the vector tools to draw shapes and text</li>
<li>Add layers for sponsors, numbers, and trim</li>
<li>Preview on the 3D model</li>
<li>Export to DDS format for AC</li>
</ol>)"},
        {"Setting Up Physics", R"(<h2>Setting Up Physics</h2>
<p>Use the car setup editor to configure:</p>
<ul>
<li><b>Drivetrain</b>: Engine power curve, gear ratios, differential</li>
<li><b>Suspension</b>: Springs, dampers, anti-roll bars, geometry</li>
<li><b>Tyres</b>: Compound model, pressure, temperature ranges</li>
<li><b>Aerodynamics</b>: Downforce, drag, ride height sensitivity</li>
<li><b>Electronics</b>: TC, ABS, ECU maps</li>
</ul>
<p>Compare your setup side-by-side with reference setups.</p>)"},
        {"Exporting to Game", R"(<h2>Exporting to Game</h2>
<h3>Assetto Corsa</h3>
<ul>
<li>Export finished mods to KN5 format</li>
<li>Install via Mod Manager or manual copy to content/</li>
<li>Launch AC directly from ksEditor to test</li>
</ul>
<h3>Format Pipeline</h3>
<pre>
FBX/OBJ/glTF â†’ ksEditor â†’ KN5 â†’ Assetto Corsa
                        â†’ FBX â†’ Blender/Maya (further editing)
                        â†’ glTF â†’ Web/Unreal/Unity
</pre>)"},
        {"EditorModule Base Class", R"(<h2>EditorModule Base Class</h2>
<p>All editor features inherit from <code>EditorModule</code> or <code>ModuleGuiBase</code>.</p>
<h3>Key Methods</h3>
<pre>
virtual bool initialize()
virtual void shutdown()
virtual QString moduleName() const
virtual QString moduleId() const
virtual int getModulePriority() const
virtual QDockWidget* getOrCreateDockWidget(QMainWindow*)
</pre>
<h3>Lifecycle</h3>
<ol>
<li>Constructor: set up data structures</li>
<li>initialize(): register UI, connect signals</li>
<li>Activation: module becomes active</li>
<li>Deactivation: module releases resources</li>
<li>shutdown(): final cleanup</li>
</ol>)"},
        {"Module System", R"(<h2>Module System</h2>
<h3>Architecture</h3>
<p>ksEditor uses a plugin-based module architecture. Each module is
a self-contained feature with its own UI, state, and dependencies.</p>
<h3>Registration</h3>
<p>Modules register themselves with the ModuleManager, which handles
loading order, dependency resolution, and lifecycle management.</p>
<h3>AC1 Plugin</h3>
<p>The <code>ksAssettoCorsa.dll</code> plugin provides AC-specific features:</p>
<ul>
<li>Shared memory telemetry reader</li>
<li>Content browser for installed cars/tracks</li>
<li>CSP config editor</li>
<li>Workshop module with Steam integration</li>
<li>Assets library with SQLite database</li>
<li>KN5 parser, writer, and decrypt</li>
</ul>)"},
        {"File Format API", R"(<h2>File Format API</h2>
<p>ksEditor's file format layer provides unified read/write access
to over 50 different formats.</p>
<h3>Core Interfaces</h3>
<ul>
<li>FileFormatPlugin: base class for format handlers</li>
<li>MeshFormat: 3D mesh import/export</li>
<li>ImageFormat: texture import/export</li>
<li>AudioFormat: audio file import/export</li>
</ul>
<h3>Registered Formats</h3>
<p>Mesh: KN5, FBX, OBJ, glTF, GLB, STL, PLY, DAE, 3DS, X, MD5, MD3, MD2, SMD, IQM, BVH</p>
<p>Image: DDS, PNG, JPG, BMP, TGA, TIFF, HDR, EXR, PSD, WEBP, QOI</p>
<p>Audio: WAV, MP3, OGG, FLAC, AIFF, M4A, MIDI, MOD, S3M, XM, IT</p>)"},
        {"Scripting (Python/Lua)", R"(<h2>Scripting (Python/Lua)</h2>
<h3>Python Scripting</h3>
<p>ksEditor supports Python scripting for automation:</p>
<ul>
<li>Access the active scene and selection</li>
<li>Run batch conversion pipelines</li>
<li>Create custom export presets</li>
<li>Automate material adjustments</li>
</ul>
<h3>Lua Scripting</h3>
<p>Lua scripts can be used for CSP-style configuration and simple
automation tasks within the editor.</p>)"},
    };
    return docs.value(topic, QString());
}

void HelpEditorModule::populateTopicTree() {
    m_topicTree->clear();

    auto* gettingStarted = new QTreeWidgetItem(m_topicTree, {"Getting Started"});
    gettingStarted->addChild(new QTreeWidgetItem({"Installation Guide"}));
    gettingStarted->addChild(new QTreeWidgetItem({"Quick Start Tutorial"}));
    gettingStarted->addChild(new QTreeWidgetItem({"User Interface Overview"}));
    gettingStarted->addChild(new QTreeWidgetItem({"Project Management"}));

    auto* modules = new QTreeWidgetItem(m_topicTree, {"Modules"});
    modules->addChild(new QTreeWidgetItem({"3D Modeling"}));
    modules->addChild(new QTreeWidgetItem({"Audio Editor"}));
    modules->addChild(new QTreeWidgetItem({"Graphics Viewport"}));
    modules->addChild(new QTreeWidgetItem({"Physics Editor"}));
    modules->addChild(new QTreeWidgetItem({"Livery Editor"}));
    modules->addChild(new QTreeWidgetItem({"Mod Manager"}));
    modules->addChild(new QTreeWidgetItem({"File Format Converter"}));

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

    QString content = topicContent(topic);
    if (content.isEmpty()) {
        content = topicContent("Welcome");
    }
    m_contentBrowser->setHtml(content);

    m_history = m_history.mid(0, m_historyIndex + 1);
    m_history.append(topic);
    m_historyIndex = m_history.size() - 1;
}

void HelpEditorModule::onSearchTextChanged(const QString& text) {
    m_searchResults->clear();
    if (text.isEmpty() || text.length() < 2) return;

    QString lower = text.toLower();
    QStringList allTopics = {"Welcome", "Installation Guide", "Quick Start Tutorial",
        "User Interface Overview", "Project Management", "3D Modeling",
        "Audio Editor", "Graphics Viewport", "Physics Editor", "Livery Editor",
        "Mod Manager", "File Format Converter", "glTF/GLB Import/Export",
        "FBX Format Support", "KN5 Format", "OBJ/STL/PLY",
        "Creating a New Car Mod", "Designing a Livery", "Setting Up Physics",
        "Exporting to Game", "EditorModule Base Class", "Module System",
        "File Format API", "Scripting (Python/Lua)"};

    for (const QString& topic : allTopics) {
        if (topic.contains(lower, Qt::CaseInsensitive)) {
            QString section = "General";
            if (topic == "Installation Guide" || topic == "Quick Start Tutorial" ||
                topic == "User Interface Overview" || topic == "Project Management")
                section = "Getting Started";
            else if (topic == "3D Modeling" || topic == "Audio Editor" || topic == "Graphics Viewport" ||
                     topic == "Physics Editor" || topic == "Livery Editor" || topic == "Mod Manager" ||
                     topic == "File Format Converter")
                section = "Modules";
            else if (topic == "glTF/GLB Import/Export" || topic == "FBX Format Support" ||
                     topic == "KN5 Format" || topic == "OBJ/STL/PLY")
                section = "File Formats";
            else if (topic == "Creating a New Car Mod" || topic == "Designing a Livery" ||
                     topic == "Setting Up Physics" || topic == "Exporting to Game")
                section = "Tutorials";
            else if (topic == "EditorModule Base Class" || topic == "Module System" ||
                     topic == "File Format API" || topic == "Scripting (Python/Lua)")
                section = "API Reference";

            auto* item = new QTreeWidgetItem(m_searchResults, {topic, section, "100%"});
            item->setData(0, Qt::UserRole, topic);
        }
    }

    if (m_searchResults->topLevelItemCount() == 0) {
        new QTreeWidgetItem(m_searchResults, {"No results found", "", ""});
    }

    m_searchResults->expandAll();
    m_searchResults->setSortingEnabled(true);
    m_searchResults->sortByColumn(0, Qt::AscendingOrder);
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

