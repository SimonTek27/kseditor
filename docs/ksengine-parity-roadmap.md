# ksengine — Roadmap Parity (multi-target)

**Aggiornato:** 2026-09-30

Target: **AC** feeling/telemetria · **rF2** piattaforma · **GD** libreria  
Identità prodotto: **ksim** (formati AC solo in `adapters/`).

---

## 0. Stato attuale

| Area | Stato |
| --- | --- |
| Loop 1 kHz + ECS | OK |
| Fisica veicolo | OK |
| FFB / input | OK |
| Shared memory | **DONE** |
| UDP `:20777` | **DONE** |
| TCP `:20778` | **DONE** |
| Lap / settori | **DONE** |
| Race HUD | **DONE** |
| C API headless | **DONE** |
| Replay binary | **DONE** |
| **Determinismo test** | **DONE** (`test_determinism`) |
| **Golden harness** | Scaffold + CTest (`test_PhysicsGolden`) |
| **RaceSession flags/penalties** | Scaffold (`RaceSession.h`) |
| Qt-free engine+sim | **0 hit Qt** |
| Audio bank | Stub |
| Multiplayer maturo | TODO |

---

## 1. Fondamenta — P0

| # | Milestone | Status |
| --- | --- | --- |
| 1.1 | Validazione fisica vs dati reali | **Scaffold** — CSV sintetico; target corr > 0.95 con giro reale |
| 1.2 | Shared-memory | **DONE** |
| 1.3 | Headless C API | **DONE** |
| 1.4 | Determinismo & replay | **DONE** test dual-run; replay via C API |

### CTest parity

```bash
ctest -R "test_determinism|test_PhysicsGolden" --output-on-failure
```

---

## 2. AC-like — P1

| # | Milestone | Status |
| --- | --- | --- |
| 2.1 | Renderer validato | TODO |
| 2.2 | Loader contenuti | Parziale |
| 2.3 | Feeling correlato | Bloccato da 1.1 reale |
| 2.4 | Sessioni (flag, penalità) | **Scaffold** `RaceSession` — da cablare in SimulationLoop |
| 2.5 | Audio bank | Stub |

## 3. rF2-like — P2

| # | Status |
| --- | --- |
| 3.1 Multiplayer maturo | TODO |
| 3.2 Dedicated server | Sbloccato (C API) |
| 3.3 Tooling pista | TODO |
| 3.4 Mod SDK Lua | TODO |
| 3.5 AI gara | TODO |

## 4. Godot — P3

| # | Status |
| --- | --- |
| 4.1 GDExtension | API pronta |
| 4.2 Headless no Vulkan | TODO flag |
| 4.3 Scene bridge | TODO |

---

## Prossimi passi

1. Cablare `RaceSession` in `SimulationLoop` (sostituire PHASE_* grezzi)
2. Export telemetria reale → `tests/data/golden_lap.csv` → corr > 0.95
3. Dedicated server binary su C API
4. Flag yellow/SC → riduci velocità AI + SM `flag` channel
