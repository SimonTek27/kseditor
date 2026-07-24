#include "ModuleManager.h"
#include "../editor/EditorModule.h"
#include "LogManager.h"
#include "../../modules/modellingEditor/3DModeling_Module.h"
#include "../../modules/PhysicsEditor/PhysicsEditor.h"
#include "../assets/AssetsLibraryModule.h"
#include "../workshop/WorkshopEditorQmlBridge.h"
#include "../workshop/WorkshopEditorModule.h"
#include "../modmanager/ModManager.h"
#include "../../modules/LicensePlatesEditor/LicensePlateEditorModule.h"
#include "../modmanager/ContentRepair.h"
#include "../tools/FormatToolsQmlBridge.h"
#include "../FfbEditor/FfbEditorQmlBridge.h"
#include "../../modules/ShowroomEditor/ShowroomEditorModule.h"
#include "../../modules/LiveryEditor/LiveryEditorModule.h"
#include "../AIEditor/AIEditorModule.h"
#include "../eventEditor/championshipEditor/ChampionshipEditorModule.h"
#include "../textEditor/TextEditorModule.h"
#include "../../modules/PhysicsEditor/SetupEditor/SetupEditorQmlBridge.h"
#include "../../modules/PhysicsEditor/telemetry/TelemetryViewerQmlBridge.h"
#include "../weather/WeatherEditorModule.h"
#include "../../modules/modellingEditor/TrackBuilder/TrackSurfaceEditorModule.h"
#include "../../modules/modellingEditor/TrackBuilder/TrackMapEditorModule.h"
#include "../../modules/modellingEditor/TrackBuilder/TrackLightingEditorModule.h"
#include "../../modules/modellingEditor/TrackBuilder/DRSZoneEditorModule.h"
#include "../../modules/LiveryEditor/SkinIniEditorModule.h"
#include "../../modules/LiveryEditor/GUISkinEditorModule.h"
#include "../../modules/soundEditor/SoundEditorModule.h"
#include "../../core/formatToolsEditor/FormatToolsEditorModule.h"
#include "../../core/Scripting/luaScript/LuaScriptEditorModule.h"
#include "../../core/eventEditor/specialEventsEditor/SpecialEventsEditorModule.h"
#include "../../core/eventEditor/raceConfigEditor/RaceConfigEditorModule.h"
#include "../../core/eventEditor/careerEditor/CareerEditorModule.h"
#include "../../modules/ShowroomEditor/ShowroomPPEditorModule.h"
#include "../../modules/modellingEditor/TrackBuilder/cameratrackEditor/TrackCameraEditorModule.h"
#include "../../modules/modellingEditor/CarBuilder/cameracarEditor/CameraEditorModule.h"
#include "../../modules/modellingEditor/CharacterBuilder/DriverEditorModule.h"
#include "../../core/Config/CspConfigEditorModule.h"
#include "../../modules/fontEditor/FontCreatorQmlBridge.h"
#include "../../modules/displayEditor/DisplayEditorModule.h"
#include "../../core/ppfiltersEditor/PPFiltersQmlBridge.h"
#include "../../core/ServerConfigEditor/ServerConfigEditorModule.h"
#include "../../core/vcs/VcsEditorModule.h"
#include "../../core/archive/ArchiveEditorModule.h"
#include "../../modules/VREditor/VREditorModule.h"
#include "../../core/animation/AnimationEditorModule.h"
#include "../../core/3dprint/ThreeDPrintEditorModule.h"
#include "../../core/Audio/AudioEditorModule.h"
#include "../../core/material/MaterialEditorModule.h"
#include "../../core/network/NetworkEditorModule.h"
#include "../../core/mesh/MeshEditorModule.h"
#include "../../core/Graphics/GraphicsEditorModule.h"
#include "../../core/assets/AssetEditorModule.h"
#include "../../core/FileFormat/FileFormatEditorModule.h"
#include "../../core/help/HelpEditorModule.h"
#include "../../core/modmanager/ModManagerEditorModule.h"
#include "../../core/sys/SystemEditorModule.h"
#include "../../core/tools/ToolsEditorModule.h"
#include "../../core/ui/UIEditorModule.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QFileInfo>
#include <algorithm>

ModuleManager::ModuleManager(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadModules();

    LOG_INFO("ModuleManager", QString("Loaded %1 modules").arg(m_modules.size()));
}

ModuleManager::~ModuleManager()
{
    // Cleanup modules
    qDeleteAll(m_modules);
}

void ModuleManager::setupUI()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_stackedWidget = new QStackedWidget(this);
    layout->addWidget(m_stackedWidget);

    setLayout(layout);
}

void ModuleManager::loadModules()
{
    // ── Real modules ────────────────────────────────────────────────
    registerModule(new ks::ContentRepairModule(this));
    registerModule(new ks::KSModelerModule(this));
    registerModule(new ks::PhysicsEditorModule(this));
    registerModule(new ks::AssetsLibraryModule(this));
    registerModule(new ks::WorkshopEditorModule(this));
    registerModule(new ks::AIEditorModule(this));
    registerModule(new ks::ModManagerModule(this));
    registerModule(new ks::LicensePlateEditorModule(this));
    registerModule(new ks::FormatToolsModule(this));
    registerModule(new ks::FfbEditorModule(this));
    registerModule(new ks::ShowroomEditorModule(this));
    registerModule(new ks::LiveryEditorModule(this));
    registerModule(new ks::ChampionshipEditorModule(this));
    registerModule(new ks::TextEditorModule(this));
    registerModule(new ks::SetupEditorModule(this));
    registerModule(new ks::TelemetryViewerModule(this));
    registerModule(new ks::weather::WeatherEditorModule(this));
    registerModule(new ks::TrackSurfaceEditorModule(this));
    registerModule(new ks::TrackMapEditorModule(this));
    registerModule(new ks::TrackLightingEditorModule(this));
    registerModule(new ks::DRSZoneEditorModule(this));
    registerModule(new ks::SkinIniEditorModule(this));
    registerModule(new ks::GUISkinEditorModule(this));
    registerModule(new ks::TrackCameraEditorModule(this));
    registerModule(new ks::CameraEditorModule(this));
    registerModule(new ks::DriverEditorModule(this));
    registerModule(new ks::SoundEditorModule(this));
    registerModule(new ks::FormatToolsEditorModule(this));
    registerModule(new ks::LuaScriptEditorModule(this));
    registerModule(new ks::SpecialEventsEditorModule(this));
    registerModule(new ks::RaceConfigEditorModule(this));
    registerModule(new ks::CspConfigEditorModule(this));
    registerModule(new ks::FontCreatorEditorModule(this));
    registerModule(new ks::DisplayEditorModule(this));
    registerModule(new ks::PPFiltersEditorModule(this));
    registerModule(new ks::CareerEditorModule(this));
    registerModule(new ks::ServerConfigEditorModule(this));
    registerModule(new ks::ShowroomPPEditorModule(this));
    registerModule(new ks::VcsEditorModule(this));
    registerModule(new ks::ArchiveEditorModule(this));
    registerModule(new ks::VREditorModule(this));
    registerModule(new ks::AnimationEditorModule(this));
    registerModule(new ks::printing::ThreeDPrintEditorModule(this));
    registerModule(new ks::audio::AudioEditorModule(this));
    registerModule(new ks::material::MaterialEditorModule(this));
    registerModule(new ks::network::NetworkEditorModule(this));
    registerModule(new ks::mesh::MeshEditorModule(this));
    registerModule(new ks::graphics::GraphicsEditorModule(this));
    registerModule(new ks::assets::AssetEditorModule(this));
    registerModule(new ks::fileformat::FileFormatEditorModule(this));
    registerModule(new ks::help::HelpEditorModule(this));
    registerModule(new ks::modmanager::ModManagerEditorModule(this));
    registerModule(new ks::sys::SystemEditorModule(this));
    registerModule(new ks::tools::ToolsEditorModule(this));
    registerModule(new ks::ui::UIEditorModule(this));

    // Sort modules by priority (higher priority first)
    std::sort(m_modules.begin(), m_modules.end(), [](ks::EditorModule* a, ks::EditorModule* b) {
        return a->getModulePriority() > b->getModulePriority();
    });

    // Rebuild stacked widget in sorted order
    while (m_stackedWidget->count() > 0) {
        m_stackedWidget->removeWidget(m_stackedWidget->widget(0));
    }
    for (auto* mod : m_modules) {
        m_stackedWidget->addWidget(mod);
    }
}

QString ModuleManager::moduleName(int index) const
{
    if (index >= 0 && index < m_modules.size()) {
        QString name = m_modules[index]->moduleName();
        if (name.isEmpty()) name = m_modules[index]->getModuleName();
        return name;
    }
    return QString();
}

int ModuleManager::moduleIndex(const QString& name) const
{
    for (int i = 0; i < m_modules.size(); ++i) {
        QString modName = m_modules[i]->moduleName();
        if (modName.isEmpty()) modName = m_modules[i]->getModuleName();
        if (modName == name) {
            return i;
        }
    }
    return -1;
}

ks::EditorModule* ModuleManager::currentEditorModule() const
{
    return editorModule(currentModule());
}

ks::EditorModule* ModuleManager::editorModule(int index) const
{
    if (index >= 0 && index < m_modules.size()) {
        return m_modules[index];
    }
    return nullptr;
}

void ModuleManager::setCurrentModule(int index)
{
    if (index < 0 || index >= m_stackedWidget->count()) {
        return;
    }

    int current = currentModule();
    if (current != index) {
        emit moduleAboutToChange(current, index);

        // Deactivate old module
        if (current >= 0 && current < m_modules.size()) {
            m_modules[current]->onDeactivation();
        }

        m_stackedWidget->setCurrentIndex(index);

        // Activate new module
        if (index >= 0 && index < m_modules.size()) {
            m_modules[index]->onActivation();
        }

        emit moduleChanged(index);

        LOG_INFO("ModuleManager", QString("Switched to module: %1").arg(moduleName(index)));
    }
}

void ModuleManager::setCurrentModule(const QString& name)
{
    int index = moduleIndex(name);
    if (index >= 0) {
        setCurrentModule(index);
    }
}

void ModuleManager::registerModule(ks::EditorModule* module)
{
    if (module && !m_modules.contains(module)) {
        m_modules.append(module);
        m_stackedWidget->addWidget(module);
    }
}

void ModuleManager::unregisterModule(ks::EditorModule* module)
{
    if (module) {
        int index = m_modules.indexOf(module);
        if (index >= 0) {
            m_stackedWidget->removeWidget(module);
            m_modules.removeAt(index);
        }
    }
}

void ModuleManager::importFile(const QString& filePath)
{
    QString ext = QFileInfo(filePath).suffix().toLower();
    if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" || ext == "gif") {
        qWarning() << "Cannot import image file as model:" << filePath;
        return;
    }

    ks::EditorModule* current = currentEditorModule();
    if (current) {
        // Delegate to the current module if it supports import
        current->importFile(filePath);
        emit fileImported(filePath);
    }
}

void ModuleManager::exportFile(const QString& filePath)
{
    ks::EditorModule* current = currentEditorModule();
    if (current) {
        current->exportFile(filePath);
        emit fileExported(filePath);
    }
}

void ModuleManager::newProject(const QString& name, const QString& path)
{
    ks::EditorModule* current = currentEditorModule();
    if (current) {
        current->newProject(name, path);
        emit projectCreated(name, path);
    }
}

void ModuleManager::openProject(const QString& projectPath)
{
    ks::EditorModule* current = currentEditorModule();
    if (current) {
        current->openProject(projectPath);
        emit projectOpened(projectPath);
    }
}

void ModuleManager::saveProject()
{
    ks::EditorModule* current = currentEditorModule();
    if (current) {
        current->saveProject();
    }
}

bool ModuleManager::canCut() const
{
    ks::EditorModule* current = currentEditorModule();
    return current ? current->canCut() : false;
}

bool ModuleManager::canCopy() const
{
    ks::EditorModule* current = currentEditorModule();
    return current ? current->canCopy() : false;
}

bool ModuleManager::canPaste() const
{
    ks::EditorModule* current = currentEditorModule();
    return current ? current->canPaste() : false;
}

bool ModuleManager::canDelete() const
{
    ks::EditorModule* current = currentEditorModule();
    return current ? current->canDelete() : false;
}

void ModuleManager::buildCurrentProject()
{
    ks::EditorModule* current = currentEditorModule();
    if (current) {
        current->saveProject();
    }
    qWarning() << "ModuleManager::buildCurrentProject() is deprecated. Use MainWindow::buildProject() instead.";
}
