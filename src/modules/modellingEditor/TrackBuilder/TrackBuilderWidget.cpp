#include "TrackBuilderWidget.h"
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QStandardPaths>

namespace ks { namespace track {

static const QString kStyle = R"(
QWidget          { background:#1e1e1e; color:#d4d4d4; font-size:12px; }
QTabWidget::pane { border:1px solid #3c3c3c; }
QTabBar::tab     { background:#2d2d30; color:#ccc; padding:6px 12px; border:none; }
QTabBar::tab:selected { background:#007acc; color:#fff; }
QGroupBox        { border:1px solid #3c3c3c; border-radius:4px; margin-top:8px; padding:6px; }
QGroupBox::title { color:#9cdcfe; subcontrol-origin:margin; left:8px; }
QPushButton      { background:#3e3e42; color:#d4d4d4; border:none; padding:5px 10px; border-radius:3px; }
QPushButton:hover{ background:#007acc; }
QPushButton:pressed{ background:#005a9e; }
QToolButton      { background:#3e3e42; color:#d4d4d4; border:none; padding:5px; border-radius:3px; min-width:32px; min-height:32px; }
QToolButton:hover{ background:#007acc; }
QToolButton:checked{ background:#005a9e; }
QSlider::groove:horizontal { height:4px; background:#3c3c3c; border-radius:2px; }
QSlider::handle:horizontal { background:#007acc; width:14px; height:14px; margin:-5px 0; border-radius:7px; }
QComboBox        { background:#3e3e42; border:1px solid #555; padding:3px 6px; border-radius:3px; }
QLineEdit        { background:#3e3e42; border:1px solid #555; padding:3px 6px; border-radius:3px; }
QListWidget      { background:#252526; border:1px solid #3c3c3c; }
QListWidget::item:selected{ background:#094771; }
QCheckBox::indicator { width:14px; height:14px; border:1px solid #555; background:#3e3e42; }
QCheckBox::indicator:checked { background:#007acc; }
QProgressBar     { background:#3c3c3c; border:none; border-radius:3px; height:8px; }
QProgressBar::chunk { background:#007acc; border-radius:3px; }
QDoubleSpinBox   { background:#3e3e42; border:1px solid #555; padding:3px; border-radius:3px; }
)";

// ============================================================================
TrackBuilderWidget::TrackBuilderWidget(QWidget* parent) : QWidget(parent)
{
    setStyleSheet(kStyle);
    setMinimumWidth(280);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4,4,4,4);
    mainLayout->setSpacing(4);

    // ---- Top bar (project name + save/open/new) ---------------------------
    auto* topBar = new QWidget();
    auto* topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(0,0,0,0);
    topLayout->setSpacing(3);

    m_trackNameLabel = new QLabel("No project");
    m_trackNameLabel->setStyleSheet("color:#9cdcfe; font-weight:bold; font-size:13px;");
    topLayout->addWidget(m_trackNameLabel,1);

    auto* btnNew  = makeToolBtn(u8"\U0001F4C4","New Track");
    auto* btnOpen = makeToolBtn(u8"\U0001F4C2","Open Track");
    auto* btnSave = makeToolBtn(u8"\U0001F4BE","Save Track");
    connect(btnNew,  &QToolButton::clicked, this, &TrackBuilderWidget::onNewProject);
    connect(btnOpen, &QToolButton::clicked, this, &TrackBuilderWidget::onOpenProject);
    connect(btnSave, &QToolButton::clicked, this, &TrackBuilderWidget::onSaveProject);
    topLayout->addWidget(btnNew);
    topLayout->addWidget(btnOpen);
    topLayout->addWidget(btnSave);

    mainLayout->addWidget(topBar);

    // ---- Status bar -------------------------------------------------------
    m_statusLabel = new QLabel("Ready");
    m_statusLabel->setStyleSheet("color:#888; font-size:11px;");
    mainLayout->addWidget(m_statusLabel);

    // ---- Main tabs --------------------------------------------------------
    m_tabs = new QTabWidget();
    m_tabs->addTab(buildTerrainTab(),  u8"\U0001F3D4 Terrain");
    m_tabs->addTab(buildRoadsTab(),    u8"\U0001F6E3 Roads");
    m_tabs->addTab(buildKerbsTab(),    u8"\u25A3 Kerbs");
    m_tabs->addTab(buildWallsTab(),    u8"\u2588 Walls");
    m_tabs->addTab(buildPropsTab(),    u8"\U0001F333 Props");
    m_tabs->addTab(buildLightsTab(),   u8"\U0001F4A1 Lights");
    m_tabs->addTab(buildStartPitTab(), u8"\U0001F3C1 Start/Pit");
    m_tabs->addTab(buildAILineTab(),   u8"\U0001F916 AI");
    m_tabs->addTab(buildPhysicsTab(),  u8"\u2699 Physics");
    m_tabs->addTab(buildExportTab(),   u8"\U0001F4E4 Export");
    mainLayout->addWidget(m_tabs,1);
}

// ============================================================================
void TrackBuilderWidget::setModule(TrackBuilderModule* module)
{
    m_module = module;
    if (!m_module) return;
    connect(m_module, &TrackBuilderModule::projectChanged,
            this, &TrackBuilderWidget::onModuleProjectChanged);
    connect(m_module, &TrackBuilderModule::exportProgress,
            this, &TrackBuilderWidget::onExportProgress);
    onModuleProjectChanged();
}

// ============================================================================
// Terrain tab
// ============================================================================
QWidget* TrackBuilderWidget::buildTerrainTab()
{
    auto* w = new QWidget();
    auto* vl = new QVBoxLayout(w);
    vl->setSpacing(6); vl->setContentsMargins(6,6,6,6);

    // Brush mode buttons
    auto* modeGroup = new QGroupBox("Brush Mode");
    auto* modeGrid  = new QGridLayout(modeGroup);
    modeGrid->setSpacing(4);
    struct Btn{ QString icon; QString tip; QString mode; };
    QVector<Btn> modes={
        {u8"\u2191","Raise","raise"},{u8"\u2193","Lower","lower"},
        {u8"\u223C","Smooth","smooth"},{u8"=","Flatten","flatten"},
        {u8"\u2248","Noise","noise"},{u8"\u2198","Ramp","ramp"},
        {u8"\U0001F4A7","Erosion","erosion"}
    };
    for (int i=0;i<modes.size();++i) {
        auto* b=makeToolBtn(modes[i].icon,modes[i].tip);
        b->setCheckable(true);
        QString mode=modes[i].mode;
        connect(b,&QToolButton::clicked,[this,mode](){ if(m_module) m_module->setTerrainBrushMode(mode); });
        modeGrid->addWidget(b,i/4,i%4);
    }
    vl->addWidget(modeGroup);

    // Brush radius
    auto* radiusGroup=new QGroupBox("Brush Radius");
    auto* rl=new QHBoxLayout(radiusGroup);
    m_brushRadiusSlider=new QSlider(Qt::Horizontal);
    m_brushRadiusSlider->setRange(1,200); m_brushRadiusSlider->setValue(50);
    m_brushRadiusLabel=new QLabel("50 m");
    connect(m_brushRadiusSlider,&QSlider::valueChanged,[this](int v){
        m_brushRadiusLabel->setText(QString::number(v)+" m");
        if(m_module) m_module->setTerrainBrushRadius(float(v));
    });
    rl->addWidget(m_brushRadiusSlider); rl->addWidget(m_brushRadiusLabel);
    vl->addWidget(radiusGroup);

    // Brush strength
    auto* strGroup=new QGroupBox("Brush Strength");
    auto* sl=new QHBoxLayout(strGroup);
    m_brushStrengthSlider=new QSlider(Qt::Horizontal);
    m_brushStrengthSlider->setRange(1,100); m_brushStrengthSlider->setValue(50);
    m_brushStrengthLabel=new QLabel("50%");
    connect(m_brushStrengthSlider,&QSlider::valueChanged,[this](int v){
        m_brushStrengthLabel->setText(QString::number(v)+"%");
        if(m_module) m_module->setTerrainBrushStrength(v/100.f);
    });
    sl->addWidget(m_brushStrengthSlider); sl->addWidget(m_brushStrengthLabel);
    vl->addWidget(strGroup);

    // Import
    auto* importGroup=new QGroupBox("Import Heightmap");
    auto* il=new QVBoxLayout(importGroup);
    auto* btnImg=new QPushButton(u8"\U0001F5BC  Import from Image (PNG/TIF)");
    auto* btnSRTM=new QPushButton(u8"\U0001F30D  Import SRTM (.hgt)");
    connect(btnImg, &QPushButton::clicked, this, &TrackBuilderWidget::onImportTerrain);
    connect(btnSRTM,&QPushButton::clicked, this, &TrackBuilderWidget::onImportSRTM);
    il->addWidget(btnImg); il->addWidget(btnSRTM);
    vl->addWidget(importGroup);

    // Erosion
    auto* eroGroup=new QGroupBox("Sculpt Ops");
    auto* erl=new QHBoxLayout(eroGroup);
    auto* btnErode=new QPushButton("Thermal Erode");
    auto* btnHydro=new QPushButton("Hydraulic");
    connect(btnErode,&QPushButton::clicked,[this](){ if(m_module)m_module->erodeTerrain(5); });
    connect(btnHydro,&QPushButton::clicked,[this](){ if(m_module)m_module->hydraulicErosion(50); });
    erl->addWidget(btnErode); erl->addWidget(btnHydro);
    vl->addWidget(eroGroup);

    vl->addStretch();
    return w;
}

// ============================================================================
// Roads tab
// ============================================================================
QWidget* TrackBuilderWidget::buildRoadsTab()
{
    auto* w=new QWidget();
    auto* vl=new QVBoxLayout(w);
    vl->setSpacing(6); vl->setContentsMargins(6,6,6,6);

    // Road list
    auto* listGroup=new QGroupBox("Roads");
    auto* lg=new QVBoxLayout(listGroup);
    m_roadList=new QListWidget();
    m_roadList->setMaximumHeight(120);
    auto* roadBtns=new QHBoxLayout();
    auto* btnAdd=new QPushButton("+ Add Road");
    auto* btnDel=new QPushButton("- Remove");
    connect(btnAdd,&QPushButton::clicked,this,&TrackBuilderWidget::onAddRoad);
    connect(btnDel,&QPushButton::clicked,this,&TrackBuilderWidget::onRemoveRoad);
    roadBtns->addWidget(btnAdd); roadBtns->addWidget(btnDel);
    lg->addWidget(m_roadList); lg->addLayout(roadBtns);
    vl->addWidget(listGroup);

    // Road properties
    auto* propGroup=new QGroupBox("Road Properties");
    auto* pg=new QGridLayout(propGroup);
    pg->addWidget(new QLabel("Name:"),0,0);
    m_roadNameEdit=new QLineEdit();
    pg->addWidget(m_roadNameEdit,0,1);
    pg->addWidget(new QLabel("Surface:"),1,0);
    m_roadSurfaceCombo=new QComboBox();
    m_roadSurfaceCombo->addItems({"Asphalt","Concrete","Gravel","Dirt","Grass","Sand","Ice"});
    pg->addWidget(m_roadSurfaceCombo,1,1);
    m_roadBridgeCheck=new QCheckBox("Bridge");
    m_bridgeHeightSpin=new QDoubleSpinBox();
    m_bridgeHeightSpin->setRange(0,50); m_bridgeHeightSpin->setValue(3.0);
    m_bridgeHeightSpin->setSuffix(" m");
    pg->addWidget(m_roadBridgeCheck,2,0);
    pg->addWidget(m_bridgeHeightSpin,2,1);
    vl->addWidget(propGroup);

    // Width / camber hint
    auto* hintLabel=new QLabel("Tip: click on the 3D view to\nadd road points. Each point\nhas individual width & camber.");
    hintLabel->setStyleSheet("color:#888; font-size:11px;");
    hintLabel->setWordWrap(true);
    vl->addWidget(hintLabel);

    vl->addStretch();
    return w;
}

// ============================================================================
// Kerbs tab
// ============================================================================
QWidget* TrackBuilderWidget::buildKerbsTab()
{
    auto* w=new QWidget();
    auto* vl=new QVBoxLayout(w);
    vl->setSpacing(6); vl->setContentsMargins(6,6,6,6);

    auto* g=new QGroupBox("Add Kerb to Road");
    auto* gl=new QGridLayout(g);
    gl->addWidget(new QLabel("Style:"),0,0);
    auto* styleCombo=new QComboBox();
    styleCombo->addItems({"Sausage","Flat","Rumble","Piano","Wave Red","Wave White"});
    gl->addWidget(styleCombo,0,1);
    auto* leftCheck=new QCheckBox("Left side");
    leftCheck->setChecked(true);
    gl->addWidget(leftCheck,1,0,1,2);
    auto* btnAddKerb=new QPushButton("+ Add Kerb to Selected Road");
    connect(btnAddKerb,&QPushButton::clicked,[this,styleCombo,leftCheck](){
        if(!m_module||!m_roadList) return;
        auto* item=m_roadList->currentItem();
        if(!item) return;
        QString roadId=item->data(Qt::UserRole).toString();
        m_module->addKerb(roadId,leftCheck->isChecked(),
                           styleCombo->currentText().remove(' '));
    });
    gl->addWidget(btnAddKerb,2,0,1,2);
    vl->addWidget(g);
    vl->addStretch();
    return w;
}

// ============================================================================
// Walls tab
// ============================================================================
QWidget* TrackBuilderWidget::buildWallsTab()
{
    auto* w=new QWidget();
    auto* vl=new QVBoxLayout(w);
    vl->setSpacing(6); vl->setContentsMargins(6,6,6,6);

    auto* g=new QGroupBox("Wall Settings");
    auto* gl=new QGridLayout(g);
    gl->addWidget(new QLabel("Type:"),0,0);
    auto* typeCombo=new QComboBox();
    typeCombo->addItems({"Concrete","TireStack","Armco","Wood","Mesh","Invisible"});
    gl->addWidget(typeCombo,0,1);
    gl->addWidget(new QLabel("Height:"),1,0);
    auto* heightSpin=new QDoubleSpinBox();
    heightSpin->setRange(0.1,10.0); heightSpin->setValue(1.2); heightSpin->setSuffix(" m");
    gl->addWidget(heightSpin,1,1);
    auto* btnAdd=new QPushButton("+ Add Wall");
    connect(btnAdd,&QPushButton::clicked,this,&TrackBuilderWidget::onAddWall);
    gl->addWidget(btnAdd,2,0,1,2);
    vl->addWidget(g);
    vl->addStretch();
    return w;
}

// ============================================================================
// Props tab
// ============================================================================
QWidget* TrackBuilderWidget::buildPropsTab()
{
    auto* w=new QWidget();
    auto* vl=new QVBoxLayout(w);
    vl->setSpacing(6); vl->setContentsMargins(6,6,6,6);

    auto* g=new QGroupBox("Asset Bank Props");
    auto* gl=new QVBoxLayout(g);
    auto* list=new QListWidget();
    list->addItems({"barrier_concrete","barrier_tire","cone_orange",
                    "tree_pine_2d","tree_oak_2d","tree_pine_3d",
                    "rock_small","rock_large","sign_post",
                    "floodlight_pole","pit_building","grandstand"});
    gl->addWidget(list);
    auto* btnPlace=new QPushButton(u8"\U0001F4CC  Place Selected at Origin");
    connect(btnPlace,&QPushButton::clicked,[this,list](){
        if(!m_module) return;
        auto* item=list->currentItem();
        if(!item) return;
        m_module->addProp(item->text(),0,0,0,0,item->text());
    });
    gl->addWidget(btnPlace);
    vl->addWidget(g);
    vl->addStretch();
    return w;
}

// ============================================================================
// Lights tab
// ============================================================================
QWidget* TrackBuilderWidget::buildLightsTab()
{
    auto* w=new QWidget();
    auto* vl=new QVBoxLayout(w);
    vl->setSpacing(6); vl->setContentsMargins(6,6,6,6);

    auto* g=new QGroupBox("Add Light");
    auto* gl=new QGridLayout(g);
    gl->addWidget(new QLabel("Type:"),0,0);
    auto* typeCombo=new QComboBox();
    typeCombo->addItems({"Street","Floodlight","Ambient","Spot"});
    gl->addWidget(typeCombo,0,1);
    auto* btnAdd=new QPushButton("+ Add Light at Origin");
    connect(btnAdd,&QPushButton::clicked,[this,typeCombo](){
        if(m_module) m_module->addLight(typeCombo->currentText(),0,5,0);
    });
    gl->addWidget(btnAdd,1,0,1,2);
    vl->addWidget(g);
    vl->addStretch();
    return w;
}

// ============================================================================
// Start/Pit tab
// ============================================================================
QWidget* TrackBuilderWidget::buildStartPitTab()
{
    auto* w=new QWidget();
    auto* vl=new QVBoxLayout(w);
    vl->setSpacing(6); vl->setContentsMargins(6,6,6,6);

    auto* sg=new QGroupBox("Start Positions");
    auto* sl=new QVBoxLayout(sg);
    auto* btnAddStart=new QPushButton("+ Add Start at Origin");
    auto* btnClearStart=new QPushButton("Clear All");
    connect(btnAddStart,&QPushButton::clicked,this,&TrackBuilderWidget::onAddStartPos);
    connect(btnClearStart,&QPushButton::clicked,[this](){ if(m_module) m_module->clearStartPositions(); });
    sl->addWidget(btnAddStart); sl->addWidget(btnClearStart);
    vl->addWidget(sg);

    auto* pg=new QGroupBox("Pit Positions");
    auto* pl=new QVBoxLayout(pg);
    auto* btnAddPit=new QPushButton("+ Add Pit at Origin");
    auto* btnClearPit=new QPushButton("Clear All");
    connect(btnAddPit,&QPushButton::clicked,this,&TrackBuilderWidget::onAddPitPos);
    connect(btnClearPit,&QPushButton::clicked,[this](){ if(m_module) m_module->clearPitPositions(); });
    pl->addWidget(btnAddPit); pl->addWidget(btnClearPit);
    vl->addWidget(pg);

    vl->addStretch();
    return w;
}

// ============================================================================
// AI Line tab
// ============================================================================
QWidget* TrackBuilderWidget::buildAILineTab()
{
    auto* w=new QWidget();
    auto* vl=new QVBoxLayout(w);
    vl->setSpacing(6); vl->setContentsMargins(6,6,6,6);

    auto* g=new QGroupBox("AI Line Generation");
    auto* gl=new QVBoxLayout(g);
    auto* btnAuto=new QPushButton(u8"\U0001F916  Auto-Generate from Roads");
    auto* btnSmooth=new QPushButton(u8"\u223C  Smooth (3 passes)");
    auto* btnClear=new QPushButton(u8"\U0001F5D1  Clear");
    connect(btnAuto,  &QPushButton::clicked,this,&TrackBuilderWidget::onAutoAILine);
    connect(btnSmooth,&QPushButton::clicked,this,&TrackBuilderWidget::onSmoothAILine);
    connect(btnClear, &QPushButton::clicked,[this](){ if(m_module) m_module->clearAILine(); });
    gl->addWidget(btnAuto); gl->addWidget(btnSmooth); gl->addWidget(btnClear);
    vl->addWidget(g);

    auto* hint=new QLabel("The AI line defines the racing line\n"
                           "used by opponent cars. Auto-generate\n"
                           "uses road centre-lines as a base.");
    hint->setStyleSheet("color:#888; font-size:11px;");
    hint->setWordWrap(true);
    vl->addWidget(hint);
    vl->addStretch();
    return w;
}

// ============================================================================
// Physics Roads tab (Noise Labs)
// ============================================================================
QWidget* TrackBuilderWidget::buildPhysicsTab()
{
    auto* w=new QWidget();
    auto* vl=new QVBoxLayout(w);
    vl->setSpacing(6); vl->setContentsMargins(6,6,6,6);

    auto* g=new QGroupBox("Physics Road (Noise Labs)");
    auto* gl=new QGridLayout(g);
    gl->addWidget(new QLabel("Noise Amplitude:"),0,0);
    auto* ampSpin=new QDoubleSpinBox(); ampSpin->setRange(0,0.1); ampSpin->setSingleStep(0.001); ampSpin->setValue(0.002); ampSpin->setDecimals(4);
    gl->addWidget(ampSpin,0,1);
    gl->addWidget(new QLabel("Noise Frequency:"),1,0);
    auto* freqSpin=new QDoubleSpinBox(); freqSpin->setRange(0.1,100); freqSpin->setValue(10);
    gl->addWidget(freqSpin,1,1);
    gl->addWidget(new QLabel("Grip Multiplier:"),2,0);
    auto* gripSpin=new QDoubleSpinBox(); gripSpin->setRange(0.1,2); gripSpin->setSingleStep(0.05); gripSpin->setValue(1.0);
    gl->addWidget(gripSpin,2,1);
    auto* btnAdd=new QPushButton("+ Add Physics Road to Selected Road");
    connect(btnAdd,&QPushButton::clicked,[this,ampSpin,freqSpin,gripSpin](){
        if(!m_module||!m_roadList) return;
        auto* item=m_roadList->currentItem();
        if(!item) return;
        QString roadId=item->data(Qt::UserRole).toString();
        QString pid=m_module->addPhysicsRoad(roadId);
        m_module->setPhysicsRoadNoise(pid,float(ampSpin->value()),float(freqSpin->value()));
        m_module->setPhysicsRoadGrip(pid,float(gripSpin->value()),1.f);
    });
    gl->addWidget(btnAdd,3,0,1,2);
    vl->addWidget(g);
    vl->addStretch();
    return w;
}

// ============================================================================
// Export tab
// ============================================================================
QWidget* TrackBuilderWidget::buildExportTab()
{
    auto* w=new QWidget();
    auto* vl=new QVBoxLayout(w);
    vl->setSpacing(6); vl->setContentsMargins(6,6,6,6);

    // Output directory
    auto* dirGroup=new QGroupBox("Output Directory");
    auto* dgl=new QHBoxLayout(dirGroup);
    m_exportDirEdit=new QLineEdit();
    m_exportDirEdit->setPlaceholderText("Select output folder…");
    auto* btnBrowse=new QPushButton(u8"\U0001F4C2");
    btnBrowse->setMaximumWidth(32);
    connect(btnBrowse,&QPushButton::clicked,[this](){
        QString d=QFileDialog::getExistingDirectory(this,"Output Directory",
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        if(!d.isEmpty()) m_exportDirEdit->setText(d);
    });
    dgl->addWidget(m_exportDirEdit,1); dgl->addWidget(btnBrowse);
    vl->addWidget(dirGroup);

    m_exportZipCheck=new QCheckBox("Create ZIP (drag-and-drop to Content Manager)");
    m_exportZipCheck->setChecked(true);
    vl->addWidget(m_exportZipCheck);

    // Validation
    auto* valGroup=new QGroupBox("Pre-export Validation");
    auto* vgl=new QVBoxLayout(valGroup);
    m_validationList=new QListWidget();
    m_validationList->setMaximumHeight(100);
    auto* btnValidate=new QPushButton(u8"\u2714  Validate");
    connect(btnValidate,&QPushButton::clicked,this,&TrackBuilderWidget::onValidate);
    vgl->addWidget(m_validationList); vgl->addWidget(btnValidate);
    vl->addWidget(valGroup);

    // Export button
    auto* btnExport=new QPushButton(u8"\U0001F4E4  Export to Assetto Corsa");
    btnExport->setStyleSheet("QPushButton{background:#007acc;color:white;padding:10px;font-size:14px;font-weight:bold;}"
                              "QPushButton:hover{background:#005a9e;}");
    connect(btnExport,&QPushButton::clicked,this,&TrackBuilderWidget::onExport);
    vl->addWidget(btnExport);

    // Progress
    m_exportProgress=new QProgressBar();
    m_exportProgress->setRange(0,100); m_exportProgress->setValue(0);
    m_exportStatusLabel=new QLabel("");
    m_exportStatusLabel->setStyleSheet("color:#888; font-size:11px;");
    vl->addWidget(m_exportProgress);
    vl->addWidget(m_exportStatusLabel);
    vl->addStretch();
    return w;
}

// ============================================================================
// Helpers
// ============================================================================
QToolButton* TrackBuilderWidget::makeToolBtn(const QString& icon, const QString& tip)
{
    auto* b=new QToolButton();
    b->setText(icon); b->setToolTip(tip);
    return b;
}

QGroupBox* TrackBuilderWidget::makeGroup(const QString& title, QLayout* layout)
{
    auto* g=new QGroupBox(title);
    g->setLayout(layout);
    return g;
}

// ============================================================================
// Slots
// ============================================================================
void TrackBuilderWidget::onNewProject()
{
    if (!m_module) return;
    bool ok; QString name=QInputDialog::getText(this,"New Track","Track name:",
        QLineEdit::Normal,"New Track",&ok);
    if (!ok||name.isEmpty()) return;
    m_module->newProject(name);
}

void TrackBuilderWidget::onOpenProject()
{
    if (!m_module) return;
    QString path=QFileDialog::getOpenFileName(this,"Open Track Project","",
        "ksTrackBuilder (*.kstb);;JSON (*.json)");
    if (path.isEmpty()) return;
    m_module->loadProject(path);
}

void TrackBuilderWidget::onSaveProject()
{
    if (!m_module) return;
    QString path=QFileDialog::getSaveFileName(this,"Save Track Project","",
        "ksTrackBuilder (*.kstb);;JSON (*.json)");
    if (path.isEmpty()) return;
    if (!path.contains('.')) path+=".kstb";
    m_module->saveProject(path);
}

void TrackBuilderWidget::onExport()
{
    if (!m_module) return;
    QString dir=m_exportDirEdit->text();
    if (dir.isEmpty()) {
        dir=QFileDialog::getExistingDirectory(this,"Export Directory");
        if (dir.isEmpty()) return;
        m_exportDirEdit->setText(dir);
    }
    m_exportProgress->setValue(0);
    m_exportStatusLabel->setText("Exporting…");
    m_module->exportToAC(dir, m_exportZipCheck->isChecked());
}

void TrackBuilderWidget::onTerrainBrushChanged(int idx) {
    static const QStringList modes = {"raise", "lower", "smooth", "flatten", "noise", "ramp", "erosion"};
    if (m_module && idx >= 0 && idx < modes.size()) {
        m_module->setTerrainBrushMode(modes[idx]);
    }
}
void TrackBuilderWidget::onBrushRadiusChanged(int val)
{
    if (m_module) m_module->setTerrainBrushRadius(float(val));
}
void TrackBuilderWidget::onBrushStrengthChanged(int val)
{
    if (m_module) m_module->setTerrainBrushStrength(val/100.f);
}

void TrackBuilderWidget::onAddRoad()
{
    if (!m_module) return;
    bool ok; QString name=QInputDialog::getText(this,"Add Road","Road name:",
        QLineEdit::Normal,"Road",&ok);
    if (!ok||name.isEmpty()) return;
    QString id=m_module->addRoad(name);
    auto* item=new QListWidgetItem(name);
    item->setData(Qt::UserRole,id);
    m_roadList->addItem(item);
}

void TrackBuilderWidget::onRemoveRoad()
{
    if (!m_module||!m_roadList) return;
    auto* item=m_roadList->currentItem();
    if (!item) return;
    m_module->removeRoad(item->data(Qt::UserRole).toString());
    delete m_roadList->takeItem(m_roadList->row(item));
}

void TrackBuilderWidget::onAddWall()
{
    if (m_module) m_module->addWall("Wall");
}
void TrackBuilderWidget::onAddStartPos()
{
    if (m_module) m_module->addStartPosition(0,0,0,0);
}
void TrackBuilderWidget::onAddPitPos()
{
    if (m_module) m_module->addPitPosition(10,0,0,0);
}
void TrackBuilderWidget::onAutoAILine()
{
    if (m_module) m_module->autoGenerateAILine();
}
void TrackBuilderWidget::onSmoothAILine()
{
    if (m_module) m_module->smoothAILine(3);
}

void TrackBuilderWidget::onImportTerrain()
{
    if (!m_module) return;
    QString path=QFileDialog::getOpenFileName(this,"Import Heightmap","",
        "Images (*.png *.tif *.tiff *.jpg)");
    if (path.isEmpty()) return;
    m_module->importTerrainFromImage(path,-50.f,200.f);
}

void TrackBuilderWidget::onImportSRTM()
{
    if (!m_module) return;
    QString path=QFileDialog::getOpenFileName(this,"Import SRTM","","SRTM (*.hgt)");
    if (path.isEmpty()) return;
    m_module->importTerrainFromSRTM(path);
}

void TrackBuilderWidget::onValidate()
{
    if (!m_module||!m_validationList) return;
    m_validationList->clear();
    for (const QString& msg:m_module->exportValidationMessages()) {
        auto* item=new QListWidgetItem(msg);
        item->setForeground(msg=="OK"?QColor("#4ec9b0"):QColor("#f48771"));
        m_validationList->addItem(item);
    }
}

void TrackBuilderWidget::onModuleProjectChanged()
{
    if (!m_module) return;
    m_trackNameLabel->setText(m_module->trackName()+
                               (m_module->isDirty()?" *":""));
    m_statusLabel->setText(QString("%1 road(s), %2 prop(s)")
                            .arg(m_module->roadCount())
                            .arg(m_module->propCount()));
}

void TrackBuilderWidget::onExportProgress(int pct, const QString& stage)
{
    m_exportProgress->setValue(pct);
    m_exportStatusLabel->setText(stage);
}

}} // namespace ks::track
