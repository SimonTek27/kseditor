#include "RaceSessionManager.h"
#include <algorithm>
#include <cstdio>

namespace ks::sim {

RaceSessionManager::RaceSessionManager() = default;
RaceSessionManager::~RaceSessionManager() = default;

void RaceSessionManager::configure(const RaceConfig& config)
{
    m_config = config;
    m_currentLap = 0;
    m_sessionTime = 0;
    m_timing = LapTiming();
    m_pendingPenalties.clear();
    m_trackLimitsViolations.clear();

    m_standings.clear();
    for (int i = 0; i < config.numCars; ++i) {
        DriverStanding standing;
        standing.carIndex = i;
        standing.carName = "Car " + std::to_string(i + 1);
        standing.gridPosition = i + 1;
        standing.position = i + 1;
        m_standings.push_back(standing);
    }
}

void RaceSessionManager::startSession()
{
    m_active = true;
    m_paused = false;
    m_sessionStart = std::chrono::steady_clock::now();
    m_sessionTime = 0;
    m_lapStartTime = 0;

    if (m_config.sessionTimeSeconds > 0) {
        m_remainingTime = static_cast<float>(m_config.sessionTimeSeconds);
    } else if (m_config.totalLaps > 0) {
        m_remainingTime = 1e9f;
    }

    applyPenalties();

    printf("RaceSessionManager: Session started Laps:%d Time:%ds\n",
           m_config.totalLaps, m_config.sessionTimeSeconds);
    if (onSessionStarted) onSessionStarted();
}

void RaceSessionManager::pauseSession()
{
    if (!m_active || m_paused) return;
    m_paused = true;
    printf("RaceSessionManager: Session paused at %fs\n", m_sessionTime);
    if (onSessionPaused) onSessionPaused();
}

void RaceSessionManager::resumeSession()
{
    if (!m_active || !m_paused) return;
    m_paused = false;
    m_sessionStart = std::chrono::steady_clock::now() - std::chrono::milliseconds(static_cast<long long>(m_sessionTime * 1000));
    printf("RaceSessionManager: Session resumed\n");
}

void RaceSessionManager::endSession()
{
    m_active = false;
    m_paused = false;

    applyPenalties();
    updateStandings();

    printf("RaceSessionManager: Session ended Lap:%d Time:%fs Best:%fs\n",
           m_currentLap, m_sessionTime, m_timing.bestLapTime);

    for (size_t i = 0; i < m_standings.size(); ++i) {
        const auto& s = m_standings[i];
        printf("  P%d %s Laps:%d Best:%fs %s\n",
               s.position, s.driverName.c_str(), s.currentLap, s.bestLapTime,
               s.disqualified ? "[DSQ]" : "");
    }

    if (onSessionEnded) onSessionEnded();
}

void RaceSessionManager::update(const ks::physics::SimulationState& state, float dt)
{
    if (!m_active || m_paused) return;

    updateCountdown(dt);
    if (m_countingDown) return;

    auto now = std::chrono::steady_clock::now();
    m_sessionTime = std::chrono::duration<float>(now - m_sessionStart).count();

    if (m_config.sessionTimeSeconds > 0) {
        m_remainingTime = m_config.sessionTimeSeconds - m_sessionTime;
        if (m_remainingTime <= 0) {
            applyPenalties();
            endSession();
            return;
        }
    }

    m_timing.currentLapDistance = state.currentLapDistance;
    m_timing.lapTime = m_sessionTime - m_lapStartTime;

    if (m_playerCarIndex < (int)m_standings.size()) {
        auto& player = m_standings[m_playerCarIndex];
        player.currentLap = m_currentLap;
        player.totalDistance = state.currentLapDistance + m_currentLap * 1000.0f;
        player.lastLapTime = m_timing.lastLapTime;
        player.bestLapTime = m_timing.bestLapTime;
        player.totalTime = m_sessionTime;
    }

    checkLapCrossing(state);
}

void RaceSessionManager::checkLapCrossing(const ks::physics::SimulationState& state)
{
    float currentDist = state.currentLapDistance;

    if (m_crossedLine && currentDist < 10.0f) {
        m_crossedLine = false;
        m_currentLap++;

        float lapTime = m_sessionTime - m_lapStartTime;
        m_timing.lastLapTime = lapTime;
        m_timing.lapNumber = m_currentLap;

        if (lapTime < m_timing.bestLapTime && lapTime > 1.0f) {
            m_timing.bestLapTime = lapTime;
        }

        if (m_playerCarIndex < (int)m_standings.size()) {
            auto& player = m_standings[m_playerCarIndex];
            player.currentLap = m_currentLap;
            player.lastLapTime = lapTime;
            player.bestLapTime = m_timing.bestLapTime;
            player.totalTime = m_sessionTime;
        }

        printf("RaceSessionManager: Lap %d completed in %fs (best: %fs)\n",
               m_currentLap, lapTime, m_timing.bestLapTime);

        if (onLapCompleted) onLapCompleted(m_currentLap, lapTime, m_timing.bestLapTime);

        if (m_config.totalLaps > 0 && m_currentLap >= m_config.totalLaps) {
            applyPenalties();
            endSession();
            return;
        }

        m_lapStartTime = m_sessionTime;
        m_timing.sectorTimes[0] = 0;
        m_timing.sectorTimes[1] = 0;
        m_timing.sectorTimes[2] = 0;
    }

    if (currentDist > 50.0f) {
        m_crossedLine = true;
    }
}

void RaceSessionManager::updateStandings()
{
    sortStandings();
    for (size_t i = 0; i < m_standings.size(); ++i) {
        int oldPos = m_standings[i].position;
        m_standings[i].position = static_cast<int>(i + 1);
        if (m_standings[i].carIndex == m_playerCarIndex && oldPos != m_standings[i].position) {
            if (onPositionChanged) onPositionChanged(m_standings[i].position);
        }
    }
}

void RaceSessionManager::sortStandings()
{
    std::sort(m_standings.begin(), m_standings.end(),
              [](const DriverStanding& a, const DriverStanding& b) {
                  if (a.disqualified != b.disqualified) return !a.disqualified;
                  if (a.finished != b.finished) return !a.finished;
                  if (a.currentLap != b.currentLap) return a.currentLap > b.currentLap;
                  return a.totalDistance > b.totalDistance;
              });
}

void RaceSessionManager::setGridPosition(int carIndex, int gridPosition)
{
    if (carIndex >= 0 && carIndex < (int)m_standings.size()) {
        m_standings[carIndex].gridPosition = gridPosition;
    }
}

void RaceSessionManager::startCountdown(float countdownSeconds)
{
    m_countingDown = true;
    m_countdownValue = countdownSeconds;
    m_countdownTimer = 0;
    m_lastCountdownInt = static_cast<int>(countdownSeconds) + 1;
    printf("RaceSessionManager: Starting countdown %fs\n", countdownSeconds);
}

void RaceSessionManager::updateCountdown(float dt)
{
    if (!m_countingDown) return;

    m_countdownTimer += dt;
    m_countdownValue = std::max(0.0f, m_countdownValue - dt);

    int currentInt = static_cast<int>(m_countdownValue);
    if (currentInt != m_lastCountdownInt && currentInt >= 0) {
        m_lastCountdownInt = currentInt;
        if (onCountdownTick) onCountdownTick(currentInt);
        printf("RaceSessionManager: Countdown %d\n", currentInt);
    }

    if (m_countdownValue <= 0.0f) {
        m_countingDown = false;
        if (onCountdownFinished) onCountdownFinished();
        printf("RaceSessionManager: Countdown finished - GO!\n");
    }
}

void RaceSessionManager::addPenalty(int carIndex, Penalty::Type type, float value, const std::string& reason)
{
    if (carIndex < 0 || carIndex >= (int)m_standings.size()) return;

    Penalty penalty;
    penalty.type = type;
    penalty.targetCarIndex = carIndex;
    penalty.value = value;
    penalty.reason = reason;
    m_pendingPenalties.push_back(penalty);

    if (onPenaltyIssued) onPenaltyIssued(carIndex, reason, reason);
}

void RaceSessionManager::servePenalty(int carIndex)
{
    for (auto& p : m_pendingPenalties) {
        if (p.targetCarIndex == carIndex && !p.served) {
            p.served = true;
            printf("RaceSessionManager: Penalty served by car %d\n", carIndex);
            break;
        }
    }
}

void RaceSessionManager::reportTrackLimitsViolation(int carIndex)
{
    if (carIndex < 0 || carIndex >= (int)m_standings.size()) return;

    while ((int)m_trackLimitsViolations.size() <= carIndex) {
        m_trackLimitsViolations.push_back(0);
    }
    m_trackLimitsViolations[carIndex]++;

    int violations = m_trackLimitsViolations[carIndex];
    if (onTrackLimitsWarning) onTrackLimitsWarning(carIndex, violations);

    if (violations >= 3) {
        addPenalty(carIndex, Penalty::Type::TimeAdded, 1.0f, "Track limits (3 violations)");
        m_trackLimitsViolations[carIndex] = 0;
    }
}

void RaceSessionManager::applyPenalties()
{
    for (const auto& penalty : m_pendingPenalties) {
        if (penalty.served) continue;
        if (penalty.targetCarIndex < 0 || penalty.targetCarIndex >= (int)m_standings.size()) continue;

        auto& standing = m_standings[penalty.targetCarIndex];

        switch (penalty.type) {
            case Penalty::Type::TimeAdded:
                standing.totalTime += penalty.value;
                break;
            case Penalty::Type::Disqualification:
                standing.disqualified = true;
                break;
            default:
                break;
        }
    }
}

} // namespace ks::sim
