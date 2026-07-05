#include "ContentBrowser.h"
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QDebug>
#include <QImage>
#include <QPainter>
#include <QtCore/private/qzipreader_p.h>

// ============================================================================
// Browsing operations
// ============================================================================

QVector<ContentBrowser::ContentItem> ContentBrowser::browseContent(
    const QString& contentPath, const ContentFilter& filter) {

    QVector<ContentItem> items;

    // Browse cars
    QString carsPath = contentPath + "/cars";
    if (QDir(carsPath).exists()) {
        items.append(browseCars(carsPath, filter));
    }

    // Browse tracks
    QString tracksPath = contentPath + "/tracks";
    if (QDir(tracksPath).exists()) {
        items.append(browseTracks(tracksPath, filter));
    }

    // Browse weather
    QString weatherPath = contentPath + "/weather";
    if (QDir(weatherPath).exists()) {
        items.append(browseWeather(weatherPath));
    }

    return items;
}

QVector<ContentBrowser::ContentItem> ContentBrowser::browseCars(
    const QString& carsPath, const ContentFilter& filter) {

    QVector<ContentItem> items;

    QDir carsDir(carsPath);
    if (!carsDir.exists()) return items;

    QFileInfoList dirs = carsDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QFileInfo& dirInfo : dirs) {
        ContentItem item = getItemInfo(dirInfo.absoluteFilePath());
        if (item.type == "car" && matchesFilter(item, filter)) {
            items.append(item);
        }
    }

    return items;
}

QVector<ContentBrowser::ContentItem> ContentBrowser::browseTracks(
    const QString& tracksPath, const ContentFilter& filter) {

    QVector<ContentItem> items;

    QDir tracksDir(tracksPath);
    if (!tracksDir.exists()) return items;

    QFileInfoList dirs = tracksDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QFileInfo& dirInfo : dirs) {
        ContentItem item = getItemInfo(dirInfo.absoluteFilePath());
        if (item.type == "track" && matchesFilter(item, filter)) {
            items.append(item);
        }
    }

    return items;
}

QVector<ContentBrowser::ContentItem> ContentBrowser::browseWeather(const QString& weatherPath) {
    QVector<ContentItem> items;

    QDir weatherDir(weatherPath);
    if (!weatherDir.exists()) return items;

    QFileInfoList dirs = weatherDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QFileInfo& dirInfo : dirs) {
        ContentItem item = getItemInfo(dirInfo.absoluteFilePath());
        if (item.type == "weather") {
            items.append(item);
        }
    }

    return items;
}

// ============================================================================
// Content information
// ============================================================================

ContentBrowser::ContentItem ContentBrowser::getItemInfo(const QString& itemPath) {
    ContentItem item;
    item.path = itemPath;
    item.name = QFileInfo(itemPath).fileName();
    item.lastModified = QFileInfo(itemPath).lastModified();
    item.size = 0;

    QDirIterator it(itemPath, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        item.size += it.fileInfo().size();
    }

    // Determine type and parse info
    if (QFile::exists(itemPath + "/data/car.ini")) {
        item.type = "car";
        parseCarInfo(itemPath, item);
    } else if (QFile::exists(itemPath + "/data/surfaces.ini") ||
               QFile::exists(itemPath + "/ui/ui_track.json")) {
        item.type = "track";
        parseTrackInfo(itemPath, item);
    } else if (QFile::exists(itemPath + "/weather.ini")) {
        item.type = "weather";
        parseWeatherInfo(itemPath, item);
    }

    // Check for preview
    if (hasPreview(itemPath)) {
        item.previewPath = getPreviewPath(itemPath);
    }

    return item;
}

ContentBrowser::ContentStats ContentBrowser::getContentStats(const QString& contentPath) {
    ContentStats stats;

    QVector<ContentItem> cars = browseCars(contentPath + "/cars");
    QVector<ContentItem> tracks = browseTracks(contentPath + "/tracks");
    QVector<ContentItem> weather = browseWeather(contentPath + "/weather");

    stats.totalCars = cars.size();
    stats.totalTracks = tracks.size();
    stats.totalWeather = weather.size();

    for (const ContentItem& item : cars) {
        stats.totalSize += item.size;
        if (item.isMod) stats.totalMods++;
        else stats.totalStock++;
    }

    for (const ContentItem& item : tracks) {
        stats.totalSize += item.size;
        if (item.isMod) stats.totalMods++;
        else stats.totalStock++;
    }

    return stats;
}

// ============================================================================
// Preview generation
// ============================================================================

bool ContentBrowser::generatePreview(const QString& itemPath) {
    QString previewPath = itemPath;
    if (QDir(itemPath).exists()) {
        previewPath = itemPath + "/preview.png";
    }
    if (hasPreview(itemPath)) return true;

    QFileInfo fi(itemPath);
    QImage preview(256, 256, QImage::Format_RGB32);
    QColor bgColor = fi.isDir() ? QColor("#2d2d44") : QColor("#1a1a2e");
    preview.fill(bgColor);

    QPainter painter(&preview);
    painter.setPen(QColor("#8888aa"));
    QFont font = painter.font();
    font.setPointSize(10);
    painter.setFont(font);
    QString label = fi.completeSuffix().isEmpty() ? fi.fileName() : fi.completeSuffix().toUpper();
    painter.drawText(preview.rect(), Qt::AlignCenter, label);
    painter.end();

    QString outPath;
    if (QDir(itemPath).exists()) {
        outPath = itemPath + "/preview.png";
    } else {
        outPath = fi.path() + "/" + fi.baseName() + "_preview.png";
    }
    return preview.save(outPath);
}

bool ContentBrowser::hasPreview(const QString& itemPath) {
    QStringList previewNames;
    previewNames << "preview.jpg" << "preview.png" << "thumb.jpg" << "thumb.png";

    for (const QString& name : previewNames) {
        if (QFile::exists(itemPath + "/" + name)) {
            return true;
        }
    }

    return false;
}

QString ContentBrowser::getPreviewPath(const QString& itemPath) {
    QStringList previewNames;
    previewNames << "preview.jpg" << "preview.png" << "thumb.jpg" << "thumb.png";

    for (const QString& name : previewNames) {
        QString path = itemPath + "/" + name;
        if (QFile::exists(path)) {
            return path;
        }
    }

    return QString();
}

// ============================================================================
// Content validation
// ============================================================================

bool ContentBrowser::validateContent(const QString& itemPath, QString* error) {
    QFileInfo info(itemPath);

    if (!info.isDir()) {
        if (error) *error = "Path is not a directory";
        return false;
    }

    // Check for required files based on type
    if (QFile::exists(itemPath + "/data/car.ini")) {
        return validateCar(itemPath, error);
    } else if (QFile::exists(itemPath + "/data/surfaces.ini")) {
        return validateTrack(itemPath, error);
    }

    return true;
}

bool ContentBrowser::validateCar(const QString& carPath, QString* error) {
    // Check required files
    QStringList requiredFiles;
    requiredFiles << "data/car.ini" << "data/tyres.ini" << "data/engine.ini";

    for (const QString& file : requiredFiles) {
        if (!QFile::exists(carPath + "/" + file)) {
            if (error) *error = "Missing required file: " + file;
            return false;
        }
    }

    // Check for model
    if (!QFile::exists(carPath + "/models.kn5") &&
        !QFile::exists(carPath + "/body.kn5")) {
        if (error) *error = "No KN5 model found";
        return false;
    }

    return true;
}

bool ContentBrowser::validateTrack(const QString& trackPath, QString* error) {
    QStringList requiredFiles;
    requiredFiles << "data/surfaces.ini";

    bool hasMap = QFile::exists(trackPath + "/data/map.png") ||
                  QFile::exists(trackPath + "/data/map.jpg") ||
                  QFile::exists(trackPath + "/map.png");
    bool hasPreview = QFile::exists(trackPath + "/preview.png") ||
                      QFile::exists(trackPath + "/preview.jpg") ||
                      QFile::exists(trackPath + "/ui/preview.png");

    if (!hasMap) {
        if (error) *error = "Missing track map (data/map.png or map.png)";
        return false;
    }

    for (const QString& file : requiredFiles) {
        if (!QFile::exists(trackPath + "/" + file)) {
            if (error) *error = "Missing required file: " + file;
            return false;
        }
    }

    return true;
}

// ============================================================================
// Content management
// ============================================================================

bool ContentBrowser::installMod(const QString& modPath, const QString& contentPath) {
    QFileInfo modInfo(modPath);

    if (!modInfo.exists()) {
        return false;
    }

    // Determine mod type from path
    QString targetType;
    if (modPath.contains("/cars/") || modPath.endsWith(".car")) {
        targetType = "cars";
    } else if (modPath.contains("/tracks/") || modPath.endsWith(".track")) {
        targetType = "tracks";
    } else if (modPath.contains("/weather/")) {
        targetType = "weather";
    }

    if (targetType.isEmpty()) {
        return false;
    }

    // Extract mod name
    QString modName = modInfo.completeBaseName();
    QString destPath = contentPath + "/" + targetType + "/" + modName;

    // Copy files
    if (modInfo.isDir()) {
        return copyDirectoryRecursive(modPath, destPath);
    } else if (modPath.endsWith(".zip")) {
        QFile zipFile(modPath);
        if (!zipFile.open(QIODevice::ReadOnly)) {
            return false;
        }
        QZipReader reader(&zipFile);
        if (!reader.exists()) {
            return false;
        }
        QDir().mkpath(destPath);
        reader.extractAll(destPath);
        zipFile.close();
        return true;
    }

    return false;
}

bool ContentBrowser::uninstallMod(const QString& modName, const QString& contentPath) {
    // Find and remove the mod
    QStringList types;
    types << "cars" << "tracks" << "weather";

    for (const QString& type : types) {
        QString modPath = contentPath + "/" + type + "/" + modName;
        if (QDir(modPath).exists()) {
            return QDir(modPath).removeRecursively();
        }
    }

    return false;
}

bool ContentBrowser::updateMod(const QString& modName, const QString& newPath,
                                const QString& contentPath) {
    // Uninstall old version
    uninstallMod(modName, contentPath);

    // Install new version
    return installMod(newPath, contentPath);
}

// ============================================================================
// Content search
// ============================================================================

QVector<ContentBrowser::ContentItem> ContentBrowser::searchContent(
    const QString& contentPath, const QString& query, const QString& type) {

    QVector<ContentItem> items;

    ContentFilter filter;
    filter.searchQuery = query;
    filter.type = type;

    return browseContent(contentPath, filter);
}

// ============================================================================
// Content comparison
// ============================================================================

QMap<QString, QPair<QString, QString>> ContentBrowser::compareContent(
    const QString& path1, const QString& path2) {

    QMap<QString, QPair<QString, QString>> differences;

    ContentItem item1 = getItemInfo(path1);
    ContentItem item2 = getItemInfo(path2);

    if (item1.name != item2.name) {
        differences["name"] = qMakePair(item1.name, item2.name);
    }

    if (item1.author != item2.author) {
        differences["author"] = qMakePair(item1.author, item2.author);
    }

    if (item1.version != item2.version) {
        differences["version"] = qMakePair(item1.version, item2.version);
    }

    return differences;
}

// ============================================================================
// Utility
// ============================================================================

QString ContentBrowser::getContentTypeName(const QString& type) {
    static QMap<QString, QString> typeNames;
    if (typeNames.isEmpty()) {
        typeNames["car"] = "Car";
        typeNames["track"] = "Track";
        typeNames["weather"] = "Weather";
        typeNames["font"] = "Font";
        typeNames["showroom"] = "Showroom";
    }
    return typeNames.value(type, type);
}

QString ContentBrowser::getDrivetrainName(int drivetrain) {
    switch (drivetrain) {
        case 0: return "FWD";
        case 1: return "RWD";
        case 2: return "AWD";
        default: return "Unknown";
    }
}

QString ContentBrowser::getTransmissionName(int transmission) {
    switch (transmission) {
        case 0: return "Manual";
        case 1: return "Sequential";
        case 2: return "Automatic";
        default: return "Unknown";
    }
}

QStringList ContentBrowser::getContentTypes() {
    return QStringList() << "car" << "track" << "weather" << "font" << "showroom";
}

// ============================================================================
// Private helpers
// ============================================================================

bool ContentBrowser::parseCarInfo(const QString& carPath, ContentItem& item) {
    QString iniPath = carPath + "/data/car.ini";
    QFile file(iniPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    QString currentSection;

    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();

        if (line.startsWith('[') && line.endsWith(']')) {
            currentSection = line.mid(1, line.length() - 2);
            continue;
        }

        if (line.contains('=')) {
            int eqPos = line.indexOf('=');
            QString key = line.left(eqPos).trimmed();
            QString value = line.mid(eqPos + 1).trimmed();

            if (key == "NAME") item.name = value;
            else if (key == "DESCRIPTION") item.description = value;
            else if (key == "AUTHOR") item.author = value;
            else if (key == "VERSION") item.version = value;
            else if (key == "BRAND") item.manufacturer = value;
            else if (key == "YEAR") item.year = value.toInt();
            else if (key == "POWER") item.power = value.toInt();
            else if (key == "WEIGHT") item.weight = value.toFloat();
            else if (key == "DRIVETRAIN") item.drivetrain = value.toInt();
        }
    }

    file.close();
    return true;
}

bool ContentBrowser::parseTrackInfo(const QString& trackPath, ContentItem& item) {
    // Try ui_track.json first
    QString jsonPath = trackPath + "/ui/ui_track.json";
    QFile jsonFile(jsonPath);
    if (jsonFile.open(QIODevice::ReadOnly)) {
        QByteArray data = jsonFile.readAll();
        jsonFile.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            item.name = obj["name"].toString();
            item.description = obj["description"].toString();
            item.author = obj["author"].toString();
            item.version = obj["version"].toString();
            return true;
        }
    }

    // Fallback to surfaces.ini
    QString iniPath = trackPath + "/data/surfaces.ini";
    QFile iniFile(iniPath);
    if (iniFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&iniFile);
        while (!stream.atEnd()) {
            QString line = stream.readLine().trimmed();
            if (line.contains('=')) {
                int eqPos = line.indexOf('=');
                QString key = line.left(eqPos).trimmed();
                QString value = line.mid(eqPos + 1).trimmed();

                if (key == "NAME") {
                    item.name = value;
                    break;
                }
            }
        }
        iniFile.close();
    }

    return true;
}

bool ContentBrowser::parseWeatherInfo(const QString& weatherPath, ContentItem& item) {
    QString iniPath = weatherPath + "/weather.ini";
    QFile file(iniPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        if (line.contains('=')) {
            int eqPos = line.indexOf('=');
            QString key = line.left(eqPos).trimmed();
            QString value = line.mid(eqPos + 1).trimmed();

            if (key == "NAME") {
                item.name = value;
                break;
            }
        }
    }

    file.close();
    return true;
}

bool ContentBrowser::matchesFilter(const ContentItem& item, const ContentFilter& filter) {
    // Type filter
    if (!filter.type.isEmpty() && item.type != filter.type) {
        return false;
    }

    // Search query
    if (!filter.searchQuery.isEmpty()) {
        if (!item.name.contains(filter.searchQuery, Qt::CaseInsensitive) &&
            !item.description.contains(filter.searchQuery, Qt::CaseInsensitive) &&
            !item.author.contains(filter.searchQuery, Qt::CaseInsensitive)) {
            return false;
        }
    }

    // Author filter
    if (!filter.author.isEmpty() && item.author != filter.author) {
        return false;
    }

    // Rating filter
    if (filter.minRating > 0 && item.rating < filter.minRating) {
        return false;
    }

    // Installed filter
    if (filter.installedOnly && !item.isInstalled) {
        return false;
    }

    // Mods filter
    if (filter.modsOnly && !item.isMod) {
        return false;
    }

    // Stock filter
    if (filter.stockOnly && item.isMod) {
        return false;
    }

    // Year filter
    if (filter.yearMin > 0 && item.year < filter.yearMin) {
        return false;
    }
    if (filter.yearMax > 0 && item.year > filter.yearMax) {
        return false;
    }

    // Power filter
    if (filter.powerMin > 0 && item.power < filter.powerMin) {
        return false;
    }
    if (filter.powerMax > 0 && item.power > filter.powerMax) {
        return false;
    }

    // Weight filter
    if (filter.weightMin > 0 && item.weight < filter.weightMin) {
        return false;
    }
    if (filter.weightMax > 0 && item.weight > filter.weightMax) {
        return false;
    }

    // Drivetrain filter
    if (filter.drivetrain >= 0 && item.drivetrain != filter.drivetrain) {
        return false;
    }

    return true;
}

bool ContentBrowser::copyDirectoryRecursive(const QString& sourceDir, const QString& destDir) {
    QDir source(sourceDir);
    if (!source.exists()) return false;

    if (!QDir().mkpath(destDir)) return false;

    QFileInfoList entries = source.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
    for (const QFileInfo& entry : entries) {
        QString destPath = destDir + "/" + entry.fileName();
        if (entry.isDir()) {
            if (!copyDirectoryRecursive(entry.absoluteFilePath(), destPath)) {
                return false;
            }
        } else {
            if (!QFile::copy(entry.absoluteFilePath(), destPath)) {
                if (QFile::exists(destPath)) {
                    QFile::remove(destPath);
                    if (!QFile::copy(entry.absoluteFilePath(), destPath)) {
                        return false;
                    }
                } else {
                    return false;
                }
            }
        }
    }

    return true;
}
