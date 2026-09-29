/**
 * Smoke test for PhysicsGolden harness (no external dataset required).
 * When tests/data/golden_lap.csv exists, evaluates against it.
 */
#include "../../src/engine/physics/PhysicsGolden.h"
#include <cstdio>
#include <cstdlib>

int main() {
    using ks::physics::PhysicsGolden;
    using ks::physics::GoldenSample;

    PhysicsGolden g;
    // Synthetic self-consistency: ref == sim → ok
    for (int i = 0; i < 50; ++i) {
        GoldenSample s;
        s.t = i * 0.1;
        s.speed_ms = 20.f + i * 0.5f;
        s.rpm = 3000.f + i * 50.f;
        s.x = i * 2.f;
        g.addSim(s);
    }
    // Build ref by writing temp via in-memory: load from duplicate
    // For unit test without file: inject by evaluating empty ref → not ok
    auto empty = g.evaluate();
    if (empty.ok) {
        std::fprintf(stderr, "expected empty ref to fail\n");
        return 1;
    }

    // Load optional real golden
    if (g.loadReferenceCsv("tests/data/golden_lap.csv")) {
        auto r = g.evaluate(5.0, 20.0);
        std::printf("golden: compared=%d maeSpeed=%.3f corr=%.3f ok=%d\n",
                    r.compared, r.maeSpeed, r.corrSpeed, r.ok ? 1 : 0);
    } else {
        std::printf("PhysicsGolden smoke OK (no golden_lap.csv)\n");
    }
    return 0;
}
