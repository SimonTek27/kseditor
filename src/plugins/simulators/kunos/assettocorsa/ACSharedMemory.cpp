#include "ACSharedMemory.h"
#include <QDebug>
#include <QFile>
#include <QDataStream>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#endif

// ============================================================================
// ACSharedMemory implementation
// ============================================================================

ACSharedMemory::ACSharedMemory(QObject* parent)
    : QObject(parent) {
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &ACSharedMemory::updateData);
}

ACSharedMemory::~ACSharedMemory() {
    detach();
}

bool ACSharedMemory::attach() {
    if (m_attached) return true;

#ifdef _WIN32
    // Map static data
    HANDLE hMapFile = OpenFileMappingA(FILE_MAP_READ, FALSE, "ac_pm_static");
    if (hMapFile) {
        m_staticPtr = MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, sizeof(SPageFileStatic));
        if (m_staticPtr) {
            m_staticMappedFile = hMapFile;
        } else {
            CloseHandle(hMapFile);
        }
    }

    // Map graphics data
    hMapFile = OpenFileMappingA(FILE_MAP_READ, FALSE, "ac_pm_graphic");
    if (hMapFile) {
        m_graphicsPtr = MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, sizeof(SPageFileGraphics));
        if (m_graphicsPtr) {
            m_graphicsMappedFile = hMapFile;
        } else {
            CloseHandle(hMapFile);
        }
    }

    // Map physics data
    hMapFile = OpenFileMappingA(FILE_MAP_READ, FALSE, "ac_pm_physics");
    if (hMapFile) {
        m_physicsPtr = MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, sizeof(SPageFilePhysics));
        if (m_physicsPtr) {
            m_physicsMappedFile = hMapFile;
        } else {
            CloseHandle(hMapFile);
        }
    }

    // Check if at least one mapping succeeded
    if (m_staticPtr || m_graphicsPtr || m_physicsPtr) {
        m_attached = true;
        updateData();
        emit connectionStateChanged(true);
        return true;
    }
#else
    // On non-Windows platforms, shared memory is not directly supported
    // Would need to implement using platform-specific IPC mechanisms
#endif

    m_attached = false;
    emit connectionStateChanged(false);
    return false;
}

void ACSharedMemory::detach() {
    if (!m_attached) return;

#ifdef _WIN32
    if (m_staticPtr) {
        UnmapViewOfFile(m_staticPtr);
        m_staticPtr = nullptr;
    }
    if (m_staticMappedFile) {
        CloseHandle((HANDLE)m_staticMappedFile);
        m_staticMappedFile = nullptr;
    }

    if (m_graphicsPtr) {
        UnmapViewOfFile(m_graphicsPtr);
        m_graphicsPtr = nullptr;
    }
    if (m_graphicsMappedFile) {
        CloseHandle((HANDLE)m_graphicsMappedFile);
        m_graphicsMappedFile = nullptr;
    }

    if (m_physicsPtr) {
        UnmapViewOfFile(m_physicsPtr);
        m_physicsPtr = nullptr;
    }
    if (m_physicsMappedFile) {
        CloseHandle((HANDLE)m_physicsMappedFile);
        m_physicsMappedFile = nullptr;
    }
#endif

    m_attached = false;
    emit connectionStateChanged(false);
}

float ACSharedMemory::getTyreTemp(int wheel) const {
    if (wheel < 0 || wheel > 3) return 0;
    return m_physics.tyreTemperature[wheel];
}

float ACSharedMemory::getTyrePressure(int wheel) const {
    if (wheel < 0 || wheel > 3) return 0;
    return m_physics.tyrePressure[wheel];
}

float ACSharedMemory::getTyreSlip(int wheel) const {
    if (wheel < 0 || wheel > 3) return 0;
    return m_graphics.slipRatio[wheel];
}

QString ACSharedMemory::getPlayerName() const {
    return QString::fromLatin1(m_static.playerName).trimmed();
}

QString ACSharedMemory::getTrackName() const {
    return QString::fromLatin1(m_static.trackName).trimmed();
}

void ACSharedMemory::startAutoUpdate(int intervalMs) {
    m_updateTimer->start(intervalMs);
}

void ACSharedMemory::stopAutoUpdate() {
    m_updateTimer->stop();
}

void ACSharedMemory::updateData() {
    if (!m_attached) return;

#ifdef _WIN32
    if (m_staticPtr) {
        memcpy(&m_static, m_staticPtr, sizeof(SPageFileStatic));
    }
    if (m_graphicsPtr) {
        memcpy(&m_graphics, m_graphicsPtr, sizeof(SPageFileGraphics));
    }
    if (m_physicsPtr) {
        memcpy(&m_physics, m_physicsPtr, sizeof(SPageFilePhysics));
    }
#endif

    emit dataUpdated();
}

bool ACSharedMemory::mapSharedMemory() {
    return attach();
}

void ACSharedMemory::unmapSharedMemory() {
    detach();
}

// ============================================================================
// ACTelemetryRecorder implementation
// ============================================================================

ACTelemetryRecorder::ACTelemetryRecorder(QObject* parent)
    : QObject(parent) {
    m_sampleTimer = new QTimer(this);
    connect(m_sampleTimer, &QTimer::timeout, this, &ACTelemetryRecorder::sampleData);
}

ACTelemetryRecorder::~ACTelemetryRecorder() {
    stopRecording();
}

bool ACTelemetryRecorder::startRecording(ACSharedMemory* shm) {
    if (!shm || !shm->isAttached()) {
        return false;
    }

    m_shm = shm;
    m_samples.clear();
    m_recording = true;
    m_startTime = shm->getSessionTime();

    m_sampleTimer->start(100); // 10 samples per second

    return true;
}

bool ACTelemetryRecorder::stopRecording() {
    m_recording = false;
    m_sampleTimer->stop();
    return true;
}

bool ACTelemetryRecorder::saveToFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    // Write header
    stream << quint32(0x54454C4D); // "TELM" magic
    stream << quint32(1); // version
    stream << quint32(m_samples.size());

    // Write samples
    for (const TelemetrySample& sample : m_samples) {
        stream << sample.timestamp;
        stream << sample.speed;
        stream << sample.rpm;
        stream << sample.gear;
        stream << sample.throttle;
        stream << sample.brake;
        stream << sample.steering;
        for (int i = 0; i < 4; ++i) stream << sample.tyreTemps[i];
        for (int i = 0; i < 4; ++i) stream << sample.tyrePressures[i];
        stream << sample.fuel;
        stream << sample.lapTime;
    }

    file.close();
    return true;
}

bool ACTelemetryRecorder::loadFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    // Read header
    quint32 magic = 0;
    stream >> magic;
    if (magic != 0x54454C4D) {
        return false;
    }

    quint32 version = 0;
    stream >> version;
    if (version != 1) {
        return false;
    }

    quint32 sampleCount = 0;
    stream >> sampleCount;

    m_samples.clear();
    m_samples.reserve(sampleCount);

    for (quint32 i = 0; i < sampleCount; ++i) {
        TelemetrySample sample;
        stream >> sample.timestamp;
        stream >> sample.speed;
        stream >> sample.rpm;
        stream >> sample.gear;
        stream >> sample.throttle;
        stream >> sample.brake;
        stream >> sample.steering;
        for (int t = 0; t < 4; ++t) stream >> sample.tyreTemps[t];
        for (int t = 0; t < 4; ++t) stream >> sample.tyrePressures[t];
        stream >> sample.fuel;
        stream >> sample.lapTime;
        m_samples.append(sample);
    }

    file.close();
    return true;
}

float ACTelemetryRecorder::getDuration() const {
    if (m_samples.isEmpty()) return 0;
    return m_samples.last().timestamp - m_samples.first().timestamp;
}

float ACTelemetryRecorder::getMaxSpeed() const {
    float maxSpeed = 0;
    for (const TelemetrySample& sample : m_samples) {
        if (sample.speed > maxSpeed) maxSpeed = sample.speed;
    }
    return maxSpeed;
}

float ACTelemetryRecorder::getMaxRPM() const {
    float maxRPM = 0;
    for (const TelemetrySample& sample : m_samples) {
        if (sample.rpm > maxRPM) maxRPM = sample.rpm;
    }
    return maxRPM;
}

void ACTelemetryRecorder::sampleData() {
    if (!m_recording || !m_shm) return;

    TelemetrySample sample;
    sample.timestamp = m_shm->getSessionTime() - m_startTime;
    sample.speed = m_shm->getSpeed();
    sample.rpm = m_shm->getRPM();
    sample.gear = m_shm->getGear();
    sample.throttle = m_shm->getThrottle();
    sample.brake = m_shm->getBrake();
    sample.steering = m_shm->getSteering();

    for (int i = 0; i < 4; ++i) {
        sample.tyreTemps[i] = m_shm->getTyreTemp(i);
        sample.tyrePressures[i] = m_shm->getTyrePressure(i);
    }

    sample.fuel = 0; // AC shared memory does not expose fuel level
    sample.lapTime = m_shm->getSessionTime();

    m_samples.append(sample);
}
