# Parity status snapshot — 2026-10-03

Source zip integrated + pit repair stack completed.

## Closed (engine / sim)

| Area | Status |
|------|--------|
| Qt-free engine+sim | 0 Qt hits |
| Shared memory / UDP 20777 / TCP 20778 | DONE |
| Lap/sectors, Race HUD, C API, determinism | DONE |
| Audio bank (RIFF/FEV/FSB5) | DONE |
| Renderer validation | DONE |
| Multiplayer + dedicated server | DONE |
| Track tooling + Mod SDK Lua + Race AI | DONE |
| Headless + scene bridge | DONE |
| Damage RF2 + mechanical wear | DONE |
| Garage spawn / exit FSM | DONE |
| Pit queue + collisions | DONE |
| **Pit lane repair** | **DONE** — `PitLaneRepair.h` |
| Setup save/load file | **DONE** — `SetupFile.h` (wire UI still open) |
| Upgrades + race components + livery/sound | DONE |

## Open — LFS-like (section 5)

| # | Item | Priority |
|---|------|----------|
| 5.1 | Server browser / LAN discovery | P0 |
| 5.2 | Bidirectional control API (InSim-style) | P0 |
| 5.3 | Session modes not hardcoded race-only | P1 |
| 5.4 | Track limits → penalties wiring | P1 |
| 5.5 | Setup → physics + network share | P1 (file I/O done) |
| 5.6 | Replay playback | P1 |
| 5.7 | Multi layout tracks | P1 |
| 5.8 | Weather/time UI | P1 |
| 5.9–5.11 | PB persist, championships, docs | P2 |

## Physics validation

| 1.1 Golden vs real lap | Scaffold — need real CSV (corr target > 0.95) |
| 2.3 Related feel | Blocked by 1.1 |

## Pit repair quick ref

```cpp
PitLaneRepair repair;
auto out = repair.update(dt, in, damage, GarageExitPhase::InGarage);
vehicle.setFuel(out.fuelL);
```

See `docs/PIT_LANE_REPAIR.md`, `docs/ksengine-parity-roadmap.md`.
