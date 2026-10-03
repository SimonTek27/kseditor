# Parity status — 2026-10-03

## Wired on GitHub (runtime)

| Feature | Status |
|---------|--------|
| FeatureHub + session modes | Done |
| Garage exit / pit queue / collision / repair | Done |
| AI multi-car + garage spawn | Done |
| Track pit boxes + snap to pose | Done |
| **Post-snap physics hold (0.35s)** | Done |
| Damage HUD + power scale | Done |
| SM / UDP / TCP telemetry | Done |
| **SimulatorServer headless** | Done |
| **FeatureMethods + Telemetry in SimulatorApp CMake** | Done |

## Tick order

```
applyInput (blocks during snap hold)
vehicle + multiCar physics
m_features.tick
updatePitLane (decays snap hold, freezes vel)
updateGarageExit (no leave while hold)
updatePitRepair
publish SM/UDP/TCP
render HUD
```

## SimulatorServer

```
SimulatorServer [--announce] [--track DIR] [--ai N] [--name NAME]
```

Discovery UDP :20779, control TCP :20780.

## Still open

| Item | Notes |
|------|-------|
| Full dedicated multiplayer server | beyond FeatureHub host |
| Integrate hold with vehicle integrate() pause flag | optional |
