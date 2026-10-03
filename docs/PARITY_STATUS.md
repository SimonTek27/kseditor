# Parity status — 2026-10-03

## Wired on GitHub (runtime)

| Feature | Status |
|---------|--------|
| FeatureHub + session modes | Done |
| Garage / pit stack + AI | Done |
| Track pit boxes + snap | Done |
| **Vehicle setFrozen (integrate pause)** | Done |
| Post-snap hold (0.35s + frozen) | Done |
| Damage HUD + telemetry SM/UDP/TCP | Done |
| SimulatorServer headless | Done |
| **Server --game-port (NetworkManager)** | Done (HAS_KSNET) |

## SimulatorServer

```
SimulatorServer [--announce] [--track DIR] [--ai N] [--name NAME] [--game-port 40000]
```

| Port | Service |
|------|---------|
| UDP 20779 | LAN discovery |
| TCP 20780 | External control API |
| --game-port | Multiplayer host (when HAS_KSNET) |

## Still open

| Item | Notes |
|------|-------|
| Full state sync for remote cars | needs HAS_KSNET + CarState broadcast in tick |
| Restore optional CMake deps (ImGui/Python) | if needed for editor builds |
