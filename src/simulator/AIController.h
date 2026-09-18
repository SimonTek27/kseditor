#pragma once

#include "MathTypes.h"
#include <string>
#include <vector>
#include <functional>
#include "engine/FileFormat/AiSpline.h"

namespace ks::sim {

// Note: no Q_OBJECT - AIController is not a QObject. MOC should not process this header.
#ifndef Q_MOC_RUN
class AIController {
public:
    AIController();
    ~AIController();

    bool loadSpline(const std::string& trackDirectory);

    void update(const vec3& carPosition, float carHeading,
                float speed, int gear, float dt);

    float throttle() const { return m_throttle; }
    float brake() const { return m_brake; }
    float steering() const { return m_steering; }
    int targetGear() const { return m_targetGear; }
    bool isReady() const { return m_splineLoaded; }

    void setLookaheadDistance(float d) { m_lookaheadDist = d; }
    void setSpeedFactor(float f) { m_speedFactor = f; }
    void setAggression(float a) { m_aggression = std::clamp(a, 0.0f, 1.0f); }

    std::function<void(int)> onSplineLoaded;
    std::function<void(int)> onLapCompleted;

private:
    int findNearestPoint(const vec3& pos) const;
    int findLookaheadPoint(int nearestIdx, float dist) const;
    float calculateSteering(const vec3& carPos, float carHeading,
                            const vec3& targetPos) const;
    float calculateThrottle(float currentSpeed, float targetSpeed,
                            float curvature, float dt) const;
    float calculateBrake(float currentSpeed, float targetSpeed,
                         float curvature, float dt) const;
    int calculateGear(float speed) const;

    bool m_splineLoaded = false;
    ks::ai::AiSpline m_spline;
    std::vector<float> m_cumulativeDistance;

    int m_currentIdx = 0;
    int m_prevLap = 0;

    float m_throttle = 0;
    float m_brake = 0;
    float m_steering = 0;
    int m_targetGear = 1;

    float m_lookaheadDist = 30.0f;
    float m_speedFactor = 1.0f;
    float m_aggression = 0.5f;
    float m_maxSteerRate = 2.0f;
};
#endif // Q_MOC_RUN

} // namespace ks::sim
