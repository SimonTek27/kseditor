#include "ContentRepairTool.h"
#include "../sys/LogManager.h"
#include <QFile>
#include <QTextStream>
#include <algorithm>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QImage>
#include <QPainter>
#include <QFont>
#include <QDebug>

// ============================================================================
// Validation operations
// ============================================================================

ContentRepairTool::RepairReport ContentRepairTool::validateCar(const QString& carPath) {
    RepairReport report;
    report.contentPath = carPath;
    report.contentType = "car";

    report.issues.append(validateCarStructure(carPath));
    report.issues.append(validatePhysicsFiles(carPath));
    report.issues.append(validateTextures(carPath));
    report.issues.append(validateModels(carPath));
    report.issues.append(validateMeshFiles(carPath));
    report.issues.append(validatePhysicsValues(carPath));
    report.issues.append(validateSuspensionGeometry(carPath));
    report.issues.append(validateAeroBalance(carPath));
    report.issues.append(validateTyreData(carPath));
    report.issues.append(validateEngineData(carPath));

    for (const RepairIssue& issue : report.issues) {
        if (issue.severity == "error") report.errorCount++;
        else if (issue.severity == "warning") report.warningCount++;
        else report.infoCount++;

        if (issue.autoFixable) report.autoFixableCount++;
    }

    return report;
}

ContentRepairTool::RepairReport ContentRepairTool::validateTrack(const QString& trackPath) {
    RepairReport report;
    report.contentPath = trackPath;
    report.contentType = "track";

    report.issues.append(validateTrackStructure(trackPath));
    report.issues.append(validateTrackSurfaces(trackPath));
    report.issues.append(validateTrackAI(trackPath));

    for (const RepairIssue& issue : report.issues) {
        if (issue.severity == "error") report.errorCount++;
        else if (issue.severity == "warning") report.warningCount++;
        else report.infoCount++;

        if (issue.autoFixable) report.autoFixableCount++;
    }

    return report;
}

ContentRepairTool::RepairReport ContentRepairTool::validateContent(const QString& contentPath) {
    RepairReport report;

    // Determine content type
    if (QFile::exists(contentPath + "/data/car.ini")) {
        return validateCar(contentPath);
    } else if (QFile::exists(contentPath + "/data/surfaces.ini")) {
        return validateTrack(contentPath);
    }

    return report;
}

// ============================================================================
// Repair operations
// ============================================================================

bool ContentRepairTool::fixIssue(const RepairIssue& issue) {
    if (!issue.autoFixable) return false;

    if (issue.id == "MISSING_PREVIEW") {
        return fixMissingPreview(issue.filePath);
    } else if (issue.id == "MISSING_MAP") {
        return fixMissingMap(issue.filePath);
    } else if (issue.id == "INVALID_INI") {
        return fixInvalidIni(issue.filePath);
    } else if (issue.id == "MISSING_AERO_INI") {
        return fixMissingAeroIni(issue.filePath);
    } else if (issue.id == "NO_DIFFERENTIAL_SECTION") {
        return fixMissingDifferentialIni(issue.filePath);
    }

    return false;
}

bool ContentRepairTool::fixAllIssues(RepairReport& report) {
    bool allFixed = true;

    for (RepairIssue& issue : report.issues) {
        if (issue.autoFixable) {
            if (!fixIssue(issue)) {
                allFixed = false;
            }
        }
    }

    return allFixed;
}

int ContentRepairTool::autoFix(RepairReport& report) {
    int fixedCount = 0;

    for (RepairIssue& issue : report.issues) {
        if (issue.autoFixable) {
            if (fixIssue(issue)) {
                issue.severity = "fixed";
                fixedCount++;
            }
        }
    }

    return fixedCount;
}

// ============================================================================
// Specific validations
// ============================================================================

QVector<ContentRepairTool::RepairIssue> ContentRepairTool::validateCarStructure(const QString& carPath) {
    QVector<RepairIssue> issues;

    // Check required files
    QStringList requiredFiles = getRequiredCarFiles();
    for (const QString& file : requiredFiles) {
        if (!QFile::exists(carPath + "/" + file)) {
            issues.append(createIssue("MISSING_" + file.toUpper().replace("/", "_"),
                                      "Missing required file",
                                      "Required file not found: " + file,
                                      "error", "structure", carPath + "/" + file));
        }
    }

    // Check optional files
    QStringList optionalFiles = getOptionalCarFiles();
    for (const QString& file : optionalFiles) {
        if (!QFile::exists(carPath + "/" + file)) {
            issues.append(createIssue("OPTIONAL_" + file.toUpper().replace("/", "_"),
                                      "Missing optional file",
                                      "Optional file not found: " + file,
                                      "info", "structure", carPath + "/" + file));
        }
    }

    // Check for preview
    if (!QFile::exists(carPath + "/preview.png") && !QFile::exists(carPath + "/preview.jpg")) {
        issues.append(createIssue("MISSING_PREVIEW",
                                  "Missing preview image",
                                  "No preview image found for this car",
                                  "warning", "structure", carPath));
    }

    // Check data folder
    if (!QDir(carPath + "/data").exists()) {
        issues.append(createIssue("MISSING_DATA_FOLDER",
                                  "Missing data folder",
                                  "No data folder found",
                                  "error", "structure", carPath));
    }

    return issues;
}

QVector<ContentRepairTool::RepairIssue> ContentRepairTool::validatePhysicsFiles(const QString& carPath) {
    QVector<RepairIssue> issues;

    QString dataPath = carPath + "/data";
    if (!QDir(dataPath).exists()) return issues;

    // Check car.ini
    QString carIniPath = dataPath + "/car.ini";
    if (QFile::exists(carIniPath)) {
        QFile file(carIniPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            issues.append(createIssue("CAR_INI_READ_ERROR",
                                      "Cannot read car.ini",
                                      "Failed to open car.ini for reading",
                                      "error", "physics", carIniPath));
        } else {
            QTextStream stream(&file);
            bool hasName = false;
            bool hasMass = false;

            while (!stream.atEnd()) {
                QString line = stream.readLine().trimmed();
                if (line.contains("NAME=")) hasName = true;
                if (line.contains("MASSES=")) hasMass = true;
            }

            file.close();

            if (!hasName) {
                issues.append(createIssue("CAR_INI_NO_NAME",
                                          "Missing NAME in car.ini",
                                          "car.ini does not contain a NAME entry",
                                          "warning", "physics", carIniPath));
            }

            if (!hasMass) {
                issues.append(createIssue("CAR_INI_NO_MASS",
                                          "Missing MASSES in car.ini",
                                          "car.ini does not contain a MASSES entry",
                                          "error", "physics", carIniPath));
            }
        }
    }

    // Check tyres.ini
    QString tyresIniPath = dataPath + "/tyres.ini";
    if (!QFile::exists(tyresIniPath)) {
        issues.append(createIssue("MISSING_TYRES_INI",
                                  "Missing tyres.ini",
                                  "No tyres.ini found in data folder",
                                  "error", "physics", tyresIniPath));
    }

    // Check engine.ini
    QString engineIniPath = dataPath + "/engine.ini";
    if (!QFile::exists(engineIniPath)) {
        issues.append(createIssue("MISSING_ENGINE_INI",
                                  "Missing engine.ini",
                                  "No engine.ini found in data folder",
                                  "error", "physics", engineIniPath));
    }

    return issues;
}

QVector<ContentRepairTool::RepairIssue> ContentRepairTool::validateTextures(const QString& carPath) {
    QVector<RepairIssue> issues;

    QString texturePath = carPath + "/textures";
    if (!QDir(texturePath).exists()) return issues;

    QStringList textureFiles = QDir(texturePath).entryList(QStringList() << "*.dds" << "*.png" << "*.jpg", QDir::Files);

    for (const QString& file : textureFiles) {
        QString filePath = texturePath + "/" + file;
        QFileInfo info(filePath);

        if (info.size() == 0) {
            issues.append(createIssue("EMPTY_TEXTURE",
                                      "Empty texture file",
                                      "Texture file is empty: " + file,
                                      "error", "texture", filePath));
        } else if (info.size() > 10 * 1024 * 1024) { // > 10MB
            issues.append(createIssue("LARGE_TEXTURE",
                                      "Unusually large texture",
                                      "Texture file is very large: " + file + " (" + QString::number(info.size() / 1024 / 1024) + " MB)",
                                      "warning", "texture", filePath));
        }
    }

    return issues;
}

QVector<ContentRepairTool::RepairIssue> ContentRepairTool::validateModels(const QString& carPath) {
    QVector<RepairIssue> issues;

    // Check for KN5 files
    QStringList kn5Files = QDir(carPath).entryList(QStringList() << "*.kn5", QDir::Files);

    if (kn5Files.isEmpty()) {
        issues.append(createIssue("NO_KN5_MODELS",
                                  "No KN5 models found",
                                  "No .kn5 model files found in car folder",
                                  "error", "model", carPath));
    }

    for (const QString& file : kn5Files) {
        QString filePath = carPath + "/" + file;
        QFileInfo info(filePath);

        if (info.size() == 0) {
            issues.append(createIssue("EMPTY_KN5",
                                      "Empty KN5 file",
                                      "KN5 file is empty: " + file,
                                      "error", "model", filePath));
        }
    }

    return issues;
}

QVector<ContentRepairTool::RepairIssue> ContentRepairTool::validateTrackStructure(const QString& trackPath) {
    QVector<RepairIssue> issues;

    // Check required files
    QStringList requiredFiles = getRequiredTrackFiles();
    for (const QString& file : requiredFiles) {
        if (!QFile::exists(trackPath + "/" + file)) {
            issues.append(createIssue("MISSING_" + file.toUpper().replace("/", "_"),
                                      "Missing required file",
                                      "Required file not found: " + file,
                                      "error", "structure", trackPath + "/" + file));
        }
    }

    // Check for preview
    if (!QFile::exists(trackPath + "/ui/ui_track.json")) {
        issues.append(createIssue("MISSING_UI_TRACK",
                                  "Missing ui_track.json",
                                  "No ui_track.json found in ui folder",
                                  "error", "structure", trackPath));
    }

    return issues;
}

QVector<ContentRepairTool::RepairIssue> ContentRepairTool::validateTrackSurfaces(const QString& trackPath) {
    QVector<RepairIssue> issues;

    QString surfacesPath = trackPath + "/data/surfaces.ini";
    if (!QFile::exists(surfacesPath)) {
        issues.append(createIssue("MISSING_SURFACES_INI",
                                  "Missing surfaces.ini",
                                  "No surfaces.ini found in data folder",
                                  "error", "config", surfacesPath));
        return issues;
    }

    // Validate surfaces.ini content
    QFile file(surfacesPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        bool hasSurfaces = false;

        while (!stream.atEnd()) {
            QString line = stream.readLine().trimmed();
            if (line.startsWith("[SURFACE_")) {
                hasSurfaces = true;
                break;
            }
        }

        file.close();

        if (!hasSurfaces) {
            issues.append(createIssue("NO_SURFACES_DEFINED",
                                      "No surfaces defined",
                                      "surfaces.ini does not contain any surface definitions",
                                      "error", "config", surfacesPath));
        }
    }

    return issues;
}

QVector<ContentRepairTool::RepairIssue> ContentRepairTool::validateTrackAI(const QString& trackPath) {
    QVector<RepairIssue> issues;

    QString aiPath = trackPath + "/ai";
    if (!QDir(aiPath).exists()) {
        issues.append(createIssue("MISSING_AI_FOLDER",
                                  "Missing AI folder",
                                  "No ai folder found",
                                  "warning", "structure", trackPath));
        return issues;
    }

    // Check for AI line
    if (!QFile::exists(aiPath + "/fast_lane.ai")) {
        issues.append(createIssue("MISSING_AI_LINE",
                                  "Missing AI line",
                                  "No fast_lane.ai found",
                                  "warning", "structure", aiPath));
    }

    return issues;
}

// ============================================================================
// Auto-fix operations
// ============================================================================

bool ContentRepairTool::fixMissingFile(const QString& filePath, const QString& templatePath) {
    if (QFile::exists(templatePath)) {
        return QFile::copy(templatePath, filePath);
    }
    return false;
}

bool ContentRepairTool::fixInvalidIni(const QString& iniPath) {
    // Try to repair invalid INI file
    QFile file(iniPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    QString content = stream.readAll();
    file.close();

    // Basic repairs
    content.replace("\r\n", "\n");
    content.replace("\r", "\n");

    // Remove null bytes
    content.replace(QChar(0), "");

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream outStream(&file);
        outStream << content;
        file.close();
        return true;
    }

    return false;
}

bool ContentRepairTool::fixMissingAeroIni(const QString& carDataPath) {
    // Create a basic aero.ini with sensible defaults
    QString aeroPath = carDataPath + "/aero.ini";
    QFile file(aeroPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QTextStream out(&file);
    out << "; Auto-generated aero.ini by ksEditor ContentRepair\n";
    out << "; Replace with proper values for your car\n\n";
    out << "[DATA]\n";
    out << "CD=0.35\n";
    out << "FRONT=100\n";
    out << "REAR=200\n\n";
    out << "[FRONT]\n";
    out << "WING_ANGLE=0\n\n";
    out << "[REAR]\n";
    out << "WING_ANGLE=0\n";
    file.close();

    LOG_INFO("ContentRepairTool", QString("Created default aero.ini: %1").arg(aeroPath));
    return true;
}

bool ContentRepairTool::fixMissingDifferentialIni(const QString& diffIniPath) {
    // Add a default [DIFFERENTIAL] section to existing differential.ini
    QFile file(diffIniPath);
    if (!file.open(QIODevice::Append | QIODevice::Text)) return false;

    QTextStream out(&file);
    out << "\n[DIFFERENTIAL]\n";
    out << "POWER=60\n";
    out << "COAST=30\n";
    out << "PRELOAD=20\n";
    file.close();

    LOG_INFO("ContentRepairTool", QString("Added differential section: %1").arg(diffIniPath));
    return true;
}

bool ContentRepairTool::fixMissingTexture(const QString& texturePath) {
    QDir().mkpath(QFileInfo(texturePath).absolutePath());

    QImage tex(256, 256, QImage::Format_ARGB32);
    tex.fill(QColor(80, 60, 100));

    QPainter p(&tex);
    p.setPen(QPen(QColor(120, 100, 140), 2));
    p.drawRect(tex.rect().adjusted(2, 2, -2, -2));
    p.drawLine(0, 0, 255, 255);
    p.drawLine(255, 0, 0, 255);
    QFont f("Segoe UI", 9);
    p.setFont(f);
    p.setPen(QColor(180, 160, 200));
    p.drawText(tex.rect(), Qt::AlignCenter, "MISSING\nTEXTURE");
    p.end();

    if (texturePath.toLower().endsWith(".dds")) {
        if (!tex.save(texturePath, "DDS")) {
            QString pngPath = texturePath;
            pngPath.replace(".dds", ".png", Qt::CaseInsensitive);
            tex.save(pngPath, "PNG");
        }
    } else {
        tex.save(texturePath, "PNG");
    }

    LOG_INFO("ContentRepairTool", QString("Created placeholder texture: %1").arg(texturePath));
    return true;
}

bool ContentRepairTool::fixMissingPreview(const QString& contentPath) {
    QString previewPath = contentPath + "/preview.jpg";
    QDir().mkpath(contentPath);

    QImage preview(512, 256, QImage::Format_ARGB32);
    preview.fill(QColor(40, 42, 52));

    QPainter p(&preview);
    p.setRenderHint(QPainter::Antialiasing);

    // Dark background gradient
    QLinearGradient bg(0, 0, 0, 256);
    bg.setColorAt(0, QColor(45, 47, 58));
    bg.setColorAt(1, QColor(35, 37, 45));
    p.setBrush(bg);
    p.setPen(Qt::NoPen);
    p.drawRect(preview.rect());

    // Center icon area
    QRectF iconRect(206, 50, 100, 100);
    p.setBrush(QColor(55, 57, 68));
    p.setPen(QPen(QColor(75, 77, 88), 2));
    p.drawRoundedRect(iconRect, 12, 12);

    // Eye slash icon
    p.setPen(QPen(QColor(130, 133, 145), 3));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QPointF(256, 100), 20, 20);
    p.drawLine(QPointF(242, 86), QPointF(270, 114));

    // Filename
    QString label = QFileInfo(contentPath).fileName();
    QFont f("Segoe UI", 13, QFont::Bold);
    p.setFont(f);
    p.setPen(QColor(180, 185, 200));
    label = QFontMetrics(p.font()).elidedText(label, Qt::ElideMiddle, 400);
    p.drawText(QRect(56, 165, 400, 30), Qt::AlignCenter, label);

    // Sub-label
    p.setFont(QFont("Segoe UI", 10));
    p.setPen(QColor(100, 102, 115));
    p.drawText(QRect(56, 190, 400, 20), Qt::AlignCenter, "preview missing");
    p.end();

    if (!preview.save(previewPath, "JPG", 85)) return false;

    LOG_INFO("ContentRepairTool", QString("Created placeholder preview: %1").arg(previewPath));
    return true;
}

bool ContentRepairTool::fixMissingMap(const QString& trackPath) {
    QString mapPath = trackPath + "/map.png";
    QDir().mkpath(trackPath);

    QImage map(256, 256, QImage::Format_ARGB32);
    QPainter p(&map);
    p.setRenderHint(QPainter::Antialiasing);

    // Dark background
    p.fillRect(map.rect(), QColor(30, 35, 40));

    // Outer border
    p.setPen(QPen(QColor(60, 70, 80), 3));
    p.setBrush(Qt::NoBrush);
    p.drawRect(map.rect().adjusted(10, 10, -10, -10));

    // Inner grid
    p.setPen(QPen(QColor(45, 50, 58), 1));
    for (int i = 0; i <= 8; i++) {
        int pos = 10 + i * (236 / 8);
        p.drawLine(pos, 10, pos, 246);
        p.drawLine(10, pos, 246, pos);
    }

    // Track outline placeholder (oval)
    p.setPen(QPen(QColor(200, 160, 60), 3));
    p.setBrush(QBrush(QColor(60, 80, 50, 40)));
    p.drawEllipse(QPointF(128, 128), 60, 40);

    // Start/finish line
    p.setPen(QPen(Qt::white, 2));
    p.drawLine(100, 108, 100, 148);

    // Label
    QFont f("Segoe UI", 10);
    p.setFont(f);
    p.setPen(QColor(100, 110, 120));
    p.drawText(map.rect().adjusted(0, 0, 0, -8), Qt::AlignBottom | Qt::AlignHCenter, "track map unavailable");
    p.end();

    if (!map.save(mapPath, "PNG")) return false;

    LOG_INFO("ContentRepairTool", QString("Created placeholder map: %1").arg(mapPath));
    return true;
}

// ============================================================================
// File integrity
// ============================================================================

bool ContentRepairTool::verifyFileIntegrity(const QString& filePath) {
    QFile file(filePath);
    if (!file.exists()) return false;
    if (file.size() == 0) return false;

    QString ext = QFileInfo(filePath).suffix().toLower();

    if (ext == "ini" || ext == "cfg" || ext == "json" || ext == "txt" || ext == "lut") {
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
        QByteArray content = file.readAll();
        file.close();
        if (content.contains('\0')) return false;
        if (content.isEmpty()) return false;
        return true;
    }

    if (ext == "dds") {
        if (!file.open(QIODevice::ReadOnly)) return false;
        QByteArray header = file.read(4);
        file.close();
        return header == "DDS ";
    }

    if (ext == "kn5") {
        if (!file.open(QIODevice::ReadOnly)) return false;
        QByteArray magic = file.read(3);
        file.close();
        return magic == "KN5";
    }

    if (ext == "wav") {
        if (!file.open(QIODevice::ReadOnly)) return false;
        QByteArray header = file.read(4);
        file.close();
        return header == "RIFF";
    }

    if (ext == "jpg" || ext == "jpeg") {
        if (!file.open(QIODevice::ReadOnly)) return false;
        QByteArray header = file.read(2);
        file.close();
        return header == QByteArray::fromHex("ffd8");
    }

    if (ext == "png") {
        if (!file.open(QIODevice::ReadOnly)) return false;
        QByteArray header = file.read(8);
        file.close();
        return header == QByteArray::fromHex("89504e470d0a1a0a");
    }

    return true;
}

QString ContentRepairTool::calculateFileHash(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return QString();

    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (hash.addData(&file)) {
        return hash.result().toHex();
    }

    return QString();
}

bool ContentRepairTool::backupFile(const QString& filePath) {
    if (!QFile::exists(filePath)) return false;

    QString backupPath = filePath + ".bak";
    return QFile::copy(filePath, backupPath);
}

bool ContentRepairTool::restoreFile(const QString& filePath) {
    QString backupPath = filePath + ".bak";
    if (!QFile::exists(backupPath)) return false;

    QFile::remove(filePath);
    return QFile::copy(backupPath, filePath);
}

// ============================================================================
// Utility
// ============================================================================

QStringList ContentRepairTool::getRequiredCarFiles() {
    return QStringList() << "data/car.ini" << "data/tyres.ini" << "data/engine.ini"
                         << "data/brakes.ini" << "data/suspensions.ini";
}

QStringList ContentRepairTool::getRequiredTrackFiles() {
    return QStringList() << "data/surfaces.ini" << "ui/ui_track.json";
}

QStringList ContentRepairTool::getOptionalCarFiles() {
    return QStringList() << "data/aero.ini" << "data/differential.ini"
                         << "data/electronics.ini" << "data/damage.ini"
                         << "data/cameras.ini" << "data/driver3d.ini";
}

QStringList ContentRepairTool::getOptionalTrackFiles() {
    return QStringList() << "data/ai_hints.ini" << "data/grass.ini"
                         << "data/terrain.ini" << "data/cameras.ini"
                         << "models.ini";
}

// ============================================================================
// Advanced Validations
// ============================================================================

QVector<ContentRepairTool::RepairIssue> ContentRepairTool::validateMeshFiles(const QString& carPath) {
    QVector<RepairIssue> issues;

    QStringList kn5Files = QDir(carPath).entryList(QStringList() << "*.kn5", QDir::Files);

    for (const QString& file : kn5Files) {
        QString filePath = carPath + "/" + file;
        QFile f(filePath);

        if (!f.open(QIODevice::ReadOnly)) {
            issues.append(createIssue("KN5_READ_ERROR",
                "Cannot read KN5 file",
                "Failed to open: " + file,
                "error", "model", filePath));
            continue;
        }

        QByteArray magic = f.read(4);
        f.close();

        if (magic != "KN5 ") {
            issues.append(createIssue("KN5_INVALID_HEADER",
                "Invalid KN5 header",
                "File does not have a valid KN5 header: " + file,
                "error", "model", filePath));
            continue;
        }

        // Check file size
        QFileInfo fi(filePath);
        if (fi.size() < 1024) {
            issues.append(createIssue("KN5_TOO_SMALL",
                "KN5 file is too small",
                "File is " + QString::number(fi.size()) + " bytes, expected at least 1KB",
                "warning", "model", filePath));
        }
    }

    // Check for .kn5 in data folder too
    QString dataPath = carPath + "/data";
    if (QDir(dataPath).exists()) {
        QStringList dataKn5 = QDir(dataPath).entryList(QStringList() << "*.kn5", QDir::Files);
        for (const QString& file : dataKn5) {
            QString filePath = dataPath + "/" + file;
            QFile f(filePath);

            if (!f.open(QIODevice::ReadOnly)) {
                issues.append(createIssue("KN5_DATA_READ_ERROR",
                    "Cannot read KN5 in data folder",
                    "Failed to open: data/" + file,
                    "error", "model", filePath));
                continue;
            }

            QByteArray magic = f.read(4);
            f.close();

            if (magic != "KN5 ") {
                issues.append(createIssue("KN5_DATA_INVALID",
                    "Invalid KN5 in data folder",
                    "data/" + file + " has an invalid header",
                    "error", "model", filePath));
            }
        }
    }

    // Check for non-KN5 model files
    QStringList otherModels = QDir(carPath).entryList(
        QStringList() << "*.fbx" << "*.obj" << "*.glb" << "*.gltf", QDir::Files);
    if (!otherModels.isEmpty() && kn5Files.isEmpty()) {
        issues.append(createIssue("NO_KN5_BUT_SOURCE_EXISTS",
            "Only source models found, no compiled .kn5",
            "Source models exist but no compiled .kn5: " + otherModels.join(", "),
            "warning", "model", carPath));
    }

    return issues;
}

QVector<ContentRepairTool::RepairIssue> ContentRepairTool::validatePhysicsValues(const QString& carPath) {
    QVector<RepairIssue> issues;

    QString dataPath = carPath + "/data";
    if (!QDir(dataPath).exists()) return issues;

    // Validate car.ini values
    QString carIniPath = dataPath + "/car.ini";
    if (QFile::exists(carIniPath)) {
        QFile file(carIniPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();

                // Check MASSES
                if (line.startsWith("MASSES=")) {
                    QStringList parts = line.mid(7).split('|');
                    if (parts.size() >= 3) {
                        float totalMass = parts[0].toFloat();
                        if (totalMass < 200 || totalMass > 10000) {
                            issues.append(createIssue("MASS_OUT_OF_RANGE",
                                "Car mass seems unrealistic",
                                "Total mass is " + QString::number(totalMass) + " kg (expected 200-10000)",
                                "warning", "physics", carIniPath));
                        }
                    }
                }

                // Check MAXFUEL
                if (line.startsWith("MAXFUEL=")) {
                    float fuel = line.mid(8).toFloat();
                    if (fuel < 1 || fuel > 200) {
                        issues.append(createIssue("FUEL_OUT_OF_RANGE",
                            "Fuel capacity seems unrealistic",
                            "Max fuel is " + QString::number(fuel) + " L (expected 1-200)",
                            "warning", "physics", carIniPath));
                    }
                }
            }
            file.close();
        }
    }

    // Validate engine.ini values
    QString engineIniPath = dataPath + "/engine.ini";
    if (QFile::exists(engineIniPath)) {
        QFile file(engineIniPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();

                if (line.startsWith("MAXRPM=")) {
                    float rpm = line.mid(7).toFloat();
                    if (rpm < 2000 || rpm > 15000) {
                        issues.append(createIssue("MAXRPM_OUT_OF_RANGE",
                            "Max RPM seems unrealistic",
                            "MAXRPM is " + QString::number(rpm) + " (expected 2000-15000)",
                            "warning", "physics", engineIniPath));
                    }
                }

                if (line.startsWith("IDLE_RPM=")) {
                    float rpm = line.mid(9).toFloat();
                    if (rpm < 300 || rpm > 3000) {
                        issues.append(createIssue("IDLE_RPM_OUT_OF_RANGE",
                            "Idle RPM seems unrealistic",
                            "IDLE_RPM is " + QString::number(rpm) + " (expected 300-3000)",
                            "warning", "physics", engineIniPath));
                    }
                }

                if (line.startsWith("FUEL_CONSUMPTION=")) {
                    float cons = line.mid(17).toFloat();
                    if (cons < 0.0001f || cons > 1.0f) {
                        issues.append(createIssue("FUEL_CONSUMPTION_ODD",
                            "Fuel consumption seems unrealistic",
                            "FUEL_CONSUMPTION is " + QString::number(cons) + " (expected 0.0001-1.0)",
                            "warning", "physics", engineIniPath));
                    }
                }
            }
            file.close();
        }
    }

    // Validate brakes.ini
    QString brakesIniPath = dataPath + "/brakes.ini";
    if (QFile::exists(brakesIniPath)) {
        QFile file(brakesIniPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();

                if (line.startsWith("BRAKE_TORQUE_")) {
                    float torque = line.mid(line.indexOf('=') + 1).toFloat();
                    if (torque < 100 || torque > 100000) {
                        issues.append(createIssue("BRAKE_TORQUE_SUSPICIOUS",
                            "Suspicious brake torque value",
                            line.left(line.indexOf('=')) + " is " + QString::number(torque) + " (expected 100-100000)",
                            "warning", "physics", brakesIniPath));
                    }
                }
            }
            file.close();
        }
    }

    // Validate differential.ini
    QString diffIniPath = dataPath + "/differential.ini";
    if (QFile::exists(diffIniPath)) {
        QFile file(diffIniPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            bool hasDiff = false;
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (line.startsWith("[DIFFERENTIAL]") || line.startsWith("[DIFF_")) {
                    hasDiff = true;
                    break;
                }
            }
            file.close();

            if (!hasDiff) {
                issues.append(createIssue("NO_DIFFERENTIAL_SECTION",
                    "No differential section found",
                    "differential.ini has no [DIFFERENTIAL] or [DIFF_*] section",
                    "info", "physics", diffIniPath));
            }
        }
    }

    return issues;
}

QVector<ContentRepairTool::RepairIssue> ContentRepairTool::validateSuspensionGeometry(const QString& carPath) {
    QVector<RepairIssue> issues;

    QString dataPath = carPath + "/data";
    if (!QDir(dataPath).exists()) return issues;

    // Validate suspensions.ini
    QString suspIniPath = dataPath + "/suspensions.ini";
    if (QFile::exists(suspIniPath)) {
        QFile file(suspIniPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();

                if (line.startsWith("SPRING_RATE=")) {
                    float rate = line.mid(12).toFloat();
                    if (rate < 1000 || rate > 500000) {
                        issues.append(createIssue("SPRING_RATE_SUSPICIOUS",
                            "Suspicious spring rate",
                            "SPRING_RATE is " + QString::number(rate) + " (expected 1000-500000)",
                            "warning", "physics", suspIniPath));
                    }
                }

                if (line.startsWith("DAMPING=")) {
                    float damp = line.mid(8).toFloat();
                    if (damp < 100 || damp > 100000) {
                        issues.append(createIssue("DAMPING_SUSPICIOUS",
                            "Suspicious damping value",
                            "DAMPING is " + QString::number(damp) + " (expected 100-100000)",
                            "warning", "physics", suspIniPath));
                    }
                }

                if (line.startsWith("RIDE_HEIGHT=")) {
                    float height = line.mid(12).toFloat();
                    if (height < 0.01f || height > 0.5f) {
                        issues.append(createIssue("RIDE_HEIGHT_SUSPICIOUS",
                            "Suspicious ride height",
                            "RIDE_HEIGHT is " + QString::number(height) + "m (expected 0.01-0.5)",
                            "warning", "physics", suspIniPath));
                    }
                }

                if (line.startsWith("CAMBER=")) {
                    float camber = line.mid(7).toFloat();
                    if (camber < -10 || camber > 10) {
                        issues.append(createIssue("CAMBER_OUT_OF_RANGE",
                            "Camber angle is unrealistic",
                            "CAMBER is " + QString::number(camber) + " deg (expected -10 to 10)",
                            "warning", "physics", suspIniPath));
                    }
                }

                if (line.startsWith("TOE=")) {
                    float toe = line.mid(4).toFloat();
                    if (toe < -5 || toe > 5) {
                        issues.append(createIssue("TOE_OUT_OF_RANGE",
                            "Toe angle is unrealistic",
                            "TOE is " + QString::number(toe) + " deg (expected -5 to 5)",
                            "warning", "physics", suspIniPath));
                    }
                }
            }
            file.close();
        }
    }

    return issues;
}

QVector<ContentRepairTool::RepairIssue> ContentRepairTool::validateAeroBalance(const QString& carPath) {
    QVector<RepairIssue> issues;

    QString dataPath = carPath + "/data";
    if (!QDir(dataPath).exists()) return issues;

    QString aeroIniPath = dataPath + "/aero.ini";
    if (!QFile::exists(aeroIniPath)) {
        issues.append(createIssue("MISSING_AERO_INI",
            "Missing aero.ini",
            "No aero.ini found - car will have no downforce at speed",
            "info", "physics", dataPath));
        return issues;
    }

    QFile file(aeroIniPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        bool hasAero = false;
        float cd = 0.5f;
        bool hasCd = false;

        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();

            if (line.startsWith("[FRONT]") || line.startsWith("[REAR]") ||
                line.startsWith("[AERO_") || line.startsWith("[DATA]")) {
                hasAero = true;
            }

            if (line.startsWith("CD=")) {
                cd = line.mid(3).toFloat();
                hasCd = true;
                if (cd < 0.1f || cd > 2.0f) {
                    issues.append(createIssue("CD_OUT_OF_RANGE",
                        "Drag coefficient is unrealistic",
                        "CD is " + QString::number(cd) + " (expected 0.1-2.0)",
                        "warning", "physics", aeroIniPath));
                }
            }

            if (line.startsWith("FRONT=")) {
                float front = line.mid(6).toFloat();
                if (front < 0 || front > 100000) {
                    issues.append(createIssue("FRONT_DF_OUT_OF_RANGE",
                        "Front downforce is unrealistic",
                        "FRONT downforce is " + QString::number(front) + " (expected 0-100000)",
                        "warning", "physics", aeroIniPath));
                }
            }

            if (line.startsWith("REAR=")) {
                float rear = line.mid(5).toFloat();
                if (rear < 0 || rear > 100000) {
                    issues.append(createIssue("REAR_DF_OUT_OF_RANGE",
                        "Rear downforce is unrealistic",
                        "REAR downforce is " + QString::number(rear) + " (expected 0-100000)",
                        "warning", "physics", aeroIniPath));
                }
            }
        }
        file.close();

        if (hasCd && cd < 0.2f) {
            issues.append(createIssue("VERY_LOW_CD",
                "Very low drag coefficient",
                "CD is " + QString::number(cd) + " - might cause unrealistic top speed",
                "info", "physics", aeroIniPath));
        }

        if (!hasAero) {
            issues.append(createIssue("NO_AERO_SECTION",
                "No aerodynamic sections defined",
                "aero.ini has no [FRONT], [REAR], or [AERO_*] sections",
                "error", "physics", aeroIniPath));
        }
    }

    return issues;
}

QVector<ContentRepairTool::RepairIssue> ContentRepairTool::validateTyreData(const QString& carPath) {
    QVector<RepairIssue> issues;

    QString dataPath = carPath + "/data";
    if (!QDir(dataPath).exists()) return issues;

    QString tyresIniPath = dataPath + "/tyres.ini";
    if (!QFile::exists(tyresIniPath)) {
        return issues;
    }

    QFile file(tyresIniPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();

            if (line.startsWith("PRESSURE_")) {
                float psi = line.mid(line.indexOf('=') + 1).toFloat();
                if (psi < 10 || psi > 60) {
                    issues.append(createIssue("TYRE_PRESSURE_OUT_OF_RANGE",
                        "Tyre pressure seems unrealistic",
                        line.left(line.indexOf('=')) + " is " + QString::number(psi) + " psi (expected 10-60)",
                        "warning", "physics", tyresIniPath));
                }
            }

            if (line.startsWith("CARCASS=")) {
                float carcass = line.mid(8).toFloat();
                if (carcass < 0.1f || carcass > 10.0f) {
                    issues.append(createIssue("CARCASS_STIFFNESS_ODD",
                        "Carcass stiffness seems unusual",
                        "CARCASS is " + QString::number(carcass) + " (expected 0.1-10.0)",
                        "warning", "physics", tyresIniPath));
                }
            }

            if (line.startsWith("HEAT_CAPACITY=")) {
                float hc = line.mid(14).toFloat();
                if (hc < 1 || hc > 100000) {
                    issues.append(createIssue("HEAT_CAPACITY_ODD",
                        "Tyre heat capacity seems unrealistic",
                        "HEAT_CAPACITY is " + QString::number(hc) + " (expected 1-100000)",
                        "warning", "physics", tyresIniPath));
                }
            }
        }
        file.close();
    }

    return issues;
}

QVector<ContentRepairTool::RepairIssue> ContentRepairTool::validateEngineData(const QString& carPath) {
    QVector<RepairIssue> issues;

    QString dataPath = carPath + "/data";
    if (!QDir(dataPath).exists()) return issues;

    QString engineIniPath = dataPath + "/engine.ini";
    if (!QFile::exists(engineIniPath)) return issues;

    QFile file(engineIniPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QVector<double> rpm, power, torque;

        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();

            if (line.startsWith("RPM_")) {
                rpm.append(line.mid(line.indexOf('=') + 1).toDouble());
            }
            if (line.startsWith("POWER_")) {
                power.append(line.mid(line.indexOf('=') + 1).toDouble());
            }
            if (line.startsWith("TORQUE_")) {
                torque.append(line.mid(line.indexOf('=') + 1).toDouble());
            }
        }
        file.close();

        if (rpm.isEmpty()) {
            issues.append(createIssue("ENGINE_NO_RPM_DATA",
                "No RPM data in engine.ini",
                "No RPM_ entries found - engine curve cannot be determined",
                "error", "physics", engineIniPath));
        }

        if (rpm.size() != power.size()) {
            issues.append(createIssue("ENGINE_POWER_MISMATCH",
                "RPM/POWER count mismatch",
                "Found " + QString::number(rpm.size()) + " RPM entries vs "
                + QString::number(power.size()) + " POWER entries",
                "error", "physics", engineIniPath));
        }

        if (rpm.size() != torque.size()) {
            issues.append(createIssue("ENGINE_TORQUE_MISMATCH",
                "RPM/TORQUE count mismatch",
                "Found " + QString::number(rpm.size()) + " RPM entries vs "
                + QString::number(torque.size()) + " TORQUE entries",
                "error", "physics", engineIniPath));
        }

        if (!rpm.isEmpty()) {
            // Check RPM values are monotonically increasing
            for (int i = 1; i < rpm.size(); ++i) {
                if (rpm[i] <= rpm[i - 1]) {
                    issues.append(createIssue("ENGINE_RPM_NOT_MONOTONIC",
                        "RPM values are not monotonically increasing",
                        "RPM_" + QString::number(i) + " (" + QString::number(rpm[i])
                        + ") <= RPM_" + QString::number(i - 1) + " (" + QString::number(rpm[i - 1]) + ")",
                        "error", "physics", engineIniPath));
                    break;
                }
            }
        }

        if (!power.isEmpty()) {
            float maxPower = *std::max_element(power.begin(), power.end());
            if (maxPower < 20 || maxPower > 5000) {
                issues.append(createIssue("ENGINE_POWER_EXTREME",
                    "Engine power seems unrealistic",
                    "Max power is " + QString::number(maxPower) + " HP (expected 20-5000)",
                    "warning", "physics", engineIniPath));
            }
        }
    }

    return issues;
}

// ============================================================================
// Private helpers
// ============================================================================

ContentRepairTool::RepairIssue ContentRepairTool::createIssue(
    const QString& id, const QString& title, const QString& description,
    const QString& severity, const QString& category, const QString& filePath) {

    RepairIssue issue;
    issue.id = id;
    issue.title = title;
    issue.description = description;
    issue.severity = severity;
    issue.category = category;
    issue.filePath = filePath;

    // Determine if auto-fixable
    if (id.startsWith("MISSING_PREVIEW") || id.startsWith("MISSING_MAP") ||
        id.startsWith("INVALID_INI") || id == "MISSING_AERO_INI" ||
        id == "NO_DIFFERENTIAL_SECTION" || id == "ENGINE_NO_RPM_DATA" ||
        id == "MISSING_TYRES_INI") {
        issue.autoFixable = true;
        issue.suggestedFix = "Auto-fix available: " + title;
    }

    return issue;
}

// ============================================================================
// ContentRepairManager implementation
// ============================================================================

ContentRepairManager::ContentRepairManager() {
}

ContentRepairTool::RepairReport ContentRepairManager::scanContent(const QString& contentPath) {
    ContentRepairTool::RepairReport report = ContentRepairTool::validateContent(contentPath);
    m_reports.append(report);
    return report;
}

ContentRepairTool::RepairReport ContentRepairManager::scanCar(const QString& carPath) {
    ContentRepairTool::RepairReport report = ContentRepairTool::validateCar(carPath);
    m_reports.append(report);
    return report;
}

ContentRepairTool::RepairReport ContentRepairManager::scanTrack(const QString& trackPath) {
    ContentRepairTool::RepairReport report = ContentRepairTool::validateTrack(trackPath);
    m_reports.append(report);
    return report;
}

bool ContentRepairManager::fixSelected(const QVector<ContentRepairTool::RepairIssue>& issues) {
    bool anyFixed = false;

    for (const ContentRepairTool::RepairIssue& issue : issues) {
        if (ContentRepairTool::fixIssue(issue)) {
            m_fixedCount++;
            anyFixed = true;
        }
    }

    return anyFixed;
}

bool ContentRepairManager::fixAll(ContentRepairTool::RepairReport& report) {
    bool allFixed = ContentRepairTool::fixAllIssues(report);
    m_fixedCount += report.autoFixableCount;
    return allFixed;
}

bool ContentRepairManager::autoFix(ContentRepairTool::RepairReport& report) {
    int fixedCount = ContentRepairTool::autoFix(report);
    if (fixedCount > 0) {
        m_fixedCount += fixedCount;
    }
    return fixedCount > 0;
}

QString ContentRepairManager::generateReport(const ContentRepairTool::RepairReport& report) {
    QString text;
    text += "Content Repair Report\n";
    text += "====================\n\n";
    text += "Path: " + report.contentPath + "\n";
    text += "Type: " + report.contentType + "\n\n";
    text += "Summary:\n";
    text += "  Errors: " + QString::number(report.errorCount) + "\n";
    text += "  Warnings: " + QString::number(report.warningCount) + "\n";
    text += "  Info: " + QString::number(report.infoCount) + "\n";
    text += "  Auto-fixable: " + QString::number(report.autoFixableCount) + "\n\n";

    text += "Issues:\n";
    for (const ContentRepairTool::RepairIssue& issue : report.issues) {
        text += "  [" + issue.severity.toUpper() + "] " + issue.title + "\n";
        text += "    " + issue.description + "\n";
        text += "    File: " + issue.filePath + "\n\n";
    }

    return text;
}

bool ContentRepairManager::exportReport(const ContentRepairTool::RepairReport& report, const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    stream << generateReport(report);
    file.close();

    return true;
}

int ContentRepairManager::getTotalIssues() const {
    int total = 0;
    for (const ContentRepairTool::RepairReport& report : m_reports) {
        total += report.issues.size();
    }
    return total;
}

int ContentRepairManager::getFixedIssues() const {
    return m_fixedCount;
}
