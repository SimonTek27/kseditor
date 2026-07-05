#pragma once

#include "AiBehaviorModel.h"
#include <QString>
#include <QVector>
#include <QMap>
#include <QElapsedTimer>
#include <functional>

namespace ks {

struct MultiCarDriver {
    int id;
    QString name;
    AiBehaviorModel::AiDriverProfile profile;
    AiBehaviorModel::AiRaceState state;

    int position = 0;
    int startPosition = 0;
    int lap = 0;
    int totalLaps = 0;
    float raceTime = 0.0f;
    float sectorTimes[3] = {0, 0, 0};
    float lastLapTime = 0.0f;
    float bestLapTime = 1e9f;

    float currentSpeed = 0.0f;
    float trackProgress = 0.0f;
    float distanceToCarAhead = 1e6f;
    float distanceToCarBehind = 1e6f;

    bool finished = false;
    bool dnf = false;
    bool inPits = false;
    int pitStops = 0;

    float tireWear = 0.0f;
    float fuel = 100.0f;
    float energyRecovery = 0.0f;

    QVector<float> lapTimes;
    QVector<AiBehaviorModel::AiDecision> decisionHistory;
};

struct MultiCarGrid {
    QVector<MultiCarDriver> drivers;
    int totalLaps;
    int leaderLap = 0;
    float trackLength;
    bool raceActive = false;
    bool raceComplete = false;
};

struct RaceEvent {
    enum Type { POSITION_CHANGE, LAP_COMPLETED, OVERTAKE, DNF, PIT_STOP, FINISH, CRASH };
    Type type;
    float timestamp;
    int driverId;
    int otherDriverId = -1;
    QString description;
};

class MultiCarAI {
public:
    MultiCarAI();

    void setupRace(int numDrivers, int totalLaps, float trackLength);
    void addDriver(const AiBehaviorModel::AiDriverProfile& profile, const QString& name = QString());
    void clearGrid();

    void tick(float deltaTime);
    bool isRaceComplete() const { return m_grid.raceComplete; }

    MultiCarGrid& grid() { return m_grid; }
    const MultiCarGrid& grid() const { return m_grid; }

    QVector<RaceEvent> pendingEvents() const { return m_pendingEvents; }
    QVector<RaceEvent> consumeEvents();
    void clearEvents();

    int getDriverPosition(int driverId) const;
    const MultiCarDriver* getDriver(int driverId) const;
    MultiCarDriver* getDriver(int driverId);

    QVector<MultiCarDriver> getLeaderboard() const;
    QMap<int, QVector<float>> getSectorTimes() const;
    QVector<float> getLapTimeHistory(int driverId) const;

    void setDriverProfile(int driverId, const AiBehaviorModel::AiDriverProfile& profile);
    void forceDNF(int driverId);
    void forcePitStop(int driverId);

    float getEstimatedRaceTime() const;
    float getFastestLap() const { return m_fastestLap; }
    int getFastestLapDriver() const { return m_fastestLapDriver; }

    int getTotalOvertakes() const { return m_totalOvertakes; }
    int getTotalPositionChanges() const { return m_totalPositionChanges; }
    float getAveragePositionChangePerLap() const;

    using TickCallback = std::function<void(float deltaTime, const MultiCarGrid&)>;
    void setTickCallback(TickCallback cb) { m_tickCallback = cb; }

    void advanceDriver(MultiCarDriver& driver, float deltaTime);

private:
    void updatePositions();
    void detectOvertakes();
    void updateGaps();
    void checkLapCompletion(MultiCarDriver& driver);
    void applyDraftEffect(MultiCarDriver& driver);
    float calculateSpeedForDriver(const MultiCarDriver& driver) const;
    float calculateCornerSlowdown(const MultiCarDriver& driver, float baseSpeed) const;
    bool checkCollision(const MultiCarDriver& a, const MultiCarDriver& b) const;

    MultiCarGrid m_grid;
    float m_raceTime = 0.0f;
    float m_fastestLap = 1e9f;
    int m_fastestLapDriver = -1;
    int m_totalOvertakes = 0;
    int m_totalPositionChanges = 0;
    int m_lastPositionHash = 0;
    QVector<RaceEvent> m_pendingEvents;
    TickCallback m_tickCallback;

    static constexpr float kDraftRange = 15.0f;
    static constexpr float kDraftBoost = 1.08f;
    static constexpr float kCollisionDistance = 3.0f;
    static constexpr float kPitStopTime = 25.0f;
    static constexpr int kMinLapsForPit = 3;
};

} // namespace ks
