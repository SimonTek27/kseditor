#pragma once
// ============================================================================
// ACLivePreviewBridge.h
// Connects AC shared memory telemetry to audio engine parameter updates.
// Polls ACSharedMemory at ~60fps and drives all AC engine audio params.
// ============================================================================
#include "ACSharedMemory.h"
#include <QObject>
#include <QTimer>
#include <QMap>
#include <QString>
#include "Audio/AudioTypes.h"

namespace ks { namespace audio {

class ACLivePreviewBridge : public QObject {
    Q_OBJECT
public:
    explicit ACLivePreviewBridge(QObject* parent = nullptr);
    ~ACLivePreviewBridge() override;

    // Set the audio engine and active event instance to drive
    void setAudioEngine(KSAudioEngine* engine);
    void setEngineExteriorInstance(int instanceId);
    void setEngineInteriorInstance(int instanceId);

    // Start/stop live polling
    bool startLivePreview();
    void stopLivePreview();
    bool isRunning() const { return m_running; }

    // Manual param override (for editor slider testing without AC running)
    void setManualRPM(float rpm);
    void setManualThrottle(float throttle);
    void setManualBoost(float boost);
    void setManualGear(int gear);
    void setManualSpeed(float kmh);
    void setUseManualParams(bool use) { m_useManual = use; }

    // Doppler: set car world position for listener calculation
    void setCarPosition(float x, float y, float z);
    void setCarVelocity(float vx, float vy, float vz);

signals:
    void rpmChanged(float rpm);
    void throttleChanged(float throttle);
    void speedChanged(float kmh);
    void gearChanged(int gear);
    void previewStarted();
    void previewStopped();
    void error(const QString& msg);

private slots:
    void onPollTimer();

private:
    void applyParams(float rpm, float throttle, float brake,
                     float boost, int gear, float speed,
                     float clutch);
    float calcBoost(float rpm, float throttle) const;
    float calcDrivetrainSpeed(float rpm, int gear) const;

    ACSharedMemory*   m_shm      = nullptr;
    KSAudioEngine*    m_engine   = nullptr;
    QTimer*           m_timer    = nullptr;

    int m_extInstanceId = -1;
    int m_intInstanceId = -1;

    bool  m_running   = false;
    bool  m_useManual = false;

    // Manual params (for editor preview without AC)
    float m_manualRPM      = 1000.f;
    float m_manualThrottle = 0.f;
    float m_manualBoost    = 0.f;
    int   m_manualGear     = 1;
    float m_manualSpeed    = 0.f;

    // Car state for Doppler
    float m_carPosX=0, m_carPosY=0, m_carPosZ=0;
    float m_carVelX=0, m_carVelY=0, m_carVelZ=0;

    // Gear ratios for drivetrain_speed calculation (default AC values)
    static constexpr float GEAR_RATIOS[7] = {3.29f,2.16f,1.61f,1.27f,1.03f,0.84f,0.68f};
    static constexpr float FINAL_DRIVE    = 3.8f;
    static constexpr float WHEEL_RADIUS   = 0.33f; // metres
};

}} // ks::audio
