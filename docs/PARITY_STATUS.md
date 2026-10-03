# Parity status — 2026-10-03 (wiring complete)

| Feature | Status |
|---------|--------|
| Session modes | **WIRED** menu → beginSession |
| Track limits | **WIRED** FeatureHub::tick |
| Weather / ToD | **WIRED** |
| LAN discovery :20779 | **WIRED** |
| Control API :20780 | **WIRED** |
| Setup load/save | **WIRED** |
| Replay load | **WIRED** KS_REPLAY_FILE |
| PB store | READY |
| Multi layout | READY |

## API

- `SimulationLoop::beginSession(GameSessionMode)`
- `SimulationLoop::startFeatureServices(bool)`
- `SimulationLoop::features()`
- `GameMenuOverlay::onStartSessionRequested`
- Env: `KS_REPLAY_FILE`, `KS_GOLDEN_CSV`, `KS_AI_CARS`

See `docs/WIRING_DIFF.md` and `FeatureHub.h`.
