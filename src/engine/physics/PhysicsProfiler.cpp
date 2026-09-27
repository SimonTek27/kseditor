#include "PhysicsProfiler.h"

namespace ks {
namespace physics {

PhysicsProfiler& PhysicsProfiler::instance() {
    static PhysicsProfiler s;
    return s;
}

void PhysicsProfiler::beginFrame() {
    if (!m_enabled) return;
    m_frameStart = clock::now();
}

void PhysicsProfiler::endFrame() {
    if (!m_enabled) return;
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(clock::now() - m_frameStart).count();
    m_lastFrameMs = ns / 1e6;
    ++m_frameCount;
    const double a = 0.05;
    m_avgFrameMs = (m_frameCount == 1) ? m_lastFrameMs : (m_avgFrameMs * (1.0 - a) + m_lastFrameMs * a);
}

void PhysicsProfiler::beginSection(const std::string& name) {
    if (!m_enabled) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sectionStart[name] = clock::now();
}

void PhysicsProfiler::endSection(const std::string& name) {
    if (!m_enabled) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sectionStart.erase(name);
}

void PhysicsProfiler::beginSubsystem(Subsystem s) {
    if (!m_enabled) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_subStart[static_cast<int>(s)] = clock::now();
}

void PhysicsProfiler::endSubsystem(Subsystem s) {
    if (!m_enabled) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_subStart.erase(static_cast<int>(s));
}

} // namespace physics
} // namespace ks
