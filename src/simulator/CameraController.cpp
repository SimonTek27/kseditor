#include "CameraController.h"
#include <cmath>
#include <algorithm>

namespace ks::sim {

CameraController::CameraController() = default;

void CameraController::update(float dt, const mat4& carBodyMatrix, float speedKmh)
{
    switch (m_mode) {
    case Mode::Cockpit: updateCockpit(carBodyMatrix, speedKmh); break;
    case Mode::Chase:   updateChase(carBodyMatrix, speedKmh, dt); break;
    case Mode::Free:    updateFree(dt); break;
    }
    updateProjection();
}

void CameraController::updateCockpit(const mat4& carMatrix, float speedKmh)
{
    vec3 localPos = m_cockpitOffset;

    float bobSpeed = speedKmh / 3.6f;
    m_headBobPhase += bobSpeed * 0.05f;
    float bob = std::sin(m_headBobPhase) * m_headBobAmplitude * bobSpeed;
    localPos = localPos + vec3(0, bob, 0);

    m_position = carMatrix * localPos;

    vec3 localTarget = m_cockpitLookTarget;
    vec3 worldTarget = carMatrix * localTarget;

    m_forward = (worldTarget - m_position).normalized();
    m_up = vec3(0, 1, 0);

    m_viewMatrix = mat4::lookAt(m_position, m_position + m_forward, m_up);
}

void CameraController::updateChase(const mat4& carMatrix, float speedKmh, float dt)
{
    (void)speedKmh;

    vec3 carPos = carMatrix * vec3(0, 0, 0);
    vec3 carForward = (carMatrix * vec3(0, 0, 1)) - carPos;
    carForward = carForward.normalized();
    vec3 carUp = (carMatrix * vec3(0, 1, 0)) - carPos;
    carUp = carUp.normalized();

    vec3 targetPos = carPos - carForward * m_chaseDistance + carUp * m_chaseHeight;

    if (!m_chaseInitialized) {
        m_chaseCurrentPos = targetPos;
        m_chaseInitialized = true;
    }

    float lagFactor = 1.0f - std::exp(-m_chaseLag * dt);
    m_chaseCurrentPos = m_chaseCurrentPos + (targetPos - m_chaseCurrentPos) * lagFactor;

    m_position = m_chaseCurrentPos;

    vec3 lookTarget = carPos + carUp * 1.0f;
    m_forward = (lookTarget - m_position).normalized();
    m_up = vec3(0, 1, 0);

    m_viewMatrix = mat4::lookAt(m_position, lookTarget, m_up);
}

void CameraController::updateFree(float dt)
{
    (void)dt;

    m_viewMatrix = mat4::lookAt(m_position, m_position + m_forward, m_up);
}

void CameraController::updateProjection()
{
    m_projectionMatrix = mat4::perspective(
        m_fov * 3.14159f / 180.0f,
        m_aspectRatio,
        m_nearPlane,
        m_farPlane
    );
}

void CameraController::setAspectRatio(float ar)
{
    m_aspectRatio = ar;
}

void CameraController::moveFree(const vec3& delta)
{
    vec3 right = m_forward.cross(m_up).normalized();
    m_position = m_position + m_forward * delta.z + right * delta.x + m_up * delta.y;
}

void CameraController::rotateFree(float yawDelta, float pitchDelta)
{
    m_freeYaw += yawDelta;
    m_freePitch += pitchDelta;
    m_freePitch = std::clamp(m_freePitch, -89.0f, 89.0f);

    float yawRad = m_freeYaw * 3.14159f / 180.0f;
    float pitchRad = m_freePitch * 3.14159f / 180.0f;

    m_forward = vec3(
        std::cos(pitchRad) * std::sin(yawRad),
        std::sin(pitchRad),
        std::cos(pitchRad) * std::cos(yawRad)
    ).normalized();
}

} // namespace ks::sim
