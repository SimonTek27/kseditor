# ksengine — Roadmap Parity (multi-target)

**Aggiornato:** 2026-09-29 — progresso P0 telemetria + C API + golden scaffold.

Target:

- **AC** = feeling / contenuti / telemetria / renderer
- **rF2** = multiplayer, dedicated server, tooling, modding
- **GD** = ksengine come libreria sotto host esterno

> Identità prodotto: **ksim**. Formati AC solo in `adapters/` — niente branding AC nell’UI.

---

## 0. Stato attuale

| Area | Stato | Evidenza |
| --- | --- | --- |
| Loop | 1 kHz fixed + render disaccoppiato | `SimulationLoop` |
| Fisica | Pacejka, aero, sosp, freni, wear, danno, meteo | `src/engine/physics/*` |
| Input / FFB | XInput + SDK volanti | `devices/simracing/*` |
| **Shared memory** | **DONE** — physics/graphics/static Win+POSIX | `AcSharedMemoryPublisher` |
| **UDP out** | **DONE** — `:20777` binary `KSIM` + JSON | `UdpTelemetryBridge` |
| **TCP out** | **DONE** — listen `:20778`, length-prefix | `TcpTelemetryBridge` |
| Lap / settori | **DONE** | `LapSectorTimer` |
| Race HUD | **DONE** Compact/Race/Engineer | `RaceTelemetryHud` |
| **C API headless** | **DONE** create/step/get_state/replay | `include/ksengine_c.h` |
| **Golden harness** | **Scaffold** synthetic CSV | `PhysicsGolden` + `tests/data/golden_lap.csv` |
| Replay | **DONE** binary frames | `ReplaySystem` via C API |
| Render Vulkan | Presente, TAA opt-in, no visual diff | shaders |
| UI nativa | NativeUiHub, garage/pit/menu cinematic | `src/simulator/ui/*` |
| Rete game | yojimbo base | `NetworkManager` |
| Audio bank | Stub FMOD | `BankParserBridge` |
| Qt-free | Engine + simulator app strict | `check_no_qt` |

---

## 1. Fondamenta — P0

| # | Milestone | Status | Note |
| --- | --- | --- | --- |
| 1.1 | Validazione fisica vs dati reali | **In corso** | `PhysicsGolden` + CSV sintetico. Sostituire con export AC/rF2; target **corr speed > 0.95** |
| 1.2 | Shared-memory telemetria | **DONE** | Canali base: speed, rpm, gear, fuel, tyres, times, pos |
| 1.3 | Headless / C API | **DONE** | `ks_engine_create/step/get_state`; replay path |
| 1.4 | Determinismo & replay | **Parziale** | `ReplaySystem` record/load; manca seed RNG globale + bit-exact assert test |

### Telemetria canali (parity AC metric)

| Canale | SM | UDP | TCP |
|--------|----|-----|-----|
| speed / rpm / gear / fuel | ✓ | ✓ | ✓ |
| tyres temp/wear | ✓ | ✓ | ✓ |
| lap times / sector | ✓ | ✓ | ✓ |
| position / spline | ✓ | ✓ | ✓ |
| FFB torque | ✓ | — | — |

---

## 2. AC-like — P1

| # | Milestone | Status |
| --- | --- | --- |
| 2.1 | Renderer scena validato (screenshot diff) | TODO |
| 2.2 | Loader contenuti (kn5 / formato proprio) | Parziale TrackLoader |
| 2.3 | Feeling correlato (dopo 1.1 reale) | TODO |
| 2.4 | Sessioni (quali, bandiere, penalità) | Parziale countdown/green |
| 2.5 | Audio bank non-stub | TODO |

## 3. rF2-like — P2

| # | Milestone | Status |
| --- | --- | --- |
| 3.1 | Multiplayer maturo (lag comp, list) | TODO |
| 3.2 | Server dedicato headless | Sbloccato da 1.3 |
| 3.3 | Tooling pista / AI line | TODO |
| 3.4 | Mod SDK Lua | TODO |
| 3.5 | AI gara completa | TODO |

## 4. Godot-hosted — P3

| # | Milestone | Status |
| --- | --- | --- |
| 4.1 | GDExtension su C API | TODO (API pronta) |
| 4.2 | Headless senza Vulkan | TODO flag |
| 4.3 | Bridge scene tree | TODO |

---

## Prossimi passi operativi

1. **1.1** — Registrare un giro reale → CSV → `test_PhysicsGolden` → corr > 0.95  
2. **1.4** — Test determinismo: stesso seed → stessi frame hash  
3. **Wire TCP** in `SimulationLoop::tick` (come UDP)  
4. **2.4** — Flag/penalties in session manager  
5. **3.2** — Binary dedicated server linkando solo `ksengine` C API  

## Metriche

| Target | Metrica | Soglia |
| --- | --- | --- |
| AC | corr speed vs telemetria | > 0.95 |
| AC | shared-memory channels | set base ✓ |
| rF2 | multiplayer | 8+ client + dedicated |
| GD | GDExtension guida veicolo | C API ✓ base |
