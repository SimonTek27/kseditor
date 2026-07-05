#include "LiverySystem.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QDateTime>
#include <QUuid>
#include <QImage>
#include <QPainter>
#include <QRegularExpression>
#include "../LicensePlatesEditor/LicensePlateEditorModule.h"

// ============================================================================
// LiverySystem — Skin management
// ============================================================================

QVector<LiverySystem::SkinInfo> LiverySystem::getSkins(const QString& carPath)
{
    QVector<SkinInfo> skins;
    QDir skinsDir(carPath + "/skins");
    if (!skinsDir.exists()) return skins;

    const auto entries = skinsDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo& entry : entries) {
        if (entry.isDir()) {
            SkinInfo info = getSkinInfo(entry.absoluteFilePath());
            if (info.isValid) {
                skins.append(info);
            }
        }
    }
    return skins;
}

LiverySystem::SkinInfo LiverySystem::getSkinInfo(const QString& skinPath)
{
    SkinInfo info;
    info.path = skinPath;
    info.name = QFileInfo(skinPath).fileName();
    info.isValid = false;

    QFile iniFile(skinPath + "/skin.ini");
    if (iniFile.exists()) {
        info.isValid = true;
        info.size = 0;

        QFileInfo dirInfo(skinPath);
        info.lastModified = dirInfo.lastModified();

        if (iniFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&iniFile);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (line.toUpper().startsWith("NAME=")) {
                    info.name = line.mid(5).trimmed();
                }
            }
            iniFile.close();
        }

        QString previewPath = getPreviewPath(skinPath);
        info.previewPath = previewPath;
    }

    return info;
}

bool LiverySystem::createSkin(const QString& carPath, const QString& skinName)
{
    QString skinPath = carPath + "/skins/" + skinName;
    QDir dir;
    if (!dir.mkpath(skinPath)) {
        qWarning() << "Failed to create skin directory:" << skinPath;
        return false;
    }

    return createDefaultFiles(skinPath);
}

bool LiverySystem::deleteSkin(const QString& carPath, const QString& skinName)
{
    if (isDefaultSkin(skinName)) {
        qWarning() << "Cannot delete default skin";
        return false;
    }

    QString skinPath = carPath + "/skins/" + skinName;
    QDir dir(skinPath);
    if (!dir.exists()) return false;

    return dir.removeRecursively();
}

bool LiverySystem::duplicateSkin(const QString& carPath, const QString& sourceName, const QString& destName)
{
    QString srcPath = carPath + "/skins/" + sourceName;
    QString dstPath = carPath + "/skins/" + destName;

    QDir srcDir(srcPath);
    if (!srcDir.exists()) return false;

    QDir dstDir;
    if (!dstDir.mkpath(dstPath)) return false;

    const auto files = srcDir.entryInfoList(QDir::Files);
    for (const QFileInfo& file : files) {
        if (!QFile::copy(file.absoluteFilePath(), dstPath + "/" + file.fileName())) {
            qWarning() << "Failed to copy file:" << file.absoluteFilePath();
            return false;
        }
    }

    QFile iniFile(dstPath + "/skin.ini");
    if (iniFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
        QString content = QString::fromUtf8(iniFile.readAll());
        content.replace(QRegularExpression("NAME=.*"), "NAME=" + destName);
        iniFile.resize(0);
        QTextStream out(&iniFile);
        out << content;
        iniFile.close();
    }

    return true;
}

// ============================================================================
// LiverySystem — Skin configuration (INI)
// ============================================================================

LiverySystem::SkinConfig LiverySystem::loadSkinConfig(const QString& skinPath)
{
    SkinConfig config;
    config.path = skinPath;
    config.name = QFileInfo(skinPath).fileName();

    loadFromIni(config, skinPath + "/skin.ini");

    QFile layersFile(skinPath + "/skin_layers.json");
    if (layersFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(layersFile.readAll());
        layersFile.close();

        QJsonObject root = doc.object();
        config.baseColor = root["baseColor"].toString("#ffffff");
        config.licensePlateText = root["licensePlateText"].toString();
        config.licensePlateCountry = root["licensePlateCountry"].toString();
        config.hasNumber = root["hasNumber"].toBool(false);
        config.carNumber = root["carNumber"].toInt(0);
        config.driverName = root["driverName"].toString();
        config.teamName = root["teamName"].toString();

        QJsonArray layersArr = root["layers"].toArray();
        for (const QJsonValue& val : layersArr) {
            QJsonObject obj = val.toObject();
            LiveryLayer layer;
            layer.name = obj["name"].toString();
            layer.type = obj["type"].toString("decal");
            layer.opacity = obj["opacity"].toDouble(1.0);
            layer.position[0] = obj["posX"].toDouble(0);
            layer.position[1] = obj["posY"].toDouble(0);
            layer.size[0] = obj["sizeW"].toDouble(1);
            layer.size[1] = obj["sizeH"].toDouble(1);
            layer.rotation = obj["rotation"].toDouble(0);
            layer.texturePath = obj["texturePath"].toString();
            layer.tintColor = QColor(obj["tintColor"].toString("#ffffff"));
            layer.visible = obj["visible"].toBool(true);
            config.layers.append(layer);
        }
    }

    return config;
}

bool LiverySystem::saveSkinConfig(const SkinConfig& config, const QString& skinPath)
{
    bool ok = saveToIni(config, skinPath + "/skin.ini");

    QJsonObject root;
    root["baseColor"] = config.baseColor;
    root["licensePlateText"] = config.licensePlateText;
    root["licensePlateCountry"] = config.licensePlateCountry;
    root["hasNumber"] = config.hasNumber;
    root["carNumber"] = config.carNumber;
    root["driverName"] = config.driverName;
    root["teamName"] = config.teamName;

    QJsonArray layersArr;
    for (const LiveryLayer& layer : config.layers) {
        QJsonObject obj;
        obj["name"] = layer.name;
        obj["type"] = layer.type;
        obj["opacity"] = static_cast<double>(layer.opacity);
        obj["posX"] = static_cast<double>(layer.position[0]);
        obj["posY"] = static_cast<double>(layer.position[1]);
        obj["sizeW"] = static_cast<double>(layer.size[0]);
        obj["sizeH"] = static_cast<double>(layer.size[1]);
        obj["rotation"] = static_cast<double>(layer.rotation);
        obj["texturePath"] = layer.texturePath;
        obj["tintColor"] = layer.tintColor.name();
        obj["visible"] = layer.visible;
        layersArr.append(obj);
    }
    root["layers"] = layersArr;

    QFile layersFile(skinPath + "/skin_layers.json");
    if (layersFile.open(QIODevice::WriteOnly)) {
        layersFile.write(QJsonDocument(root).toJson());
        layersFile.close();
    } else {
        qWarning() << "Failed to write skin layers:" << skinPath;
        ok = false;
    }

    return ok;
}

bool LiverySystem::loadFromIni(SkinConfig& config, const QString& iniPath)
{
    QFile file(iniPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream in(&file);
    bool inSkinSection = false;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        if (line.isEmpty() || line.startsWith(';') || line.startsWith('#')) continue;

        if (line.startsWith('[')) {
            inSkinSection = line.toUpper().startsWith("[SKIN]");
            continue;
        }

        if (inSkinSection) {
            int eq = line.indexOf('=');
            if (eq > 0) {
                QString key = line.left(eq).trimmed().toUpper();
                QString value = line.mid(eq + 1).trimmed();

                if (key == "NAME") config.name = value;
                else if (key == "PRIORITY") { /* stored but not in config struct */ }
                else if (key == "BASE_COLOR") config.baseColor = value;
                else if (key == "LICENSE_PLATE") config.licensePlateText = value;
                else if (key == "COUNTRY") config.licensePlateCountry = value;
                else if (key == "NUMBER") { config.hasNumber = true; config.carNumber = value.toInt(); }
                else if (key == "DRIVER") config.driverName = value;
                else if (key == "TEAM") config.teamName = value;
            }
        }
    }

    file.close();
    return true;
}

bool LiverySystem::saveToIni(const SkinConfig& config, const QString& iniPath)
{
    QFile file(iniPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out << "[SKIN]\n";
    out << "NAME=" << config.name << "\n";
    out << "PRIORITY=0\n";

    if (!config.baseColor.isEmpty()) {
        out << "BASE_COLOR=" << config.baseColor << "\n";
    }
    if (!config.licensePlateText.isEmpty()) {
        out << "LICENSE_PLATE=" << config.licensePlateText << "\n";
    }
    if (!config.licensePlateCountry.isEmpty()) {
        out << "COUNTRY=" << config.licensePlateCountry << "\n";
    }
    if (config.hasNumber) {
        out << "NUMBER=" << config.carNumber << "\n";
    }
    if (!config.driverName.isEmpty()) {
        out << "DRIVER=" << config.driverName << "\n";
    }
    if (!config.teamName.isEmpty()) {
        out << "TEAM=" << config.teamName << "\n";
    }

    file.close();
    return true;
}

// ============================================================================
// LiverySystem — Layer operations
// ============================================================================

bool LiverySystem::addLayer(SkinConfig& config, const LiveryLayer& layer)
{
    if (!validateLayer(layer)) return false;
    config.layers.append(layer);
    return true;
}

bool LiverySystem::removeLayer(SkinConfig& config, int index)
{
    if (index < 0 || index >= config.layers.size()) return false;
    config.layers.removeAt(index);
    return true;
}

bool LiverySystem::moveLayer(SkinConfig& config, int fromIndex, int toIndex)
{
    if (fromIndex < 0 || fromIndex >= config.layers.size()) return false;
    if (toIndex < 0 || toIndex >= config.layers.size()) return false;
    if (fromIndex == toIndex) return true;

    config.layers.move(fromIndex, toIndex);
    return true;
}

bool LiverySystem::updateLayer(SkinConfig& config, int index, const LiveryLayer& layer)
{
    if (index < 0 || index >= config.layers.size()) return false;
    if (!validateLayer(layer)) return false;
    config.layers[index] = layer;
    return true;
}

// ============================================================================
// LiverySystem — License plate
// ============================================================================

bool LiverySystem::generateLicensePlate(const QString& text, const QString& country,
                                         const QString& outputPath)
{
    ks::LicensePlatesManager manager;
    if (!manager.validatePlateText(text, country)) {
        qWarning() << "Invalid plate text:" << text << "for country:" << country;
        return false;
    }

    ks::PlateGenerationParams params;
    params.text = text;
    params.countryCode = country;
    params.width = 512;
    params.height = 128;

    ks::LicensePlateResult result = manager.generatePlateSimple(params);
    if (!result.success) {
        qWarning() << "Failed to generate license plate";
        return false;
    }

    return manager.savePlateTexture(result, outputPath);
}

QStringList LiverySystem::getSupportedCountries()
{
    ks::LicensePlatesManager manager;
    return manager.availableCountries();
}

bool LiverySystem::isValidPlateText(const QString& text, const QString& country)
{
    ks::LicensePlatesManager manager;
    return manager.validatePlateText(text, country);
}

// ============================================================================
// LiverySystem — Preview generation
// ============================================================================

bool LiverySystem::generatePreview(const QString& skinPath)
{
    SkinConfig config = loadSkinConfig(skinPath);
    QString previewPath = getPreviewPath(skinPath);

    QImage preview(512, 512, QImage::Format_ARGB32);
    preview.fill(Qt::white);

    QPainter painter(&preview);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor base(config.baseColor);
    if (!config.baseColor.isEmpty() && base.isValid()) {
        painter.fillRect(preview.rect(), base);
    }

    for (const LiveryLayer& layer : config.layers) {
        if (!layer.visible) continue;
        if (layer.texturePath.isEmpty()) continue;

        QImage layerTex(layer.texturePath);
        if (layerTex.isNull()) continue;

        int x = static_cast<int>(layer.position[0] * preview.width());
        int y = static_cast<int>(layer.position[1] * preview.height());
        int w = static_cast<int>(layer.size[0] * preview.width());
        int h = static_cast<int>(layer.size[1] * preview.height());

        QRect targetRect(x, y, w, h);
        painter.setOpacity(layer.opacity);
        painter.drawImage(targetRect, layerTex);
    }

    painter.end();

    if (!preview.save(previewPath, "JPG", 85)) {
        qWarning() << "Failed to save preview:" << previewPath;
        return false;
    }

    return true;
}

bool LiverySystem::hasPreview(const QString& skinPath)
{
    return QFile::exists(getPreviewPath(skinPath));
}

QString LiverySystem::getPreviewPath(const QString& skinPath)
{
    return skinPath + "/preview.jpg";
}

// ============================================================================
// LiverySystem — Export / Import
// ============================================================================

bool LiverySystem::exportSkin(const QString& skinPath, const QString& outputPath)
{
    QDir skinDir(skinPath);
    if (!skinDir.exists()) return false;

    QFile outputFile(outputPath);
    if (!outputFile.open(QIODevice::WriteOnly)) {
        return false;
    }

    QJsonObject exportData;
    exportData["skinName"] = QFileInfo(skinPath).fileName();

    SkinConfig config = loadSkinConfig(skinPath);
    exportData["baseColor"] = config.baseColor;
    exportData["driverName"] = config.driverName;
    exportData["teamName"] = config.teamName;
    exportData["hasNumber"] = config.hasNumber;
    exportData["carNumber"] = config.carNumber;
    exportData["licensePlateText"] = config.licensePlateText;
    exportData["licensePlateCountry"] = config.licensePlateCountry;

    QJsonArray layersArr;
    for (const LiveryLayer& layer : config.layers) {
        QJsonObject obj;
        obj["name"] = layer.name;
        obj["type"] = layer.type;
        obj["opacity"] = static_cast<double>(layer.opacity);
        obj["posX"] = static_cast<double>(layer.position[0]);
        obj["posY"] = static_cast<double>(layer.position[1]);
        obj["sizeW"] = static_cast<double>(layer.size[0]);
        obj["sizeH"] = static_cast<double>(layer.size[1]);
        obj["rotation"] = static_cast<double>(layer.rotation);
        obj["texturePath"] = QFileInfo(layer.texturePath).fileName();
        obj["tintColor"] = layer.tintColor.name();
        obj["visible"] = layer.visible;
        layersArr.append(obj);
    }
    exportData["layers"] = layersArr;

    outputFile.write(QJsonDocument(exportData).toJson());
    outputFile.close();
    return true;
}

bool LiverySystem::importSkin(const QString& importPath, const QString& carPath)
{
    QFile importFile(importPath);
    if (!importFile.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(importFile.readAll());
    importFile.close();

    QJsonObject root = doc.object();
    QString skinName = root["skinName"].toString();
    if (skinName.isEmpty()) {
        skinName = "imported_" + QUuid::createUuid().toString(QUuid::Id128).left(8);
    }

    QString skinPath = carPath + "/skins/" + skinName;
    QDir dir;
    if (!dir.mkpath(skinPath)) {
        return false;
    }

    SkinConfig config;
    config.name = skinName;
    config.path = skinPath;
    config.baseColor = root["baseColor"].toString("#ffffff");
    config.driverName = root["driverName"].toString();
    config.teamName = root["teamName"].toString();
    config.hasNumber = root["hasNumber"].toBool(false);
    config.carNumber = root["carNumber"].toInt(0);
    config.licensePlateText = root["licensePlateText"].toString();
    config.licensePlateCountry = root["licensePlateCountry"].toString();

    QJsonArray layersArr = root["layers"].toArray();
    for (const QJsonValue& val : layersArr) {
        QJsonObject obj = val.toObject();
        LiveryLayer layer;
        layer.name = obj["name"].toString();
        layer.type = obj["type"].toString("decal");
        layer.opacity = obj["opacity"].toDouble(1.0);
        layer.position[0] = obj["posX"].toDouble(0);
        layer.position[1] = obj["posY"].toDouble(0);
        layer.size[0] = obj["sizeW"].toDouble(1);
        layer.size[1] = obj["sizeH"].toDouble(1);
        layer.rotation = obj["rotation"].toDouble(0);
        layer.texturePath = obj["texturePath"].toString();
        layer.tintColor = QColor(obj["tintColor"].toString("#ffffff"));
        layer.visible = obj["visible"].toBool(true);
        config.layers.append(layer);
    }

    return saveSkinConfig(config, skinPath);
}

// ============================================================================
// LiverySystem — Validation
// ============================================================================

bool LiverySystem::validateSkin(const QString& skinPath, QString* error)
{
    QDir skinDir(skinPath);
    if (!skinDir.exists()) {
        if (error) *error = "Skin directory does not exist: " + skinPath;
        return false;
    }

    QFile iniFile(skinPath + "/skin.ini");
    if (!iniFile.exists()) {
        if (error) *error = "Missing skin.ini in " + skinPath;
        return false;
    }

    SkinConfig config;
    if (!loadFromIni(config, skinPath + "/skin.ini")) {
        if (error) *error = "Failed to parse skin.ini";
        return false;
    }

    if (config.name.isEmpty()) {
        if (error) *error = "Skin name is empty in skin.ini";
        return false;
    }

    return true;
}

bool LiverySystem::validateLayer(const LiveryLayer& layer, QString* error)
{
    if (layer.name.isEmpty()) {
        if (error) *error = "Layer name is empty";
        return false;
    }

    if (layer.opacity < 0.0f || layer.opacity > 1.0f) {
        if (error) *error = "Layer opacity out of range [0, 1]";
        return false;
    }

    if (layer.size[0] <= 0 || layer.size[1] <= 0) {
        if (error) *error = "Layer size must be positive";
        return false;
    }

    QStringList validTypes = getLayerTypes();
    if (!validTypes.contains(layer.type)) {
        if (error) *error = "Invalid layer type: " + layer.type;
        return false;
    }

    return true;
}

// ============================================================================
// LiverySystem — Utility
// ============================================================================

QStringList LiverySystem::getLayerTypes()
{
    return QStringList() << "decal" << "paint" << "texture";
}

QString LiverySystem::getDefaultSkinName()
{
    return "default";
}

bool LiverySystem::isDefaultSkin(const QString& skinName)
{
    return skinName.toLower() == "default";
}

// ============================================================================
// LiverySystem — Private helpers
// ============================================================================

bool LiverySystem::createDefaultFiles(const QString& skinPath)
{
    QFile iniFile(skinPath + "/skin.ini");
    if (iniFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&iniFile);
        out << "[SKIN]\n";
        out << "NAME=" << QFileInfo(skinPath).fileName() << "\n";
        out << "PRIORITY=0\n";
        iniFile.close();
    } else {
        qWarning() << "Failed to create skin.ini:" << skinPath;
        return false;
    }

    SkinConfig config;
    config.name = QFileInfo(skinPath).fileName();
    config.baseColor = "#ffffff";
    saveSkinConfig(config, skinPath);

    return true;
}

bool LiverySystem::createPreviewTexture(const QString& skinPath)
{
    return generatePreview(skinPath);
}

// ============================================================================
// LiveryManager — High-level interface
// ============================================================================

LiveryManager::LiveryManager(const QString& carPath)
    : m_carPath(carPath)
{
}

bool LiveryManager::loadSkins()
{
    m_skins = LiverySystem::getSkins(m_carPath);
    return true;
}

bool LiveryManager::createSkin(const QString& name)
{
    bool ok = LiverySystem::createSkin(m_carPath, name);
    if (ok) {
        loadSkins();
    }
    return ok;
}

bool LiveryManager::deleteSkin(const QString& name)
{
    bool ok = LiverySystem::deleteSkin(m_carPath, name);
    if (ok) {
        loadSkins();
    }
    return ok;
}

bool LiveryManager::duplicateSkin(const QString& sourceName, const QString& destName)
{
    bool ok = LiverySystem::duplicateSkin(m_carPath, sourceName, destName);
    if (ok) {
        loadSkins();
    }
    return ok;
}

bool LiveryManager::setCurrentSkin(const QString& skinName)
{
    for (const LiverySystem::SkinInfo& skin : m_skins) {
        if (skin.name == skinName) {
            m_currentSkin = skinName;
            m_config = LiverySystem::loadSkinConfig(skin.path);
            return true;
        }
    }
    return false;
}

bool LiveryManager::saveCurrentSkin()
{
    if (m_currentSkin.isEmpty()) return false;

    QString skinPath = m_carPath + "/skins/" + m_currentSkin;
    return LiverySystem::saveSkinConfig(m_config, skinPath);
}

bool LiveryManager::addLayer(const LiverySystem::LiveryLayer& layer)
{
    return LiverySystem::addLayer(m_config, layer);
}

bool LiveryManager::removeLayer(int index)
{
    return LiverySystem::removeLayer(m_config, index);
}

bool LiveryManager::generateLicensePlate(const QString& text, const QString& country)
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
    }

    return ok;
}
