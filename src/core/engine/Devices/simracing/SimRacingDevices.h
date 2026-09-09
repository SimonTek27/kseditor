#pragma once

#include <QObject>
#include <QVector>
#include <QVector3D>
#include <QMatrix4x4>
#include <QRect>
#include <QScreen>
#include <QSettings>
#include <QVariant>
#include <memory>

namespace ks::device {

// ============================================================================
// Monitor Configuration
// ============================================================================

struct MonitorInfo {
    int index = 0;
    QString name;
    QRect geometry;
    QRect availableGeometry;
    float dpi = 96.0f;
    bool isPrimary = false;
    float physicalWidthMm = 0;
    float physicalHeightMm = 0;
};

struct TripleMonitorConfig {
    bool enabled = false;
    int centerMonitorIndex = -1;
    float bezelCompensationMm = 20.0f;
    float fovHorizontal = 108.0f;
    float eyeDistance = 0.63f;
    bool verticalSync = true;
    int targetFps = 60;

    enum class Arrangement : uint8_t {
        Horizontal = 0,
        Landscape,
        PortraitTop,
        VerticalStack,
        Custom
    };
    Arrangement arrangement = Arrangement::Horizontal;

    void load(const QString& path) {
        if (path.isEmpty()) return;
        QSettings ini(path, QSettings::IniFormat);
        ini.beginGroup("TripleMonitor");
        enabled = ini.value("enabled", enabled).toBool();
        centerMonitorIndex = ini.value("centerMonitorIndex", centerMonitorIndex).toInt();
        bezelCompensationMm = ini.value("bezelCompensationMm", bezelCompensationMm).toFloat();
        fovHorizontal = ini.value("fovHorizontal", fovHorizontal).toFloat();
        eyeDistance = ini.value("eyeDistance", eyeDistance).toFloat();
        verticalSync = ini.value("verticalSync", verticalSync).toBool();
        targetFps = ini.value("targetFps", targetFps).toInt();
        arrangement = static_cast<Arrangement>(ini.value("arrangement", 0).toInt());
        ini.endGroup();
    }

    void save(const QString& path) const {
        if (path.isEmpty()) return;
        QSettings ini(path, QSettings::IniFormat);
        ini.beginGroup("TripleMonitor");
        ini.setValue("enabled", enabled);
        ini.setValue("centerMonitorIndex", centerMonitorIndex);
        ini.setValue("bezelCompensationMm", bezelCompensationMm);
        ini.setValue("fovHorizontal", fovHorizontal);
        ini.setValue("eyeDistance", eyeDistance);
        ini.setValue("verticalSync", verticalSync);
        ini.setValue("targetFps", targetFps);
        ini.setValue("arrangement", static_cast<int>(arrangement));
        ini.endGroup();
    }
};

// ============================================================================
// TripleMonitorManager
// ============================================================================
// Manages multi-monitor rendering for racing sims.
// Splits the viewport into 3 sections with per-eye projection correction
// to account for bezels and physical monitor angles.
// ============================================================================

class TripleMonitorManager : public QObject {
    Q_OBJECT
public:
    explicit TripleMonitorManager(QObject* parent = nullptr);
    ~TripleMonitorManager() override;

    // Detection
    void detectMonitors();
    int monitorCount() const { return static_cast<int>(m_monitors.size()); }
    const QVector<MonitorInfo>& monitors() const { return m_monitors; }
    const MonitorInfo& monitor(int index) const { return m_monitors[index]; }

    // Configuration
    void setConfig(const TripleMonitorConfig& config) { m_config = config; recalculate(); }
    const TripleMonitorConfig& config() const { return m_config; }

    // Per-eye rendering
    struct EyeViewport {
        QMatrix4x4 projection;
        QMatrix4x4 view;
        QRect viewport;
        float fovOffsetDeg = 0.0f;
    };

    EyeViewport leftEye() const { return m_leftEye; }
    EyeViewport centerEye() const { return m_centerEye; }
    EyeViewport rightEye() const { return m_rightEye; }

    // Update matrices based on driver position and FOV
    void recalculate();
    void updateView(const QMatrix4x4& baseView);

    // Combined viewport for full span
    QRect combinedViewport() const { return m_combinedViewport; }

signals:
    void monitorsChanged();
    void configChanged();

private:
    void calculateEyeProjections();
    float calculatePhysicalFovDeg() const;
    float calculateBezelOffsetDeg(int monitorIndex) const;

    TripleMonitorConfig m_config;
    QVector<MonitorInfo> m_monitors;

    EyeViewport m_leftEye;
    EyeViewport m_centerEye;
    EyeViewport m_rightEye;
    QRect m_combinedViewport;

    QMatrix4x4 m_baseView;
};

// ============================================================================
// Racing Input Device Types
// ============================================================================

enum class InputDeviceType : uint8_t {
    Keyboard = 0,
    Gamepad,
    SteeringWheel,
    FlightStick,
    Pedals,
    Shifter,
    Handbrake,
    MotionPlatform,
    ButtKicker,
    Custom
};

enum class AxisType : uint8_t {
    None = 0,
    SteeringWheel,     // rotation
    Throttle,          // analog axis
    Brake,             // analog axis
    Clutch,            // analog axis
    Handbrake,         // analog axis
    AccelerometerX,    // gyro
    AccelerometerY,
    AccelerometerZ,
    ForceFeedbackX,    // FFB
    ForceFeedbackY,
    POV,               // hat switch
    Button
};

struct InputAxis {
    AxisType type = AxisType::None;
    int deviceAxisIndex = -1;
    float value = 0.0f;
    float rawValue = 0.0f;
    float minRange = -1.0f;
    float maxRange = 1.0f;
    float deadZone = 0.0f;
    float saturation = 1.0f;
    float gamma = 1.0f;
    bool inverted = false;
};

struct InputButton {
    int deviceButtonIndex = -1;
    bool pressed = false;
    bool justPressed = false;
    bool justReleased = false;
    bool toggled = false;

    enum class Function : uint8_t {
        None = 0,
        ShiftUp,
        ShiftDown,
        Handbrake,
        LookLeft,
        LookRight,
        LookBack,
        DRS,
        ERS,
        PitLimiter,
        Flash,
        Horn,
        Headlights,
        Wipers,
        Menu,
        Reset,
        ToggleFFB,
        ToggleMirror,
        ToggleHUD,
    };
    Function function = Function::None;
};

struct InputDeviceProfile {
    QString name;
    QString deviceName;
    QString deviceGuid;
    InputDeviceType type = InputDeviceType::Gamepad;
    QVector<InputAxis> axes;
    QVector<InputButton> buttons;
    bool isGameController = false;
};

// ============================================================================
// ForceFeedbackEffect
// ============================================================================

struct ForceFeedbackEffect {
    enum class Type : uint8_t {
        Constant = 0,
        Spring,
        Damper,
        Friction,
        Sine,
        Square,
        Triangle,
        SawtoothUp,
        SawtoothDown,
        Ramp,
        Collision
    };

    Type type = Type::Constant;
    float magnitude = 0.0f;
    float direction = 0.0f;     // radians, 0 = forward
    float duration = 0.0f;      // seconds, 0 = infinite
    float period = 0.0f;        // seconds for periodic effects
    float attackLevel = 0.0f;
    float fadeLevel = 0.0f;
    float attackTime = 0.0f;
    float fadeTime = 0.0f;
    int conditionOffset = 0;
    int conditionSaturation = 0;
    int conditionCoefficient = 0;
    int conditionDeadband = 0;
    int conditionCenter = 0;
};

// ============================================================================
// RacingInputManager
// ============================================================================
// Detects and manages racing simulation input devices: steering wheels,
// pedals, shifters, handbrakes. Supports FFB, per-device profiles,
// dead zones, gamma curves, and force feedback effects.
// ============================================================================

class RacingInputManager : public QObject {
    Q_OBJECT
public:
    explicit RacingInputManager(QObject* parent = nullptr);
    ~RacingInputManager() override;

    // Detection
    bool initialize();
    void shutdown();
    void update(double dt);
    void scanDevices();

    // Device management
    int deviceCount() const { return static_cast<int>(m_devices.size()); }
    const InputDeviceProfile& device(int index) const { return m_devices[index]; }
    void selectDevice(int index);
    int selectedDevice() const { return m_selectedDevice; }

    // Axis access
    float getAxis(AxisType type) const;
    float getRawAxis(AxisType type) const;
    void setAxisDeadZone(AxisType type, float deadZone);
    void setAxisGamma(AxisType type, float gamma);
    void setAxisInverted(AxisType type, bool inverted);

    // Button access
    bool isButtonPressed(InputButton::Function function) const;
    bool isButtonJustPressed(InputButton::Function function) const;
    bool isButtonJustReleased(InputButton::Function function) const;

    // Force feedback
    bool isFFBSupported() const { return m_ffbSupported; }
    bool isFFBEnabled() const { return m_ffbEnabled; }
    void setFFBEnabled(bool enabled) { m_ffbEnabled = enabled; }
    void setFFBStrength(float strength) { m_ffbStrength = qBound(0.0f, strength, 1.0f); }
    float ffbStrength() const { return m_ffbStrength; }
    void playFFBEffect(const ForceFeedbackEffect& effect);
    void stopAllFFB();
    void setConstantForce(float force);
    void setSpringForce(float center, float stiffness, float damping);
    void setDamperForce(float velocity, float coefficient);
    void updateFFBFromPhysics(float aligningTorqueNm);

    // Steering-specific
    void setSteeringRange(float degrees) { m_steeringRange = degrees; }
    float steeringRange() const { return m_steeringRange; }
    float steeringAngle() const;

    // Calibration
    void beginCalibration();
    void endCalibration();
    bool isCalibrating() const { return m_calibrating; }

    // Profiles
    void saveProfile(const QString& path) const;
    void loadProfile(const QString& path);

signals:
    void deviceConnected(int index);
    void deviceDisconnected(int index);
    void deviceSelected(int index);
    void ffBUpdate(float force);
    void buttonPressed(InputButton::Function function);
    void calibrationComplete();

private:
    struct DeviceState {
        QVector<float> axisValues;
        QVector<bool> buttonStates;
        QVector<bool> prevButtonStates;
    };

    void processAxes(int deviceIndex);
    void processButtons(int deviceIndex);
    void applyAxisFilters(InputAxis& axis);
    float applyDeadZone(float value, float deadZone) const;
    float applyGammaCurve(float value, float gamma) const;
    float mapAxisRange(float raw, float minRange, float maxRange) const;

    QVector<InputDeviceProfile> m_devices;
    QVector<DeviceState> m_deviceStates;
    int m_selectedDevice = -1;
    bool m_initialized = false;
    bool m_calibrating = false;

    // FFB
    bool m_ffbSupported = false;
    bool m_ffbEnabled = false;
    float m_ffbStrength = 0.75f;
    float m_steeringRange = 900.0f;

    double m_updateAccumulator = 0.0;
    static constexpr double INPUT_POLL_RATE = 1000.0; // Hz
};

} // namespace ks::device