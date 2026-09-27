#include "AIController.h"
#include <cmath>
#include <limits>
#include <cstdio>
#include <algorithm>

namespace ks::sim {

AIController::AIController() = default;
AIController::~AIController() = default;

bool AIController::loadSpline(const std::string& trackDirectory)
{
    std::string splinePath = trackDirectory + "/ai/fast_lane.ai";
    m_spline = ks::ai::AiFileReader::readSpline(splinePath);

    if (!m_spline.isValid()) {
        // try .txt companion
        m_spline = ks::ai::AiFileReader::readSpline(splinePath + ".txt");
    }

    if (!m_spline.isValid()) {
        std::printf("AIController: Failed to parse spline from: %s\n", splinePath.c_str());
        return false;
    }

    m_cumulativeDistance.resize(m_spline.points.size());
    m_cumulativeDistance[0] = 0.0f;
    for (size_t i = 1; i < m_spline.points.size(); ++i) {
        float dx = m_spline.points[i].position.x - m_spline.points[i - 1].position.x;
        float dy = m_spline.points[i].position.y - m_spline.points[i - 1].position.y;
        float dz = m_spline.points[i].position.z - m_spline.points[i - 1].position.z;
        m_cumulativeDistance[i] = m_cumulativeDistance[i - 1]
                                  + std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    m_splineLoaded = true;
    m_currentIdx = 0;

    std::printf("AIController: Loaded spline with %d points\n", (int)m_spline.points.size());
    if (onSplineLoaded) onSplineLoaded(static_cast<int>(m_spline.points.size()));
    return true;
}

void AIController::update(const vec3& carPosition, float carHeading,
                          float speed, int /*gear*/, float dt)
{
    if (!m_splineLoaded || m_spline.points.empty()) {
        m_throttle = 0; m_brake = 1.0f; m_steering = 0;
        return;
    }

    int nearest = findNearestPoint(carPosition);
    m_currentIdx = nearest;

    int lap = m_spline.points[static_cast<size_t>(nearest)].lap;
    if (lap > m_prevLap) {
        m_prevLap = lap;
        if (onLapCompleted) onLapCompleted(lap);
    }

    int lookahead = findLookaheadPoint(nearest, m_lookaheadDist);
    const auto& targetPoint = m_spline.points[static_cast<size_t>(lookahead)];
    vec3 targetPos(targetPoint.position.x, targetPoint.position.y, targetPoint.position.z);

    m_steering = calculateSteering(carPosition, carHeading, targetPos);

    float targetSpeed = targetPoint.speed * m_speedFactor;
    float curvature = std::abs(targetPoint.curvature);
    if (curvature > 0.01f)
        targetSpeed *= (1.0f - curvature * m_aggression * 0.5f);

    m_throttle = calculateThrottle(speed, targetSpeed, curvature, dt);
    m_brake = calculateBrake(speed, targetSpeed, curvature, dt);
    m_targetGear = calculateGear(speed);
}

int AIController::findNearestPoint(const vec3& pos) const
{
    if (m_spline.points.empty()) return 0;

    int bestIdx = 0;
    float bestDist = std::numeric_limits<float>::max();
    for (size_t i = 0; i < m_spline.points.size(); ++i) {
        float dx = m_spline.points[i].position.x - pos.x;
        float dy = m_spline.points[i].position.y - pos.y;
        float dz = m_spline.points[i].position.z - pos.z;
        float d = dx*dx + dy*dy + dz*dz;
        if (d < bestDist) {
            bestDist = d;
            bestIdx = static_cast<int>(i);
        }
    }
    return bestIdx;
}

int AIController::findLookaheadPoint(int nearestIdx, float dist) const
{
    if (m_spline.points.empty()) return 0;
    float base = m_cumulativeDistance[static_cast<size_t>(nearestIdx)];
    float target = base + dist;
    for (size_t i = static_cast<size_t>(nearestIdx); i < m_spline.points.size(); ++i) {
        if (m_cumulativeDistance[i] >= target)
            return static_cast<int>(i);
    }
    return static_cast<int>(m_spline.points.size()) - 1;
}

float AIController::calculateSteering(const vec3& carPos, float carHeading,
                                      const vec3& targetPos) const
{
    float dx = targetPos.x - carPos.x;
    float dz = targetPos.z - carPos.z;
    float targetHeading = std::atan2(dx, dz);
    float err = targetHeading - carHeading;
    while (err > 3.14159f) err -= 6.28318f;
    while (err < -3.14159f) err += 6.28318f;
    return std::clamp(err * 1.2f, -1.0f, 1.0f);
}

float AIController::calculateThrottle(float currentSpeed, float targetSpeed,
                                      float /*curvature*/, float /*dt*/) const
{
    float err = targetSpeed - currentSpeed;
    if (err <= 0.5f) return 0.0f;
    return std::clamp(err * 0.08f, 0.0f, 1.0f);
}

float AIController::calculateBrake(float currentSpeed, float targetSpeed,
                                   float /*curvature*/, float /*dt*/) const
{
    float err = currentSpeed - targetSpeed;
    if (err <= 1.0f) return 0.0f;
    return std::clamp(err * 0.1f, 0.0f, 1.0f);
}

int AIController::calculateGear(float speed) const
{
    // speed m/s rough gear map
    if (speed < 10) return 1;
    if (speed < 20) return 2;
    if (speed < 30) return 3;
    if (speed < 40) return 4;
    if (speed < 50) return 5;
    return 6;
}

} // namespace ks::sim
