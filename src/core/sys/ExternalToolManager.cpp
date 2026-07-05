#include "ExternalToolManager.h"
#include "core/archive/SevenZipLibrary.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QStandardPaths>

namespace ks { namespace tools {

QStringList ExternalPluginManager::getBuiltinPluginIds() {
    return QStringList() << "7Zip" << "FbxConverter" << "Fmod" << "ImageMagick" << "VLC" << "Bc7Tool" << "CefSharp" << "NativeLibs";
}

ExternalPluginManager::ExternalPluginManager(QObject* parent)
    : QObject(parent) {
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QString pluginsPath = appData + "/AcTools Content Manager/Plugins";
    scanPluginsDirectory(pluginsPath);
}

ExternalPluginManager::~ExternalPluginManager() {
}

ExternalPluginManager* ExternalPluginManager::instance() {
    static ExternalPluginManager inst;
    return &inst;
}

void ExternalPluginManager::scanPluginsDirectory(const QString& path) {
    m_pluginsPath = path;

    if (!QDir(path).exists()) {
        return;
    }

    QDir dir(path);
    for (const QFileInfo& info : dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QString pluginPath = info.filePath();
        loadManifest(pluginPath);
    }

    qDebug() << "Scanned plugins from:" << path << "found:" << m_plugins.size();
}

void ExternalPluginManager::loadManifest(const QString& pluginPath) {
    QFile manifestFile(pluginPath + "/Manifest.json");
    if (!manifestFile.open(QIODevice::ReadOnly)) {
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(manifestFile.readAll());
    if (doc.isNull() || !doc.object().contains("id")) {
        return;
    }

    QJsonObject obj = doc.object();

    ExternalPluginInfo plugin;
    plugin.id = obj.value("id").toString();
    plugin.name = obj.value("name").toString();
    plugin.description = obj.value("description").toString();
    plugin.version = obj.value("version").toString();
    plugin.appVersion = obj.value("appVersion").toString();
    plugin.size = obj.value("size").toInt();
    plugin.hidden = obj.value("hidden").toBool();
    plugin.recommended = obj.value("recommended").toBool(true);
    plugin.installed = obj.value("installedVersion").toString().isEmpty() == false;
    plugin.sourcePath = pluginPath;
    plugin.executablePath = findPluginExecutable(plugin.id);

    m_plugins[plugin.id] = plugin;
    emit pluginDiscovered(plugin);
}

QStringList ExternalPluginManager::getAvailablePlugins() const {
    return m_plugins.keys();
}

ExternalPluginInfo* ExternalPluginManager::getPlugin(const QString& id) {
    if (m_plugins.contains(id)) {
        return &m_plugins[id];
    }
    return nullptr;
}

bool ExternalPluginManager::isPluginAvailable(const QString& id) const {
    return m_plugins.contains(id);
}

bool ExternalPluginManager::isPluginInstalled(const QString& id) const {
    if (m_plugins.contains(id)) {
        ExternalPluginInfo info = m_plugins.value(id);
        return info.installed && !info.executablePath.isEmpty();
    }
    return false;
}

QString ExternalPluginManager::getPluginExecutable(const QString& id) const {
    if (m_plugins.contains(id)) {
        return m_plugins.value(id).executablePath;
    }
    return QString();
}

QString ExternalPluginManager::findPluginExecutable(const QString& id) {
    if (m_plugins.contains(id)) {
        ExternalPluginInfo info = m_plugins.value(id);
        QString basePath = info.sourcePath;

        QStringList exts = {".exe", ""};
        QStringList searchNames = {
            id,
            id + ".exe",
            id + "Tool",
            id + "Tool.exe"
        };

        for (const QString& searchName : searchNames) {
            for (const QString& ext : exts) {
                QString fullPath = basePath + "/" + searchName + ext;
                if (QFile::exists(fullPath)) {
                    return fullPath;
                }
            }
        }

        return basePath + "/" + id + ".exe";
    }
    return QString();
}

bool ExternalPluginManager::runPlugin(const QString& id, const QStringList& arguments) {
    QString exePath = findPluginExecutable(id);
    if (exePath.isEmpty() || !QFile::exists(exePath)) {
        emit error("Plugin not found: " + id);
        return false;
    }

    QProcess process;
    process.setProgram(exePath);
    process.setArguments(arguments);
    process.setProcessChannelMode(QProcess::MergedChannels);

    process.start();
    if (!process.waitForFinished(60000)) {
        process.kill();
        emit error("Plugin timeout: " + id);
        return false;
    }

    return process.exitCode() == 0;
}

ExternalToolBase::ExternalToolBase(const QString& toolId, QObject* parent)
    : QObject(parent), m_toolId(toolId) {
    m_executablePath = ExternalPluginManager::instance()->findPluginExecutable(toolId);
}

ExternalToolBase::~ExternalToolBase() {
}

bool ExternalToolBase::isAvailable() const {
    return !m_executablePath.isEmpty() && QFile::exists(m_executablePath);
}

QString ExternalToolBase::executablePath() const {
    return m_executablePath;
}

QString ExternalToolBase::runProcess(const QStringList& arguments, const QString& workingDir) {
    if (!isAvailable()) {
        return "{}";
    }

    QProcess process;
    process.setProgram(m_executablePath);
    process.setArguments(arguments);

    if (!workingDir.isEmpty()) {
        process.setWorkingDirectory(workingDir);
    }

    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start();

    if (!process.waitForFinished(60000)) {
        process.kill();
        return "{\"error\":\"timeout\"}";
    }

    m_lastExitCode = process.exitCode();
    return QString::fromUtf8(process.readAll());
}

QJsonObject FbxConverterTool::convert(const QString& inputFbx, const QString& outputKn5, bool generateLods) {
    QStringList args;
    args << "-i" << inputFbx << "-o" << outputKn5;
    if (generateLods) {
        args << "-lod";
    }

    QString output = runProcess(args);
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
    return doc.object();
}

QJsonObject FbxConverterTool::generateLods(const QString& inputKn5, const QString& outputDir, int lodCount) {
    QStringList args;
    args << "-lod" << "-i" << inputKn5 << "-o" << outputDir << "-count" << QString::number(lodCount);

    QString output = runProcess(args);
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
    return doc.object();
}

QJsonObject SevenZipTool::extract(const QString& archivePath, const QString& outputDir) {
    if (!isAvailable()) {
        // Fall back to direct library if external tool not found
        return ks::archive::SevenZipLibrary::instance()->extract(archivePath, outputDir);
    }
    QStringList args;
    args << "x" << archivePath << "-o" << outputDir << "-y";

    QString output = runProcess(args);
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
    return doc.object();
}

QJsonObject SevenZipTool::compress(const QStringList& files, const QString& outputArchive) {
    if (!isAvailable()) {
        return ks::archive::SevenZipLibrary::instance()->compress(files, outputArchive);
    }
    QStringList args = files;
    args << "-t7z" << outputArchive << "-mx9";

    QString output = runProcess(args);
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
    return doc.object();
}

QJsonObject SevenZipTool::listContents(const QString& archivePath) {
    if (!isAvailable()) {
        return ks::archive::SevenZipLibrary::instance()->listContents(archivePath);
    }
    QStringList args;
    args << "l" << archivePath;

    QString output = runProcess(args);
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
    return doc.object();
}

QJsonObject AudioTool::extractBanks(const QString& audioProjectPath, const QString& outputDir) {
    QStringList args;
    args << "-bank" << audioProjectPath << "-output" << outputDir;

    QString output = runProcess(args);
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
    return doc.object();
}

QJsonObject AudioTool::buildBanks(const QString& inputDir, const QString& outputBank) {
    QStringList args;
    args << "-build" << inputDir << "-output" << outputBank;

    QString output = runProcess(args);
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
    return doc.object();
}

QJsonObject ImageMagickTool::convertImage(const QString& input, const QString& output, const QStringList& options) {
    QStringList args = options;
    args << input << output;

    QString outputStr = runProcess(args);
    QJsonDocument doc = QJsonDocument::fromJson(outputStr.toUtf8());
    return doc.object();
}

QJsonObject ImageMagickTool::createThumbnail(const QString& input, const QString& output, int size) {
    QStringList args;
    args << input << "-resize" << QString::number(size) + "x" << output;

    QString outputStr = runProcess(args);
    QJsonDocument doc = QJsonDocument::fromJson(outputStr.toUtf8());
    return doc.object();
}

QJsonObject ImageMagickTool::generateMipmaps(const QString& input, const QString& outputDir) {
    QStringList args;
    args << input << "-mipmap" << outputDir;

    QString outputStr = runProcess(args);
    QJsonDocument doc = QJsonDocument::fromJson(outputStr.toUtf8());
    return doc.object();
}

QJsonObject Bc7Tool::compressDds(const QString& input, const QString& output, bool highQuality) {
    QStringList args;
    args << "-i" << input << "-o" << output;
    if (highQuality) {
        args << "-quality";
    }

    QString outputStr = runProcess(args);
    QJsonDocument doc = QJsonDocument::fromJson(outputStr.toUtf8());
    return doc.object();
}

QJsonObject Bc7Tool::compressPng(const QString& input, const QString& output) {
    QStringList args;
    args << "-png" << "-i" << input << "-o" << output;

    QString outputStr = runProcess(args);
    QJsonDocument doc = QJsonDocument::fromJson(outputStr.toUtf8());
    return doc.object();
}

} } // namespace ks::tools
