# Parity status — 2026-10-03 (full commit)

## Wired and on GitHub

| Feature | Files |
|---------|-------|
| FeatureHub | `FeatureHub.h` |
| Session modes | `SessionController.h`, menu, `beginSession` |
| Track limits | `TrackLimitsMonitor.h` |
| Weather / ToD | `WeatherControl.h` |
| LAN discovery | `ServerDiscovery.h` :20779 |
| Control API | `ExternalControlApi.h` :20780 |
| Setup | `SetupFile.h`, `ApplySetup.h` |
| Replay | `ReplayRecorder` + `loadReplayFile` |
| PB | `PersonalBestStore.h` |
| Layouts | `TrackLayout.h` |
| Pit stack | GarageExit, PitLaneQueue/Collision/Repair |

## SimulationLoop / App

- `beginSession(GameSessionMode)`
- `startFeatureServices` / `features()`
- `loadReplayFile`
- Menu PRACTICE / QUICK RACE / TIME ATTACK
- Env: `KS_REPLAY_FILE`, `KS_GOLDEN_CSV`, `KS_AI_CARS`

## Commit tip

See latest master commits for FeatureHub + menu + app wiring.
