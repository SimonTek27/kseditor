/**
 * C API implementation — thin wrapper around VehicleSimulator.
 * Headless: no window, no Qt, no Vulkan required for step/get_state.
 */
#include "ksengine_c.h"
#include "../physics/VehicleSimulator.h"
#include "../physics/ReplaySystem.h"
#include <memory>
#include <string>
#include <cmath>
#include <cstring>

struct KsEngine {
    ks::physics::VehicleSimulator vehicle;
    ks::physics::ReplaySystem replay;
    KsInput input{};
    double fixedDt = 0.001;
    double accum = 0.0;
    double time = 0.0;
    uint32_t seed = 0;
    std::string replayPath;
    bool recording = false;
};

extern "C" {

KSAPI const char* ks_engine_version(void) {
    return "0.1.0";
}

KSAPI KsEngine* ks_engine_create(uint32_t seed) {
    auto* e = new KsEngine();
    e->seed = seed;
    e->vehicle.reset();
    e->vehicle.startSimulation();
    return e;
}

KSAPI void ks_engine_destroy(KsEngine* eng) {
    if (!eng) return;
    if (eng->recording && !eng->replayPath.empty())
        eng->replay.save(eng->replayPath);
    delete eng;
}

KSAPI void ks_engine_set_timestep(KsEngine* eng, double dt) {
    if (!eng) return;
    if (dt > 1e-5 && dt < 0.05)
        eng->fixedDt = dt;
}

KSAPI void ks_engine_reset(KsEngine* eng) {
    if (!eng) return;
    eng->vehicle.reset();
    eng->vehicle.startSimulation();
    eng->accum = 0;
    eng->time = 0;
    eng->replay.clear();
}

KSAPI void ks_engine_set_input(KsEngine* eng, const KsInput* in) {
    if (!eng || !in) return;
    eng->input = *in;
    eng->vehicle.setThrottle(in->throttle);
    eng->vehicle.setBrake(in->brake);
    eng->vehicle.setSteering(in->steer);
}

KSAPI void ks_engine_step(KsEngine* eng, double dt) {
    if (!eng || dt <= 0.0) return;
    eng->accum += dt;
    int guard = 0;
    while (eng->accum >= eng->fixedDt && guard++ < 64) {
        eng->vehicle.updatePhysics(eng->fixedDt);
        eng->time += eng->fixedDt;
        eng->accum -= eng->fixedDt;

        if (eng->recording) {
            ks::physics::ReplayFrame f;
            f.time = eng->time;
            const auto st = eng->vehicle.getState();
            f.x = static_cast<float>(st.position.x);
            f.y = static_cast<float>(st.position.y);
            f.z = static_cast<float>(st.position.z);
            f.speed = static_cast<float>(st.speed);
            f.throttle = eng->input.throttle;
            f.brake = eng->input.brake;
            f.steer = eng->input.steer;
            f.gear = eng->vehicle.currentGear();
            eng->replay.recordFrame(f);
        }
    }
}

KSAPI void ks_engine_get_state(const KsEngine* eng, KsVehicleState* out) {
    if (!eng || !out) return;
    std::memset(out, 0, sizeof(*out));
    const auto st = eng->vehicle.getState();
    out->x = static_cast<float>(st.position.x);
    out->y = static_cast<float>(st.position.y);
    out->z = static_cast<float>(st.position.z);
    out->speed_ms = static_cast<float>(st.speed);
    out->rpm = static_cast<float>(eng->vehicle.rpm());
    out->gear = eng->vehicle.currentGear();
    out->throttle = eng->input.throttle;
    out->brake = eng->input.brake;
    out->steer = eng->input.steer;
}

KSAPI int ks_engine_set_replay_path(KsEngine* eng, const char* path_utf8) {
    if (!eng) return 0;
    if (!path_utf8 || !path_utf8[0]) {
        if (eng->recording && !eng->replayPath.empty())
            eng->replay.save(eng->replayPath);
        eng->recording = false;
        eng->replayPath.clear();
        eng->replay.setRecording(false);
        return 1;
    }
    eng->replayPath = path_utf8;
    eng->replay.clear();
    eng->replay.setRecording(true);
    eng->recording = true;
    return 1;
}

} // extern "C"
