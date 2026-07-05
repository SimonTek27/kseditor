#include "ShowroomSystem.h"
#include "../../core/FileFormat/INIParser.h"
#include "../../core/sys/LogManager.h"
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPainter>
#include <QImage>
#include <QFont>
#include <QFileInfo>
#include <cmath>
#include "core/editor/EditorConfig.h"

ShowroomSystem::ShowroomConfig ShowroomSystem::loadConfig(const QString& configPath)
{
    ShowroomConfig config;

    INIParser ini;
    if (!ini.load(configPath)) {
        LOG_WARNING("ShowroomSystem", QString("Failed to load showroom config from: %1").arg(configPath));
        return getDefaultConfig();
    }

    config.name = ini.string("SHOWROOM", "NAME", "Default Showroom");
    config.description = ini.string("SHOWROOM", "DESCRIPTION", "");
    config.cameraDistance = static_cast<float>(ini.real("CAMERA", "DISTANCE", 5.0));
    config.cameraHeight = static_cast<float>(ini.real("CAMERA", "HEIGHT", 2.0));
    config.cameraAngle = static_cast<float>(ini.real("CAMERA", "ANGLE", 30.0));
    config.cameraFov = static_cast<float>(ini.real("CAMERA", "FOV", 60.0));
    config.rotateSpeed = static_cast<float>(ini.real("CAMERA", "ROTATE_SPEED", 0.5));
    config.autoRotate = ini.boolean("CAMERA", "AUTO_ROTATE", true);
    config.backgroundPath = ini.string("BACKGROUND", "PATH", "");
    config.ambientColor = QColor(ini.string("LIGHTING", "AMBIENT_COLOR", "#C8C8C8"));
    config.sunColor = QColor(ini.string("LIGHTING", "SUN_COLOR", "#FFFAF0"));
    config.sunIntensity = static_cast<float>(ini.real("LIGHTING", "SUN_INTENSITY", 1.0));
    config.ambientIntensity = static_cast<float>(ini.real("LIGHTING", "AMBIENT_INTENSITY", 0.3));

    LOG_INFO("ShowroomSystem", QString("Loaded showroom config from: %1").arg(configPath));
    return config;
}

bool ShowroomSystem::saveConfig(const ShowroomConfig& config, const QString& configPath)
{
    INIParser ini;

    if (QFile::exists(configPath)) {
        ini.load(configPath);
    }

    ini.setValue("SHOWROOM", "NAME", config.name);
    ini.setValue("SHOWROOM", "DESCRIPTION", config.description);
    ini.setValue("CAMERA", "DISTANCE", static_cast<double>(config.cameraDistance));
    ini.setValue("CAMERA", "HEIGHT", static_cast<double>(config.cameraHeight));
    ini.setValue("CAMERA", "ANGLE", static_cast<double>(config.cameraAngle));
    ini.setValue("CAMERA", "FOV", static_cast<double>(config.cameraFov));
    ini.setValue("CAMERA", "ROTATE_SPEED", static_cast<double>(config.rotateSpeed));
    ini.setValue("CAMERA", "AUTO_ROTATE", config.autoRotate);
    ini.setValue("BACKGROUND", "PATH", config.backgroundPath);
    ini.setValue("LIGHTING", "AMBIENT_COLOR", config.ambientColor.name());
    ini.setValue("LIGHTING", "SUN_COLOR", config.sunColor.name());
    ini.setValue("LIGHTING", "SUN_INTENSITY", static_cast<double>(config.sunIntensity));
    ini.setValue("LIGHTING", "AMBIENT_INTENSITY", static_cast<double>(config.ambientIntensity));

    if (!ini.save(configPath)) {
        LOG_ERROR("ShowroomSystem", QString("Failed to save showroom config to: %1").arg(configPath));
        return false;
    }

    LOG_INFO("ShowroomSystem", QString("Saved showroom config to: %1").arg(configPath));
    return true;
}

ShowroomSystem::ShowroomConfig ShowroomSystem::getDefaultConfig()
{
    ShowroomConfig config;
    config.name = "Default Showroom";
    config.description = EditorConfig::instance().showroomConfigDesc();
    config.cameraDistance = 5.0f;
    config.cameraHeight = 2.0f;
    config.cameraAngle = 30.0f;
    config.cameraFov = 60.0f;
    config.rotateSpeed = 0.5f;
    config.autoRotate = true;
    config.backgroundPath = "";
    config.ambientColor = QColor(200, 200, 200);
    config.sunColor = QColor(255, 250, 240);
    config.sunIntensity = 1.0f;
    config.ambientIntensity = 0.3f;
    return config;
}

QVector<ShowroomSystem::ShowroomCamera> ShowroomSystem::loadCameras(const QString& configPath)
{
    QVector<ShowroomCamera> cameras;

    INIParser ini;
    if (!ini.load(configPath)) {
        cameras.append(getDefaultCamera());
        return cameras;
    }

    QStringList sections = ini.sections();
    for (const auto& section : sections) {
        if (section.startsWith("CAMERA_")) {
            ShowroomCamera cam;
            cam.name = ini.string(section, "NAME", section);
            cam.position[0] = static_cast<float>(ini.real(section, "POS_X", 0.0));
            cam.position[1] = static_cast<float>(ini.real(section, "POS_Y", 2.0));
            cam.position[2] = static_cast<float>(ini.real(section, "POS_Z", 5.0));
            cam.target[0] = static_cast<float>(ini.real(section, "TARG_X", 0.0));
            cam.target[1] = static_cast<float>(ini.real(section, "TARG_Y", 0.0));
            cam.target[2] = static_cast<float>(ini.real(section, "TARG_Z", 0.0));
            cam.up[0] = static_cast<float>(ini.real(section, "UP_X", 0.0));
            cam.up[1] = static_cast<float>(ini.real(section, "UP_Y", 1.0));
            cam.up[2] = static_cast<float>(ini.real(section, "UP_Z", 0.0));
            cam.fov = static_cast<float>(ini.real(section, "FOV", 60.0));
            cam.isActive = ini.boolean(section, "ACTIVE", true);
            cameras.append(cam);
        }
    }

    if (cameras.isEmpty()) {
        cameras.append(getDefaultCamera());
    }

    return cameras;
}

bool ShowroomSystem::saveCameras(const QVector<ShowroomCamera>& cameras, const QString& configPath)
{
    INIParser ini;

    if (QFile::exists(configPath)) {
        ini.load(configPath);
    }

    for (int i = 0; i < cameras.size(); ++i) {
        QString section = QString("CAMERA_%1").arg(i);
        ini.removeSection(section);
        ini.setValue(section, "NAME", cameras[i].name);
        ini.setValue(section, "POS_X", static_cast<double>(cameras[i].position[0]));
        ini.setValue(section, "POS_Y", static_cast<double>(cameras[i].position[1]));
        ini.setValue(section, "POS_Z", static_cast<double>(cameras[i].position[2]));
        ini.setValue(section, "TARG_X", static_cast<double>(cameras[i].target[0]));
        ini.setValue(section, "TARG_Y", static_cast<double>(cameras[i].target[1]));
        ini.setValue(section, "TARG_Z", static_cast<double>(cameras[i].target[2]));
        ini.setValue(section, "UP_X", static_cast<double>(cameras[i].up[0]));
        ini.setValue(section, "UP_Y", static_cast<double>(cameras[i].up[1]));
        ini.setValue(section, "UP_Z", static_cast<double>(cameras[i].up[2]));
        ini.setValue(section, "FOV", static_cast<double>(cameras[i].fov));
        ini.setValue(section, "ACTIVE", cameras[i].isActive);
    }

    return ini.save(configPath);
}

ShowroomSystem::ShowroomCamera ShowroomSystem::getDefaultCamera()
{
    ShowroomCamera cam;
    cam.name = "Default";
    cam.position[0] = 0.0f;
    cam.position[1] = 2.0f;
    cam.position[2] = 5.0f;
    cam.target[0] = 0.0f;
    cam.target[1] = 0.0f;
    cam.target[2] = 0.0f;
    cam.up[0] = 0.0f;
    cam.up[1] = 1.0f;
    cam.up[2] = 0.0f;
    cam.fov = 60.0f;
    cam.isActive = true;
    return cam;
}

QVector<ShowroomSystem::ShowroomLight> ShowroomSystem::loadLights(const QString& configPath)
{
    QVector<ShowroomLight> lights;

    INIParser ini;
    if (!ini.load(configPath)) {
        lights = getDefaultLights();
        return lights;
    }

    QStringList sections = ini.sections();
    for (const auto& section : sections) {
        if (section.startsWith("LIGHT_")) {
            ShowroomLight light;
            light.name = ini.string(section, "NAME", section);
            light.type = ini.string(section, "TYPE", "directional");
            light.position[0] = static_cast<float>(ini.real(section, "POS_X", 0.0));
            light.position[1] = static_cast<float>(ini.real(section, "POS_Y", 5.0));
            light.position[2] = static_cast<float>(ini.real(section, "POS_Z", 0.0));
            light.direction[0] = static_cast<float>(ini.real(section, "DIR_X", 0.0));
            light.direction[1] = static_cast<float>(ini.real(section, "DIR_Y", -1.0));
            light.direction[2] = static_cast<float>(ini.real(section, "DIR_Z", 0.0));
            light.color = QColor(ini.string(section, "COLOR", "#FFFFFF"));
            light.intensity = static_cast<float>(ini.real(section, "INTENSITY", 1.0));
            light.range = static_cast<float>(ini.real(section, "RANGE", 10.0));
            light.isActive = ini.boolean(section, "ACTIVE", true);
            lights.append(light);
        }
    }

    if (lights.isEmpty()) {
        lights = getDefaultLights();
    }

    return lights;
}

bool ShowroomSystem::saveLights(const QVector<ShowroomLight>& lights, const QString& configPath)
{
    INIParser ini;

    if (QFile::exists(configPath)) {
        ini.load(configPath);
    }

    for (int i = 0; i < lights.size(); ++i) {
        QString section = QString("LIGHT_%1").arg(i);
        ini.removeSection(section);
        ini.setValue(section, "NAME", lights[i].name);
        ini.setValue(section, "TYPE", lights[i].type);
        ini.setValue(section, "POS_X", static_cast<double>(lights[i].position[0]));
        ini.setValue(section, "POS_Y", static_cast<double>(lights[i].position[1]));
        ini.setValue(section, "POS_Z", static_cast<double>(lights[i].position[2]));
        ini.setValue(section, "DIR_X", static_cast<double>(lights[i].direction[0]));
        ini.setValue(section, "DIR_Y", static_cast<double>(lights[i].direction[1]));
        ini.setValue(section, "DIR_Z", static_cast<double>(lights[i].direction[2]));
        ini.setValue(section, "COLOR", lights[i].color.name());
        ini.setValue(section, "INTENSITY", static_cast<double>(lights[i].intensity));
        ini.setValue(section, "RANGE", static_cast<double>(lights[i].range));
        ini.setValue(section, "ACTIVE", lights[i].isActive);
    }

    return ini.save(configPath);
}

QVector<ShowroomSystem::ShowroomLight> ShowroomSystem::getDefaultLights()
{
    QVector<ShowroomLight> lights;

    ShowroomLight sun;
    sun.name = "Sun";
    sun.type = "directional";
    sun.position[0] = 5.0f;
    sun.position[1] = 10.0f;
    sun.position[2] = 5.0f;
    sun.direction[0] = -0.5f;
    sun.direction[1] = -1.0f;
    sun.direction[2] = -0.5f;
    sun.color = QColor(255, 250, 240);
    sun.intensity = 1.0f;
    sun.range = 100.0f;
    sun.isActive = true;
    lights.append(sun);

    ShowroomLight ambient;
    ambient.name = "Ambient";
    ambient.type = "point";
    ambient.position[0] = 0.0f;
    ambient.position[1] = 3.0f;
    ambient.position[2] = 0.0f;
    ambient.direction[0] = 0.0f;
    ambient.direction[1] = -1.0f;
    ambient.direction[2] = 0.0f;
    ambient.color = QColor(200, 200, 220);
    ambient.intensity = 0.3f;
    ambient.range = 20.0f;
    ambient.isActive = true;
    lights.append(ambient);

    ShowroomLight fill;
    fill.name = "Fill";
    fill.type = "spot";
    fill.position[0] = -3.0f;
    fill.position[1] = 4.0f;
    fill.position[2] = 2.0f;
    fill.direction[0] = 0.3f;
    fill.direction[1] = -0.8f;
    fill.direction[2] = -0.2f;
    fill.color = QColor(180, 200, 255);
    fill.intensity = 0.5f;
    fill.range = 15.0f;
    fill.isActive = true;
    lights.append(fill);

    return lights;
}

bool ShowroomSystem::generatePreview(const QString& carPath, const PreviewConfig& config)
{
    if (carPath.isEmpty()) {
        LOG_ERROR("ShowroomSystem", "Cannot generate preview: car path is empty");
        return false;
    }

    LOG_INFO("ShowroomSystem", QString("Generating preview for: %1 (%2x%3)")
        .arg(carPath).arg(config.width).arg(config.height));

    const int w = config.width;
    const int h = config.height;

    QImage image(w, h, QImage::Format_ARGB32);
    image.fill(config.useTransparentBackground ? Qt::transparent : QColor(30, 30, 35));

    QPainter painter(&image);
    if (!painter.isActive()) {
        LOG_ERROR("ShowroomSystem", "Failed to initialize QPainter");
        return false;
    }
    painter.setRenderHint(QPainter::Antialiasing, true);

    const int groundY = static_cast<int>(h * 0.65);
    const float centerX = w / 2.0f;
    const float lightScale = w / 20.0f;

    // Draw grid floor
    QPen gridPen(QColor(55, 55, 60), 1, Qt::SolidLine);
    painter.setPen(gridPen);
    int gridSpacing = qMax(8, h / 20);
    for (int y = groundY; y <= h; y += gridSpacing) {
        painter.drawLine(0, y, w, y);
    }
    for (int x = 0; x <= w; x += gridSpacing) {
        painter.drawLine(x, groundY, x, h);
    }

    // Horizon line
    painter.setPen(QPen(QColor(80, 80, 90), 2));
    painter.drawLine(0, groundY, w, groundY);

    // Draw light position indicators
    QVector<ShowroomLight> lights = getDefaultLights();
    for (const auto& light : lights) {
        if (!light.isActive) continue;

        float lx = centerX + light.position[0] * lightScale;
        float ly = groundY - light.position[1] * (lightScale * 0.5f);
        int radius = qBound(4, static_cast<int>(light.intensity * 8), 16);

        // Glow effect
        painter.setPen(Qt::NoPen);
        QColor glowColor = light.color;
        glowColor.setAlpha(50);
        painter.setBrush(glowColor);
        painter.drawEllipse(QPointF(lx, ly), radius * 2.5f, radius * 2.5f);

        // Core dot
        painter.setBrush(light.color);
        painter.drawEllipse(QPointF(lx, ly), radius, radius);

        // Light label
        QFont labelFont("Segoe UI", 8);
        painter.setFont(labelFont);
        painter.setPen(QColor(180, 180, 180));
        painter.drawText(QPointF(lx + radius + 4, ly + 3), light.name);
    }

    // Draw camera frustum
    float camAngleRad = config.cameraAngle * 3.14159265f / 180.0f;
    float camX = centerX + config.cameraDistance * sinf(camAngleRad) * lightScale;
    float camY = groundY - config.cameraHeight * (lightScale * 0.5f);
    float targetX = centerX;
    float targetY = static_cast<float>(groundY);

    float dirX = targetX - camX;
    float dirY = targetY - camY;
    float dirLen = sqrtf(dirX * dirX + dirY * dirY);
    if (dirLen > 0.001f) { dirX /= dirLen; dirY /= dirLen; }

    float perpX = -dirY;
    float perpY = dirX;

    float fovRad = config.fov * 3.14159265f / 180.0f;
    float halfFov = fovRad * 0.5f;
    float frustumLen = lightScale * 2.0f;

    QPointF leftTip(camX + dirX * frustumLen + perpX * frustumLen * tanf(halfFov),
                    camY + dirY * frustumLen + perpY * frustumLen * tanf(halfFov));
    QPointF rightTip(camX + dirX * frustumLen - perpX * frustumLen * tanf(halfFov),
                     camY + dirY * frustumLen - perpY * frustumLen * tanf(halfFov));

    QPen camPen(Qt::white, 2);
    painter.setPen(camPen);
    painter.setBrush(QColor(255, 255, 255, 25));
    QPolygonF frustum;
    frustum << QPointF(camX, camY) << leftTip << rightTip;
    painter.drawPolygon(frustum);

    // Camera origin dot
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255, 200));
    painter.drawEllipse(QPointF(camX, camY), 5, 5);

    // Camera-to-target dashed line
    painter.setPen(QPen(QColor(255, 255, 255, 80), 1, Qt::DashLine));
    painter.setBrush(Qt::NoBrush);
    painter.drawLine(QPointF(camX, camY), QPointF(targetX, targetY));

    // Settings text overlay
    QFont textFont("Segoe UI", 10);
    painter.setFont(textFont);

    int textMargin = 15;
    int textBottom = h - 15;
    int lineH = 18;

    // Background panel
    QRect panelRect(textMargin - 5, textBottom - 5 * lineH - 5, 340, 5 * lineH + 15);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 120));
    painter.drawRoundedRect(panelRect, 6, 6);

    painter.setPen(QColor(220, 220, 220));
    int ty = textBottom - 4 * lineH;
    painter.drawText(textMargin, ty, QString("Camera: dist=%1  height=%2  angle=%3")
        .arg(config.cameraDistance, 0, 'f', 1)
        .arg(config.cameraHeight, 0, 'f', 1)
        .arg(config.cameraAngle, 0, 'f', 1));
    ty += lineH;
    painter.drawText(textMargin, ty, QString("FOV: %1 deg").arg(config.fov, 0, 'f', 1));
    ty += lineH;
    painter.drawText(textMargin, ty, QString("Resolution: %1 x %2").arg(w).arg(h));
    ty += lineH;
    painter.drawText(textMargin, ty, QString("Lights: %1 active").arg(lights.size()));
    ty += lineH;
    painter.drawText(textMargin, ty, QString("Car: %1").arg(carPath));

    painter.end();

    // Save to output
    QString outputPath = config.outputPath;
    if (outputPath.isEmpty()) {
        outputPath = carPath + "/preview.png";
    }
    QDir().mkpath(QFileInfo(outputPath).absolutePath());

    if (!image.save(outputPath, "PNG")) {
        LOG_ERROR("ShowroomSystem", QString("Failed to save preview: %1").arg(outputPath));
        return false;
    }

    LOG_INFO("ShowroomSystem", QString("Saved preview: %1").arg(outputPath));
    return true;
}

bool ShowroomSystem::generateThumbnail(const QString& carPath, const QString& outputPath)
{
    PreviewConfig thumbConfig;
    thumbConfig.width = 256;
    thumbConfig.height = 256;
    thumbConfig.samples = 2;
    thumbConfig.outputPath = outputPath.isEmpty() ? carPath + "/thumbnail.png" : outputPath;

    return generatePreview(carPath, thumbConfig);
}

bool ShowroomSystem::generateAllPreviews(const QString& carsDir, const PreviewConfig& config)
{
    QDir dir(carsDir);
    if (!dir.exists()) {
        LOG_ERROR("ShowroomSystem", QString("Cars directory does not exist: %1").arg(carsDir));
        return false;
    }

    QStringList carDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    int successCount = 0;

    for (const auto& carDir : carDirs) {
        QString carPath = carsDir + "/" + carDir;

        // Check for per-car showroom config override
        PreviewConfig carConfig = config;
        QString localIni = carPath + "/showroom.ini";
        if (QFile::exists(localIni)) {
            ShowroomConfig showroomCfg = loadConfig(localIni);
            carConfig.cameraDistance = showroomCfg.cameraDistance;
            carConfig.cameraHeight = showroomCfg.cameraHeight;
            carConfig.cameraAngle = showroomCfg.cameraAngle;
            carConfig.fov = showroomCfg.cameraFov;
            LOG_INFO("ShowroomSystem", QString("Using local showroom config for: %1").arg(carDir));
        }

        if (generatePreview(carPath, carConfig)) {
            successCount++;
        }
    }

    LOG_INFO("ShowroomSystem", QString("Generated previews: %1/%2 cars").arg(successCount).arg(carDirs.size()));
    return successCount > 0;
}

bool ShowroomSystem::validateConfig(const ShowroomConfig& config, QString* error)
{
    if (config.cameraDistance <= 0.0f) {
        if (error) *error = "Camera distance must be positive";
        return false;
    }
    if (config.cameraHeight < 0.0f) {
        if (error) *error = "Camera height cannot be negative";
        return false;
    }
    if (config.cameraFov <= 0.0f || config.cameraFov > 180.0f) {
        if (error) *error = "Camera FOV must be between 0 and 180 degrees";
        return false;
    }
    if (config.rotateSpeed < 0.0f) {
        if (error) *error = "Rotate speed cannot be negative";
        return false;
    }
    if (config.sunIntensity < 0.0f) {
        if (error) *error = "Sun intensity cannot be negative";
        return false;
    }
    if (config.ambientIntensity < 0.0f) {
        if (error) *error = "Ambient intensity cannot be negative";
        return false;
    }
    return true;
}

bool ShowroomSystem::validatePreviewConfig(const PreviewConfig& config, QString* error)
{
    if (config.width <= 0 || config.width > 7680) {
        if (error) *error = "Width must be between 1 and 7680";
        return false;
    }
    if (config.height <= 0 || config.height > 4320) {
        if (error) *error = "Height must be between 1 and 4320";
        return false;
    }
    if (config.samples < 1 || config.samples > 16) {
        if (error) *error = "Samples must be between 1 and 16";
        return false;
    }
    if (config.fov <= 0.0f || config.fov > 180.0f) {
        if (error) *error = "FOV must be between 0 and 180 degrees";
        return false;
    }
    return true;
}

QStringList ShowroomSystem::getAvailableShowrooms(const QString& showroomsDir)
{
    QStringList showrooms;
    QDir dir(showroomsDir);
    if (!dir.exists()) return showrooms;

    QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const auto& entry : entries) {
        QString iniFile = showroomsDir + "/" + entry + "/showroom.ini";
        if (QFile::exists(iniFile)) {
            showrooms.append(entry);
        }
    }
    return showrooms;
}

QString ShowroomSystem::getDefaultShowroomDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/showrooms";
}

// ── ShowroomManager ──────────────────────────────────────────────

ShowroomManager::ShowroomManager(const QString& acPath)
    : m_acPath(acPath)
{
}

bool ShowroomManager::loadShowroom(const QString& showroomName)
{
    QString configPath = m_acPath + "/showroom/" + showroomName + "/showroom.ini";
    m_config = ShowroomSystem::loadConfig(configPath);
    m_cameras = ShowroomSystem::loadCameras(configPath);
    m_lights = ShowroomSystem::loadLights(configPath);
    LOG_INFO("ShowroomManager", QString("Loaded showroom: %1").arg(showroomName));
    return true;
}

bool ShowroomManager::saveShowroom(const QString& showroomName)
{
    QString configPath = m_acPath + "/showroom/" + showroomName + "/showroom.ini";
    QDir().mkpath(QFileInfo(configPath).absolutePath());

    bool ok = ShowroomSystem::saveConfig(m_config, configPath);
    ok &= ShowroomSystem::saveCameras(m_cameras, configPath);
    ok &= ShowroomSystem::saveLights(m_lights, configPath);
    return ok;
}

bool ShowroomManager::generateCarPreview(const QString& carPath, const QString& outputPath)
{
    ShowroomSystem::PreviewConfig config;
    config.outputPath = outputPath;
    config.cameraDistance = m_config.cameraDistance;
    config.cameraHeight = m_config.cameraHeight;
    config.cameraAngle = m_config.cameraAngle;
    config.fov = m_config.cameraFov;
    return ShowroomSystem::generatePreview(carPath, config);
}

bool ShowroomManager::generateCarThumbnail(const QString& carPath)
{
    return ShowroomSystem::generateThumbnail(carPath, "");
}
