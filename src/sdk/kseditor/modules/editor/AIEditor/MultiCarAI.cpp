#include "MultiCarAI.h"
#include <QtMath>
#include <algorithm>
#include <cmath>

namespace ks {

MultiCarAI::MultiCarAI()
{
}

void MultiCarAI::setupRace(int numDrivers, int totalLaps, float trackLength)
{
    m_grid.drivers.clear();
    m_grid.totalLaps = totalLaps;
    m_grid.trackLength = trackLength;
    m_grid.leaderLap = 0;
    m_grid.raceActive = true;
    m_grid.raceComplete = false;
    m_raceTime = 0.0f;
    m_fastestLap = 1e9f;
    m_fastestLapDriver = -1;
    m_totalOvertakes = 0;
    m_totalPositionChanges = 0;
    m_lastPositionHash = 0;
    m_pendingEvents.clear();

    for (int i = 0; i < numDrivers; ++i) {
        MultiCarDriver driver;
        driver.id = i;
        driver.name = QString("AI Driver %1").arg(i + 1);
        driver.profile = AiBehaviorModel::getRandomProfile();
        driver.startPosition = i;
        driver.position = i + 1;
        driver.trackProgress = -(i * 2.0f);
        driver.totalLaps = totalLaps;
        driver.energyRecovery = driver.profile.skill * 100.0f;
        m_grid.drivers.append(driver);
    }
    updatePositions();
}

void MultiCarAI::addDriver(const AiBehaviorModel::AiDriverProfile& profile, const QString& name)
{
    MultiCarDriver driver;
    driver.id = m_grid.drivers.size();
    driver.name = name.isEmpty() ? QString("AI Driver %1").arg(driver.id + 1) : name;
    driver.profile = profile;
    driver.startPosition = m_grid.drivers.size();
    driver.position = m_grid.drivers.size() + 1;
    driver.totalLaps = m_grid.totalLaps;
    m_grid.drivers.append(driver);
    updatePositions();
}

void MultiCarAI::clearGrid()
{
    m_grid.drivers.clear();
    m_grid.raceActive = false;
    m_grid.raceComplete = false;
    m_pendingEvents.clear();
}

void MultiCarAI::tick(float deltaTime)
{
    if (!m_grid.raceActive || m_grid.raceComplete) return;
    deltaTime = qMin(deltaTime, 0.05f);

    int positionHashBefore = 0;
    for (const auto& d : m_grid.drivers) positionHashBefore = positionHashBefore * 31 + d.position;

    for (auto& driver : m_grid.drivers) {
        if (driver.finished || driver.dnf) continue;
        applyDraftEffect(driver);
        advanceDriver(driver, deltaTime);
    }

    updateGaps();

    for (auto& driver : m_grid.drivers) {
        if (driver.finished || driver.dnf) continue;
        if (driver.state.needsPitStop || driver.inPits) {
            driver.pitStops++;
            driver.tireWear = 0.0f;
            driver.fuel = 100.0f;
            driver.inPits = false;
            driver.state.needsPitStop = false;

            RaceEvent ev;
            ev.type = RaceEvent::PIT_STOP;
            ev.timestamp = m_raceTime;
            ev.driverId = driver.id;
            ev.description = QString("%1 pitted (stop #%2)").arg(driver.name).arg(driver.pitStops);
            m_pendingEvents.append(ev);
        }
    }

    updatePositions();
    detectOvertakes();

    for (auto& driver : m_grid.drivers) {
        if (driver.finished || driver.dnf) continue;
        checkLapCompletion(driver);
    }

    int positionHashAfter = 0;
    for (const auto& d : m_grid.drivers) positionHashAfter = positionHashAfter * 31 + d.position;
    if (positionHashBefore != positionHashAfter) m_totalPositionChanges++;

    m_raceTime += deltaTime;

    bool allFinished = true;
    for (const auto& d : m_grid.drivers) {
        if (!d.finished && !d.dnf) { allFinished = false; break; }
    }
    if (allFinished) {
        m_grid.raceComplete = true;
        m_grid.raceActive = false;

        RaceEvent ev;
        ev.type = RaceEvent::FINISH;
        ev.timestamp = m_raceTime;
        ev.description = "Race complete";
        m_pendingEvents.append(ev);
    }

    if (m_tickCallback) {
        m_tickCallback(deltaTime, m_grid);
    }
}

QVector<RaceEvent> MultiCarAI::consumeEvents()
{
    auto events = m_pendingEvents;
    m_pendingEvents.clear();
    return events;
}

void MultiCarAI::clearEvents()
{
    m_pendingEvents.clear();
}

int MultiCarAI::getDriverPosition(int driverId) const
{
    for (const auto& d : m_grid.drivers) {
        if (d.id == driverId) return d.position;
    }
    return -1;
}

const MultiCarDriver* MultiCarAI::getDriver(int driverId) const
{
    for (const auto& d : m_grid.drivers) {
        if (d.id == driverId) return &d;
    }
    return nullptr;
}

MultiCarDriver* MultiCarAI::getDriver(int driverId)
{
    for (auto& d : m_grid.drivers) {
        if (d.id == driverId) return &d;
    }
    return nullptr;
}

QVector<MultiCarDriver> MultiCarAI::getLeaderboard() const
{
    QVector<MultiCarDriver> sorted = m_grid.drivers;
    std::sort(sorted.begin(), sorted.end(), [](const MultiCarDriver& a, const MultiCarDriver& b) {
        if (a.finished && !b.finished) return true;
        if (!a.finished && b.finished) return false;
        if (a.dnf && !b.dnf) return false;
        if (!a.dnf && b.dnf) return true;
        if (a.lap != b.lap) return a.lap > b.lap;
        return a.trackProgress > b.trackProgress;
    });
    return sorted;
}

QMap<int, QVector<float>> MultiCarAI::getSectorTimes() const
{
    QMap<int, QVector<float>> result;
    for (const auto& d : m_grid.drivers) {
        result[d.id] = { d.sectorTimes[0], d.sectorTimes[1], d.sectorTimes[2], d.lastLapTime };
    }
    return result;
}

QVector<float> MultiCarAI::getLapTimeHistory(int driverId) const
{
    const auto* d = getDriver(driverId);
    if (!d) return {};
    return d->lapTimes;
}

void MultiCarAI::setDriverProfile(int driverId, const AiBehaviorModel::AiDriverProfile& profile)
{
    auto* d = getDriver(driverId);
    if (d) d->profile = profile;
}

void MultiCarAI::forceDNF(int driverId)
{
    auto* d = getDriver(driverId);
    if (!d || d->finished) return;
    d->dnf = true;

    RaceEvent ev;
    ev.type = RaceEvent::DNF;
    ev.timestamp = m_raceTime;
    ev.driverId = driverId;
    ev.description = QString("%1 did not finish").arg(d->name);
    m_pendingEvents.append(ev);
}

void MultiCarAI::forcePitStop(int driverId)
{
    auto* d = getDriver(driverId);
    if (d) d->state.needsPitStop = true;
}

float MultiCarAI::getEstimatedRaceTime() const
{
    float avgSpeed = 0;
    int count = 0;
    for (const auto& d : m_grid.drivers) {
        if (d.currentSpeed > 0) { avgSpeed += d.currentSpeed; count++; }
    }
    if (count == 0 || m_grid.trackLength <= 0) return 0;
    avgSpeed /= count;
    return (m_grid.trackLength * m_grid.totalLaps) / (avgSpeed * 0.277f) / 60.0f;
}

float MultiCarAI::getAveragePositionChangePerLap() const
{
    int totalLapsCompleted = 0;
    for (const auto& d : m_grid.drivers) totalLapsCompleted = qMax(totalLapsCompleted, d.lap);
    if (totalLapsCompleted <= 0) return 0;
    return static_cast<float>(m_totalPositionChanges) / totalLapsCompleted;
}

void MultiCarAI::advanceDriver(MultiCarDriver& driver, float deltaTime)
{
    AiBehaviorModel::AiRaceState state;
    state.position = driver.position;
    state.totalLaps = driver.totalLaps;
    state.currentLap = driver.lap;
    state.lapTime = driver.lastLapTime;
    state.bestLapTime = driver.bestLapTime;
    state.gapAhead = driver.distanceToCarAhead;
    state.gapBehind = driver.distanceToCarBehind;
    state.fuel = driver.fuel;
    state.tireWear = driver.tireWear;
    state.inPitLane = driver.inPits;
    state.needsPitStop = driver.state.needsPitStop;
    state.isDefending = driver.state.isDefending;
    state.isOvertaking = driver.state.isOvertaking;

    float baseSpeed = calculateSpeedForDriver(driver);
    float speed = baseSpeed;

    if (driver.distanceToCarAhead < 20.0f) {
        auto decision = AiBehaviorModel::calculateDecision(driver.profile, state, speed, driver.distanceToCarAhead);
        driver.state.isOvertaking = decision.isOvertaking;
        driver.state.isDefending = decision.isDefending;
        driver.decisionHistory.append(decision);

        if (decision.isOvertaking) {
            speed *= 1.02f;
        }
        if (decision.shouldPit) {
            driver.state.needsPitStop = true;
        }
    }

    if (driver.distanceToCarBehind < 10.0f) {
        auto defendProb = AiBehaviorModel::calculateDefendProbability(driver.profile, driver.distanceToCarBehind);
        if (defendProb > 0.5f) {
            driver.state.isDefending = true;
            speed *= 0.98f;
        }
    }

    float cornerSlowdown = calculateCornerSlowdown(driver, speed);
    speed -= cornerSlowdown;

    // Recharge energy recovery during cornering/braking
    if (cornerSlowdown > 0) {
        driver.energyRecovery = qMin(100.0f, driver.energyRecovery + cornerSlowdown * 0.05f);
    }
    // Deplete when deploying on straights (roughly inversely proportional to corner time)
    float deployRate = 1.0f - (cornerSlowdown / qMax(1.0f, speed));
    driver.energyRecovery = qMax(0.0f, driver.energyRecovery - deployRate * 0.5f);

    if (driver.tireWear > 60.0f) speed *= (1.0f - (driver.tireWear - 60.0f) / 200.0f);

    float consistencyFactor = 1.0f + (driver.profile.consistency - 0.5f) * 0.1f;
    speed *= consistencyFactor;

    // Rubber-banding: slower cars get a small boost, faster cars get a small penalty
    float rubberBandFactor = 0.0f;
    if (driver.distanceToCarAhead < 50.0f) {
        rubberBandFactor = (1.0f - driver.distanceToCarAhead / 50.0f) * 0.03f;
    } else if (driver.distanceToCarBehind < 50.0f) {
        rubberBandFactor = -(1.0f - driver.distanceToCarBehind / 50.0f) * 0.03f;
    }
    speed *= (1.0f + rubberBandFactor);

    driver.currentSpeed = qMax(40.0f, speed);
    driver.trackProgress += driver.currentSpeed * deltaTime * 0.277f;

    float tireWearRate = (1.0f - driver.profile.tireManagement) * 0.5f;
    float tireLoad = 1.0f + (driver.currentSpeed / 200.0f) * 0.5f;
    driver.tireWear += tireWearRate * tireLoad * deltaTime / 60.0f;
    driver.tireWear = qMin(100.0f, driver.tireWear);

    float fuelConsumption = driver.profile.fuelManagement * 0.5f + 0.5f;
    driver.fuel -= fuelConsumption * deltaTime / 60.0f;
    driver.fuel = qMax(0.0f, driver.fuel);

    if (driver.fuel < 5.0f) {
        driver.state.needsPitStop = true;
    }

    if (AiBehaviorModel::shouldMakeMistake(driver.profile)) {
        float mag = AiBehaviorModel::calculateMistakeMagnitude(driver.profile);
        driver.currentSpeed *= (1.0f - mag * 0.3f);
        driver.trackProgress -= mag * 5.0f;
    }
}

void MultiCarAI::updatePositions()
{
    std::sort(m_grid.drivers.begin(), m_grid.drivers.end(), [](const MultiCarDriver& a, const MultiCarDriver& b) {
        if (a.dnf && !b.dnf) return false;
        if (!a.dnf && b.dnf) return true;
        if (a.finished && !b.finished) return true;
        if (!a.finished && b.finished) return false;
        if (a.lap != b.lap) return a.lap > b.lap;
        return a.trackProgress > b.trackProgress;
    });

    for (int i = 0; i < m_grid.drivers.size(); ++i) {
        m_grid.drivers[i].position = i + 1;
    }
}

void MultiCarAI::detectOvertakes()
{
    for (int i = 0; i < m_grid.drivers.size() - 1; ++i) {
        auto& ahead = m_grid.drivers[i];
        auto& behind = m_grid.drivers[i + 1];
        if (ahead.dnf || behind.dnf || ahead.finished || behind.finished) continue;

        if (ahead.lap == behind.lap && behind.trackProgress > ahead.trackProgress) {
            float delta = behind.trackProgress - ahead.trackProgress;
            if (delta > 5.0f || (behind.state.isOvertaking && delta > 0)) {
                std::swap(ahead.position, behind.position);
                std::swap(ahead.trackProgress, behind.trackProgress);
                m_totalOvertakes++;

                RaceEvent ev;
                ev.type = RaceEvent::OVERTAKE;
                ev.timestamp = m_raceTime;
                ev.driverId = behind.id;
                ev.otherDriverId = ahead.id;
                ev.description = QString("%1 overtook %2").arg(behind.name).arg(ahead.name);
                m_pendingEvents.append(ev);
            }
        }
    }
}

void MultiCarAI::updateGaps()
{
    QVector<MultiCarDriver*> sorted;
    for (auto& d : m_grid.drivers) sorted.append(&d);
    std::sort(sorted.begin(), sorted.end(), [](const MultiCarDriver* a, const MultiCarDriver* b) {
        if (a->dnf && !b->dnf) return false;
        if (!a->dnf && b->dnf) return true;
        if (a->lap != b->lap) return a->lap > b->lap;
        return a->trackProgress > b->trackProgress;
    });

    for (int i = 0; i < sorted.size(); ++i) {
        if (sorted[i]->dnf || sorted[i]->finished) continue;

        float gapAhead = 1e6f;
        for (int j = i - 1; j >= 0; --j) {
            if (sorted[j]->dnf || sorted[j]->finished) continue;
            float lapDiff = static_cast<float>(sorted[j]->lap - sorted[i]->lap);
            gapAhead = (lapDiff * m_grid.trackLength) + (sorted[j]->trackProgress - sorted[i]->trackProgress);
            break;
        }
        sorted[i]->distanceToCarAhead = gapAhead;

        float gapBehind = 1e6f;
        for (int j = i + 1; j < sorted.size(); ++j) {
            if (sorted[j]->dnf || sorted[j]->finished) continue;
            float lapDiff = static_cast<float>(sorted[i]->lap - sorted[j]->lap);
            gapBehind = (lapDiff * m_grid.trackLength) + (sorted[i]->trackProgress - sorted[j]->trackProgress);
            break;
        }
        sorted[i]->distanceToCarBehind = gapBehind;

        if (gapAhead < kCollisionDistance && gapAhead > 0) {
            sorted[i]->currentSpeed *= 0.95f;
        }
    }
}

void MultiCarAI::checkLapCompletion(MultiCarDriver& driver)
{
    if (driver.trackProgress >= m_grid.trackLength) {
        driver.trackProgress -= m_grid.trackLength;
        driver.lap++;

        float lapTime = 0;
        if (driver.lap > 1) {
            lapTime = m_raceTime - (driver.lap - 1) * (m_grid.trackLength / qMax(1.0f, driver.currentSpeed * 0.277f));
        }
        driver.lastLapTime = qMax(0.1f, lapTime);
        driver.lapTimes.append(driver.lastLapTime);

        if (driver.lastLapTime > 0 && driver.lastLapTime < driver.bestLapTime) {
            driver.bestLapTime = driver.lastLapTime;
            if (driver.bestLapTime < m_fastestLap) {
                m_fastestLap = driver.bestLapTime;
                m_fastestLapDriver = driver.id;
            }
        }

        if (driver.lap > m_grid.leaderLap) {
            m_grid.leaderLap = driver.lap;
        }

        // Leader-based race completion: if leader finishes all laps, complete the race
        int leaderIdx = -1;
        for (int i = 0; i < m_grid.drivers.size(); ++i) {
            if (m_grid.drivers[i].position == 1 && !m_grid.drivers[i].dnf) {
                leaderIdx = i;
                break;
            }
        }
        if (leaderIdx >= 0) {
            const auto& leader = m_grid.drivers[leaderIdx];
            if (leader.lap >= leader.totalLaps) {
                // All remaining cars finish when leader does
                for (auto& d : m_grid.drivers) {
                    if (!d.finished && !d.dnf) {
                        d.finished = true;
                        RaceEvent fev;
                        fev.type = RaceEvent::FINISH;
                        fev.timestamp = m_raceTime;
                        fev.driverId = d.id;
                        fev.description = QString("%1 finished in P%2")
                            .arg(d.name).arg(d.position);
                        m_pendingEvents.append(fev);
                    }
                }
            }
        }

        RaceEvent ev;
        ev.type = RaceEvent::LAP_COMPLETED;
        ev.timestamp = m_raceTime;
        ev.driverId = driver.id;
        ev.description = QString("%1 lap %2/%3 (%4s)")
            .arg(driver.name).arg(driver.lap).arg(driver.totalLaps)
            .arg(driver.lastLapTime, 0, 'f', 3);
        m_pendingEvents.append(ev);

        // Fallback: individual finish if leader check didn't catch it
        if (driver.lap >= driver.totalLaps) {
            if (!driver.finished) {
                driver.finished = true;
                RaceEvent fev;
                fev.type = RaceEvent::FINISH;
                fev.timestamp = m_raceTime;
                fev.driverId = driver.id;
                fev.description = QString("%1 finished in P%2").arg(driver.name).arg(driver.position);
                m_pendingEvents.append(fev);
            }
        }
    }
}

void MultiCarAI::applyDraftEffect(MultiCarDriver& driver)
{
    if (driver.distanceToCarAhead < kDraftRange && driver.distanceToCarAhead > 0) {
        float draftFactor = 1.0f - (driver.distanceToCarAhead / kDraftRange);
        driver.currentSpeed *= 1.0f + (draftFactor * (kDraftBoost - 1.0f));
    }
}

float MultiCarAI::calculateSpeedForDriver(const MultiCarDriver& driver) const
{
    float skill = driver.profile.skill;
    float racePace = AiBehaviorModel::calculateRacePace(driver.profile, driver.lap);

    float base = 120.0f + skill * 130.0f;
    float paceFactor = 0.6f + racePace * 0.4f;
    float speed = base * paceFactor;

    float tirePenalty = 0.0f;
    if (driver.tireWear > 50.0f) tirePenalty = (driver.tireWear - 50.0f) * 0.3f;
    speed -= tirePenalty;

    float fuelPenalty = 0.0f;
    if (driver.fuel < 15.0f) fuelPenalty = (15.0f - driver.fuel) * 0.2f;
    speed -= fuelPenalty;

    // Energy recovery boost (deploy on straights when energy is available)
    float ersBoost = driver.energyRecovery / 100.0f * 8.0f;
    speed += ersBoost;

    return qMax(80.0f, speed);
}

float MultiCarAI::calculateCornerSlowdown(const MultiCarDriver& driver, float baseSpeed) const
{
    float cornerFactor = 0.7f + driver.profile.skill * 0.3f;
    float consistencyBonus = driver.profile.consistency * 0.05f;
    float aggressionPenalty = (0.5f - driver.profile.aggression) * 0.05f;
    float wetPenalty = (1.0f - driver.profile.wetSkill) * 0.15f;
    return baseSpeed * (1.0f - cornerFactor + consistencyBonus + aggressionPenalty + wetPenalty) * 0.15f;
}

bool MultiCarAI::checkCollision(const MultiCarDriver& a, const MultiCarDriver& b) const
{
    if (a.lap != b.lap) return false;
    float dist = qAbs(a.trackProgress - b.trackProgress);
    return dist < kCollisionDistance;
}

} // namespace ks
