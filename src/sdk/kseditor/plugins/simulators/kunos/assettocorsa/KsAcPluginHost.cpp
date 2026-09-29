#include "KsAcPluginHost.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>

namespace ks {

KsAcPluginHost& KsAcPluginHost::instance()
{
    static KsAcPluginHost host;
    return host;
}

KsAcPluginHost::KsAcPluginHost(QObject* parent)
    : QObject(parent)
{
}

bool KsAcPluginHost::load()
{
    if (m_loaded)
        return true;

    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        appDir + QStringLiteral("/plugins/ksassettocorsa.dll"),
        appDir + QStringLiteral("/plugins/ksAssettoCorsa.dll"),
        appDir + QStringLiteral("/../bin/plugins/ksassettocorsa.dll"),
        appDir + QStringLiteral("/../bin/plugins/ksAssettoCorsa.dll"),
        appDir + QStringLiteral("/bin/plugins/ksassettocorsa.dll"),
    };

    for (const QString& path : candidates) {
        if (QFile::exists(path)) {
            m_lib.setFileName(path);
            break;
        }
    }
    if (m_lib.fileName().isEmpty())
        m_lib.setFileName(candidates.first());

    if (!m_lib.load()) {
        qWarning() << "[KsAcPluginHost] Assetto Corsa plugin not loaded:"
                   << m_lib.fileName() << "-" << m_lib.errorString();
        return false;
    }

    if (!resolveExports()) {
        qWarning() << "[KsAcPluginHost] missing exports in" << m_lib.fileName();
        m_lib.unload();
        return false;
    }

    m_loaded = true;
    qInfo() << "[KsAcPluginHost] loaded" << m_lib.fileName()
            << "version" << pluginVersion();
    return true;
}

bool KsAcPluginHost::resolveExports()
{
    m_getPluginId = reinterpret_cast<Fn_cstr>(m_lib.resolve("getPluginId"));
    m_getPluginName = reinterpret_cast<Fn_cstr>(m_lib.resolve("getPluginName"));
    m_getPluginVersion = reinterpret_cast<Fn_cstr>(m_lib.resolve("getPluginVersion"));
    m_getPluginDescription = reinterpret_cast<Fn_cstr>(m_lib.resolve("getPluginDescription"));
    m_initializePlugin = reinterpret_cast<Fn_bool>(m_lib.resolve("initializePlugin"));
    m_shutdownPlugin = reinterpret_cast<Fn_void>(m_lib.resolve("shutdownPlugin"));
    m_isPluginAvailable = reinterpret_cast<Fn_bool>(m_lib.resolve("isPluginAvailable"));
    m_getInstallPath = reinterpret_cast<Fn_cstr>(m_lib.resolve("getInstallPath"));
    m_setInstallPath = reinterpret_cast<Fn_setPath>(m_lib.resolve("setInstallPath"));
    m_getCarCount = reinterpret_cast<Fn_int>(m_lib.resolve("getCarCount"));
    m_getTrackCount = reinterpret_cast<Fn_int>(m_lib.resolve("getTrackCount"));
    m_getCarListJson = reinterpret_cast<Fn_cstr>(m_lib.resolve("getCarListJson"));
    m_getTrackListJson = reinterpret_cast<Fn_cstr>(m_lib.resolve("getTrackListJson"));
    m_getContentSummaryJson = reinterpret_cast<Fn_cstr>(m_lib.resolve("getContentSummaryJson"));
    m_launchGame = reinterpret_cast<Fn_launch>(m_lib.resolve("launchGame"));
    m_launchPractice = reinterpret_cast<Fn_launchTrack>(m_lib.resolve("launchPractice"));
    m_launchTimeTrial = reinterpret_cast<Fn_launchTrack>(m_lib.resolve("launchTimeTrial"));
    m_stopGame = reinterpret_cast<Fn_bool>(m_lib.resolve("stopGame"));
    m_isGameRunning = reinterpret_cast<Fn_bool>(m_lib.resolve("isGameRunning"));
    m_telemetryAttach = reinterpret_cast<Fn_bool>(m_lib.resolve("telemetryAttach"));
    m_telemetryDetach = reinterpret_cast<Fn_void>(m_lib.resolve("telemetryDetach"));
    m_telemetryIsAttached = reinterpret_cast<Fn_bool>(m_lib.resolve("telemetryIsAttached"));
    m_telemetryGetJson = reinterpret_cast<Fn_cstr>(m_lib.resolve("telemetryGetJson"));
    m_kn5Validate = reinterpret_cast<Fn_bool_path>(m_lib.resolve("kn5Validate"));
    m_kn5GetInfoJson = reinterpret_cast<Fn_str_path>(m_lib.resolve("kn5GetInfoJson"));
    m_workshopGetCategoriesJson = reinterpret_cast<Fn_cstr>(m_lib.resolve("workshopGetCategoriesJson"));
    m_workshopRefresh = reinterpret_cast<Fn_void>(m_lib.resolve("workshopRefresh"));
    m_workshopGetItemsJson = reinterpret_cast<Fn_cstr>(m_lib.resolve("workshopGetItemsJson"));

    return m_getPluginId && m_getPluginName && m_getPluginVersion
        && m_getPluginDescription && m_initializePlugin && m_shutdownPlugin
        && m_isPluginAvailable && m_getInstallPath && m_setInstallPath
        && m_getCarCount && m_getTrackCount;
}

bool KsAcPluginHost::initialize()
{
    if (!m_loaded && !load())
        return false;
    return m_initializePlugin && m_initializePlugin();
}

void KsAcPluginHost::shutdown()
{
    if (!m_loaded)
        return;
    if (m_shutdownPlugin)
        m_shutdownPlugin();
    m_lib.unload();
    m_loaded = false;
}

QString KsAcPluginHost::pluginId() const
{
    if (!m_loaded || !m_getPluginId)
        return {};
    return QString::fromUtf8(m_getPluginId());
}

QString KsAcPluginHost::pluginName() const
{
    if (!m_loaded || !m_getPluginName)
        return {};
    return QString::fromUtf8(m_getPluginName());
}

QString KsAcPluginHost::pluginVersion() const
{
    if (!m_loaded || !m_getPluginVersion)
        return {};
    return QString::fromUtf8(m_getPluginVersion());
}

QString KsAcPluginHost::pluginDescription() const
{
    if (!m_loaded || !m_getPluginDescription)
        return {};
    return QString::fromUtf8(m_getPluginDescription());
}

bool KsAcPluginHost::isPluginAvailable() const
{
    if (!m_loaded || !m_isPluginAvailable)
        return false;
    return m_isPluginAvailable();
}

QString KsAcPluginHost::installPath() const
{
    if (!m_loaded || !m_getInstallPath)
        return {};
    return QString::fromUtf8(m_getInstallPath());
}

void KsAcPluginHost::setInstallPath(const QString& path)
{
    if (!m_loaded && !load())
        return;
    if (m_setInstallPath)
        m_setInstallPath(path.toUtf8().constData());
}

int KsAcPluginHost::carCount() const
{
    if (!m_loaded || !m_getCarCount)
        return 0;
    return m_getCarCount();
}

int KsAcPluginHost::trackCount() const
{
    if (!m_loaded || !m_getTrackCount)
        return 0;
    return m_getTrackCount();
}

QString KsAcPluginHost::carListJson() const
{
    if (!m_loaded || !m_getCarListJson)
        return QStringLiteral("[]");
    const char* s = m_getCarListJson();
    return s ? QString::fromUtf8(s) : QStringLiteral("[]");
}

QString KsAcPluginHost::trackListJson() const
{
    if (!m_loaded || !m_getTrackListJson)
        return QStringLiteral("[]");
    const char* s = m_getTrackListJson();
    return s ? QString::fromUtf8(s) : QStringLiteral("[]");
}

QString KsAcPluginHost::contentSummaryJson() const
{
    if (!m_loaded || !m_getContentSummaryJson)
        return QStringLiteral("{}");
    const char* s = m_getContentSummaryJson();
    return s ? QString::fromUtf8(s) : QStringLiteral("{}");
}

bool KsAcPluginHost::launchGame(const QString& car, const QString& track)
{
    if (!m_loaded || !m_launchGame)
        return false;
    return m_launchGame(car.toUtf8().constData(), track.toUtf8().constData());
}

bool KsAcPluginHost::launchPractice(const QString& track)
{
    if (!m_loaded || !m_launchPractice)
        return false;
    return m_launchPractice(track.toUtf8().constData());
}

bool KsAcPluginHost::launchTimeTrial(const QString& track)
{
    if (!m_loaded || !m_launchTimeTrial)
        return false;
    return m_launchTimeTrial(track.toUtf8().constData());
}

bool KsAcPluginHost::stopGame()
{
    if (!m_loaded || !m_stopGame)
        return false;
    return m_stopGame();
}

bool KsAcPluginHost::isGameRunning() const
{
    if (!m_loaded || !m_isGameRunning)
        return false;
    return m_isGameRunning();
}

bool KsAcPluginHost::telemetryAttach()
{
    if (!m_loaded || !m_telemetryAttach)
        return false;
    return m_telemetryAttach();
}

void KsAcPluginHost::telemetryDetach()
{
    if (m_loaded && m_telemetryDetach)
        m_telemetryDetach();
}

bool KsAcPluginHost::telemetryIsAttached() const
{
    if (!m_loaded || !m_telemetryIsAttached)
        return false;
    return m_telemetryIsAttached();
}

QString KsAcPluginHost::telemetryGetJson() const
{
    if (!m_loaded || !m_telemetryGetJson)
        return QStringLiteral("{}");
    const char* s = m_telemetryGetJson();
    return s ? QString::fromUtf8(s) : QStringLiteral("{}");
}

bool KsAcPluginHost::kn5Validate(const QString& path) const
{
    if (!m_loaded || !m_kn5Validate)
        return false;
    return m_kn5Validate(path.toUtf8().constData());
}

QString KsAcPluginHost::kn5GetInfoJson(const QString& path) const
{
    if (!m_loaded || !m_kn5GetInfoJson)
        return QStringLiteral("{}");
    const char* s = m_kn5GetInfoJson(path.toUtf8().constData());
    return s ? QString::fromUtf8(s) : QStringLiteral("{}");
}

QString KsAcPluginHost::workshopGetCategoriesJson() const
{
    if (!m_loaded || !m_workshopGetCategoriesJson)
        return QStringLiteral("[]");
    const char* s = m_workshopGetCategoriesJson();
    return s ? QString::fromUtf8(s) : QStringLiteral("[]");
}

void KsAcPluginHost::workshopRefresh()
{
    if (m_loaded && m_workshopRefresh)
        m_workshopRefresh();
}

QString KsAcPluginHost::workshopGetItemsJson() const
{
    if (!m_loaded || !m_workshopGetItemsJson)
        return QStringLiteral("[]");
    const char* s = m_workshopGetItemsJson();
    return s ? QString::fromUtf8(s) : QStringLiteral("[]");
}

} // namespace ks
