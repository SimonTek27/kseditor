#pragma once

#include <QString>
#include <QByteArray>
#include <QVector>
#include <QTimer>
#include <QObject>

/**
 * @brief Assetto Corsa Shared Memory Reader
 *
 * Reads real-time telemetry data from Assetto Corsa's shared memory.
 * Based on sim_info.py from AC Python documentation and
 * AC-SimInfo-Docs-Markdown (github.com/dcratliff19/AC-SimInfo-Docs-Markdown)
 *
 * The shared memory structures provide:
 * - Physics data (speed, RPM, throttle, brake, gear, etc.)
 * - Graphics data (camera, weather, time)
 * - Static data (car info, track info, driver info)
 */
class ACSharedMemory : public QObject {
    Q_OBJECT
public:
    // Shared memory structures (matching AC's memory layout)
    struct SPageFileStatic {
        char acVersion[32];
        char acGameName[32];
        int sectorCount;
        int maxCars;
        char playerName[64];
        char playerTeam[64];
        char playerGuid[64];
        int playerSkinIndex;
        int playerClassIndex;
        int numCars;
        int playerCarIndex;
        int trackLength;
        char trackName[64];
        char trackConfig[32];
        float durationTime;
    };

    struct SPageFileGraphics {
        int packetId;
        float gas;
        float brake;
        float clutch;
        int gear;
        int rpms;
        float steerAngle;
        float speedKmh;
        float velocity[3];
        float accG[3];
        float wheelAngle[4];
        float slipAngle[4];
        float slipRatio[4];
        float tyreSlip[4];
        float wheelLoad[4];
        float tyrePressure[4];
        float wheelCombination[4][2];
        float driveDamage[4];
        float tcDamage;
        float vDamage[3];
        int warningLights;
        float time;
        float timeDelta;
    };

    struct SPageFilePhysics {
        int packetId;
        float gas;
        float brake;
        float clutch;
        int gear;
        int rpms;
        float steerAngle;
        float speedKmh;
        float velocity[3];
        float accG[3];
        float wheelAngle[4];
        float slipAngle[4];
        float slipRatio[4];
        float tyreSlip[4];
        float wheelLoad[4];
        float tyrePressure[4];
        float tyreTemperature[4];
        float wheelCombination[4][2];
        float driveDamage[4];
        float tcDamage;
        float vDamage[3];
        int warningLights;
        float time;
        float timeDelta;
        float roadTemperature;
        float ambientTemperature;
        float roadDepth;
        float rainIntensity;
        float windSpeed;
        float windDirection;
    };

    // Constructor/Destructor
    explicit ACSharedMemory(QObject* parent = nullptr);
    ~ACSharedMemory() override;

    // Connection
    bool attach();
    void detach();
    bool isAttached() const { return m_attached; }

    // Data access
    SPageFileStatic getStatic() const { return m_static; }
    SPageFileGraphics getGraphics() const { return m_graphics; }
    SPageFilePhysics getPhysics() const { return m_physics; }

    // Convenience methods
    float getSpeed() const { return m_graphics.speedKmh; }
    int getRPM() const { return m_graphics.rpms; }
    int getGear() const { return m_graphics.gear; }
    float getThrottle() const { return m_graphics.gas; }
    float getBrake() const { return m_graphics.brake; }
    float getSteering() const { return m_graphics.steerAngle; }

    // Tyre data
    float getTyreTemp(int wheel) const;
    float getTyrePressure(int wheel) const;
    float getTyreSlip(int wheel) const;

    // Session info
    float getSessionTime() const { return m_graphics.time; }
    float getTimeDelta() const { return m_graphics.timeDelta; }
    QString getPlayerName() const;
    QString getTrackName() const;
    int getNumCars() const { return m_static.numCars; }

    // Auto-update
    void startAutoUpdate(int intervalMs = 16); // ~60fps
    void stopAutoUpdate();

signals:
    void dataUpdated();
    void connectionStateChanged(bool connected);

private slots:
    void updateData();

private:
    bool mapSharedMemory();
    void unmapSharedMemory();

    void* m_staticMappedFile = nullptr;
    void* m_graphicsMappedFile = nullptr;
    void* m_physicsMappedFile = nullptr;

    void* m_staticPtr = nullptr;
    void* m_graphicsPtr = nullptr;
    void* m_physicsPtr = nullptr;

    SPageFileStatic m_static;
    SPageFileGraphics m_graphics;
    SPageFilePhysics m_physics;

    bool m_attached = false;
    QTimer* m_updateTimer = nullptr;
};

/**
 * @brief AC Telemetry Recorder - Records telemetry data for analysis
 */
class ACTelemetryRecorder : public QObject {
    Q_OBJECT
public:
    struct TelemetrySample {
        float timestamp;
        float speed;
        int rpm;
        int gear;
        float throttle;
        float brake;
        float steering;
        float tyreTemps[4];
        float tyrePressures[4];
        float fuel;
        float lapTime;
    };

    explicit ACTelemetryRecorder(QObject* parent = nullptr);
    ~ACTelemetryRecorder() override;

    bool startRecording(ACSharedMemory* shm);
    bool stopRecording();
    bool isRecording() const { return m_recording; }

    bool saveToFile(const QString& filePath);
    bool loadFromFile(const QString& filePath);

    QVector<TelemetrySample> getSamples() const { return m_samples; }
    float getDuration() const;
    float getMaxSpeed() const;
    float getMaxRPM() const;

private slots:
    void sampleData();

private:
    ACSharedMemory* m_shm = nullptr;
    QTimer* m_sampleTimer = nullptr;
    QVector<TelemetrySample> m_samples;
    bool m_recording = false;
    float m_startTime = 0;
};
