#pragma once

#include "MathTypes.h"
#include "engine/physics/PhysicsCoreTypes.h"
#include <chrono>
#include <string>
#include <vector>
#include <functional>

namespace ks::sim {

struct RaceConfig {
    enum class SessionType { Practice, Qualifying, Race };
    enum class TrackCondition { Dry, Damp, Wet };

    SessionType sessionType = SessionType::Race;
    int totalLaps = 0;
    int sessionTimeSeconds = 0;
    int numCars = 1;
    TrackCondition trackCondition = TrackCondition::Dry;
    float trackTemperature = 25.0f;
    float ambientTemperature = 20.0f;
    float windSpeed = 0.0f;
    float windDirection = 0.0f;

    bool useGridPositions = true;
    bool qualifyingEnabled = false;
    int qualifyingTimeSeconds = 300;
    float gridSpacing = 5.0f;
};

struct Penalty {
    enum class Type { DriveThrough, StopGo, TimeAdded, Disqualification };
    Type type = Type::TimeAdded;
    int targetCarIndex = -1;
    float value = 0;
    std::string reason;
    bool served = false;
};

struct LapTiming {
    int lapNumber = 0;
    float lapTime = 0;
    float bestLapTime = 1e9f;
    float lastLapTime = 0;
    float sectorTimes[3] = {0, 0, 0};
    float currentLapDistance = 0;
    bool valid = true;
    bool trackLimitsViolation = false;
};

struct DriverStanding {
    int carIndex = 0;
    std::string carName;
    std::string driverName;
    int position = 0;
    int gridPosition = 0;
    int currentLap = 0;
    float totalDistance = 0;
    float lastLapTime = 0;
    float bestLapTime = 1e9f;
    float totalTime = 0;
    bool finished = false;
    bool pitlane = false;
    bool disqualified = false;
    std::vector<Penalty> penalties;
};

class RaceSessionManager {
public:
    RaceSessionManager();
    ~RaceSessionManager();

    void configure(const RaceConfig& config);
    void startSession();
    void pauseSession();
    void resumeSession();
    void endSession();
    bool isActive() const { return m_active; }

    void update(const ks::physics::SimulationState& state, float dt);

    float sessionTime() const { return m_sessionTime; }
    float remainingTime() const { return m_remainingTime; }
    int currentLap() const { return m_currentLap; }
    const LapTiming& timing() const { return m_timing; }
    const std::vector<DriverStanding>& standings() const { return m_standings; }

    void setPlayerCarIndex(int idx) { m_playerCarIndex = idx; }

    void setGridPosition(int carIndex, int gridPosition);
    void startCountdown(float countdownSeconds = 5.0f);
    bool isCountingDown() const { return m_countingDown; }
    float countdownValue() const { return m_countdownValue; }

    void addPenalty(int carIndex, Penalty::Type type, float value, const std::string& reason);
    void servePenalty(int carIndex);
    const std::vector<Penalty>& pendingPenalties() const { return m_pendingPenalties; }

    void reportTrackLimitsViolation(int carIndex);

    std::function<void()> onSessionStarted;
    std::function<void()> onSessionPaused;
    std::function<void()> onSessionEnded;
    std::function<void(int, float, float)> onLapCompleted;
    std::function<void(int, float)> onSectorCompleted;
    std::function<void(int)> onPositionChanged;
    std::function<void(float)> onSessionTimeWarning;
    std::function<void(int)> onCountdownTick;
    std::function<void()> onCountdownFinished;
    std::function<void(int, const std::string&, const std::string&)> onPenaltyIssued;
    std::function<void(int, int)> onTrackLimitsWarning;

private:
    void checkLapCrossing(const ks::physics::SimulationState& state);
    void updateStandings();
    void sortStandings();
    void applyPenalties();
    void updateCountdown(float dt);

    RaceConfig m_config;
    bool m_active = false;
    bool m_paused = false;

    std::chrono::steady_clock::time_point m_sessionStart;
    float m_sessionTime = 0;
    float m_remainingTime = 0;
    float m_lapStartTime = 0;

    bool m_countingDown = false;
    float m_countdownValue = 0;
    float m_countdownTimer = 0;
    int m_lastCountdownInt = 0;

    int m_currentLap = 0;
    float m_lastCrossingDistance = 0;
    bool m_crossedLine = false;
    LapTiming m_timing;

    std::vector<Penalty> m_pendingPenalties;
    std::vector<int> m_trackLimitsViolations;

    std::vector<DriverStanding> m_standings;
    int m_playerCarIndex = 0;
};

} // namespace ks::sim
