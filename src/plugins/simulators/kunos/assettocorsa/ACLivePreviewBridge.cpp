#include "ACLivePreviewBridge.h"
#include <QDebug>
#include <QtMath>
#include <cmath>
#include <QThread>

namespace ks { namespace audio {

constexpr float ACLivePreviewBridge::GEAR_RATIOS[7];
constexpr float ACLivePreviewBridge::FINAL_DRIVE;
constexpr float ACLivePreviewBridge::WHEEL_RADIUS;

ACLivePreviewBridge::ACLivePreviewBridge(QObject* parent)
    : QObject(parent)
{
    m_shm   = new ACSharedMemory(this);
    m_timer = new QTimer(this);
    m_timer->setInterval(16); // ~60 Hz
    connect(m_timer, &QTimer::timeout, this, &ACLivePreviewBridge::onPollTimer);
}

ACLivePreviewBridge::~ACLivePreviewBridge() {
    stopLivePreview();
}

void ACLivePreviewBridge::setAudioEngine(KSAudioEngine* engine) {
    m_engine = engine;
}

void ACLivePreviewBridge::setEngineExteriorInstance(int id) { m_extInstanceId = id; }
void ACLivePreviewBridge::setEngineInteriorInstance(int id) { m_intInstanceId = id; }

bool ACLivePreviewBridge::startLivePreview() {
    if (m_running) return true;

    if (!m_useManual) {
        bool attached = m_shm->attach();
        if (!attached) {
            qWarning() << "ACLivePreviewBridge: AC shared memory not available — "
                          "falling back to manual params";
            m_useManual = true;
        }
    }

    m_running = true;
    m_timer->start();
    emit previewStarted();
    return true;
}

void ACLivePreviewBridge::stopLivePreview() {
    if (!m_running) return;
    m_timer->stop();
    m_shm->detach();
    m_running = false;
    emit previewStopped();
}

// ============================================================================
// Core polling loop — called at ~60 Hz
// ============================================================================
void ACLivePreviewBridge::onPollTimer() {
    float rpm, throttle, brake, boost, speed, clutch;
    int   gear;

    if (m_useManual || !m_shm->isAttached()) {
        rpm      = m_manualRPM;
        throttle = m_manualThrottle;
        brake    = 0.f;
        boost    = m_manualBoost;
        gear     = m_manualGear;
        speed    = m_manualSpeed;
        clutch   = 0.f;
    } else {
        const auto& phys = m_shm->getPhysics();
        rpm      = float(phys.rpms);
        throttle = phys.gas;
        brake    = phys.brake;
        gear     = phys.gear;
        speed    = phys.speedKmh;
        clutch   = phys.clutch;
        boost    = calcBoost(rpm, throttle);
    }

    applyParams(rpm, throttle, brake, boost, gear, speed, clutch);

    emit rpmChanged(rpm);
    emit throttleChanged(throttle);
    emit speedChanged(speed);
    emit gearChanged(gear);
}

// ============================================================================
// Apply all AC audio params to engine events in one batch per frame
// ============================================================================
void ACLivePreviewBridge::applyParams(float rpm, float throttle, float brake,
                                       float boost, int gear, float speed,
                                       float clutch)
{
    if (!m_engine) return;

    float drivetrainSpeed = calcDrivetrainSpeed(rpm, gear);

    float airPressure = throttle * (rpm / 8000.f);

    static float prevThrottle = 0.f;
    float bov = 0.f;
    if (boost > 0.3f && prevThrottle > 0.8f && throttle < 0.3f)
        bov = boost;
    prevThrottle = throttle;

    float decay = qBound(0.f, (1.f - throttle) * (rpm / 8000.f), 1.f);

    // Apply to exterior instance
    if (m_extInstanceId >= 0) {
        m_engine->setEventParameter(m_extInstanceId, "rpms", rpm);
        m_engine->setEventParameter(m_extInstanceId, "throttle", throttle);
        m_engine->setEventParameter(m_extInstanceId, "brake", brake);
        m_engine->setEventParameter(m_extInstanceId, "boost", boost);
        m_engine->setEventParameter(m_extInstanceId, "decay", decay);
        m_engine->setEventParameter(m_extInstanceId, "speed", speed / 3.6f);
        m_engine->setEventParameter(m_extInstanceId, "gear", float(gear));
    }

    // Apply to interior instance
    if (m_intInstanceId >= 0) {
        m_engine->setEventParameter(m_intInstanceId, "rpms", rpm);
        m_engine->setEventParameter(m_intInstanceId, "throttle", throttle);
        m_engine->setEventParameter(m_intInstanceId, "brake", brake);
        m_engine->setEventParameter(m_intInstanceId, "boost", boost);
        m_engine->setEventParameter(m_intInstanceId, "decay", decay);
        m_engine->setEventParameter(m_intInstanceId, "speed", speed / 3.6f);
        m_engine->setEventParameter(m_intInstanceId, "gear", float(gear));
    }

    // Update 3D position for Doppler
    if (m_extInstanceId >= 0) {
        m_engine->setEventPosition(m_extInstanceId, QVector3D(m_carPosX, m_carPosY, m_carPosZ));
    }
}

// ============================================================================
// Manual param setters (for editor slider testing)
// ============================================================================
void ACLivePreviewBridge::setManualRPM(float rpm)          { m_manualRPM = rpm; }
void ACLivePreviewBridge::setManualThrottle(float t)       { m_manualThrottle = t; }
void ACLivePreviewBridge::setManualBoost(float b)          { m_manualBoost = b; }
void ACLivePreviewBridge::setManualGear(int g)             { m_manualGear = g; }
void ACLivePreviewBridge::setManualSpeed(float s)          { m_manualSpeed = s; }
void ACLivePreviewBridge::setCarPosition(float x,float y,float z) { m_carPosX=x; m_carPosY=y; m_carPosZ=z; }
void ACLivePreviewBridge::setCarVelocity(float vx,float vy,float vz) { m_carVelX=vx; m_carVelY=vy; m_carVelZ=vz; }

// ============================================================================
// Helpers
// ============================================================================
float ACLivePreviewBridge::calcBoost(float rpm, float throttle) const {
    if (rpm < 2000.f || throttle < 0.1f) return 0.f;
    float rpmFactor = qBound(0.f, (rpm - 2000.f) / 5000.f, 1.f);
    return rpmFactor * throttle * 1.5f;
}

float ACLivePreviewBridge::calcDrivetrainSpeed(float rpm, int gear) const {
    int g = qBound(1, gear, 7) - 1;
    if (rpm < 100.f) return 0.f;
    float angularVelocity = (rpm / 60.f) * 2.f * float(M_PI);
    float wheelRPM        = angularVelocity / (GEAR_RATIOS[g] * FINAL_DRIVE);
    return wheelRPM * WHEEL_RADIUS;
}

}} // ks::audio
