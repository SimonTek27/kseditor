#include "DeviceManager.h"
#include <QDebug>
#include <QStandardPaths>
#include <QDir>

namespace ks::device {

DeviceManager* DeviceManager::s_instance = nullptr;

DeviceManager* DeviceManager::instance()
{
    return s_instance;
}

DeviceManager::DeviceManager(QObject* parent)
    : QObject(parent)
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    m_tripleMonitor = std::make_unique<TripleMonitorManager>(this);
    m_racingInput = std::make_unique<RacingInputManager>(this);
    m_vr = std::make_unique<XrManager>(this);

    connect(m_tripleMonitor.get(), &TripleMonitorManager::monitorsChanged,
            this, &DeviceManager::onMonitorChanged);

    connect(m_vr.get(), &XrManager::sessionRunningChanged,
            this, &DeviceManager::onVRSessionChanged);

    connect(m_racingInput.get(), &RacingInputManager::deviceSelected,
            this, &DeviceManager::onInputDeviceChanged);

    m_statusTimer.setInterval(2000);
    connect(&m_statusTimer, &QTimer::timeout, this, [this]() {
        emit deviceStatusChanged(status());
    });
}

DeviceManager::~DeviceManager()
{
    shutdown();
    s_instance = nullptr;
}

bool DeviceManager::initialize()
{
    if (m_initialized) return true;

    qInfo() << "DeviceManager: Initializing...";

    m_tripleMonitor->detectMonitors();
    m_racingInput->initialize();

    m_statusTimer.start();

    m_initialized = true;
    qInfo() << "DeviceManager: Initialized -"
            << m_tripleMonitor->monitorCount() << "monitors,"
            << m_racingInput->deviceCount() << "input devices";

    return true;
}

void DeviceManager::shutdown()
{
    if (!m_initialized) return;

    m_statusTimer.stop();

    m_racingInput->shutdown();

    if (m_vr->isInitialized()) {
        m_vr->shutdown();
    }

    m_initialized = false;
    qInfo() << "DeviceManager: Shutdown complete";
}

void DeviceManager::setRenderMode(RenderMode mode)
{
    if (m_renderMode == mode) return;

    if (mode == RenderMode::VR && !m_vr->isInitialized()) {
        qWarning() << "DeviceManager: VR not available, falling back to single monitor";
        mode = RenderMode::SingleMonitor;
    }

    m_renderMode = mode;
    emit renderModeChanged(mode);

    qInfo() << "DeviceManager: Render mode changed to" << static_cast<int>(mode);
}

bool DeviceManager::isVRActive() const
{
    return m_renderMode == RenderMode::VR && m_vr->isSessionRunning();
}

bool DeviceManager::isTripleActive() const
{
    return m_renderMode == RenderMode::TripleMonitor &&
           m_tripleMonitor->monitorCount() >= 3;
}

DeviceManager::DeviceStatus DeviceManager::status() const
{
    DeviceStatus s;
    s.tripleMonitorConnected = m_tripleMonitor->monitorCount() >= 3;
    s.monitorCount = m_tripleMonitor->monitorCount();
    s.vrConnected = m_vr->isInitialized();
    s.vrSessionRunning = m_vr->isSessionRunning();
    s.steeringWheelConnected = m_racingInput->deviceCount() > 0;

    if (m_racingInput->selectedDevice() >= 0) {
        s.activeDeviceName = m_racingInput->device(m_racingInput->selectedDevice()).name;
    }

    return s;
}

void DeviceManager::loadProfiles(const QString& basePath)
{
    QDir dir(basePath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString inputProfile = dir.filePath("input.ini");
    if (QFile::exists(inputProfile)) {
        m_racingInput->loadProfile(inputProfile);
        qInfo() << "DeviceManager: Loaded input profile from" << inputProfile;
    }

    QString monitorProfile = dir.filePath("monitors.ini");
    if (QFile::exists(monitorProfile)) {
        TripleMonitorConfig config;
        config.load(monitorProfile);
        m_tripleMonitor->setConfig(config);
        qInfo() << "DeviceManager: Loaded monitor profile from" << monitorProfile;
    }
}

void DeviceManager::saveProfiles(const QString& basePath) const
{
    QDir dir(basePath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    m_racingInput->saveProfile(dir.filePath("input.ini"));
    m_tripleMonitor->config().save(dir.filePath("monitors.ini"));

    qInfo() << "DeviceManager: Saved profiles to" << basePath;
}

void DeviceManager::onMonitorChanged()
{
    qInfo() << "DeviceManager: Monitor configuration changed -"
            << m_tripleMonitor->monitorCount() << "monitors detected";
    emit deviceStatusChanged(status());
}

void DeviceManager::onVRSessionChanged(bool running)
{
    qInfo() << "DeviceManager: VR session" << (running ? "started" : "stopped");
    emit deviceStatusChanged(status());
}

void DeviceManager::onInputDeviceChanged(int index)
{
    if (index >= 0 && index < m_racingInput->deviceCount()) {
        qInfo() << "DeviceManager: Input device changed to"
                << m_racingInput->device(index).name;
    }
    emit deviceStatusChanged(status());
}

} // namespace ks::device