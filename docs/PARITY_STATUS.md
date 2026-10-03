# Parity status — 2026-10-03

## Wired on GitHub (runtime)

| Feature | Status | Notes |
|---------|--------|-------|
| FeatureHub | Done | session, discovery, control API, limits, weather, PB, setup, replay |
| Session modes | Done | Practice / Qualify / Race / Time Attack |
| LAN discovery | Done | UDP `:20779` |
| Control API | Done | TCP `:20780` |
| Track limits / Weather / Setup / Replay / PB | Done | |
| Garage exit + Pit queue + Collision + Repair | Done | full tick stack |
| Garage layout default | Done | linear row + bindBox |
| **AI spawn into garage boxes** | Done | `spawnAiGrid` on beginSession |
| **Damage → HUD** | Done | RaceHudSample damageOverall / engineHealth / warning |
| **Damage → power scale** | Done | `applyDamageEffects` → setEnginePower |

## Tick order

```
applyInput → physics
updateLapAndSurface
m_features.tick
updatePitLane (+ AI bodies)
updateGarageExit
updatePitRepair (+ applyDamageEffects)
render → pushRaceSample (damage strip)
```

## Still open

| Item | Notes |
|------|-------|
| Track-supplied pit boxes | Load poses from track data instead of linear row |
| Vehicle teleport to box | Physics API to snap position on spawn |
| Full SM/UDP re-expand | Optional telemetry bridges in simplified cpp |
| Root CMake include | `-DKSIMULATOR_QT_FREE=ON` + include cmake file |
