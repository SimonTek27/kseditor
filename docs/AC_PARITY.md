# Roadmap: chiudere il gap con Assetto Corsa

Obiettivo: runtime open con **compatibilità contenuti/apps AC**, non clonare il binary Kunos.

## Già in repo
| Area | Stato |
|------|--------|
| Pacejka + aero + suspension + diff | Presente |
| TrackSurface (grip/wet/rubber) | Presente |
| Fixed timestep ~1 ms | SimulationLoop |
| FFB bridge | Presente |
| AC INI load (tyres/engine/aero/…) | VehicleSimulator |
| UDP telemetry listener | Presente |
| **Shared memory publisher AC-like** | **Nuovo** |
| **surfaces.ini loader** | **Nuovo** |
| **Lap/sector timer** | **Nuovo** |

## Priorità prossimi step

### P0 — Feeling / dati
1. Validare Pacejka su auto AC note (setup baseline)
2. Rubber banking + temperature tyre core (già campi SM)
3. Surface per-wheel da mesh collision (oggi griglia)

### P1 — Contenuti
1. KN5 mesh load → NativeRenderer
2. AI line / fast_lane.ai → AIDriver
3. car.ini completo (BALLAST, FUEL, …)

### P2 — Sessione
1. Practice / Qualify / Race phases come AC
2. Pit limiter + penalty flags in graphics page
3. Multiplayer state → shared memory numCars

### P3 — Grafica (non CSP-in-engine)
1. Scene pass Vulkan (track + car body)
2. Adapter CSP solo in `adapters/assetto_corsa`

## Non fare
- Portare CSP dentro `src/engine`
- Dipendere da Qt nel path physics/sim
- Rompere layout shared memory senza bump versione

## Integrazione loop
```cpp
// SimulationLoop::tick after physics:
AcLiveInput live = buildFrom(vehicle, lapTimer, track);
acShm.publish(live);
```
