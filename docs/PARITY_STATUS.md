# Parity status — 2026-10-03

## Wired on GitHub (runtime)

| Feature | Status | Notes |
|---------|--------|-------|
| FeatureHub | Done | session, discovery, control API, limits, weather, PB, setup, replay |
| Session modes | Done | Practice / Qualify / Race / Time Attack via menu → `beginSession` |
| LAN discovery | Done | UDP `:20779` |
| Control API | Done | TCP `:20780` |
| Track limits | Done | `TrackLimitsMonitor` |
| Weather / ToD | Done | `WeatherControl` + tick sync |
| Setup load/save | Done | FeatureHub callbacks |
| Replay load | Done | `loadReplayFile` |
| Personal bests | Done | `onLapCompleted` |
| **Garage exit** | Done | InGarage → OnTrack state machine |
| **Pit queue** | Done | spacing, ClearedToMove, multi-car feed |
| **Pit collision** | Done | OBB + corridor, soft impulse |
| **Pit repair** | Done | jobs from DamageSystem, Stop-Go auto-request |
| **Garage layout** | Done | default linear row + `configurePitAxis` / `bindBox` |

## SimulationLoop tick order

```
applyInput → vehicle physics
updateLapAndSurface (green)
m_features.tick
updatePitLane          // player + AI bodies
updateGarageExit       // pathBlocked: queue | contact | service
updatePitRepair        // InGarage stationary service
```

## Build

- `CMakeLists_ksimulator_QtFree.cmake` includes `SimulationLoop_FeatureMethods.cpp`
- Pit stack headers are header-only under `src/simulator/`

## Still open / optional

| Item | Notes |
|------|-------|
| Real track pit boxes | Replace `setupDefaultGarageLayout` with track-supplied poses |
| Shared-memory / UDP publish | Re-expand from simplified SimulationLoop.cpp if needed |
| AI grid spawn into garage boxes | Assign `GarageLayout` indices per AI car |
| Mechanical damage → HUD | Telemetry channels for zone damage |
| CMake root hook | `include(CMakeLists_ksimulator_QtFree.cmake)` when flag ON |

## Env

| Variable | Purpose |
|----------|---------|
| `KS_REPLAY_FILE` | Menu load replay path |
| `KS_GOLDEN_CSV` | Golden telemetry export |
| `KS_AI_CARS` | AI grid size hint |
