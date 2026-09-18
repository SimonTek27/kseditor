#include "SimRacingDevices.h"
#include <QGuiApplication>
#include <QScreen>
#include <QWindow>
#include <QSettings>
#include <QDebug>
#include <QtMath>
#include <cmath>

namespace ks::device {

// ============================================================================
// TripleMonitorManager
// ============================================================================

TripleMonitorManager::TripleMonitorManager(QObject* parent)
    : QObject(parent) {
    detectMonitors();
}

TripleMonitorManager::~TripleMonitorManager() = default;

void TripleMonitorManager::detectMonitors()
{
    m_monitors.clear();
    const auto screens = QGuiApplication::screens();
    for (int i = 0; i < screens.size(); ++i) {
        MonitorInfo info;
        info.index = i;
        info.name = screens[i]->name();
        info.geometry = screens[i]->geometry();
        info.availableGeometry = screens[i]->availableGeometry();
        info.dpi = screens[i]->logicalDotsPerInch();
        info.isPrimary = (QGuiApplication::primaryScreen() == screens[i]);
        info.physicalWidthMm = screens[i]->physicalSize().width();
        info.physicalHeightMm = screens[i]->physicalSize().height();
        m_monitors.append(info);
        qInfo() << "Monitor" << i << ":" << info.name
                << info.geometry.width() << "x" << info.geometry.height()
                << (info.isPrimary ? "(primary)" : "");
    }
    emit monitorsChanged();
}

void TripleMonitorManager::recalculate()
{
    if (m_monitors.size() < 3) {
        qWarning() << "TripleMonitor: Less than 3 monitors detected";
        m_leftEye = m_centerEye = m_rightEye = {};
        return;
    }

    calculateEyeProjections();

    int center = m_config.centerMonitorIndex;
    if (center < 0 || center >= m_monitors.size()) {
        for (int i = 0; i < m_monitors.size(); ++i) {
            if (m_monitors[i].isPrimary) { center = i; break; }
        }
        if (center < 0) center = 1;
    }

    QRect leftGeo, centerGeo, rightGeo;
    switch (m_config.arrangement) {
        case TripleMonitorConfig::Arrangement::Horizontal:
            leftGeo = m_monitors[qMax(0, center - 1)].geometry;
            centerGeo = m_monitors[center].geometry;
            rightGeo = m_monitors[qMin(m_monitors.size() - 1, center + 1)].geometry;
            break;
        default:
            leftGeo = m_monitors[qMax(0, center - 1)].geometry;
            centerGeo = m_monitors[center].geometry;
            rightGeo = m_monitors[qMin(m_monitors.size() - 1, center + 1)].geometry;
            break;
    }

    m_combinedViewport = QRect(
        qMin(leftGeo.x(), qMin(centerGeo.x(), rightGeo.x())),
        qMin(leftGeo.y(), qMin(centerGeo.y(), rightGeo.y())),
        leftGeo.width() + centerGeo.width() + rightGeo.width(),
        qMax(leftGeo.height(), qMax(centerGeo.height(), rightGeo.height()))
    );

    m_leftEye.viewport = QRect(0, 0, leftGeo.width(), leftGeo.height());
    m_centerEye.viewport = QRect(leftGeo.width(), 0, centerGeo.width(), centerGeo.height());
    m_rightEye.viewport = QRect(leftGeo.width() + centerGeo.width(), 0,
                                rightGeo.width(), rightGeo.height());

    emit configChanged();
}

void TripleMonitorManager::updateView(const QMatrix4x4& baseView)
{
    m_baseView = baseView;
    recalculate();
}

void TripleMonitorManager::calculateEyeProjections()
{
    float totalFov = m_config.fovHorizontal;
    float perEyeFov = totalFov / 3.0f;
    float aspectPerMonitor = 16.0f / 9.0f;

    float bezelOffsetDeg = calculateBezelOffsetDeg(0);

    m_centerEye.fovOffsetDeg = 0.0f;
    m_centerEye.projection.perspective(perEyeFov, aspectPerMonitor, 0.1f, 1000.0f);
    m_centerEye.view = m_baseView;

    m_leftEye.fovOffsetDeg = -(perEyeFov + bezelOffsetDeg);
    QMatrix4x4 leftRot;
    leftRot.rotate(-(perEyeFov + bezelOffsetDeg), 0, 1, 0);
    m_leftEye.projection.perspective(perEyeFov, aspectPerMonitor, 0.1f, 1000.0f);
    m_leftEye.view = leftRot * m_baseView;

    m_rightEye.fovOffsetDeg = (perEyeFov + bezelOffsetDeg);
    QMatrix4x4 rightRot;
    rightRot.rotate((perEyeFov + bezelOffsetDeg), 0, 1, 0);
    m_rightEye.projection.perspective(perEyeFov, aspectPerMonitor, 0.1f, 1000.0f);
    m_rightEye.view = rightRot * m_baseView;
}

float TripleMonitorManager::calculatePhysicalFovDeg() const
{
    if (m_monitors.isEmpty()) return 60.0f;

    int centerIdx = m_config.centerMonitorIndex;
    if (centerIdx < 0 || centerIdx >= m_monitors.size()) centerIdx = 0;
    const auto& center = m_monitors[centerIdx];

    float widthMm = center.physicalWidthMm;
    float eyeDistMm = m_config.eyeDistance * 1000.0f;
    return 2.0f * qRadiansToDegrees(qAtan(widthMm / (2.0f * eyeDistMm)));
}

float TripleMonitorManager::calculateBezelOffsetDeg(int monitorIndex) const
{
    float eyeDistMm = m_config.eyeDistance * 1000.0f;
    float halfBezel = m_config.bezelCompensationMm / 2.0f;
    return qRadiansToDegrees(qAtan(halfBezel / eyeDistMm));
}

// ============================================================================
// RacingInputManager
// ============================================================================

RacingInputManager::RacingInputManager(QObject* parent)
    : QObject(parent) {}

RacingInputManager::~RacingInputManager() {
    shutdown();
}

bool RacingInputManager::initialize()
{
    if (m_initialized) return true;

    scanDevices();
    m_initialized = true;
    qInfo() << "RacingInputManager: Initialized with" << m_devices.size() << "devices";
    return true;
}

void RacingInputManager::shutdown()
{
    m_devices.clear();
    m_deviceStates.clear();
    m_selectedDevice = -1;
    m_initialized = false;
}

void RacingInputManager::update(double dt)
{
    if (!m_initialized) return;

    m_updateAccumulator += dt;
    double pollInterval = 1.0 / INPUT_POLL_RATE;
    if (m_updateAccumulator < pollInterval) return;
    m_updateAccumulator -= pollInterval;

    for (int i = 0; i < m_devices.size(); ++i) {
        processAxes(i);
        processButtons(i);
    }
}

void RacingInputManager::scanDevices()
{
    m_devices.clear();
    m_deviceStates.clear();

    InputDeviceProfile keyboard;
    keyboard.name = "Keyboard";
    keyboard.deviceName = "System Keyboard";
    keyboard.type = InputDeviceType::Keyboard;

    InputAxis steerAxis;
    steerAxis.type = AxisType::SteeringWheel;
    steerAxis.deviceAxisIndex = 0;
    steerAxis.minRange = -1.0f;
    steerAxis.maxRange = 1.0f;
    keyboard.axes.append(steerAxis);

    InputAxis throttleAxis;
    throttleAxis.type = AxisType::Throttle;
    throttleAxis.deviceAxisIndex = 1;
    throttleAxis.minRange = 0.0f;
    throttleAxis.maxRange = 1.0f;
    keyboard.axes.append(throttleAxis);

    InputAxis brakeAxis;
    brakeAxis.type = AxisType::Brake;
    brakeAxis.deviceAxisIndex = 2;
    brakeAxis.minRange = 0.0f;
    brakeAxis.maxRange = 1.0f;
    keyboard.axes.append(brakeAxis);

    InputButton shiftUp;
    shiftUp.deviceButtonIndex = 0;
    shiftUp.function = InputButton::Function::ShiftUp;
    keyboard.buttons.append(shiftUp);

    InputButton shiftDown;
    shiftDown.deviceButtonIndex = 1;
    shiftDown.function = InputButton::Function::ShiftDown;
    keyboard.buttons.append(shiftDown);

    keyboard.isGameController = false;
    m_devices.append(keyboard);

    DeviceState ks;
    ks.axisValues.resize(3);
    ks.buttonStates.resize(2);
    ks.prevButtonStates.resize(2);
    m_deviceStates.append(ks);

    qInfo() << "RacingInputManager: Scanned" << m_devices.size() << "devices";
    for (int i = 0; i < m_devices.size(); ++i) {
        qInfo() << "  " << m_devices[i].name << "-" << m_devices[i].axes.size() << "axes,"
                << m_devices[i].buttons.size() << "buttons";
    }
}

void RacingInputManager::selectDevice(int index)
{
    if (index < 0 || index >= m_devices.size()) return;
    m_selectedDevice = index;
    emit deviceSelected(index);
    qInfo() << "RacingInputManager: Selected device" << m_devices[index].name;
}

void RacingInputManager::processAxes(int deviceIndex)
{
    if (deviceIndex < 0 || deviceIndex >= m_devices.size()) return;
    auto& profile = m_devices[deviceIndex];
    auto& state = m_deviceStates[deviceIndex];

    for (int i = 0; i < profile.axes.size(); ++i) {
        auto& axis = profile.axes[i];
        float raw = state.axisValues.value(i, 0.0f);
        float mapped = mapAxisRange(raw, axis.minRange, axis.maxRange);
        applyAxisFilters(axis);
        axis.rawValue = raw;
        axis.value = mapped;
    }
}

void RacingInputManager::processButtons(int deviceIndex)
{
    if (deviceIndex < 0 || deviceIndex >= m_devices.size()) return;
    auto& profile = m_devices[deviceIndex];
    auto& state = m_deviceStates[deviceIndex];

    for (int i = 0; i < profile.buttons.size(); ++i) {
        auto& btn = profile.buttons[i];
        int idx = btn.deviceButtonIndex;
        if (idx < 0 || idx >= state.buttonStates.size()) continue;

        bool current = state.buttonStates[idx];
        bool previous = state.prevButtonStates[idx];
        btn.pressed = current;
        btn.justPressed = current && !previous;
        btn.justReleased = !current && previous;

        if (btn.justPressed) {
            emit buttonPressed(btn.function);
        }
    }

    state.prevButtonStates = state.buttonStates;
}

void RacingInputManager::applyAxisFilters(InputAxis& axis)
{
    axis.value = applyDeadZone(axis.value, axis.deadZone);
    if (axis.inverted) axis.value = -axis.value;
    axis.value = qBound(-1.0f, axis.value, 1.0f);
}

float RacingInputManager::applyDeadZone(float value, float deadZone) const
{
    if (deadZone <= 0.0f) return value;
    if (std::abs(value) < deadZone) return 0.0f;
    float sign = (value > 0) ? 1.0f : -1.0f;
    return sign * (std::abs(value) - deadZone) / (1.0f - deadZone);
}

float RacingInputManager::applyGammaCurve(float value, float gamma) const
{
    if (gamma <= 0.0f || gamma >= 2.0f) return value;
    float sign = (value > 0) ? 1.0f : -1.0f;
    return sign * std::pow(std::abs(value), gamma);
}

float RacingInputManager::mapAxisRange(float raw, float minRange, float maxRange) const
{
    float range = maxRange - minRange;
    if (range <= 0.0f) return 0.0f;
    return (raw - minRange) / range * 2.0f - 1.0f;
}

float RacingInputManager::getAxis(AxisType type) const
{
    if (m_selectedDevice < 0 || m_selectedDevice >= m_devices.size()) return 0.0f;
    const auto& axes = m_devices[m_selectedDevice].axes;
    for (const auto& axis : axes) {
        if (axis.type == type) return axis.value;
    }
    return 0.0f;
}

float RacingInputManager::getRawAxis(AxisType type) const
{
    if (m_selectedDevice < 0 || m_selectedDevice >= m_devices.size()) return 0.0f;
    const auto& axes = m_devices[m_selectedDevice].axes;
    for (const auto& axis : axes) {
        if (axis.type == type) return axis.rawValue;
    }
    return 0.0f;
}

void RacingInputManager::setAxisDeadZone(AxisType type, float deadZone)
{
    if (m_selectedDevice < 0 || m_selectedDevice >= m_devices.size()) return;
    auto& axes = m_devices[m_selectedDevice].axes;
    for (auto& axis : axes) {
        if (axis.type == type) {
            axis.deadZone = qBound(0.0f, deadZone, 0.5f);
            return;
        }
    }
}

void RacingInputManager::setAxisGamma(AxisType type, float gamma)
{
    if (m_selectedDevice < 0 || m_selectedDevice >= m_devices.size()) return;
    auto& axes = m_devices[m_selectedDevice].axes;
    for (auto& axis : axes) {
        if (axis.type == type) {
            axis.gamma = qBound(0.1f, gamma, 3.0f);
            return;
        }
    }
}

void RacingInputManager::setAxisInverted(AxisType type, bool inverted)
{
    if (m_selectedDevice < 0 || m_selectedDevice >= m_devices.size()) return;
    auto& axes = m_devices[m_selectedDevice].axes;
    for (auto& axis : axes) {
        if (axis.type == type) {
            axis.inverted = inverted;
            return;
        }
    }
}

bool RacingInputManager::isButtonPressed(InputButton::Function function) const
{
    if (m_selectedDevice < 0 || m_selectedDevice >= m_devices.size()) return false;
    for (const auto& btn : m_devices[m_selectedDevice].buttons) {
        if (btn.function == function) return btn.pressed;
    }
    return false;
}

bool RacingInputManager::isButtonJustPressed(InputButton::Function function) const
{
    if (m_selectedDevice < 0 || m_selectedDevice >= m_devices.size()) return false;
    for (const auto& btn : m_devices[m_selectedDevice].buttons) {
        if (btn.function == function) return btn.justPressed;
    }
    return false;
}

bool RacingInputManager::isButtonJustReleased(InputButton::Function function) const
{
    if (m_selectedDevice < 0 || m_selectedDevice >= m_devices.size()) return false;
    for (const auto& btn : m_devices[m_selectedDevice].buttons) {
        if (btn.function == function) return btn.justReleased;
    }
    return false;
}

float RacingInputManager::steeringAngle() const
{
    float normalized = getAxis(AxisType::SteeringWheel);
    return normalized * (m_steeringRange / 2.0f);
}

void RacingInputManager::beginCalibration()
{
    m_calibrating = true;
    qInfo() << "RacingInputManager: Calibration started";
}

void RacingInputManager::endCalibration()
{
    m_calibrating = false;
    emit calibrationComplete();
    qInfo() << "RacingInputManager: Calibration completed";
}

void RacingInputManager::playFFBEffect(const ForceFeedbackEffect& effect)
{
    if (!m_ffbEnabled || !m_ffbSupported) return;
    emit ffBUpdate(effect.magnitude * m_ffbStrength);
}

void RacingInputManager::stopAllFFB()
{
    emit ffBUpdate(0.0f);
}

void RacingInputManager::setConstantForce(float force)
{
    if (!m_ffbEnabled) return;
    emit ffBUpdate(force * m_ffbStrength);
}

void RacingInputManager::setSpringForce(float center, float stiffness, float damping)
{
    if (!m_ffbEnabled) return;
    float angle = steeringAngle();
    float springTorque = -(angle - center) * stiffness * 0.01f - damping * 0.001f * angle;
    emit ffBUpdate(qBound(-1.0f, springTorque * m_ffbStrength, 1.0f));
}

void RacingInputManager::setDamperForce(float velocity, float coefficient)
{
    if (!m_ffbEnabled) return;
    emit ffBUpdate(qBound(-1.0f, -velocity * coefficient * 0.01f * m_ffbStrength, 1.0f));
}

void RacingInputManager::updateFFBFromPhysics(float aligningTorqueNm)
{
    if (!m_ffbEnabled) return;
    float norm = qBound(-1.0f, aligningTorqueNm / 12.0f, 1.0f);
    emit ffBUpdate(norm * m_ffbStrength);
}

void RacingInputManager::saveProfile(const QString& path) const
{
    if (path.isEmpty() || m_selectedDevice < 0) return;
    QSettings ini(path, QSettings::IniFormat);
    ini.beginGroup("RacingInput");
    ini.setValue("selectedDevice", m_selectedDevice);
    ini.setValue("steeringRange", m_steeringRange);
    ini.setValue("ffbEnabled", m_ffbEnabled);
    ini.setValue("ffbStrength", m_ffbStrength);

    const auto& device = m_devices[m_selectedDevice];
    ini.beginGroup("Device");
    ini.setValue("name", device.name);
    ini.setValue("type", static_cast<int>(device.type));
    ini.endGroup();

    ini.beginGroup("Axes");
    for (const auto& axis : device.axes) {
        QString key = QString("axis_%1").arg(static_cast<int>(axis.type));
        ini.setValue(key + "_deadZone", axis.deadZone);
        ini.setValue(key + "_gamma", axis.gamma);
        ini.setValue(key + "_inverted", axis.inverted);
    }
    ini.endGroup();
    ini.endGroup();
}

void RacingInputManager::loadProfile(const QString& path)
{
    if (path.isEmpty()) return;
    QSettings ini(path, QSettings::IniFormat);
    ini.beginGroup("RacingInput");
    m_steeringRange = ini.value("steeringRange", 900.0f).toFloat();
    m_ffbEnabled = ini.value("ffbEnabled", false).toBool();
    m_ffbStrength = ini.value("ffbStrength", 0.75f).toFloat();

    int devIdx = ini.value("selectedDevice", 0).toInt();
    if (devIdx >= 0 && devIdx < m_devices.size()) {
        selectDevice(devIdx);
    }

    ini.beginGroup("Axes");
    if (m_selectedDevice >= 0 && m_selectedDevice < m_devices.size()) {
        auto& axes = m_devices[m_selectedDevice].axes;
        for (auto& axis : axes) {
            QString key = QString("axis_%1").arg(static_cast<int>(axis.type));
            axis.deadZone = ini.value(key + "_deadZone", axis.deadZone).toFloat();
            axis.gamma = ini.value(key + "_gamma", axis.gamma).toFloat();
            axis.inverted = ini.value(key + "_inverted", axis.inverted).toBool();
        }
    }
    ini.endGroup();
    ini.endGroup();
}

} // namespace ks::device