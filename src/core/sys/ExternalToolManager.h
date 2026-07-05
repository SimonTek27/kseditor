#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QObject>
#include <QProcess>
#include <QDir>
#include <QJsonObject>

namespace ks { namespace tools {

struct ExternalPluginInfo {
    QString id;
    QString name;
    QString description;
    QString version;
    QString appVersion;
    qint64 size = 0;
    bool hidden = false;
    bool recommended = true;
    bool installed = false;
    QString executablePath;
    QString sourcePath;
};

class ExternalPluginManager : public QObject {
    Q_OBJECT

public:
    static ExternalPluginManager* instance();

    explicit ExternalPluginManager(QObject* parent = nullptr);
    ~ExternalPluginManager();

    void scanPluginsDirectory(const QString& path);

    QStringList getAvailablePlugins() const;
    ExternalPluginInfo* getPlugin(const QString& id);
    bool isPluginAvailable(const QString& id) const;
    bool isPluginInstalled(const QString& id) const;

    bool runPlugin(const QString& id, const QStringList& arguments);
    QString getPluginExecutable(const QString& id) const;

    QString findPluginExecutable(const QString& id);

    static QStringList getBuiltinPluginIds();

signals:
    void pluginDiscovered(const ExternalPluginInfo& plugin);
    void pluginInstalled(const QString& id);
    void error(const QString& message);

private:
    void loadManifest(const QString& pluginPath);

    QMap<QString, ExternalPluginInfo> m_plugins;
    QString m_pluginsPath;
};

class ExternalToolBase : public QObject {
    Q_OBJECT

public:
    explicit ExternalToolBase(const QString& toolId, QObject* parent = nullptr);
    virtual ~ExternalToolBase();

    virtual bool isAvailable() const;
    virtual QString executablePath() const;

    virtual QJsonObject execute(const QStringList& args) = 0;

protected:
    QString runProcess(const QStringList& arguments, const QString& workingDir = QString());

    QString m_toolId;
    QString m_executablePath;
    QString m_lastOutput;
    int m_lastExitCode = 0;
};

class FbxConverterTool : public ExternalToolBase {
    Q_OBJECT

public:
    explicit FbxConverterTool(QObject* parent = nullptr);

    QJsonObject convert(const QString& inputFbx, const QString& outputKn5, bool generateLods = false);
    QJsonObject generateLods(const QString& inputKn5, const QString& outputDir, int lodCount = 4);
};

class SevenZipTool : public ExternalToolBase {
    Q_OBJECT

public:
    explicit SevenZipTool(QObject* parent = nullptr);

    QJsonObject extract(const QString& archivePath, const QString& outputDir);
    QJsonObject compress(const QStringList& files, const QString& outputArchive);
    QJsonObject listContents(const QString& archivePath);
};

class AudioTool : public ExternalToolBase {
    Q_OBJECT

public:
    explicit AudioTool(QObject* parent = nullptr);

    QJsonObject extractBanks(const QString& audioProjectPath, const QString& outputDir);
    QJsonObject buildBanks(const QString& inputDir, const QString& outputBank);
};

class ImageMagickTool : public ExternalToolBase {
    Q_OBJECT

public:
    explicit ImageMagickTool(QObject* parent = nullptr);

    QJsonObject convertImage(const QString& input, const QString& output, const QStringList& options = QStringList());
    QJsonObject createThumbnail(const QString& input, const QString& output, int size = 256);
    QJsonObject generateMipmaps(const QString& input, const QString& outputDir);
};

class Bc7Tool : public ExternalToolBase {
    Q_OBJECT

public:
    explicit Bc7Tool(QObject* parent = nullptr);

    QJsonObject compressDds(const QString& input, const QString& output, bool highQuality = false);
    QJsonObject compressPng(const QString& input, const QString& output);
};

} } // namespace ks::tools