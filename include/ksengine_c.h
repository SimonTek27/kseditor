/**
 * ksengine C API — headless / GDExtension / dedicated server entry points.
 * Qt-free. ABI intended to stay stable across minor versions.
 */
#ifndef KSENGINE_C_H
#define KSENGINE_C_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#if defined(_WIN32) && defined(KSENGINE_DLL)
#  ifdef KSENGINE_BUILD
#    define KSAPI __declspec(dllexport)
#  else
#    define KSAPI __declspec(dllimport)
#  endif
#else
#  define KSAPI
#endif

typedef struct KsEngine KsEngine;

typedef struct KsVehicleState {
    float x, y, z;
    float yaw, pitch, roll;
    float speed_ms;
    float rpm;
    int32_t gear;
    float throttle, brake, steer;
    float fuel_l;
    float tyre_temp_fl, tyre_temp_fr, tyre_temp_rl, tyre_temp_rr;
} KsVehicleState;

typedef struct KsInput {
    float throttle; /* 0..1 */
    float brake;
    float steer;    /* -1..1 */
    float clutch;
    int32_t gear_request; /* 0 = none */
} KsInput;

/** Create engine instance. seed=0 → time-based; non-zero → deterministic. */
KSAPI KsEngine* ks_engine_create(uint32_t seed);

KSAPI void ks_engine_destroy(KsEngine* eng);

/** Fixed physics step in seconds (default 0.001). */
KSAPI void ks_engine_set_timestep(KsEngine* eng, double dt);

KSAPI void ks_engine_reset(KsEngine* eng);

/** Apply driver input for subsequent steps. */
KSAPI void ks_engine_set_input(KsEngine* eng, const KsInput* in);

/** Advance simulation by dt seconds (may subdivide into fixed steps). */
KSAPI void ks_engine_step(KsEngine* eng, double dt);

/** Read primary vehicle state. */
KSAPI void ks_engine_get_state(const KsEngine* eng, KsVehicleState* out);

/** Optional: enable binary replay recording to path (empty = stop). */
KSAPI int ks_engine_set_replay_path(KsEngine* eng, const char* path_utf8);

/** Version string "major.minor.patch". */
KSAPI const char* ks_engine_version(void);

#ifdef __cplusplus
}
#endif

#endif /* KSENGINE_C_H */
