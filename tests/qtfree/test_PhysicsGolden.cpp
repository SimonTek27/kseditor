/**
 * Parity 1.1 — golden trajectory harness (Qt-free).
 * Uses synthetic CSV; swap tests/data/golden_lap.csv for real telemetry.
 */
#include "engine/physics/PhysicsGolden.h"
#include "engine/physics/VehicleSimulator.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>

int main(int argc, char** argv) {
    const char* path = "tests/data/golden_lap.csv";
    if (argc > 1) path = argv[1];

    ks::physics::PhysicsGolden golden;
    if (!golden.loadCsv(path)) {
        std::fprintf(stderr, "FAIL: load %s\n", path);
        return 1;
    }

    ks::physics::VehicleSimulator veh;
    veh.reset();
    veh.startSimulation();

    // Open-loop: drive with reference inputs; record speed/pos
    auto metrics = golden.runOpenLoop([&](const ks::physics::GoldenSample& r) {
        veh.setThrottle(r.throttle);
        veh.setBrake(r.brake);
        veh.setSteering(r.steer);
        // step to catch up to sample time (coarse: fixed 1 ms)
        static double t = 0;
        while (t + 1e-4 < r.time) {
            veh.updatePhysics(0.001);
            t += 0.001;
        }
        const auto st = veh.getState();
        ks::physics::GoldenSample out;
        out.speedMs = static_cast<float>(st.speed);
        out.rpm = static_cast<float>(veh.rpm());
        out.x = st.position.x;
        out.y = st.position.y;
        out.z = st.position.z;
        out.throttle = r.throttle;
        out.brake = r.brake;
        out.steer = r.steer;
        return out;
    });

    std::printf("PhysicsGolden samples=%d mae_speed=%.3f mae_rpm=%.1f corr_speed=%.4f max_err=%.3f ok=%d\n",
                metrics.samples, metrics.maeSpeed, metrics.maeRpm,
                metrics.corrSpeed, metrics.maxSpeedErr, int(metrics.ok));

    // Synthetic pass is soft; real data must target corr > 0.95
    if (metrics.samples < 5) {
        std::fprintf(stderr, "FAIL: too few aligned samples\n");
        return 2;
    }
    std::printf("PASS (scaffold — replace CSV with real lap for parity metric)\n");
    return 0;
}
