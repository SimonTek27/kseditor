#pragma once
#include "NetworkConfig.h"
#include <QVector3D>
#include <QVector>
#include <QMap>
#include <QtGlobal>
#include <deque>
#include <cmath>
#include <algorithm>

namespace ks::sim::net {

struct Snapshot {
    uint32_t frame = 0;
    double time = 0;
    QVector<CarStateData> cars;
};

class SnapshotInterp {
public:
    void push(const Snapshot& s) {
        m_buf.push_back(s);
        while (m_buf.size() > 32) m_buf.pop_front();
    }
    bool sample(double renderTime, QVector<CarStateData>& out) const {
        if (m_buf.size() < 2) return false;
        const Snapshot* a = nullptr; const Snapshot* b = nullptr;
        for (size_t i = 0; i + 1 < m_buf.size(); ++i) {
            if (m_buf[i].time <= renderTime && renderTime <= m_buf[i+1].time) {
                a = &m_buf[i]; b = &m_buf[i+1]; break;
            }
        }
        if (!a) {
            if (renderTime > m_buf.back().time) {
                out = m_buf.back().cars;
                extrapolate(out, renderTime - m_buf.back().time);
                return true;
            }
            return false;
        }
        double span = b->time - a->time;
        double t = span > 1e-6 ? (renderTime - a->time) / span : 0;
        t = qBound(0.0, t, 1.0);
        out = a->cars;
        for (int i = 0; i < out.size() && i < b->cars.size(); ++i) {
            const auto& B = b->cars[i];
            out[i].posX += float((B.posX - out[i].posX) * t);
            out[i].posY += float((B.posY - out[i].posY) * t);
            out[i].posZ += float((B.posZ - out[i].posZ) * t);
            out[i].speed += float((B.speed - out[i].speed) * t);
            out[i].rpm += float((B.rpm - out[i].rpm) * t);
        }
        return true;
    }
private:
    void extrapolate(QVector<CarStateData>& cars, double dt) const {
        dt = qBound(0.0, dt, 0.3);
        for (auto& c : cars) {
            c.posX += c.velX * float(dt);
            c.posY += c.velY * float(dt);
            c.posZ += c.velZ * float(dt);
        }
    }
    std::deque<Snapshot> m_buf;
};

enum class SessionStage { Practice, Qualifying, Race, Finished };
struct RaceEntry { uint32_t carId = 0; int gridSlot = 0; int laps = 0; double bestLap = 0; int penalties = 0; bool dq = false; };

class RaceServer {
public:
    SessionStage stage = SessionStage::Practice;
    double sessionTimeLeft = 600.0;
    int totalLaps = 10;

    void setGrid(const QVector<uint32_t>& carIds) {
        m_entries.clear();
        for (int i = 0; i < carIds.size(); ++i) {
            RaceEntry e; e.carId = carIds[i]; e.gridSlot = i;
            m_entries[carIds[i]] = e;
        }
    }
    void update(double dt) {
        if (stage == SessionStage::Finished) return;
        sessionTimeLeft -= dt;
        if (sessionTimeLeft <= 0 || (stage == SessionStage::Race && raceComplete())) advanceStage();
    }
    void reportLap(uint32_t carId, double lapTime, bool cutTrack) {
        auto it = m_entries.find(carId);
        if (it == m_entries.end()) return;
        it->laps++;
        if (it->bestLap <= 0 || lapTime < it->bestLap) it->bestLap = lapTime;
        if (cutTrack) {
            it->penalties++;
            if (it->penalties >= 3) it->dq = true;
        }
    }
    void reportCut(uint32_t carId) {
        auto it = m_entries.find(carId);
        if (it == m_entries.end()) return;
        it->penalties++;
        if (it->penalties >= 3) it->dq = true;
    }
    QVector<RaceEntry> standings() const {
        auto v = m_entries.values().toVector();
        std::sort(v.begin(), v.end(), [](const RaceEntry& a, const RaceEntry& b){
            if (a.dq != b.dq) return !a.dq;
            if (a.laps != b.laps) return a.laps > b.laps;
            return a.bestLap < b.bestLap;
        });
        return v;
    }
private:
    bool raceComplete() const {
        for (auto e : m_entries) if (e.laps >= totalLaps && !e.dq) return true;
        return false;
    }
    void advanceStage() {
        if (stage == SessionStage::Practice) { stage = SessionStage::Qualifying; sessionTimeLeft = 600; }
        else if (stage == SessionStage::Qualifying) { stage = SessionStage::Race; sessionTimeLeft = 3600; }
        else stage = SessionStage::Finished;
    }
    QMap<uint32_t, RaceEntry> m_entries;
};

struct PredictedInput { uint32_t frame = 0; InputData input; QVector3D pos; float speed = 0; };

class ClientPrediction {
public:
    void pushLocal(uint32_t frame, const InputData& in, const QVector3D& pos, float speed) {
        m_history.push_back({frame, in, pos, speed});
        while (m_history.size() > 120) m_history.pop_front();
    }
    QVector3D reconcile(uint32_t serverFrame, const QVector3D& serverPos, float& correctionOut) {
        for (auto it = m_history.begin(); it != m_history.end(); ++it) {
            if (it->frame == serverFrame) {
                QVector3D err = serverPos - it->pos;
                float len = err.length();
                correctionOut = len;
                if (len > 0.5f) {
                    m_history.erase(m_history.begin(), it + 1);
                    return serverPos;
                }
                m_history.erase(m_history.begin(), it + 1);
                correctionOut = 0;
                return QVector3D();
            }
        }
        correctionOut = 0;
        return QVector3D();
    }
private:
    std::deque<PredictedInput> m_history;
};

} // namespace ks::sim::net
