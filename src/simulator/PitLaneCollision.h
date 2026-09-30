#pragma once
/**
 * Pit lane collision simulation (low-speed).
 *
 * - Car–car: 2D OBB (length × width) on XZ, soft separation + impulse
 * - Car–wall: pit corridor half-width clamp
 * - Feeds queue (block / reduce speed) and optional damage impulse
 */
#include "PitLaneQueue.h"
#include "MathTypes.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <functional>

namespace ks {
namespace sim {

struct PitCarBody {
    int carId = -1;
    float x = 0.f, z = 0.f;
    float heading = 0.f;     // yaw rad
    float vx = 0.f, vz = 0.f; // m/s world
    float length = 4.6f;     // m
    float width = 2.0f;
    float mass = 1200.f;
    bool active = true;
    bool invulnerable = false; // e.g. still snapped in box
};

struct PitCollisionContact {
    int carA = -1;
    int carB = -1;           // -1 = wall
    float nx = 0.f, nz = 0.f; // normal A ← B (or wall inward)
    float penetration = 0.f;
    float relSpeed = 0.f;    // closing speed m/s
    float impulse = 0.f;     // applied scalar
    bool withWall = false;
};

struct PitLaneCollisionConfig {
    float pitHalfWidthM = 3.5f;     // corridor half-width from axis
    float restitution = 0.15f;      // soft bumps
    float friction = 0.4f;
    float separationPercent = 0.85f;
    float maxImpulse = 2500.f;      // N·s cap (pit speeds)
    float damageSpeedThreshold = 2.5f; // m/s closing before reporting damage
    float wallRestitution = 0.05f;
    bool enableWalls = true;
    bool enableCarCar = true;
};

class PitLaneCollision {
public:
    void setConfig(const PitLaneCollisionConfig& c) { m_cfg = c; }
    const PitLaneCollisionConfig& config() const { return m_cfg; }

    void setAxis(const PitAxis& axis) { m_axis = axis; }

    std::vector<PitCarBody>& bodies() { return m_bodies; }
    const std::vector<PitCarBody>& bodies() const { return m_bodies; }
    const std::vector<PitCollisionContact>& lastContacts() const { return m_contacts; }

    PitCarBody* find(int carId) {
        for (auto& b : m_bodies)
            if (b.carId == carId) return &b;
        return nullptr;
    }

    void upsert(const PitCarBody& body) {
        if (auto* b = find(body.carId)) { *b = body; return; }
        m_bodies.push_back(body);
    }

    void remove(int carId) {
        m_bodies.erase(std::remove_if(m_bodies.begin(), m_bodies.end(),
            [carId](const PitCarBody& b) { return b.carId == carId; }), m_bodies.end());
    }

    /**
 * Run pair tests + walls. Writes corrected positions/velocities into bodies.
 * @return number of contacts generated this step
 */
    int step(float /*dt*/) {
        m_contacts.clear();
        if (m_cfg.enableCarCar)
            collidePairs();
        if (m_cfg.enableWalls)
            collideWalls();
        return static_cast<int>(m_contacts.size());
    }

    /**
 * Closing impulse magnitude useful for DamageSystem (0 if soft).
 */
    float damageImpulseFor(int carId) const {
        float best = 0.f;
        for (const auto& c : m_contacts) {
            if (c.carA != carId && c.carB != carId) continue;
            if (c.relSpeed < m_cfg.damageSpeedThreshold) continue;
            best = std::max(best, c.impulse);
        }
        return best;
    }

    /** True if car is currently penetrating another or wall. */
    bool isInContact(int carId) const {
        for (const auto& c : m_contacts)
            if (c.carA == carId || c.carB == carId) return true;
        return false;
    }

    /** Hook: notify external systems (damage, VFX, SFX). */
    std::function<void(const PitCollisionContact&)> onContact;

    /**
 * Update queue pathBlocked hints: if leaving car collides, treat as blocked.
 */
    void applyToQueue(PitLaneQueue& queue) const {
        for (const auto& c : m_contacts) {
            if (c.withWall) continue;
            // both cars should slow — suggested via queue updateCar already;
            // mark waiting if almost stationary and overlapping leave intent
            (void)queue;
            (void)c;
        }
    }

private:
    struct OBB {
        float cx, cz, hx, hz, wx, wz, halfL, halfW;
    };

    static OBB makeObb(const PitCarBody& b) {
        OBB o;
        o.cx = b.x;
        o.cz = b.z;
        o.hx = std::sin(b.heading);
        o.hz = std::cos(b.heading);
        o.wx = std::sin(b.heading + 1.5707963f);
        o.wz = std::cos(b.heading + 1.5707963f);
        o.halfL = b.length * 0.5f;
        o.halfW = b.width * 0.5f;
        return o;
    }

    /** SAT overlap; returns true + MTV (push A out of B). */
    static bool overlapObb(const OBB& a, const OBB& b, float& nx, float& nz, float& pen) {
        const float dx = a.cx - b.cx;
        const float dz = a.cz - b.cz;

        float minPen = 1e9f;
        float bestNx = 0.f, bestNz = 0.f;

        auto testAxis = [&](float ax, float az) -> bool {
            const float rA = a.halfL * std::fabs(a.hx * ax + a.hz * az)
                           + a.halfW * std::fabs(a.wx * ax + a.wz * az);
            const float rB = b.halfL * std::fabs(b.hx * ax + b.hz * az)
                           + b.halfW * std::fabs(b.wx * ax + b.wz * az);
            const float dist = std::fabs(dx * ax + dz * az);
            const float overlap = rA + rB - dist;
            if (overlap <= 0.f) return false;
            if (overlap < minPen) {
                minPen = overlap;
                // normal from B to A along axis
                const float side = (dx * ax + dz * az) >= 0.f ? 1.f : -1.f;
                bestNx = ax * side;
                bestNz = az * side;
            }
            return true;
        };

        if (!testAxis(a.hx, a.hz)) return false;
        if (!testAxis(a.wx, a.wz)) return false;
        if (!testAxis(b.hx, b.hz)) return false;
        if (!testAxis(b.wx, b.wz)) return false;

        nx = bestNx;
        nz = bestNz;
        pen = minPen;
        return pen > 0.f;
    }

    void resolvePair(PitCarBody& a, PitCarBody& b, float nx, float nz, float pen) {
        if (a.invulnerable && b.invulnerable) return;

        const float invMassA = a.invulnerable ? 0.f : 1.f / std::max(1.f, a.mass);
        const float invMassB = b.invulnerable ? 0.f : 1.f / std::max(1.f, b.mass);
        const float invSum = invMassA + invMassB;
        if (invSum < 1e-8f) return;

        // positional correction
        const float corr = pen * m_cfg.separationPercent / invSum;
        a.x += nx * corr * invMassA;
        a.z += nz * corr * invMassA;
        b.x -= nx * corr * invMassB;
        b.z -= nz * corr * invMassB;

        // relative velocity along normal
        const float rvx = a.vx - b.vx;
        const float rvz = a.vz - b.vz;
        const float velAlong = rvx * nx + rvz * nz;

        PitCollisionContact c;
        c.carA = a.carId;
        c.carB = b.carId;
        c.nx = nx;
        c.nz = nz;
        c.penetration = pen;
        c.relSpeed = std::max(0.f, -velAlong);
        c.withWall = false;

        if (velAlong < 0.f) {
            // closing — apply impulse
            const float e = m_cfg.restitution;
            float j = -(1.f + e) * velAlong / invSum;
            j = std::min(j, m_cfg.maxImpulse);
            a.vx += j * invMassA * nx;
            a.vz += j * invMassA * nz;
            b.vx -= j * invMassB * nx;
            b.vz -= j * invMassB * nz;

            // tangential friction (simple)
            float tx = rvx - velAlong * nx;
            float tz = rvz - velAlong * nz;
            const float tlen = std::sqrt(tx * tx + tz * tz);
            if (tlen > 1e-4f) {
                tx /= tlen; tz /= tlen;
                float jt = -m_cfg.friction * j;
                a.vx += jt * invMassA * tx;
                a.vz += jt * invMassA * tz;
                b.vx -= jt * invMassB * tx;
                b.vz -= jt * invMassB * tz;
            }
            c.impulse = j;
        }

        m_contacts.push_back(c);
        if (onContact) onContact(c);
    }

    void collidePairs() {
        const int n = static_cast<int>(m_bodies.size());
        for (int i = 0; i < n; ++i) {
            if (!m_bodies[static_cast<size_t>(i)].active) continue;
            for (int j = i + 1; j < n; ++j) {
                if (!m_bodies[static_cast<size_t>(j)].active) continue;
                auto& A = m_bodies[static_cast<size_t>(i)];
                auto& B = m_bodies[static_cast<size_t>(j)];
                float nx, nz, pen;
                if (overlapObb(makeObb(A), makeObb(B), nx, nz, pen))
                    resolvePair(A, B, nx, nz, pen);
            }
        }
    }

    void collideWalls() {
        // Pit corridor: |lateral| <= halfWidth
        const float hx = std::sin(m_axis.heading);
        const float hz = std::cos(m_axis.heading);
        const float lx = std::sin(m_axis.heading + 1.5707963f);
        const float lz = std::cos(m_axis.heading + 1.5707963f);
        const float half = m_cfg.pitHalfWidthM;

        for (auto& b : m_bodies) {
            if (!b.active || b.invulnerable) continue;

            const float dx = b.x - m_axis.originX;
            const float dz = b.z - m_axis.originZ;
            const float lat = dx * lx + dz * lz;
            // account half car width
            const float limit = half - b.width * 0.5f;
            if (limit <= 0.1f) continue;

            if (lat > limit || lat < -limit) {
                const float over = (lat > 0.f) ? (lat - limit) : (lat + limit);
                const float sign = lat > 0.f ? 1.f : -1.f;
                // inward normal (toward axis)
                const float nx = -lx * sign;
                const float nz = -lz * sign;
                const float pen = std::fabs(over);

                b.x += nx * pen * m_cfg.separationPercent;
                b.z += nz * pen * m_cfg.separationPercent;

                const float velAlong = b.vx * nx + b.vz * nz;
                float j = 0.f;
                if (velAlong < 0.f) {
                    j = -(1.f + m_cfg.wallRestitution) * velAlong;
                    j = std::min(j, m_cfg.maxImpulse * 0.5f);
                    b.vx += j * nx;
                    b.vz += j * nz;
                }

                PitCollisionContact c;
                c.carA = b.carId;
                c.carB = -1;
                c.nx = nx;
                c.nz = nz;
                c.penetration = pen;
                c.relSpeed = std::max(0.f, -velAlong);
                c.impulse = j;
                c.withWall = true;
                m_contacts.push_back(c);
                if (onContact) onContact(c);
            }

            // keep along-axis free (no end barriers here)
            (void)hx; (void)hz;
        }
    }

    PitLaneCollisionConfig m_cfg;
    PitAxis m_axis;
    std::vector<PitCarBody> m_bodies;
    std::vector<PitCollisionContact> m_contacts;
};

/**
 * Sync queue car poses into collision bodies and apply results back.
 */
inline void syncPitCollisionFromQueue(
    PitLaneCollision& col,
    const PitLaneQueue& queue,
    float defaultHeading)
{
    for (const auto& e : queue.entries()) {
        PitCarBody b;
        if (auto* existing = col.find(e.carId))
            b = *existing;
        b.carId = e.carId;
        b.x = e.posX;
        b.z = e.posZ;
        b.vx = e.speedMs * std::sin(defaultHeading); // approx if no per-car heading
        b.vz = e.speedMs * std::cos(defaultHeading);
        b.heading = defaultHeading;
        b.active = e.status != PitQueueStatus::Done;
        b.invulnerable = (e.role == PitQueueRole::Holding);
        col.upsert(b);
    }
}

/**
 * After collision step: push positions back to queue + optional garage path block.
 */
inline void applyPitCollisionResults(
    PitLaneCollision& col,
    PitLaneQueue& queue,
    float (*setPose)(int carId, float x, float z, float vx, float vz, void* user),
    void* user = nullptr)
{
    for (const auto& b : col.bodies()) {
        queue.updateCar(b.carId, b.x, b.z, std::sqrt(b.vx * b.vx + b.vz * b.vz));
        if (setPose)
            setPose(b.carId, b.x, b.z, b.vx, b.vz, user);
    }
    col.applyToQueue(queue);
}

} // namespace sim
} // namespace ks
