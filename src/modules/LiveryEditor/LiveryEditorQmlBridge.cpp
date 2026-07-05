#include "LiveryEditorQmlBridge.h"
#include "LiveryEditorModule.h"
#include <QFileInfo>

namespace ks {

LiveryEditorQmlBridge* LiveryEditorQmlBridge::s_instance = nullptr;

LiveryEditorQmlBridge* LiveryEditorQmlBridge::instance()
{
    if (!s_instance)
        s_instance = new LiveryEditorQmlBridge();
    return s_instance;
}

bool LiveryEditorQmlBridge::loadCar(const QString& path)
{
    m_carPath = path;
    m_manager = std::make_unique<LiveryManager>(path);
    if (!m_manager->loadSkins()) return false;
    m_skins = m_manager->getSkins();
    emit carPathChanged();
    emit skinListChanged();
    LiveryEditor::instance()->setCarPath(path);
    return true;
}

QVariantList LiveryEditorQmlBridge::getSkins()
{
    QVariantList list;
    for (const auto& s : m_skins) {
        QVariantMap m;
        m["name"] = s.name;
        m["path"] = s.path;
        m["previewPath"] = s.previewPath;
        m["isValid"] = s.isValid;
        list.append(m);
    }
    return list;
}

bool LiveryEditorQmlBridge::createSkin(const QString& name)
{
    if (!m_manager) return false;
    if (!m_manager->createSkin(name)) return false;
    m_skins = m_manager->getSkins();
    emit skinListChanged();
    return true;
}

bool LiveryEditorQmlBridge::deleteSkin(const QString& name)
{
    if (!m_manager) return false;
    if (!m_manager->deleteSkin(name)) return false;
    m_skins = m_manager->getSkins();
    if (m_currentSkin == name) { m_currentSkin.clear(); emit currentSkinChanged(); }
    emit skinListChanged();
    return true;
}

bool LiveryEditorQmlBridge::duplicateSkin(const QString& sourceName, const QString& destName)
{
    if (!m_manager) return false;
    if (!m_manager->duplicateSkin(sourceName, destName)) return false;
    m_skins = m_manager->getSkins();
    emit skinListChanged();
    return true;
}

bool LiveryEditorQmlBridge::selectSkin(const QString& name)
{
    if (!m_manager) return false;
    if (!m_manager->setCurrentSkin(name)) return false;
    m_currentSkin = name;
    emit currentSkinChanged();
    emit skinLoaded();
    return true;
}

QVariantList LiveryEditorQmlBridge::getLayers()
{
    QVariantList list;
    if (!m_manager) return list;
    const auto& config = m_manager->currentConfig();
    for (const auto& layer : config.layers) {
        list.append(layerToVariant(layer));
    }
    return list;
}

bool LiveryEditorQmlBridge::addLayer(const QVariantMap& v)
{
    if (!m_manager) return false;
    LiverySystem::LiveryLayer layer = variantToLayer(v);
    return m_manager->addLayer(layer);
}

bool LiveryEditorQmlBridge::removeLayer(int index)
{
    if (!m_manager) return false;
    return m_manager->removeLayer(index);
}

bool LiveryEditorQmlBridge::moveLayer(int from, int to)
{
    if (!m_manager) return false;
    return LiverySystem::moveLayer(m_manager->currentConfig(), from, to);
}

bool LiveryEditorQmlBridge::updateLayer(int index, const QVariantMap& v)
{
    if (!m_manager) return false;
    LiverySystem::LiveryLayer layer = variantToLayer(v);
    return LiverySystem::updateLayer(m_manager->currentConfig(), index, layer);
}

bool LiveryEditorQmlBridge::exportSkin(const QString& outputPath)
{
    return LiveryEditor::instance()->exportSkin(outputPath);
}

bool LiveryEditorQmlBridge::importSkin(const QString& importPath)
{
    if (!m_manager) return false;
    if (!LiverySystem::importSkin(importPath, m_carPath)) return false;
    m_skins = m_manager->getSkins();
    emit skinListChanged();
    return true;
}

bool LiveryEditorQmlBridge::generateLicensePlate(const QString& text, const QString& country)
{
    return LiveryEditor::instance()->generateLicensePlate(text, country);
}

QStringList LiveryEditorQmlBridge::getSupportedCountries()
{
    return LiverySystem::getSupportedCountries();
}

QVariantMap LiveryEditorQmlBridge::getSkinConfig()
{
    QVariantMap m;
    if (!m_manager) return m;
    const auto& config = m_manager->currentConfig();
    m["name"] = config.name;
    m["path"] = config.path;
    m["baseColor"] = config.baseColor;
    m["licensePlateText"] = config.licensePlateText;
    m["licensePlateCountry"] = config.licensePlateCountry;
    m["hasNumber"] = config.hasNumber;
    m["carNumber"] = config.carNumber;
    m["driverName"] = config.driverName;
    m["teamName"] = config.teamName;
    return m;
}

QVariantMap LiveryEditorQmlBridge::layerToVariant(const LiverySystem::LiveryLayer& layer) const
{
    QVariantMap m;
    m["name"] = layer.name;
    m["type"] = layer.type;
    m["opacity"] = layer.opacity;
    m["posX"] = layer.position[0];
    m["posY"] = layer.position[1];
    m["sizeX"] = layer.size[0];
    m["sizeY"] = layer.size[1];
    m["rotation"] = layer.rotation;
    m["texturePath"] = layer.texturePath;
    m["tintColor"] = layer.tintColor.name();
    m["visible"] = layer.visible;
    return m;
}

LiverySystem::LiveryLayer LiveryEditorQmlBridge::variantToLayer(const QVariantMap& v) const
{
    LiverySystem::LiveryLayer layer;
    layer.name = v.value("name").toString();
    layer.type = v.value("type").toString();
    layer.opacity = v.value("opacity", 1.0f).toFloat();
    layer.position[0] = v.value("posX", 0.0f).toFloat();
    layer.position[1] = v.value("posY", 0.0f).toFloat();
    layer.size[0] = v.value("sizeX", 1.0f).toFloat();
    layer.size[1] = v.value("sizeY", 1.0f).toFloat();
    layer.rotation = v.value("rotation", 0.0f).toFloat();
    layer.texturePath = v.value("texturePath").toString();
    layer.tintColor = QColor(v.value("tintColor", "#ffffff").toString());
    layer.visible = v.value("visible", true).toBool();
    return layer;
}

} // namespace ks
