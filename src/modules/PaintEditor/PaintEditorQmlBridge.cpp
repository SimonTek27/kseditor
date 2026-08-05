#include "PaintEditorQmlBridge.h"
#include "PaintEditorModule.h"
#include <QFileInfo>

namespace ks {
namespace paint {

PaintEditorQmlBridge* PaintEditorQmlBridge::s_instance = nullptr;

PaintEditorQmlBridge* PaintEditorQmlBridge::instance()
{
    if (!s_instance)
        s_instance = new PaintEditorQmlBridge();
    return s_instance;
}

bool PaintEditorQmlBridge::loadCar(const QString& path)
{
    m_carPath = path;
    m_manager = std::make_unique<PaintManager>(path);
    if (!m_manager->loadSkins()) return false;
    m_skins = m_manager->getSkins();
    emit carPathChanged();
    emit skinListChanged();
    ks::PaintEditor::instance()->setCarPath(path);
    return true;
}

QVariantList PaintEditorQmlBridge::getSkins()
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

bool PaintEditorQmlBridge::createSkin(const QString& name)
{
    if (!m_manager) return false;
    if (!m_manager->createSkin(name)) return false;
    m_skins = m_manager->getSkins();
    emit skinListChanged();
    return true;
}

bool PaintEditorQmlBridge::deleteSkin(const QString& name)
{
    if (!m_manager) return false;
    if (!m_manager->deleteSkin(name)) return false;
    m_skins = m_manager->getSkins();
    if (m_currentSkin == name) { m_currentSkin.clear(); emit currentSkinChanged(); }
    emit skinListChanged();
    return true;
}

bool PaintEditorQmlBridge::duplicateSkin(const QString& sourceName, const QString& destName)
{
    if (!m_manager) return false;
    if (!m_manager->duplicateSkin(sourceName, destName)) return false;
    m_skins = m_manager->getSkins();
    emit skinListChanged();
    return true;
}

bool PaintEditorQmlBridge::selectSkin(const QString& name)
{
    if (!m_manager) return false;
    if (!m_manager->setCurrentSkin(name)) return false;
    m_currentSkin = name;
    emit currentSkinChanged();
    emit skinLoaded();
    return true;
}

QVariantList PaintEditorQmlBridge::getLayers()
{
    QVariantList list;
    if (!m_manager) return list;
    const auto& config = m_manager->currentConfig();
    for (const auto& layer : config.layers) {
        list.append(layerToVariant(layer));
    }
    return list;
}

bool PaintEditorQmlBridge::addLayer(const QVariantMap& v)
{
    if (!m_manager) return false;
    PaintSystem::PaintLayer layer = variantToLayer(v);
    return m_manager->addLayer(layer);
}

bool PaintEditorQmlBridge::removeLayer(int index)
{
    if (!m_manager) return false;
    return m_manager->removeLayer(index);
}

bool PaintEditorQmlBridge::moveLayer(int from, int to)
{
    if (!m_manager) return false;
    return PaintSystem::moveLayer(m_manager->currentConfig(), from, to);
}

bool PaintEditorQmlBridge::updateLayer(int index, const QVariantMap& v)
{
    if (!m_manager) return false;
    PaintSystem::PaintLayer layer = variantToLayer(v);
    return PaintSystem::updateLayer(m_manager->currentConfig(), index, layer);
}

bool PaintEditorQmlBridge::exportSkin(const QString& outputPath)
{
return ks::PaintEditor::instance()->exportSkin(outputPath);
}

bool PaintEditorQmlBridge::importSkin(const QString& importPath)
{
    if (!m_manager) return false;
    if (!PaintSystem::importSkin(importPath, m_carPath)) return false;
    m_skins = m_manager->getSkins();
    emit skinListChanged();
    return true;
}

bool PaintEditorQmlBridge::generateLicensePlate(const QString& text, const QString& country)
{
return ks::PaintEditor::instance()->generateLicensePlate(text, country);
}

QStringList PaintEditorQmlBridge::getSupportedCountries()
{
    return PaintSystem::getSupportedCountries();
}

QVariantMap PaintEditorQmlBridge::getSkinConfig()
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

QVariantMap PaintEditorQmlBridge::layerToVariant(const PaintSystem::PaintLayer& layer) const
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

PaintSystem::PaintLayer PaintEditorQmlBridge::variantToLayer(const QVariantMap& v) const
{
    PaintSystem::PaintLayer layer;
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

} // namespace paint
} // namespace ks