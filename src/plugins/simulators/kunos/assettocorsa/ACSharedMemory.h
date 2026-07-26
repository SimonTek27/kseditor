#pragma once

#include <QString>
#include <QByteArray>
#include <QVector>
#include <QTimer>
#include <QObject>

#include "ksAssettoCorsa_export.h"

/**
 * @brief Assetto Corsa Shared Memory Reader
 *
 * Reads real-time telemetry data from Assetto Corsa's shared memory.
 * Based on AC SDK (ac_sdk.h) and sim_info.py from AC Python documentation.
 *
 * The shared memory structures provide:
 * - Physics data (speed, RPM, throttle, brake, gear, etc.)
 * - Graphics data (camera, weather, time)
 * - Static data (car info, track info, driver info)
 */
class KS_ASSETTOCORSA_API ACSharedMemory : public QObject {
    Q_OBJECT
public:
    // Shared memory structures — byte-exact layout matching AC's ac_sdk.h
    struct SPageFileStatic {
        char acVersion[32];         // 0
        char acGameName[32];        // 32
        int sectorCount;            // 64
        int maxCars;                // 68
        char playerName[64];        // 72
        char playerTeam[64];        // 136
        char playerGuid[64];        // 200
        int playerSkinIndex;        // 264
        int playerClassIndex;       // 268
        int numCars;                // 272
        int playerCarIndex;         // 276
        int trackLength;            // 280
        char trackName[64];         // 284
        char trackConfig[32];       // 348
        float durationTime;         // 380
        // Total: 384 bytes
    };

    struct SPageFileGraphics {
        int packetId;               // 0
        float gas;                  // 4
        float brake;                // 8
        float clutch;               // 12
        int gear;                   // 16
        int rpms;                   // 20
        float steerAngle;           // 24
        float speedKmh;             // 28
        float velocity[3];          // 32
        float accG[3];              // 44
        float wheelSlip[4];         // 56
        float wheelLoad[4];         // 72
        float wheelPressure[4];     // 88
        float wheelTemperature[4];  // 104 — core temp per wheel
        float wheelWear[4];         // 120
        float wheelDirtyLevel[4];   // 136
        float suspensionTravel[4];  // 152
        float camberRAD[4];         // 168
        float casterRAD[4];         // 184
        float coilovers[4];         // 200
        float fuel;                 // 216
        float fuelMax;              // 220
        float bestLap;              // 224
        float lastLap;              // 228
        int rpm;                    // 232 — duplicate of rpms
        float maxRpm;               // 236
        int gearCount;              // 240
        int drsEnabled;             // 244
        int tcEnabled;              // 248
        int absEnabled;             // 252
        float tcCut;                // 256
        float engineLimiter;        // 260
        float engineCut;            // 264
        float aeroDamage;           // 268
        float engineDamage;         // 272
        float brakeDamage[4];       // 276
        float waterTemperature;     // 292
        float oilTemperature;       // 296
        float oilPressure;          // 300
        int warningLights;          // 304
        float time;                 // 308
        float timeDelta;            // 312
        int performanceMeter;       // 316
        int revLimiterAnalog;       // 320
        float turboBoost;           // 324
        int ignitionOn;             // 328
        int starterOn;              // 332
        int engineRunning;          // 336
        float egoExhaust;           // 340
        int brakeLights;            // 344
        int highBeams;              // 348
        int lowBeams;               // 352
        int fogLights;              // 356
        int hazardLights;           // 360
        int turnSignalLeft;         // 364
        int turnSignalRight;        // 368
        int wiperLevel;             // 372
        int horn;                   // 376
        int headlightsOn;           // 380
        float roadTemperature;      // 384
        float ambientTemperature;   // 388
        float rainIntensity;        // 392
        float windSpeed;            // 396
        float windDirection;        // 400
        float tyreCoreTemperature[4]; // 404
        float tyreSurfaceTemperature[4]; // 420
        // Total: ~436 bytes
    };

    struct SPageFilePhysics {
        int packetId;               // 0
        float gas;                  // 4
        float brake;                // 8
        float clutch;               // 12
        int gear;                   // 16
        int rpms;                   // 20
        float steerAngle;           // 24
        float speedKmh;             // 28
        float velocity[3];          // 32
        float accG[3];              // 44
        float wheelSlip[4];         // 56
        float wheelLoad[4];         // 72
        float wheelPressure[4];     // 88
        float wheelTemperature[4];  // 104 — core temp per wheel
        float wheelWear[4];         // 120
        float wheelDirtyLevel[4];   // 136
        float suspensionTravel[4];  // 152
        float camberRAD[4];         // 168
        float casterRAD[4];         // 184
        float coilovers[4];         // 200
        float fuel;                 // 216
        float fuelMax;              // 220
        float bestLap;              // 224
        float lastLap;              // 228
        int rpm;                    // 232 — duplicate of rpms
        float maxRpm;               // 236
        int gearCount;              // 240
        int drsEnabled;             // 244
        int tcEnabled;              // 248
        int absEnabled;             // 252
        float tcCut;                // 256
        float engineLimiter;        // 260
        float engineCut;            // 264
        float aeroDamage;           // 268
        float engineDamage;         // 272
        float brakeDamage[4];       // 276
        float waterTemperature;     // 292
        float oilTemperature;       // 296
        float oilPressure;          // 300
        int warningLights;          // 304
        float time;                 // 308
        float timeDelta;            // 312
        int performanceMeter;       // 316
        int revLimiterAnalog;       // 320
        float turboBoost;           // 324
        int ignitionOn;             // 328
        int starterOn;              // 332
        int engineRunning;          // 336
        float egoExhaust;           // 340
        int brakeLights;            // 344
        int highBeams;              // 348
        int lowBeams;               // 352
        int fogLights;              // 356
        int hazardLights;           // 360
        int turnSignalLeft;         // 364
        int turnSignalRight;        // 368
        int wiperLevel;             // 372
        int horn;                   // 376
        int headlightsOn;           // 380
        float roadTemperature;      // 384
        float ambientTemperature;   // 388
        float rainIntensity;        // 392
        float windSpeed;            // 396
        float windDirection;        // 400
        float tyreCoreTemperature[4]; // 404
        float tyreSurfaceTemperature[4]; // 420
        float enginePower;          // 436
        float batteryCharge;        // 440
        float boostPressure;        // 444
    };

    // Constructor/Destructor
    explicit ACSharedMemory(QObject* parent = nullptr);
    ~ACSharedMemory() override;

    // Connection
    bool attach();
    void detach();
    bool isAttached() const { return m_attached; }
    bool waitForConnection(int timeoutMs = 5000);

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
    float getTyreSurfaceTemp(int wheel) const;
    float getTyrePressure(int wheel) const;
    float getTyreSlip(int wheel) const;
    float getTyreWear(int wheel) const;
    float getTyreDirtyLevel(int wheel) const;

    // Car state data
    float getFuel() const { return m_physics.fuel; }
    float getFuelMax() const { return m_physics.fuelMax; }
    float getMaxRpm() const { return m_physics.maxRpm; }
    float getTurboBoost() const { return m_physics.turboBoost; }
    float getBestLap() const { return m_physics.bestLap; }
    float getLastLap() const { return m_physics.lastLap; }
    int getDrsEnabled() const { return m_physics.drsEnabled; }
    int getTcEnabled() const { return m_physics.tcEnabled; }
    int getAbsEnabled() const { return m_physics.absEnabled; }
    float getAeroDamage() const { return m_physics.aeroDamage; }
    float getEngineDamage() const { return m_physics.engineDamage; }
    float getWaterTemperature() const { return m_physics.waterTemperature; }
    float getOilTemperature() const { return m_physics.oilTemperature; }
    float getEnginePower() const { return m_physics.enginePower; }
    bool isEngineRunning() const { return m_physics.engineRunning != 0; }
    int getGearCount() const { return m_physics.gearCount; }
    int getIgnitionOn() const { return m_physics.ignitionOn; }
    int getStarterOn() const { return m_physics.starterOn; }

    // Session info
    float getSessionTime() const { return m_graphics.time; }
    float getTimeDelta() const { return m_graphics.timeDelta; }
    QString getPlayerName() const;
    QString getTrackName() const;
    int getNumCars() const { return m_static.numCars; }

    // Weather
    float getRoadTemperature() const { return m_physics.roadTemperature; }
    float getAmbientTemperature() const { return m_physics.ambientTemperature; }
    float getRainIntensity() const { return m_physics.rainIntensity; }
    float getWindSpeed() const { return m_physics.windSpeed; }
    float getWindDirection() const { return m_physics.windDirection; }

    // Auto-update
    void startAutoUpdate(int intervalMs = 16); // ~60fps
    void stopAutoUpdate();
    bool isAutoUpdating() const { return m_updateTimer->isActive(); }

    // Raw pointer access for field-level reads
    const void* graphicsPtr() const { return m_graphicsPtr; }
    const void* physicsPtr() const { return m_physicsPtr; }
    const void* staticPtr() const { return m_staticPtr; }

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
    int m_retryCount = 0;
    QTimer* m_updateTimer = nullptr;
};

/**
 * @brief AC Telemetry Recorder - Records telemetry data for analysis
 */
class KS_ASSETTOCORSA_API ACTelemetryRecorder : public QObject {
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
