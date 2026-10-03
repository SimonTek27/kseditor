# Parity status — 2026-10-03 (feature completion pass)

## Closed this pass (LFS section 5)

| # | Feature | Module |
|---|---------|--------|
| 5.1 | LAN server discovery | `ServerDiscovery.h` (UDP :20779) |
| 5.2 | External control API | `ExternalControlApi.h` (TCP :20780) |
| 5.3 | Session modes | `SessionController.h` |
| 5.4 | Track limits → penalties | `TrackLimitsMonitor.h` |
| 5.5 | Setup file + apply | `SetupFile.h` + `ApplySetup.h` |
| 5.6 | Replay load/play | `ReplayRecorder` (wire UI) |
| 5.7 | Multi layouts | `TrackLayout.h` |
| 5.8 | Weather / time-of-day | `WeatherControl.h` |
| 5.9 | Persistent PB | `PersonalBestStore.h` |

## Pit / garage / damage

PitLaneRepair, GarageExit, PitLaneQueue, PitLaneCollision, MechanicalDamage — DONE

## Needs SimulationLoop / UI wiring

- Menu → SessionController
- tick → TrackLimitsMonitor + WeatherControl
- MultiplayerOverlay ← ServerDiscovery
- Control API handlers
- Replay menu → loadReplay/startPlayback
- Real golden CSV (1.1)

Repo: SimonTek27/kseditor
