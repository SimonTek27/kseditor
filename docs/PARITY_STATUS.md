# Parity status — 2026-10-03

## Wired on GitHub (runtime)

| Feature | Status | Notes |
|---------|--------|-------|
| FeatureHub | Done | session, discovery, API, limits, weather, PB, setup, replay |
| Session modes | Done | menu → beginSession |
| Garage exit + Pit queue/collision/repair | Done | full tick stack |
| AI multi-car in pit | Done | bodies in queue + OBB |
| AI spawn into garage boxes | Done | spawnAiGrid |
| Damage → HUD + power scale | Done | RaceHudSample + setEnginePower |
| **Track pit boxes** | Done | `loadGarageFromTrack` (pit_boxes.ini / garage.ini) |
| **Snap to box** | Done | `snapVehicleToPose` via SimulationState |
| **SM / UDP / TCP telemetry** | Done | publish restored in tick |

## Tick order

```
applyInput → vehicle + multiCar physics
updateLapAndSurface
m_features.tick
updatePitLane / updateGarageExit / updatePitRepair
publishSharedMemory + UDP + TCP
render (damage HUD)
```

## Track pit file format (optional)

```
[BOX_0]
X=10.0
Y=0.0
Z=-5.0
HEADING=1.57
PIT_HEADING=1.57

[BOX_1]
...
```

Paths tried: `data/pit_boxes.ini`, `pit_boxes.ini`, `data/garage.ini`, `garage.ini`, `data/pits.ini`

## Still open

| Item | Notes |
|------|-------|
| Full physics teleport stability | may need integrate-hold after snap |
| AI spline load on track load | MultiCarManager::loadAiSpline |
| Dedicated server executable | SimulatorServerApp |
