#pragma once

/**
 * @file PhysicsProfiler.h
 * @brief Lightweight frame/section profiler — Qt-free
 */

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>

namespace ks {
namespace physics {

class PhysicsProfiler {
public:
    enum Subsystem {
        Engine = 0, Drivetrain, Differential, Brakes, Aero, Suspension,
        Tires, VehicleDynamics, DamageModel, WeatherPhysics, Total
    };

    static PhysicsProfiler& instance();

    void setEnabled(bool e) { m_enabled = e; }
    bool isEnabled() const { return m_enabled; }

    void beginFrame();
    void endFrame();
    void beginSection(const std::string& name);
    void endSection(const std::string& name);
    void beginSubsystem(Subsystem s);
    void endSubsystem(Subsystem s);

    double frameTimeMs() const { return m_lastFrameMs; }
    double avgFrameTimeMs() const { return m_avgFrameMs; }
    int fps() const { return m_lastFrameMs > 0.01 ? static_cast<int>(1000.0 / m_lastFrameMs) : 0; }
    int frameCount() const { return m_frameCount; }

    void setGpuFrameTimeMs(double ms) { m_gpuFrameMs = ms; }
    void setGpuAvgFrameTimeMs(double ms) { m_gpuAvgFrameMs = ms; }
    double gpuFrameTimeMs() const { return m_gpuFrameMs; }
    double gpuAvgFrameTimeMs() const { return m_gpuAvgFrameMs; }
    int gpuFps() const {
        return m_gpuFrameMs > 0.01 ? static_cast<int>(1000.0 / m_gpuFrameMs) : 0;
    }

private:
    PhysicsProfiler() = default;
    using clock = std::chrono::steady_clock;

    bool m_enabled = true;
    clock::time_point m_frameStart{};
    double m_lastFrameMs = 0.0;
    double m_avgFrameMs = 0.0;
    int m_frameCount = 0;
    double m_gpuFrameMs = 0.0;
    double m_gpuAvgFrameMs = 0.0;
    std::mutex m_mutex;
    std::unordered_map<std::string, clock::time_point> m_sectionStart;
    std::unordered_map<int, clock::time_point> m_subStart;
};

struct ProfilerSection {
    explicit ProfilerSection(const std::string& name) : m_name(name) {
        PhysicsProfiler::instance().beginSection(m_name);
    }
    ~ProfilerSection() { PhysicsProfiler::instance().endSection(m_name); }
    std::string m_name;
};

#define PROFILE_FRAME() ::ks::physics::PhysicsProfiler::instance().beginFrame()
#define PROFILE_END_FRAME() ::ks::physics::PhysicsProfiler::instance().endFrame()
#define PROFILE_SECTION(name) ::ks::physics::ProfilerSection _ps_##__LINE__(name)

} // namespace physics
} // namespace ks
