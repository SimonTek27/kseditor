#pragma once
#include <QVector3D>
#include <QVector>
#include <QtGlobal>
#include <cmath>
#include <algorithm>

namespace ks::ai {

struct AITarget { QVector3D pos; float speed = 20.0f; float curvature = 0.0f; };

struct AIInputs {
    QVector3D pos;
    QVector3D vel;
    float heading = 0;
    float speedMs = 0;
    float wetness = 0;
    bool damaged = false;
};

struct AIOutputs {
    float steer = 0, throttle = 0, brake = 0;
    int gearShift = 0;
};

class AIDriver {
public:
    int aggression = 2;
    float skill = 0.85f;
    float lookaheadBase = 12.0f;
    float lateralError = 0, prevLateralError = 0, integral = 0;

    void setPath(const QVector<AITarget>& path) { m_path = path; m_idx = 0; }
    bool hasPath() const { return !m_path.isEmpty(); }

    AIOutputs update(const AIInputs& in, float dt) {
        AIOutputs out;
        if (m_path.isEmpty()) { out.brake = 1.0f; return out; }
        int n = m_path.size();
        int nearest = nearestIdx(in.pos);
        m_idx = nearest;
        float lookahead = lookaheadBase + in.speedMs * 0.6f;
        int look = lookaheadIdx(nearest, lookahead);
        const auto& tp = m_path[look];
        QVector3D toT = tp.pos - in.pos;
        float targetHeading = std::atan2(toT.x(), toT.z());
        float dh = wrap(targetHeading - in.heading);
        float lateral = toT.x() * std::cos(in.heading) - toT.z() * std::sin(in.heading);
        lateralError = lateral;
        float dErr = dt > 0 ? (lateralError - prevLateralError) / dt : 0;
        prevLateralError = lateralError;
        integral = std::clamp(integral + lateralError * dt, -2.0f, 2.0f);
        float kp = 0.12f + aggression * 0.02f;
        float kd = 0.02f, ki = 0.01f;
        float steer = dh * 1.8f + lateral * kp * 0.1f + dErr * kd * 0.05f + integral * ki;
        float errScale = 1.0f - skill * 0.1f + aggression * 0.03f;
        steer *= errScale;
        out.steer = qBound(-1.0f, steer, 1.0f);

        float targetSpeed = tp.speed * skill;
        targetSpeed *= (1.0f - in.wetness * 0.3f);
        if (in.damaged) targetSpeed *= 0.9f;
        targetSpeed *= (1.0f - std::abs(tp.curvature) * (0.5f - aggression * 0.05f));
        float dv = targetSpeed - in.speedMs;
        if (dv > 0.5f) { out.throttle = qBound(0.0f, dv * 0.2f, 1.0f); out.brake = 0; }
        else if (dv < -1.0f) { out.brake = qBound(0.0f, -dv * 0.25f, 1.0f); out.throttle = 0; }
        updateAvoidance(in, out, dt);
        return out;
    }

    void setObstacles(const QVector<QVector3D>& obs) { m_obstacles = obs; }

private:
    QVector<AITarget> m_path;
    QVector<QVector3D> m_obstacles;
    int m_idx = 0;

    static float wrap(float a) {
        while (a > 3.14159f) a -= 6.28318f;
        while (a < -3.14159f) a += 6.28318f;
        return a;
    }
    int nearestIdx(const QVector3D& p) const {
        int best = 0; float bd = 1e30f;
        for (int i = 0; i < m_path.size(); ++i) {
            float d = (m_path[i].pos - p).lengthSquared();
            if (d < bd) { bd = d; best = i; }
        }
        return best;
    }
    int lookaheadIdx(int from, float dist) const {
        if (m_path.isEmpty()) return 0;
        float acc = 0; int idx = from;
        for (int k = 0; k < m_path.size(); ++k) {
            int nx = (idx + 1) % m_path.size();
            acc += (m_path[nx].pos - m_path[idx].pos).length();
            idx = nx;
            if (acc >= dist) break;
        }
        return idx;
    }
    void updateAvoidance(const AIInputs& in, AIOutputs& out, float dt) {
        Q_UNUSED(dt);
        QVector3D fwd(std::sin(in.heading), 0, std::cos(in.heading));
        for (const auto& o : m_obstacles) {
            QVector3D d = o - in.pos;
            float along = QVector3D::dotProduct(d, fwd);
            if (along < 2 || along > 25) continue;
            QVector3D lat = d - fwd * along;
            if (lat.length() < 3.5f) {
                out.brake = qMax(out.brake, 0.7f);
                out.throttle = 0.0f;
                float side = (fwd.x() * d.z() - fwd.z() * d.x()) > 0 ? -1.0f : 1.0f;
                out.steer = qBound(-1.0f, out.steer + side * 0.4f, 1.0f);
                break;
            }
        }
    }
};

} // namespace ks::ai
