#pragma once

#include <QObject>
#include <QVector3D>
#include <QString>

namespace ksEditor {

// ============================================================================
// DrivingAudio - Audio engine for driving simulator
// Uses the existing KSAudioEngine (Qt Multimedia) for sound effects:
// Engine, skid, wind, surface, indicators
// ============================================================================
class DrivingAudio : public QObject {
    Q_OBJECT

public:
    explicit DrivingAudio(QObject* parent = nullptr);
    ~DrivingAudio();

    bool initialize();
    void shutdown();

    bool isInitialized() const { return m_initialized; }

    void setWindDirection(const QVector3D& dir);
    void setWindIntensity(float intensity); // 0.0 - 1.0

    void setGlobalVolume(float volume); // 0.0 - 1.0
    void setEngineVolume(float volume);
    void setEnvironmentVolume(float volume);

    void playEngineSound(float rpm, float throttle);
    void playSkidSound(bool skidding);
    void playWindSound();
    void playSurfaceSound(const QString& surfaceType);

private:
    void stopAllSources();

    bool m_initialized = false;

    // Event IDs from KSAudioEngine
    int m_engineEventId = -1;
    int m_skidEventId = -1;
    int m_windEventId = -1;
    int m_surfaceEventId = -1;

    // State
    float m_currentRPM = 0.0f;
    float m_currentThrottle = 0.0f;
    bool m_isSkidding = false;
    QString m_currentSurface = "asphalt";
    float m_windIntensity = 0.0f;
    QVector3D m_windDirection;

    // Volume
    float m_globalVolume = 1.0f;
    float m_engineVolume = 1.0f;
    float m_environmentVolume = 1.0f;
};

} // namespace ksEditor
