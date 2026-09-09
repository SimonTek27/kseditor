#include "DrivingAudio.h"
#include "AudioTypes.h"
#include <QDebug>
#include <cmath>

namespace ksEditor {

DrivingAudio::DrivingAudio(QObject* parent) : QObject(parent) {}

DrivingAudio::~DrivingAudio() {
    shutdown();
}

bool DrivingAudio::initialize() {
    if (m_initialized) return true;

    auto* engine = ks::audio::KSAudioEngine::instance();
    if (!engine) {
        qWarning() << "DrivingAudio: ks::audio::KSAudioEngine not available";
        return false;
    }

    m_initialized = true;
    qInfo() << "DrivingAudio: Initialized";
    return true;
}

void DrivingAudio::shutdown() {
    if (!m_initialized) return;

    stopAllSources();
    m_initialized = false;
    qInfo() << "DrivingAudio: Shutdown";
}

void DrivingAudio::setWindDirection(const QVector3D& dir) {
    m_windDirection = dir.normalized();
}

void DrivingAudio::setWindIntensity(float intensity) {
    m_windIntensity = std::clamp(intensity, 0.0f, 1.0f);
}

void DrivingAudio::setGlobalVolume(float volume) {
    m_globalVolume = std::clamp(volume, 0.0f, 1.0f);
    auto* engine = ks::audio::KSAudioEngine::instance();
    if (engine) {
        engine->setMasterVolume(m_globalVolume);
    }
}

void DrivingAudio::setEngineVolume(float volume) {
    m_engineVolume = std::clamp(volume, 0.0f, 1.0f);
}

void DrivingAudio::setEnvironmentVolume(float volume) {
    m_environmentVolume = std::clamp(volume, 0.0f, 1.0f);
}

void DrivingAudio::playEngineSound(float rpm, float throttle) {
    if (!m_initialized) return;

    m_currentRPM = rpm;
    m_currentThrottle = throttle;

    auto* engine = ks::audio::KSAudioEngine::instance();
    if (!engine) return;

    if (m_engineEventId < 0) {
        m_engineEventId = engine->playEvent("engine/loop");
    }

    if (m_engineEventId >= 0) {
        engine->setEventParameter(m_engineEventId, "rpm", rpm);
        engine->setEventParameter(m_engineEventId, "throttle", throttle);
        engine->setEventVolume(m_engineEventId, m_engineVolume * m_globalVolume);
    }
}

void DrivingAudio::playSkidSound(bool skidding) {
    if (!m_initialized) return;

    auto* engine = ks::audio::KSAudioEngine::instance();
    if (!engine) return;

    if (skidding && !m_isSkidding) {
        m_skidEventId = engine->playEvent("surface/skid");
    } else if (!skidding && m_isSkidding) {
        if (m_skidEventId >= 0) {
            engine->stopEvent(m_skidEventId);
            m_skidEventId = -1;
        }
    }

    m_isSkidding = skidding;
}

void DrivingAudio::playWindSound() {
    if (!m_initialized) return;

    auto* engine = ks::audio::KSAudioEngine::instance();
    if (!engine) return;

    if (m_windEventId < 0) {
        m_windEventId = engine->playEvent("environment/wind");
    }

    if (m_windEventId >= 0) {
        engine->setEventParameter(m_windEventId, "intensity", m_windIntensity);
        engine->setEventVolume(m_windEventId, m_environmentVolume * m_globalVolume);
    }
}

void DrivingAudio::playSurfaceSound(const QString& surfaceType) {
    if (!m_initialized) return;
    if (surfaceType == m_currentSurface) return;

    auto* engine = ks::audio::KSAudioEngine::instance();
    if (!engine) return;

    if (m_surfaceEventId >= 0) {
        engine->stopEvent(m_surfaceEventId);
        m_surfaceEventId = -1;
    }

    QString eventPath = "surface/" + surfaceType;
    m_surfaceEventId = engine->playEvent(eventPath);
    m_currentSurface = surfaceType;
}

void DrivingAudio::stopAllSources() {
    auto* engine = ks::audio::KSAudioEngine::instance();
    if (!engine) return;

    if (m_engineEventId >= 0) { engine->stopEvent(m_engineEventId); m_engineEventId = -1; }
    if (m_skidEventId >= 0) { engine->stopEvent(m_skidEventId); m_skidEventId = -1; }
    if (m_windEventId >= 0) { engine->stopEvent(m_windEventId); m_windEventId = -1; }
    if (m_surfaceEventId >= 0) { engine->stopEvent(m_surfaceEventId); m_surfaceEventId = -1; }

    m_isSkidding = false;
}

} // namespace ksEditor
