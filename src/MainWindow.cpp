#include "MainWindow.h"
#include "sys/LogManager.h"
#include "sys/ProjectSerializer.h"
#include "assets/ProjectBuilder.h"
#include "Graphics/VulkanIntegration.h"
#include "Config/KsConfigIntegration.h"
#include "core/editor/EditorConfig.h"
#include "core/textEditor/TextEditorModule.h"
#include "core/tools/AutoSave.h"
#include "../modules/modellingEditor/wizard/CarWizard.h"
#include "../modules/modellingEditor/wizard/TrackWizard.h"
#include "../modules/modellingEditor/wizard/CharacterWizard.h"
#include "../modules/modellingEditor/3DModeling_Module.h"
#include "../modules/PhysicsEditor/PhysicsEditor.h"
#include "../modules/LiveryEditor/LiveryEditorModule.h"
#include "../modules/modellingEditor/TrackBuilder/TrackBuilderModule.h"
#include "../modules/soundEditor/AudioCore.h"
#include "../modules/soundEditor/KSAudioBankParser.h"
#include "../modules/fontEditor/FontCreatorQmlBridge.h"
#include "../modules/displayEditor/DisplayEditor.h"
#include "../modules/displayEditor/DisplayEditorQmlBridge.h"
#include "../modules/modellingEditor/TrackBuilder/TrackSurfaceEditorModule.h"
#include "../modules/modellingEditor/CharacterBuilder/DriverEditorModule.h"
#include "../modules/ShowroomEditor/ShowroomEditorModule.h"

#include <QApplication>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QDir>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QTimer>
#include <QLineEdit>
#include <QUndoView>
#include <QStandardPaths>
#include <QFontComboBox>
#include <QProcess>
#include <QPushButton>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDesktopServices>
#include <QUrl>

#ifdef Q_OS_WIN
#include <windows.h>
#include <winreg.h>

#include <QWindow>
#include <QOperatingSystemVersion>

#endif

void MainWindow::applyWindowFrameTheme(const QString& themeKey) {
    const auto& theme = ks::editor::RibbonThemeManager::instance().theme(themeKey);
    
#ifdef Q_OS_WIN
    // Windows: Use DWM for colored title bar
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows10) {
        HWND hwnd = reinterpret_cast<HWND>(winId());
        
        // Set title bar color
        COLORREF color = RGB(theme.windowBorder.red(), 
                             theme.windowBorder.green(), 
                             theme.windowBorder.blue());
        
        // Dark mode title bar
        BOOL darkMode = TRUE;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, 
                              &darkMode, sizeof(darkMode));
        
        // Set border color
        DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, 
                              &color, sizeof(color));
        
        // Set title bar background color
        DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, 
                              &color, sizeof(color));
    }
#endif
    
    // Qt-side styling
    setStyleSheet(QString(R"(
        QMainWindow {
            background: %1;
            border: 2px solid %2;
            border-radius: 0px;
        }
        
        QMainWindow::title {
            background: %3;
            color: %4;
            padding: 4px;
        }
    )")
    .arg(theme.centralBg.name())
    .arg(theme.windowBorder.name())
    .arg(theme.titleBarBg.name())
    .arg(theme.titleBarText.name()));
}

void MainWindow::setupRibbon() {
    m_ribbonBar = new ks::editor::RibbonBar(this);
    
    // Setup all 6 tabs
    setupCarTab();
    setupTrackTab();
    setupCharacterTab();
    setupShowroomTab();
    setupSoundTab();
    setupFontTab();
    
    // Replace menu bar
    setMenuWidget(m_ribbonBar);
    
    // Connect theme changes
    connect(m_ribbonBar, &ks::editor::RibbonBar::currentChanged,
            this, [this](int index) {
        // Map tab index to theme key
        static const QStringList themeKeys = {
            "car", "track", "character", "showroom", "sound", "font"
        };
        
        if (index >= 0 && index < themeKeys.size()) {
            QString themeKey = themeKeys[index];
            
            // Apply theme to ribbon
            m_ribbonBar->applyTheme(themeKey);
            
            // Apply theme to window frame
            ks::editor::RibbonThemeManager::instance().applyWindowFrame(this, themeKey);
            
            // Update window title color
            updateWindowTitle();
        }
    });
    
    // Initial theme (CAR = index 0)
    m_ribbonBar->applyTheme("car");
    ks::editor::RibbonThemeManager::instance().applyWindowFrame(this, "car");
}

void MainWindow::setupCarTab() {
    auto* carTab = new ks::editor::RibbonTab("CAR", this);
    
    // Panel: 3D Model
    auto* modelPanel = carTab->addPanel("3D Model");
    auto* modelGroup = modelPanel->addGroup("Actions");
    
    auto* importModelBtn = modelGroup->addButton(QIcon(":/icons/import.svg"), "Import FBX");
    importModelBtn->setStyle(ks::editor::RibbonButton::Style::Primary);
    connect(importModelBtn, &QToolButton::clicked, this, [this]() {
        QStringList files = QFileDialog::getOpenFileNames(this, tr("Import 3D models"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            tr("FBX Files (*.fbx);;All Files (*)"));
        if (!files.isEmpty()) {
            for (const QString &f : files) m_moduleManager->importFile(f);
        }
    });
    
    auto* exportKn5Btn = modelGroup->addButton(QIcon(":/icons/export.svg"), "Export KN5");
    connect(exportKn5Btn, &QToolButton::clicked, this, [this]() {
        QString folder = QFileDialog::getExistingDirectory(this, tr("Select export folder"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        if (!folder.isEmpty()) m_moduleManager->exportFile(folder);
    });
    
    // Panel: Physics
    auto* physPanel = carTab->addPanel("Physics");
    auto* physGroup = physPanel->addGroup("Setup");
    
    auto* suspBtn = physGroup->addButton(QIcon(":/icons/suspension.svg"), "Suspension");
    auto* aeroBtn = physGroup->addButton(QIcon(":/icons/aero.svg"), "Aerodynamics");
    auto* engineBtn = physGroup->addButton(QIcon(":/icons/engine.svg"), "Engine");
    
    connect(suspBtn, &QToolButton::clicked, this, [this]() {
        for (auto* mod : m_moduleManager->modules()) {
            if (auto* phys = qobject_cast<ks::PhysicsEditorModule*>(mod)) {
                phys->onShowSuspGeometry();
                m_moduleManager->setCurrentModule(m_moduleManager->moduleIndex(phys->moduleName()));
                return;
            }
        }
    });
    connect(aeroBtn, &QToolButton::clicked, this, [this]() {
        for (auto* mod : m_moduleManager->modules()) {
            if (auto* phys = qobject_cast<ks::PhysicsEditorModule*>(mod)) {
                phys->onShowFfbPreview();
                m_moduleManager->setCurrentModule(m_moduleManager->moduleIndex(phys->moduleName()));
                return;
            }
        }
    });
    connect(engineBtn, &QToolButton::clicked, this, [this]() {
        for (auto* mod : m_moduleManager->modules()) {
            if (auto* phys = qobject_cast<ks::PhysicsEditorModule*>(mod)) {
                phys->onShowEngineCurve();
                m_moduleManager->setCurrentModule(m_moduleManager->moduleIndex(phys->moduleName()));
                return;
            }
        }
    });
    
    // Panel: Liveries
    auto* liveryPanel = carTab->addPanel("Liveries");
    auto* liveryGroup = liveryPanel->addGroup("Skins");
    
    auto* newSkinBtn = liveryGroup->addButton(QIcon(":/icons/skin.svg"), "New Skin");
    newSkinBtn->setStyle(ks::editor::RibbonButton::Style::Success);
    
    auto* skinEditorBtn = liveryGroup->addButton(QIcon(":/icons/edit.svg"), "Skin Editor");
    
    connect(newSkinBtn, &QToolButton::clicked, this, [this]() {
        bool ok;
        QString name = QInputDialog::getText(this, tr("New Skin"), tr("Skin name:"), QLineEdit::Normal, QString(), &ok);
        if (ok && !name.isEmpty()) {
            ks::LiveryEditor::instance()->createSkin(name);
        }
    });
    connect(skinEditorBtn, &QToolButton::clicked, this, [this]() {
        for (auto* mod : m_moduleManager->modules()) {
            if (auto* livery = qobject_cast<ks::LiveryEditorModule*>(mod)) {
                m_moduleManager->setCurrentModule(m_moduleManager->moduleIndex(livery->getModuleName()));
                return;
            }
        }
    });
    
    // Panel: Data
    auto* dataPanel = carTab->addPanel("Data");
    auto* dataGroup = dataPanel->addGroup("Files");
    
    auto* editAcdBtn = dataGroup->addButton(QIcon(":/icons/code.svg"), "Edit data.acd");
    auto* editUiBtn = dataGroup->addButton(QIcon(":/icons/ui.svg"), "Edit UI");
    
    connect(editAcdBtn, &QToolButton::clicked, this, [this]() {
        for (auto* mod : m_moduleManager->modules()) {
            if (auto* phys = qobject_cast<ks::PhysicsEditorModule*>(mod)) {
                phys->onShowAcdBrowser();
                m_moduleManager->setCurrentModule(m_moduleManager->moduleIndex(phys->moduleName()));
                return;
            }
        }
    });
    connect(editUiBtn, &QToolButton::clicked, this, [this]() {
        QString iniPath = QFileDialog::getOpenFileName(this, tr("Open Display Config"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            tr("Display INI (*.ini);;All Files (*)"));
        if (!iniPath.isEmpty()) {
            auto* bridge = ks::DisplayEditorQmlBridge::instance();
            if (bridge->loadFromFile(iniPath)) {
                bridge->editorRequested();
            }
        }
    });
    
    m_ribbonBar->addTab(carTab);
}

void MainWindow::setupTrackTab() {
    auto* trackTab = new ks::editor::RibbonTab("TRACK", this);
    
    auto* trackPanel = trackTab->addPanel("Track");
    auto* trackGroup = trackPanel->addGroup("Creation");
    
    auto* newTrackBtn = trackGroup->addButton(QIcon(":/icons/track.svg"), "New Track");
    newTrackBtn->setStyle(ks::editor::RibbonButton::Style::Primary);
    
    auto* importTrackBtn = trackGroup->addButton(QIcon(":/icons/import.svg"), "Import");
    
    connect(newTrackBtn, &QToolButton::clicked, this, [this]() {
        bool ok;
        QString name = QInputDialog::getText(this, tr("New Track"), tr("Track name:"), QLineEdit::Normal, "New Track", &ok);
        if (ok && !name.isEmpty()) {
            auto* trackModule = ks::track::TrackBuilderModule::instance();
            if (trackModule) trackModule->newProject(name);
        }
    });
    connect(importTrackBtn, &QToolButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, tr("Import Track Project"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            tr("Track Project (*.json);;All Files (*)"));
        if (!path.isEmpty()) {
            auto* trackModule = ks::track::TrackBuilderModule::instance();
            if (trackModule) trackModule->loadProject(path);
        }
    });
    
    auto* terrainPanel = trackTab->addPanel("Terrain");
    auto* terrainGroup = terrainPanel->addGroup("Surface");
    
    auto* surfBtn = terrainGroup->addButton(QIcon(":/icons/surface.svg"), "Surfaces");
    auto* roadBtn = terrainGroup->addButton(QIcon(":/icons/road.svg"), "Road Mesh");
    
    connect(surfBtn, &QToolButton::clicked, this, [this]() {
        for (auto* mod : m_moduleManager->modules()) {
            if (qobject_cast<ks::TrackSurfaceEditorModule*>(mod)) {
                m_moduleManager->setCurrentModule(m_moduleManager->moduleIndex(mod->getModuleName()));
                return;
            }
        }
    });
    connect(roadBtn, &QToolButton::clicked, this, [this]() {
        auto* trackModule = ks::track::TrackBuilderModule::instance();
        if (trackModule) trackModule->addRoad();
    });
    
    auto* aiPanel = trackTab->addPanel("AI");
    auto* aiGroup = aiPanel->addGroup("Paths");
    
    auto* aiLineBtn = aiGroup->addButton(QIcon(":/icons/ai.svg"), "AI Line");
    auto* pitBtn = aiGroup->addButton(QIcon(":/icons/pit.svg"), "Pit Lane");
    
    connect(aiLineBtn, &QToolButton::clicked, this, [this]() {
        auto* trackModule = ks::track::TrackBuilderModule::instance();
        if (trackModule) trackModule->autoGenerateAILine();
    });
    connect(pitBtn, &QToolButton::clicked, this, [this]() {
        auto* trackModule = ks::track::TrackBuilderModule::instance();
        if (trackModule) trackModule->addPitPosition(0, 0, 0, 0);
    });
    
    m_ribbonBar->addTab(trackTab);
}

void MainWindow::setupCharacterTab() {
    auto* charTab = new ks::editor::RibbonTab("CHARACTER", this);
    
    auto* driverPanel = charTab->addPanel("Driver");
    auto* driverGroup = driverPanel->addGroup("Model");
    
    auto* importDriverBtn = driverGroup->addButton(QIcon(":/icons/character.svg"), "Import Model");
    importDriverBtn->setStyle(ks::editor::RibbonButton::Style::Primary);
    
    auto* rigBtn = driverGroup->addButton(QIcon(":/icons/rig.svg"), "Rigging");
    
    connect(importDriverBtn, &QToolButton::clicked, this, [this]() {
        QStringList files = QFileDialog::getOpenFileNames(this, tr("Import Character Model"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            tr("3D Files (*.fbx *.obj *.glb *.kn5);;All Files (*)"));
        if (!files.isEmpty()) {
            for (const QString &f : files) m_moduleManager->importFile(f);
        }
    });
    connect(rigBtn, &QToolButton::clicked, this, [this]() {
        for (auto* mod : m_moduleManager->modules()) {
            if (qobject_cast<ks::DriverEditorModule*>(mod)) {
                m_moduleManager->setCurrentModule(m_moduleManager->moduleIndex(mod->getModuleName()));
                return;
            }
        }
    });
    
    auto* animPanel = charTab->addPanel("Animation");
    auto* animGroup = animPanel->addGroup("Motions");
    
    auto* steeringBtn = animGroup->addButton(QIcon(":/icons/steering.svg"), "Steering");
    auto* shiftingBtn = animGroup->addButton(QIcon(":/icons/shift.svg"), "Shifting");
    auto* idleBtn = animGroup->addButton(QIcon(":/icons/idle.svg"), "Idle");
    
    connect(steeringBtn, &QToolButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, tr("Import Steering Animation"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            tr("Animation Files (*.fbx *.bvh *.ksanim);;All Files (*)"));
        if (!path.isEmpty()) m_moduleManager->importFile(path);
    });
    connect(shiftingBtn, &QToolButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, tr("Import Shifting Animation"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            tr("Animation Files (*.fbx *.bvh *.ksanim);;All Files (*)"));
        if (!path.isEmpty()) m_moduleManager->importFile(path);
    });
    connect(idleBtn, &QToolButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, tr("Import Idle Animation"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            tr("Animation Files (*.fbx *.bvh *.ksanim);;All Files (*)"));
        if (!path.isEmpty()) m_moduleManager->importFile(path);
    });
    
    auto* texPanel = charTab->addPanel("Textures");
    auto* texGroup = texPanel->addGroup("Materials");
    
    auto* suitBtn = texGroup->addButton(QIcon(":/icons/suit.svg"), "Suit");
    auto* helmetBtn = texGroup->addButton(QIcon(":/icons/helmet.svg"), "Helmet");
    auto* glovesBtn = texGroup->addButton(QIcon(":/icons/gloves.svg"), "Gloves");
    
    connect(suitBtn, &QToolButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, tr("Import Suit Texture"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            tr("Image Files (*.png *.jpg *.dds);;All Files (*)"));
        if (!path.isEmpty()) m_moduleManager->importFile(path);
    });
    connect(helmetBtn, &QToolButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, tr("Import Helmet Texture"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            tr("Image Files (*.png *.jpg *.dds);;All Files (*)"));
        if (!path.isEmpty()) m_moduleManager->importFile(path);
    });
    connect(glovesBtn, &QToolButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, tr("Import Gloves Texture"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            tr("Image Files (*.png *.jpg *.dds);;All Files (*)"));
        if (!path.isEmpty()) m_moduleManager->importFile(path);
    });
    
    m_ribbonBar->addTab(charTab);
}

void MainWindow::setupShowroomTab() {
    auto* showroomTab = new ks::editor::RibbonTab("SHOWROOM", this);
    
    auto* scenePanel = showroomTab->addPanel("Scene");
    auto* sceneGroup = scenePanel->addGroup("Setup");
    
    auto* newSceneBtn = sceneGroup->addButton(QIcon(":/icons/scene.svg"), "New Scene");
    newSceneBtn->setStyle(ks::editor::RibbonButton::Style::Primary);
    
    auto* bgBtn = sceneGroup->addButton(QIcon(":/icons/background.svg"), "Background");
    auto* floorBtn = sceneGroup->addButton(QIcon(":/icons/floor.svg"), "Floor");
    
    connect(newSceneBtn, &QToolButton::clicked, this, [this]() {
        for (auto* mod : m_moduleManager->modules()) {
            if (qobject_cast<ks::ShowroomEditorModule*>(mod)) {
                m_moduleManager->setCurrentModule(m_moduleManager->moduleIndex(mod->getModuleName()));
                return;
            }
        }
    });
    connect(bgBtn, &QToolButton::clicked, this, [this]() {
        for (auto* mod : m_moduleManager->modules()) {
            if (auto* showroom = qobject_cast<ks::ShowroomEditorModule*>(mod)) {
                m_moduleManager->setCurrentModule(m_moduleManager->moduleIndex(showroom->moduleName()));
                return;
            }
        }
    });
    connect(floorBtn, &QToolButton::clicked, this, [this]() {
        for (auto* mod : m_moduleManager->modules()) {
            if (auto* showroom = qobject_cast<ks::ShowroomEditorModule*>(mod)) {
                m_moduleManager->setCurrentModule(m_moduleManager->moduleIndex(showroom->moduleName()));
                return;
            }
        }
    });
    
    auto* lightPanel = showroomTab->addPanel("Lighting");
    auto* lightGroup = lightPanel->addGroup("Lights");
    
    auto* ambientBtn = lightGroup->addButton(QIcon(":/icons/ambient.svg"), "Ambient");
    auto* spotBtn = lightGroup->addButton(QIcon(":/icons/spotlight.svg"), "Spot");
    auto* hdriBtn = lightGroup->addButton(QIcon(":/icons/hdri.svg"), "HDRI");
    
    connect(ambientBtn, &QToolButton::clicked, this, [this]() {
        for (auto* mod : m_moduleManager->modules()) {
            if (auto* showroom = qobject_cast<ks::ShowroomEditorModule*>(mod)) {
                m_moduleManager->setCurrentModule(m_moduleManager->moduleIndex(showroom->moduleName()));
                return;
            }
        }
    });
    connect(spotBtn, &QToolButton::clicked, this, [this]() {
        for (auto* mod : m_moduleManager->modules()) {
            if (auto* showroom = qobject_cast<ks::ShowroomEditorModule*>(mod)) {
                m_moduleManager->setCurrentModule(m_moduleManager->moduleIndex(showroom->moduleName()));
                return;
            }
        }
    });
    connect(hdriBtn, &QToolButton::clicked, this, [this]() {
        for (auto* mod : m_moduleManager->modules()) {
            if (auto* showroom = qobject_cast<ks::ShowroomEditorModule*>(mod)) {
                m_moduleManager->setCurrentModule(m_moduleManager->moduleIndex(showroom->moduleName()));
                return;
            }
        }
    });
    
    auto* cameraPanel = showroomTab->addPanel("Camera");
    auto* cameraGroup = cameraPanel->addGroup("Views");
    
    auto* orbitBtn = cameraGroup->addButton(QIcon(":/icons/orbit.svg"), "Orbit");
    auto* turntableBtn = cameraGroup->addButton(QIcon(":/icons/turntable.svg"), "Turntable");
    auto* renderBtn = cameraGroup->addButton(QIcon(":/icons/render.svg"), "Render");
    renderBtn->setStyle(ks::editor::RibbonButton::Style::Success);
    
    connect(orbitBtn, &QToolButton::clicked, this, [this]() {
        for (auto* mod : m_moduleManager->modules()) {
            if (auto* showroom = qobject_cast<ks::ShowroomEditorModule*>(mod)) {
                m_moduleManager->setCurrentModule(m_moduleManager->moduleIndex(showroom->moduleName()));
                return;
            }
        }
    });
    connect(turntableBtn, &QToolButton::clicked, this, [this]() {
        for (auto* mod : m_moduleManager->modules()) {
            if (auto* showroom = qobject_cast<ks::ShowroomEditorModule*>(mod)) {
                m_moduleManager->setCurrentModule(m_moduleManager->moduleIndex(showroom->moduleName()));
                return;
            }
        }
    });
    connect(renderBtn, &QToolButton::clicked, this, [this]() {
        for (auto* mod : m_moduleManager->modules()) {
            if (auto* showroom = qobject_cast<ks::ShowroomEditorModule*>(mod)) {
                showroom->onGeneratePreview();
                m_moduleManager->setCurrentModule(m_moduleManager->moduleIndex(showroom->moduleName()));
                return;
            }
        }
    });
    
    m_ribbonBar->addTab(showroomTab);
}

void MainWindow::setupSoundTab() {
    auto* soundTab = new ks::editor::RibbonTab("SOUND", this);
    
    auto* enginePanel = soundTab->addPanel("Engine");
    auto* engineGroup = enginePanel->addGroup("Audio");
    
    auto* importEngineBtn = engineGroup->addButton(QIcon(":/icons/engine.svg"), "Engine Sound");
    importEngineBtn->setStyle(ks::editor::RibbonButton::Style::Primary);
    
    auto* exhaustBtn = engineGroup->addButton(QIcon(":/icons/exhaust.svg"), "Exhaust");
    auto* intakeBtn = engineGroup->addButton(QIcon(":/icons/intake.svg"), "Intake");
    
    connect(importEngineBtn, &QToolButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, tr("Import Engine Sound"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            tr("Audio Files (*.wav *.ogg *.mp3 *.flac);;All Files (*)"));
        if (!path.isEmpty()) m_moduleManager->importFile(path);
    });
    connect(exhaustBtn, &QToolButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, tr("Import Exhaust Sound"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            tr("Audio Files (*.wav *.ogg *.mp3 *.flac);;All Files (*)"));
        if (!path.isEmpty()) m_moduleManager->importFile(path);
    });
    connect(intakeBtn, &QToolButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, tr("Import Intake Sound"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            tr("Audio Files (*.wav *.ogg *.mp3 *.flac);;All Files (*)"));
        if (!path.isEmpty()) m_moduleManager->importFile(path);
    });
    
    auto* gearPanel = soundTab->addPanel("Transmission");
    auto* gearGroup = gearPanel->addGroup("Sounds");
    
    auto* shiftBtn = gearGroup->addButton(QIcon(":/icons/shift.svg"), "Shift");
    auto* clutchBtn = gearGroup->addButton(QIcon(":/icons/clutch.svg"), "Clutch");
    auto* backfireBtn = gearGroup->addButton(QIcon(":/icons/fire.svg"), "Backfire");
    
    connect(shiftBtn, &QToolButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, tr("Import Shift Sound"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            tr("Audio Files (*.wav *.ogg *.mp3 *.flac);;All Files (*)"));
        if (!path.isEmpty()) m_moduleManager->importFile(path);
    });
    connect(clutchBtn, &QToolButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, tr("Import Clutch Sound"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            tr("Audio Files (*.wav *.ogg *.mp3 *.flac);;All Files (*)"));
        if (!path.isEmpty()) m_moduleManager->importFile(path);
    });
    connect(backfireBtn, &QToolButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, tr("Import Backfire Sound"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            tr("Audio Files (*.wav *.ogg *.mp3 *.flac);;All Files (*)"));
        if (!path.isEmpty()) m_moduleManager->importFile(path);
    });
    
    auto* surfacePanel = soundTab->addPanel("Surfaces");
    auto* surfaceGroup = surfacePanel->addGroup("Road");
    
    auto* tireBtn = surfaceGroup->addButton(QIcon(":/icons/tire.svg"), "Tire Scrub");
    auto* gravelBtn = surfaceGroup->addButton(QIcon(":/icons/gravel.svg"), "Gravel");
    auto* curbBtn = surfaceGroup->addButton(QIcon(":/icons/curb.svg"), "Curbs");
    
    connect(tireBtn, &QToolButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, tr("Import Tire Scrub Sound"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            tr("Audio Files (*.wav *.ogg *.mp3 *.flac);;All Files (*)"));
        if (!path.isEmpty()) m_moduleManager->importFile(path);
    });
    connect(gravelBtn, &QToolButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, tr("Import Gravel Sound"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            tr("Audio Files (*.wav *.ogg *.mp3 *.flac);;All Files (*)"));
        if (!path.isEmpty()) m_moduleManager->importFile(path);
    });
    connect(curbBtn, &QToolButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, tr("Import Curb Sound"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            tr("Audio Files (*.wav *.ogg *.mp3 *.flac);;All Files (*)"));
        if (!path.isEmpty()) m_moduleManager->importFile(path);
    });
    
    auto* audioStudioPanel = soundTab->addPanel("ksAudioStudio");
    auto* audioStudioGroup = audioStudioPanel->addGroup("Tools");

    auto* bankBtn = audioStudioGroup->addButton(QIcon(":/icons/bank.svg"), "Build Audio Bank");
    bankBtn->setStyle(ks::editor::RibbonButton::Style::Warning);

    auto* eventsBtn = audioStudioGroup->addButton(QIcon(":/icons/events.svg"), "Events");
    auto* paramsBtn = audioStudioGroup->addButton(QIcon(":/icons/params.svg"), "Parameters");

    auto* recordStudioBtn = audioStudioGroup->addButton(QIcon(":/icons/record.svg"), "Recording Studio");
    recordStudioBtn->setStyle(ks::editor::RibbonButton::Style::Primary);

    connect(bankBtn, &QToolButton::clicked, this, [this]() {
        auto* audio = ks::AudioEditorModule::instance();
        if (audio) audio->onBuildBanks();
    });
    connect(eventsBtn, &QToolButton::clicked, this, [this]() {
        auto* audio = ks::AudioEditorModule::instance();
        if (audio) audio->onImportAsset();
    });
    connect(recordStudioBtn, &QToolButton::clicked, this, [this]() {
        auto* audio = ks::AudioEditorModule::instance();
        if (audio) audio->onNewProject();
    });

    m_ribbonBar->addTab(soundTab);
}

void MainWindow::setupFontTab() {
    auto* fontTab = new ks::editor::RibbonTab("FONT", this);
    
    auto* typePanel = fontTab->addPanel("Typeface");
    auto* typeGroup = typePanel->addGroup("Font");
    
    auto* newFontBtn = typeGroup->addButton(QIcon(":/icons/font.svg"), "New Font");
    newFontBtn->setStyle(ks::editor::RibbonButton::Style::Primary);
    
    auto* importFontBtn = typeGroup->addButton(QIcon(":/icons/import.svg"), "Import");
    auto* exportFontBtn = typeGroup->addButton(QIcon(":/icons/export.svg"), "Export");
    
    auto* glyphPanel = fontTab->addPanel("Glyphs");
    auto* glyphGroup = glyphPanel->addGroup("Editor");
    
    auto* editGlyphBtn = glyphGroup->addButton(QIcon(":/icons/glyph.svg"), "Edit Glyph");
    auto* metricsBtn = glyphGroup->addButton(QIcon(":/icons/metrics.svg"), "Metrics");
    auto* kerningBtn = glyphGroup->addButton(QIcon(":/icons/kerning.svg"), "Kerning");
    
    auto* previewPanel = fontTab->addPanel("Preview");
    auto* previewGroup = previewPanel->addGroup("View");
    
    auto* previewBtn = previewGroup->addButton(QIcon(":/icons/preview.svg"), "Preview");
    previewBtn->setStyle(ks::editor::RibbonButton::Style::Success);
    
    connect(newFontBtn, &QToolButton::clicked, this, [this]() {
        auto* fontBridge = ks::FontCreatorQmlBridge::instance();
        if (fontBridge) {
            QStringList fonts = fontBridge->getSystemFonts();
            if (!fonts.isEmpty()) fontBridge->setFontFamily(fonts.first());
        }
    });
    connect(importFontBtn, &QToolButton::clicked, this, [this]() {
        auto* fontBridge = ks::FontCreatorQmlBridge::instance();
        if (!fontBridge) return;
        QString path = QFileDialog::getOpenFileName(this, tr("Import Font Config"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            tr("Font Config (*.json *.acf);;All Files (*)"));
        if (!path.isEmpty()) {
            if (path.endsWith(".json"))
                fontBridge->importFromJSON(path);
            else
                fontBridge->loadPreset(path);
        }
    });
    connect(exportFontBtn, &QToolButton::clicked, this, [this]() {
        auto* fontBridge = ks::FontCreatorQmlBridge::instance();
        if (!fontBridge) return;
        QString path = QFileDialog::getSaveFileName(this, tr("Export Font"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            tr("Font Atlas (*.png);;AC Font (*.acf);;All Files (*)"));
        if (!path.isEmpty()) {
            if (path.endsWith(".acf"))
                fontBridge->savePreset(path);
            else
                fontBridge->generateAtlas(path);
        }
    });
    connect(editGlyphBtn, &QToolButton::clicked, this, [this]() {
        // Toggle glyph editor mode - the QML UI handles this
    });
    connect(metricsBtn, &QToolButton::clicked, this, [this]() {
        auto* fontBridge = ks::FontCreatorQmlBridge::instance();
        if (fontBridge) {
            QVariantList glyphs = fontBridge->getGlyphs();
            setStatusMessage(tr("Loaded %1 glyphs with metrics").arg(glyphs.size()));
        }
    });
    connect(kerningBtn, &QToolButton::clicked, this, [this]() {
        auto* fontBridge = ks::FontCreatorQmlBridge::instance();
        if (fontBridge) {
            fontBridge->extractKerning();
            setStatusMessage(tr("Extracted kerning pairs from current font"));
        }
    });
    connect(previewBtn, &QToolButton::clicked, this, [this]() {
        auto* fontBridge = ks::FontCreatorQmlBridge::instance();
        if (fontBridge) {
            QString preview = fontBridge->getPreviewText();
            setStatusMessage(tr("Font preview: %1").arg(preview.isEmpty() ? "No preview text set" : preview.left(50)));
        }
    });
    
    m_ribbonBar->addTab(fontTab);
}

// ==================== Constructor / Destructor ====================

MainWindow::MainWindow(const QString& projectPath, QWidget* parent)
    : QMainWindow(parent)
    , m_moduleManager(new ModuleManager(this))
    , m_settings(new SettingsManager(this))
    , m_undoStack(new QUndoStack(this))
    , m_currentProjectPath(projectPath)
    , m_projectBuilder(new ProjectBuilder(this))
{
    LOG_INFO("MainWindow", "Initializing main window");

    // Initialize help system
    if (ks::HelpSystem::instance()) {
        ks::HelpSystem::instance()->initialize(this);
        ks::HelpSystem::instance()->enableHelp(true);
        ks::HelpSystem::instance()->startQuickStartGuide();
    }

    // Set window properties
    setWindowTitle(Constants::APP_NAME);
    resize(Constants::DEFAULT_WINDOW_SIZE);
    setMinimumSize(Constants::MIN_WINDOW_SIZE);
    setAcceptDrops(true);

    // Setup UI
    setupUI();
    setupMenuBar();
    setupRibbon();
    setupToolBar();
    setupStatusBar();
    setupDockWidgets();
    setupConnections();
    createActions();
    updateActions();

    // Detect environment
    detectSimulator();
    detectCSPVersion();
    updateRecentProjectsMenu();
    updateWindowTitle();

    // Setup auto-recovery timer
    m_sessionRecoveryTimer = new QTimer(this);
    connect(m_sessionRecoveryTimer, &QTimer::timeout, this, &MainWindow::saveSessionBackup);
    m_sessionRecoveryTimer->start(30000); // Save every 30 seconds

    // Setup crash recovery
    m_crashRecovery = new ks::CrashRecovery(this);
    connect(m_crashRecovery, &ks::CrashRecovery::recoveryNeeded,
            this, &MainWindow::showRecoveryDialog);
    m_crashRecovery->startSession();
    if (!m_currentProjectPath.isEmpty()) {
        m_crashRecovery->addOpenDocument(m_currentProjectPath);
        m_crashRecovery->setActiveDocument(m_currentProjectPath);
    }

    // Setup template system
    m_templateManager = ks::TemplateManager::instance();
    m_templateManager->initialize();

    // Setup file comparison system
    m_diffEngine = ks::FileComparisonEngine::instance();
    m_diffEngine->initialize();

    // Restore window geometry
    QSettings settings(Constants::APP_NAME, Constants::APP_NAME);
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());

    LOG_INFO("MainWindow", "Main window initialized");
}

MainWindow::~MainWindow()
{
    LOG_INFO("MainWindow", "Shutting down main window");

    // End crash recovery session
    if (m_crashRecovery) {
        m_crashRecovery->endSession();
    }

    // Cancel any running build
    if (m_projectBuilder) {
        m_projectBuilder->cancel();
    }

    // Save window state
    QSettings settings(Constants::APP_NAME, Constants::APP_NAME);
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
}

// ==================== Setup Methods ====================

void MainWindow::setupUI()
{
    setCentralWidget(m_moduleManager);
}

void MainWindow::setupMenuBar()
{
    m_menuBar = menuBar();

    setupFileMenu();
    setupEditMenu();
    setupViewMenu();
    setupModulesMenu();
    setupSettingsMenu();
    setupHelpMenu();
}

void MainWindow::setupFileMenu()
{
    QMenu* fileMenu = m_menuBar->addMenu(tr("&File"));

    QAction* newAct = new QAction(tr("&New Project..."), this);
    newAct->setShortcut(QKeySequence::New);
    newAct->setStatusTip(tr("Create a new ksEditor project"));
    connect(newAct, &QAction::triggered, this, &MainWindow::newProject);
    fileMenu->addAction(newAct);

    QAction* openAct = new QAction(tr("&Open Project..."), this);
    openAct->setShortcut(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing project"));
    connect(openAct, &QAction::triggered, this, &MainWindow::openProject);
    QAction* import3DAct = new QAction(tr("&Import 3D..."), this);
    import3DAct->setStatusTip(tr("Import 3D model(s) into the current module"));
    connect(import3DAct, &QAction::triggered, this, [this]() {
        QStringList files = QFileDialog::getOpenFileNames(this, tr("Import 3D files"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            tr("3D Files (*.fbx *.obj *.glb *.gltf *.dae *.kn5);;All Files (*)"));
        if (!files.isEmpty()) {
            for (const QString &f : files) m_moduleManager->importFile(f);
        }
    });
    fileMenu->addAction(import3DAct);

    QAction* export3DAct = new QAction(tr("&Export 3D..."), this);
    export3DAct->setStatusTip(tr("Export selected object(s) or project to supported 3D formats"));
    connect(export3DAct, &QAction::triggered, this, [this]() {
        QString folder = QFileDialog::getExistingDirectory(this, tr("Select export folder"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        if (!folder.isEmpty()) m_moduleManager->exportFile(folder);
    });
    fileMenu->addAction(export3DAct);

    fileMenu->addSeparator();
    QMenu* recentMenu = fileMenu->addMenu(tr("Recent Projects"));
    for (int i = 0; i < Constants::MAX_RECENT_PROJECTS; ++i) {
        QAction* recentAct = new QAction(tr("Empty"), this);
        recentAct->setVisible(false);
        connect(recentAct, &QAction::triggered, this, [this, recentAct]() {
            openRecentProject(recentAct->data().toString());
        });
        m_recentProjectsActions.append(recentAct);
        recentMenu->addAction(recentAct);
    }
    fileMenu->addMenu(recentMenu);
    fileMenu->addSeparator();

    m_saveAction = new QAction(tr("&Save"), this);
    m_saveAction->setShortcut(QKeySequence::Save);
    m_saveAction->setStatusTip(tr("Save the current project"));
    connect(m_saveAction, &QAction::triggered, this, &MainWindow::saveProject);
    fileMenu->addAction(m_saveAction);

    m_saveAsAction = new QAction(tr("Save &As..."), this);
    m_saveAsAction->setShortcut(QKeySequence::SaveAs);
    m_saveAsAction->setStatusTip(tr("Save the project with a new name"));
    connect(m_saveAsAction, &QAction::triggered, this, &MainWindow::saveProjectAs);
    fileMenu->addAction(m_saveAsAction);
    fileMenu->addSeparator();

    m_buildAction = new QAction(tr("&Build"), this);
    m_buildAction->setShortcut(QKeySequence(tr("Ctrl+B")));
    m_buildAction->setStatusTip(tr("Build the current project"));
    connect(m_buildAction, &QAction::triggered, this, &MainWindow::buildProject);
    fileMenu->addAction(m_buildAction);
    fileMenu->addSeparator();

    QAction* closeAct = new QAction(tr("&Close Project"), this);
    closeAct->setShortcut(QKeySequence(tr("Ctrl+W")));
    closeAct->setStatusTip(tr("Close the current project"));
    connect(closeAct, &QAction::triggered, this, &MainWindow::closeProject);
    fileMenu->addAction(closeAct);
    fileMenu->addSeparator();

    QAction* exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcut(QKeySequence::Quit);
    connect(exitAct, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(exitAct);
}

void MainWindow::setupEditMenu()
{
    QMenu* editMenu = m_menuBar->addMenu(tr("&Edit"));

    m_undoAction = new QAction(tr("&Undo"), this);
    m_undoAction->setShortcut(QKeySequence::Undo);
    connect(m_undoAction, &QAction::triggered, this, &MainWindow::undo);
    editMenu->addAction(m_undoAction);

    m_redoAction = new QAction(tr("&Redo"), this);
    m_redoAction->setShortcut(QKeySequence::Redo);
    connect(m_redoAction, &QAction::triggered, this, &MainWindow::redo);
    editMenu->addAction(m_redoAction);

    editMenu->addSeparator();

    m_cutAction = new QAction(tr("Cu&t"), this);
    m_cutAction->setShortcut(QKeySequence::Cut);
    connect(m_cutAction, &QAction::triggered, this, &MainWindow::cut);
    editMenu->addAction(m_cutAction);

    m_copyAction = new QAction(tr("&Copy"), this);
    m_copyAction->setShortcut(QKeySequence::Copy);
    connect(m_copyAction, &QAction::triggered, this, &MainWindow::copy);
    editMenu->addAction(m_copyAction);

    m_pasteAction = new QAction(tr("&Paste"), this);
    m_pasteAction->setShortcut(QKeySequence::Paste);
    connect(m_pasteAction, &QAction::triggered, this, &MainWindow::paste);
    editMenu->addAction(m_pasteAction);

    m_deleteAction = new QAction(tr("&Delete"), this);
    m_deleteAction->setShortcut(QKeySequence::Delete);
    connect(m_deleteAction, &QAction::triggered, this, &MainWindow::deleteSelected);
    editMenu->addAction(m_deleteAction);
}

void MainWindow::setupViewMenu()
{
    QMenu* viewMenu = m_menuBar->addMenu(tr("&View"));

    QAction* fullscreenAct = new QAction(tr("&Fullscreen"), this);
    fullscreenAct->setShortcut(Qt::Key_F11);
    connect(fullscreenAct, &QAction::triggered, this, &MainWindow::toggleFullscreen);
    viewMenu->addAction(fullscreenAct);
    viewMenu->addSeparator();

    QAction* sidebarAct = new QAction(tr("&Sidebar"), this);
    sidebarAct->setCheckable(true);
    sidebarAct->setChecked(true);
    connect(sidebarAct, &QAction::triggered, this, &MainWindow::toggleSidebar);
    viewMenu->addAction(sidebarAct);

    QAction* propertiesAct = new QAction(tr("&Properties"), this);
    propertiesAct->setCheckable(true);
    propertiesAct->setChecked(true);
    connect(propertiesAct, &QAction::triggered, this, &MainWindow::toggleProperties);
    viewMenu->addAction(propertiesAct);

    QAction* outputAct = new QAction(tr("&Output"), this);
    outputAct->setCheckable(true);
    outputAct->setChecked(true);
    connect(outputAct, &QAction::toggled, [this](bool checked) {
        if (m_outputDock) m_outputDock->setVisible(checked);
    });
    viewMenu->addAction(outputAct);
    viewMenu->addSeparator();

    QAction* statusbarAct = new QAction(tr("&Status Bar"), this);
    statusbarAct->setCheckable(true);
    statusbarAct->setChecked(true);
    connect(statusbarAct, &QAction::triggered, this, &MainWindow::toggleStatusBar);
    viewMenu->addAction(statusbarAct);
    viewMenu->addSeparator();

    QAction* resetLayoutAct = new QAction(tr("&Reset Layout"), this);
    connect(resetLayoutAct, &QAction::triggered, this, &MainWindow::resetLayout);
    viewMenu->addAction(resetLayoutAct);
    viewMenu->addSeparator();

    QAction* scriptConsoleAct = new QAction(tr("Script Console"), this);
    scriptConsoleAct->setShortcut(Qt::Key_F12);
    connect(scriptConsoleAct, &QAction::triggered, this, &MainWindow::toggleScriptConsole);
    viewMenu->addAction(scriptConsoleAct);

    QAction* terminalAct = new QAction(tr("Terminal"), this);
    terminalAct->setShortcut(QKeySequence(static_cast<int>(Qt::CTRL) | static_cast<int>(Qt::Key_AsciiTilde)));
    connect(terminalAct, &QAction::triggered, this, &MainWindow::toggleTerminal);
    viewMenu->addAction(terminalAct);

    QAction* sourceControlAct = new QAction(tr("Source Control"), this);
    sourceControlAct->setCheckable(true);
    sourceControlAct->setChecked(false);
    connect(sourceControlAct, &QAction::triggered, this, &MainWindow::toggleGit);
    connect(m_gitDock, &QDockWidget::visibilityChanged, [sourceControlAct](bool visible) {
        sourceControlAct->setChecked(visible);
    });
    viewMenu->addAction(sourceControlAct);

    viewMenu->addSeparator();

    QAction* searchResultsAct = new QAction(tr("Find in Files..."), this);
    searchResultsAct->setCheckable(true);
    searchResultsAct->setChecked(false);
    searchResultsAct->setShortcut(QKeySequence(static_cast<int>(Qt::CTRL) | static_cast<int>(Qt::SHIFT) | static_cast<int>(Qt::Key_F)));
    connect(searchResultsAct, &QAction::triggered, [this]() {
        if (m_searchDock) {
            m_searchDock->setVisible(!m_searchDock->isVisible());
            if (m_searchDock->isVisible() && m_projectSearch) {
                m_projectSearch->setFocus();
            }
        }
    });
    connect(m_searchDock, &QDockWidget::visibilityChanged, [this, searchResultsAct](bool visible) {
        searchResultsAct->setChecked(visible);
        if (visible && m_projectSearch) m_projectSearch->focusSearchInput();
    });
    viewMenu->addAction(searchResultsAct);
}

void MainWindow::setupModulesMenu()
{
    QMenu* modulesMenu = m_menuBar->addMenu(tr("&Modules"));

    m_moduleGroup = new QActionGroup(this);
    m_moduleGroup->setExclusive(true);

    for (int i = 0; i < m_moduleManager->moduleCount(); ++i) {
        QString name = m_moduleManager->moduleName(i);
        QAction* moduleAct = new QAction(name, this);
        moduleAct->setCheckable(true);
        moduleAct->setChecked(i == 0);
        moduleAct->setData(i);
        if (i < 9) moduleAct->setShortcut(QKeySequence(static_cast<int>(Qt::CTRL) | (static_cast<int>(Qt::Key_1) + i)));
        m_moduleGroup->addAction(moduleAct);
        modulesMenu->addAction(moduleAct);
    }

    connect(m_moduleGroup, &QActionGroup::triggered, [this](QAction* action) {
        switchToModule(action->data().toInt());
    });
}

void MainWindow::setupSettingsMenu()
{
    QMenu* settingsMenu = m_menuBar->addMenu(tr("Se&ttings"));

    QAction* prefsAct = new QAction(tr("&Preferences..."), this);
    prefsAct->setShortcut(Qt::CTRL + Qt::Key_P);
    connect(prefsAct, &QAction::triggered, this, &MainWindow::showSettings);
    settingsMenu->addAction(prefsAct);
    settingsMenu->addSeparator();

    QAction* runInAC = new QAction(tr("&Launch in Simulator"), this);
    connect(runInAC, &QAction::triggered, this, &MainWindow::runInSimulator);
    settingsMenu->addAction(runInAC);
}

void MainWindow::setupHelpMenu()
{
    QMenu* helpMenu = m_menuBar->addMenu(tr("&Help"));

    QAction* aboutAct = new QAction(tr("&About ksEditor"), this);
    connect(aboutAct, &QAction::triggered, this, &MainWindow::showAbout);
    helpMenu->addAction(aboutAct);

    QAction* docsAct = new QAction(tr("&Documentation"), this);
    docsAct->setShortcut(Qt::Key_F1);
    helpMenu->addAction(docsAct);
}

void MainWindow::setupToolBar()
{
    m_mainToolBar = addToolBar(tr("Main Toolbar"));
    m_mainToolBar->setObjectName("MainToolBar");

    QAction* newAct = m_mainToolBar->addAction(tr("New"), this, &MainWindow::newProject);
    newAct->setToolTip(tr("New Project"));

    QAction* openAct = m_mainToolBar->addAction(tr("Open"), this, &MainWindow::openProject);
    openAct->setToolTip(tr("Open Project"));

    m_mainToolBar->addAction(m_saveAction);

    m_mainToolBar->addSeparator();

    m_mainToolBar->addAction(m_buildAction);

    m_mainToolBar->addSeparator();

    m_mainToolBar->addAction(tr("Undo"), this, &MainWindow::undo)->setToolTip(tr("Undo"));
    m_mainToolBar->addAction(tr("Redo"), this, &MainWindow::redo)->setToolTip(tr("Redo"));

    m_moduleToolBar = addToolBar(tr("Module Toolbar"));
    m_moduleToolBar->setObjectName("ModuleToolBar");
}

void MainWindow::setupStatusBar()
{
    m_statusBar = statusBar();

    m_simPathLabel = new QLabel(tr("Sim Path: Not detected"), this);
    m_simPathLabel->setTextFormat(Qt::PlainText);
    m_simPathLabel->setStyleSheet("color: #888; font-size: 11px; font-family: 'Consolas', monospace; padding: 0 8px;");
    m_statusBar->addPermanentWidget(m_simPathLabel);

    m_cspVersionLabel = new QLabel(tr("CSP: N/A"), this);
    m_cspVersionLabel->setTextFormat(Qt::PlainText);
    m_cspVersionLabel->setStyleSheet("color: #888; font-size: 11px; font-family: 'Consolas', monospace; padding: 0 8px;");
    m_statusBar->addPermanentWidget(m_cspVersionLabel);

    m_navSpeedLabel = new QLabel(tr("Speed: 1.0x"), this);
    m_navSpeedLabel->setStyleSheet("color: #888; font-size: 11px; font-family: 'Consolas', monospace; padding: 0 8px;");
    m_statusBar->addPermanentWidget(m_navSpeedLabel);

    m_snapStatusLabel = new QLabel(tr("Snap: Off"), this);
    m_snapStatusLabel->setStyleSheet("color: #888; font-size: 11px; font-family: 'Consolas', monospace; padding: 0 8px;");
    m_statusBar->addPermanentWidget(m_snapStatusLabel);

    m_placementStatusLabel = new QLabel(tr("Place: Off"), this);
    m_placementStatusLabel->setStyleSheet("color: #888; font-size: 11px; font-family: 'Consolas', monospace; padding: 0 8px;");
    m_statusBar->addPermanentWidget(m_placementStatusLabel);

    m_statusLabel = new QLabel(tr("Ready"), this);
    m_statusLabel->setStyleSheet("color: #aaa; padding: 0 4px;");
    m_statusBar->addWidget(m_statusLabel, 1);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setMaximumWidth(Constants::PROGRESS_BAR_WIDTH);
    m_progressBar->setVisible(false);
    m_statusBar->addPermanentWidget(m_progressBar);
}

void MainWindow::setStatusNavSpeed(const QString& text)
{
    if (m_navSpeedLabel) m_navSpeedLabel->setText(text);
}

void MainWindow::setStatusSnap(const QString& text)
{
    if (m_snapStatusLabel) m_snapStatusLabel->setText(text);
}

void MainWindow::setStatusPlacement(const QString& text)
{
    if (m_placementStatusLabel) m_placementStatusLabel->setText(text);
}

void MainWindow::setupDockWidgets()
{
    m_sidebarDock = new QDockWidget(tr("Sidebar"), this);
    m_sidebarDock->setObjectName("SidebarDock");
    m_sidebarDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_sidebarDock->setMinimumWidth(Constants::MIN_SIDEBAR_WIDTH);

    m_fileTree = new FileTreeWidget(this);
    m_sidebarDock->setWidget(m_fileTree);
    addDockWidget(Qt::LeftDockWidgetArea, m_sidebarDock);
    connect(m_fileTree, &FileTreeWidget::fileActivated,
            this, &MainWindow::onFileTreeActivated);

    m_propertiesDock = new QDockWidget(tr("Properties"), this);
    m_propertiesDock->setObjectName("PropertiesDock");
    m_propertiesDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    QWidget* propertiesWidget = new QWidget(this);
    propertiesWidget->setMinimumWidth(Constants::MIN_PROPERTIES_WIDTH);
    m_propertiesDock->setWidget(propertiesWidget);
    addDockWidget(Qt::RightDockWidgetArea, m_propertiesDock);

    m_outputDock = new QDockWidget(tr("Output"), this);
    m_outputDock->setObjectName("OutputDock");
    m_outputDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    m_outputDock->setMinimumHeight(Constants::MIN_OUTPUT_HEIGHT);

    QWidget* outputWidget = new QWidget(this);
    m_outputDock->setWidget(outputWidget);
    addDockWidget(Qt::BottomDockWidgetArea, m_outputDock);

    // Script console dock
    m_scriptConsoleDock = createScriptConsoleDock();
    addDockWidget(Qt::BottomDockWidgetArea, m_scriptConsoleDock);
    m_scriptConsoleDock->hide();

    // Search results dock
    m_projectSearch = new ProjectSearchWidget(this);
    m_searchDock = new QDockWidget(tr("Search Results"), this);
    m_searchDock->setObjectName("SearchResultsDock");
    m_searchDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    m_searchDock->setWidget(m_projectSearch);
    addDockWidget(Qt::BottomDockWidgetArea, m_searchDock);
    m_searchDock->hide();

    connect(m_projectSearch, &ProjectSearchWidget::resultActivated,
            this, &MainWindow::onSearchResultActivated);

    // Terminal dock
    m_terminal = new TerminalWidget(this);
    m_terminalDock = new QDockWidget(tr("Terminal"), this);
    m_terminalDock->setObjectName("TerminalDock");
    m_terminalDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    m_terminalDock->setMinimumHeight(Constants::MIN_OUTPUT_HEIGHT);
    m_terminalDock->setWidget(m_terminal);
    addDockWidget(Qt::BottomDockWidgetArea, m_terminalDock);
    m_terminalDock->hide();

    // Git status dock
    m_gitStatus = new GitStatusWidget(this);
    m_gitDock = new QDockWidget(tr("Source Control"), this);
    m_gitDock->setObjectName("GitDock");
    m_gitDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_gitDock->setWidget(m_gitStatus);
    addDockWidget(Qt::RightDockWidgetArea, m_gitDock);
    m_gitDock->hide();

    connect(m_gitStatus, &GitStatusWidget::openFileRequested, this, [this](const QString& filePath, int line) {
        if (filePath.isEmpty()) return;
        m_moduleManager->setCurrentModule("Text Editor");
        if (line > 0) {
            ks::TextEditorModule* te = qobject_cast<ks::TextEditorModule*>(m_moduleManager->currentEditorModule());
            if (te) te->openFileAtLine(filePath, line);
        } else {
            m_moduleManager->importFile(filePath);
        }
    });

    tabifyDockWidget(m_sidebarDock, m_propertiesDock);
    tabifyDockWidget(m_sidebarDock, m_gitDock);
}

QDockWidget* MainWindow::createScriptConsoleDock()
{
    QDockWidget* dock = new QDockWidget(tr("Script Console"), this);
    dock->setObjectName("ScriptConsoleDock");

    QWidget* consoleWidget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(consoleWidget);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    m_scriptOutput = new QPlainTextEdit(consoleWidget);
    m_scriptOutput->setReadOnly(true);
    m_scriptOutput->setStyleSheet(
        "QPlainTextEdit { background: #1e1e1e; color: #d4d4d4; font-family: 'Consolas', monospace; "
        "font-size: 12px; border: 1px solid #333; }"
    );
    m_scriptOutput->setMaximumBlockCount(1000);
    layout->addWidget(m_scriptOutput, 1);

    QHBoxLayout* inputLayout = new QHBoxLayout();
    m_scriptInput = new QLineEdit(consoleWidget);
    m_scriptInput->setStyleSheet(
        "QLineEdit { background: #2d2d2d; color: #d4d4d4; font-family: 'Consolas', monospace; "
        "font-size: 12px; border: 1px solid #555; padding: 4px; }"
    );
    m_scriptInput->setPlaceholderText(tr("Type JavaScript and press Enter..."));
    inputLayout->addWidget(m_scriptInput, 1);

    QPushButton* runBtn = new QPushButton(tr("Run"), consoleWidget);
    runBtn->setStyleSheet(
        "QPushButton { background: #0078d4; color: white; border: none; padding: 4px 12px; "
        "border-radius: 3px; } QPushButton:hover { background: #1a8ae8; }"
    );
    inputLayout->addWidget(runBtn);

    layout->addLayout(inputLayout);

    dock->setWidget(consoleWidget);
    dock->resize(400, 200);

    m_scriptEngine = new QJSEngine(this);

    connect(m_scriptInput, &QLineEdit::returnPressed, this, &MainWindow::executeScript);
    connect(runBtn, &QPushButton::clicked, this, &MainWindow::executeScript);

    return dock;
}

void MainWindow::toggleScriptConsole()
{
    if (m_scriptConsoleDock) {
        m_scriptConsoleDock->setVisible(!m_scriptConsoleDock->isVisible());
        if (m_scriptConsoleDock->isVisible()) {
            m_scriptInput->setFocus();
        }
    }
}

void MainWindow::toggleTerminal()
{
    if (m_terminalDock) {
        m_terminalDock->setVisible(!m_terminalDock->isVisible());
        if (m_terminalDock->isVisible() && m_terminal) {
            m_terminal->setFocus();
        }
    }
}

void MainWindow::toggleGit()
{
    if (m_gitDock) {
        m_gitDock->setVisible(!m_gitDock->isVisible());
        if (m_gitDock->isVisible() && m_gitStatus) {
            m_gitStatus->refresh();
        }
    }
}

void MainWindow::executeScript()
{
    QString code = m_scriptInput->text().trimmed();
    if (code.isEmpty()) return;

    m_scriptOutput->appendPlainText("> " + code);

    if (code == "clear" || code == "cls") {
        m_scriptOutput->clear();
        m_scriptInput->clear();
        return;
    }

    QJSValue result = m_scriptEngine->evaluate(code);
    if (result.isError()) {
        m_scriptOutput->appendPlainText("Error: " + result.toString());
    } else if (!result.isUndefined()) {
        m_scriptOutput->appendPlainText(result.toString());
    }

    m_scriptInput->clear();
}

void MainWindow::setupConnections()
{
    connect(m_moduleManager, &ModuleManager::moduleChanged, this, &MainWindow::onModuleChanged);
    connect(m_undoStack, &QUndoStack::canUndoChanged, m_undoAction, &QAction::setEnabled);
    connect(m_undoStack, &QUndoStack::canRedoChanged, m_redoAction, &QAction::setEnabled);
    connect(m_undoStack, &QUndoStack::cleanChanged, [this](bool clean) {
        updateWindowTitle();
    });

    connect(m_projectBuilder, &ProjectBuilder::progressUpdated, this, &MainWindow::onBuildProgress);
    connect(m_projectBuilder, &ProjectBuilder::buildComplete, this, &MainWindow::onBuildComplete);
}

void MainWindow::createActions()
{
    // Actions are created in their respective setup methods
    updateActions();

    // Connect undo/redo signals
    m_undoAction->setEnabled(false);
    m_redoAction->setEnabled(false);
}

void MainWindow::updateActions()
{
    const bool hasProject = !m_currentProjectPath.isEmpty();

    m_saveAction->setEnabled(hasProject);
    m_saveAsAction->setEnabled(hasProject);
    m_buildAction->setEnabled(hasProject);

    // Edit actions depend on focus widget or current module
    QWidget* focusWidget = QApplication::focusWidget();
    bool hasEditableText = (qobject_cast<QLineEdit*>(focusWidget) != nullptr);

    m_cutAction->setEnabled(hasProject && (hasEditableText || m_moduleManager->canCut()));
    m_copyAction->setEnabled(hasProject && (hasEditableText || m_moduleManager->canCopy()));
    m_pasteAction->setEnabled(hasProject && (hasEditableText || m_moduleManager->canPaste()));
    m_deleteAction->setEnabled(hasProject && m_moduleManager->canDelete());
}

// ==================== Path Detection ====================

QString MainWindow::detectSimulatorFromRegistry() const
{
#ifdef Q_OS_WIN
    QStringList keys = EditorConfig::instance().registryKeys();
    for (const QString& keyPath : keys) {
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyPath.toStdWString().c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            wchar_t path[MAX_PATH];
            DWORD size = sizeof(path);
            DWORD type = REG_SZ;

            if (RegQueryValueExW(hKey, L"InstallLocation", nullptr, &type,
                                 reinterpret_cast<LPBYTE>(path), &size) == ERROR_SUCCESS) {
                RegCloseKey(hKey);
                return QString::fromWCharArray(path);
            }
            RegCloseKey(hKey);
        }
    }
#endif
    return QString();
}

QStringList MainWindow::getDefaultSimulatorPaths() const
{
    return EditorConfig::instance().defaultSearchPaths();
}

void MainWindow::detectSimulator()
{
    QStringList possiblePaths;

    QString registryPath = detectSimulatorFromRegistry();
    if (!registryPath.isEmpty()) {
        possiblePaths << registryPath;
    }

    QString userPath = m_settings->value("ksPath").toString();
    if (!userPath.isEmpty()) {
        possiblePaths.prepend(userPath);
    }

    possiblePaths << getDefaultSimulatorPaths();

    possiblePaths.removeDuplicates();

    for (const QString& path : possiblePaths) {
        QDir dir(path);
        if (dir.exists() && dir.exists(EditorConfig::instance().simExeName())) {
            setSimPath(path);
            return;
        }
    }

    LOG_WARNING("MainWindow", "Simulator not found in standard locations");
    setStatusMessage(tr("Simulator not found. Please set path in Settings."), 5000);
}

void MainWindow::detectCSPVersion()
{
    if (m_simPath.isEmpty()) {
        setCSPVersion(tr("Not installed"));
        return;
    }

    QString extensionPath = m_simPath + "/extension";
    QDir dir(extensionPath);

    QStringList cspFiles = dir.entryList(QStringList() << "*.dll");
    for (const QString& file : cspFiles) {
        if (file.contains("custom_shaders_patch", Qt::CaseInsensitive)) {
            setCSPVersion(tr("Detected"));
            return;
        }
    }

    // Also check in settings.ini
    QSettings settings(m_simPath + "/cfg/settings.ini", QSettings::IniFormat);
    QString cspVersion = settings.value("CUSTOM_SHADERS_PATCH_VERSION", tr("Not installed")).toString();
    setCSPVersion(cspVersion);
}

// ==================== File Operations ====================

bool MainWindow::createProjectFile(const QString& path, const QString& name)
{
    ks::ProjectData data;
    data.name = name;
    data.version = "1.0";
    data.created = QDateTime::currentDateTime();
    data.modified = data.created;
    data.filePath = path;
    data.activeModule = m_moduleManager->moduleName(0);
    data.activeModuleIndex = 0;

    if (!ks::ProjectSerializer::instance().save(path, data)) {
        LOG_ERROR("MainWindow", "Failed to create project file: " + path);
        return false;
    }

    LOG_INFO("MainWindow", "Created project: " + path);
    return true;
}

bool MainWindow::loadProjectFile(const QString& path)
{
    ks::ProjectData data;
    if (!ks::ProjectSerializer::instance().load(path, data)) {
        LOG_ERROR("MainWindow", "Failed to load project: " + path);
        return false;
    }

    LOG_INFO("MainWindow", "Loaded project: " + data.name + " v" + data.version);

    for (auto* mod : m_moduleManager->modules()) {
        QString modId = mod->moduleId();
        if (data.modelerState.contains(modId)) {
            QJsonObject modState = QJsonObject::fromVariantMap(data.modelerState[modId].toMap());
            mod->deserializeProject(modState);
        }
    }

    if (data.modelerState.contains("scene")) {
        for (auto* mod : m_moduleManager->modules()) {
            if (auto* modeler = qobject_cast<ks::KSModelerModule*>(mod)) {
                QJsonObject sceneJson = QJsonObject::fromVariantMap(data.modelerState["scene"].toMap());
                ks::ProjectSerializer::deserializeScene(modeler->sceneGraph(), sceneJson);
                break;
            }
        }
    }

    if (data.activeModuleIndex >= 0 && data.activeModuleIndex < m_moduleManager->moduleCount()) {
        m_moduleManager->setCurrentModule(data.activeModuleIndex);
    }

    return true;
}

void MainWindow::newProject()
{
    QString projectName = QInputDialog::getText(this, tr("New Project"),
                                                tr("Project name:"));
    if (projectName.isEmpty()) return;

    QString projectDir = QFileDialog::getExistingDirectory(this, tr("Project Location"));
    if (projectDir.isEmpty()) return;

    QString projectPath = QString("%1/%2%3").arg(projectDir, projectName, Constants::PROJECT_EXTENSION);

    if (!createProjectFile(projectPath, projectName)) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to create project file"));
        return;
    }

    m_currentProjectPath = projectPath;
    addToRecentProjects(projectPath);
    updateWindowTitle();
    updateActions();

    if (m_fileTree) {
        QFileInfo projectFile(projectPath);
        m_fileTree->setRootPath(projectFile.absolutePath());
    }
    if (m_projectSearch) {
        QFileInfo projectFile(projectPath);
        m_projectSearch->setSearchRoot(projectFile.absolutePath());
    }
    if (m_gitStatus) {
        QFileInfo projectFile(projectPath);
        m_gitStatus->setRepoPath(projectFile.absolutePath());
    }

    emit projectOpened(projectPath);
    LOG_INFO("MainWindow", "Created new project: " + projectPath);
}

void MainWindow::openProject()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("Open Project"),
        QDir::homePath(),
        tr("ksEditor Project (*%1);;All Files (*)").arg(Constants::PROJECT_EXTENSION)
    );

    if (!filePath.isEmpty()) {
        openRecentProject(filePath);
    }
}

void MainWindow::openRecentProject(const QString& path)
{
    // Close current project first
    if (!closeProject()) {
        return;
    }

    if (!loadProjectFile(path)) {
        QMessageBox::warning(this, tr("Error"),
                           tr("Failed to open project: %1").arg(path));
        return;
    }

    m_currentProjectPath = path;
    addToRecentProjects(path);
    updateWindowTitle();
    updateActions();

    if (m_crashRecovery) {
        m_crashRecovery->addOpenDocument(path);
        m_crashRecovery->setActiveDocument(path);
    }

    // Set file tree root and search root to project directory
    QFileInfo projectFile(path);
    if (m_fileTree && projectFile.exists()) {
        m_fileTree->setRootPath(projectFile.absolutePath());
    }
    if (m_projectSearch && projectFile.exists()) {
        m_projectSearch->setSearchRoot(projectFile.absolutePath());
    }
    if (m_gitStatus && projectFile.exists()) {
        m_gitStatus->setRepoPath(projectFile.absolutePath());
    }

    emit projectOpened(path);
    LOG_INFO("MainWindow", "Opened project: " + path);
}

bool MainWindow::saveProject()
{
    if (m_currentProjectPath.isEmpty()) {
        return saveProjectAs();
    }

    return saveProjectFile(m_currentProjectPath);
}

bool MainWindow::saveProjectAs()
{
    QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Save Project As"),
        QDir::homePath(),
        tr("ksEditor Project (*%1)").arg(Constants::PROJECT_EXTENSION)
    );

    if (filePath.isEmpty()) return false;

    // Ensure extension
    if (!filePath.endsWith(Constants::PROJECT_EXTENSION, Qt::CaseInsensitive)) {
        filePath += Constants::PROJECT_EXTENSION;
    }

    if (saveProjectFile(filePath)) {
        m_currentProjectPath = filePath;
        addToRecentProjects(filePath);
        updateWindowTitle();
        return true;
    }

    return false;
}

bool MainWindow::saveProjectFile(const QString& path)
{
    LOG_INFO("MainWindow", "Saving project: " + path);

    ks::ProjectData data;
    data.filePath = path;
    data.modified = QDateTime::currentDateTime();
    data.activeModuleIndex = m_moduleManager->currentModule();

    QFileInfo info(path);
    data.name = info.completeBaseName();

    for (auto* mod : m_moduleManager->modules()) {
        QJsonObject modState = mod->serializeProject();
        if (!modState.isEmpty()) {
            data.modelerState[mod->moduleId()] = modState.toVariantMap();
        }
    }

    for (auto* mod : m_moduleManager->modules()) {
        if (auto* modeler = qobject_cast<ks::KSModelerModule*>(mod)) {
            data.modelerState["scene"] = ks::ProjectSerializer::serializeScene(modeler->sceneGraph());
            break;
        }
    }

    if (!ks::ProjectSerializer::instance().save(path, data)) {
        QMessageBox::warning(this, tr("Save Failed"),
                           tr("Could not save project to: %1").arg(path));
        return false;
    }

    m_undoStack->setClean();
    updateWindowTitle();
    setStatusMessage(tr("Project saved"), 2000);
    return true;
}

void MainWindow::buildProject()
{
    if (m_currentProjectPath.isEmpty()) {
        QMessageBox::information(this, tr("No Project"),
                               tr("Please create or open a project first."));
        return;
    }

    if (m_simPath.isEmpty()) {
        QMessageBox::warning(this, tr("Simulator Not Found"),
                           tr("Please set the simulator path in Settings."));
        return;
    }

    setStatusMessage(tr("Building project..."));
    setProgress(0);

    m_projectBuilder->build(m_currentProjectPath, m_simPath);
}

bool MainWindow::closeProject()
{
    if (m_currentProjectPath.isEmpty()) {
        return true;
    }

    if (!promptForUnsavedChanges()) {
        return false;
    }

    // Clear undo stack
    m_undoStack->clear();

    m_currentProjectPath.clear();
    updateWindowTitle();
    updateActions();

    if (m_fileTree) {
        m_fileTree->setRootPath(QDir::homePath());
    }
    if (m_projectSearch) {
        m_projectSearch->setSearchRoot(QString());
    }
    if (m_gitStatus) {
        m_gitStatus->setRepoPath(QString());
    }

    emit projectClosed();
    LOG_INFO("MainWindow", "Closed project");

    return true;
}

bool MainWindow::promptForUnsavedChanges()
{
    if (m_undoStack->isClean()) {
        return true;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, tr("Unsaved Changes"),
        tr("Save changes before closing?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
    );

    switch (reply) {
        case QMessageBox::Save:
            return saveProject();
        case QMessageBox::Discard:
            return true;
        default:
            return false;
    }
}

void MainWindow::addToRecentProjects(const QString& path)
{
    QStringList recent = m_settings->value("recentProjects").toStringList();
    recent.removeAll(path);
    recent.prepend(path);

    while (recent.size() > Constants::MAX_RECENT_PROJECTS) {
        recent.removeLast();
    }

    m_settings->setValue("recentProjects", recent);
    updateRecentProjectsMenu();
}

void MainWindow::updateRecentProjectsMenu()
{
    QStringList recent = m_settings->value("recentProjects").toStringList();

    for (int i = 0; i < m_recentProjectsActions.size(); ++i) {
        if (i < recent.size()) {
            QFileInfo info(recent[i]);
            QString displayText = QString("%1. %2").arg(i + 1).arg(info.fileName());
            m_recentProjectsActions[i]->setText(displayText);
            m_recentProjectsActions[i]->setData(recent[i]);
            m_recentProjectsActions[i]->setVisible(true);
            m_recentProjectsActions[i]->setToolTip(recent[i]);
        } else {
            m_recentProjectsActions[i]->setVisible(false);
        }
    }
}

// ==================== Module Switching ====================

bool MainWindow::switchToModule(int index)
{
    if (index < 0 || index >= m_moduleManager->moduleCount()) {
        LOG_ERROR("MainWindow", "Invalid module index: " + QString::number(index));
        return false;
    }

    m_moduleManager->setCurrentModule(index);
    updateWindowTitle();
    updateActions();
    return true;
}

bool MainWindow::switchToModule(const QString& moduleName)
{
    int index = m_moduleManager->moduleIndex(moduleName);
    if (index >= 0) {
        return switchToModule(index);
    }

    LOG_WARNING("MainWindow", "Module not found: " + moduleName);
    return false;
}

void MainWindow::onModuleChanged(int index)
{
    const QList<QAction*>& actions = m_moduleGroup->actions();
    if (index >= 0 && index < actions.size()) {
        actions[index]->setChecked(true);
    }

    emit moduleChanged(index);
    updateActions();
}

// ==================== Edit Operations ====================

void MainWindow::undo()
{
    if (m_undoStack->canUndo()) {
        m_undoStack->undo();
        setStatusMessage(tr("Undo"), 1000);
    }
}

void MainWindow::redo()
{
    if (m_undoStack->canRedo()) {
        m_undoStack->redo();
        setStatusMessage(tr("Redo"), 1000);
    }
}

void MainWindow::cut()
{
    QWidget* focusWidget = QApplication::focusWidget();
    if (auto* edit = qobject_cast<QLineEdit*>(focusWidget)) {
        edit->cut();
    } else if (m_moduleManager->currentEditorModule()) {
        m_moduleManager->currentEditorModule()->cut();
    }
}

void MainWindow::copy()
{
    QWidget* focusWidget = QApplication::focusWidget();
    if (auto* edit = qobject_cast<QLineEdit*>(focusWidget)) {
        edit->copy();
    } else if (m_moduleManager->currentEditorModule()) {
        m_moduleManager->currentEditorModule()->copy();
    }
}

void MainWindow::paste()
{
    QWidget* focusWidget = QApplication::focusWidget();
    if (auto* edit = qobject_cast<QLineEdit*>(focusWidget)) {
        edit->paste();
    } else if (m_moduleManager->currentEditorModule()) {
        m_moduleManager->currentEditorModule()->paste();
    }
}

void MainWindow::deleteSelected()
{
    if (m_moduleManager->currentEditorModule()) {
        m_moduleManager->currentEditorModule()->deleteSelected();
    }
}

// ==================== View Operations ====================

void MainWindow::toggleFullscreen()
{
    if (isFullScreen()) {
        showNormal();
    } else {
        showFullScreen();
    }
}

void MainWindow::toggleSidebar()
{
    if (m_sidebarDock) {
        m_sidebarDock->setVisible(!m_sidebarDock->isVisible());
    }
}

void MainWindow::onFileTreeActivated(const QString& filePath)
{
    if (filePath.isEmpty()) return;

    QFileInfo fi(filePath);
    QString suffix = fi.suffix().toLower();

    if (suffix == "ksep") {
        openRecentProject(filePath);
        return;
    }

    QStringList textExtensions = {"txt", "md", "log", "ini", "cfg", "conf",
        "json", "xml", "yaml", "yml", "toml", "csv", "env",
        "cpp", "c", "h", "hpp", "hxx", "cxx", "cs", "java", "kt",
        "py", "lua", "js", "ts", "rs", "go", "swift",
        "glsl", "vert", "frag", "comp", "css", "html", "htm"};

    if (textExtensions.contains(suffix)) {
        if (m_moduleManager->currentEditorModule() &&
            m_moduleManager->currentEditorModule()->moduleId() != "textEditor") {
            m_moduleManager->setCurrentModule("Text Editor");
        }
        m_moduleManager->importFile(filePath);
    } else if (suffix == "kn5" || suffix == "fbx" || suffix == "obj" ||
               suffix == "glb" || suffix == "gltf" || suffix == "dae" || suffix == "stl") {
        m_moduleManager->importFile(filePath);
    } else {
        QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
    }
}

void MainWindow::onSearchResultActivated(const QString& filePath, int lineNumber)
{
    if (filePath.isEmpty()) return;

    ks::TextEditorModule* textEditor = nullptr;
    for (auto* mod : m_moduleManager->modules()) {
        if (mod->moduleId() == "textEditor") {
            textEditor = qobject_cast<ks::TextEditorModule*>(mod);
            break;
        }
    }

    if (textEditor) {
        m_moduleManager->setCurrentModule("Text Editor");
        textEditor->openFileAtLine(filePath, lineNumber);
    }
}

void MainWindow::toggleStatusBar()
{
    if (m_statusBar) {
        m_statusBar->setVisible(!m_statusBar->isVisible());
    }
}

void MainWindow::toggleProperties()
{
    if (m_propertiesDock) {
        m_propertiesDock->setVisible(!m_propertiesDock->isVisible());
    }
}

void MainWindow::resetLayout()
{
    QSettings settings(Constants::APP_NAME, Constants::APP_NAME);
    settings.remove("geometry");
    settings.remove("windowState");

    QMessageBox::information(this, tr("Reset Layout"),
                           tr("Please restart the application to reset the layout."));
}

void MainWindow::saveWindowLayout()
{
    QSettings settings(Constants::APP_NAME, Constants::APP_NAME);
    settings.setValue("windowGeometry", saveGeometry());
    settings.setValue("windowState", saveState());
    setStatusMessage(tr("Window layout saved"), 2000);
}

void MainWindow::loadWindowLayout()
{
    QSettings settings(Constants::APP_NAME, Constants::APP_NAME);
    QByteArray geometry = settings.value("windowGeometry").toByteArray();
    QByteArray state = settings.value("windowState").toByteArray();
    if (!geometry.isEmpty()) restoreGeometry(geometry);
    if (!state.isEmpty()) restoreState(state);
}

void MainWindow::resetWindowLayout()
{
    resetLayout();
}

void MainWindow::showRecoveryDialog(const QVector<ks::CrashRecovery::Session>& sessions)
{
    if (sessions.isEmpty()) return;

    QString message = tr("Crash recovery data found for %1 session(s):\n\n").arg(sessions.size());
    for (const auto& session : sessions) {
        QDateTime dt = QDateTime::fromMSecsSinceEpoch(session.timestamp);
        message += tr("- %1: %2 document(s)\n").arg(dt.toString(Qt::ISODate)).arg(session.openDocuments.size());
    }
    message += tr("\nWould you like to recover?");

    auto reply = QMessageBox::question(this, tr("Crash Recovery"), message,
                                       QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes && !sessions.isEmpty()) {
        m_crashRecovery->recoverSession(sessions.first());
        setStatusMessage(tr("Session recovered"), 3000);
    }
}

// ==================== Settings ====================

void MainWindow::showSettings()
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Settings"));
    dlg.setMinimumWidth(500);
    dlg.setMinimumHeight(400);

    QVBoxLayout* mainLayout = new QVBoxLayout(&dlg);

    QTabWidget* tabs = new QTabWidget(&dlg);

    // General tab
    QWidget* generalTab = new QWidget;
    QFormLayout* generalLayout = new QFormLayout(generalTab);
    QLineEdit* simPathEdit = new QLineEdit(m_simPath, generalTab);
    QPushButton* browseBtn = new QPushButton("...", generalTab);
    browseBtn->setMaximumWidth(30);
    QHBoxLayout* simPathLayout = new QHBoxLayout;
    simPathLayout->addWidget(simPathEdit);
    simPathLayout->addWidget(browseBtn);
    generalLayout->addRow(tr("Simulator Path:"), simPathLayout);
    connect(browseBtn, &QPushButton::clicked, [&]() {
        QString dir = QFileDialog::getExistingDirectory(&dlg, tr("Select Simulator Path"), simPathEdit->text());
        if (!dir.isEmpty()) simPathEdit->setText(dir);
    });

    QLineEdit* cspVersionEdit = new QLineEdit(m_cspVersion, generalTab);
    generalLayout->addRow(tr("CSP Version:"), cspVersionEdit);

    QSpinBox* recentCountSpin = new QSpinBox(generalTab);
    recentCountSpin->setRange(3, 30);
    recentCountSpin->setValue(m_settings->integer("ui/recentProjectsMax", 10));
    generalLayout->addRow(tr("Max Recent Projects:"), recentCountSpin);

    QCheckBox* autoSaveCheck = new QCheckBox(generalTab);
    autoSaveCheck->setChecked(m_settings->boolean("editor/autoSave", true));
    generalLayout->addRow(tr("Enable Auto-Save:"), autoSaveCheck);

    QSpinBox* autoSaveInterval = new QSpinBox(generalTab);
    autoSaveInterval->setRange(1, 60);
    autoSaveInterval->setValue(m_settings->integer("editor/autoSaveInterval", 5));
    autoSaveInterval->setSuffix(" min");
    generalLayout->addRow(tr("Auto-Save Interval:"), autoSaveInterval);

    tabs->addTab(generalTab, tr("General"));

    // Appearance tab
    QWidget* appearanceTab = new QWidget;
    QFormLayout* appearanceLayout = new QFormLayout(appearanceTab);

    QComboBox* themeCombo = new QComboBox(appearanceTab);
    themeCombo->addItems({"Dark", "Light", "System"});
    themeCombo->setCurrentIndex(m_settings->integer("ui/theme", 0));
    appearanceLayout->addRow(tr("Theme:"), themeCombo);

    QFontComboBox* fontCombo = new QFontComboBox(appearanceTab);
    fontCombo->setCurrentFont(QFont(m_settings->string("ui/font", "Segoe UI")));
    appearanceLayout->addRow(tr("Font:"), fontCombo);

    QSpinBox* fontSizeSpin = new QSpinBox(appearanceTab);
    fontSizeSpin->setRange(8, 24);
    fontSizeSpin->setValue(m_settings->integer("ui/fontSize", 10));
    appearanceLayout->addRow(tr("Font Size:"), fontSizeSpin);

    tabs->addTab(appearanceTab, tr("Appearance"));

    // Editor tab
    QWidget* editorTab = new QWidget;
    QFormLayout* editorLayout = new QFormLayout(editorTab);

    QComboBox* gizmoCombo = new QComboBox(editorTab);
    gizmoCombo->addItems({tr("Translate"), tr("Rotate"), tr("Scale")});
    gizmoCombo->setCurrentIndex(m_settings->integer("editor/defaultGizmo", 0));
    editorLayout->addRow(tr("Default Gizmo:"), gizmoCombo);

    QDoubleSpinBox* snapSpin = new QDoubleSpinBox(editorTab);
    snapSpin->setRange(0.001, 10.0);
    snapSpin->setSingleStep(0.1);
    snapSpin->setDecimals(3);
    snapSpin->setValue(m_settings->real("editor/snapValue", 0.1));
    editorLayout->addRow(tr("Snap Value:"), snapSpin);

    QCheckBox* snapCheck = new QCheckBox(editorTab);
    snapCheck->setChecked(m_settings->boolean("editor/snapEnabled", false));
    editorLayout->addRow(tr("Enable Snapping:"), snapCheck);

    tabs->addTab(editorTab, tr("Editor"));

    mainLayout->addWidget(tabs);

    // Buttons
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply, &dlg);
    mainLayout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, [&]() {
        m_simPath = simPathEdit->text();
        m_cspVersion = cspVersionEdit->text();
        m_settings->setValue("simulator/path", m_simPath);
        m_settings->setValue("simulator/cspVersion", m_cspVersion);
        m_settings->setValue("ui/recentProjectsMax", recentCountSpin->value());
        m_settings->setValue("editor/autoSave", autoSaveCheck->isChecked());
        m_settings->setValue("editor/autoSaveInterval", autoSaveInterval->value());
        m_settings->setValue("ui/theme", themeCombo->currentIndex());
        m_settings->setValue("ui/font", fontCombo->currentFont().family());
        m_settings->setValue("ui/fontSize", fontSizeSpin->value());
        m_settings->setValue("editor/defaultGizmo", gizmoCombo->currentIndex());
        m_settings->setValue("editor/snapValue", snapSpin->value());
        m_settings->setValue("editor/snapEnabled", snapCheck->isChecked());
        m_settings->sync();

        // Update autosave timer
        if (m_autoSaveTimer) {
            if (autoSaveCheck->isChecked() && autoSaveInterval->value() > 0) {
                m_autoSaveTimer->start(autoSaveInterval->value() * 60 * 1000);
            } else {
                m_autoSaveTimer->stop();
            }
        }

        emit simPathChanged(m_simPath);
        dlg.accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    dlg.exec();
}

void MainWindow::showAbout()
{
    QString aboutText = tr(
        "<h3>ksEditor Qt</h3>"
        "<p>Version 1.16</p>"
        "<p>%1</p>"
        "<p>Built with Qt %2</p>"
        "<p>&copy; 2024 ksEditor Team</p>"
    ).arg(EditorConfig::instance().aboutDescription()).arg(qVersion());

    QMessageBox::about(this, tr("About ksEditor Qt"), aboutText);
}

void MainWindow::runInSimulator()
{
    if (m_simPath.isEmpty()) {
        QMessageBox::warning(this, tr("Simulator Not Found"),
                           tr("Please set the simulator path in Settings."));
        return;
    }

    QString exeName = EditorConfig::instance().simExeName();
    QString exePath = m_simPath + "/" + exeName;
    if (!QFile::exists(exePath)) {
        QMessageBox::warning(this, tr("Error"),
                           tr("Simulator executable not found in the specified path."));
        return;
    }

    bool started = QProcess::startDetached(exePath, QStringList(), m_simPath);
    if (started) {
        setStatusMessage(tr("Launched simulator"), 3000);
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Failed to launch simulator."));
    }
}

// ==================== Update Methods ====================

void MainWindow::updateWindowTitle()
{
    QString title = Constants::APP_NAME;

    if (!m_currentProjectPath.isEmpty()) {
        QFileInfo info(m_currentProjectPath);
        title += " - " + info.baseName();
    }

    int currentModule = m_moduleManager->currentModule();
    if (currentModule >= 0) {
        title += " [" + m_moduleManager->moduleName(currentModule) + "]";
    }

    if (!m_undoStack->isClean()) {
        title += " *";
    }

    setWindowTitle(title);
}

// ==================== Status Bar ====================

void MainWindow::setStatusMessage(const QString& message, int timeoutMs)
{
    m_statusLabel->setText(message);

    if (timeoutMs > 0) {
        QTimer::singleShot(timeoutMs, this, [this]() {
            if (m_statusLabel->text() != tr("Ready")) {
                m_statusLabel->setText(tr("Ready"));
            }
        });
    }
}

void MainWindow::setProgress(int value, int maximum)
{
    if (value < 0) {
        clearProgress();
        return;
    }

    m_progressBar->setVisible(true);
    m_progressBar->setMaximum(maximum);
    m_progressBar->setValue(value);
}

void MainWindow::clearProgress()
{
    m_progressBar->setVisible(false);
    m_progressBar->setValue(0);
}

// ==================== Build Progress ====================

void MainWindow::onBuildProgress(int percent)
{
    setProgress(percent);
    setStatusMessage(tr("Building... %1%").arg(percent));
}

void MainWindow::onBuildComplete(bool success, const QString& message)
{
    clearProgress();

    if (success) {
        setStatusMessage(tr("Build completed successfully"), 3000);
        QMessageBox::information(this, tr("Build Complete"), message);
    } else {
        setStatusMessage(tr("Build failed"), 5000);
        QMessageBox::warning(this, tr("Build Failed"), message);
    }
}

void MainWindow::performAutoSave()
{
    if (m_currentProjectPath.isEmpty()) return;

    if (!m_undoStack->isClean()) {
        LOG_INFO("MainWindow", "Auto-saving project...");
        saveProjectFile(m_currentProjectPath);
        setStatusMessage(tr("Project auto-saved"), 2000);
    }
}

void MainWindow::saveSessionBackup()
{
    if (m_crashRecovery) {
        m_crashRecovery->saveSession();
    }
}

void MainWindow::createProjectFromTemplate(const QString& templateId)
{
    if (!m_templateManager) return;

    if (!m_templateManager->hasTemplate(templateId)) {
        QMessageBox::warning(this, tr("Template Not Found"),
                           tr("Template '%1' not found.").arg(templateId));
        return;
    }

    QString outputDir = QFileDialog::getExistingDirectory(this, tr("Select Project Directory"));
    if (outputDir.isEmpty()) return;

    QString projectPath = m_templateManager->getProjectPathFromTemplate(templateId, outputDir);
    if (projectPath.isEmpty()) return;

    if (!m_templateManager->initializeTemplateProject(templateId, projectPath)) {
        QMessageBox::warning(this, tr("Error"),
                           tr("Failed to create project from template."));
        return;
    }

    openRecentProject(projectPath);
    setStatusMessage(tr("Project created from template"), 3000);
}

void MainWindow::showTemplateBrowser()
{
    if (!m_templateManager || !m_templateManager->isInitialized()) return;

    QDialog dlg(this);
    dlg.setWindowTitle(tr("Template Browser"));
    dlg.setMinimumSize(600, 400);

    QVBoxLayout* layout = new QVBoxLayout(&dlg);

    QListWidget* list = new QListWidget(&dlg);
    QVector<ks::TemplateManager::TemplateInfo> infos = m_templateManager->getAllTemplateInfos();
    for (const auto& info : infos) {
        QListWidgetItem* item = new QListWidgetItem(info.name, list);
        item->setData(Qt::UserRole, info.id);
        item->setToolTip(info.description);
    }
    layout->addWidget(list);

    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    connect(list, &QListWidget::itemDoubleClicked, &dlg, &QDialog::accept);

    if (dlg.exec() == QDialog::Accepted && list->currentItem()) {
        QString templateId = list->currentItem()->data(Qt::UserRole).toString();
        createProjectFromTemplate(templateId);
    }
}

void MainWindow::showFileDiff(const QString& filePath)
{
    if (!m_diffEngine || !m_diffEngine->isInitialized()) return;

    QString backupPath = filePath + ".bak";
    if (!QFile::exists(backupPath)) {
        QMessageBox::information(this, tr("No Backup"),
                               tr("No backup file found for comparison."));
        return;
    }

    ks::FileDiffResult result = m_diffEngine->compareFiles(backupPath, filePath);

    QDialog dlg(this);
    dlg.setWindowTitle(tr("File Diff: %1").arg(QFileInfo(filePath).fileName()));
    dlg.setMinimumSize(700, 500);

    QVBoxLayout* layout = new QVBoxLayout(&dlg);
    QPlainTextEdit* output = new QPlainTextEdit(&dlg);
    output->setReadOnly(true);
    output->setPlainText(m_diffEngine->generateHumanReadableReport({result}));
    layout->addWidget(output);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    dlg.exec();
}

void MainWindow::compareProjectFiles()
{
    if (!m_diffEngine || !m_diffEngine->isInitialized()) return;

    QString dir1 = QFileDialog::getExistingDirectory(this, tr("Select First Directory"));
    if (dir1.isEmpty()) return;

    QString dir2 = QFileDialog::getExistingDirectory(this, tr("Select Second Directory"));
    if (dir2.isEmpty()) return;

    QVector<ks::FileDiffResult> diffs = m_diffEngine->compareDirectories(dir1, dir2);

    QDialog dlg(this);
    dlg.setWindowTitle(tr("Directory Comparison"));
    dlg.setMinimumSize(700, 500);

    QVBoxLayout* layout = new QVBoxLayout(&dlg);
    QPlainTextEdit* output = new QPlainTextEdit(&dlg);
    output->setReadOnly(true);
    output->setPlainText(m_diffEngine->generateHumanReadableReport(diffs));
    layout->addWidget(output);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    dlg.exec();
}

void MainWindow::saveProjectLayout()
{
    saveWindowLayout();
}

void MainWindow::loadProjectLayout()
{
    loadWindowLayout();
}

void MainWindow::resetProjectLayout()
{
    resetWindowLayout();
}

// ==================== Simulator Path ====================

bool MainWindow::setSimPath(const QString& path)
{
    QDir dir(path);
    if (!dir.exists()) {
        LOG_ERROR("MainWindow", "Invalid simulator path: " + path);
        return false;
    }

    if (!dir.exists(EditorConfig::instance().simExeName())) {
        LOG_ERROR("MainWindow", "Simulator executable not found in: " + path);
        return false;
    }

    m_simPath = path;
    m_simPathLabel->setText(tr("Sim: %1").arg(QFileInfo(path).fileName()));

    m_settings->setValue("ksPath", path);
    emit simPathChanged(path);

    LOG_INFO("MainWindow", "Simulator path set: " + path);

    // Initialize simulator Vulkan Integration using KsPlugin
    if (KsIntegration::initialize(path + "/system")) {
        KsVulkanIntegration* ksVulkan = KsVulkanIntegration::instance();
        if (ksVulkan) {
            if (!ksVulkan->initialize(path + "/system")) {
                LOG_WARNING("MainWindow", "KS Vulkan Integration initialization returned false");
            } else {
                LOG_INFO("MainWindow", "KS Vulkan Integration initialized successfully");
                ksVulkan->applyGraphicsSettings();
                ksVulkan->applyLightingSettings();
            }
        }
    } else {
        LOG_ERROR("MainWindow", "Failed to initialize KS Config system");
    }

    // Re-detect CSP after path is set
    detectCSPVersion();

    return true;
}

void MainWindow::setCSPVersion(const QString& version)
{
    m_cspVersion = version;
    m_cspVersionLabel->setText(tr("CSP: %1").arg(version));
    emit cspVersionChanged(version);
}

// ==================== Protected Methods ====================

void MainWindow::closeEvent(QCloseEvent* event)
{
    // Cancel any running build
    if (m_projectBuilder && m_projectBuilder->isRunning()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, tr("Build in Progress"),
            tr("A build is currently in progress. Cancel and exit?"),
            QMessageBox::Yes | QMessageBox::No
        );

        if (reply == QMessageBox::Yes) {
            m_projectBuilder->cancel();
        } else {
            event->ignore();
            return;
        }
    }

    if (!promptForUnsavedChanges()) {
        event->ignore();
        return;
    }

    // Save window state
    QSettings settings(Constants::APP_NAME, Constants::APP_NAME);
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());

    event->accept();
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    // Ctrl+Tab for module switching
    if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_Tab) {
        int next = (m_moduleManager->currentModule() + 1) % m_moduleManager->moduleCount();
        switchToModule(next);
        event->accept();
        return;
    }

    QMainWindow::keyPressEvent(event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event)
{
    const QMimeData* mimeData = event->mimeData();

    if (!mimeData->hasUrls()) {
        return;
    }

    QList<QUrl> urlList = mimeData->urls();

    for (const QUrl& url : urlList) {
        QString filePath = url.toLocalFile();
        QFileInfo fi(filePath);
        QString suffix = fi.suffix().toLower();

        if (suffix == "ksep") {
            openRecentProject(filePath);
        } else if (suffix == "fbx" || suffix == "kn5") {
            if (!hasOpenProject()) {
                QMessageBox::warning(this, tr("No Project"),
                                   tr("Please open or create a project before importing files."));
                return;
            }
            m_moduleManager->importFile(filePath);
        } else if (suffix == "png" || suffix == "jpg" || suffix == "jpeg" ||
                   suffix == "bmp" || suffix == "tga" || suffix == "tiff") {
            QMessageBox::warning(this, tr("Unsupported Format"),
                               tr("Cannot read \"%1\" - this editor does not support image input.")
                               .arg(fi.fileName()));
        } else {
            QMessageBox::warning(this, tr("Unsupported Format"),
                               tr("Cannot read \"%1\" - unsupported file format.")
                               .arg(fi.fileName()));
        }
    }

    event->acceptProposedAction();
}
