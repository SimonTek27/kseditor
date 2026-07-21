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
#include <QPainterPath>
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
            if (obj.contains("vectorData")) {
                layer.vectorData = QString::fromUtf8(
                    QJsonDocument(obj["vectorData"].toObject()).toJson());
            }
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
        if (!layer.vectorData.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(layer.vectorData.toUtf8());
            if (doc.isObject()) {
                obj["vectorData"] = doc.object();
            }
        }
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

        if (layer.type == "vector" && !layer.vectorData.isEmpty()) {
            QImage vectorImage = renderVectorPreview(layer.vectorData, preview.width(), preview.height());
            if (!vectorImage.isNull()) {
                painter.setOpacity(layer.opacity);
                painter.drawImage(0, 0, vectorImage);
            }
            continue;
        }

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

static QImage renderVectorDataToImage(const QString& vectorData, int width, int height)
{
    QJsonDocument doc = QJsonDocument::fromJson(vectorData.toUtf8());
    if (!doc.isArray()) return {};

    QJsonArray shapes = doc.array();
    if (shapes.isEmpty()) return {};

    // Calculate bounds
    QRectF bounds;
    for (const QJsonValue& val : shapes) {
        QJsonObject obj = val.toObject();
        int type = obj["type"].toInt(0);
        float x = obj["x"].toDouble(0);
        float y = obj["y"].toDouble(0);
        float w = obj["w"].toDouble(100);
        float h = obj["h"].toDouble(100);

        QRectF r(x, y, w, h);
        if (bounds.isNull())
            bounds = r;
        else
            bounds = bounds.united(r);

        if (type >= 2) {
            QJsonArray pts = obj["points"].toArray();
            for (const QJsonValue& pv : pts) {
                QJsonObject p = pv.toObject();
                QPointF pt(p["x"].toDouble(), p["y"].toDouble());
                bounds = bounds.united(QRectF(pt, QSizeF(1, 1)));
            }
        }
    }

    if (bounds.isNull()) return {};

    float margin = qMax(bounds.width(), bounds.height()) * 0.1f;
    bounds.adjust(-margin, -margin, margin, margin);

    QImage image(width, height, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);

    float scaleX = width / bounds.width();
    float scaleY = height / bounds.height();
    float scale = qMin(scaleX, scaleY);

    painter.scale(scale, scale);
    painter.translate(-bounds.x(), -bounds.y());

    for (const QJsonValue& val : shapes) {
        QJsonObject obj = val.toObject();
        int type = obj["type"].toInt(0);
        float x = obj["x"].toDouble(0);
        float y = obj["y"].toDouble(0);
        float w = obj["w"].toDouble(100);
        float h = obj["h"].toDouble(100);
        QColor fillColor(obj["fill"].toString("#ffff0000"));
        QColor strokeColor(obj["stroke"].toString("#ff000000"));
        float strokeWidth = obj["strokeWidth"].toDouble(1.0);
        float opacity = obj["opacity"].toDouble(1.0);
        bool filled = obj["filled"].toBool(true);

        painter.setOpacity(opacity);
        QPainterPath path;

        switch (type) {
        case 0: { // Rectangle
            QRectF rect(x, y, w, h);
            path.addRoundedRect(rect, 2, 2);
            break;
        }
        case 1: { // Ellipse
            QRectF rect(x, y, w, h);
            path.addEllipse(rect);
            break;
        }
        case 2: // Line
        case 3: // Polygon
        case 4: // Path
        {
            QJsonArray pts = obj["points"].toArray();
            bool first = true;
            for (const QJsonValue& pv : pts) {
                QJsonObject p = pv.toObject();
                QPointF pt(p["x"].toDouble(), p["y"].toDouble());
                if (first) {
                    path.moveTo(pt);
                    first = false;
                } else {
                    path.lineTo(pt);
                }
            }
            if (type == 3) path.closeSubpath();
            break;
        }
        }

        if (filled && type != 2) {
            painter.setBrush(fillColor);
            painter.setPen(QPen(strokeColor, strokeWidth));
        } else {
            painter.setBrush(Qt::NoBrush);
            QPen pen(type == 2 ? fillColor : strokeColor, strokeWidth);
            pen.setCapStyle(Qt::RoundCap);
            pen.setJoinStyle(Qt::RoundJoin);
            painter.setPen(pen);
        }

        painter.drawPath(path);
    }

    painter.end();
    return image;
}

QImage LiverySystem::renderVectorPreview(const QString& vectorData, int width, int height)
{
    return renderVectorDataToImage(vectorData, width, height);
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
        if (!layer.vectorData.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(layer.vectorData.toUtf8());
            if (doc.isObject()) {
                obj["vectorData"] = doc.object();
            }
        }
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
        if (obj.contains("vectorData")) {
            layer.vectorData = QString::fromUtf8(
                QJsonDocument(obj["vectorData"].toObject()).toJson());
        }
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
    return QStringList() << "decal" << "paint" << "texture" << "vector";
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

// ============================================================================
// DDS Export
// ============================================================================

QVector<LiverySystem::UndoAction> LiverySystem::s_undoStack;
QVector<LiverySystem::UndoAction> LiverySystem::s_redoStack;

bool LiverySystem::exportSkinAsDDS(const QString& skinPath, const QString& outputPath)
{
    QImage livery(skinPath + "/livery.png");
    if (livery.isNull()) {
        livery = QImage(skinPath + "/sides_1.png");
    }
    if (livery.isNull()) return false;

    return saveTextureAsDDS(livery, outputPath);
}

bool LiverySystem::saveTextureAsDDS(const QImage& image, const QString& outputPath)
{
    if (image.isNull()) return false;

    QImage rgba = image.convertToFormat(QImage::Format_RGBA8888);

    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly)) return false;

    int width = rgba.width();
    int height = rgba.height();

    // Simple DDS header for DXT5 compression
    struct DDSHeader {
        char magic[4] = {'D', 'D', 'S', ' '};
        uint32_t size = 124;
        uint32_t flags = 0x00021007;
        uint32_t height;
        uint32_t width;
        uint32_t pitchOrLinearSize;
        uint32_t depth = 0;
        uint32_t mipMapCount = 1;
        uint32_t reserved[11] = {0};
        struct PixelFormat {
            uint32_t size = 32;
            uint32_t flags = 0x00000004;
            char fourCC[4] = {'D', 'X', 'T', '5'};
            uint32_t rgbBitCount = 0;
            uint32_t rBitMask = 0;
            uint32_t gBitMask = 0;
            uint32_t bBitMask = 0;
            uint32_t aBitMask = 0;
        } pixelFormat;
        struct {
            uint32_t caps1 = 0x00001000;
            uint32_t caps2 = 0;
            uint32_t caps3 = 0;
            uint32_t caps4 = 0;
        } caps;
        uint32_t reserved2 = 0;
    };

    int blockCount = ((width + 3) / 4) * ((height + 3) / 4);
    int dataSize = blockCount * 16;

    DDSHeader header;
    header.width = width;
    header.height = height;
    header.pitchOrLinearSize = dataSize;

    file.write(reinterpret_cast<const char*>(&header), sizeof(header));

    // Simple block compression: average each 4x4 block
    for (int by = 0; by < height; by += 4) {
        for (int bx = 0; bx < width; bx += 4) {
            // Collect 4x4 block of colors
            uint8_t block[16][4];
            int blockIdx = 0;
            for (int py = 0; py < 4; ++py) {
                for (int px = 0; px < 4; ++px) {
                    int x = std::min(bx + px, width - 1);
                    int y = std::min(by + py, height - 1);
                    QRgb pixel = rgba.pixel(x, y);
                    block[blockIdx][0] = qRed(pixel);
                    block[blockIdx][1] = qGreen(pixel);
                    block[blockIdx][2] = qBlue(pixel);
                    block[blockIdx][3] = qAlpha(pixel);
                    blockIdx++;
                }
            }

            // DXT5 block: 16 bytes per block (8 bytes alpha + 8 bytes color)
            uint8_t dxt5Block[16] = {0};

            // Alpha block (BC4-like): store min/max alpha, then 3-bit indices
            uint8_t minAlpha = 255, maxAlpha = 0;
            for (int i = 0; i < 16; ++i) {
                minAlpha = std::min(minAlpha, block[i][3]);
                maxAlpha = std::max(maxAlpha, block[i][3]);
            }
            dxt5Block[0] = maxAlpha;
            dxt5Block[1] = minAlpha;

            uint64_t alphaBits = 0;
            if (maxAlpha > minAlpha) {
                for (int i = 0; i < 16; ++i) {
                    int idx = (16 * (block[i][3] - minAlpha)) / std::max(1, (maxAlpha - minAlpha));
                    idx = std::clamp(idx, 0, 15);
                    if (idx >= 8) idx = 7;
                    alphaBits |= static_cast<uint64_t>(idx) << (i * 3);
                }
                for (int b = 0; b < 6; ++b) {
                    dxt5Block[2 + b] = (alphaBits >> (b * 8)) & 0xFF;
                }
            }

            // Color block (BC1): store 565 colors
            int rSum = 0, gSum = 0, bSum = 0;
            for (int i = 0; i < 16; ++i) {
                rSum += block[i][0];
                gSum += block[i][1];
                bSum += block[i][2];
            }

            uint16_t c0 = ((rSum / 16) >> 3) << 11
                        | ((gSum / 16) >> 2) << 5
                        | ((bSum / 16) >> 3);
            uint8_t c0_lo = c0 & 0xFF;
            uint8_t c0_hi = (c0 >> 8) & 0xFF;
            uint16_t c1 = c0; // Same color = no interpolation
            uint8_t c1_lo = c1 & 0xFF;
            uint8_t c1_hi = (c1 >> 8) & 0xFF;

            dxt5Block[8] = c0_lo;
            dxt5Block[9] = c0_hi;
            dxt5Block[10] = c1_lo;
            dxt5Block[11] = c1_hi;

            file.write(reinterpret_cast<const char*>(dxt5Block), 16);
        }
    }

    file.close();
    return true;
}

// ============================================================================
// Decal Import
// ============================================================================

bool LiverySystem::importDecal(const QString& decalPath, const QString& skinPath)
{
    QImage decal(decalPath);
    if (decal.isNull()) return false;

    QFileInfo fi(decalPath);
    QString decalName = fi.completeBaseName();
    QString destPath = skinPath + "/" + decalName + ".png";

    if (!decal.save(destPath, "PNG")) return false;

    return true;
}

QStringList LiverySystem::getSupportedDecalFormats()
{
    return {"PNG (*.png)", "JPEG (*.jpg *.jpeg)", "DDS (*.dds)",
            "TGA (*.tga)", "BMP (*.bmp)", "TIFF (*.tiff)"};
}

// ============================================================================
// Template System
// ============================================================================

QVector<LiverySystem::LiveryTemplate> LiverySystem::getBuiltinTemplates()
{
    QVector<LiveryTemplate> templates;

    {
        LiveryTemplate tmpl;
        tmpl.name = "Racing Stripes";
        tmpl.description = "Classic dual racing stripes over base color";
        tmpl.baseColor = QColor(200, 0, 0);
        tmpl.stripes = {{"Center Stripe", QColor(255, 255, 255)},
                         {"Side Stripe 1", QColor(200, 200, 200)},
                         {"Side Stripe 2", QColor(200, 200, 200)}};
        tmpl.hasRaceNumber = true;
        tmpl.hasLicensePlate = true;
        templates.append(tmpl);
    }

    {
        LiveryTemplate tmpl;
        tmpl.name = "Carbon Edition";
        tmpl.description = "Matte carbon fiber look with accent accents";
        tmpl.baseColor = QColor(30, 30, 30);
        tmpl.stripes = {{"Accent Stripe", QColor(255, 100, 0)}};
        tmpl.hasRaceNumber = true;
        templates.append(tmpl);
    }

    {
        LiveryTemplate tmpl;
        tmpl.name = "National Flag";
        tmpl.description = "Flag-inspired livery with patriotic colors";
        tmpl.baseColor = QColor(0, 50, 200);
        tmpl.stripes = {{"White Band", QColor(255, 255, 255)},
                         {"Red Band", QColor(200, 0, 0)}};
        tmpl.hasLicensePlate = true;
        templates.append(tmpl);
    }

    {
        LiveryTemplate tmpl;
        tmpl.name = "Clean Canvas";
        tmpl.description = "Solid color base with no decorations";
        tmpl.baseColor = QColor(255, 255, 255);
        templates.append(tmpl);
    }

    {
        LiveryTemplate tmpl;
        tmpl.name = "Gradient Flow";
        tmpl.description = "Smooth color gradient from front to rear";
        tmpl.baseColor = QColor(0, 100, 200);
        tmpl.stripes = {{"Gradient Accent", QColor(0, 200, 255)}};
        templates.append(tmpl);
    }

    {
        LiveryTemplate tmpl;
        tmpl.name = "Gulf-Inspired";
        tmpl.description = "Classic Gulf racing colors: light blue with orange stripe";
        tmpl.baseColor = QColor(0, 150, 200);
        tmpl.stripes = {{"Racing Stripe", QColor(255, 150, 0)},
                         {"Accent", QColor(200, 100, 0)}};
        tmpl.hasRaceNumber = true;
        tmpl.hasLicensePlate = true;
        templates.append(tmpl);
    }

    return templates;
}

bool LiverySystem::createSkinFromTemplate(const QString& carPath, const QString& skinName,
                                           const LiveryTemplate& tmpl)
{
    QString skinPath = carPath + "/skins/" + skinName;
    QDir().mkpath(skinPath);

    // Create base livery texture
    int res = tmpl.textureResolution;
    QImage base(res, res, QImage::Format_RGBA8888);
    base.fill(tmpl.baseColor);

    QPainter painter(&base);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw stripes
    for (int i = 0; i < tmpl.stripes.size(); ++i) {
        painter.fillRect(QRect(0, res * (0.2 + i * 0.15), res, res * 0.05),
                         tmpl.stripes[i].second);
    }

    painter.end();

    // Save base texture
    base.save(skinPath + "/livery.png", "PNG");

    // Create skin.ini
    QFile ini(skinPath + "/skin.ini");
    if (ini.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream s(&ini);
        s << "[SKIN]\n";
        s << "NAME=" << skinName << "\n";
        s << "PRIORITY=0\n";
        s << "BASE_COLOR=" << tmpl.baseColor.name() << "\n";
        if (tmpl.hasLicensePlate) {
            s << "COUNTRY=IT\n";
            s << "LICENSE_PLATE=ABC 123\n";
        }
        if (tmpl.hasRaceNumber) {
            s << "NUMBER=1\n";
        }
        ini.close();
    }

    return true;
}

// ============================================================================
// Undo/Redo System
// ============================================================================

void LiverySystem::pushUndo(const UndoAction& action)
{
    s_undoStack.append(action);
    if (s_undoStack.size() > 50) {
        s_undoStack.removeFirst();
    }
    s_redoStack.clear();
}

bool LiverySystem::canUndo()
{
    return !s_undoStack.isEmpty();
}

bool LiverySystem::canRedo()
{
    return !s_redoStack.isEmpty();
}

LiverySystem::UndoAction LiverySystem::undoLast()
{
    if (s_undoStack.isEmpty()) return {};
    UndoAction action = s_undoStack.takeLast();
    s_redoStack.append(action);
    return action;
}

LiverySystem::UndoAction LiverySystem::redoLast()
{
    if (s_redoStack.isEmpty()) return {};
    UndoAction action = s_redoStack.takeLast();
    s_undoStack.append(action);
    return action;
}

void LiverySystem::clearUndoRedo()
{
    s_undoStack.clear();
    s_redoStack.clear();
}

// ============================================================================
// Color Palette
// ============================================================================

QVector<LiverySystem::ColorSwatch> LiverySystem::getDefaultPalette()
{
    return {
        {"Race Red", QColor(255, 0, 0)},
        {"Race Blue", QColor(0, 50, 255)},
        {"Race Green", QColor(0, 180, 0)},
        {"Race Yellow", QColor(255, 200, 0)},
        {"Race Orange", QColor(255, 100, 0)},
        {"Race Purple", QColor(150, 0, 255)},
        {"Race Pink", QColor(255, 0, 150)},
        {"Cyan", QColor(0, 200, 255)},
        {"White", QColor(255, 255, 255)},
        {"Silver", QColor(200, 200, 200)},
        {"Gray", QColor(128, 128, 128)},
        {"Black", QColor(30, 30, 30)},
        {"Matte Black", QColor(20, 20, 20)},
        {"Carbon", QColor(40, 40, 40)},
        {"Gold", QColor(200, 170, 0)},
        {"Bronze", QColor(150, 100, 30)},
        {"Navy", QColor(0, 0, 100)},
        {"Dark Green", QColor(0, 60, 0)},
        {"Burgundy", QColor(100, 0, 30)},
        {"Chrome", QColor(180, 180, 200)},
    };
}

QVector<LiverySystem::ColorSwatch> LiverySystem::loadPalette(const QString& path)
{
    QVector<ColorSwatch> palette;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return getDefaultPalette();

    QTextStream s(&file);
    while (!s.atEnd()) {
        QString line = s.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;
        QStringList parts = line.split(',');
        if (parts.size() >= 2) {
            ColorSwatch swatch;
            swatch.name = parts[0].trimmed();
            swatch.color = QColor(parts[1].trimmed());
            if (swatch.color.isValid()) palette.append(swatch);
        }
    }
    file.close();

    return palette.isEmpty() ? getDefaultPalette() : palette;
}

bool LiverySystem::savePalette(const QVector<ColorSwatch>& palette, const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QTextStream s(&file);
    for (const auto& swatch : palette) {
        s << swatch.name << "," << swatch.color.name() << "\n";
    }
    file.close();
    return true;
}
