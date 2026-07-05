#include "LiveryEditorModule.h"
#include "LiveryEditorWidget.h"
#include "../../core/sys/LogManager.h"
#include <QMainWindow>
#include <QFileInfo>
#include <QDebug>
#include <QFile>
#include <QDir>

namespace ks {

// ═══════════════════════════════════════════════════════════════════════
// LiveryEditor Implementation
// ═══════════════════════════════════════════════════════════════════════

LiveryEditor* LiveryEditor::s_instance = nullptr;

LiveryEditor::LiveryEditor(QObject* parent)
    : QObject(parent)
{
}

LiveryEditor* LiveryEditor::instance()
{
    if (!s_instance) {
        s_instance = new LiveryEditor();
    }
    return s_instance;
}

void LiveryEditor::setCarPath(const QString& path)
{
    if (m_carPath == path) return;
    m_carPath = path;

    if (m_liveryManager) {
        delete m_liveryManager;
        m_liveryManager = nullptr;
    }

    m_liveryManager = new LiveryManager(path);
    m_liveryManager->loadSkins();

    m_currentSkin.clear();
    m_config = LiverySystem::SkinConfig();

    emit skinListChanged();
}

bool LiveryEditor::loadSkins()
{
    if (!m_liveryManager) return false;
    m_liveryManager->loadSkins();
    emit skinListChanged();
    return true;
}

bool LiveryEditor::createSkin(const QString& name)
{
    if (!m_liveryManager) return false;
    bool ok = m_liveryManager->createSkin(name);
    if (ok) {
        emit skinListChanged();
    }
    return ok;
}

bool LiveryEditor::deleteSkin(const QString& name)
{
    if (!m_liveryManager) return false;
    bool ok = m_liveryManager->deleteSkin(name);
    if (ok) {
        if (m_currentSkin == name) {
            m_currentSkin.clear();
            m_config = LiverySystem::SkinConfig();
        }
        emit skinListChanged();
    }
    return ok;
}

bool LiveryEditor::duplicateSkin(const QString& sourceName, const QString& destName)
{
    if (!m_liveryManager) return false;
    bool ok = m_liveryManager->duplicateSkin(sourceName, destName);
    if (ok) {
        emit skinListChanged();
    }
    return ok;
}

bool LiveryEditor::setCurrentSkin(const QString& skinName)
{
    if (!m_liveryManager) return false;
    bool ok = m_liveryManager->setCurrentSkin(skinName);
    if (ok) {
        m_currentSkin = skinName;
        m_config = m_liveryManager->currentConfig();

        QString skinPath = m_carPath + "/skins/" + skinName;
        loadLiveryTexture(skinPath);

        emit skinLoaded(skinName);
    }
    return ok;
}

QStringList LiveryEditor::getSkinNames() const
{
    if (!m_liveryManager) return QStringList();

    QStringList names;
    const auto skins = m_liveryManager->getSkins();
    for (const auto& skin : skins) {
        names.append(skin.name);
    }
    return names;
}

bool LiveryEditor::loadLiveryTexture(const QString& skinPath)
{
    QString texturePath = skinPath + "/sides_1.png";
    if (!QFile::exists(texturePath)) {
        texturePath = skinPath + "/livery.png";
    }

    if (!QFile::exists(texturePath)) {
        qWarning() << "No livery texture found in:" << skinPath;
        return false;
    }

    QImage texture(texturePath);
    if (texture.isNull()) {
        qWarning() << "Failed to load livery texture:" << texturePath;
        return false;
    }

    m_liveryPainter.setTexture(texture);
    emit textureLoaded(texture);
    return true;
}

bool LiveryEditor::saveLiveryTexture(const QImage& texture, const QString& skinPath)
{
    if (texture.isNull()) {
        qWarning() << "Cannot save null texture";
        return false;
    }

    QString texturePath = skinPath + "/sides_1.png";
    bool saved = texture.save(texturePath, "PNG");
    if (!saved) {
        qWarning() << "Failed to save livery texture:" << texturePath;
        return false;
    }

    emit textureSaved(texturePath);
    return true;
}

bool LiveryEditor::addLayer(const LiverySystem::LiveryLayer& layer)
{
    bool ok = LiverySystem::addLayer(m_config, layer);
    if (ok) emit liveryModified();
    return ok;
}

bool LiveryEditor::removeLayer(int index)
{
    bool ok = LiverySystem::removeLayer(m_config, index);
    if (ok) emit liveryModified();
    return ok;
}

bool LiveryEditor::moveLayer(int fromIndex, int toIndex)
{
    bool ok = LiverySystem::moveLayer(m_config, fromIndex, toIndex);
    if (ok) emit liveryModified();
    return ok;
}

bool LiveryEditor::updateLayer(int index, const LiverySystem::LiveryLayer& layer)
{
    bool ok = LiverySystem::updateLayer(m_config, index, layer);
    if (ok) emit liveryModified();
    return ok;
}

bool LiveryEditor::generateLicensePlate(const QString& text, const QString& country)
{
    if (m_currentSkin.isEmpty()) return false;

    QString skinPath = m_carPath + "/skins/" + m_currentSkin;
    QString outputPath = skinPath + "/license_plate.png";

    bool ok = LiverySystem::generateLicensePlate(text, country, outputPath);
    if (ok) {
        LiverySystem::LiveryLayer layer;
        layer.name = "license_plate";
        layer.type = "decal";
        layer.opacity = 1.0f;
        layer.position[0] = 0.7f;
        layer.position[1] = 0.3f;
        layer.size[0] = 0.25f;
        layer.size[1] = 0.1f;
        layer.texturePath = outputPath;
        layer.visible = true;
        m_config.layers.append(layer);

        m_config.licensePlateText = text;
        m_config.licensePlateCountry = country;

        emit liveryModified();
    }
    return ok;
}

bool LiveryEditor::saveCurrentSkin()
{
    if (m_currentSkin.isEmpty()) return false;

    QString skinPath = m_carPath + "/skins/" + m_currentSkin;
    bool ok = LiverySystem::saveSkinConfig(m_config, skinPath);
    if (ok) {
        emit skinSaved(m_currentSkin);
    }
    return ok;
}

bool LiveryEditor::exportSkin(const QString& outputPath)
{
    if (m_currentSkin.isEmpty()) return false;

    QString skinPath = m_carPath + "/skins/" + m_currentSkin;
    return LiverySystem::exportSkin(skinPath, outputPath);
}

bool LiveryEditor::importSkin(const QString& importPath)
{
    if (m_carPath.isEmpty()) return false;

    bool ok = LiverySystem::importSkin(importPath, m_carPath);
    if (ok) {
        loadSkins();
    }
    return ok;
}

// ═══════════════════════════════════════════════════════════════════════
// LiveryEditorModule Implementation
// ═══════════════════════════════════════════════════════════════════════

LiveryEditorModule::LiveryEditorModule(QWidget* parent)
    : EditorModule(parent)
{
}

bool LiveryEditorModule::initialize()
{
    LOG_INFO("LiveryEditorModule", "Initializing Livery Editor module");
    return true;
}

void LiveryEditorModule::shutdown()
{
    LOG_INFO("LiveryEditorModule", "Shutting down Livery Editor module");
}

QDockWidget* LiveryEditorModule::getOrCreateDockWidget(QMainWindow* mainWindow)
{
    if (!m_dockWidget) {
        m_dockWidget = new QDockWidget("Livery Editor", mainWindow);
        m_dockWidget->setAllowedAreas(Qt::AllDockWidgetAreas);

        m_editorWidget = new LiveryEditorWidget(m_dockWidget);
        m_dockWidget->setWidget(m_editorWidget);
    }
    return m_dockWidget;
}

void LiveryEditorModule::importFile(const QString& filePath)
{
    if (!m_editorWidget) return;

    QFileInfo info(filePath);
    if (info.isDir()) {
        m_editorWidget->setCarPath(filePath);
    } else {
        m_editorWidget->setCarPath(info.absolutePath());
    }

    LOG_INFO("LiveryEditorModule", QString("Imported: %1").arg(filePath));
}

void LiveryEditorModule::exportFile(const QString& filePath)
{
    if (LiveryEditor* editor = LiveryEditor::instance()) {
        if (editor->exportSkin(filePath)) {
            LOG_INFO("LiveryEditorModule", QString("Exported to: %1").arg(filePath));
        } else {
            LOG_ERROR("LiveryEditorModule", QString("Export failed: %1").arg(filePath));
        }
    }
}

void LiveryEditorModule::onActivation()
{
    LOG_INFO("LiveryEditorModule", "Livery Editor activated");
}

void LiveryEditorModule::onDeactivation()
{
    LOG_INFO("LiveryEditorModule", "Livery Editor deactivated");
}

} // namespace ks
