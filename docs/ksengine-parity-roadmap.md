# ksengine — Roadmap Parity (multi-target)

**Stato verificato:** 2026-09-29, su `src/engine` + `src/simulator` (build Qt-free verde, `check_no_qt.ps1 -Strict` → 0/801).

La roadmap è organizzata per **aree**; ogni area indica a quali target serve:

- **AC** = Assetto Corsa-like (feeling, contenuti, telemetria condivisa, renderer "production")
- **rF2** = rFactor 2-like (piattaforma motorsport: multiplayer, dedicated server, tooling pista, modding)
- **GD** = Godot-hosted (ksengine come libreria di simulazione sotto un host esterno)

---

## 0. Stato attuale (evidenza)

| Area | Stato | Evidenza |
| --- | --- | --- |
| Loop | Fixed timestep **1 kHz** con accumulator, render disaccoppiato; ECS a 120 Hz | `src/simulator/SimulationLoop.h:189` (`m_physicsDt = 0.001`), `SimulationLoop.cpp:432` |
| Fisica veicolo | Pacejka, aero, diff, sospensione, engine, brake thermal/wear, tire wear/flatspot, danno, ERS/DRS, ibrido, meteo | `src/engine/physics/*` |
| Input / FFB | InputManager + XInput; FFB con SDK reali (Fanatec, Logitech, Moza, Simucube, Thrustmaster) + condition effects + HIL | `src/engine/devices/simracing/*`, `src/simulator/SimulationLoop.cpp:442-457` |
| Telemetria | CSV (`TelemetryPhysics`), overlay UI, listener UDP; **no shared-memory** | `src/engine/physics/TelemetryPhysics.h`, `src/simulator/UdpTelemetryListener.*` |
| Render | Vulkan reale: GBuffer → lighting (PBR CSM + volumetrici) → resolve TAA; TAA **opt-in** (nessuna verifica visiva) | `docs/QT_FREE_STATUS.md:109-129`, `src/simulator/shaders/*` |
| UI nativa | NativeUiHub + UiRenderer + UiGpuPass Vulkan + font atlas; HUD, menu, garage, overlay | `src/simulator/ui/*` |
| Rete | yojimbo (client/server), chat, collaborazione, multi-auto; maturità base | `src/simulator/NetworkManager.h` (`HAS_YOJIMBO`) |
| Audio | WASAPI, mixer, track/car audio, parser INI suoni; bank FMOD ancora **stub** (`valid=false`) | `src/simulator/SimulatorAudio.*`, `src/engine/Audio/BankParserBridge` |
| Adapter AC | Import/export FSPRO, parser CSP config, GUIDs | `src/adapters/assetto_corsa/*` |
| Scripting | Lua 5.4.8 vendored, `ScriptModule` nei sistemi ECS | `src/engine/Scripting/*` |
| Test | 7 test Qt-free (CTest) + test unit Qt; nessun test di validazione fisica vs dati reali | `tests/qtfree/CMakeLists.txt` |

---

## 1. Fondamenta (tutti i target) — P0

Base senza la quale nessun target è raggiungibile.

| # | Milestone | Target | Note |
| --- | --- | --- | --- |
| 1.1 | **Validazione fisica vs dati reali** | AC, rF2 | Golden test: giri registrati (es. da telemetria AC/rF2) → confronto velocità/ giri / slip. Esiste già `test_LapTimeValidation` da estendere. |
| 1.2 | **Telemetria shared-memory** | AC | Stile AC: mappatura memoria condivisa con canali (velocità, giri, gomme, fuel, danno). Oggi solo CSV + UDP. |
| 1.3 | **Headless / libreria** | GD, rF2 | Target `ksengine` già separato; serve API C stabile + modalità senza window per server dedicato e GDExtension. |
| 1.4 | **Determinismo & replay** | tutti | Fisso 1 kHz + seed → replay bit-exact. Esiste `ReplaySystem` da collegare al loop. |

## 2. AC-like — feeling, contenuti, renderer — P1

| # | Milestone | Gap oggi |
| --- | --- | --- |
| 2.1 | **Renderer scena validato** | TAA/deferred compila ma nessuna verifica visiva; serve pipeline "production" con riferimenti (screenshot diff). |
| 2.2 | **Loader contenuti** | `TrackLoader` esiste; manca caricamento asset AC (kn5) via adapter, o formato proprio documentato. |
| 2.3 | **Feeling / correlazione pista** | Pacejka presente ma non validato; serve tuning su dati reali (1.1) + FFB condition effects estese (kerb, rumble, ABS). |
| 2.4 | **Sessioni gara** | `RaceSessionManager` esiste; mancano giri di qualifica, safety car, penalità, gestione bandiere. |
| 2.5 | **Audio banca completa** | `BankParserBridge` è stub (`valid=false`); serve reader FMOD bank o formato proprio. |

## 3. rF2-like — piattaforma motorsport — P2

| # | Milestone | Gap oggi |
| --- | --- | --- |
| 3.1 | **Multiplayer maturo** | yojimbo base presente; mancano lag compensation, gestione sessioni, lista server, NAT. |
| 3.2 | **Server dedicato** | Richiede 1.3 (headless); poi build senza render/UI. |
| 3.3 | **Tooling pista** | Validazione pista (larghezza, AI line, superfici); esiste `AiFileReader` (binary AC) da riusare. |
| 3.4 | **Modding / SDK** | Formato asset documentato + API scripting Lua esposta ai mod (oggi Lua è interno). |
| 3.5 | **AI di gara** | `AIDriver` (fisica) + `AIController` (simulator) esistono; manca gara AI completa (sorpassi, pit, strategia). |

## 4. Godot-hosted — ksengine come libreria — P3

| # | Milestone | Gap oggi |
| --- | --- | --- |
| 4.1 | **GDExtension C++** | Wrapper C API su `SimulationLoop` + `VehicleSimulator`; nessun binding oggi. |
| 4.2 | **Headless puro** | Server/headless senza Vulkan: render opzionale (oggi `NativeRenderer` è Vulkan-only). |
| 4.3 | **Bridge scene tree** | Sincronizzazione stato veicoli → nodi Godot (la direzione è inversa a `syncCarTransforms`). |

---

## Sequenza consigliata

1. **Fondamenta (1.1 → 1.3)** — validazione fisica prima di tutto: senza correlazione pista nessun "parity" è misurabile.
2. **AC-like (2.x)** — massimo valore percepito: renderer validato + telemetria + feeling.
3. **rF2-like (3.x)** — sbloccato da 1.3 (headless) e 3.1 (rete).
4. **GD-hosted (4.x)** — sbloccato da 1.3; ultimo perché è un packaging, non una capability.

## Metriche di parity

| Target | Metrica | Soglia "parity" |
| --- | --- | --- |
| AC | Correlazione giri vs telemetria reale | > 0.95 su circuito di riferimento |
| AC | Canali shared-memory | ≥ set base AC (velocità, giri, gomme, fuel, danno) |
| rF2 | Multiplayer | 8+ client, lag comp, server dedicato headless |
| rF2 | Frequenza sim | 1 kHz validato, determinismo replay |
| GD | Host esterno | GDExtension carica e guida un veicolo senza codice Godot custom |
