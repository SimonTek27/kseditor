#include "SDKBackend.h"
#include "core/assets/Paths.h"
#include "plugins/simulators/kunos/KsPlugin.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QColor>
#include <QDebug>
#include <QSettings>
#include <QProcessEnvironment>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace ks {

SDKBackend* SDKBackend::s_instance = nullptr;

SDKBackend::SDKBackend(QObject* parent)
    : QObject(parent)
    , m_initialized(false)
{
}

SDKBackend::~SDKBackend()
{
    shutdown();
}

SDKBackend* SDKBackend::instance()
{
    if (!s_instance) {
        s_instance = new SDKBackend();
    }
    return s_instance;
}

bool SDKBackend::initialize()
{
    ks::plugins::kunos::KsPlugin* plugin = ks::plugins::kunos::KsPlugin::instance();
    if (!plugin->initialize()) {
        qWarning() << "Kunos plugin initialization failed";
        return false;
    }

    m_initialized = true;
    emit initialized(true);
    return true;
}

void SDKBackend::shutdown()
{
    m_initialized = false;
}

QString SDKBackend::getFolderPath(KsFolderID folder)
{
    ks::plugins::kunos::KsPlugin* plugin = ks::plugins::kunos::KsPlugin::instance();
    QString root = plugin->installPath();

    switch (folder) {
    case KsFolderID::Root:
        return root;
    case KsFolderID::ContentCars:
        return root + "/content/cars";
    case KsFolderID::ContentTracks:
        return root + "/content/tracks";
    case KsFolderID::ContentDrivers:
        return root + "/content/drivers";
    case KsFolderID::ContentFonts:
        return root + "/content/fonts";
    case KsFolderID::ExtRoot:
        return root + "/extension";
    case KsFolderID::ExtCfgSys:
        return root + "/extension/config";
    case KsFolderID::ExtTextures:
        return root + "/extension/textures";
    case KsFolderID::ExtLua:
        return root + "/extension/lua";
    case KsFolderID::ExtInternal:
        return root + "/extension/internal";
    case KsFolderID::Apps:
        return root + "/apps";
    case KsFolderID::AppsLua:
        return root + "/apps/lua";
    case KsFolderID::Cfg:
    {
        QDir docs(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        return docs.absolutePath() + "/ksEditor/cfg";
    }
    case KsFolderID::Logs:
    {
        QDir docs(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        return docs.absolutePath() + "/ksEditor/logs";
    }
    case KsFolderID::Screenshots:
    {
        QDir docs(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        return docs.absolutePath() + "/ksEditor/screens";
    }
    case KsFolderID::RaceResults:
    {
        QDir docs(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        return docs.absolutePath() + "/ksEditor/out";
    }
    case KsFolderID::UserSetups:
    {
        QDir docs(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        return docs.absolutePath() + "/ksEditor/setups";
    }
    case KsFolderID::PPFilters:
        return root + "/system/cfg/ppfilters";
    case KsFolderID::AppData:
        return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    case KsFolderID::AppDataLocal:
        return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    case KsFolderID::AppDataTemp:
        return QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    default:
        return root;
    }
}

QString SDKBackend::getContentPath(KsFolderID folderId)
{
    return getFolderPath(folderId);
}

QString SDKBackend::getCarModelPath(const QString& carId)
{
    QStringList searchPatterns = {
        carId + ".kn5",
        carId + "_lod_a.kn5",
        carId + "_lod_b.kn5",
        carId + "_lod_c.kn5"
    };

    QString carPath = getFolderPath(KsFolderID::ContentCars) + "/" + carId;
    QDir dir(carPath);
    if (!dir.exists()) return QString();

    for (const QString& pattern : searchPatterns) {
        QStringList files = dir.entryList(QStringList(pattern), QDir::Files);
        if (!files.isEmpty()) {
            return carPath + "/" + files.first();
        }
    }

    return QString();
}

QString SDKBackend::getTrackPath(const QString& trackId)
{
    return getFolderPath(KsFolderID::ContentTracks) + "/" + trackId;
}

QString SDKBackend::getCarDataPath(const QString& carId)
{
    return getFolderPath(KsFolderID::ContentCars) + "/" + carId + "/data";
}

QString SDKBackend::getTrackDataPath(const QString& trackId, const QString& layout)
{
    if (layout.isEmpty()) {
        return getFolderPath(KsFolderID::ContentTracks) + "/" + trackId + "/data";
    }
    return getFolderPath(KsFolderID::ContentTracks) + "/" + trackId + "/" + layout + "/data";
}

QStringList SDKBackend::getCarList()
{
    QString carsPath = getFolderPath(KsFolderID::ContentCars);
    QDir dir(carsPath);
    if (!dir.exists()) return QStringList();
    return dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
}

QStringList SDKBackend::getTrackList()
{
    QString tracksPath = getFolderPath(KsFolderID::ContentTracks);
    QDir dir(tracksPath);
    if (!dir.exists()) return QStringList();
    return dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
}

QStringList SDKBackend::getCarDataFiles(const QString& carId)
{
    QStringList dataFiles;
    QString carPath = getCarDataPath(carId);

    QDir dir(carPath);
    if (dir.exists()) {
        dataFiles = dir.entryList(QStringList() << "*.ini" << "*.json", QDir::Files);
    }

    return dataFiles;
}

QStringList SDKBackend::getTrackDataFiles(const QString& trackId)
{
    QStringList dataFiles;
    QString trackPath = getTrackPath(trackId);

    QDir dir(trackPath);
    if (dir.exists()) {
        dataFiles = dir.entryList(QStringList() << "*.ini" << "*.json", QDir::Files);

        QStringList layouts = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& layout : layouts) {
            if (layout == "ui" || layout == "data") continue;
            QString layoutDataPath = trackPath + "/" + layout + "/data";
            QDir layoutDir(layoutDataPath);
            if (layoutDir.exists()) {
                dataFiles.append(layoutDir.entryList(QStringList() << "*.ini" << "*.json", QDir::Files));
            }
        }
    }

    return dataFiles;
}

bool SDKBackend::loadCarSpec(const QString& carId, KsCarSpec& spec)
{
    QString carIniPath = getCarDataPath(carId) + "/car.ini";
    QFile file(carIniPath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open car.ini for:" << carId;
        return false;
    }

    spec = KsCarSpec();
    QTextStream in(&file);
    QString currentSection;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        if (line.isEmpty() || line.startsWith(";")) continue;

        if (line.startsWith("[") && line.endsWith("]")) {
            currentSection = line.mid(1, line.length() - 2);
            continue;
        }

        int eqPos = line.indexOf("=");
        if (eqPos <= 0) continue;

        QString key = line.left(eqPos).trimmed();
        QString value = line.mid(eqPos + 1).trimmed();

        if (currentSection == "INFO") {
            if (key == "SCREEN_NAME") spec.screenName = value;
            else if (key == "SHORT_NAME") spec.shortName = value;
        }
        else if (currentSection == "BASIC") {
            if (key == "TOTALMASS") spec.totalMass = value.toFloat();
            else if (key == "INERTIA") {
                QStringList parts = value.split(",");
                if (parts.size() >= 3) {
                    spec.inertia[0] = parts[0].toFloat();
                    spec.inertia[1] = parts[1].toFloat();
                    spec.inertia[2] = parts[2].toFloat();
                }
            }
            else if (key == "GRAPHICS_OFFSET") {
                QStringList parts = value.split(",");
                if (parts.size() >= 3) {
                    spec.graphicsOffset[0] = parts[0].toFloat();
                    spec.graphicsOffset[1] = parts[1].toFloat();
                    spec.graphicsOffset[2] = parts[2].toFloat();
                }
            }
            else if (key == "GRAPHICS_PITCH_ROTATION") {
                spec.graphicsPitchRotation = value.toFloat();
            }
        }
        else if (currentSection == "CONTROLS") {
            if (key == "STEER_LOCK") spec.steerLock = value.toFloat();
            else if (key == "STEER_RATIO") spec.steerRatio = value.toFloat();
            else if (key == "FFMULT") spec.ffMult = value.toFloat();
        }
    }

    file.close();
    return !spec.screenName.isEmpty();
}

bool SDKBackend::loadTrackSpec(const QString& trackId, KsTrackSpec& spec)
{
    spec = KsTrackSpec();
    spec.trackId = trackId;

    QString trackUiPath = getTrackPath(trackId) + "/ui/ui_track.json";
    QFile file(trackUiPath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QDir trackDir(getTrackPath(trackId));
        QStringList kn5Files = trackDir.entryList(QStringList() << "*.kn5");
        if (!kn5Files.isEmpty()) {
            spec.name = trackId;
            return true;
        }
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        spec.name = obj.value("name").toString(trackId);
        spec.country = obj.value("country").toString("");
        spec.length = obj.value("length").toDouble(0.0);
        spec.width = obj.value("width").toDouble(10.0);
        spec.pitCount = obj.value("pits").toInt(0);
    }

    return true;
}

bool SDKBackend::loadTrackLayouts(const QString& trackId, QList<KsTrackLayout>& layouts)
{
    layouts.clear();
    QString trackPath = getTrackPath(trackId);
    QDir dir(trackPath);

    if (!dir.exists()) return false;

    QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& subdir : subdirs) {
        if (subdir == "ui" || subdir == "data") continue;

        KsTrackLayout layout;
        layout.id = subdir;
        layout.name = subdir;
        layout.pits = 0;
        layout.config = subdir;

        QString dataPath = trackPath + "/" + subdir + "/data/track.ini";
        QFile trackIni(dataPath);
        if (trackIni.open(QIODevice::ReadOnly)) {
            QTextStream in(&trackIni);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (line.startsWith("LENGTH=")) layout.length = line.mid(7).toFloat();
                else if (line.startsWith("WIDTH=")) layout.width = line.mid(6).toFloat();
                else if (line.startsWith("PITS=")) layout.pits = line.mid(5).toInt();
            }
            trackIni.close();
        }

        layouts.append(layout);
    }

    return true;
}

bool SDKBackend::loadCarModelFiles(const QString& carId, QStringList& files)
{
    files.clear();
    QString carPath = getFolderPath(KsFolderID::ContentCars) + "/" + carId;
    QDir dir(carPath);

    if (!dir.exists()) return false;

    files = dir.entryList(QStringList() << "*.kn5" << "*.fbx", QDir::Files);
    return !files.isEmpty();
}

bool SDKBackend::loadTrackModelFiles(const QString& trackId, QStringList& files)
{
    files.clear();
    QString trackPath = getTrackPath(trackId);
    QDir dir(trackPath);

    if (!dir.exists()) return false;

    files = dir.entryList(QStringList() << "*.kn5", QDir::Files);
    return !files.isEmpty();
}

bool SDKBackend::loadCarTyres(const QString& carId, QList<KsTyreSpec>& tyres)
{
    tyres.clear();
    QString tyreIniPath = getCarDataPath(carId) + "/tyres.ini";

    QFile file(tyreIniPath);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QTextStream in(&file);
    bool inTyre = false;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        
        if (line.startsWith("[") && line.endsWith("]")) {
            inTyre = true;
            KsTyreSpec tyre;
            tyres.append(tyre);
        }
        else if (inTyre && !tyres.isEmpty()) {
            KsTyreSpec& t = tyres.last();
            if (line.startsWith("RADIUS=")) t.radius = line.mid(7).toFloat();
            else if (line.startsWith("WIDTH=")) t.width = line.mid(6).toFloat();
            else if (line.startsWith("PROFILE=")) t.height = line.mid(8).toFloat();
            else if (line.startsWith("RIM=")) t.rim = line.mid(4).toFloat();
            else if (line.startsWith("COMPOUND=")) t.compound = line.mid(9).trimmed();
            else if (line.startsWith("PRESSURE=")) t.pressure = line.mid(9).toFloat();
            else if (line.startsWith("TEMPERATURE=")) t.temperature = line.mid(12).toFloat();
        }
    }

    file.close();
    return !tyres.isEmpty();
}

bool SDKBackend::loadCarEngine(const QString& carId, KsEngineSpec& engine)
{
    engine = KsEngineSpec();

    QString engineIniPath = getCarDataPath(carId) + "/engine.ini";
    QFile file(engineIniPath);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QTextStream in(&file);
    bool inEngine = false;
    int gearIndex = 0;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        if (line == "[ENGINE]" || line == "[HEADER]" || line == "[DATA]") {
            inEngine = true;
            continue;
        }
        if (line.startsWith("[") && line.endsWith("]")) {
            inEngine = false;
            continue;
        }

        if (!inEngine) continue;

        if (line.startsWith("MAX_POWER=")) engine.maxPower = line.mid(10).toFloat();
        else if (line.startsWith("MAX_TORQUE=")) engine.maxTorque = line.mid(11).toFloat();
        else if (line.startsWith("LIMITER=")) engine.redline = line.mid(8).toFloat();
        else if (line.startsWith("MAX_RPM=")) engine.maxRpm = line.mid(8).toFloat();
        else if (line.startsWith("MIN_RPM=")) engine.minRpm = line.mid(8).toFloat();
        else if (line.startsWith("GEARS=")) engine.gears = line.mid(6).toInt();
        else if (line.startsWith("FINAL=")) engine.finalDrive = line.mid(6).toFloat();
        else if (line.startsWith("GEAR_") && gearIndex < 8) {
            QStringList parts = line.split('=');
            if (parts.size() == 2) {
                engine.gearRatios[gearIndex++] = parts[1].toFloat();
            }
        }
    }

    file.close();
    return true;
}

float SDKBackend::calculateDownforce(float speedMs, float aoa, float cl)
{
    const float rho = 1.225f;
    return 0.5f * rho * speedMs * speedMs * aoa * cl;
}

float SDKBackend::calculateDrag(float speedMs, float cd, float area)
{
    const float rho = 1.225f;
    return 0.5f * rho * speedMs * speedMs * cd * area;
}

float SDKBackend::calculateCornerG(float speedMs, float radius)
{
    if (radius <= 0.001f) return 0;
    return (speedMs * speedMs) / (radius * 9.81f);
}

float SDKBackend::calculateBrakeDistance(float speedMs, float deceleration)
{
    if (deceleration <= 0.001f) return 1e9f;
    return (speedMs * speedMs) / (2.0f * deceleration);
}

float SDKBackend::calculateStoppingDistance(float speedMs, float reactionTime)
{
    return speedMs * reactionTime;
}

QMap<QString, QString> SDKBackend::getCarBrands()
{
    QMap<QString, QString> brands;
    brands["ferrari"] = "Ferrari";
    brands["lamborghini"] = "Lamborghini";
    brands["porsche"] = "Porsche";
    brands["audi"] = "Audi";
    brands["bmw"] = "BMW";
    brands["mercedes"] = "Mercedes-Benz";
    brands["mclaren"] = "McLaren";
    brands["ford"] = "Ford";
    brands["chevrolet"] = "Chevrolet";
    brands["nissan"] = "Nissan";
    brands["toyota"] = "Toyota";
    brands["honda"] = "Honda";
    brands["koenigsegg"] = "Koenigsegg";
    brands["mazda"] = "Mazda";
    brands["subaru"] = "Subaru";
    brands["mitsubishi"] = "Mitsubishi";
    brands["aston_martin"] = "Aston Martin";
    brands["jaguar"] = "Jaguar";
    brands["lotus"] = "Lotus";
    return brands;
}

QMap<QString, QString> SDKBackend::getTrackCountries()
{
    QMap<QString, QString> countries;
    countries["italy"] = "Italy";
    countries["germany"] = "Germany";
    countries["uk"] = "United Kingdom";
    countries["usa"] = "United States";
    countries["france"] = "France";
    countries["spain"] = "Spain";
    countries["japan"] = "Japan";
    countries["australia"] = "Australia";
    countries["belgium"] = "Belgium";
    countries["monaco"] = "Monaco";
    countries["netherlands"] = "Netherlands";
    countries["brazil"] = "Brazil";
    countries["singapore"] = "Singapore";
    countries["abu dhabi"] = "Abu Dhabi";
    countries["canada"] = "Canada";
    countries["china"] = "China";
    countries["russia"] = "Russia";
    countries["south_korea"] = "South Korea";
    countries["mexico"] = "Mexico";
    countries["austria"] = "Austria";
    countries["hungary"] = "Hungary";
    countries["portugal"] = "Portugal";
    return countries;
}

static bool copyDirectoryContents(const QString& sourceDir, const QString& destDir, const QStringList& filters)
{
    QDir().mkpath(destDir);
    QDir src(sourceDir);
    QFileInfoList entries = src.entryInfoList(filters, QDir::Files | QDir::NoDotAndDotDot | QDir::AllDirs, QDir::Name);
    for (const auto& fi : entries) {
        QString destPath = destDir + "/" + fi.fileName();
        if (fi.isDir()) {
            if (!copyDirectoryContents(fi.absoluteFilePath(), destPath, filters))
                return false;
        } else {
            if (!QFile::copy(fi.absoluteFilePath(), destPath))
                return false;
        }
    }
    return true;
}

bool SDKBackend::exportCar(const QString& carId, const QString& outputPath)
{
    QString carPath = getFolderPath(KsFolderID::ContentCars) + "/" + carId;
    if (!QDir(carPath).exists()) return false;

    QDir().mkpath(outputPath);
    QStringList filters;
    filters << "*.ini" << "*.kn5" << "*.dds" << "*.png" << "*.lua" << "*.json";
    return copyDirectoryContents(carPath, outputPath, filters);
}

bool SDKBackend::exportTrack(const QString& trackId, const QString& outputPath)
{
    QString trackPath = getFolderPath(KsFolderID::ContentTracks) + "/" + trackId;
    if (!QDir(trackPath).exists()) return false;

    QDir().mkpath(outputPath);
    QStringList filters;
    filters << "*.ini" << "*.kn5" << "*.dds" << "*.png" << "*.lua" << "*.json" << "*.obj" << "*.fbx";
    return copyDirectoryContents(trackPath, outputPath, filters);
}

} // namespace ks