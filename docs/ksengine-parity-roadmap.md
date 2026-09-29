# ksengine — Roadmap Parity (multi-target)

**Updated:** 2026-09-29

Targets: **AC-like** | **rF2-like** | **GD-hosted** (library under external host).

Product identity: **ksim** is independent; format interoperability is technical only.

---

## 0. Stato attuale

| Area | Stato |
| --- | --- |
| Loop | Fixed **1 kHz**, render decoupled |
| Fisica | Pacejka, aero, diff, sospensioni, thermal, wear, danno, ERS/DRS, meteo |
| FFB | Fanatec / Logitech / Moza / Simucube / Thrustmaster |
| Telemetria | CSV, UDP, HUD; **shared-memory publisher** (`adapters` / `AcSharedMemory*`) |
| UI | Menu cinematico, Garage, Pit Lane, Race HUD |
| Rete | yojimbo base |
| Qt-free | engine + simulator strict clean |

---

## 1. Fondamenta (P0) — tutti i target

| # | Milestone | Stato |
| --- | --- | --- |
| 1.1 | Validazione fisica vs dati (golden) | **Harness** `PhysicsGolden` + doc — serve dataset reale |
| 1.2 | Shared-memory telemetria | **Done** (publisher layout + pages) — wire in SimulationLoop residuale |
| 1.3 | Headless / C API | **Done** `include/ksengine_c.h` + stub runtime |
| 1.4 | Determinismo & replay | ReplaySystem presente; **seed** in C API + frame record |

### Sequenza operativa
1. Collegare SM + LapSectorTimer in `SimulationLoop::tick`
2. Riempire golden CSV da un giro registrato
3. Server headless con solo `ksengine_c` (no Vulkan)

---

## 2. AC-like (P1)

| # | Milestone | Gap |
| --- | --- | --- |
| 2.1 | Renderer scena validato | Screenshot / diff |
| 2.2 | Loader contenuti | kn5 / formato proprio |
| 2.3 | Feeling pista | Tuning post-1.1 + FFB kerb/rumble |
| 2.4 | Sessioni | Quali, bandiere, penalità |
| 2.5 | Audio bank | FMOD reader oltre stub |

## 3. rF2-like (P2)

Multiplayer maturo, dedicated server (usa 1.3), tooling pista, SDK mod, AI gara.

## 4. GD-hosted (P3)

GDExtension su C API (1.3), headless senza Vulkan, bridge scene tree.

---

## Metriche parity

| Target | Metrica | Soglia |
| --- | --- | --- |
| AC-like | Correlazione giri vs telemetria | > 0.95 |
| AC-like | Canali SM | speed, rpm, tyres, fuel, damage |
| rF2-like | MP | 8+ client, dedicated headless |
| GD | Extension | guida veicolo senza codice host custom |
