#include "TrackBuilderPage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>

namespace ks { namespace track {

static const QString kPageStyle = R"(
QWidget       { background:#1a1a1a; color:#d4d4d4; }
QToolBar      { background:#252526; border-bottom:1px solid #3c3c3c; spacing:2px; }
QToolButton   { background:transparent; color:#d4d4d4; border:none; padding:6px 10px;
                font-size:20px; }
QToolButton:hover   { background:#3e3e42; border-radius:3px; }
QToolButton:checked { background:#007acc; border-radius:3px; }
QSplitter::handle   { background:#3c3c3c; width:1px; }
)";

// ============================================================================
TrackBuilderPage::TrackBuilderPage(QWidget* parent) : QWidget(parent)
{
    setStyleSheet(kPageStyle);
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);

    // ---- Module (logic, no UI) -------------------------------------------
    m_module = new TrackBuilderModule(this);
    m_module->newProject("New Track"); // start with empty project

    // ---- Top toolbar -------------------------------------------------------
    m_toolbar = new QToolBar();
    m_toolbar->setMovable(false);
    m_toolbar->setIconSize(QSize(24,24));

    // File actions
    auto* actNew  = m_toolbar->addAction(u8"\U0001F4C4", this, &TrackBuilderPage::onNewProject);
    auto* actOpen = m_toolbar->addAction(u8"\U0001F4C2", this, &TrackBuilderPage::onOpenProject);
    auto* actSave = m_toolbar->addAction(u8"\U0001F4BE", this, &TrackBuilderPage::onSaveProject);
    actNew->setToolTip("New Track Project");
    actOpen->setToolTip("Open Track Project (.kstb)");
    actSave->setToolTip("Save Track Project");

    m_toolbar->addSeparator();

    // Edit mode actions (exclusive)
    auto* modeGroup = new QActionGroup(this);
    modeGroup->setExclusive(true);

    makeToolAction(modeGroup, u8"\U0001F5B1",  "Navigate",       "navigate",   true);
    m_toolbar->addSeparator();
    makeToolAction(modeGroup, u8"\u2191",       "Raise Terrain",  "raise");
    makeToolAction(modeGroup, u8"\u2193",       "Lower Terrain",  "lower");
    makeToolAction(modeGroup, u8"\u223C",       "Smooth Terrain", "smooth");
    makeToolAction(modeGroup, u8"=",            "Flatten",        "flatten");
    makeToolAction(modeGroup, u8"\u2248",       "Noise",          "noise");
    makeToolAction(modeGroup, u8"\U0001F4A7",   "Hydraulic Erode","erosion");
    m_toolbar->addSeparator();
    makeToolAction(modeGroup, u8"\U0001F6E3",   "Place Road Point","road");
    makeToolAction(modeGroup, u8"\u2588",       "Place Wall Point","wall");
    makeToolAction(modeGroup, u8"\U0001F333",   "Place Prop",     "prop");
    makeToolAction(modeGroup, u8"\U0001F3C1",   "Place Start",    "start");
    makeToolAction(modeGroup, u8"\U0001F528",   "Place Pit Box",  "pit");
    m_toolbar->addSeparator();

    connect(modeGroup, &QActionGroup::triggered,
            this, &TrackBuilderPage::onToolChanged);

    // Export button (right-aligned)
    auto* spacer = new QWidget(); spacer->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    m_toolbar->addWidget(spacer);
    auto* actExport = m_toolbar->addAction(u8"\U0001F4E4  Export to AC",
                                            this, &TrackBuilderPage::onExport);
    actExport->setToolTip("Export track to Assetto Corsa format");

    mainLayout->addWidget(m_toolbar);

    // ---- Splitter: viewport | panel ----------------------------------------
    auto* splitter = new QSplitter(Qt::Horizontal);

    m_viewport = new TrackViewport();
    m_viewport->setModule(m_module);
    m_viewport->setMinimumWidth(400);
    splitter->addWidget(m_viewport);

    m_panel = new TrackBuilderWidget();
    m_panel->setModule(m_module);
    m_panel->setFixedWidth(300);
    splitter->addWidget(m_panel);

    splitter->setStretchFactor(0,3);  // viewport gets 3/4
    splitter->setStretchFactor(1,1);
    mainLayout->addWidget(splitter,1);

    // Wire panel signals
    connect(m_panel, &TrackBuilderWidget::requestNewProject,  this, &TrackBuilderPage::onNewProject);
    connect(m_panel, &TrackBuilderWidget::requestOpenProject, this, &TrackBuilderPage::onOpenProject);
    connect(m_panel, &TrackBuilderWidget::requestSaveProject, this, &TrackBuilderPage::onSaveProject);
    connect(m_panel, &TrackBuilderWidget::requestExport,      this, &TrackBuilderPage::onExport);

    // Wire viewport signals back to module
    connect(m_viewport, &TrackViewport::roadPointPlaced,
            [this](float x,float y,float z){
                QStringList ids=m_module->roadIds();
                if (ids.isEmpty()) ids<<m_module->addRoad("Road 1");
                m_module->addRoadPoint(ids.last(),x,y,z);
            });
    connect(m_viewport, &TrackViewport::propPlaced,
            [this](float x,float y,float z){
                m_module->addProp("generic_prop",x,y,z,0.f,"Prop");
            });
}

TrackBuilderPage::~TrackBuilderPage() {}

// ============================================================================
QAction* TrackBuilderPage::makeToolAction(QActionGroup* grp, const QString& icon,
                                           const QString& tip, const QString& mode,
                                           bool checked)
{
    auto* act = m_toolbar->addAction(icon);
    act->setToolTip(tip);
    act->setCheckable(true);
    act->setChecked(checked);
    act->setData(mode);
    grp->addAction(act);
    return act;
}

// ============================================================================
void TrackBuilderPage::onToolChanged(QAction* action)
{
    QString mode = action->data().toString();
    m_module->setActiveTool(mode);
    m_viewport->setEditMode(mode);

    // Set terrain brush mode if applicable
    if (mode=="raise"||mode=="lower"||mode=="smooth"||
        mode=="flatten"||mode=="noise"||mode=="erosion")
        m_module->setTerrainBrushMode(mode);
}

void TrackBuilderPage::onNewProject()
{
    if (m_module->isDirty()) {
        auto btn=QMessageBox::question(this,"Unsaved Changes",
            "Save current project before creating a new one?",
            QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel);
        if (btn==QMessageBox::Cancel) return;
        if (btn==QMessageBox::Save) onSaveProject();
    }
    m_module->newProject("New Track");
    m_viewport->resetCamera();
}

void TrackBuilderPage::onOpenProject()
{
    QString path=QFileDialog::getOpenFileName(this,"Open Track Project",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "ksTrackBuilder (*.kstb);;JSON (*.json)");
    if (path.isEmpty()) return;
    m_module->loadProject(path);
    m_viewport->focusOnTerrain();
}

void TrackBuilderPage::onSaveProject()
{
    QString path=QFileDialog::getSaveFileName(this,"Save Track Project",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "ksTrackBuilder (*.kstb);;JSON (*.json)");
    if (path.isEmpty()) return;
    if (!path.contains('.')) path+=".kstb";
    m_module->saveProject(path);
}

void TrackBuilderPage::onExport()
{
    // Validate first
    QStringList msgs=m_module->exportValidationMessages();
    if (msgs.first()!="OK") {
        QString warning="Export warnings:\n\n"+msgs.join("\n")+"\n\nContinue anyway?";
        if (QMessageBox::question(this,"Validation",warning,
            QMessageBox::Yes|QMessageBox::No)==QMessageBox::No) return;
    }
    QString dir=QFileDialog::getExistingDirectory(this,"Export Directory",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    if (dir.isEmpty()) return;
    m_module->exportToAC(dir, true);
}

}} // namespace ks::track
