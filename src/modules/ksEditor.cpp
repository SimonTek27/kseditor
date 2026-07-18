#include "ksEditor.h"
#include "../core/mesh/RenderEngine.h"
#include "../modules/modellingEditor/3DModeling.h"
#include "../modules/modellingEditor/3DModeling_io.h"
#include "../modules/PhysicsEditor/PhysicsEngine.h"
#include "../plugins/simulators/kunos/KsPlugin.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>
#include <QFileDialog>
#include <QJsonArray>
#include <QStandardPaths>
#include <QSettings>
#include <QApplication>
#include <QFont>

namespace ks {

ksEditor::ksEditor(QObject* parent)
    : QObject(parent)
{
}

ksEditor::~ksEditor() { shutdown(); }

bool ksEditor::initialize()
{
    loadPreferences();

    m_audioStudio = new AudioStudio(this);

    m_scene = new geometry::Scene3D(this);
    m_renderer = new rendering::RenderEngine(this);
    m_importer = new io::ImportExport3D(this);

    m_physicsWorld = new physics::PhysicsWorld(this);
    m_carPhysics = new physics::CarPhysics(this);

    // Initialize new editor modules
    m_weatherEditor = new WeatherEditorModule(nullptr);
    m_trackSurfaceEditor = new TrackSurfaceEditorModule(nullptr);
    m_cameraEditor = new CameraEditorModule(nullptr);
    m_trackMapEditor = new TrackMapEditorModule(nullptr);
    m_driverEditor = new DriverEditorModule(nullptr);
    m_skinIniEditor = new SkinIniEditorModule(nullptr);
    m_showroomPPEditor = new ShowroomPPEditorModule(nullptr);
    m_trackLightingEditor = new TrackLightingEditorModule(nullptr);
    m_drsZoneEditor = new DRSZoneEditorModule(nullptr);
    m_trackCameraEditor = new TrackCameraEditorModule(nullptr);
    m_raceConfigEditor = new RaceConfigEditorModule(nullptr);
    m_specialEventsEditor = new SpecialEventsEditorModule(nullptr);
    m_careerEditor = new CareerEditorModule(nullptr);
    m_guiSkinEditor = new GUISkinEditorModule(nullptr);
    m_luaScriptEditor = new LuaScriptEditorModule(nullptr);
    m_pythonScriptEngine = new PythonScriptEngine(this);
    m_vrEditor = new VREditorModule(nullptr);

    m_weatherEditor->initialize();
    m_trackSurfaceEditor->initialize();
    m_cameraEditor->initialize();
    m_trackMapEditor->initialize();
    m_driverEditor->initialize();
    m_skinIniEditor->initialize();
    m_showroomPPEditor->initialize();
    m_trackLightingEditor->initialize();
    m_drsZoneEditor->initialize();
    m_trackCameraEditor->initialize();
    m_raceConfigEditor->initialize();
    m_specialEventsEditor->initialize();
    m_careerEditor->initialize();
    m_guiSkinEditor->initialize();
    m_luaScriptEditor->initialize();
    m_pythonScriptEngine->initialize();
    m_pythonScriptEngine->setScriptsDirectory(QCoreApplication::applicationDirPath() + "/scripts");
    m_vrEditor->initialize();

    m_undoStack.reserve(50);
    m_redoStack.reserve(50);

    emit statusMessage("ksEditor initialized");
    return true;
}

void ksEditor::shutdown()
{
    savePreferences();

    delete m_vrEditor;           m_vrEditor = nullptr;
    delete m_luaScriptEditor;    m_luaScriptEditor = nullptr;
    delete m_guiSkinEditor;      m_guiSkinEditor = nullptr;
    delete m_careerEditor;       m_careerEditor = nullptr;
    delete m_specialEventsEditor; m_specialEventsEditor = nullptr;
    delete m_raceConfigEditor;   m_raceConfigEditor = nullptr;
    delete m_drsZoneEditor;      m_drsZoneEditor = nullptr;
    delete m_trackCameraEditor;  m_trackCameraEditor = nullptr;
    delete m_trackLightingEditor; m_trackLightingEditor = nullptr;
    delete m_showroomPPEditor;   m_showroomPPEditor = nullptr;
    delete m_skinIniEditor;      m_skinIniEditor = nullptr;
    delete m_driverEditor;       m_driverEditor = nullptr;
    delete m_trackMapEditor;     m_trackMapEditor = nullptr;
    delete m_cameraEditor;       m_cameraEditor = nullptr;
    delete m_trackSurfaceEditor; m_trackSurfaceEditor = nullptr;
    delete m_weatherEditor;      m_weatherEditor = nullptr;

    delete m_carPhysics;        m_carPhysics = nullptr;
    delete m_physicsWorld;      m_physicsWorld = nullptr;
    delete m_importer;          m_importer = nullptr;
    delete m_scene;             m_scene = nullptr;
    delete m_renderer;          m_renderer = nullptr;
    delete m_audioStudio;       m_audioStudio = nullptr;
    delete m_currentAudioProject; m_currentAudioProject = nullptr;
}

void ksEditor::setMode(Mode mode)
{
    if (m_mode == mode) return;
    m_mode = mode;
    emit modeChanged(mode);
}

void ksEditor::createAudioProject(const QString& name)
{
    if (name.isEmpty()) {
        emit errorMessage("Project name cannot be empty");
        return;
    }
    
    // Create a new audio project
    delete m_currentAudioProject;
    m_currentAudioProject = new AudioProject(this);
    m_currentAudioProject->setName(name);
    
    emit audioProjectCreated(name);
    emit statusMessage("Audio project created: " + name);
}

void ksEditor::importAudio(const QString& path)
{
    if (!QFile::exists(path)) {
        emit errorMessage("File not found: " + path);
        return;
    }
    
    // Import audio file
    AudioManager audioManager;
    if (audioManager.importAudio(path, QStandardPaths::writableLocation(QStandardPaths::MusicLocation))) {
        emit statusMessage("Audio imported successfully: " + path);
        emit audioImported(path);
    } else {
        emit errorMessage("Failed to import audio: " + path);
    }
}

void ksEditor::exportAudioBank(const QString& path)
{
    if (!m_currentAudioProject) {
        emit errorMessage("No audio project to export");
        return;
    }
    emit audioBankExported(path);
    emit statusMessage("Audio bank exported: " + path);
}

void ksEditor::import3DModel(const QString& path)
{
    if (!QFile::exists(path)) {
        emit errorMessage("File not found: " + path);
        return;
    }
    
    // Import 3D model
    if (!m_importer) {
        emit errorMessage("3D import system not initialized");
        return;
    }
    auto result = m_importer->import(path);
    if (result.success && result.scene) {
        m_scene = result.scene;
        emit statusMessage("3D model imported successfully: " + path);
        emit modelImported(path);
    } else {
        emit errorMessage("Failed to import 3D model: " + path);
    }
}

void ksEditor::export3DModel(const QString& path, const QString& format)
{
    if (!m_scene || m_scene->allObjects().isEmpty()) {
        emit errorMessage("No 3D model to export");
        return;
    }
    
    // Export 3D model
    if (!m_importer) {
        emit errorMessage("3D export system not initialized");
        return;
    }
    io::ImportExport3D::Format fmt = io::ImportExport3D::OBJ;
    if (format == "fbx") fmt = io::ImportExport3D::FBX;
    else if (format == "gltf") fmt = io::ImportExport3D::GLTF;
    else if (format == "glb") fmt = io::ImportExport3D::GLB_FILE;
    else if (format == "stl") fmt = io::ImportExport3D::STL;
    else if (format == "dae") fmt = io::ImportExport3D::DAE;
    else if (format == "dxf") fmt = io::ImportExport3D::DXF;
    if (m_importer->exportScene(m_scene, path, fmt)) {
        emit statusMessage("3D model exported successfully: " + path);
        emit modelExported(path);
    } else {
        emit errorMessage("Failed to export 3D model: " + path);
    }
}

void ksEditor::simulate(float dt)
{
    // Update physics world
    if (m_physicsEnabled && m_physicsWorld) {
        m_physicsWorld->stepSimulation(dt);
    }

    // Drive car physics with default inputs for editor preview
    if (m_physicsEnabled && m_carPhysics && m_mode == Physics) {
        m_carPhysics->update(dt);
    }

    emit simulationUpdated(dt);
}

void ksEditor::exportToAC(const QString& outputDir)
{
    if (outputDir.isEmpty()) {
        emit errorMessage("Output directory cannot be empty");
        return;
    }
    
    if (!m_scene) {
        emit errorMessage("No scene to export");
        return;
    }
    
    emit acExported(outputDir);
    emit statusMessage("Exported to AC: " + outputDir);
}

void ksEditor::newProject(const QString& name)
{
    m_projectPath.clear();
    m_modified = false;
    m_undoStack.clear();
    m_redoStack.clear();
    emit statusMessage("New project: " + name);
}

bool ksEditor::loadProject(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        emit error("Failed to load project: " + path);
        return false;
    }
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull() || !doc.isObject()) {
        emit error("Invalid project file");
        return false;
    }
    m_projectPath = path;
    m_modified = false;
    emit projectLoaded(path);
    emit statusMessage("Loaded project: " + QFileInfo(path).fileName());
    return true;
}

bool ksEditor::saveProject(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        emit error("Failed to save project: " + path);
        return false;
    }
    QJsonObject root;
    root["mode"] = m_mode;
    QJsonArray recentArray;
    for (const QString& f : m_recentFiles) recentArray.append(f);
    root["recentFiles"] = recentArray;
    QJsonDocument doc(root);
    file.write(doc.toJson());
    m_projectPath = path;
    m_modified = false;
    emit projectSaved(path);
    emit statusMessage("Saved project: " + QFileInfo(path).fileName());
    return true;
}

void ksEditor::setRecentFiles(const QStringList& files) { m_recentFiles = files; }

void ksEditor::setPreference(const QString& key, const QVariant& value) { m_settings.setValue(key, value); }

QVariant ksEditor::getPreference(const QString& key, const QVariant& defaultValue) const { return m_settings.value(key, defaultValue); }

void ksEditor::onNewProject() { newProject("Untitled"); }

void ksEditor::onOpenProject()
{
    QString path = QFileDialog::getOpenFileName(nullptr, "Open Project", QString(), "ksEditor Project (*.ksproj);;All Files (*)");
    if (!path.isEmpty()) loadProject(path);
}

void ksEditor::onSaveProject()
{
    if (m_projectPath.isEmpty()) onExportAsset();
    else saveProject(m_projectPath);
}

void ksEditor::onImportAsset()
{
    QString path = QFileDialog::getOpenFileName(nullptr, "Import Asset", QString(),
        "All Supported (*.wav *.ogg *.mp3 *.obj *.fbx *.glb *.gltf *.kn5);;All Files (*)");
    if (path.isEmpty()) return;
    QString ext = QFileInfo(path).suffix().toLower();
    if (ext == "wav" || ext == "ogg" || ext == "mp3") importAudio(path);
    else import3DModel(path);
    emit statusMessage("Imported: " + QFileInfo(path).fileName());
}

void ksEditor::onExportAsset()
{
    QString path = QFileDialog::getSaveFileName(nullptr, "Save Project", QString(), "ksEditor Project (*.ksproj)");
    if (!path.isEmpty()) saveProject(path);
}

void ksEditor::pushUndoAction(const QString& description, const QJsonObject& state)
{
    QJsonObject captured = state;
    if (captured.isEmpty()) {
        captured["mode"] = static_cast<int>(m_mode);
        if (m_scene) {
            QJsonObject sceneState;
            sceneState["objectCount"] = m_scene->allObjects().size();
            captured["scene"] = sceneState;
        }
    }
    m_undoStack.append({description, captured});
    m_redoStack.clear();
    m_modified = true;
}

void ksEditor::onUndo()
{
    if (m_undoStack.isEmpty()) return;
    Action action = m_undoStack.takeLast();

    QJsonObject currentState;
    currentState["mode"] = static_cast<int>(m_mode);
    if (m_scene && m_scene->allObjects().size() > 0) {
        QJsonObject sceneState;
        sceneState["objectCount"] = m_scene->allObjects().size();
        currentState["scene"] = sceneState;
    }
    m_redoStack.append({action.description, currentState});

    if (action.state.contains("mode")) {
        m_mode = static_cast<Mode>(action.state["mode"].toInt());
    }
    m_modified = true;
    emit statusMessage("Undo: " + action.description);
    emit modeChanged(m_mode);
}

void ksEditor::onRedo()
{
    if (m_redoStack.isEmpty()) return;
    Action action = m_redoStack.takeLast();

    QJsonObject currentState;
    currentState["mode"] = static_cast<int>(m_mode);
    m_undoStack.append({action.description, currentState});

    if (action.state.contains("mode")) {
        m_mode = static_cast<Mode>(action.state["mode"].toInt());
    }
    m_modified = true;
    emit statusMessage("Redo: " + action.description);
    emit modeChanged(m_mode);
}

void ksEditor::onPreferences()
{
    QSettings s;
    bool ok;
    int fontSize = s.value("editor/fontSize", 12).toInt(&ok);
    if (ok && fontSize >= 8 && fontSize <= 72) {
        QFont f = qApp->font();
        f.setPointSize(fontSize);
        qApp->setFont(f);
    }
    emit statusMessage("Preferences updated");
}

void ksEditor::onModeAudio() { setMode(Audio); }
void ksEditor::onModeModel() { setMode(Model); }
void ksEditor::onModePhysics() { setMode(Physics); }
void ksEditor::onModeSetup() { setMode(Setup); }
void ksEditor::onModeTelemetry() { setMode(Telemetry); }
void ksEditor::onModeWorkshop() { setMode(Workshop); }
void ksEditor::onModeWeather() { setMode(Weather); }
void ksEditor::onModeTrackSurface() { setMode(TrackSurface); }
void ksEditor::onModeCamera() { setMode(Camera); }
void ksEditor::onModeTrackMap() { setMode(TrackMap); }
void ksEditor::onModeDriver() { setMode(Driver); }
void ksEditor::onModeSkinIni() { setMode(SkinIni); }
void ksEditor::onModeShowroomPP() { setMode(ShowroomPP); }
void ksEditor::onModeTrackLighting() { setMode(TrackLighting); }
void ksEditor::onModeDRSZone() { setMode(DRSZone); }
void ksEditor::onModeCameraTrack() { setMode(CameraTrack); }
void ksEditor::onModeRaceConfig() { setMode(RaceConfig); }
void ksEditor::onModeSpecialEvents() { setMode(SpecialEvents); }
void ksEditor::onModeCareer() { setMode(Career); }
void ksEditor::onModeGUISkin() { setMode(GUISkin); }
void ksEditor::onModeLuaScript() { setMode(LuaScript); }
void ksEditor::onModeVR() { setMode(VR); }

void ksEditor::setupConnections()
{
    m_undoStack.reserve(50);
    m_redoStack.reserve(50);
}

void ksEditor::loadPreferences()
{
    if (m_settings.contains("recentFiles"))
        m_recentFiles = m_settings.value("recentFiles").toStringList();
}

void ksEditor::savePreferences()
{
    m_settings.setValue("recentFiles", m_recentFiles);
}

} // namespace ks