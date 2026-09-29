#pragma once
/**
 * AC-style lap / sector timing (Qt-free).
 * Sector splits as normalized spline positions [0,1].
 */
#include <cstdint>
#include <vector>
#include <algorithm>
#include <cmath>

namespace ks {
namespace physics {

class LapSectorTimer {
public:
    void configure(int sectorCount = 3) {
        m_sectorCount = std::max(1, sectorCount);
        m_sectorSplits.assign(static_cast<size_t>(m_sectorCount), 0.f);
        for (int i = 0; i < m_sectorCount; ++i)
            m_sectorSplits[static_cast<size_t>(i)] =
                static_cast<float>(i) / static_cast<float>(m_sectorCount);
        reset();
    }

    /** Custom splits (must be ascending in [0,1), first usually 0). */
    void setSplits(std::vector<float> splits) {
        if (splits.empty()) return;
        std::sort(splits.begin(), splits.end());
        m_sectorSplits = std::move(splits);
        m_sectorCount = static_cast<int>(m_sectorSplits.size());
        reset();
    }

    void reset() {
        m_lapMs = 0;
        m_lastLapMs = 0;
        m_bestLapMs = 0;
        m_sectorIndex = 0;
        m_completedLaps = 0;
        m_prevSpline = 0.f;
        m_running = false;
        m_sectorTimes.assign(static_cast<size_t>(m_sectorCount), 0);
        m_bestSectors.assign(static_cast<size_t>(m_sectorCount), 0);
    }

    void start() { m_running = true; m_lapMs = 0; m_sectorIndex = 0; }
    void stop() { m_running = false; }

    /**
     * @param dtSec physics step
     * @param normalizedSpline 0..1 along track centerline
     */
    void update(double dtSec, float normalizedSpline) {
        if (!m_running) return;
        m_lapMs += static_cast<int>(dtSec * 1000.0);

        // Lap complete: crossed start/finish (spline wrap)
        if (m_prevSpline > 0.85f && normalizedSpline < 0.15f) {
            onLapComplete();
        } else {
            // Sector boundary
            int next = (m_sectorIndex + 1) % m_sectorCount;
            float split = m_sectorSplits[static_cast<size_t>(next)];
            if (next == 0) {
                // handled by lap wrap
            } else if (m_prevSpline < split && normalizedSpline >= split) {
                m_sectorTimes[static_cast<size_t>(m_sectorIndex)] = m_lapMs;
                if (m_bestSectors[static_cast<size_t>(m_sectorIndex)] == 0 ||
                    m_lapMs < m_bestSectors[static_cast<size_t>(m_sectorIndex)])
                    m_bestSectors[static_cast<size_t>(m_sectorIndex)] = m_lapMs;
                m_sectorIndex = next;
            }
        }
        m_prevSpline = normalizedSpline;
    }

    int currentTimeMs() const { return m_lapMs; }
    int lastTimeMs() const { return m_lastLapMs; }
    int bestTimeMs() const { return m_bestLapMs; }
    int sectorIndex() const { return m_sectorIndex; }
    int completedLaps() const { return m_completedLaps; }
    int sectorCount() const { return m_sectorCount; }
    bool isRunning() const { return m_running; }

private:
    void onLapComplete() {
        m_lastLapMs = m_lapMs;
        if (m_bestLapMs == 0 || m_lapMs < m_bestLapMs)
            m_bestLapMs = m_lapMs;
        m_sectorTimes[static_cast<size_t>(m_sectorIndex)] = m_lapMs;
        ++m_completedLaps;
        m_lapMs = 0;
        m_sectorIndex = 0;
    }

    int m_sectorCount = 3;
    std::vector<float> m_sectorSplits;
    std::vector<int> m_sectorTimes;
    std::vector<int> m_bestSectors;
    int m_lapMs = 0;
    int m_lastLapMs = 0;
    int m_bestLapMs = 0;
    int m_sectorIndex = 0;
    int m_completedLaps = 0;
    float m_prevSpline = 0.f;
    bool m_running = false;
};

} // namespace physics
} // namespace ks
