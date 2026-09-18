#include "AIController.h"
#include <cstdio>
#include <cmath>
#include <limits>

namespace ks::sim {

AIController::AIController() = default;
AIController::~AIController() = default;

bool AIController::loadSpline(const std::string& trackDirectory)
{
    std::string splinePath = trackDirectory + "/ai/fast_lane.ai";
    ks::ai::AiFileReader reader;
    m_spline = reader.readSpline(QString::fromStdString(splinePath));

    if (!m_spline.isValid()) {
        printf("AIController: Failed to parse spline from: %s\n", splinePath.c_str());
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

    printf("AIController: Loaded spline with %d points\n", (int)m_spline.points.size());
    if (onSplineLoaded) onSplineLoaded(static_cast<int>(m_spline.points.size()));
    return true;
}

void AIController::update(const vec3& carPosition, float carHeading,
                          float speed, int gear, float dt)
{
    if (!m_splineLoaded || m_spline.points.isEmpty()) {
        m_throttle = 0; m_brake = 1.0f; m_steering = 0;
        return;
    }

    int nearest = findNearestPoint(carPosition);
    m_currentIdx = nearest;

    int lap = m_spline.points[nearest].lap;
    if (lap > m_prevLap) {
        m_prevLap = lap;
        if (onLapCompleted) onLapCompleted(lap);
    }

    int lookahead = findLookaheadPoint(nearest, m_lookaheadDist);
    const auto& targetPoint = m_spline.points[lookahead];
    vec3 targetPos(targetPoint.position.x, targetPoint.position.y, targetPoint.position.z);

    m_steering = calculateSteering(carPosition, carHeading, targetPos);

    float targetSpeed = targetPoint.speed * m_speedFactor;
    float curvature = std::abs(targetPoint.curvature);
    if (curvature > 0.01f) {
        targetSpeed *= (1.0f - curvature * m_aggression * 0.5f);
    }

    m_throttle = calculateThrottle(speed, targetSpeed, curvature, dt);
    m_brake = calculateBrake(speed, targetSpeed, curvature, dt);
    m_targetGear = calculateGear(speed);
}

int AIController::findNearestPoint(const vec3& pos) const
{
    if (m_spline.points.isEmpty()) return 0;

    int bestIdx = 0;
    float bestDist = std::numeric_limits<float>::max();

    for (int i = 0; i < m_spline.points.size(); ++i) {
        const auto& p = m_spline.points[i].position;
        float dx = pos.x - p.x;
        float dy = pos.y - p.y;
        float dz = pos.z - p.z;
        float dist = dx * dx + dy * dy + dz * dz;
        if (dist < bestDist) {
            bestDist = dist;
            bestIdx = i;
        }
    }

    return bestIdx;
}

int AIController::findLookaheadPoint(int nearestIdx, float dist) const
{
    int n = m_spline.points.size();
    if (n == 0) return 0;

    float distAccum = 0;
    int idx = nearestIdx;

    while (distAccum < dist) {
        int nextIdx = (idx + 1) % n;
        float dx = m_spline.points[nextIdx].position.x - m_spline.points[idx].position.x;
        float dy = m_spline.points[nextIdx].position.y - m_spline.points[idx].position.y;
        float dz = m_spline.points[nextIdx].position.z - m_spline.points[idx].position.z;
        distAccum += std::sqrt(dx * dx + dy * dy + dz * dz);
        idx = nextIdx;
    }

    return idx;
}

float AIController::calculateSteering(const vec3& carPos, float carHeading,
                                      const vec3& targetPos) const
{
    float dx = targetPos.x - carPos.x;
    float dz = targetPos.z - carPos.z;

    float targetAngle = std::atan2(dx, dz);
    float angleDiff = targetAngle - carHeading;
    while (angleDiff > M_PI) angleDiff -= 2.0f * (float)M_PI;
    while (angleDiff < -M_PI) angleDiff += 2.0f * (float)M_PI;

    float steer = angleDiff / ((float)M_PI * 0.5f);
    return std::clamp(steer, -1.0f, 1.0f);
}

float AIController::calculateThrottle(float currentSpeed, float targetSpeed,
                                      float curvature, float dt) const
{
    (void)dt;
    float speedError = targetSpeed - currentSpeed;
    float throttle = 0;

    if (speedError > 0.5f) {
        throttle = std::clamp(speedError / (targetSpeed * 0.3f + 1.0f), 0.0f, 1.0f);
        throttle *= (1.0f - curvature * 0.5f);
    }

    return throttle;
}

float AIController::calculateBrake(float currentSpeed, float targetSpeed,
                                   float curvature, float dt) const
{
    (void)dt;
    float speedError = currentSpeed - targetSpeed;
    float brake = 0;

    if (speedError > 1.0f) {
        brake = std::clamp(speedError / (targetSpeed * 0.4f + 1.0f), 0.0f, 1.0f);
        brake *= (1.0f + curvature * m_aggression);
    }

    return std::clamp(brake, 0.0f, 1.0f);
}

int AIController::calculateGear(float speed) const
{
    float speedKmh = speed * 3.6f;
    if (speedKmh < 15) return -1;
    if (speedKmh < 30) return 1;
    if (speedKmh < 60) return 2;
    if (speedKmh < 100) return 3;
    if (speedKmh < 140) return 4;
    if (speedKmh < 190) return 5;
    return 6;
}

} // namespace ks::sim
