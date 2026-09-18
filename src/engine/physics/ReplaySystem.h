#pragma once
#include <QVector3D>
#include <QVector>
#include <QString>
#include <QFile>
#include <QDataStream>
#include <QtGlobal>
#include <deque>

namespace ks::physics {

struct ReplayFrame {
    double time = 0;
    QVector3D pos;
    QVector3D rot;
    QVector3D vel;
    float speed = 0, rpm = 0;
    int gear = 1;
    float throttle = 0, brake = 0, steer = 0;
    uint32_t carId = 0;
};

class ReplayRecorder {
public:
    void start(double rateHz = 60.0) { m_recording = true; m_rate = rateHz; m_accum = 0; m_frames.clear(); }
    void stop() { m_recording = false; }
    bool isRecording() const { return m_recording; }
    void push(const ReplayFrame& f, double dt) {
        if (!m_recording) return;
        m_accum += dt;
        double interval = 1.0 / m_rate;
        if (m_accum >= interval) { m_accum = 0; m_frames.push_back(f); if (m_frames.size() > 36000) m_frames.pop_front(); }
    }
    bool save(const QString& path) const {
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly)) return false;
        QDataStream ds(&f);
        ds << quint32(0x4B535250) << quint32(1) << quint32(m_frames.size());
        for (auto& fr : m_frames) {
            ds << fr.time << fr.pos << fr.rot << fr.vel << fr.speed << fr.rpm
               << fr.gear << fr.throttle << fr.brake << fr.steer << fr.carId;
        }
        return true;
    }
    bool load(const QString& path) {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) return false;
        QDataStream ds(&f);
        quint32 magic, ver, n;
        ds >> magic >> ver >> n;
        if (magic != 0x4B535250) return false;
        m_frames.clear();
        for (quint32 i = 0; i < n && !ds.atEnd(); ++i) {
            ReplayFrame fr; ds >> fr.time >> fr.pos >> fr.rot >> fr.vel
                >> fr.speed >> fr.rpm >> fr.gear >> fr.throttle >> fr.brake >> fr.steer >> fr.carId;
            m_frames.push_back(fr);
        }
        return true;
    }
    const std::deque<ReplayFrame>& frames() const { return m_frames; }
private:
    bool m_recording = false;
    double m_rate = 60.0, m_accum = 0;
    std::deque<ReplayFrame> m_frames;
};

class ReplayPlayer {
public:
    void setData(const std::deque<ReplayFrame>& frames) { m_frames = frames; m_t = 0; }
    void play() { m_playing = true; }
    void pause() { m_playing = false; }
    void seek(double t) { m_t = t; }
    bool sample(double time, ReplayFrame& out) const {
        if (m_frames.size() < 2) return false;
        for (size_t i = 0; i + 1 < m_frames.size(); ++i) {
            if (m_frames[i].time <= time && time <= m_frames[i+1].time) {
                double span = m_frames[i+1].time - m_frames[i].time;
                double k = span > 1e-6 ? (time - m_frames[i].time) / span : 0;
                out = m_frames[i];
                out.pos = out.pos * (1 - k) + m_frames[i+1].pos * k;
                out.speed += float((m_frames[i+1].speed - out.speed) * k);
                return true;
            }
        }
        return false;
    }
private:
    std::deque<ReplayFrame> m_frames;
    double m_t = 0;
    bool m_playing = false;
};

} // namespace ks::physics
