#include "ACEQmlBridge.h"
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QBuffer>

namespace ks {

// ─── ACEContentQmlBridge ────────────────────────────────────────────────────

ACEContentQmlBridge::ACEContentQmlBridge(QObject* parent)
    : QObject(parent)
    , m_finder(new ACEContentFinder(this)) {
    connect(m_finder, &ACEContentFinder::contentChanged, this, &ACEContentQmlBridge::contentChanged);
}

bool ACEContentQmlBridge::isAceInstalled() const {
    return m_finder->isAceInstalled();
}

QString ACEContentQmlBridge::getAceRoot() const {
    return m_finder->findAceRoot();
}

QStringList ACEContentQmlBridge::getCars() const {
    return m_finder->findCars();
}

QStringList ACEContentQmlBridge::getTracks() const {
    return m_finder->findTracks();
}

QStringList ACEContentQmlBridge::getMods() const {
    return m_finder->findMods();
}

void ACEContentQmlBridge::setCustomPath(const QString& path) {
    m_finder->setCustomPath(path);
}

void ACEContentQmlBridge::refresh() {
    emit contentChanged();
}

QString ACEContentQmlBridge::getContentFolder() const {
    QString root = getAceRoot();
    if (root.isEmpty()) return QString();
    return ACEPaths::getContentDirectory(root);
}

QString ACEContentQmlBridge::getModsFolder() const {
    return ACEPaths::findModDirectory();
}

QStringList ACEContentQmlBridge::getSkins(const QString& carName) const {
    if (carName.isEmpty()) return QStringList();
    QString root = getAceRoot();
    if (root.isEmpty()) return QStringList();
    QString carPath = ACEPaths::getCarsDirectory(root) + "/" + carName;
    return ACEPaths::getSkinNames(carPath);
}

QVariantMap ACEContentQmlBridge::getCarInfo(const QString& carName) const {
    if (carName.isEmpty()) return QVariantMap();
    QString root = getAceRoot();
    if (root.isEmpty()) return QVariantMap();
    QString carPath = ACEPaths::getCarsDirectory(root) + "/" + carName;
    ACECarInfo info = ACEContentReader::readCarInfo(carPath);
    return info.toJson().toVariantMap();
}

QStringList ACEContentQmlBridge::getModContents(const QString& modFileName) const {
    if (modFileName.isEmpty()) return QStringList();
    QString modsDir = ACEPaths::findModDirectory();
    if (modsDir.isEmpty()) return QStringList();
    QString modPath = modsDir + "/" + modFileName;
    return ACEPackageExtractor::listContents(modPath);
}

// ─── ACEPackageQmlBridge ────────────────────────────────────────────────────

ACEPackageQmlBridge::ACEPackageQmlBridge(QObject* parent)
    : QObject(parent)
    , m_parser(new ACEPackageParser()) {
}

bool ACEPackageQmlBridge::openPackage(const QString& filePath) {
    if (m_parser->isOpen()) m_parser->close();

    if (!m_parser->open(filePath)) {
        emit error(QString("Cannot open package: %1").arg(filePath));
        return false;
    }

    if (!m_parser->readManifest()) {
        emit error(QString("Cannot read manifest: %1").arg(filePath));
        m_parser->close();
        return false;
    }

    emit packageOpened(filePath);
    return true;
}

void ACEPackageQmlBridge::closePackage() {
    m_parser->close();
    emit packageClosed();
}

bool ACEPackageQmlBridge::isOpen() const {
    return m_parser->isOpen();
}

QVariantMap ACEPackageQmlBridge::getManifest() const {
    if (!m_parser->isOpen()) return QVariantMap();
    return m_parser->manifest().toJson().toVariantMap();
}

QStringList ACEPackageQmlBridge::getEntryNames() const {
    if (!m_parser->isOpen()) return QStringList();
    return m_parser->entryNames();
}

int ACEPackageQmlBridge::getEntryCount() const {
    if (!m_parser->isOpen()) return 0;
    return m_parser->entryCount();
}

bool ACEPackageQmlBridge::extractFile(const QString& entryName, const QString& outputPath) {
    if (!m_parser->isOpen()) {
        emit error("No package open");
        return false;
    }
    return ACEPackageExtractor::extractFile(m_parser->manifest().packagePath, entryName, outputPath);
}

bool ACEPackageQmlBridge::extractAll(const QString& outputDir) {
    if (!m_parser->isOpen()) {
        emit error("No package open");
        return false;
    }
    return ACEPackageExtractor::extractAll(m_parser->manifest().packagePath, outputDir);
}

QVariantMap ACEPackageQmlBridge::getEntryInfo(const QString& entryName) const {
    if (!m_parser->isOpen()) return QVariantMap();
    int idx = m_parser->findEntry(entryName);
    if (idx < 0) return QVariantMap();
    return m_parser->manifest().entries[idx].toJson().toVariantMap();
}

QVariantList ACEPackageQmlBridge::getFilesByExtension(const QString& ext) const {
    if (!m_parser->isOpen()) return QVariantList();
    QVariantList result;
    for (const auto& entry : m_parser->manifest().entries) {
        if (!entry.isDirectory() && entry.path.endsWith(ext, Qt::CaseInsensitive)) {
            result.append(entry.path);
        }
    }
    return result;
}

qint64 ACEPackageQmlBridge::getEntrySize(const QString& entryName) const {
    if (!m_parser->isOpen()) return 0;
    int idx = m_parser->findEntry(entryName);
    if (idx < 0) return 0;
    return m_parser->manifest().entries[idx].size;
}

// ─── ACEProtobufQmlBridgeEvo ───────────────────────────────────────────────────

QStringList ACEProtobufQmlBridgeEvo::s_knownProtoFiles = {
    "CarData.proto",
    "CarBehaviour.proto",
    "CarSetup.proto",
    "CarSetupLimits.proto",
    "CarTuning.proto",
    "TyresData.proto",
    "PhysicsSnapshot.proto",
    "AudioData.proto",
    "Scene.proto",
    "TrackData.proto",
    "Weather.proto",
    "Renderer.proto",
    "CameraProperties.proto",
    "PostProcessing.proto",
    "Options.proto",
    "Customization.proto",
    "Gameplay.proto",
    "GameplaySettings.proto",
    "AiCarData.proto",
    "AIData.proto",
    "NetcodeData.proto",
    "InputConfiguration.proto",
    "CarCustomizationCommands.proto",
    "CarSelectionClientCommands.proto",
    "CarSetupCommands.proto",
    "GameClientCommands.proto",
    "GameModeCommands.proto",
    "Leaderboards.proto",
    "PenaltySystem.proto",
    "DateTime.proto",
    "DriverManager.proto",
    "TrafficProperties.proto",
    "Mirrors.proto",
    "GraphicsSettingsOverride.proto",
    "ThumbnailSettings.proto",
    "RuntimeStats.proto",
    "CustomTable.proto",
    "GamePlatformData.proto",
    "GamePrintables.proto",
    "BackendMessage.proto",
    "BackendMessages.proto",
};

QVariantMap ACEProtobufQmlBridgeEvo::s_fieldNames = {
    {"slip", "Tyre slip ratio"},
    {"lock", "Tyre lock state"},
    {"tyre_pressure", "Tyre pressure (PSI)"},
    {"tyre_temperature_c", "Tyre temperature (Celsius)"},
    {"brake_temperature_c", "Brake temperature (Celsius)"},
    {"brake_pressure", "Brake pressure"},
    {"tyre_compound_front", "Front tyre compound"},
    {"tyre_compound_rear", "Rear tyre compound"},
    {"engine_type", "Engine type"},
    {"has_kers", "Has KERS/ERS"},
    {"max_gears", "Maximum gears"},
    {"downforce_controllers", "Downforce controllers"},
    {"front_lift", "Front lift coefficient"},
    {"aero", "Aerodynamic settings"},
    {"DRSData", "DRS configuration"},
    {"width", "Tyre width (m)"},
    {"radius", "Tyre radius (m)"},
    {"RATE", "Spring rate"},
    {"DAMP", "Damping"},
    {"MASS", "Mass (kg)"},
    {"DY0", "Lateral friction coefficient"},
    {"DX0", "Longitudinal friction coefficient"},
};

ACEProtobufQmlBridgeEvo::ACEProtobufQmlBridgeEvo(QObject* parent)
    : QObject(parent) {
}

QVariantMap ACEProtobufQmlBridgeEvo::decodeMessage(const QByteArray& data) {
    return ProtobufDecoder::decodeMessage(data);
}

QString ACEProtobufQmlBridgeEvo::printMessage(const QVariantMap& message) {
    return ProtobufDecoder::printMessage(message);
}

QStringList ACEProtobufQmlBridgeEvo::extractStrings(const QByteArray& data, int minLength) {
    return ProtobufDecoder::extractStrings(data, minLength);
}

QVariantMap ACEProtobufQmlBridgeEvo::decodeFileContent(const QString& packagePath, const QString& entryPath) {
    ACEPackageParser parser;
    if (!parser.open(packagePath)) return QVariantMap();
    if (!parser.readManifest()) return QVariantMap();

    QByteArray data = parser.readFile(entryPath);
    if (data.isEmpty()) return QVariantMap();

    return ProtobufDecoder::decodeMessage(data);
}

QVariantMap ACEProtobufQmlBridgeEvo::decodeHexString(const QString& hexString) {
    QByteArray data = QByteArray::fromHex(hexString.toUtf8());
    if (data.isEmpty()) return QVariantMap();
    return ProtobufDecoder::decodeMessage(data);
}

QStringList ACEProtobufQmlBridgeEvo::getKnownProtoFiles() const {
    return s_knownProtoFiles;
}

QVariantMap ACEProtobufQmlBridgeEvo::getFieldNames() const {
    return s_fieldNames;
}

} // namespace ks
