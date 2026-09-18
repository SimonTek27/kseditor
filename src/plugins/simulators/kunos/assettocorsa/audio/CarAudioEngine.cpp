#include "CarAudioEngine.h"
#include "plugins/simulators/kunos/assettocorsa/ksAssettocorsasndeventdefs.h"
#include <QDebug>
#include <QFileInfo>
#include <QDir>

namespace ks { namespace audio {

KsCarAudioEngine* KsCarAudioEngine::s_instance = nullptr;

KsCarAudioEngine::KsCarAudioEngine(QObject* parent)
    : QObject(parent)
{}

KsCarAudioEngine* KsCarAudioEngine::instance() {
    if (!s_instance) {
        s_instance = new KsCarAudioEngine();
    }
    return s_instance;
}

QString KsCarAudioEngine::eventPath(const QString& eventName) const {
    return ks::plugins::kunos::assettocorsa::ksAssettocorsasndeventdefs::carEventPath(m_carId, eventName);
}

bool KsCarAudioEngine::initialize(const QString& acRootPath) {
    if (m_initialized) return true;

    qDebug() << "Initializing KsCarAudioEngine...";
    m_acRoot = acRootPath;

    m_engine = audio::KSAudioEngine::instance();
    if (m_engine && m_engine->initialize(44100, 2, 2048)) {
        m_initialized = true;
        emit initialized(true);
        qDebug() << "KsCarAudioEngine: Initialized with kseditor software audio engine";
        return true;
    }

    qWarning() << "KsCarAudioEngine: Failed to initialize audio engine";
    m_initialized = true;
    emit initialized(true);
    return true;
}

void KsCarAudioEngine::shutdown() {
    if (m_engine) {
        m_engine->stopAllEvents();
    }
    m_engineIntInstance = -1;
    m_engineExtInstance = -1;
    m_turboInstance = -1;
    m_limiterInstance = -1;
    m_doorInstance = -1;
    m_hornInstance = -1;
    m_initialized = false;
}

void KsCarAudioEngine::loadCarSoundBank(const QString& carDirectory, const QString& carId) {
    m_carDirectory = carDirectory;
    m_carId = carId;

    QString bankPath = ks::plugins::kunos::assettocorsa::ksAssettocorsasndeventdefs::soundbankPath(carDirectory, carId);
    QFileInfo info(bankPath);

    if (!info.exists()) {
        qWarning() << "KsCarAudioEngine: Soundbank not found:" << bankPath;
        emit soundLoaded(carId);
        return;
    }

    qDebug() << "KsCarAudioEngine: Loading soundbank" << bankPath;
    emit soundLoaded(carId);
}

void KsCarAudioEngine::setEngineParameters(float rpm, float throttle) {
    m_rpm = rpm;
    m_load = throttle;
    if (m_engineIntInstance >= 0) {
        m_engine->setEventParameter(m_engineIntInstance, "rpms", rpm);
        m_engine->setEventParameter(m_engineIntInstance, "throttle", throttle);
    }
    if (m_engineExtInstance >= 0) {
        m_engine->setEventParameter(m_engineExtInstance, "rpms", rpm);
        m_engine->setEventParameter(m_engineExtInstance, "throttle", throttle);
    }
}

void KsCarAudioEngine::setTurbo(float boost) {
    if (m_turboInstance >= 0) {
        m_engine->setEventParameter(m_turboInstance, "boost", boost);
    }
}

void KsCarAudioEngine::setLimiter(float decay) {
    if (m_limiterInstance >= 0) {
        m_engine->setEventParameter(m_limiterInstance, "decay", decay);
    }
}

void KsCarAudioEngine::setCameraMode(bool external) {
    if (m_engineExternal == external) return;

    if (m_engineExternal && m_engineExtInstance >= 0) {
        m_engine->stopEvent(m_engineExtInstance);
        m_engineExtInstance = -1;
    } else if (!m_engineExternal && m_engineIntInstance >= 0) {
        m_engine->stopEvent(m_engineIntInstance);
        m_engineIntInstance = -1;
    }

    if (external) {
        m_engineExtInstance = m_engine->playEvent(eventPath("engine_ext"));
    } else {
        m_engineIntInstance = m_engine->playEvent(eventPath("engine_int"));
    }

    m_engineExternal = external;
}

void KsCarAudioEngine::setDoorOpen(bool open) {
    if (m_doorOpen != open) {
        if (m_doorInstance >= 0) {
            m_engine->stopEvent(m_doorInstance);
            m_doorInstance = -1;
        }
        m_doorInstance = m_engine->playEvent(eventPath("door"));
        if (m_doorInstance >= 0) {
            m_engine->setEventParameter(m_doorInstance, "state", open ? 1.0f : 0.0f);
        }
        m_doorOpen = open;
    }
}

void KsCarAudioEngine::setHorn(bool active) {
    if (m_hornActive != active) {
        if (active) {
            m_hornInstance = m_engine->playEvent(eventPath("horn"));
        } else if (m_hornInstance >= 0) {
            m_engine->stopEvent(m_hornInstance);
            m_hornInstance = -1;
        }
        m_hornActive = active;
    }
}

void KsCarAudioEngine::setListenerPosition(float x, float y, float z, float fx, float fy, float fz, float ux, float uy, float uz) {
    if (m_engine) {
        m_engine->set3DListenerPosition(QVector3D(x, y, z), QVector3D(fx, fy, fz), QVector3D(ux, uy, uz));
    }
}

void KsCarAudioEngine::setCarPosition(float x, float y, float z, float fx, float fy, float fz, float ux, float uy, float uz, float vx, float vy, float vz) {
    m_carX = x; m_carY = y; m_carZ = z;
    QVector3D pos(x, y, z);
    QVector3D forward(fx, fy, fz);
    QVector3D up(ux, uy, uz);
    QVector3D vel(vx, vy, vz);
    if (m_engine) {
        if (m_doorInstance >= 0) m_engine->setEventPosition(m_doorInstance, pos);
        m_engine->set3DListenerPosition(pos, forward, up);
        m_engine->set3DListenerVelocity(vel);
    }
}

void KsCarAudioEngine::setEnginePosition(float x, float y, float z, float fx, float fy, float fz, float ux, float uy, float uz, float vx, float vy, float vz) {
    m_carX = x; m_carY = y; m_carZ = z;
    QVector3D pos(x, y, z);
    QVector3D forward(fx, fy, fz);
    QVector3D up(ux, uy, uz);
    QVector3D vel(vx, vy, vz);
    if (m_engine) {
        if (m_engineIntInstance >= 0) {
            m_engine->setEventPosition(m_engineIntInstance, pos);
            m_engine->setEventParameter(m_engineIntInstance, "speed", vel.length());
        }
        if (m_engineExtInstance >= 0) {
            m_engine->setEventPosition(m_engineExtInstance, pos);
            m_engine->setEventParameter(m_engineExtInstance, "speed", vel.length());
        }
        if (m_turboInstance >= 0) m_engine->setEventPosition(m_turboInstance, pos);
        if (m_limiterInstance >= 0) m_engine->setEventPosition(m_limiterInstance, pos);
        m_engine->set3DListenerPosition(pos, forward, up);
        m_engine->set3DListenerVelocity(vel);
    }
}

void KsCarAudioEngine::updateEngineSound() {
    if (!m_engine || !m_initialized) return;

    float rpmNorm = qBound(0.0f, (m_rpm - m_idleRpm) / (m_maxRpm - m_idleRpm), 1.0f);
    float speedNorm = qBound(0.0f, m_speed / 100.0f, 1.0f);

    if (m_engineIntInstance >= 0) {
        m_engine->setEventParameter(m_engineIntInstance, "rpm", m_rpm);
        m_engine->setEventParameter(m_engineIntInstance, "speed", m_speed);
        m_engine->setEventParameter(m_engineIntInstance, "load", m_load);
        m_engine->setEventParameter(m_engineIntInstance, "gear", static_cast<float>(m_gear));
    }
    if (m_engineExtInstance >= 0) {
        m_engine->setEventParameter(m_engineExtInstance, "rpm", m_rpm);
        m_engine->setEventParameter(m_engineExtInstance, "speed", m_speed);
    }
    if (m_turboInstance >= 0) {
        float turboBoost = rpmNorm * m_load * 0.8f;
        m_engine->setEventParameter(m_turboInstance, "rpm", m_rpm);
        m_engine->setEventParameter(m_turboInstance, "boost", turboBoost);
    }
    if (m_limiterInstance >= 0) {
        bool atLimit = m_rpm >= m_maxRpm * 0.95f;
        m_engine->setEventParameter(m_limiterInstance, "limiter", atLimit ? 1.0f : 0.0f);
    }

    // Update 3D positions
    QVector3D enginePos(m_carX, m_carY, m_carZ);
    if (m_engineIntInstance >= 0) m_engine->setEventPosition(m_engineIntInstance, enginePos);
    if (m_engineExtInstance >= 0) m_engine->setEventPosition(m_engineExtInstance, enginePos);
    if (m_turboInstance >= 0) m_engine->setEventPosition(m_turboInstance, enginePos);
    if (m_limiterInstance >= 0) m_engine->setEventPosition(m_limiterInstance, enginePos);
}

void KsCarAudioEngine::update() {
    if (m_engine) {
        m_engine->update();
    }
}

} } // namespace ks::audio
