#include "PhysicsProfiler.h"
#include <QDebug>
#include <algorithm>
#include <numeric>

namespace ks {
namespace physics {

// ============================================================================
// Singleton
// ============================================================================

PhysicsProfiler& PhysicsProfiler::instanceRef() {
    static PhysicsProfiler instance;
    return instance;
}

PhysicsProfiler* PhysicsProfiler::instance() {
    return &instanceRef();
}

PhysicsProfiler::PhysicsProfiler(QObject* parent)
    : QObject(parent)
{
    m_frameTimer.start();
}

// ============================================================================
// Frame management
// ============================================================================

void PhysicsProfiler::beginFrame() {
    if (!m_enabled) return;

    QMutexLocker locker(&m_mutex);
    m_frameTimer.restart();
}

void PhysicsProfiler::endFrame() {
    if (!m_enabled) return;

    QMutexLocker locker(&m_mutex);

    qint64 frameTime = m_frameTimer.nsecsElapsed();
    m_lastFrameMs = frameTime / 1000000.0;
    m_totalFrameMs += m_lastFrameMs;
    m_peakFrameMs = std::max(m_peakFrameMs, m_lastFrameMs);
    m_frameCount++;

    FrameStats stats;
    stats.frameTime = frameTime;
    stats.sectionsCount = m_sectionStats.size();
    m_frameHistory.append(stats);

    trimHistory();

    emit profileUpdated(m_lastFrameMs, fps());
    emit frameCompleted(frameTime);

    // Check for bottleneck (any subsystem > 40% of frame)
    for (int i = 0; i < Total; ++i) {
        double total = m_subSamples[i].totalMs;
        if (total == 0.0) continue;
        double pct = m_totalFrameMs > 0 ? (total / m_totalFrameMs) * 100.0 : 0.0;
        if (pct > 40.0) {
            emit bottleneckDetected(subsystemName(static_cast<Subsystem>(i)), pct);
        }
    }
}

// ============================================================================
// String-based section profiling
// ============================================================================

void PhysicsProfiler::beginSection(const QString& name) {
    if (!m_enabled) return;

    QMutexLocker locker(&m_mutex);
    m_sectionTimers[name].start();
}

void PhysicsProfiler::endSection(const QString& name) {
    if (!m_enabled) return;

    QMutexLocker locker(&m_mutex);

    auto it = m_sectionTimers.find(name);
    if (it != m_sectionTimers.end()) {
        qint64 time = it->nsecsElapsed();
        updateSectionStats(name, time);
        emit sectionCompleted(name, time);
        m_sectionTimers.erase(it);
    }
}

QMap<QString, PhysicsProfiler::SectionStats> PhysicsProfiler::getSectionStats() const {
    QMutexLocker locker(&m_mutex);
    return m_sectionStats;
}

// ============================================================================
// Enum-based subsystem profiling
// ============================================================================

void PhysicsProfiler::beginSubsystem(Subsystem s) {
    if (!m_enabled) return;

    QMutexLocker locker(&m_mutex);
    int idx = static_cast<int>(s);
    if (idx >= 0 && idx < Total) {
        m_subTimers[idx].start();
        m_subTimerActive[idx] = true;
    }
}

void PhysicsProfiler::endSubsystem(Subsystem s) {
    if (!m_enabled) return;

    QMutexLocker locker(&m_mutex);

    int idx = static_cast<int>(s);
    if (idx >= 0 && idx < Total && m_subTimerActive[idx]) {
        qint64 time = m_subTimers[idx].nsecsElapsed();
        updateSubsystemStats(s, time);
        m_subTimerActive[idx] = false;
    }
}

double PhysicsProfiler::subsystemTimeMs(Subsystem s) const {
    QMutexLocker locker(&m_mutex);
    int idx = static_cast<int>(s);
    if (idx < 0 || idx >= Total) return 0.0;
    return m_subSamples[idx].totalMs;
}

double PhysicsProfiler::subsystemPercent(Subsystem s) const {
    QMutexLocker locker(&m_mutex);
    int idx = static_cast<int>(s);
    if (idx < 0 || idx >= Total) return 0.0;
    double total = m_subSamples[idx].totalMs;
    if (total == 0.0) return 0.0;
    return m_totalFrameMs > 0 ? (total / m_totalFrameMs) * 100.0 : 0.0;
}

QString PhysicsProfiler::subsystemName(Subsystem s) const {
    static const QStringList names = {
        "Engine", "Drivetrain", "Differential", "Brakes", "ABS/TC",
        "Aero", "WeightTransfer", "PerWheelForces", "VehicleDynamics",
        "Fuel", "TireThermal", "LapTimer", "ERS/Hybrid", "DRS",
        "DamageModel", "WeatherPhysics"
    };
    int idx = static_cast<int>(s);
    return (idx >= 0 && idx < names.size()) ? names[idx] : "Unknown";
}

QVector<QPair<QString, double>> PhysicsProfiler::allSubsystemTimes() const {
    QMutexLocker locker(&m_mutex);
    QVector<QPair<QString, double>> result;
    for (int i = 0; i < Total; ++i) {
        double t = m_subSamples[i].totalMs;
        if (t > 0) result.append({subsystemName(static_cast<Subsystem>(i)), t});
    }
    return result;
}

QVariantList PhysicsProfiler::allSubsystemPercentages() const {
    QMutexLocker locker(&m_mutex);
    QVector<QPair<QString, double>> sorted;
    for (int i = 0; i < Total; ++i) {
        double total = m_subSamples[i].totalMs;
        if (total == 0.0) continue;
        double pct = m_totalFrameMs > 0 ? (total / m_totalFrameMs) * 100.0 : 0.0;
        if (pct > 0) {
            sorted.append({subsystemName(static_cast<Subsystem>(i)), pct});
        }
    }
    std::sort(sorted.begin(), sorted.end(),
              [](const QPair<QString, double>& a, const QPair<QString, double>& b) {
                  return a.second > b.second;
              });
    QVariantList result;
    for (const auto& pair : sorted) {
        QVariantMap item;
        item["key"] = pair.first;
        item["value"] = pair.second;
        result.append(item);
    }
    return result;
}

QVariantList PhysicsProfiler::allSubsystemTimesList() const {
    QMutexLocker locker(&m_mutex);
    QVector<QPair<QString, double>> sorted;
    for (int i = 0; i < Total; ++i) {
        double t = m_subSamples[i].totalMs;
        if (t > 0) {
            sorted.append({subsystemName(static_cast<Subsystem>(i)), t});
        }
    }
    std::sort(sorted.begin(), sorted.end(),
              [](const QPair<QString, double>& a, const QPair<QString, double>& b) {
                  return a.second > b.second;
              });
    QVariantList result;
    for (const auto& pair : sorted) {
        QVariantMap item;
        item["key"] = pair.first;
        item["value"] = pair.second;
        result.append(item);
    }
    return result;
}

// ============================================================================
// Configuration
// ============================================================================

void PhysicsProfiler::reset() {
    QMutexLocker locker(&m_mutex);
    m_sectionStats.clear();
    m_sectionTimers.clear();
    m_subTimerActive.fill(false);
    m_subSamples.fill(ProfileSample{});
    m_frameHistory.clear();
    m_frameCount = 0;
    m_totalFrameMs = 0;
    m_lastFrameMs = 0;
    m_peakFrameMs = 0;
    emit statsReset();
}

void PhysicsProfiler::setEnabled(bool e) {
    if (m_enabled != e) {
        m_enabled = e;
        emit enabledChanged();
    }
}

// ============================================================================
// Internal
// ============================================================================

void PhysicsProfiler::updateSectionStats(const QString& name, qint64 time) {
    auto& stats = m_sectionStats[name];
    stats.totalTime += time;
    stats.maxTime = std::max(stats.maxTime, time);
    stats.minTime = std::min(stats.minTime, time);
    stats.count++;
}

void PhysicsProfiler::updateSubsystemStats(Subsystem s, qint64 time) {
    double ms = time / 1000000.0;
    int idx = static_cast<int>(s);
    if (idx < 0 || idx >= Total) return;
    auto& sample = m_subSamples[idx];
    sample.totalMs += ms;
    sample.minMs = std::min(sample.minMs, ms);
    sample.maxMs = std::max(sample.maxMs, ms);
    sample.count++;
}

void PhysicsProfiler::trimHistory() {
    while (m_frameHistory.size() > m_maxHistorySize) {
        m_frameHistory.removeFirst();
    }
}

} // namespace physics
} // namespace ks
